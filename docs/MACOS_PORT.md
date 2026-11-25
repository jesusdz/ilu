# macOS port plan

Incremental steps to port ilu to macOS. The Makefile keeps working on Linux (OS detected with
`uname -s`). The macOS platform layer is `code/platform_macos.mm`, included by `platform.cpp`,
which is compiled as Objective-C++ on macOS.

| Step | Description | Status |
|------|-------------|--------|
| 1 | Build the command-line tools on macOS | Done |
| 2 | POSIX fixes in `ilu_core.h` (`PLATFORM_POSIX`, dispatch semaphores, ...), DXC path (`dxc/macos`) | Done |
| 3 | Stub `platform_macos.mm`: compiles, loads the engine dylib, `ilu --build-assets` runs headless | Done |
| 4 | Cocoa window: NSWindow + CAMetalLayer-backed view, `USE_UPDATE_THREAD 1`, Retina at 1:1 pixels | Done |
| 5 | Vulkan via MoltenVK: `VK_EXT_metal_surface`, portability enumeration, `VK_KHR_portability_subset` | Done |
| 5b | Full screen without the scaled-mode shrink (see below) | Done, switch path to verify |
| 6 | Input: keyboard (kVK codes), mouse (pixels, Y flipped), GameController.framework gamepad | Done, gamepad to verify |
| 7 | Audio: Core Audio | Done |
| 8 | File watching (FSEvents, since kqueue misses in-place saves) and hot-reload checks | Done |
| 9 | Packaging as a `.app` (Info.plist, `-mmacosx-version-min`, bundled MoltenVK and DXC) | |
| 10 | Backend-agnostic `ilu_gfx.h`, a first step towards a native Metal backend (see below) | Done, paths to verify |


## Development setup

Vulkan comes from Homebrew (the LunarG Vulkan SDK, installed in `/usr/local/lib`, works too):

```
brew install vulkan-loader molten-vk vulkan-validationlayers
```

- The engine looks for the loader as `libvulkan.1.dylib` (next to the executable or in the default
  search paths), then in `/opt/homebrew/lib`.
- Homebrew's validation layer manifest names its library without a path. `make` (build_and_run)
  runs ilu with `DYLD_FALLBACK_LIBRARY_PATH=/opt/homebrew/lib` so it can be loaded. Running
  `build/ilu` directly works too, but without validation (a warning is logged).


## Units

Window sizes and every position sent to the engine are in pixels, like on Windows (1:1 pixels,
OS display scaling ignored). AppKit works in points, converted with the window's backing scale
factor (2 on Retina modes).


## Step 5b: Full screen without the shrink

### Problem

In a scaled display mode, macOS renders the desktop at 2x the "looks like" size and then shrinks
the whole framebuffer to the panel's native size. For example, on a 3840x2160 panel set to "looks
like 2560x1440", everything is rendered at 5120x2880 and shrunk to 75%. The engine pays for 78%
more pixels than the panel has, and the image is resampled (slightly soft).

Only display modes whose pixel size equals the panel's native size avoid it:
- the 2x mode "looks like" native/2 (e.g. 1920x1080 on a 4K panel), macOS's default, and
- the 1x native mode (e.g. 3840x2160 at 1x).

### Goal

When ilu goes full screen, it shows 1:1 native panel pixels **whatever display mode the user has
selected**. If the current mode is already native-sized, nothing changes. Otherwise ilu switches
the display to a native-sized mode while it is full screen, and restores the user's mode
afterwards.

Windowed mode is out of scope: the shrink applies to the whole desktop, so avoiding it there would
mean changing the user's display mode while ilu runs in a window.

### Approach

1. Find the display the window is on (`NSScreen` → `CGDirectDisplayID` via
   `deviceDescription[@"NSScreenNumber"]`).
2. Find the panel's native pixel size. That's the mode with the native flag
   (`kDisplayModeNativeFlag` in `CGDisplayModeGetIOFlags`), or else the largest 1x mode.
3. If the current mode's pixel size (`CGDisplayModeGetPixelWidth/Height`) already equals the
   native size, do not switch.
4. Otherwise pick the target mode from `CGDisplayCopyAllDisplayModes`, using
   `kCGDisplayShowDuplicateLowResolutionModes` so that the 2x modes are listed:
   - pixel size == native size,
   - prefer the 2x mode (keeps the menu bar and system UI readable), then 1x,
   - prefer the current refresh rate, then the highest.
5. Save the current mode, then switch with `CGBeginDisplayConfiguration`,
   `CGConfigureDisplayWithDisplayMode`, and
   `CGCompleteDisplayConfiguration(..., kCGConfigureForAppOnly)`. App-only means macOS restores the
   mode itself if ilu exits or crashes.
6. Cover the display with a borderless window (like on Windows) instead of AppKit's full-screen
   Space, whose animation doesn't combine well with a mode switch. The menu bar and Dock are hidden
   with `NSApp.presentationOptions`, which macOS undoes by itself while another app is active.
   Every full-screen request ends up in `IluWindow`'s `toggleFullScreen:` override: the green
   button, View > Toggle Full Screen (Ctrl+Cmd+F) and the system shortcuts.
7. Restore the saved mode:
   - when leaving full screen,
   - when the app resigns active (Cmd+Tab, Mission Control). Full screen is applied again when it
     becomes active.

   The window first gets its size and place from before full screen, then the mode is restored.
   Otherwise it would cover the display in the user's mode (5120x2880 pixels in "looks like
   2560x1440"), too big for the engine's render targets (256 MB heap, light grid up to 4096x2560).
8. The engine gets the new size through the normal `WindowResize` event, and recreates the
   swapchain.

### Things to verify

- The blank-screen time while the monitor resyncs (about 1-2 s per switch). Avoid switching back
  and forth unnecessarily.
- Other windows are restored to their place after leaving full screen.
- Multiple displays: only the display ilu is on changes.
- Cmd+Tab out and back, Cmd+Q and crashes while full screen restore the user's mode.
- Displays whose current mode is already native-sized: no switch, no blank screen.
- Vsync and frame pacing at the panel's refresh rate (160 Hz on the M27UP).

### Later (optional)

A setting to keep the user's mode (no switch) for people who prefer to avoid the blank screen.
In that case, the drawable can be rendered at the native size and let macOS resample it, as a
cheaper but softer option.


## Step 10: Backend-agnostic graphics API

The engine no longer sees Vulkan. `ilu_gfx.h` has no Vulkan types or headers (`engine.cpp` fails
to compile if `VK_VERSION_1_0` gets defined), all Vulkan structs live in `ilu_gfx_vulkan.cpp`, the
device is opaque (the `gpu` macro gets it), and resources are handles from a `HandlePool` each.

### Small cleanups

- The header comment still says "made on top of Vulkan", and its function list is out of date.
- Unused API: `WaitQueueIdle`, `InsertDebugLabel`, `ReadTimestamp`, `IsSamePipeline`,
  `IsSwapchainImage`, `ImageUsageStorage`/`Transient`/`InputAttachment`,
  `BufferUsageUniformTexelBuffer`, `BufferStateIndirectArgs`,
  `Alignment::optimalBufferCopyRowPitch`.
- Backend policy decided by the engine: it picks heaps by name and calls
  `ResetHeap(HeapType_RTs)` itself, and `GetSwapchainPreRotation` (Android's Vulkan pre-transform)
  reaches the camera code.

### Only with a second backend

- Render passes and framebuffers follow Vulkan 1.0. Metal, D3D12 and Vulkan dynamic rendering
  describe the attachments when a pass begins instead.
- Bind group allocators are Vulkan descriptor pools. They could hide behind persistent and
  per-frame bind groups.
- `platform.cpp` includes `ilu_gfx_vulkan.cpp` unconditionally, so selecting a backend needs a
  build or platform switch.
- Shaders are loaded as SPIR-V (`.spv`). Metal needs MSL (SPIRV-Cross, for instance).

### Things to verify

- Linux, Windows and Android builds, not compiled since `handle_pool.h` and the `gpu` macro.
- Quitting, window resize, removing a material and shader hot reload, not run since the handle
  pools (destruction now happens when the pools are compacted, at the end of the frame).
