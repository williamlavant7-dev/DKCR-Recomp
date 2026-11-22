// Video recording: see recorder.h.
//
// Frames: the render thread copies each wanted window image into one of a few slots (a full queue drops the frame:
// recording never holds the game up); the encoder thread converts it (swscale, to the encoder's size and format) and
// stamps it with its frame number at the recording's frame rate, from the time it was shown - a frame that lands on
// a number already written is left out, a gap leaves the number out (the player shows the last frame longer).
// Sound: each audio source goes through an SDL audio stream that turns it into 48 kHz stereo float; the encoder thread
// takes what the wall clock says is due (100 ms behind, for the sources' jitter), mixes the sources and encodes AAC.
#include "recorder.h"

#include <SDL3/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform.h"
#include "rt.h"
#include "settings.h"

#define NSLOTS 16  // (about a quarter second of frames: the encoder catching up after a slow moment)
#define AUDIO_RATE 48000
#define AUDIO_SOURCES 2
#define AUDIO_LAG_NS 100000000ull  // (the sources' own pacing jitter: sound is taken this far behind the clock)

typedef struct {
    uint8_t *px;
    uint32_t w, h, stride;
    bool bgr;
    uint64_t t_ns;
    _Atomic int state;  // 0 free, 1 being filled, 2 ready for the encoder
} Slot;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;  // (start / stop)
static pthread_t g_thread;
static bool g_joinable;  // (g_lock) g_thread to join: a thread that ended on its own - a failed setup - too
static _Atomic bool g_on, g_stop, g_starting;  // (starting: the encoder thread is setting the file up)
static Slot g_slots[NSLOTS];
static _Atomic uint32_t g_slot_next;  // (the producer's next slot, in order)
static char g_last_file[512], g_encoder_name[64];

static pthread_mutex_t g_audio_lock = PTHREAD_MUTEX_INITIALIZER;
static SDL_AudioStream *g_audio_in[AUDIO_SOURCES];
static int g_audio_spec[AUDIO_SOURCES][3];  // (fmt, channels, rate) the stream converts from
static bool g_audio_on;

// The encoder thread's own state
static AVFormatContext *oc;
static AVCodecContext *vctx, *actx;
static AVStream *vst, *ast;
static struct SwsContext *sws;
static AVFrame *vframe, *aframe;
static AVPacket *pkt;
static int fps;
static uint64_t t0;
static int64_t last_pts, audio_done;

bool recorder_active(void) { return atomic_load(&g_on) || atomic_load(&g_starting); }
const char *recorder_last_file(void) { return g_last_file; }
const char *recorder_encoder(void) { return g_encoder_name; }

// Every game frame (30 fps: every second one): the video is the game's frames one by one
bool recorder_wants_frame(void) {
    if (!atomic_load(&g_on) || atomic_load(&g_stop)) return false;
    static uint32_t n;
    return fps >= 60 || (n++ & 1) == 0;
}

void recorder_video_frame(const uint8_t *px, uint32_t w, uint32_t h, uint32_t stride, bool bgr, uint64_t t_ns) {
    if (!atomic_load(&g_on) || atomic_load(&g_stop) || !w || !h) return;
    uint32_t i = atomic_load(&g_slot_next) % NSLOTS;
    Slot *s = &g_slots[i];
    // (the encoder behind: wait for it rather than leave the frame out - up to 100 ms, then the frame is left out)
    for (int tries = 0; atomic_load(&s->state) != 0 && tries < 100; tries++) SDL_DelayNS(1000000);
    int free_ = 0;
    if (!atomic_compare_exchange_strong(&s->state, &free_, 1)) return;
    size_t need = (size_t)w * h * 4;
    if (!s->px || (size_t)s->w * s->h * 4 < need) {
        free(s->px);
        s->px = malloc(need);
        if (!s->px) { atomic_store(&s->state, 0); return; }
    }
    for (uint32_t y = 0; y < h; y++) memcpy(s->px + (size_t)y * w * 4, px + (size_t)y * stride, (size_t)w * 4);
    s->w = w, s->h = h, s->stride = w * 4, s->bgr = bgr, s->t_ns = t_ns;
    atomic_store(&s->state, 2);
    atomic_fetch_add(&g_slot_next, 1);
}

void recorder_audio(int source, const void *data, uint32_t bytes, int fmt, int channels, int rate) {
    if (!atomic_load(&g_on) || source < 0 || source >= AUDIO_SOURCES || !data || !bytes) return;
    pthread_mutex_lock(&g_audio_lock);
    if (g_audio_on) {
        int *sp = g_audio_spec[source];
        if (!g_audio_in[source] || sp[0] != fmt || sp[1] != channels || sp[2] != rate) {
            if (g_audio_in[source]) SDL_DestroyAudioStream(g_audio_in[source]);
            SDL_AudioSpec in = {fmt == REC_AUDIO_F32 ? SDL_AUDIO_F32 : SDL_AUDIO_S16, channels, rate};
            SDL_AudioSpec out = {SDL_AUDIO_F32, 2, AUDIO_RATE};
            g_audio_in[source] = SDL_CreateAudioStream(&in, &out);
            sp[0] = fmt, sp[1] = channels, sp[2] = rate;
        }
        if (g_audio_in[source]) SDL_PutAudioStreamData(g_audio_in[source], data, (int)bytes);
    }
    pthread_mutex_unlock(&g_audio_lock);
}

// ------------------------------------------------------------------ the encoder thread
static void write_packets(AVCodecContext *ctx, AVStream *st) {
    while (avcodec_receive_packet(ctx, pkt) == 0) {
        av_packet_rescale_ts(pkt, ctx->time_base, st->time_base);
        pkt->stream_index = st->index;
        av_interleaved_write_frame(oc, pkt);
    }
}

// Frame by frame: each frame is the one after the last (a constant frame rate, no frame left out). Only a real hitch -
// the frame shown more than a frame late by the clock - skips numbers, so the video holds the frame as the game did
// and the sound stays in step.
static void encode_slot(Slot *s) {
    double at = (double)(int64_t)(s->t_ns - t0) * fps / 1e9;
    int64_t pts = last_pts + 1;
    if (at > (double)pts + 1.5) pts = (int64_t)floor(at);
    {
        sws = sws_getCachedContext(sws, (int)s->w, (int)s->h, s->bgr ? AV_PIX_FMT_BGRA : AV_PIX_FMT_RGBA, vctx->width,
                                   vctx->height, vctx->pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);
        if (sws && av_frame_make_writable(vframe) >= 0) {
            const uint8_t *src[1] = {s->px};
            int src_stride[1] = {(int)s->stride};
            sws_scale(sws, src, src_stride, 0, (int)s->h, vframe->data, vframe->linesize);
            vframe->pts = pts;
            last_pts = pts;
            if (avcodec_send_frame(vctx, vframe) >= 0) write_packets(vctx, vst);
        }
    }
    atomic_store(&s->state, 0);
}

// Sound due by the clock (or, at the end, everything the sources still hold): 1024-sample AAC frames
static void encode_audio(bool all) {
    if (!actx) return;
    // (signed: within the first 100 ms nothing is due - unsigned, it wrapped to hours of silence and never returned)
    int64_t due = (int64_t)(((double)(int64_t)(plat_now_ns() - t0) - (double)AUDIO_LAG_NS) * AUDIO_RATE / 1e9);
    float mix[1024 * 2], part[1024 * 2];
    for (;;) {
        int n = actx->frame_size;
        if (!all && audio_done + n > due) break;
        memset(mix, 0, sizeof(float) * (size_t)n * 2);
        int got_max = 0;
        pthread_mutex_lock(&g_audio_lock);
        for (int s = 0; s < AUDIO_SOURCES; s++) {
            if (!g_audio_in[s]) continue;
            int got = SDL_GetAudioStreamData(g_audio_in[s], part, (int)(sizeof(float) * (size_t)n * 2)) / (int)(2 * sizeof(float));
            for (int i = 0; i < got * 2; i++) mix[i] += part[i];
            if (got > got_max) got_max = got;
        }
        pthread_mutex_unlock(&g_audio_lock);
        if (all && !got_max) break;  // (the end: nothing more to take)
        if (av_frame_make_writable(aframe) < 0) break;
        float *l = (float *)aframe->data[0], *r = (float *)aframe->data[1];
        for (int i = 0; i < n; i++) {
            l[i] = fminf(1.0f, fmaxf(-1.0f, mix[2 * i]));
            r[i] = fminf(1.0f, fmaxf(-1.0f, mix[2 * i + 1]));
        }
        aframe->pts = audio_done;
        audio_done += n;
        if (avcodec_send_frame(actx, aframe) >= 0) write_packets(actx, ast);
    }
}

static bool setup(void);
static void teardown(void);

// The encoder thread sets the file and encoders up itself: opening a GPU encoder took 1.7 s (NVENC), a hitch when
// it ran on the thread that pressed F8.
static void *encoder_main(void *arg) {
    (void)arg;
    if (!setup()) {
        teardown();
        atomic_store(&g_starting, false);
        return NULL;
    }
    t0 = plat_now_ns();
    last_pts = -1, audio_done = 0;
    pthread_mutex_lock(&g_audio_lock);
    g_audio_on = actx != NULL;
    pthread_mutex_unlock(&g_audio_lock);
    atomic_store(&g_slot_next, 0);
    atomic_store(&g_on, true);
    atomic_store(&g_starting, false);
    uint32_t next = 0;  // (slots are filled in order: taken in the same order)
    for (;;) {
        bool stopping = atomic_load(&g_stop);
        Slot *s = &g_slots[next % NSLOTS];
        if (atomic_load(&s->state) == 2) {
            encode_slot(s);
            next++;
            continue;
        }
        encode_audio(false);
        if (stopping) break;
        SDL_DelayNS(2000000);
    }
    encode_audio(true);
    avcodec_send_frame(vctx, NULL);
    write_packets(vctx, vst);
    if (actx) { avcodec_send_frame(actx, NULL); write_packets(actx, ast); }
    av_write_trailer(oc);
    avio_closep(&oc->pb);
    LOG("[rec] saved %s (%lld video frames)\n", g_last_file, (long long)last_pts + 1);
    return NULL;
}

// ------------------------------------------------------------------ setting up and finishing
static void teardown(void) {
    avcodec_free_context(&vctx);
    avcodec_free_context(&actx);
    if (oc) avformat_free_context(oc);
    oc = NULL;
    sws_freeContext(sws);
    sws = NULL;
    av_frame_free(&vframe);
    av_frame_free(&aframe);
    av_packet_free(&pkt);
    pthread_mutex_lock(&g_audio_lock);
    for (int s = 0; s < AUDIO_SOURCES; s++)
        if (g_audio_in[s]) SDL_DestroyAudioStream(g_audio_in[s]), g_audio_in[s] = NULL;
    g_audio_on = false;
    pthread_mutex_unlock(&g_audio_lock);
    for (int i = 0; i < NSLOTS; i++) { free(g_slots[i].px); g_slots[i].px = NULL; g_slots[i].w = g_slots[i].h = 0; atomic_store(&g_slots[i].state, 0); }
}

// The video encoder: a GPU's own first (no CPU cost), then Windows' Media Foundation, x264 where FFmpeg has it, and
// MPEG-4 part 2 (in every FFmpeg) last.
// Each codec's encoders in the order Auto tries them (Settings > Recording > Encoder: REC_ENC_*; the last is software)
static const char *const k_encoders[2][REC_ENC_N] = {
    {NULL, "h264_nvenc", "h264_amf", "h264_qsv", "h264_mf", "libx264", "mpeg4"},
    {NULL, "hevc_nvenc", "hevc_amf", "hevc_qsv", "hevc_mf", "libx265", "mpeg4"},
};

static AVCodecContext *open_video(int w, int h, int64_t bitrate, const AVCodec **used) {
    const char *force = getenv("LARECOMP_RECORD_ENCODER");  // (test: that encoder only, e.g. h264_mf as in the release)
    int codec = g_settings.rec_codec == 1, pick = g_settings.rec_encoder;
    if (pick < 0 || pick >= REC_ENC_N) pick = REC_ENC_AUTO;
    // the chosen encoder first, then Auto's order (a choice this PC can't use still records)
    const char *order[REC_ENC_N + 1];
    int n = 0;
    if (pick != REC_ENC_AUTO) order[n++] = k_encoders[codec][pick];
    for (int i = 1; i < REC_ENC_N; i++) if (i != pick) order[n++] = k_encoders[codec][i];
    for (int i = 0; i < n; i++) {
        if (force && *force && strcmp(force, order[i])) continue;
        const AVCodec *c = avcodec_find_encoder_by_name(order[i]);
        if (!c) continue;
        AVCodecContext *ctx = avcodec_alloc_context3(c);
        ctx->width = w, ctx->height = h;
        ctx->time_base = (AVRational){1, fps};
        ctx->framerate = (AVRational){fps, 1};
        ctx->bit_rate = bitrate;
        ctx->gop_size = fps * 2;
        ctx->max_b_frames = 0;
        ctx->color_range = AVCOL_RANGE_MPEG;
        ctx->colorspace = AVCOL_SPC_BT709, ctx->color_primaries = AVCOL_PRI_BT709, ctx->color_trc = AVCOL_TRC_BT709;
        const enum AVPixelFormat *fmts = NULL;
        int nf = 0;
        avcodec_get_supported_config(ctx, c, AV_CODEC_CONFIG_PIX_FORMAT, 0, (const void **)&fmts, &nf);
        ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        for (int k = 0; fmts && k < nf; k++) if (fmts[k] == AV_PIX_FMT_NV12) ctx->pix_fmt = AV_PIX_FMT_NV12;
        if (fmts && nf && ctx->pix_fmt != AV_PIX_FMT_NV12) {
            bool yuv = false;
            for (int k = 0; k < nf; k++) yuv |= fmts[k] == AV_PIX_FMT_YUV420P;
            if (!yuv) ctx->pix_fmt = fmts[0];
        }
        if (oc->oformat->flags & AVFMT_GLOBALHEADER) ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        if (strstr(c->name, "_nvenc")) av_opt_set(ctx->priv_data, "preset", "p4", 0);
        if (!strcmp(c->name, "libx264") || !strcmp(c->name, "libx265")) av_opt_set(ctx->priv_data, "preset", "veryfast", 0);
        if (avcodec_open2(ctx, c, NULL) == 0) { *used = c; return ctx; }
        LOG("[rec] encoder %s not usable here\n", c->name);
        avcodec_free_context(&ctx);
    }
    return NULL;
}

const char *recorder_dir(void) { return g_settings.rec_dir[0] ? g_settings.rec_dir : "recordings"; }

static bool setup(void) {
    const char *dir = recorder_dir();
    SDL_CreateDirectory(dir);
    bool mkv = g_settings.rec_container == 1;
    char path[1200], when[64];
    time_t now = time(NULL);
    strftime(when, sizeof when, "%Y-%m-%d %H-%M-%S", localtime(&now));
    snprintf(path, sizeof path, "%s/DKCR Recomp %s.%s", dir, when, mkv ? "mkv" : "mp4");
    fps = g_settings.rec_fps == 1 ? 30 : 60;
    // the window's size, or Output size's height (the window's shape); even sizes (4:2:0)
    int w = 0, h = 0;
    extern void renderer_window_size(int *w, int *h);
    renderer_window_size(&w, &h);
    if (w < 16 || h < 16) w = 1280, h = 720;
    static const int out_h[] = {0, 2160, 1440, 1080, 720};  // (REC_SIZE_*: 0 the window's)
    int sz = g_settings.rec_size >= 0 && g_settings.rec_size < 5 ? g_settings.rec_size : 0;
    if (out_h[sz]) w = (int)lround((double)w * out_h[sz] / h), h = out_h[sz];
    w &= ~1, h &= ~1;
    // Bitrate: Auto is 16 Mbit/s at 1080p60, scaled to the size and frame rate; else the chosen one
    static const int mbps[] = {0, 4, 8, 12, 16, 24, 32, 50, 80};
    int bi = g_settings.rec_bitrate >= 0 && g_settings.rec_bitrate < 9 ? g_settings.rec_bitrate : 0;
    int64_t bitrate = mbps[bi] ? (int64_t)mbps[bi] * 1000000
                               : (int64_t)(16e6 * ((double)w * h / (1920.0 * 1080.0)) * (fps / 60.0));
    if (bitrate < 2000000) bitrate = 2000000;
    bool ok = avformat_alloc_output_context2(&oc, NULL, mkv ? "matroska" : "mp4", path) >= 0 && oc;
    const AVCodec *vc = NULL;
    if (ok) ok = (vctx = open_video(w, h, bitrate, &vc)) != NULL;
    if (ok) {
        vst = avformat_new_stream(oc, NULL);
        ok = vst && avcodec_parameters_from_context(vst->codecpar, vctx) >= 0;
        if (vst) vst->time_base = vctx->time_base;
    }
    if (ok && g_settings.rec_audio) {
        const AVCodec *ac = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (ac) {
            actx = avcodec_alloc_context3(ac);
            actx->sample_rate = AUDIO_RATE;
            actx->sample_fmt = AV_SAMPLE_FMT_FLTP;
            av_channel_layout_default(&actx->ch_layout, 2);
            static const int kbps[] = {128, 192, 256, 320};
            actx->bit_rate = 1000 * kbps[g_settings.rec_audio_kbps >= 0 && g_settings.rec_audio_kbps < 4 ? g_settings.rec_audio_kbps : 1];
            actx->time_base = (AVRational){1, AUDIO_RATE};
            if (oc->oformat->flags & AVFMT_GLOBALHEADER) actx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            if (avcodec_open2(actx, ac, NULL) < 0 || actx->frame_size <= 0 || actx->frame_size > 1024) {
                LOG("[rec] no AAC encoder: recording without sound\n");
                avcodec_free_context(&actx);
            } else {
                ast = avformat_new_stream(oc, NULL);
                avcodec_parameters_from_context(ast->codecpar, actx);
                ast->time_base = actx->time_base;
            }
        }
    }
    if (ok) ok = avio_open(&oc->pb, path, AVIO_FLAG_WRITE) >= 0;
    if (ok) ok = avformat_write_header(oc, NULL) >= 0;
    if (ok) {
        vframe = av_frame_alloc();
        vframe->format = vctx->pix_fmt, vframe->width = vctx->width, vframe->height = vctx->height;
        ok = av_frame_get_buffer(vframe, 0) >= 0;
        pkt = av_packet_alloc();
        if (actx) {
            aframe = av_frame_alloc();
            aframe->format = actx->sample_fmt;
            aframe->nb_samples = actx->frame_size;
            av_channel_layout_copy(&aframe->ch_layout, &actx->ch_layout);
            ok = ok && av_frame_get_buffer(aframe, 0) >= 0;
        }
    }
    if (!ok) {
        LOG("[rec] couldn't start recording to %s\n", path);
        if (oc && oc->pb) avio_closep(&oc->pb);
        teardown();
        remove(path);
        return false;
    }
    snprintf(g_last_file, sizeof g_last_file, "%s", path + strlen(dir) + 1);
    snprintf(g_encoder_name, sizeof g_encoder_name, "%s", vc->name);
    LOG("[rec] recording to %s: %dx%d at %d fps, %s, %.0f Mbit/s%s\n", path, w, h, fps, vc->name, bitrate / 1e6,
        actx ? ", with sound" : "");
    return true;
}

bool recorder_start(void) {
    pthread_mutex_lock(&g_lock);
    bool ok = !atomic_load(&g_on) && !atomic_load(&g_starting);
    if (ok) {
        if (g_joinable) pthread_join(g_thread, NULL);
        atomic_store(&g_stop, false);
        atomic_store(&g_starting, true);
        g_joinable = pthread_create(&g_thread, NULL, encoder_main, NULL) == 0;
        if (!g_joinable) atomic_store(&g_starting, false);
        ok = g_joinable;
    }
    pthread_mutex_unlock(&g_lock);
    return ok;
}

void recorder_stop(void) {
    pthread_mutex_lock(&g_lock);
    if (g_joinable && (atomic_load(&g_on) || atomic_load(&g_starting))) {  // (still setting up: it stops as it starts)
        atomic_store(&g_stop, true);
        pthread_join(g_thread, NULL);
        g_joinable = false;
        if (atomic_load(&g_on)) {
            atomic_store(&g_on, false);
            teardown();
        }
    }
    pthread_mutex_unlock(&g_lock);
}

void recorder_toggle(void) {
    if (recorder_active()) recorder_stop();
    else recorder_start();
}
