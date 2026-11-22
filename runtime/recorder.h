// Video recording (runtime/recorder.c): the window's frames (before the overlay: no menu, no REC dot) and the game's
// sound into an MP4 in the data folder's recordings/. The renderer hands frames to recorder_video_frame, the audio
// services their output to recorder_audio; one encoder thread converts, encodes (a GPU's H.264 encoder where there is
// one) and writes the file. Started and stopped with F8 or from the menu's Recording tab (settings: rec_*).
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool recorder_start(void);   // false: already recording, or the file / an encoder couldn't be set up (logged)
void recorder_stop(void);    // finishes the file (waits for the encoder to write what it has)
void recorder_toggle(void);
bool recorder_active(void);
const char *recorder_last_file(void);  // the latest recording's file name ("" before any)
const char *recorder_encoder(void);    // the video encoder in use or last used ("" before any)
const char *recorder_dir(void);        // where videos go: Settings > Recording > Save folder, or recordings/

// Settings > Recording > Encoder (settings.h rec_encoder): Auto tries the GPU's first, then the rest in this order
enum { REC_ENC_AUTO = 0, REC_ENC_NVENC, REC_ENC_AMF, REC_ENC_QSV, REC_ENC_MF, REC_ENC_X26X, REC_ENC_MPEG4, REC_ENC_N };

// The renderer: is a frame wanted now (recording, a slot free)? Then the window image, top row first, 4 bytes a
// pixel (bgr: B, G, R, A order; else R, G, B, A), taken at t_ns (plat_now_ns).
bool recorder_wants_frame(void);
void recorder_video_frame(const uint8_t *px, uint32_t w, uint32_t h, uint32_t stride, bool bgr, uint64_t t_ns);

// The audio services: interleaved samples as they go to the speakers. source: 0 the audio renderer, 1 audio out.
// fmt: 0 signed 16-bit, 1 float 32-bit.
enum { REC_AUDIO_S16 = 0, REC_AUDIO_F32 = 1 };
void recorder_audio(int source, const void *data, uint32_t bytes, int fmt, int channels, int rate);

#ifdef __cplusplus
}
#endif
