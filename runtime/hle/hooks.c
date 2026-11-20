// Hook points: instructions of the game's code that call into the runtime (tools/patches.py HOOKS). The generated
// code at a hook site runs `rt_hook(c, site)` first; what a hook does lives here, so changing it rebuilds the app,
// not the game code.
//
// rt_hook returns 0: run the site's own instruction, 1: the hook did it all, 2..: jump to the site's targets
// (HOOKS[site][1], in order).
//
// DKCR has no hook sites yet (Link's Awakening's were removed with its features). To add one: name its address in
// runtime/game_addresses/patches_dkcr.h, list it in tools/patches.py HOOKS (with the instruction word it checks), and
// handle it below by comparing site with GA(NAME).
#include "../game_addresses.h"
#include <stdint.h>

#include "../cpu.h"
#include "../rt.h"

int rt_hook(Cpu *c, uint64_t site) {
    (void)c;
    LOG("[hook] no hook handles site %llx: running its own instruction\n", (unsigned long long)site);
    return 0;
}
