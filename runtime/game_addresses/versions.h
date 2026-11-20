// The supported game versions, by the main program's build ID (exefs/main, 0x40..0x53). Each one has its own
// address tables: game_addresses/patches_<key>.h and runtime_<key>.h, with the same names. An unknown build ID is
// refused: there is no fallback table. tools/game_addresses.py reads this same file.
// An X-macro list (no include guard): GA_VERSION(key, label, main build ID) and
// GA_RELEASE(key, label, SHA-256 of exefs/main.npdm).
//
// Donkey Kong Country Returns HD. Its main build ID is 16 bytes, zero-filled to the field's 20.
// (The Link's Awakening tables, patches_v1_0_*.h and runtime_v1_0_*.h, are no longer used.)

GA_VERSION(dkcr, "DKCR HD", "991ECCF3339153E469F15936FAC59C5200000000")
