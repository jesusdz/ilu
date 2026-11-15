// macOS platform layer, included by platform.cpp (compiled as Objective-C++).
// Status: window, Retina, full screen, input, audio and quit work. File watching is TODO.
// Threading: AppKit runs on the main thread, the engine on the update thread (USE_UPDATE_THREAD).
// Units: the engine gets pixels, AppKit uses points (pixels = points * backingScaleFactor).

// AppKit is imported by ilu_core.h, see the comment there.
#import <QuartzCore/CAMetalLayer.h>
#include <IOKit/graphics/IOGraphicsTypes.h> // kDisplayModeNativeFlag
#import <AudioToolbox/AudioToolbox.h>
#define Key GCKey // GameController names a generic parameter 'Key', ambiguous with the engine's enum
#import <GameController/GameController.h>
#undef Key

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
	[nsWindow makeFirstResponder:windowImpl.view]; // Changing the style makes the window first responder
	NSApp.presentationOptions = NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
	FitWindowToDisplay();
	[nsWindow makeKeyAndOrderFront:nil];
}

static void RemoveFullscreenLayout()
{
	NSWindow *nsWindow = windowImpl.window;
	NSApp.presentationOptions = NSApplicationPresentationDefault;
	nsWindow.styleMask = windowImpl.windowedStyle;
	[nsWindow makeFirstResponder:windowImpl.view]; // Changing the style makes the window first responder
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



////////////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard and mouse (main thread)

static bool keysDown[K_COUNT];                   // To release them all when the window loses focus
static bool mouseButtonsDown[MOUSE_BUTTON_COUNT];
static f64 accumulatedWheel[2];                  // Trackpads scroll in fractions of a wheel step

// Special keys by physical key code (kVK_* in HIToolbox/Events.h)
static Key KeyFromKeyCode(u16 keyCode)
{
	switch ( keyCode )
	{
		case 0x7B: return K_LEFT;
		case 0x7C: return K_RIGHT;
		case 0x7E: return K_UP;
		case 0x7D: return K_DOWN;
		case 0x35: return K_ESCAPE;
		case 0x31: return K_SPACE;
		case 0x33: return K_BACKSPACE;
		case 0x75: return K_DELETE;  // Forward delete
		case 0x24: return K_RETURN;
		case 0x4C: return K_RETURN;  // Keypad enter
		case 0x30: return K_TAB;
		case 0x7A: return K_F1;
		case 0x78: return K_F2;
		case 0x63: return K_F3;
		case 0x76: return K_F4;
		case 0x60: return K_F5;
		case 0x61: return K_F6;
		case 0x62: return K_F7;
		case 0x64: return K_F8;
		case 0x65: return K_F9;
		case 0x6D: return K_F10;
		case 0x67: return K_F11;
		case 0x6F: return K_F12;
		case 0x1D: return K_0;       // Digits by position too, for layouts that need Shift for them
		case 0x12: return K_1;
		case 0x13: return K_2;
		case 0x14: return K_3;
		case 0x15: return K_4;
		case 0x17: return K_5;
		case 0x16: return K_6;
		case 0x1A: return K_7;
		case 0x1C: return K_8;
		case 0x19: return K_9;
		default: return K_NULL;
	}
}

// Letters, digits and period from the keyboard layout (like Windows virtual keys), others by position
static Key KeyFromEvent(NSEvent *event)
{
	NSString *chars = [event charactersByApplyingModifiers:0];
	if ( chars.length == 1 )
	{
		const unichar c = [chars characterAtIndex:0];
		if ( c >= 'a' && c <= 'z' ) return (Key)(K_A + (c - 'a'));
		if ( c >= '0' && c <= '9' ) return (Key)(K_0 + (c - '0'));
		if ( c == '.' ) return K_PERIOD;
	}
	return KeyFromKeyCode(event.keyCode);
}

static void SendKey(Key key, bool down, bool repeat = false)
{
	if ( key == K_NULL || ( keysDown[key] == down && !repeat ) ) {
		return;
	}
	keysDown[key] = down;
	const PlatformEvent event = {
		.type = PlatformEventTypeKeyPress,
		.keyPress = { .code = key, .state = down ? KEY_STATE_PRESS : KEY_STATE_RELEASE },
	};
	SendPlatformEvent(platform, event);
}

static void SendMouseButton(MouseButton button, bool down)
{
	if ( mouseButtonsDown[button] == down ) {
		return;
	}
	mouseButtonsDown[button] = down;
	const PlatformEvent event = {
		.type = PlatformEventTypeMouseClick,
		.mouseClick = { .button = button, .state = down ? BUTTON_STATE_PRESS : BUTTON_STATE_RELEASE },
	};
	SendPlatformEvent(platform, event);
}

// Pixels from the top-left corner, clamped to the view like on Windows
static void SendMouseMove(NSView *view, NSEvent *event)
{
	const NSPoint point = [view convertPoint:event.locationInWindow fromView:nil];
	const NSSize size = view.bounds.size;
	const CGFloat scale = view.window.backingScaleFactor;
	const PlatformEvent platformEvent = {
		.type = PlatformEventTypeMouseMove,
		.mouseMove = { .pos = {
			.x = Clamp((i32)(point.x * scale), 0, (i32)(size.width * scale)),
			.y = Clamp((i32)((size.height - point.y) * scale), 0, (i32)(size.height * scale)),
		} },
	};
	SendPlatformEvent(platform, platformEvent);
}

static void ReleaseAllKeysAndButtons()
{
	for ( u32 i = 0; i < K_COUNT; ++i ) {
		SendKey((Key)i, false);
	}
	for ( u32 i = 0; i < MOUSE_BUTTON_COUNT; ++i ) {
		SendMouseButton((MouseButton)i, false);
	}
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

- (void)keyDown:(NSEvent *)event
{
	SendKey(KeyFromEvent(event), true, event.isARepeat);

	const bool option = event.modifierFlags & NSEventModifierFlagOption;
	if ( event.keyCode == 0x24 && option && !event.isARepeat ) {
		ToggleFullscreen(); // Option+Return, like Alt+Enter on Windows
	}
}

- (void)keyUp:(NSEvent *)event
{
	SendKey(KeyFromEvent(event), false);
}

// Modifiers don't send keyDown/keyUp. Cmd is K_CONTROL too, for Cmd+key shortcuts.
- (void)flagsChanged:(NSEvent *)event
{
	const NSEventModifierFlags flags = event.modifierFlags;
	SendKey(K_SHIFT, flags & NSEventModifierFlagShift);
	SendKey(K_CONTROL, flags & ( NSEventModifierFlagControl | NSEventModifierFlagCommand ));
	SendKey(K_ALT, flags & NSEventModifierFlagOption);
}

- (void)mouseDown:(NSEvent *)event { SendMouseMove(self, event); SendMouseButton(MOUSE_BUTTON_LEFT, true); }
- (void)mouseUp:(NSEvent *)event { SendMouseMove(self, event); SendMouseButton(MOUSE_BUTTON_LEFT, false); }
- (void)rightMouseDown:(NSEvent *)event { SendMouseMove(self, event); SendMouseButton(MOUSE_BUTTON_RIGHT, true); }
- (void)rightMouseUp:(NSEvent *)event { SendMouseMove(self, event); SendMouseButton(MOUSE_BUTTON_RIGHT, false); }

- (void)otherMouseDown:(NSEvent *)event
{
	SendMouseMove(self, event);
	if ( event.buttonNumber == 2 ) SendMouseButton(MOUSE_BUTTON_MIDDLE, true);
}

- (void)otherMouseUp:(NSEvent *)event
{
	SendMouseMove(self, event);
	if ( event.buttonNumber == 2 ) SendMouseButton(MOUSE_BUTTON_MIDDLE, false);
}

// Drags keep coming while the cursor is outside the window, like a mouse capture on Windows
- (void)mouseMoved:(NSEvent *)event { SendMouseMove(self, event); }
- (void)mouseDragged:(NSEvent *)event { SendMouseMove(self, event); }
- (void)rightMouseDragged:(NSEvent *)event { SendMouseMove(self, event); }
- (void)otherMouseDragged:(NSEvent *)event { SendMouseMove(self, event); }

// Whole wheel steps, sign like Windows (forward is -1), following the user's scroll direction setting
- (void)scrollWheel:(NSEvent *)event
{
	accumulatedWheel[0] += event.deltaX;
	accumulatedWheel[1] += event.deltaY;
	const i32 stepsX = (i32)accumulatedWheel[0];
	const i32 stepsY = (i32)accumulatedWheel[1];
	accumulatedWheel[0] -= stepsX;
	accumulatedWheel[1] -= stepsY;
	if ( stepsX != 0 || stepsY != 0 ) {
		const PlatformEvent platformEvent = {
			.type = PlatformEventTypeMouseWheel,
			.mouseWheel = { .delta = { .x = -stepsX, .y = -stepsY } },
		};
		SendPlatformEvent(platform, platformEvent);
	}
}

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
	ReleaseAllKeysAndButtons(); // Released in another app (Cmd+Tab), they would stay held otherwise
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
		nsWindow.acceptsMouseMovedEvents = YES; // Mouse moves without a button pressed
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
			const bool commandKeyUp = event.type == NSEventTypeKeyUp && ( event.modifierFlags & NSEventModifierFlagCommand );
			if ( commandKeyUp ) {
				[NSApp.keyWindow sendEvent:event]; // NSApp drops keyUp while Cmd is held
			} else {
				[NSApp sendEvent:event];
			}
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
	LOG(Info, "Input system initialization:\n");
	LOG(Info, "- GameController.framework, polled every frame\n");
	return true;
}

static ButtonState UpdateButtonState(ButtonState prevState, bool pressed)
{
	const bool wasPressed = prevState == BUTTON_STATE_PRESS || prevState == BUTTON_STATE_PRESSED;
	return pressed ?
		( wasPressed ? BUTTON_STATE_PRESSED : BUTTON_STATE_PRESS ) :
		( wasPressed ? BUTTON_STATE_RELEASE : BUTTON_STATE_IDLE );
}

// Dead zones of XInput (7849 and 8689 out of 32767), so sticks feel the same as on Windows
#define GAMEPAD_LEFT_THUMB_DEADZONE  0.2395f
#define GAMEPAD_RIGHT_THUMB_DEADZONE 0.2652f

static f32 AxisFromValue(f32 value, f32 deadzone)
{
	if ( value > deadzone ) return ( value - deadzone ) / ( 1.0f - deadzone );
	if ( value < -deadzone ) return ( value + deadzone ) / ( 1.0f - deadzone );
	return 0.0f;
}

// Update thread. Without a controller, pad is nil and every read returns 0: all released, centered.
static void UpdateGamepad(Platform &platform)
{
	Gamepad &gamepad = platform.gamepad;

	@autoreleasepool
	{
		GCExtendedGamepad *pad = GCController.controllers.firstObject.extendedGamepad;

		gamepad.start = UpdateButtonState(gamepad.start, pad.buttonMenu.isPressed);
		gamepad.back = UpdateButtonState(gamepad.back, pad.buttonOptions.isPressed);
		gamepad.up = UpdateButtonState(gamepad.up, pad.dpad.up.isPressed);
		gamepad.down = UpdateButtonState(gamepad.down, pad.dpad.down.isPressed);
		gamepad.left = UpdateButtonState(gamepad.left, pad.dpad.left.isPressed);
		gamepad.right = UpdateButtonState(gamepad.right, pad.dpad.right.isPressed);
		gamepad.a = UpdateButtonState(gamepad.a, pad.buttonA.isPressed);
		gamepad.b = UpdateButtonState(gamepad.b, pad.buttonB.isPressed);
		gamepad.x = UpdateButtonState(gamepad.x, pad.buttonX.isPressed);
		gamepad.y = UpdateButtonState(gamepad.y, pad.buttonY.isPressed);
		gamepad.leftShoulder = UpdateButtonState(gamepad.leftShoulder, pad.leftShoulder.isPressed);
		gamepad.rightShoulder = UpdateButtonState(gamepad.rightShoulder, pad.rightShoulder.isPressed);
		gamepad.leftTrigger = pad.leftTrigger.value;
		gamepad.rightTrigger = pad.rightTrigger.value;
		gamepad.leftAxis.x = AxisFromValue(pad.leftThumbstick.xAxis.value, GAMEPAD_LEFT_THUMB_DEADZONE);
		gamepad.leftAxis.y = AxisFromValue(pad.leftThumbstick.yAxis.value, GAMEPAD_LEFT_THUMB_DEADZONE);
		gamepad.rightAxis.x = AxisFromValue(pad.rightThumbstick.xAxis.value, GAMEPAD_RIGHT_THUMB_DEADZONE);
		gamepad.rightAxis.y = AxisFromValue(pad.rightThumbstick.yAxis.value, GAMEPAD_RIGHT_THUMB_DEADZONE);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Audio

// Core Audio pulls samples from a real-time callback, while the engine pushes them from the audio
// thread: a ring buffer sits in between (one producer, one consumer, positions in frames).
struct MacAudio
{
	AudioUnit unit;
	i16 *ring;                 // Interleaved stereo frames
	u32 ringFrames;
	volatile_u32 readFrame;    // Advanced by the callback
	volatile_u32 writeFrame;   // Advanced by the audio thread
	dispatch_semaphore_t consumed; // Signaled by the callback, wakes WaitForAudioDevice
};

static MacAudio macAudio;

// Core Audio's real-time thread: no locks, allocations or logging here
static OSStatus AudioRenderCallback(void *, AudioUnitRenderActionFlags *, const AudioTimeStamp *, UInt32, UInt32 frameCount, AudioBufferList *buffers)
{
	MacAudio &ma = macAudio;
	i16 *out = (i16 *)buffers->mBuffers[0].mData;

	const u32 writeFrame = ma.writeFrame;
	FullReadBarrier();
	u32 readFrame = ma.readFrame;
	const u32 available = writeFrame - readFrame;
	const u32 copyFrames = Min(frameCount, available);

	for ( u32 i = 0; i < copyFrames; ++i, ++readFrame ) {
		const u32 index = ( readFrame % ma.ringFrames ) * 2;
		out[i * 2 + 0] = ma.ring[index + 0];
		out[i * 2 + 1] = ma.ring[index + 1];
	}
	MemSet(out + copyFrames * 2, ( frameCount - copyFrames ) * 2 * sizeof(i16), 0); // Underrun: silence

	FullWriteBarrier();
	ma.readFrame = readFrame;
	dispatch_semaphore_signal(ma.consumed);
	return noErr;
}

static bool InitializeAudioDevice(Platform &platform)
{
	AudioDevice &audio = platform.audio;
	MacAudio &ma = macAudio;

	// The default output follows the device chosen in System Settings (speakers, headphones...)
	const AudioComponentDescription description = {
		.componentType = kAudioUnitType_Output,
		.componentSubType = kAudioUnitSubType_DefaultOutput,
		.componentManufacturer = kAudioUnitManufacturer_Apple,
	};
	AudioComponent component = AudioComponentFindNext(NULL, &description);
	if ( !component || AudioComponentInstanceNew(component, &ma.unit) != noErr ) {
		LOG(Error, "- Could not create the default audio output unit\n");
		return false;
	}

	// The engine's format: Core Audio converts it to the device's
	const AudioStreamBasicDescription format = {
		.mSampleRate = (Float64)audio.samplesPerSecond,
		.mFormatID = kAudioFormatLinearPCM,
		.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked,
		.mBytesPerPacket = audio.channelCount * audio.bytesPerSample,
		.mFramesPerPacket = 1,
		.mBytesPerFrame = audio.channelCount * audio.bytesPerSample,
		.mChannelsPerFrame = audio.channelCount,
		.mBitsPerChannel = audio.bytesPerSample * 8,
	};
	const AURenderCallbackStruct callback = { .inputProc = AudioRenderCallback };

	ma.ringFrames = audio.samplesPerSecond / 4; // 250 ms, well above what is kept queued
	ma.ring = (i16 *)AllocateVirtualMemory(ma.ringFrames * audio.channelCount * audio.bytesPerSample);
	ma.consumed = dispatch_semaphore_create(0);

	if ( AudioUnitSetProperty(ma.unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format)) != noErr ||
	     AudioUnitSetProperty(ma.unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callback, sizeof(callback)) != noErr ||
	     AudioUnitInitialize(ma.unit) != noErr ||
	     AudioOutputUnitStart(ma.unit) != noErr )
	{
		LOG(Error, "- Could not configure or start the audio output unit\n");
		AudioComponentInstanceDispose(ma.unit);
		return false;
	}

	LOG(Info, "- Core Audio output is playing...\n");
	audio.state = AudioStatePlaying;
	return true;
}

// Blocks until the callback consumed some audio (or timeout)
static void WaitForAudioDevice(Platform &platform)
{
	dispatch_semaphore_wait(macAudio.consumed, dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC));
}

static void UpdateAudioDevice(Platform &platform)
{
	AudioDevice &audio = platform.audio;
	MacAudio &ma = macAudio;

	// Keep two write-ahead windows queued, like on Linux
	const u32 targetFrames = audio.samplesPerSecond * 2 * audio.writeAheadMillis / 1000;
	const u32 queuedFrames = ma.writeFrame - ma.readFrame;
	if ( queuedFrames >= targetFrames ) {
		return;
	}

	SoundBuffer soundBuffer = {};
	soundBuffer.samplesPerSecond = audio.samplesPerSecond;
	soundBuffer.sampleCount = targetFrames - queuedFrames;
	soundBuffer.samples = audio.outputSamples;
	platform.OnPlatformRenderAudio(platform.host, soundBuffer);

	u32 writeFrame = ma.writeFrame;
	for ( u32 i = 0; i < soundBuffer.sampleCount; ++i, ++writeFrame ) {
		const u32 index = ( writeFrame % ma.ringFrames ) * 2;
		ma.ring[index + 0] = soundBuffer.samples[i * 2 + 0];
		ma.ring[index + 1] = soundBuffer.samples[i * 2 + 1];
	}
	FullWriteBarrier();
	ma.writeFrame = writeFrame;
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
