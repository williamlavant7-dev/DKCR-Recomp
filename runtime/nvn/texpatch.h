// Edits to the game's own UI textures as they're uploaded (button prompts, the Hero Mode icon, heart and bottle
// tints). LAS knew Link's Awakening's textures here (runtime/nvn/texpatch.c, removed with its features); DKCR's aren't
// known, so no texture is identified and nothing is changed. The renderer's calls stay until its paths are taken out.
#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "nvn.h"
#include "recolour.h"  // (TEXPATCH_RECOLOUR_BASE)

// The backend's marker for a texture: id + 1, or -1 for none (always none now)
static inline int texpatch_marker(const NvnTexture *t) { (void)t; return -1; }
static inline int texpatch_bottle_kind(int id) { (void)id; return 0; }
static inline NvnTexture *texpatch_hint_icon(void) { return NULL; }
static inline bool texpatch_is_item_cursor(int id) { (void)id; return false; }
static inline void texpatch_note_draw(int id) { (void)id; }
static inline int texpatch_title_screen(void) { return 0; }
static inline uint32_t texpatch_generation(void) { return 0; }
static inline void texpatch_apply(int id, uint32_t lv, uint8_t *blocks, uint32_t w, uint32_t h, uint32_t row) {
    (void)id, (void)lv, (void)blocks, (void)w, (void)h, (void)row;
}
static inline int texpatch_heart_kind(int id) { (void)id; return 0; }
static inline void texpatch_heart_tint(int kind, float tint[8]) { (void)kind, (void)tint; }
