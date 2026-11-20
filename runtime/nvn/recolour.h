// Live recolours by draw (Cosmetics). LAS recoloured Link's Awakening textures here (runtime/nvn/recolour*.c, removed
// with its features); DKCR has none, so no texture has a recolour class and no draw is tinted. The renderer's calls
// stay (runtime/nvn/vulkan/exec.c) until its recolour paths are taken out.
#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "nvn.h"

// (texpatch ids from here on were recolour textures: none now)
#define TEXPATCH_RECOLOUR_BASE 4096
#define RECOLOUR_TINT_FLOATS 16

static inline int recolour_class_of(int id) { (void)id; return 0; }
static inline void recolour_add(int cls[2], int c) { (void)cls, (void)c; }
static inline void recolour_tint_draw(const int cls[2], float tint[RECOLOUR_TINT_FLOATS]) { (void)cls, (void)tint; }
