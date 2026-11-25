#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "platform.h"  // (PLAT_TOUCH, before the blocks below)
#if PLAT_TOUCH
#include <stdatomic.h>
#endif
#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include "hle.h"
#include "i18n_mark.h"
#include "input_bind.h"

#define SETTINGS_FILE settings_file()

// LARECOMP_SETTINGS=<path>: another settings file (test runs never touch the player's dkcrrecomp.ini). A data folder
// from before the rename has larecomp.ini: it becomes dkcrrecomp.ini the first time the settings are read.
static const char *settings_file(void) {
    const char *e = getenv("LARECOMP_SETTINGS");
    if (e && *e) return e;
    static bool moved;
    if (!moved) {
        moved = true;
        FILE *f = fopen("dkcrrecomp.ini", "r");
        if (f) fclose(f);
        else rename("larecomp.ini", "dkcrrecomp.ini");
    }
    return "dkcrrecomp.ini";
}

const char *const g_screen_filters[N_SCREEN_FILTERS] = {LN("Off"), LN("Game Boy"), LN("Game Boy Color"),
                                                        LN("Grayscale")};
static const char *const k_screen_filter_keys[N_SCREEN_FILTERS] = {"off", "gameboy", "gameboy_color", "grayscale"};  // (else: off)
const char *const g_audio_outs[N_AUDIO_OUTS] = {LN("Stereo"), LN("5.1")};
static const char *const k_audio_out_keys[N_AUDIO_OUTS] = {"stereo", "5.1"};  // (else: stereo)

// the on-screen controls' ini names (touch_ctl_<name>; runtime/ui/mobile/touch_controls.mm's order)
const char *touch_control_name(int i) {
    static const char *const names[TOUCH_NCTL] = {"stick", "a", "b", "x", "y", "l", "r", "zl", "zr", "plus", "minus", "menu", "start", "edit"};
    return i >= 0 && i < TOUCH_NCTL ? names[i] : "";
}

Settings g_settings = {
    .aspect = ASPECT_UNLOCKED,
    .dyn_min = 540,
    .fullscreen = false,
    .volume = 1.0f,
    .reverb = true,
    .movie_audio = true,
    .rec_quality = 1,
    .rec_audio_kbps = 1,
    .rec_size = 3,
    .rec_cq = 20,
    .rec_speed = 1,
    .rec_prefix = "DKCR Recording",
    .rec_audio = true,
    .rec_dot = true,
    .rumble = true,
    .rumble_strength = 100,
    .controller_light = true,
    .show_fps = FPS_OFF,
    .fps_corner = FPS_TOP_LEFT,
    .touch_opacity = 0.6f,
    .touch_edit_button = true,
    .touch_hide_xy_hud = true,  // (the on-screen X / Y show the items)
    .touch_size = 1.0f,
    .upscaler = UPSCALE_SHARP,  // (MetalFX on macOS, AMD FSR 1 on Windows)
    .upscaler_auto = true,
    .shadow_quality = SHADOW_DEFAULT,
    .res_auto = true,
#ifndef __APPLE__
    .bg_input = true,  // (as before the setting; on macOS it needs Input Monitoring permission, so Off there)
#endif
};

// The game's own text languages: its RomFS carries these per region (regionUS/USen|USes|USfr, regionEU/EUen|EUes|...,
// regionJP/JPja, regionKR/KRkr, regionTW/TWch, regionCN/CNch), and the game picks one from the system language it
// reads back (runtime/services/am.c), exactly as it does on the console. Codes as the Switch reports them, except for
// the Chinese ones: the game only recognizes zh-Hans / zh-Hant and falls back to Japanese when told zh-CN / zh-TW.
// Then the menus' own (N_GAME_LANGUAGES on): the game has no text in them, its text stays English.
const char *const g_language_codes[N_LANGUAGES] = {"auto", "ja", "en-US", "es", "fr", "de", "it", "nl", "ru", "ko",
                                                   "zh-Hans", "zh-Hant", "pt-BR"};
const char *const g_language_names[N_LANGUAGES] = {"Auto", "Japanese", "English", "Spanish", "French", "German",
                                                   "Italian", "Dutch", "Russian", "Korean", "Chinese (Simplified)",
                                                   "Chinese (Traditional)", "Portuguese (Brazil)"};
const char *const g_language_endonyms[N_LANGUAGES] = {"Auto", "日本語", "English", "Español", "Français", "Deutsch",
                                                      "Italiano", "Nederlands", "Русский", "한국어", "简体中文", "繁體中文",
                                                      "Português (Brasil)"};

// A locale name ("es_ES.UTF-8", "zh-Hans-CN", "ja-JP", "zh_TW") -> one of the game's codes, NULL for none of its
// languages.
static const char *match_locale(const char *e) {
    if (!e || !*e || !strcmp(e, "C") || !strcmp(e, "POSIX")) return NULL;
    if (!strncmp(e, "zh", 2) && (!e[2] || strchr("_-.@", e[2]))) {
        char locale[64];
        snprintf(locale, sizeof locale, "%s", e);
        locale[strcspn(locale, ".@")] = 0;
        for (char *p = locale; *p; ++p) *p = (char)tolower((unsigned char)*p);
        bool traditional = false;
        char *next;
        for (char *part = strtok_r(locale, "_-", &next); part; part = strtok_r(NULL, "_-", &next)) {
            // An explicit script takes precedence over the region, e.g. zh-Hans-HK.
            if (!strcmp(part, "hant")) return "zh-Hant";
            if (!strcmp(part, "hans")) return "zh-Hans";
            if (!strcmp(part, "tw") || !strcmp(part, "hk") || !strcmp(part, "mo")) traditional = true;
        }
        return traditional ? "zh-Hant" : "zh-Hans";
    }
    char l[8] = {0};
    size_t n = 0;
    for (const char *p = e; *p && !strchr(".@_-", *p) && n < sizeof l - 1; p++) l[n++] = (char)tolower((unsigned char)*p);
    static const struct { const char *loc, *code; } map[] = {{"ja", "ja"}, {"ko", "ko"}, {"ru", "ru"}, {"nl", "nl"},
                                                             {"de", "de"}, {"it", "it"}, {"fr", "fr"}, {"es", "es"},
                                                             {"en", "en-US"}, {"pt", "pt-BR"}};
    for (size_t i = 0; i < sizeof map / sizeof *map; i++)
        if (!strcmp(l, map[i].loc)) return map[i].code;
    return NULL;
}

// LANG_AUTO: the host's own language, en-US when it is none of the menus'. Linux: the locale variables
// (LANG=es_ES.UTF-8 -> es); macOS / Windows: the first of the user's preferred languages that the game has (apps get
// no locale variables there).
static const char *host_language(void) {
    static const char *lang;
    if (lang) return lang;
#if defined(__APPLE__)
    CFArrayRef prefs = CFLocaleCopyPreferredLanguages();
    for (CFIndex i = 0; prefs && i < CFArrayGetCount(prefs) && !lang; i++) {
        char buf[64];
        if (CFStringGetCString((CFStringRef)CFArrayGetValueAtIndex(prefs, i), buf, sizeof buf, kCFStringEncodingUTF8))
            lang = match_locale(buf);
    }
    if (prefs) CFRelease(prefs);
#elif defined(_WIN32)
    ULONG count = 0, len = 0;
    if (GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, NULL, &len) && len) {
        WCHAR *w = (WCHAR *)calloc(len, sizeof *w);
        if (w && GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, w, &len))
            for (WCHAR *p = w; *p && !lang; p += wcslen(p) + 1) {
                char buf[64];
                if (WideCharToMultiByte(CP_UTF8, 0, p, -1, buf, sizeof buf, NULL, NULL)) lang = match_locale(buf);
            }
        free(w);
    }
#else
    const char *e = getenv("LC_ALL");
    if (!e || !*e) e = getenv("LC_MESSAGES");
    if (!e || !*e) e = getenv("LANG");
    lang = match_locale(e);
#endif
    if (!lang) lang = "en-US";
    return lang;
}

const char *settings_menu_language(void) {
    int i = g_settings.language;
    return i > LANG_AUTO && i < N_LANGUAGES ? g_language_codes[i] : host_language();
}

const char *settings_language(void) {
    const char *c = settings_menu_language();
    for (int i = N_GAME_LANGUAGES; i < N_LANGUAGES; i++)
        if (!strcmp(c, g_language_codes[i])) return "en-US";  // (the menus' only)
    return c;
}

const char *settings_language_name(void) {
    const char *c = settings_menu_language();
    for (int i = 0; i < N_LANGUAGES; i++)
        if (!strcmp(c, g_language_codes[i])) return g_language_names[i];
    return "English";
}

int g_default_upscaler = UPSCALE_SHARP;
int g_default_res_mode = RES_MATCH;
bool g_gpu_integrated;

static const char *res_name(int m) {
    static char n[8];
    if (m >= 1 && m <= 8) { snprintf(n, sizeof n, "%dx", m); return n; }
    return m == RES_Q75 ? "75%" : m == RES_Q50 ? "50%" : m == RES_Q25 ? "25%" : "match";
}

void settings_resolve_defaults(bool integrated) {
    const char *e = getenv("LARECOMP_INTEGRATED");
    if (e && *e) integrated = atoi(e) != 0;
    g_gpu_integrated = integrated;
    g_default_upscaler = integrated ? UPSCALE_BILINEAR : UPSCALE_SHARP;
    g_default_res_mode = RES_MATCH;  // (Dynamic is not a default until it is reliable: user 2026-10-18)
    if (g_settings.upscaler_auto) g_settings.upscaler = g_default_upscaler;
    if (g_settings.res_auto) g_settings.res_mode = g_default_res_mode;
    LOG("[settings] %s GPU: upscaler %s (%s), resolution %s (%s)%s, anti-aliasing %s\n", integrated ? "integrated" : "discrete / other",
        g_settings.upscaler == UPSCALE_BILINEAR ? "bilinear" : g_settings.upscaler == UPSCALE_FSR1 ? "fsr1" : "metalfx",
        g_settings.upscaler_auto ? "default" : "chosen", res_name(g_settings.res_mode), g_settings.res_auto ? "default" : "chosen",
        g_settings.dyn_res ? ", dynamic" : "", g_settings.antialias == AA_SMAA ? "smaa" : "off");
}

void settings_load(void) {
    static bool loaded;
    if (loaded) return;
    loaded = true;
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return;
    char line[1200];  // (room for a long folder: record_folder)
    bool tip_key = false, aspect_key = false;
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char key[64];
        char val[1100] = {0};
        int fields = sscanf(line, " %63[^= ] = %1099[^\n]", key, val);
        if (fields < 1) continue;
        if (fields != 2) continue;
        if (!strcmp(key, "aspect")) g_settings.aspect = !strcmp(val, "original") ? ASPECT_ORIGINAL : ASPECT_UNLOCKED;
        else if (!strcmp(key, "fullscreen")) g_settings.fullscreen = atoi(val) != 0;
        else if (!strcmp(key, "volume")) g_settings.volume = (float)atof(val);
        else if (!strcmp(key, "audio_output")) {
            g_settings.audio_out = AUDIO_OUT_STEREO;
            for (int i = 0; i < N_AUDIO_OUTS; i++) if (!strcmp(val, k_audio_out_keys[i])) g_settings.audio_out = i;
        }
        else if (!strcmp(key, "reverb")) g_settings.reverb = atoi(val) != 0;
        else if (!strcmp(key, "movie_audio")) g_settings.movie_audio = atoi(val) != 0;
        else if (!strcmp(key, "record_quality")) g_settings.rec_quality = !strcmp(val, "low") ? 0 : !strcmp(val, "high") ? 2 : 1;
        else if (!strcmp(key, "record_fps")) g_settings.rec_fps = atoi(val) == 30;
        else if (!strcmp(key, "record_sound")) g_settings.rec_audio = atoi(val) != 0;
        else if (!strcmp(key, "record_dot")) g_settings.rec_dot = atoi(val) != 0;
        else if (!strcmp(key, "record_folder")) snprintf(g_settings.rec_dir, sizeof g_settings.rec_dir, "%s", val);
        else if (!strcmp(key, "record_codec")) g_settings.rec_codec = !strcmp(val, "hevc");
        else if (!strcmp(key, "record_encoder")) {
            static const char *const e[] = {"auto", "nvenc", "amf", "qsv", "mediafoundation", "software", "mpeg4"};
            g_settings.rec_encoder = 0;
            for (int i = 0; i < 7; i++) if (!strcmp(val, e[i])) g_settings.rec_encoder = i;
        }
        else if (!strcmp(key, "record_bitrate")) {
            static const int m[] = {0, 4, 8, 12, 16, 24, 32, 50, 80};
            g_settings.rec_bitrate = 0;
            for (int i = 1; i < 9; i++) if (atoi(val) == m[i]) g_settings.rec_bitrate = i;
        }
        else if (!strcmp(key, "record_size")) {
            static const int hh[] = {0, 2160, 1440, 1080, 720};
            g_settings.rec_size = 3;  // ("window", from before the fixed sizes: 1080p)
            for (int i = 1; i < 5; i++) if (atoi(val) == hh[i]) g_settings.rec_size = i;
        }
        else if (!strcmp(key, "record_cq")) {
            int v = atoi(val);
            g_settings.rec_cq = v >= 1 && v <= 51 ? v : 20;
        }
        else if (!strcmp(key, "record_speed")) {
            static const char *const sp[] = {"fastest", "fast", "balanced", "smallest"};
            g_settings.rec_speed = 1;
            for (int i = 0; i < 4; i++) if (!strcmp(val, sp[i])) g_settings.rec_speed = i;
        }
        else if (!strcmp(key, "record_name")) snprintf(g_settings.rec_prefix, sizeof g_settings.rec_prefix, "%s", val);
        else if (!strcmp(key, "record_format")) g_settings.rec_container = !strcmp(val, "mkv");
        else if (!strcmp(key, "record_audio_bitrate")) {
            static const int k[] = {128, 192, 256, 320};
            g_settings.rec_audio_kbps = 1;
            for (int i = 0; i < 4; i++) if (atoi(val) == k[i]) g_settings.rec_audio_kbps = i;
        }
        else if (!strcmp(key, "show_fps")) {  // (older files: 0 / 1, and 1 was the top right)
            static const char *const corners[] = {"off", "top_left", "top_right", "bottom_left", "bottom_right"};
            g_settings.show_fps = atoi(val) ? FPS_TOP_RIGHT : FPS_OFF;
            for (int i = 0; i < N_FPS_CORNERS; i++)
                if (!strcmp(val, corners[i])) g_settings.show_fps = i;
        }
        else if (!strcmp(key, "fps_advanced")) g_settings.fps_advanced = atoi(val) != 0;
        else if (!strcmp(key, "fps_corner")) {
            static const char *const corners[] = {"off", "top_left", "top_right", "bottom_left", "bottom_right"};
            for (int i = 1; i < N_FPS_CORNERS; i++)
                if (!strcmp(val, corners[i])) g_settings.fps_corner = i;
        }
        else if (!strcmp(key, "frame_interp")) g_settings.frame_interp = atoi(val) != 0;
        else if (!strcmp(key, "metal_hud")) g_settings.metal_hud = atoi(val) != 0;
        else if (!strcmp(key, "resolution")) {  // (auto: never chosen, the GPU's default)
            int n = atoi(val);
            g_settings.res_auto = !strcmp(val, "auto");
            if (g_settings.res_auto) { g_settings.res_mode = g_default_res_mode; continue; }
            // ("dynamic": older files, when Dynamic was one of these - its toggle now, over Match display)
            if (!strcmp(val, "dynamic")) { g_settings.dyn_res = true, g_settings.res_mode = RES_MATCH; continue; }
            g_settings.res_mode = (!strcmp(val, "75%") || !strcmp(val, "67%")) ? RES_Q75 :
                                  !strcmp(val, "50%") ? RES_Q50 : !strcmp(val, "25%") ? RES_Q25 :
                                  (n >= 1 && n <= 4) ? n : n > 4 ? 4 : RES_MATCH;  // (5x-8x were removed)
        }
        else if (!strcmp(key, "upscaler")) {  // (metalfx / fsr1: this OS's; auto: never chosen, the GPU's default)
            g_settings.upscaler_auto = !strcmp(val, "auto");
            if (!g_settings.upscaler_auto) g_settings.upscaler = !strcmp(val, "bilinear") ? UPSCALE_BILINEAR : UPSCALE_SHARP;
            else g_settings.upscaler = g_default_upscaler;
        }
        else if (!strcmp(key, "antialias")) g_settings.antialias = !strcmp(val, "off") ? AA_OFF : AA_SMAA;  // ("fxaa": SMAA now)
        else if (!strcmp(key, "shadows"))  // ("high": test builds of #602 before 3x and 4x, the same as 2x)
            g_settings.shadow_quality = !strcmp(val, "2x") || !strcmp(val, "high") ? SHADOW_2X
                                      : !strcmp(val, "3x") ? SHADOW_3X : !strcmp(val, "4x") ? SHADOW_4X : !strcmp(val, "off") ? SHADOW_OFF
                                      : !strcmp(val, "0.5x") ? SHADOW_HALF : SHADOW_DEFAULT;
        else if (!strcmp(key, "dynamic_resolution")) g_settings.dyn_res = atoi(val) != 0;
        else if (!strcmp(key, "dynamic_min")) {
            int m = atoi(val);  // (anything else, the old 25 / 50 / 75%: 540)
            g_settings.dyn_min = m == 360 || m == 720 ? m : 540;
        }
        else if (!strcmp(key, "fps_target")) g_settings.fps_target = !strcmp(val, "30") ? FPS_TARGET_30 : !strcmp(val, "mixed") ? FPS_TARGET_MIXED : FPS_TARGET_60;
        else if (!strcmp(key, "ambient_occlusion")) g_settings.ao_quality = !strcmp(val, "high") ? AO_HIGH : !strcmp(val, "off") ? AO_OFF : AO_SWITCH;
        else if (!strcmp(key, "touch_controls")) g_settings.touch_mode = !strcmp(val, "always") ? 1 : !strcmp(val, "off") ? 2 : 0;
        else if (!strcmp(key, "touch_opacity")) g_settings.touch_opacity = (float)atof(val);
        else if (!strcmp(key, "touch_stick_fixed")) g_settings.touch_stick_fixed = atoi(val) != 0;
        else if (!strcmp(key, "touch_edit_button")) g_settings.touch_edit_button = atoi(val) != 0;
        else if (!strcmp(key, "touch_hide_xy_hud")) g_settings.touch_hide_xy_hud = atoi(val) != 0;
        else if (!strcmp(key, "touch_dpad")) g_settings.touch_dpad = atoi(val) != 0;
        else if (!strcmp(key, "touch_size")) g_settings.touch_size = (float)atof(val);
        else if (!strncmp(key, "touch_ctl_", 10)) {  // (the layout editor: x, y, size, hidden)
            for (int i = 0; i < TOUCH_NCTL; i++) {
                if (strcmp(key + 10, touch_control_name(i))) continue;
                float x = -1, y = -1, s = 1;
                int hid = 0;
                if (sscanf(val, "%f , %f , %f , %d", &x, &y, &s, &hid) >= 3) {
                    g_settings.touch_moved[i] = x >= 0 && y >= 0;
                    g_settings.touch_pos[i][0] = x, g_settings.touch_pos[i][1] = y;
                    g_settings.touch_scale[i] = s, g_settings.touch_hidden[i] = hid != 0;
                }
            }
        }
        else if (!strcmp(key, "rumble")) g_settings.rumble = atoi(val) != 0;
        else if (!strcmp(key, "rumble_strength")) g_settings.rumble_strength = atoi(val);
        else if (!strcmp(key, "controller_light")) g_settings.controller_light = atoi(val) != 0;
        else if (!strcmp(key, "background_input")) g_settings.bg_input = atoi(val) != 0;
        else if (!strcmp(key, "pause_unfocused")) g_settings.pause_unfocused = atoi(val) != 0;
        else if (!strcmp(key, "remember_window")) g_settings.remember_window = atoi(val) != 0;
        else if (!strcmp(key, "window_size")) sscanf(val, "%dx%d", &g_settings.window_w, &g_settings.window_h);
        else if (!strcmp(key, "prompts")) g_settings.prompts = !strcmp(val, "nintendo") ? PROMPTS_NINTENDO : !strcmp(val, "xbox") ?
                                                              PROMPTS_XBOX : !strcmp(val, "playstation") ? PROMPTS_PLAYSTATION : PROMPTS_AUTO;
        else if (!strcmp(key, "face_buttons_by_label")) g_settings.face_by_label = atoi(val) != 0;
        else if (!strcmp(key, "language")) {  // ("auto" or one of the game's codes; anything else: auto)
            // (zh-CN / zh-TW: what a settings file from before this wrote; the game reads zh-Hans / zh-Hant)
            const char *v = !strcmp(val, "zh-CN") ? "zh-Hans" : !strcmp(val, "zh-TW") ? "zh-Hant" : val;
            g_settings.language = LANG_AUTO;
            for (int i = 1; i < N_LANGUAGES; i++)
                if (!strcmp(v, g_language_codes[i])) g_settings.language = i;
        }
        else if (!strcmp(key, "menu_tip_seen")) g_settings.menu_tip_seen = atoi(val) != 0, tip_key = true;
        else if (!strcmp(key, "aspect_asked")) g_settings.aspect_asked = atoi(val) != 0, aspect_key = true;
        else if (!strcmp(key, "onedrive_kept")) g_settings.onedrive_kept = atoi(val) != 0;
        else if (!strcmp(key, "screen_filter")) {
            for (int i = 0; i < N_SCREEN_FILTERS; i++) if (!strcmp(val, k_screen_filter_keys[i])) g_settings.screen_filter = i;
        }
        else bind_settings_load(key, val);  // bind_pad_* / bind_key_*: runtime/hle/bindings.c
    }
    fclose(f);
    if (!tip_key) g_settings.menu_tip_seen = true;  // (a settings file from before the note: not a first launch)
    if (!aspect_key) g_settings.aspect_asked = true;  // (likewise: the aspect ratio was chosen already)
    if (g_settings.show_fps > FPS_OFF && g_settings.show_fps < N_FPS_CORNERS) g_settings.fps_corner = g_settings.show_fps;
    if (g_settings.volume < 0) g_settings.volume = 0;
    if (g_settings.volume > 2) g_settings.volume = 2;
    if (g_settings.rumble_strength < 25 || g_settings.rumble_strength > 100) g_settings.rumble_strength = 100;
    LOG("[settings] loaded %s\n", SETTINGS_FILE);
}

void settings_save(void) {
#if PLAT_TOUCH
    // Written to larecomp.ini.tmp, then put in its place: a phone ends apps in the background at any moment (the app
    // saves then too: vulkan/device.c), and a file cut short there lost every setting. One writer at a time. (From
    // Nycolazs's LASRecomp-Android-Plus via mamcjr's LASRecompAndroid, MIT.)
    static atomic_flag lock = ATOMIC_FLAG_INIT;
    char tmp[1024];
    snprintf(tmp, sizeof tmp, "%s.tmp", SETTINGS_FILE);
    while (atomic_flag_test_and_set(&lock)) {}
    FILE *f = fopen(tmp, "w");
    if (!f) { LOG("[settings] cannot write %s\n", tmp); atomic_flag_clear(&lock); return; }
#else
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) { LOG("[settings] cannot write %s\n", SETTINGS_FILE); return; }
#endif
    fprintf(f, "# DKCR Recomp settings (edited from the in-game menu: F1, Esc, or Minus+Plus on a controller)\n");
    fprintf(f, "aspect = %s\n", g_settings.aspect == ASPECT_ORIGINAL ? "original" : "unlocked");
    fprintf(f, "fullscreen = %d\n", g_settings.fullscreen);
    fprintf(f, "volume = %.2f\n", g_settings.volume);
    fprintf(f, "audio_output = %s\n", k_audio_out_keys[g_settings.audio_out == AUDIO_OUT_51]);
    fprintf(f, "reverb = %d\n", g_settings.reverb);
    fprintf(f, "movie_audio = %d\n", g_settings.movie_audio);
    {
        static const char *const q[] = {"low", "medium", "high"};
        fprintf(f, "record_quality = %s\n", q[g_settings.rec_quality < 0 || g_settings.rec_quality > 2 ? 1 : g_settings.rec_quality]);
    }
    fprintf(f, "record_fps = %d\n", g_settings.rec_fps == 1 ? 30 : 60);
    fprintf(f, "record_sound = %d\n", g_settings.rec_audio);
    fprintf(f, "record_dot = %d\n", g_settings.rec_dot);
    {
        static const char *const e[] = {"auto", "nvenc", "amf", "qsv", "mediafoundation", "software", "mpeg4"};
        static const int m[] = {0, 4, 8, 12, 16, 24, 32, 50, 80}, hh[] = {0, 2160, 1440, 1080, 720}, k[] = {128, 192, 256, 320};
        int ei = g_settings.rec_encoder >= 0 && g_settings.rec_encoder < 7 ? g_settings.rec_encoder : 0;
        int bi = g_settings.rec_bitrate >= 0 && g_settings.rec_bitrate < 9 ? g_settings.rec_bitrate : 0;
        int si = g_settings.rec_size >= 1 && g_settings.rec_size < 5 ? g_settings.rec_size : 3;
        int ki = g_settings.rec_audio_kbps >= 0 && g_settings.rec_audio_kbps < 4 ? g_settings.rec_audio_kbps : 1;
        fprintf(f, "record_folder = %s\n", g_settings.rec_dir);
        fprintf(f, "record_codec = %s\n", g_settings.rec_codec == 1 ? "hevc" : "h264");
        fprintf(f, "record_encoder = %s\n", e[ei]);
        if (bi) fprintf(f, "record_bitrate = %d\n", m[bi]); else fprintf(f, "record_bitrate = auto\n");
        if (si) fprintf(f, "record_size = %d\n", hh[si]); else fprintf(f, "record_size = window\n");
        fprintf(f, "record_format = %s\n", g_settings.rec_container == 1 ? "mkv" : "mp4");
        static const char *const sp[] = {"fastest", "fast", "balanced", "smallest"};
        fprintf(f, "record_cq = %d\n", g_settings.rec_cq >= 1 && g_settings.rec_cq <= 51 ? g_settings.rec_cq : 20);
        fprintf(f, "record_speed = %s\n", sp[g_settings.rec_speed >= 0 && g_settings.rec_speed < 4 ? g_settings.rec_speed : 1]);
        fprintf(f, "record_name = %s\n", g_settings.rec_prefix);
        fprintf(f, "record_audio_bitrate = %d\n", k[ki]);
    }
    {
        static const char *const corners[] = {"off", "top_left", "top_right", "bottom_left", "bottom_right"};
        int c = g_settings.show_fps;
        fprintf(f, "show_fps = %s\n", corners[c >= 0 && c < N_FPS_CORNERS ? c : 0]);
        c = g_settings.fps_corner;
        fprintf(f, "fps_corner = %s\n", corners[c > 0 && c < N_FPS_CORNERS ? c : FPS_TOP_LEFT]);
        fprintf(f, "fps_advanced = %d\n", g_settings.fps_advanced);
    }
    fprintf(f, "frame_interp = %d\n", g_settings.frame_interp);
#ifdef __APPLE__
    fprintf(f, "metal_hud = %d\n", g_settings.metal_hud);
#endif
    fprintf(f, "upscaler = %s\n", g_settings.upscaler_auto ? "auto" : g_settings.upscaler == UPSCALE_METALFX ? "metalfx" :
                                   g_settings.upscaler == UPSCALE_FSR1 ? "fsr1" : "bilinear");
    fprintf(f, "antialias = %s\n", g_settings.antialias == AA_SMAA ? "smaa" : "off");
    {
        static const char *sh[] = {"off", "0.5x", "default", "2x", "3x", "4x"};
        int q = g_settings.shadow_quality;
        fprintf(f, "shadows = %s\n", sh[q >= SHADOW_OFF && q <= SHADOW_4X ? q : SHADOW_DEFAULT]);
    }
    fprintf(f, "ambient_occlusion = %s\n", g_settings.ao_quality == AO_HIGH ? "high" : g_settings.ao_quality == AO_OFF ? "off" : "switch");
    fprintf(f, "resolution = %s\n", g_settings.res_auto ? "auto" : res_name(g_settings.res_mode));
    fprintf(f, "dynamic_resolution = %d\n", g_settings.dyn_res);
    fprintf(f, "dynamic_min = %d\n", g_settings.dyn_min);
    fprintf(f, "fps_target = %s\n", g_settings.fps_target == FPS_TARGET_30 ? "30" : g_settings.fps_target == FPS_TARGET_MIXED ? "mixed" : "60");
    fprintf(f, "touch_controls = %s\n", g_settings.touch_mode == 1 ? "always" : g_settings.touch_mode == 2 ? "off" : "auto");
    fprintf(f, "touch_opacity = %.2f\n", g_settings.touch_opacity);
    fprintf(f, "touch_stick_fixed = %d\ntouch_hide_xy_hud = %d\ntouch_edit_button = %d\n", g_settings.touch_stick_fixed,
            g_settings.touch_hide_xy_hud, g_settings.touch_edit_button);
    fprintf(f, "touch_size = %.2f\n", g_settings.touch_size);
    fprintf(f, "touch_dpad = %d\n", g_settings.touch_dpad);
    {
        for (int i = 0; i < TOUCH_NCTL; i++)
            if (g_settings.touch_moved[i] || g_settings.touch_hidden[i] || (g_settings.touch_scale[i] > 0.1f && fabsf(g_settings.touch_scale[i] - 1) > 0.01f))
                fprintf(f, "touch_ctl_%s = %.4f, %.4f, %.2f, %d\n", touch_control_name(i), g_settings.touch_moved[i] ? g_settings.touch_pos[i][0] : -1,
                        g_settings.touch_moved[i] ? g_settings.touch_pos[i][1] : -1, g_settings.touch_scale[i] > 0.1f ? g_settings.touch_scale[i] : 1,
                        g_settings.touch_hidden[i]);
    }
    fprintf(f, "rumble = %d\n", g_settings.rumble);
    fprintf(f, "rumble_strength = %d\n", g_settings.rumble_strength);
    fprintf(f, "controller_light = %d\n", g_settings.controller_light);
    fprintf(f, "background_input = %d\n", g_settings.bg_input);
    fprintf(f, "pause_unfocused = %d\nremember_window = %d\n", g_settings.pause_unfocused, g_settings.remember_window);
    if (g_settings.window_w > 0 && g_settings.window_h > 0)
        fprintf(f, "window_size = %dx%d\n", g_settings.window_w, g_settings.window_h);
    static const char *const prompts[] = {"auto", "nintendo", "xbox", "playstation"};
    fprintf(f, "prompts = %s\n", prompts[g_settings.prompts & 3]);
    fprintf(f, "face_buttons_by_label = %d\n", g_settings.face_by_label);
    fprintf(f, "language = %s\n", g_language_codes[g_settings.language > LANG_AUTO && g_settings.language < N_LANGUAGES
                                                     ? g_settings.language : LANG_AUTO]);
    fprintf(f, "aspect_asked = %d\n", g_settings.aspect_asked);
    fprintf(f, "menu_tip_seen = %d\n", g_settings.menu_tip_seen);
    fprintf(f, "onedrive_kept = %d\n", g_settings.onedrive_kept);
    fprintf(f, "screen_filter = %s\n", k_screen_filter_keys[g_settings.screen_filter]);
    bind_settings_save(f);
#if PLAT_TOUCH
    if (fclose(f) != 0 || rename(tmp, SETTINGS_FILE) != 0) LOG("[settings] cannot write %s\n", SETTINGS_FILE);
    atomic_flag_clear(&lock);
#else
    fclose(f);
#endif
}
