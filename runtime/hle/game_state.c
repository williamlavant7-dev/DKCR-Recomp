// What the game is doing, for the runtime's per-frame logic (frame pacing, the slow-frame log, the display's refresh
// rate, menus that only make sense in play). LAS read Link's Awakening's game manager for these (hle/warp.c, mirror.c,
// widescreen.c); DKCR's equivalents aren't known yet, so the game counts as not in play and no world on screen:
// everything that waits for play stays off.
#include <stdbool.h>
#include <stdint.h>

int game_in_play(void) { return 0; }
bool world_on_screen(void) { return false; }
const char *game_level_name(void) { return ""; }
uint64_t game_camera_zone(void) { return 0; }
bool game_camera_zone_id(uint32_t *id) { (void)id; return false; }

// The start screen is up: no game loaded, so nothing may read its memory (runtime/ui/launcher.mm sets it)
int g_start_screen;
