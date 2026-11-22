// User settings (dkcrrecomp.ini next to the executable's working directory), edited from the in-game menu.
#pragma once
#include <stdbool.h>

enum { ASPECT_UNLOCKED = 0, ASPECT_ORIGINAL = 1 };
enum { RES_MATCH = 0, /* 1..8 = 1x..8x */ RES_DYNAMIC = 9 /* output_scale.c's mode while dyn_res is on */, RES_Q75 = 10, RES_Q25 = 11, RES_Q50 = 12 };
// (11 was 67% of the display: dkcrrecomp.ini "67%" now loads as RES_Q75)
enum { UPSCALE_BILINEAR = 0, UPSCALE_METALFX = 1 /* macOS */, UPSCALE_FSR1 = 2 /* Windows: AMD FSR 1 */ };
#ifdef __APPLE__
#define UPSCALE_SHARP UPSCALE_METALFX  // each OS's non-bilinear upscaler (the default)
#else
#define UPSCALE_SHARP UPSCALE_FSR1
#endif
enum { AA_OFF = 0, AA_SMAA = 1 };
// the game's shadow map: none, half the Switch's size, or 1 to 4 times it (output_scale_shadow). In menu order.
enum { SHADOW_OFF = 0, SHADOW_HALF, SHADOW_DEFAULT, SHADOW_2X, SHADOW_3X, SHADOW_4X };
enum { FPS_TARGET_60 = 0, FPS_TARGET_30 = 1, FPS_TARGET_MIXED = 2 };  // Internal FPS target (pace.c pace_target_30)
enum { AO_SWITCH = 0, AO_HIGH = 1, AO_OFF = 2 };  // the game's ambient occlusion: the Switch's size, up to twice it (output_scale_ao), or none
enum { PROMPTS_AUTO = 0, PROMPTS_NINTENDO = 1, PROMPTS_XBOX = 2, PROMPTS_PLAYSTATION = 3 };
// System language served to the game (runtime/services/am.c GetDesiredLanguage, services/hid_set.c GetLanguageCode):
// the game picks its text language from it, as it would from the Switch's own system setting, and the menus follow it
// too (runtime/ui/i18n.mm). LANG_AUTO follows the host's language (macOS / Windows: the user's preferred languages;
// Linux: $LC_ALL / $LC_MESSAGES / $LANG); the rest are the codes of the languages the game has text for.
// The ones from N_GAME_LANGUAGES on are the menus' only (pt-BR): the game has no text for them and is told en-US.
enum { LANG_AUTO = 0, N_GAME_LANGUAGES = 12, N_LANGUAGES = 13 };
extern const char *const g_language_codes[N_LANGUAGES];  // what the game is told: "ja", "en-US", "es", ...
extern const char *const g_language_names[N_LANGUAGES];  // ("Auto", "Japanese", ..., "Spanish")
extern const char *const g_language_endonyms[N_LANGUAGES];  // each in its own language, for the menu ("日本語", "Deutsch")


enum { AUDIO_OUT_STEREO, AUDIO_OUT_51, N_AUDIO_OUTS };
enum { FPS_OFF, FPS_TOP_LEFT, FPS_TOP_RIGHT, FPS_BOTTOM_LEFT, FPS_BOTTOM_RIGHT, N_FPS_CORNERS };

typedef struct {
    int aspect;          // ASPECT_*: unlocked = fill the window (wider/taller views), original = 16:9 with bars
    bool fullscreen;
    int res_mode;        // RES_MATCH, 1..4 (N x the Switch's 1080p), RES_DYNAMIC, RES_Q75/50/25 (share of the display)
    bool res_auto;       // never chosen: the default for this GPU (settings_resolve_defaults); saved as "auto"
    bool dyn_res;        // Dynamic resolution (its own toggle since 2026-11-12, was res_mode RES_DYNAMIC): up to the
                         // display's resolution, res_mode set aside while it's on (output_scale.c)
    int upscaler;        // UPSCALE_*: how a lower internal resolution reaches the window (MetalFX: macOS, FSR 1: Windows)
    bool upscaler_auto;  // never chosen: the default for this GPU (settings_resolve_defaults); saved as "auto"
    int antialias;       // AA_*: anti-aliasing of the game image before it is scaled to the window
    int ao_quality;      // AO_*: the resolution of the game's ambient occlusion
    int shadow_quality;  // SHADOW_*: the resolution of the game's shadow map
    int fps_target;      // FPS_TARGET_*: the game's 30 or 60 fps mode, or 30 outdoors (the Field) and 60 indoors
    int dyn_min;         // Dynamic resolution's lowest render height in lines: 360, 540 or 720
    float volume;        // master volume, 1 = 100%
    int audio_out;       // AUDIO_OUT_*: speaker layout of the game's sound (runtime/services/audio.c)
    bool reverb;         // room reverb effect
    bool movie_audio;
    int rec_quality;     // video recording (runtime/recorder.c): 0 Low, 1 Medium, 2 High
    int rec_fps;         // 0: 60 fps, 1: 30 fps
    bool rec_audio;      // the game's sound in the recording
    bool rec_dot;        // the red REC dot in a corner while recording (never in the video)
    bool frame_interp;   // Frame Generation: extra images between the game's frames (runtime/nvn/interp.c)
    bool metal_hud;      // macOS: Apple's Metal Performance HUD over the game (runtime/nvn/metal.m metal_hud_apply)
    int show_fps;        // FPS_*: the frame rate counter's corner (FPS_OFF: none)
    int fps_corner;      // FPS_* (not off): the corner Show FPS goes back to when turned on (the menu's Position row)
    bool fps_advanced;   // Show advanced information: the frame rate counter as a developer overlay (menu.mm perf_overlay)
    bool rumble;         // controller vibration from the game (runtime/hle/input.c)
    int rumble_strength; // its intensity, percent (25 / 50 / 75 / 100)
    bool controller_light; // the controller's light bar (runtime/led.h, hle/input.c)
    bool bg_input;       // controllers keep working while the window isn't in front (SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS)
    bool pause_unfocused; // the game stops while its window isn't in front (runtime/hle/vi.c)
    bool remember_window; // the window opens at the size it was closed at (runtime/paths.c rt_window_remember)
    int window_w, window_h; // that size, in points (0: none saved yet)
    int language;        // LANG_AUTO or an index into g_language_codes: the game's text language
    int prompts;         // PROMPTS_*: button glyphs in our menus (auto = follow the controller SDL reports)
    // Button layout "Xbox / PlayStation": on Xbox / PlayStation pads the game's A and B swap (A on the bottom button; X / Y
    // keep the Switch's positions), and our menus confirm with the bottom button (runtime/ui/pad_nav.mm)
    bool face_by_label;
                         // positions (bottom = A, right = B, left = X, top = Y): runtime/hle/bindings.c bind_pad_host
    bool aspect_asked;   // the start screen's one-time "authentic 16:9 or unlocked?" question was answered
    bool menu_tip_seen;  // the start screen's one-time "how to open the menu" note was shown (first PLAY ever)
    bool onedrive_kept;  // the start screen's OneDrive warning (Windows) was answered with Keep here
    int screen_filter;        // 0 Off, 1 Game Boy, 2 Game Boy Color, 3 Grayscale (runtime/nvn/screen_filter.h)
    // touch screens (runtime/ui/mobile/touch_controls.mm)
    int touch_mode;      // 0 Auto (while no controller is connected), 1 Always, 2 Off
    bool touch_stick_fixed;  // the touch stick stays in its place (else its base comes to the thumb)
    bool touch_dpad;  // the on-screen stick drawn and read as an 8-way D-pad (mobile/touch_controls.mm)
    bool touch_edit_button;  // the on-screen button (top, beside the menu's) that opens the layout editor
    bool touch_hide_xy_hud;  // the game's own X / Y item HUD hidden while the on-screen controls are shown
    float touch_opacity; // 0.15 .. 1
    float touch_size;    // 0.6 .. 1.6 (1 = the default layout)
    // the layout editor (touch_controls.mm): per control, moved (its place: share of the screen), size (x), hidden;
    // saved as touch_ctl_<name> = x, y, size, hidden
#define TOUCH_NCTL 14
    bool touch_moved[TOUCH_NCTL];
    float touch_pos[TOUCH_NCTL][2];
    float touch_scale[TOUCH_NCTL];
    bool touch_hidden[TOUCH_NCTL];
} Settings;


enum { N_SCREEN_FILTERS = 4 };
extern const char *const g_screen_filters[N_SCREEN_FILTERS];  // (the menu's names)
extern const char *const g_audio_outs[N_AUDIO_OUTS];  // (the menu's names: Stereo, 5.1)

extern Settings g_settings;
// The FPS target in effect: Dynamic resolution's own (user 2026-11-12) - with a resolution of the player's choosing
// the frame pacer decides (pace.c), as at 60
static inline int settings_fps_target(void) { return g_settings.dyn_res ? g_settings.fps_target : FPS_TARGET_60; }

const char *touch_control_name(int i);  // the on-screen controls' ini names (touch_ctl_<name>), "" past TOUCH_NCTL

void settings_load(void);
void settings_save(void);
// At device init: the defaults that depend on the GPU. integrated (Vulkan: an integrated GPU; never on macOS): plain
// bilinear upscaling instead of the sharp upscaler, whose pass at the window's size costs as much as the game's frame
// there (UHD 770: FSR 1 + SMAA 8 ms of GPU per frame at 1080p, 20 ms at 4K), and Dynamic resolution instead of Match
// display. A player's own choice is kept. Anti-aliasing is off by default everywhere. LARECOMP_INTEGRATED=0/1 (test):
// treat the GPU as dedicated / integrated.
void settings_resolve_defaults(bool integrated);
extern int g_default_upscaler;  // UPSCALE_*: what the menu shows as the default
extern int g_default_res_mode;  // RES_*: likewise
// An integrated GPU (as settings_resolve_defaults was told): the fixed 1x renders 1280x720 there, what the game itself
// picked on such a GPU (runtime/nvn/output_scale.c)
extern bool g_gpu_integrated;
// The language code to serve the game ("es", "en-US", ...): the setting, or the host locale for LANG_AUTO (en-US for a
// language of the menus only)
const char *settings_language(void);
// The menus' language code: the setting's, or the host locale's (pt-BR too, which the game itself doesn't have)
const char *settings_menu_language(void);
// That code's name for the menu ("Spanish")
const char *settings_language_name(void);
