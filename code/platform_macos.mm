// macOS platform layer, included by platform.cpp (compiled as Objective-C++ on macOS).
//
// Port status: stub. It is enough to load the engine and run headless tasks such as
// `ilu --build-assets`. The window, input, gamepad, file watching and audio are not
// implemented yet, so running the engine interactively stops at InitializeWindow.

// AppKit is imported by ilu_core.h, see the comment there.

// Cocoa owns the main thread event loop, so the engine updates and renders on its own
// thread, like on Windows.
#define USE_UPDATE_THREAD 1
#define USE_AUDIO_THREAD 1
#define USE_ENGINE_HOT_RELOAD 1


////////////////////////////////////////////////////////////////////////////////////////////////////
// Window types

struct WindowImpl
{
	// TODO(macOS): NSWindow, NSView and its CAMetalLayer
	int unused;
};

static WindowImpl windowImpl;

static const char *engineLibFilename = "engine.so";
static const char *engineLibTmpFilename = "engine.tmp.so";


////////////////////////////////////////////////////////////////////////////////////////////////////
// Directories

static bool IsAbsolutePath(const char *path)
{
	const bool res = *path == '/';
	return res;
}

static void InitializeDirectories(Platform &platform)
{
	char buffer[MAX_PATH_LENGTH];
	char *workingDir = getcwd(buffer, ARRAY_COUNT(buffer));
	InitializeDirectoriesFromWorkingDir(platform, workingDir);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Window and events

static bool InitializeWindow(Window &window, u32 width, u32 height, const char *title)
{
	ZeroStruct(&window);
	window.width = width;
	window.height = height;

	ZeroStruct(&windowImpl);
	window.impl = &windowImpl;

	// TODO(macOS): Create the NSApplication, NSWindow and a CAMetalLayer-backed NSView
	LOG(Error, "InitializeWindow: Not implemented on macOS yet\n");
	return false;
}

static void CleanupWindow(Window &window)
{
	// TODO(macOS)
}

static void ShowPlatformWindow(Window &window)
{
	// TODO(macOS)
}

static void PlatformWakeMainThread()
{
	// TODO(macOS): Post an NSEvent so the main thread leaves PlatformUpdateEventLoop
}

static void PlatformUpdateEventLoop(Platform &platform)
{
	// TODO(macOS): Pump NSApplication events. Sleep meanwhile so the main loop doesn't spin.
	SleepMillis(10);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// File watching

static bool WatchDirectory(Platform &platform, const char *path, bool recursive)
{
	// TODO(macOS): kqueue (EVFILT_VNODE) on the directory
	return false;
}

static void UpdateFileWatcher(Platform &platform)
{
	// TODO(macOS)
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamepad

static bool InitializeGamepad(Platform &platform)
{
	// TODO(macOS): GameController.framework
	return false;
}

static void UpdateGamepad(Platform &platform)
{
	// TODO(macOS)
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Audio

static bool InitializeAudioDevice(Platform &platform)
{
	// TODO(macOS): Core Audio output unit. Leaving the state uninitialized runs without audio.
	LOG(Warning, "- Audio not implemented on macOS yet\n");
	return platform.audio.state != AudioStateUninitialized;
}

static void WaitForAudioDevice(Platform &platform)
{
	// TODO(macOS)
}

static void UpdateAudioDevice(Platform &platform)
{
	// TODO(macOS)
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int argc, char **argv)
{
	@autoreleasepool
	{
		Main(argc, argv);
	}
	return 0;
}
