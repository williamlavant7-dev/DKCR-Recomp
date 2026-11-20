// Per-draw hooks of the renderer (runtime/nvn/vulkan/exec.c) that LAS's Link's Awakening features answered: mirror
// mode (hle/mirror.c), the HUD in the window's corners (hle/hud_corners.c), the pause screen's backdrop
// (hle/pause_fill.c), widescreen rooms and effects (hle/widescreen.c), gold pots (hle/pot_glow.c) and the pets'
// cursor (hle/pets.c). Those went with LAS's features; DKCR has none of them, so each answers as the feature would
// switched off and the game draws as it is. The renderer's calls stay until its LAS paths are taken out.
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// mirror mode: never on, no scene composite to find for it
bool mirror_scene_size(uint32_t sw, uint32_t sh, uint32_t w, uint32_t h) { (void)sw, (void)sh, (void)w, (void)h; return false; }
bool mirror_draw(bool composite) { (void)composite; return false; }
void mirror_scene_covered(void) {}
void mirror_layout_fill(const float *u, uint64_t size) { (void)u, (void)size; }
void scene_size_log(bool composite, uint32_t sw, uint32_t sh, uint32_t w, uint32_t h) {
    (void)composite, (void)sw, (void)sh, (void)w, (void)h;
}

// the HUD in the corners and the pause screen's backdrop: the game's own layout (no replacement uniforms)
bool hud_corners_bars(uint64_t frame, int w, int h, int bar[4]) { (void)frame, (void)w, (void)h, (void)bar; return false; }
bool hud_corners_active(void) { return false; }
const void *hud_corners_ubo(const float *u, uint64_t size, float k, float ky) { (void)u, (void)size, (void)k, (void)ky; return NULL; }
bool pause_fill_backdrop(int w, int h) { (void)w, (void)h; return false; }
bool pause_fill_scene_pane(bool reshaped, const float *u, uint64_t size) { (void)reshaped, (void)u, (void)size; return false; }
const void *pause_fill_ubo(const float *u, uint64_t size, float k, float ky) { (void)u, (void)size, (void)k, (void)ky; return NULL; }

// widescreen: nothing widened (a stretch of 1)
void widescreen_backdrop_seen(uint64_t hash) { (void)hash; }
void widescreen_scene_ubo(const float *u, uint64_t size) { (void)u, (void)size; }
bool widescreen_ui_pane_partial(const float *u, uint64_t size) { (void)u, (void)size; return false; }
float widescreen_effect_stretch(uint64_t fs_hash) { (void)fs_hash; return 1.0f; }

// gold pots and the pets' cursor: none
bool pot_level_active(void) { return false; }
bool pot_glow_active(void) { return false; }
bool pot_tint(const float *m, float tint[8], uint64_t mesh) { (void)m, (void)tint, (void)mesh; return false; }
bool pot_instance_gold(const float *m) { (void)m; return false; }
void pot_gold_tint(float tint[8]) { (void)tint; }
bool pets_hide_item_cursor(void) { return false; }
