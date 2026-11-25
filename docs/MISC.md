# Experimental / misc tools

`code/misc/` holds small side projects and tech tests that are independent from the engine itself — one-off tools, learning exercises, and pieces used to prototype ideas before (or instead of) folding them into the engine proper.

* `main_interpreter`: A scripted language interpreter, following the *Crafting Interpreters* book by Robert Nystrom.
* `main_d3d12`: A graphics application template using the D3D12 API.
* `main_atof`: A custom implementation of `atof` (ASCII to float).
* `main_spirv`: A standalone SPIRV parser, precursor to `ilu_spirv.h`.
* `main_gamepad`: Linux gamepad input read straight from evdev (`/dev/input/event*`), precursor to the gamepad support in `platform_linux.cpp`.
* `main_alsa`: Linux audio output through ALSA, playing a sine wave, precursor to the audio support in `platform_linux.cpp`.
* `main_cast`: Prints the AST that `code/reflex/cast.h` builds for a C file, to debug the parser behind `reflex`.
