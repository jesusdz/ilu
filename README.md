# ilu

A home-made game engine written from scratch in C++, with a Vulkan renderer and minimal external dependencies. It's a solo, early-stage project built for learning and for shipping small games, not a general-purpose engine for others to adopt (but who knows).

Supported platforms: Windows, Linux, and Android.


## Engine features

* Hot-reloadable engine: engine code (`engine.cpp`) lives in a DLL/SO that the main executable/platform (`platform.cpp`) can reload without restarting
* Vulkan-based renderer supporting both 2D (sprites, grids) and 3D (meshes, shadow mapping, sky) rendering
* HLSL shaders compiled to SPIRV via DXC, with reflection utilities to auto-generate descriptor set bindings
* Scene graph with rooms, layers, entities, materials, and textures
* Immediate-mode UI, also used to build an in-engine editor (scene/entity/material/texture/audio inspection, texture and pipeline reloading, save/load of scenes as text or binary)
* Audio playback: WAV clips plus tracker music (MOD/S3M/XM) via the bundled `ibxm` player
* Data-driven assets with a reflection-based pipeline: asset descriptions are parsed and reflected into C structs, then serialized to a binary format the engine loads at runtime


## Repository layout

The code is organized in three layers:

1. **Single-header libraries** (`code/*.h`) — reusable, dependency-light building blocks the rest of the engine is built on.
2. **Core program** — the engine itself (`platform.cpp`, `engine.cpp`) and the several modules (`data.cpp`,`audio.cpp`, etc).
3. **Game** — gameplay code (`game.cpp`) built on top of the engine.


## Single-header libraries

### ilu_core.h

Platform and utility layer: platform identification, assertions/logging, sized type aliases, intrinsics, strings, memory (linear arena allocators, virtual memory abstraction), file reading, process execution, dynamic library loading, math, and clock/timing.

### ilu_gfx.h

Abstraction of a modern graphics API, currently implemented on top of Vulkan.

### ilu_ui.h

Immediate-mode UI library built on `ilu_gfx.h`. Used both in-game and to build the engine's editor.

### ilu_spirv.h

Utilities to parse SPIRV shader modules and extract descriptor set layouts, so client code can automate descriptor/binding creation instead of hand-writing it.

### ilu_profile.h

Lightweight instrumentation/profiling macros for timing code blocks and frames, with multi-threading support.


## Building

`make` builds on both platforms: `make.bat` on Windows, the `Makefile` on Linux. Both take the same target names.

```
# Windows (requires Visual Studio; vcenv.bat auto-detects it)
make
build\ilu.exe

# Linux (builds and runs)
make
```

On Windows, `make.bat` sets up the Visual Studio environment the first time it runs in a terminal and reuses it afterwards. From PowerShell, type `.\make`.

Main targets:

* `ilu` (default on Windows): builds `reflex` if it's missing, regenerates `code/reflex.generated.h`, then builds the executable and the engine DLL/SO
* `dll`: builds `reflex` if it's missing, regenerates `code/reflex.generated.h`, then rebuilds only the engine DLL/SO, for hot reload
* `data`: builds `ilu`, then runs `ilu --build-assets` to turn `assets/assets.txt` into `build/assets.dat`
* `reflex`: builds the reflection generator
* `clean`: deletes `build/`
* `build_and_run`, `build_and_debug` (Linux only, `make` defaults to `build_and_run`)

Shaders are compiled into `build/shaders.dat` by the engine itself when the file is missing, using the vendored DXC.

For unit tests specifically, see [docs/TESTS.md](docs/TESTS.md). The tools in `code/misc/` each have a target of the same name, see [docs/MISC.md](docs/MISC.md).


## Library dependencies

To the greatest extent possible, the engine avoids heavy external dependencies. Libraries it depends on are vendored directly in this repository as lightweight source files compiled alongside the project:

* [IBXM](https://github.com/martincameron/micromod) (player library for ProTracker MOD, Scream Tracker 3 S3M, and FastTracker 2 XM music formats, by Martin Cameron)
* [offset_allocator](https://github.com/sebbbi/OffsetAllocator) (fast, hard-realtime O(1) offset allocator with minimal fragmentation, by Sebastian Aaltonen)
* [STB libraries](https://github.com/nothings/stb) (public domain single-header libraries by Sean Barrett)
	* **stb_image:** Image loader
	* **stb_truetype:** TrueType font rasterizer
	* **stb_rect_pack:** Rectangle packer


## Tool dependencies

* **C++ compiler:** any compiler supporting C++20. The codebase is mostly C, with a light layer of C++ on top (operator overloads and RAII objects here and there).
* [DXC](https://github.com/microsoft/DirectXShaderCompiler) (the DirectX Shader Compiler binary and library, used to compile HLSL shaders to SPIRV) — vendored in this repository for both Windows and Linux, so no external install is needed.
* [Android SDK](https://developer.android.com/studio) (tools and packages required to build and deploy `.apk` files) — not vendored, install separately; see [android/README.md](android/README.md) for setup.


## Further reading

* [docs/TESTS.md](docs/TESTS.md) — unit tests
* [docs/MISC.md](docs/MISC.md) — experimental/misc tools in `code/misc/`
* [docs/LINKS.md](docs/LINKS.md) — interesting links collected while writing this engine


## License

All code here is available to anybody free of charge, without any legal obligation, under the [Unlicense](./LICENSE) terms (no terms, basically). That said, if this engine or part of it ends up in third-party projects, attribution is happily appreciated.
