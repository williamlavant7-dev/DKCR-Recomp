#!/bin/bash
# Windows (MSYS2 UCRT64): a folder to play from by double-clicking dkcrrecomp.exe, from this checkout's build.
#   tools/windows_play_folder.sh <game files folder> [out folder, default "build/DKCR Recomp"]
# The game files folder is the one with exefs/ and romfs/ in it. Run `make` first (the game code, build/dkcrgame.dll).
# The out folder gets: a release exe (no console window; the log is dkcrrecomp.log next to it), the game code, the
# DLLs both load (MSYS2's: for this machine, not for handing out - `make dist` is the release), the fonts and icon,
# and extracted/ with links (junctions) to the game files' exefs/ and romfs/ - nothing is copied or changed there.
# It is the data folder too (runtime/paths.h): saves and settings stay in it; the first time, those of build/ (where
# `build/dkcrrecomp.exe` keeps them) are copied in. Run it again after a new build: everything else is kept.
set -euo pipefail
[ $# -ge 1 ] || { echo "usage: $0 <game files folder> [out folder]"; exit 1; }
GAME=$1
OUT=${2:-build/DKCR Recomp}
[ -f "$GAME/exefs/main" ] && [ -d "$GAME/romfs" ] || { echo "$GAME has no exefs/main and romfs/"; exit 1; }
[ -f build/dkcrgame.dll ] || { echo "no build/dkcrgame.dll: run make first"; exit 1; }

make -j"$(nproc)" B=build/win-play RELEASE=1 build/win-play/dkcrrecomp.exe
mkdir -p "$OUT"
cp build/win-play/dkcrrecomp.exe build/dkcrgame.dll build/dkcrgame.dll.id "$OUT/"
# the DLLs the exe loads from MSYS2 (vulkan-1.dll comes with the graphics driver)
ldd build/win-play/dkcrrecomp.exe | awk '(/ucrt64/ || /clangarm64/) && !/vulkan-1/ {print $3}' | sort -u | while read -r f; do cp -u "$f" "$OUT/"; done
cp docs/icon/icon_1024.png "$OUT/icon.png"
cp third_party/fonts/Baloo2/Baloo2-SemiBold.ttf third_party/fonts/DejaVu/DejaVuSans.ttf \
   third_party/fonts/DejaVu/DejaVuSans-Bold.ttf "$OUT/"

# the game files: linked, not copied
mkdir -p "$OUT/extracted"
for d in exefs romfs; do
    [ -e "$OUT/extracted/$d" ] || powershell.exe -NoProfile -Command "New-Item -ItemType Junction -Path '$(cygpath -aw "$OUT/extracted/$d")' -Target '$(cygpath -aw "$GAME/$d")'" >/dev/null
done

# saves and settings: build/'s, the first time only
[ -e "$OUT/save" ] || { [ -d build/save ] && cp -R build/save "$OUT/save"; } || true
[ -e "$OUT/larecomp.ini" ] || { [ -f build/larecomp.ini ] && cp build/larecomp.ini "$OUT/"; } || true
echo "$OUT/dkcrrecomp.exe"
