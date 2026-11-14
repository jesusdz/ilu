// macOS platform layer, included by platform.cpp (compiled as Objective-C++).
// Status: window, Retina, full screen and quit work. Input, gamepad, file watching and audio are TODO.
// Threading: AppKit runs on the main thread, the engine on the update thread (USE_UPDATE_THREAD).
// Units: the engine gets pixels, AppKit uses points (pixels = points * backingScaleFactor).

// AppKit is imported by ilu_core.h, see the comment there.
#import <QuartzCore/CAMetalLayer.h>
#include <IOKit/graphics/IOGraphicsTypes.h> // kDisplayModeNativeFlag

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

	// Full screen
	bool fullscreen;                // Requested by the user (kept while another app is active)
	NSRect windowedFrame;           // Restored when leaving full screen
	NSWindowStyleMask windowedStyle;
	CGDirectDisplayID display;      // Display covered while full screen
	CGDisplayModeRef userMode;      // Mode to restore, non-null while ilu switched the display mode
	bool reapplyWindowedFrame;      // After a mode restore, once AppKit has rearranged the windows
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

// Close button and Cmd+Q: the engine releases the window and shuts down in order
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


////////////////////////////////////////////////////////////////////////////////////////////////////
// Full screen: borderless window in a native-sized display mode (no scaling), see docs/MACOS_PORT.md

static bool SetDisplayMode(CGDirectDisplayID display, CGDisplayModeRef mode)
{
	CGDisplayConfigRef config;
	if ( CGBeginDisplayConfiguration(&config) != kCGErrorSuccess ) {
		return false;
	}
	if ( CGConfigureDisplayWithDisplayMode(config, display, mode, NULL) != kCGErrorSuccess ) {
		CGCancelDisplayConfiguration(config);
		return false;
	}
	// App only: macOS restores the user's mode itself when ilu exits, even if it crashes
	return CGCompleteDisplayConfiguration(config, kCGConfigureForAppOnly) == kCGErrorSuccess;
}

// Retained mode with the panel's native pixel size, or NULL if currentMode already has it
static CGDisplayModeRef CopyNativeDisplayMode(CGDirectDisplayID display, CGDisplayModeRef currentMode)
{
	// Without this option, 2x (HiDPI) modes with the same pixel size as a 1x mode are not listed
	NSDictionary *options = @{ (NSString *)kCGDisplayShowDuplicateLowResolutionModes: @YES };
	CFArrayRef modes = CGDisplayCopyAllDisplayModes(display, (CFDictionaryRef)options);
	if ( !modes ) {
		return NULL;
	}
	const CFIndex modeCount = CFArrayGetCount(modes);

	// Native pixel size: from the mode flagged as native, or else the largest 1x mode
	size_t nativeWidth = 0, nativeHeight = 0;
	for ( u32 pass = 0; pass < 2 && nativeWidth == 0; ++pass ) {
		for ( CFIndex i = 0; i < modeCount; ++i ) {
			CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
			const size_t pixelWidth = CGDisplayModeGetPixelWidth(mode);
			const bool candidate = pass == 0 ?
				( CGDisplayModeGetIOFlags(mode) & kDisplayModeNativeFlag ) != 0 :
				pixelWidth == CGDisplayModeGetWidth(mode);
			if ( candidate && pixelWidth > nativeWidth ) {
				nativeWidth = pixelWidth;
				nativeHeight = CGDisplayModeGetPixelHeight(mode);
			}
		}
	}

	CGDisplayModeRef bestMode = NULL;
	const bool currentIsNative =
		CGDisplayModeGetPixelWidth(currentMode) == nativeWidth &&
		CGDisplayModeGetPixelHeight(currentMode) == nativeHeight;

	if ( nativeWidth > 0 && !currentIsNative )
	{
		// Prefer 2x (readable system UI), then the current refresh rate, then the highest
		const double currentRefresh = CGDisplayModeGetRefreshRate(currentMode);
		u32 bestScore = 0;
		for ( CFIndex i = 0; i < modeCount; ++i ) {
			CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
			if ( CGDisplayModeGetPixelWidth(mode) != nativeWidth ||
			     CGDisplayModeGetPixelHeight(mode) != nativeHeight ||
			     !CGDisplayModeIsUsableForDesktopGUI(mode) ) {
				continue;
			}
			const double refresh = CGDisplayModeGetRefreshRate(mode);
			const u32 score =
				( CGDisplayModeGetWidth(mode) * 2 == nativeWidth ? 1u << 30 : 0 ) +
				( refresh == currentRefresh ? 1u << 29 : 0 ) +
				(u32)refresh + 1;
			if ( score > bestScore ) {
				bestScore = score;
				bestMode = mode;
			}
		}
		CGDisplayModeRetain(bestMode);
	}

	CFRelease(modes);
	return bestMode;
}

// Switches the full-screen display to a native-sized mode, if it isn't already
static void SwitchToNativeDisplayMode()
{
	if ( windowImpl.userMode ) {
		return; // Already switched
	}

	CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(windowImpl.display);
	CGDisplayModeRef nativeMode = currentMode ? CopyNativeDisplayMode(windowImpl.display, currentMode) : NULL;

	if ( nativeMode && SetDisplayMode(windowImpl.display, nativeMode) ) {
		LOG(Info, "Display mode switched to %zux%zu points, %zux%zu pixels\n",
			CGDisplayModeGetWidth(nativeMode), CGDisplayModeGetHeight(nativeMode),
			CGDisplayModeGetPixelWidth(nativeMode), CGDisplayModeGetPixelHeight(nativeMode));
		windowImpl.userMode = currentMode;
	} else {
		if ( nativeMode ) {
			LOG(Warning, "Display mode switch failed, keeping the current mode\n");
		}
		CGDisplayModeRelease(currentMode);
	}
	CGDisplayModeRelease(nativeMode);
}

// Returns whether the display mode changed
static bool RestoreUserDisplayMode()
{
	bool restored = false;
	if ( windowImpl.userMode ) {
		restored = SetDisplayMode(windowImpl.display, windowImpl.userMode);
		if ( !restored ) {
			LOG(Warning, "Display mode restore failed\n");
		}
		CGDisplayModeRelease(windowImpl.userMode);
		windowImpl.userMode = NULL;
	}
	return restored;
}

// Resizes the window to cover the full-screen display, in its current mode
static void FitWindowToDisplay()
{
	// CGDisplayBounds is updated right after a mode switch (NSScreen later), with a top-left origin
	const CGRect bounds = CGDisplayBounds(windowImpl.display);
	if ( CGRectIsEmpty(bounds) ) {
		return; // Display disconnected
	}
	const CGFloat mainHeight = CGDisplayBounds(CGMainDisplayID()).size.height;
	const NSRect frame = NSMakeRect(bounds.origin.x, mainHeight - bounds.origin.y - bounds.size.height,
	                                bounds.size.width, bounds.size.height);
	[windowImpl.window setFrame:frame display:YES];
}

static void ApplyFullscreenLayout()
{
	SwitchToNativeDisplayMode();

	NSWindow *nsWindow = windowImpl.window;
	nsWindow.styleMask = NSWindowStyleMaskBorderless;
	NSApp.presentationOptions = NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
	FitWindowToDisplay();
	[nsWindow makeKeyAndOrderFront:nil];
}

static void RemoveFullscreenLayout()
{
	NSWindow *nsWindow = windowImpl.window;
	NSApp.presentationOptions = NSApplicationPresentationDefault;
	nsWindow.styleMask = windowImpl.windowedStyle;
	[nsWindow setFrame:windowImpl.windowedFrame display:YES];

	windowImpl.reapplyWindowedFrame = RestoreUserDisplayMode();
}

static void EnterFullscreen()
{
	NSWindow *nsWindow = windowImpl.window;
	NSScreen *screen = nsWindow.screen ? nsWindow.screen : NSScreen.mainScreen;

	windowImpl.fullscreen = true;
	windowImpl.windowedFrame = nsWindow.frame;
	windowImpl.windowedStyle = nsWindow.styleMask;
	windowImpl.display = [screen.deviceDescription[@"NSScreenNumber"] unsignedIntValue];

	ApplyFullscreenLayout();
}

static bool HasFullscreenLayout()
{
	return windowImpl.window.styleMask == NSWindowStyleMaskBorderless;
}

static void ExitFullscreen()
{
	windowImpl.fullscreen = false;

	if ( HasFullscreenLayout() ) {
		RemoveFullscreenLayout();
		[windowImpl.window makeKeyAndOrderFront:nil];
	}
}

static void ToggleFullscreen()
{
	if ( windowImpl.fullscreen ) {
		ExitFullscreen();
	} else {
		EnterFullscreen();
	}
}

@interface IluWindow : NSWindow
@end

@implementation IluWindow

// Borderless windows (full screen) can't become key by default, so they would get no input
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }

// Every full-screen request (green button, View menu, Ctrl+Cmd+F, Fn+F) ends up here
- (void)toggleFullScreen:(id)sender
{
	ToggleFullscreen();
}

@end


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

// Another app active: give back the window and display mode, full screen again on return
- (void)applicationDidResignActive:(NSNotification *)notification
{
	if ( HasFullscreenLayout() ) {
		RemoveFullscreenLayout();
	}
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
	if ( windowImpl.fullscreen && !HasFullscreenLayout() ) {
		ApplyFullscreenLayout();
	}
}

// Display arrangement or mode changed (also by AppKit, after our own mode switches)
- (void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
	if ( HasFullscreenLayout() ) {
		FitWindowToDisplay();
	} else if ( windowImpl.reapplyWindowedFrame ) {
		// AppKit moves windows when it learns about the restored mode, after RemoveFullscreenLayout
		windowImpl.reapplyWindowedFrame = false;
		[windowImpl.window setFrame:windowImpl.windowedFrame display:YES];
	}
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

	NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
	NSMenuItem *fullscreenItem = [viewMenu addItemWithTitle:@"Toggle Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
	fullscreenItem.keyEquivalentModifierMask = NSEventModifierFlagControl | NSEventModifierFlagCommand;

	NSMenuItem *viewMenuItem = [[NSMenuItem alloc] init];
	viewMenuItem.submenu = viewMenu;

	NSMenu *menuBar = [[NSMenu alloc] init];
	[menuBar addItem:appMenuItem];
	[menuBar addItem:viewMenuItem];
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

		NSWindow *nsWindow = [[IluWindow alloc] initWithContentRect:contentRect
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

		// The green button would enter AppKit's full-screen Space, send it to our own full screen
		NSButton *greenButton = [nsWindow standardWindowButton:NSWindowZoomButton];
		greenButton.target = nsWindow;
		greenButton.action = @selector(toggleFullScreen:);

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
		RunOnMainThread(^{
			if ( windowImpl.fullscreen ) {
				ExitFullscreen(); // Gives the user their display mode back before the window goes away
			}
			[nsWindow close];
		});
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
