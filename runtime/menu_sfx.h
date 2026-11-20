// The settings menu's sounds (in game and on the start screen). LAS played Link's Awakening's own menu sounds from its
// sound archive (runtime/menu_sfx.c, removed with the LAS features); DKCR's aren't hooked up yet, so every call is
// silent. The calls stay where the menus make them, so DKCR's sounds can be added here without touching the menus.
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { MENU_SFX_CURSOR, MENU_SFX_TAB, MENU_SFX_DECIDE, MENU_SFX_CANCEL, MENU_SFX_OPEN, MENU_SFX_CLOSE, MENU_SFX_N };

static inline void menu_sfx_play(int id) { (void)id; }  // any thread
// (runtime/ui/option_rows.mm, which works out from ImGui's state which sound a frame plays, then menu_sfx_play:)
// After an ImGui frame of the menu: the cursor, decide and cancel sounds of what the frame did
void menu_sfx_frame(bool back_ok);
// The item just drawn is a menu tab (its click plays the tab sound, not decide)
void menu_sfx_tab_item(void);
// This frame's press plays no decide sound (a value step plays its own)
void menu_sfx_click_silent(void);
// Nothing played for a while: the stream pauses
static inline void menu_sfx_idle_check(void) {}
// The start screen's game folder, until the game's own is set
static inline void menu_sfx_set_game_dir(const char *dir) { (void)dir; }

#ifdef __cplusplus
}
#endif
