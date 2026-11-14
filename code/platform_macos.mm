// macOS platform layer, included by platform.cpp (compiled as Objective-C++ on macOS).
//
// Port status: the window works (creation, resize, Retina, close, quit). Input, gamepad,
// file watching and audio are not implemented yet.
//
// Threading: AppKit must be used from the main thread, which only pumps events here. The
// engine updates and renders on the update thread (USE_UPDATE_THREAD), so window changes
// reach it as platform events, and the functions the update thread calls (ShowPlatformWindow,
// CleanupWindow) hand their AppKit work over to the main thread.
//
// Units: Window::width/height and all positions sent to the engine are in pixels, like on
// Windows (1:1 pixels, no scaling). AppKit works in points, so they are converted with the
// backing scale factor (2 on Retina displays).

// AppKit is imported by ilu_core.h, see the comment there.
#import <QuartzCore/CAMetalLayer.h>

#define USE_UPDATE_THREAD 1
#define USE_AUDIO_THREAD 1
#define USE_ENGINE_HOT_RELOAD 1


////////////////////////////////////////////////////////////////////////////////////////////////////
// Window types

struct WindowImpl
{
	NSWindow *window;
	NSView *view;
	CAMetalLayer *layer; // Backing layer of view, where Vulkan (MoltenVK) presents
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

// Asks the engine to release the window and the platform to quit. Called on the main thread
// when the user closes the window or quits the app (Cmd+Q), instead of letting AppKit close
// or terminate right away, so the engine shuts down in order.
static void RequestQuit()
{
	const PlatformEvent event = { .type = PlatformEventTypeWindowWillDestroy };
	SendPlatformEvent(platform, event);
	PlatformQuit();
}

// Size of the view in pixels
static NSSize GetViewPixelSize(NSView *view)
{
	const NSRect backingRect = [view convertRectToBacking:view.bounds];
	return backingRect.size;
}

// Called on the main thread whenever the view size or its backing scale factor changes
static void OnViewSizeChanged(NSView *view)
{
	NSWindow *nsWindow = view.window;
	const CGFloat scale = nsWindow ? nsWindow.backingScaleFactor : 1.0;

	if ( windowImpl.layer )
	{
		// Render 1:1 pixels instead of at point resolution (which would be upscaled)
		windowImpl.layer.contentsScale = scale;
	}

	if ( nsWindow )
	{
		// The minimum size is given in pixels, like on the other platforms
		nsWindow.contentMinSize = NSMakeSize(MIN_WINDOW_WIDTH / scale, MIN_WINDOW_HEIGHT / scale);
	}

	// The engine applies the new size on the update thread, see ProcessPlatformEvents
	const NSSize size = GetViewPixelSize(view);
	const PlatformEvent event = {
		.type = PlatformEventTypeWindowResize,
		.windowResize = { .width = (u16)size.width, .height = (u16)size.height },
	};
	SendPlatformEvent(platform, event);
}

@interface IluView : NSView
@end

@implementation IluView

- (CALayer *)makeBackingLayer
{
	return [CAMetalLayer layer];
}

- (BOOL)wantsUpdateLayer
{
	return YES; // Contents come from Vulkan, never from drawRect:
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)setFrameSize:(NSSize)newSize
{
	[super setFrameSize:newSize];
	OnViewSizeChanged(self);
}

- (void)viewDidChangeBackingProperties
{
	// Moved to a display with a different scale factor
	[super viewDidChangeBackingProperties];
	OnViewSizeChanged(self);
}

// TODO(macOS): Keyboard input. Handled here meanwhile so unhandled keys don't make AppKit beep.
- (void)keyDown:(NSEvent *)event {}
- (void)keyUp:(NSEvent *)event {}

@end

@interface IluAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation IluAppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	// Cmd+Q and the Quit menu item: let the engine shut down instead of exiting right away
	RequestQuit();
	return NSTerminateCancel;
}

- (BOOL)windowShouldClose:(NSWindow *)sender
{
	// The window is closed by CleanupWindow, once the engine has released its surface
	RequestQuit();
	return NO;
}

@end

static IluAppDelegate *appDelegate;

static void CreateMenuBar(const char *title)
{
	// A process without a bundle gets no menu bar by default, which also means no Cmd+Q
	NSString *appName = [NSString stringWithUTF8String:title];

	NSMenu *appMenu = [[NSMenu alloc] init];
	[appMenu addItemWithTitle:[@"Hide " stringByAppendingString:appName] action:@selector(hide:) keyEquivalent:@"h"];
	[appMenu addItem:[NSMenuItem separatorItem]];
	[appMenu addItemWithTitle:[@"Quit " stringByAppendingString:appName] action:@selector(terminate:) keyEquivalent:@"q"];

	NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
	appMenuItem.submenu = appMenu;

	NSMenu *menuBar = [[NSMenu alloc] init];
	[menuBar addItem:appMenuItem];
	NSApp.mainMenu = menuBar;
}

static bool InitializeWindow(Window &window, u32 width, u32 height, const char *title)
{
	ZeroStruct(&window);
	window.width = width;
	window.height = height;

	ZeroStruct(&windowImpl);
	window.impl = &windowImpl;

	@autoreleasepool
	{
		[NSApplication sharedApplication];
		// Without a bundle the process would get no Dock icon and could not take focus
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		appDelegate = [[IluAppDelegate alloc] init];
		NSApp.delegate = appDelegate;
		CreateMenuBar(title);
		[NSApp finishLaunching];

		// The requested size is in pixels: convert it to points for the screen the window opens on
		const CGFloat scale = NSScreen.mainScreen ? NSScreen.mainScreen.backingScaleFactor : 1.0;
		const NSRect contentRect = NSMakeRect(0, 0, width / scale, height / scale);

		const NSWindowStyleMask style =
			NSWindowStyleMaskTitled |
			NSWindowStyleMaskClosable |
			NSWindowStyleMaskMiniaturizable |
			NSWindowStyleMaskResizable;

		NSWindow *nsWindow = [[NSWindow alloc] initWithContentRect:contentRect
		                                                 styleMask:style
		                                                   backing:NSBackingStoreBuffered
		                                                     defer:NO];
		if ( !nsWindow )
		{
			LOG(Error, "InitializeWindow - NSWindow creation failed\n");
			return false;
		}

		nsWindow.releasedWhenClosed = NO; // Owned by windowImpl
		nsWindow.title = [NSString stringWithUTF8String:title];
		nsWindow.delegate = appDelegate;
		nsWindow.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary; // Green button
		[nsWindow center];

		IluView *view = [[IluView alloc] initWithFrame:contentRect];
		view.wantsLayer = YES; // Creates the CAMetalLayer through makeBackingLayer
		nsWindow.contentView = view;
		[nsWindow makeFirstResponder:view];

		windowImpl.window = nsWindow;
		windowImpl.view = view;
		windowImpl.layer = (CAMetalLayer *)view.layer;

		OnViewSizeChanged(view); // Applies the scale factor and minimum size

		const NSSize pixelSize = GetViewPixelSize(view);
		window.width = (u32)pixelSize.width;
		window.height = (u32)pixelSize.height;

		// The window stays hidden until the engine has initialized it, see ShowPlatformWindow
	}

	// Same moment as WM_CREATE on Windows: the engine initializes the window on the update thread
	const PlatformEvent event = { .type = PlatformEventTypeWindowWasCreated };
	SendPlatformEvent(platform, event);

	return true;
}

// Runs block on the main thread, without waiting for it when called from another thread
static void RunOnMainThread(void (^block)(void))
{
	if ( [NSThread isMainThread] ) {
		block();
	} else {
		dispatch_async(dispatch_get_main_queue(), block);
	}
}

static void CleanupWindow(Window &window)
{
	// Called on the update thread once the engine released the window
	NSWindow *nsWindow = windowImpl.window;
	if ( nsWindow ) {
		RunOnMainThread(^{ [nsWindow close]; });
	}
}

static void ShowPlatformWindow(Window &window)
{
	// Called on the update thread once the engine initialized the window
	NSWindow *nsWindow = windowImpl.window;
	RunOnMainThread(^{ [nsWindow makeKeyAndOrderFront:nil]; [NSApp activate]; });
}

static void PlatformWakeMainThread()
{
	// Called from any thread. Posting an empty NSEvent to wake the main app thread up.
	@autoreleasepool
	{
		[NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint
			modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:YES];
	}
}

static void PlatformUpdateEventLoop(Platform &platform)
{
	@autoreleasepool
	{
		// Block until there is an event, then handle every pending one
		NSDate *until = [NSDate distantFuture];
		NSEvent *event;
		while ( platform.keepRunning &&
				(event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:until inMode:NSDefaultRunLoopMode dequeue:YES]) )
		{
			[NSApp sendEvent:event];
			until = [NSDate distantPast];
		}
	}
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
