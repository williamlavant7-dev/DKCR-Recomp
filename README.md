# DKCR Recomp

> [!IMPORTANT]
> **This project was made almost entirely with AI.** Nearly all of the code, reverse engineering and documentation
> here was written by an AI (Anthropic's Claude, through Claude Code), directed, tested and reviewed by me. I made
> this for myself as a fan of the game, and sharing it is a courtesy. If you don't trust code written by AI, that is
> your right; you can simply ignore this project. As with anything on the internet, you run it at your own discretion.

**DKCR Recomp** is a static recompilation of *Donkey Kong Country Returns HD* (Nintendo Switch, 2025) into a native
Windows application. The game's code is translated ahead of time into native code, built on your own machine from
your own game files, and runs with its graphics on Vulkan.

> [!NOTE]
> **Status: early beta, Windows only for now.** The title screen, the world map and the levels play at 60 fps.
> There is no download to play yet: setting up your game files still needs building from source (below). A
> ready-to-play download comes once the start screen can set up DKCR's files for you.

## What works

- Boots to the title screen, the world map and the levels; plays at 60 fps on a modern PC.
- Controllers through SDL 3, including DualSense and Xbox controllers.
- Saves, the HUD and the menus.
- Video recording: **F8** starts and stops it (or Settings > Recording). Videos (MP4, with sound, without the menu)
  go to the `recordings` folder; quality, frame rate and sound are in the Recording tab.

Known issues:
- Some objects can pop in for a moment the first time a level starts, while its shaders are prepared. It gets better
  the more you play: prepared shaders are kept in the `cache` folder.
- Only Windows is tested. The macOS, Linux, Android and iOS code from LAS Recomp is still here, renamed but untested.

## System requirements

- Windows 10 or 11, x86-64, a CPU with AVX2 (Intel Haswell / 2013 or newer, AMD Zen or newer)
- A GPU with Vulkan 1.3 (NVIDIA, AMD or Intel, with a current driver)
- Your own game files of *Donkey Kong Country Returns HD* for Switch: the `exefs` and `romfs` folders

## Building and playing (Windows)

1. One-time setup, in PowerShell from the repository's folder:
   `powershell -ExecutionPolicy Bypass -File tools\setup_windows.ps1`. It installs MSYS2 (clang, make, SDL 3,
   FFmpeg, Vulkan, shaderc) and Python 3.12 for the tools.
2. Put your game files where the build finds them: `extracted/exefs` and `extracted/romfs` in the repository's
   folder (or pass `GAME=<folder>` to `make gen`).
3. From an MSYS2 UCRT64 shell (or Git Bash with `export PATH="/c/msys64/ucrt64/bin:/c/msys64/usr/bin:$PATH"`):
   `make gen` (recompiles the game's code from your files into `generated/`), then `make -j`.
4. `tools/windows_play_folder.sh <your game files folder>` puts together a **DKCR Recomp** folder beside the
   repository: `dkcrrecomp.exe`, the game code and every DLL it needs, with links to your game files. Double-click
   `dkcrrecomp.exe` to play. Run the script again after each build; your saves and settings stay in that folder.

More detail: [docs/BUILD-windows.md](docs/BUILD-windows.md).

Where files live: everything (saves, settings in `dkcrrecomp.ini`, the shader cache) is in the **DKCR Recomp** folder,
next to `dkcrrecomp.exe`. For a bug report, attach `dkcrrecomp.log` from that folder.

## Credits

**LAS Recomp** - DKCR Recomp is built on [LAS Recomp](https://github.com/peiikai/LASRecomp) by **Peiikai**, the
static recompilation of *The Legend of Zelda: Link's Awakening* for Switch: its recompiler, its Switch host and its
NVN graphics on Metal and Vulkan are what this project adapts to *Donkey Kong Country Returns HD*. Huge thanks to
Peiikai and LAS Recomp's contributors, listed here as in LAS Recomp's own credits.

**LAS Recomp's contributors** (GitHub handles, in order of first contribution):
- **[@dvilelaf](https://github.com/dvilelaf)** - the System language setting (#15), a packaging fix (#10) and the
  level-transition crash fix (#34)
- **[@Tensorlong](https://github.com/Tensorlong)** - the fix for the white Link on M1 Macs (#18)
- **[@mansarip](https://github.com/mansarip)** - support for unrecognised controllers (#26)
- **[@we11adam](https://github.com/we11adam)** - the right font for each language (#27) and the Chinese language
  codes (#25)
- **[@Maganra](https://github.com/Maganra)** - faster graphics checks on Linux (#42)
- **[@hjqcan](https://github.com/hjqcan)** - game addresses in one place (#57), settings and Traditional Chinese fixes
  (#53), much less memory used on Windows (#107), Vulkan fixes (#108, #127, #128), an x86 rounding fix (#115), movie
  memory fixes (#112), Windows logs (#114), resumable game-code builds (#56), and save fixes (#169, #170, #182)
- **[@gitarre1234rr5](https://github.com/gitarre1234rr5)** - controller rumble (#104) and a regression test (#88)
- **[@rkammoun](https://github.com/rkammoun)** - the fix for the black walls in the Ukuku Prairie cave on Mac (#215, #216)
- **[@NevesHenry](https://github.com/NevesHenry)** - a clear message on Linux when no file window can open (#298),
  the rumble test running in CI (#299), Linux builds finding their own SDL3 (#437), font re-imports that no longer
  empty the font (#490), a message box on Linux for a CPU without AVX2 (#491), the build and system in the log bot's
  summary (#582), an FFmpeg licence check that really checks (#583), the GPU driver in the log bot's summary (#588),
  each attached log counted once (#589), the game files check in the log bot's summary (#600), summaries for
  logs attached by editing a comment (#601), a Linux AppImage build that stops when its container fails (#653), and
  a zig download that starts over after being cut off (#667)
- **[@ky1vstar](https://github.com/ky1vstar)** - a much faster Linux ARM release build (#319), touch menus and on-screen controls for phones (#634), the frame interpolation presenter on CADisplayLink (#649), one shared macOS and iOS host layer (#685), the iOS cleanup after #238 (#697), iOS fixes for saves, errors and Files (#690), a safer, simpler iOS kit export (#704), seed generation on iOS that shows its progress and handles room shuffle re-runs (#735), a clear message on iPhones and iPads that can't run the game (#761), checksums for every iOS download (#762), and a leaner iOS CI job (#763)
- **[@theb0rg](https://github.com/theb0rg)** - the fix for AMD integrated graphics on Windows (#335)
- **[@gtf-dot](https://github.com/gtf-dot)** - the game-text font on Linux (#330)
- **[@cfcosta](https://github.com/cfcosta)** - full speed on Linux AMD handhelds like the Steam Deck and ROG Ally (#392)
- **[@gsel001](https://github.com/gsel001)** - a check in CI that the Mac app starts (#420), and rupees up to 9,999 in the
  cheats and save editor (#453)
- **[@wimw85](https://github.com/wimw85)** - much faster game code setup on the Steam Deck and other PCs (#434)
- **[@missionfloyd](https://github.com/missionfloyd)** - a much smaller game code build, without debug info (#487)
- **[@wolfchimneyrock](https://github.com/wolfchimneyrock)** - full speed on ARM Mali graphics (#527), the CPU and GPU working in parallel on Mali (#529), and faster render passes on Mali (#528, #534), a cheaper FSR upscale (#532), the game on the big cores of ARM and Intel hybrid CPUs on Linux (#533), no more flickering grass and shadows on Mali (#546), the game starting on Snapdragon (Adreno) devices (#561), much faster shaders on Mali and Adreno (#562, #563, #579), faster graphics memory on Mali with Mesa (#564), GPU tuning tools for Vulkan (#565), no more memory leak on Arm's Mali driver (#607), smaller shader stacks that keep 4 GB Mali boards from running out of memory (#645), and less shader scratch memory on PanVK (#646)
- **[@bgevko](https://github.com/bgevko)** - an option to keep the area's music during Guardian Acorns and Pieces of Power (#642)

**Mobile ports** - special thanks to the people who brought the game to phones and tablets:
- Android: **[@Nycolazs](https://github.com/Nycolazs)** ([LASRecomp-Android-Plus](https://github.com/Nycolazs/LASRecomp-Android-Plus))
  and **[@mamcjr](https://github.com/mamcjr)** ([LASRecompAndroid](https://github.com/mamcjr/LASRecompAndroid)), the
  Android ports our Android app is built from
- iOS: **[@ky1vstar](https://github.com/ky1vstar)** - the game on iPhone and iPad (#238)

Thanks also to everyone who reports issues and tests builds. Your logs and screenshots find most of the bugs.

**LAS-Randomizer** - the randomizer at the heart of this project's randomizer features, vendored (with local
changes) in `third_party/LAS-Randomizer` under the MIT license. Huge thanks to its authors and community:
- **Owen_Splat** - LAS-Randomizer (https://github.com/Owen-Splat/LAS-Randomizer)
- **Glan** - created the original early builds of the randomizer (https://github.com/la-switch/LAS-Randomizer)
- **ProfessorLaw** - additional programming
- and everyone who has reported bugs, given feedback or contributed to LAS-Randomizer

**Community mods** - some options re-implement ideas from these GameBanana mods (re-implemented in the recomp;
none of their files are used):
- **360 Controls (And More!)** by Owen_Splat - 360-degree movement, blur removal (blur removal patch by theboy181)
  (https://gamebanana.com/mods/659802)
- **XBOX UI** by fellow_demo - Xbox button prompts (https://gamebanana.com/mods/540069)
- **PS4 UI Buttons** by messiahgov - PlayStation button prompts (https://gamebanana.com/mods/324458)
- **No Hero Mode Indicator** by Boo/Calla - hide the Hero Mode icon (https://gamebanana.com/mods/441172)

**Art**
- **[Kenney](https://kenney.nl)** - the D-pad on the HUD for D-pad items, from the Input Prompts pack (CC0,
  `third_party/kenney`)

**Libraries and tools**
- [SDL 3](https://libsdl.org) - windowing, input, audio (zlib license)
- [FFmpeg](https://ffmpeg.org) - movie playback (LGPL)
- [Dear ImGui](https://github.com/ocornut/imgui) - overlay menu and start screen (MIT)
- [Baloo 2](https://github.com/EkType/Baloo2) by Ek Type - the start screen's font (SIL Open Font License 1.1, `third_party/fonts/Baloo2/OFL.txt`)
- [BIZ UDPGothic](https://github.com/googlefonts/morisawa-biz-ud-gothic) by Morisawa - the game's text on Linux when you haven't imported the Switch fonts: a free font from the same universal-design gothic family as the Switch's UD Shin Go (SIL Open Font License 1.1, `third_party/fonts/BIZUDPGothic/OFL.txt`)
- [DejaVu Sans](https://dejavu-fonts.github.io) - the in-game menu and button letters when no system font fits, and the game text's last resort (Bitstream Vera / DejaVu license, `third_party/fonts/DejaVu/LICENSE`)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - Windows and Linux renderer (MIT)
- [SMAA](https://github.com/iryoku/smaa) by Jorge Jimenez, Jose I. Echevarria, Belen Masia, Fernando Navarro and Diego
  Gutierrez - the anti-aliasing option (MIT, `third_party/smaa/LICENSE.txt`)
- [AMD FidelityFX Super Resolution 1](https://github.com/GPUOpen-Effects/FidelityFX-FSR) - the Windows and Linux upscaler
  option, ported to GLSL (MIT)
- [oead](https://github.com/zeldamods/oead) and [evfl](https://github.com/zeldamods/evfl) - Nintendo file formats, used by LAS-Randomizer
- [PyInstaller](https://pyinstaller.org) - packs the seed generator into a standalone helper (GPL 2.0+ with the
  bootloader exception, which allows distributing the packed program). The helper bundles CPython (PSF License),
  oead / evfl (GPL 2.0+), [Keystone](https://www.keystone-engine.org) (GPL 2.0; Python bindings BSD), quicktex
  (LGPL 3), Pillow (MIT-CMU), PyYAML (MIT) and appdirs (MIT)
- The iOS app has no helper: it embeds [BeeWare's](https://beeware.org) build of CPython 3.13 for iOS (PSF License;
  with bzip2, libffi, mpdecimal, OpenSSL and XZ for the standard library) and the same generator packages, built for
  iOS, inside the app, and links SDL 3 and FFmpeg statically. Their licence texts ship in the app (`LICENSES/`).
- [Zig](https://ziglang.org) - its C compiler (`zig cc`: Clang and LLD) builds the game code on the player's
  machine during setup (MIT; LLVM, Clang, LLD: Apache 2.0 with LLVM exceptions)
- The game code generator (`tools/gen.py`) is packed by PyInstaller into a standalone helper too. It bundles CPython,
  [Capstone](https://www.capstone-engine.org) (BSD 3-clause; disassembly), [Unicorn](https://www.unicorn-engine.org)
  (GPL 2.0; Python bindings BSD; runs small pieces of code to resolve jump tables) and
  [python-lz4](https://github.com/python-lz4/python-lz4) (BSD 3-clause, with the LZ4 library: BSD 2-clause)
- [shaderc](https://github.com/google/shaderc) - compiles the translated shaders for Vulkan on Windows, Linux and
  Android (Apache 2.0; with glslang and SPIRV-Tools)
- The Android app, ported from [LASRecompAndroid](https://github.com/mamcjr/LASRecompAndroid) by mamcjr (MIT), also
  ships [Python](https://www.python.org) 3.14 for Android (PSF License) to run the game code generator on the device,
  [volk](https://github.com/zeux/volk) by Arseny Kapoulkine (MIT) to load Vulkan, and
  [libadrenotools](https://github.com/bylaws/libadrenotools) and
  [liblinkernsbypass](https://github.com/bylaws/liblinkernsbypass), Copyright (c) 2021, Billy Laws (BSD 2-Clause; their
  licence text ships in the app), to load a custom Vulkan driver such as Turnip (not included)
- [envytools](https://github.com/envytools/envytools) - NVIDIA GPU documentation used for the shader translator
- An MIT-licensed open-source project's audio renderer and shader instruction decoders (license and copyright notice:
  `third_party/ryujinx/LICENSE.txt`) - the audio renderer's reverb is a port of its reverb code and the mixer is
  modelled on its renderer; its instruction decoders were the bit-layout reference for parts of the shader translator
- [XC2-Recomp-Contributions](https://github.com/circerio/XC2-Recomp-Contributions) by circerio (MIT) - the
  branch-phi test shader in the shader translator's control-flow test (`tools/test_translate_flow.c`)
- **Dusklight** by the TwilitRealm team (https://github.com/TwilitRealm/dusklight) - design inspiration for the start
  screen (Play / Settings / System layout)
- **SuperMarioWonderRecomp** by BelmanteGu (https://github.com/BelmanteGu/SuperMarioWonderRecomp) - its
  ahead-of-time shader approach is what the first-launch shader preparation follows (a shipped list of shader variants,
  built on the player's machine from their own game files)
- The Switch homebrew community's documentation ([Switchbrew](https://switchbrew.org))

## Legal

DKCR Recomp is a fan project. It is not affiliated with, endorsed by or sponsored by Nintendo. *Donkey Kong* and
*Donkey Kong Country Returns* are trademarks of Nintendo. No game files, game code, assets or fonts are included - not in
this repository and not in any release. You must provide your own legally obtained copy of the game files to play.

## License

DKCR Recomp's own code, and the LAS Recomp code it is built on (the recompiler tools, the runtime), is open source
under the [MIT License](LICENSE). Third-party components keep their own licenses: LAS-Randomizer (MIT), ported audio renderer code (MIT, see `third_party/`), Dear ImGui (MIT),
Vulkan Memory Allocator (MIT), SMAA (MIT), AMD FSR 1 (MIT), the Baloo 2 and BIZ UDPGothic fonts (SIL OFL 1.1), the DejaVu Sans font (Bitstream Vera / DejaVu license), SDL 3 (zlib), FFmpeg (LGPL 2.1+, linked dynamically, and statically in the iOS app), the
Zig compiler (MIT; its LLVM / Clang / LLD: Apache 2.0 with LLVM exceptions; its C library headers keep their own
licenses), the iOS kit's Mach-O linker (LLD as built by the Rust project: Apache 2.0 with LLVM exceptions), miniz
(MIT: the iOS kit and the .ipa it makes), and the generator helpers' bundled libraries listed under Credits (oead / evfl / Keystone in the seed
generator and Unicorn in the game code generator are under the GPL, versions in the table below: each helper is a separate program the game runs, and its
sources are this repository plus those projects' published sources). The iOS app is the exception: iOS runs no helper
processes, so it embeds CPython and the seed generator's packages - oead, evfl and Keystone (GPL 2.0+ / GPL 2.0),
quicktex (LGPL 3), Pillow, PyYAML, appdirs - and links SDL 3 and FFmpeg (LGPL 2.1+) statically; the app's `LICENSES/`
folder carries each one's licence text, its sources are this repository plus the versions in the table (the LGPL
libraries can be relinked: `make deps-ios` builds them from source, `make ipa` the app), and the game code library it
embeds is built from the player's own game files and never shared. The license covers DKCR Recomp's code only - it grants
nothing for Nintendo's game, which you must supply yourself.

### Source code of bundled third-party components

Release builds include these components; their source code for the exact versions shipped is available here. The
seed generator's packages are in the desktops' helper and inside the iOS app alike:

| Component | Version | License | Source |
|---|---|---|---|
| Keystone (seed generator; on iOS built with `tools/patches/keystone-0.9.2.patch`) | 0.9.2 | GPL 2.0 (Python bindings: BSD) | https://github.com/keystone-engine/keystone/releases/tag/0.9.2 |
| oead (seed generator) | 1.3.0 | GPL 2.0+ | https://github.com/zeldamods/oead/releases/tag/v1.3.0 |
| evfl (seed generator) | 1.2.0 | GPL 2.0+ | https://github.com/zeldamods/evfl/releases/tag/v1.2.0 |
| quicktex (seed generator) | 0.2.1 | LGPL 3.0 | https://github.com/drewcassidy/quicktex/releases/tag/v0.2.1 |
| Pillow (seed generator; on iOS BeeWare's wheel) | 10.4.0 | MIT-CMU | https://github.com/python-pillow/Pillow/releases/tag/10.4.0, https://pypi.anaconda.org/beeware/simple/pillow/ |
| PyYAML (seed generator) | 6.0.2 | MIT | https://pypi.org/project/PyYAML/6.0.2/ |
| appdirs (seed generator) | 1.4.4 | MIT | https://pypi.org/project/appdirs/1.4.4/ |
| pybind11 (oead's and quicktex's Python bindings, built into them on iOS) | 3.1.0 | BSD 3-clause | https://github.com/pybind/pybind11/releases/tag/v3.1.0 |
| PyInstaller (packs the seed and game code generators) | 6.22.3 | GPL 2.0+ with bootloader exception | https://github.com/pyinstaller/pyinstaller/releases/tag/v6.22.3 |
| CPython (seed and game code generators) | 3.12.14 (macOS), 3.12.10 (Windows), 3.12.15 (Linux), 3.14.8 (Android) | PSF | https://github.com/python/cpython/releases/tag/v3.12.14, https://github.com/python/cpython/releases/tag/v3.12.10, https://github.com/python/cpython/releases/tag/v3.12.15, https://github.com/python/cpython/releases/tag/v3.14.8 |
| CPython for iOS, BeeWare's Python-Apple-support build (the iOS app's seed generator; with bzip2 1.0.8, libffi 3.4.7, mpdecimal 4.0.0, OpenSSL 3.5.9, XZ 5.6.4) | 3.13.16 (b16) | PSF; bzip2, libffi (MIT), mpdecimal (BSD 2-clause), OpenSSL (Apache 2.0), XZ (0BSD / public domain) | https://github.com/beeware/Python-Apple-support/releases/tag/3.13-b16, https://github.com/python/cpython/releases/tag/v3.13.16 |
| Zig (builds the game code during setup; includes LLVM, Clang, LLD) | 0.17.0 | MIT; LLVM: Apache 2.0 with LLVM exceptions | https://ziglang.org/download/0.17.0/zig-0.17.0.tar.xz |
| LLD, as rust-lld (iOS kit: links the iOS game code on each desktop) | Rust 1.91.0 (LLVM 21.1.2) | Apache 2.0 with LLVM exceptions | https://static.rust-lang.org/dist/rustc-1.91.0-src.tar.xz, https://github.com/rust-lang/llvm-project |
| miniz (reads the iOS kit, writes the .ipa) | 3.0.2 | MIT | https://github.com/richgel999/miniz/releases/tag/3.0.2 |
| Capstone (game code generator) | 5.0.9 | BSD 3-clause | https://pypi.org/project/capstone/5.0.9/ |
| Unicorn (game code generator) | 2.1.4 | GPL 2.0 (Python bindings: BSD) | https://pypi.org/project/unicorn/2.1.4/, https://github.com/unicorn-engine/unicorn |
| python-lz4 (game code generator; includes LZ4) | 4.4.5 | BSD 3-clause; LZ4: BSD 2-clause | https://pypi.org/project/lz4/4.4.5/ |
| FFmpeg (LGPL configuration; dynamic on macOS, Windows and Linux, in the Android builds, static in the iOS app: `make deps-ios` relinks it from this source) | 9.0.2 | LGPL 2.1+ | https://ffmpeg.org/releases/ |
| SDL 3 | 3.4.18 (macOS, Windows; iOS, static, with `tools/patches/SDL3-3.4.18-ios27-orientation.patch`), 3.4.16 (Linux, Android) | zlib | https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18, https://github.com/libsdl-org/SDL/releases/tag/release-3.4.16 |
| shaderc (Windows, Linux, Android: GLSL to SPIR-V; Linux, Android: with glslang, SPIRV-Tools inside) | 2026.4 (Windows), 2026.3 (Linux, Android) | Apache 2.0 | https://github.com/google/shaderc |
| libiconv (Windows: used by SDL 3) | 1.19 | LGPL 2.1+ | https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.19.tar.gz |
| GCC runtime: libgcc, libstdc++ (Windows) | 16.2.0 | GPL 3.0 with the GCC Runtime Library Exception | https://gcc.gnu.org/ |
| winpthreads (Windows, mingw-w64) | 14.0.0 | MIT / BSD | https://www.mingw-w64.org/ |
| Baloo 2 font (SemiBold) | 1.640 | SIL OFL 1.1 | https://github.com/EkType/Baloo2 |
| BIZ UDPGothic font (Regular, Linux) | 1.05 (Google Fonts) | SIL OFL 1.1 | https://github.com/googlefonts/morisawa-biz-ud-gothic, https://github.com/google/fonts/tree/main/ofl/bizudpgothic |
| DejaVu Sans font (Book, Bold) | 2.37 | Bitstream Vera license (DejaVu changes public domain) | https://github.com/dejavu-fonts/dejavu-fonts/releases/tag/version_2_37 |
| SMAA (shader and lookup textures, built in) | 2013 (repository master) | MIT | https://github.com/iryoku/smaa |

The exact build scripts that fetch and build these (with checksums) are in this repository: `tools/build_deps_macos.sh`,
`tools/build_deps_windows.sh`, `tools/linux/build_deps.sh`, `tools/build_deps_ios.sh` and `tools/rando/rando_gen.spec`. The Windows build takes SDL 3, shaderc, libiconv and the
compiler runtime from MSYS2's UCRT64 packages (https://packages.msys2.org, which links each package's source).
