
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include <xcb/xcb.h>
#include <stdlib.h> // free
#include <sys/inotify.h>

#define USE_UPDATE_THREAD 1
#define USE_AUDIO_THREAD 1


////////////////////////////////////////////////////////////////////////////////////////////////////
// XCB key mappings

// Xcb key code mappings have been achieved with the 'xev' command
internal Key XcbKeyMappings[] = {
K_NULL, // 0
K_NULL, // 1
K_NULL, // 2
K_NULL, // 3
K_NULL, // 4
K_NULL, // 5
K_NULL, // 6
K_NULL, // 7
K_NULL, // 8
K_ESCAPE, // 9
K_1, // 10
K_2, // 11
K_3, // 12
K_4, // 13
K_5, // 14
K_6, // 15
K_7, // 16
K_8, // 17
K_9, // 18
K_0, // 19
K_NULL, // 20
K_NULL, // 21
K_BACKSPACE, // 22
K_TAB, // 23
K_Q, // 24
K_W, // 25
K_E, // 26
K_R, // 27
K_T, // 28
K_Y, // 29
K_U, // 30
K_I, // 31
K_O, // 32
K_P, // 33
K_NULL, // 34
K_NULL, // 35
K_RETURN, // 36
K_CONTROL, // 37
K_A, // 38
K_S, // 39
K_D, // 40
K_F, // 41
K_G, // 42
K_H, // 43
K_J, // 44
K_K, // 45
K_L, // 46
K_NULL, // 47
K_NULL, // 48
K_NULL, // 49
K_SHIFT, // 50
K_NULL, // 51
K_Z, // 52
K_X, // 53
K_C, // 54
K_V, // 55
K_B, // 56
K_N, // 57
K_M, // 58
K_NULL, // 59
K_NULL, // 60
K_NULL, // 61
K_SHIFT, // 62
K_NULL, // 63
K_ALT, // 64
K_SPACE, // 65
K_NULL, // 66
K_F1, // 67
K_F2, // 68
K_F3, // 69
K_F4, // 70
K_F5, // 71
K_F6, // 72
K_F7, // 73
K_F8, // 74
K_F9, // 75
K_F10, // 76
K_NULL, // 77
K_NULL, // 78
K_NULL, // 79
K_NULL, // 80
K_NULL, // 81
K_NULL, // 82
K_NULL, // 83
K_NULL, // 84
K_NULL, // 85
K_NULL, // 86
K_NULL, // 87
K_NULL, // 88
K_NULL, // 89
K_NULL, // 90
K_NULL, // 91
K_NULL, // 92
K_NULL, // 93
K_NULL, // 94
K_F11, // 95
K_F12, // 96
K_NULL, // 97
K_NULL, // 98
K_NULL, // 99
K_NULL, // 100
K_NULL, // 101
K_NULL, // 102
K_NULL, // 103
K_NULL, // 104
K_CONTROL, // 105
K_NULL, // 106
K_NULL, // 107
K_ALT, // 108
K_NULL, // 109
K_NULL, // 110
K_UP, // 111
K_NULL, // 112
K_LEFT, // 113
K_RIGHT, // 114
K_NULL, // 115
K_DOWN, // 116
K_NULL, // 117
K_NULL, // 118
K_DELETE, // 119
K_NULL, // 120
K_NULL, // 121
K_NULL, // 122
K_NULL, // 123
K_NULL, // 124
K_NULL, // 125
K_NULL, // 126
K_NULL, // 127
K_NULL, // 128
K_NULL, // 129
K_NULL, // 130
K_NULL, // 131
K_NULL, // 132
K_NULL, // 133
K_NULL, // 134
K_NULL, // 135
K_NULL, // 136
K_NULL, // 137
K_NULL, // 138
K_NULL, // 139
K_NULL, // 140
K_NULL, // 141
K_NULL, // 142
K_NULL, // 143
K_NULL, // 144
K_NULL, // 145
K_NULL, // 146
K_NULL, // 147
K_NULL, // 148
K_NULL, // 149
K_NULL, // 150
K_NULL, // 151
K_NULL, // 152
K_NULL, // 153
K_NULL, // 154
K_NULL, // 155
K_NULL, // 156
K_NULL, // 157
K_NULL, // 158
K_NULL, // 159
K_NULL, // 160
K_NULL, // 161
K_NULL, // 162
K_NULL, // 163
K_NULL, // 164
K_NULL, // 165
K_NULL, // 166
K_NULL, // 167
K_NULL, // 168
K_NULL, // 169
K_NULL, // 170
K_NULL, // 171
K_NULL, // 172
K_NULL, // 173
K_NULL, // 174
K_NULL, // 175
K_NULL, // 176
K_NULL, // 177
K_NULL, // 178
K_NULL, // 179
K_NULL, // 180
K_NULL, // 181
K_NULL, // 182
K_NULL, // 183
K_NULL, // 184
K_NULL, // 185
K_NULL, // 186
K_NULL, // 187
K_NULL, // 188
K_NULL, // 189
K_NULL, // 190
K_NULL, // 191
K_NULL, // 192
K_NULL, // 193
K_NULL, // 194
K_NULL, // 195
K_NULL, // 196
K_NULL, // 197
K_NULL, // 198
K_NULL, // 199
K_NULL, // 200
K_NULL, // 201
K_NULL, // 202
K_NULL, // 203
K_NULL, // 204
K_NULL, // 205
K_NULL, // 206
K_NULL, // 207
K_NULL, // 208
K_NULL, // 209
K_NULL, // 210
K_NULL, // 211
K_NULL, // 212
K_NULL, // 213
K_NULL, // 214
K_NULL, // 215
K_NULL, // 216
K_NULL, // 217
K_NULL, // 218
K_NULL, // 219
K_NULL, // 220
K_NULL, // 221
K_NULL, // 222
K_NULL, // 223
K_NULL, // 224
K_NULL, // 225
K_NULL, // 226
K_NULL, // 227
K_NULL, // 228
K_NULL, // 229
K_NULL, // 230
K_NULL, // 231
K_NULL, // 232
K_NULL, // 233
K_NULL, // 234
K_NULL, // 235
K_NULL, // 236
K_NULL, // 237
K_NULL, // 238
K_NULL, // 239
K_NULL, // 240
K_NULL, // 241
K_NULL, // 242
K_NULL, // 243
K_NULL, // 244
K_NULL, // 245
K_NULL, // 246
K_NULL, // 247
K_NULL, // 248
K_NULL, // 249
K_NULL, // 250
K_NULL, // 251
K_NULL, // 252
K_NULL, // 253
K_NULL, // 254
K_NULL, // 255
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform implementation types and state

struct WindowImpl
{
	xcb_connection_t *connection;
	xcb_window_t window;
	xcb_atom_t closeAtom;
};

static WindowImpl windowImpl;

static int gamepadFd = -1;

static DynamicLibrary audioLibrary;
static snd_pcm_t *audioPcm;

static const char *engineLibFilename = "engine.so";
static const char *engineLibTmpFilename = "engine.tmp.so";

static void XcbReportError( int xcbErrorCode, const char *context )
{
	static const char *xcbErrorMessages[] = {
		"NO_ERROR",
		"XCB_CONN_ERROR",                   // 1
		"XCB_CONN_CLOSED_EXT_NOTSUPPORTED", // 2
		"XCB_CONN_CLOSED_MEM_INSUFFICIENT", // 3
		"XCB_CONN_CLOSED_REQ_LEN_EXCEED",   // 4
		"XCB_CONN_CLOSED_PARSE_ERR",        // 5
		"XCB_CONN_CLOSED_INVALID_SCREEN",   // 6
		"XCB_CONN_CLOSED_FDPASSING_FAILED", // 7
	};
	LOG(Error, "Xcb error (%s): %s\n", context, xcbErrorMessages[xcbErrorCode]);
}

static void XcbReportGenericError( xcb_connection_t *conn, xcb_generic_error_t *err, const char *context )
{
	// TODO: Find a better way to report XCB generic errors
	LOG(Error, "Xcb generic error (%s)\n", context);
}


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

#if 0
static void PrintModifiers(uint32_t mask)
{
	const char **mod, *mods[] = {
		"Shift", "Lock", "Ctrl", "Alt",
		"Mod2", "Mod3", "Mod4", "Mod5",
		"Button1", "Button2", "Button3", "Button4", "Button5"
	};
	LOG(Info, "Modifier mask: ");
	for (mod = mods ; mask; mask >>= 1, mod++)
		if (mask & 1)
			LOG(Info, *mod);
	putchar ('\n');
}
#endif

static void XcbWindowProc(Window &window, xcb_generic_event_t *event)
{
	u32 eventType = event->response_type & ~0x80;

	switch ( eventType )
	{
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
			{
				// NOTE: xcb_key_release_event_t is an alias of xcb_key_press_event_t
				xcb_key_press_event_t *ev = (xcb_key_press_event_t *)event;
				u32 keyCode = ev->detail;
				ASSERT( keyCode < ARRAY_COUNT(XcbKeyMappings) );
				const Key mapping = XcbKeyMappings[ keyCode ];
				ASSERT( mapping < K_COUNT );
				const KeyState state = eventType == XCB_KEY_PRESS ? KEY_STATE_PRESS : KEY_STATE_RELEASE;
				const PlatformEvent event = {
					.type = PlatformEventTypeKeyPress,
					.keyPress = {
						.code = mapping,
						.state = state,
					},
				};
				SendPlatformEvent(platform, event);
				break;
			}

		case XCB_BUTTON_PRESS:
		case XCB_BUTTON_RELEASE:
			{
				// NOTE: xcb_button_release_event_t is an alias of xcb_button_press_event_t
				xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
				const ButtonState state = eventType == XCB_BUTTON_PRESS ? BUTTON_STATE_PRESS : BUTTON_STATE_RELEASE;
				const MouseButton button =
					ev->detail == 1 ? MOUSE_BUTTON_LEFT :
					ev->detail == 2 ? MOUSE_BUTTON_MIDDLE :
					MOUSE_BUTTON_RIGHT;
				const i16 wheelY = ev->detail == 4 ? -1 : ev->detail == 5 ? 1 : 0;
				const i16 wheelX = ev->detail == 6 ? -1 : ev->detail == 7 ? 1 : 0;
				PlatformEvent event = {};
				switch (ev->detail) {
					// Left, middle, right buttons
					case 1:
					case 2:
					case 3:
						event.type = PlatformEventTypeMouseClick;
						event.mouseClick.button = button;
						event.mouseClick.state = state;
						SendPlatformEvent(platform, event);
						break;
					// Mouse wheel
					case 4:
					case 5:
					case 6:
					case 7:
						event.type = PlatformEventTypeMouseWheel;
						event.mouseWheel.delta = {wheelX, wheelY};
						SendPlatformEvent(platform, event);
						break;
					default:;
				}
				break;
			}

		case XCB_MOTION_NOTIFY:
			{
				xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)event;
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseMove,
					.mouseMove = { .pos = { .x = ev->event_x, .y = ev->event_y } }
				};
				SendPlatformEvent(platform, event);
				break;
			}

		case XCB_ENTER_NOTIFY:
			{
				xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)event;
				//LOG(Info, "Mouse entered window %ld, at coordinates (%d,%d)\n",
				//		ev->event, ev->event_x, ev->event_y);
				break;
			}

		case XCB_LEAVE_NOTIFY:
			{
				xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)event;
				//LOG(Info, "Mouse left window %ld, at coordinates (%d,%d)\n",
				//		ev->event, ev->event_x, ev->event_y);
				break;
			}

		case XCB_CONFIGURE_NOTIFY:
			{
				const xcb_configure_notify_event_t *ev = (const xcb_configure_notify_event_t *)event;
				const PlatformEvent event = {
					.type = PlatformEventTypeWindowResize,
					.windowResize = { .width = ev->width, .height = ev->height },
				};
				SendPlatformEvent(platform, event);
				break;
			}

		case XCB_CLIENT_MESSAGE:
			{
				const xcb_client_message_event_t *ev = (const xcb_client_message_event_t *)event;
				if ( ev->data.data32[0] == window.impl->closeAtom )
				{
					PlatformQuit();
				}
				break;
			}

		case XCB_MAP_NOTIFY:
			// TODO: Handle this event
			break;

		case XCB_REPARENT_NOTIFY:
			// TODO: Handle this event
			break;

		default:
			/* Unknown event type, ignore it */
			LOG(Info, "Unknown window event: %d\n", event->response_type);
			break;
	}
}

static bool InitializeWindow(Window &window, u32 width, u32 height, const char *title)
{
	ZeroStruct(&window);
	window.width = width;
	window.height = height;

	ZeroStruct(&windowImpl);
	window.impl = &windowImpl;

	// Connect to the X server
	xcb_connection_t *xcbConnection = xcb_connect(NULL, NULL);

	int xcbConnError = xcb_connection_has_error(xcbConnection);
	if ( xcbConnError > 0 )
	{
		XcbReportError(xcbConnError, "xcb_connect");
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Get the first screen
	const xcb_setup_t *setup = xcb_get_setup(xcbConnection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t * screen = iter.data;

	// Configure events to capture
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t values[2] = {
		screen->black_pixel,
		XCB_EVENT_MASK_KEY_PRESS       | XCB_EVENT_MASK_KEY_RELEASE    |
		XCB_EVENT_MASK_BUTTON_PRESS    | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION  |
		XCB_EVENT_MASK_ENTER_WINDOW    | XCB_EVENT_MASK_LEAVE_WINDOW   |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY
	};

	// Create a window
	xcb_window_t xcbWindow = xcb_generate_id(xcbConnection);
	xcb_void_cookie_t createWindowCookie = xcb_create_window_checked(
		xcbConnection,                 // xcb connection
		XCB_COPY_FROM_PARENT,          // depth
		xcbWindow,                     // window id
		screen->root,                  // parent window
		0, 0,                          // x, y
		width, height,                 // width, height
		0,                             // bnorder_width
		XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
		screen->root_visual,           // visual
		mask, values);                 // value_mask, value_list

	xcb_generic_error_t *createWindowError = xcb_request_check(xcbConnection, createWindowCookie);
	if ( createWindowError )
	{
		XcbReportGenericError(xcbConnection, createWindowError, "xcb_create_window_checked");
		xcb_destroy_window(xcbConnection, xcbWindow);
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Handle close event
	// TODO: Handle xcb_intern_atom errors
	xcb_intern_atom_cookie_t protocolCookie = // handle error
		xcb_intern_atom_unchecked( xcbConnection, 1, 12, "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *protocolReply =
		xcb_intern_atom_reply( xcbConnection, protocolCookie, 0);
	xcb_intern_atom_cookie_t closeCookie = // handle error
		xcb_intern_atom_unchecked( xcbConnection, 0, 16, "WM_DELETE_WINDOW");
	xcb_intern_atom_reply_t *closeReply =
		xcb_intern_atom_reply( xcbConnection, closeCookie, 0);
	u8 dataFormat = 32;
	u32 dataLength = 1;
	void *data = &closeReply->atom;
	xcb_change_property( // handle error
			xcbConnection,
			XCB_PROP_MODE_REPLACE,
			xcbWindow,
			protocolReply->atom, XCB_ATOM_ATOM,
			dataFormat, dataLength, data);
	xcb_atom_t closeAtom = closeReply->atom;
	free(protocolReply);
	free(closeReply);

	// Map the window to the screen
	xcb_void_cookie_t mapWindowCookie = xcb_map_window_checked(xcbConnection, xcbWindow);
	xcb_generic_error_t *mapWindowError = xcb_request_check(xcbConnection, mapWindowCookie);
	if ( mapWindowError )
	{
		XcbReportGenericError(xcbConnection, mapWindowError, "xcb_map_window_checked");
		xcb_destroy_window(xcbConnection, xcbWindow);
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Flush the commands before continuing
	xcb_flush(xcbConnection);

	// Get window info at this point
	xcb_get_geometry_cookie_t cookie= xcb_get_geometry( xcbConnection, xcbWindow ); // handle error
	xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply( xcbConnection, cookie, NULL ); // handle error

	window.impl->connection = xcbConnection;
	window.impl->window = xcbWindow;
	window.impl->closeAtom = closeAtom;
	window.width = reply->width;
	window.height = reply->height;

	return true;
}

static void CleanupWindow(Window &window)
{
	if ( window.impl->window ) {
		xcb_destroy_window(window.impl->connection, window.impl->window);
		window.impl->window = 0;
	}
	if (window.impl->connection) {
		xcb_disconnect(window.impl->connection);
		window.impl->connection = nullptr;
	}
}

static void ShowPlatformWindow(Window &window)
{
	// The window is mapped to the screen at creation already
}

static void PlatformWakeMainThread()
{
	if ( !windowImpl.connection )
	{
		return;
	}

	xcb_client_message_event_t event = {};
	event.response_type = XCB_CLIENT_MESSAGE;
	event.format = 32;
	event.window = windowImpl.window;
	event.type = windowImpl.closeAtom;
	event.data.data32[0] = windowImpl.closeAtom;

	xcb_send_event(windowImpl.connection, 0, windowImpl.window, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
	xcb_flush(windowImpl.connection);
}

static void PlatformUpdateEventLoop(Platform &platform)
{
	Window &window = platform.window;

	xcb_generic_event_t *event;
#if USE_UPDATE_THREAD
	while ( platform.keepRunning && (event = xcb_wait_for_event(window.impl->connection)) != 0 )
#else
	while ( (event = xcb_poll_for_event(window.impl->connection)) != 0 )
#endif
	{
		XcbWindowProc(window, event);
		free(event);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// File watching

#define MAX_FILE_WATCHES 4

static int fileWatchInotifyFd = -1;
static int fileWatchWds[MAX_FILE_WATCHES];
static u32 fileWatchCount = 0;

static bool WatchDirectory(Platform &platform, const char *path, bool recursive)
{
	ASSERT(!recursive); // Recursive watches are not supported yet

	if (fileWatchCount == MAX_FILE_WATCHES)
	{
		LOG(Warning, "WatchDirectory: no free watch slots for %s\n", path);
		return false;
	}

	if (fileWatchInotifyFd == -1)
	{
		fileWatchInotifyFd = inotify_init1(IN_NONBLOCK);
		if (fileWatchInotifyFd == -1)
		{
			LinuxReportError("WatchDirectory - inotify_init1");
			return false;
		}
	}

	// IN_CLOSE_WRITE rather than IN_MODIFY, so we get one notification per finished write
	// instead of one per write() call while a file is being saved.
	const int wd = inotify_add_watch(fileWatchInotifyFd, path,
			IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE);
	if (wd == -1)
	{
		LinuxReportError("WatchDirectory - inotify_add_watch");
		return false;
	}

	fileWatchWds[fileWatchCount++] = wd;
	return true;
}

static void UpdateFileWatcher(Platform &platform)
{
	if (fileWatchInotifyFd == -1)
	{
		return;
	}

	// We don't care what changed or what the events contain, only that something did. The
	// buffer still has to be drained though, otherwise the kernel queue fills up and starts
	// dropping events (IN_Q_OVERFLOW).
	char buffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
	ssize_t len;
	while ((len = read(fileWatchInotifyFd, buffer, sizeof(buffer))) > 0)
	{
		platform.host.fileChangesDetected = true;
	}

	if (len == -1 && errno != EAGAIN)
	{
		LinuxReportError("UpdateFileWatcher - read");
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamepad

static ButtonState ButtonStateFromEvent(i32 value)
{
	const ButtonState res = (value == 1) ? BUTTON_STATE_PRESS : BUTTON_STATE_RELEASE;
	return res;
}

#define GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define GAMEPAD_RIGHT_THUMB_DEADZONE 8689

static f32 AxisFromEvent(i32 axis, i32 deadzoneThreshold)
{
	f32 normalizedAxis = 0.0f;
	if (axis < -deadzoneThreshold) {
		normalizedAxis = (f32)(axis + deadzoneThreshold)/(32768.0f - deadzoneThreshold);
	} else if (axis > deadzoneThreshold) {
		normalizedAxis = (f32)(axis - deadzoneThreshold)/(32767.0f - deadzoneThreshold);
	}
	return normalizedAxis;
}

static f32 TriggerFromEvent(i32 value)
{
	//ASSERT(value >= 0 && value < 256);
	static f32 maxValue = 256.0f;
	maxValue = Max(maxValue, (f32)value); // TODO(jesus): Query per-device limits with ioctl
	const f32 res = (f32)value/maxValue;
	return res;
}

static ButtonState DPadStateFromEvent(ButtonState prevState, i32 expectedValue, i32 value)
{
	ButtonState state = prevState;
	if (expectedValue == value) {
		state = BUTTON_STATE_PRESS;
	} else if (prevState != BUTTON_STATE_IDLE) {
		state = BUTTON_STATE_RELEASE;
	}
	return state;
}

static bool InitializeGamepad(Platform &platform)
{
	LOG(Info, "Input system initialization:\n");

	platform.host.gamepad = &platform.gamepad;

	gamepadFd = -1;

	char path[MAX_PATH_LENGTH] = {};
	char name[MAX_PATH_LENGTH] = {};
	const char *dirName = "/dev/input";
	bool found = false;

	Dir dir = {};

	if ( OpenDir(dir, dirName) )
	{
		DirEntry entry = {};

		while ( !found && ReadDir(dir, entry) )
		{
			SPrintf(path, "%s/%s", dirName, entry.name);

			if (StrEqN(entry.name, "event", 5))
			{
				int fd = open(path, O_RDONLY | O_NONBLOCK);
				if (fd < 0) {
					continue;
				}

				if ( ioctl(fd, EVIOCGNAME(sizeof(name)), name) != -1 )
				{
					LOG(Info, "- Device path: %s\n", path);
					LOG(Info, "- Device name: %s\n", name);
					gamepadFd = fd;
					found = true;
				}
				else
				{
					close(fd);
				}
			}
		}

		CloseDir(dir);
	}

	return found;
}

static void UpdateGamepad(Platform &platform)
{
	Gamepad &gamepad = platform.gamepad;

	if (gamepadFd != -1)
	{
		// Update button states
		for (u32 i = 0; i < ARRAY_COUNT(gamepad.buttons); ++i) {
			if (gamepad.buttons[i] == BUTTON_STATE_PRESS) {
				gamepad.buttons[i] = BUTTON_STATE_PRESSED;
			} else if (gamepad.buttons[i] == BUTTON_STATE_RELEASE) {
			}
		}

		ssize_t size = 0;
		input_event event;

		while ( (size = read(gamepadFd, &event, sizeof(event))) != -1 )
		{
			const u32 type = event.type;
			const u32 code = event.code;
			const i32 value = event.value;

			if (type == EV_KEY) {
				switch (code) {
					case BTN_START: gamepad.start = ButtonStateFromEvent(value); break;
					case BTN_SELECT: gamepad.back = ButtonStateFromEvent(value); break;
					//case BTN_MODE: codeStr = ButtonStateFromEvent(value); break;
					case BTN_TL: gamepad.leftShoulder = ButtonStateFromEvent(value); break;
					case BTN_TR: gamepad.rightShoulder = ButtonStateFromEvent(value); break;
					case BTN_A: gamepad.a = ButtonStateFromEvent(value); break;
					case BTN_B: gamepad.b = ButtonStateFromEvent(value); break;
					case BTN_X: gamepad.x = ButtonStateFromEvent(value); break;
					case BTN_Y: gamepad.y = ButtonStateFromEvent(value); break;
					//case BTN_THUMBL: codeStr = ButtonStateFromEvent(gamepad., value); break;
					//case BTN_THUMBR: codeStr = ButtonStateFromEvent(gamepad., value); break;
					default:;
				}
			} else if (type == EV_ABS) {
				switch (code) {
					case ABS_X: gamepad.leftAxis.x = AxisFromEvent(value, GAMEPAD_LEFT_THUMB_DEADZONE); break;
					case ABS_Y: gamepad.leftAxis.y = AxisFromEvent(-value, GAMEPAD_LEFT_THUMB_DEADZONE); break;
					case ABS_RX: gamepad.rightAxis.x = AxisFromEvent(value, GAMEPAD_RIGHT_THUMB_DEADZONE); break;
					case ABS_RY: gamepad.rightAxis.y = AxisFromEvent(-value, GAMEPAD_RIGHT_THUMB_DEADZONE); break;
					case ABS_Z: gamepad.leftTrigger = TriggerFromEvent(value); break;
					case ABS_RZ: gamepad.rightTrigger = TriggerFromEvent(value); break;
					case ABS_HAT0X:
						gamepad.left = DPadStateFromEvent(gamepad.left, -1, value);
						gamepad.right = DPadStateFromEvent(gamepad.right, 1, value);
						break;
					case ABS_HAT0Y:
						gamepad.up = DPadStateFromEvent(gamepad.up, -1, value);
						gamepad.down = DPadStateFromEvent(gamepad.down, 1, value);
						break;
				}
			} else if (type == EV_MSC) {
				// MSC event (not sure what this is)
			} else if (type == EV_SYN) {
				// Synchronization event (not sure what this is)
			} else {
				LOG(Warning, "- Unknown event type\n");
			}
		}

		if (errno != EAGAIN)
		{
			LOG(Warning, "Error reading gamepad input\n");
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Audio

typedef const char * SND_STRERROR (int errnum);
typedef int SND_PCM_OPEN(snd_pcm_t **pcmp, const char * name, snd_pcm_stream_t stream, int	mode );
typedef int SND_PCM_HW_PARAMS_MALLOC(snd_pcm_hw_params_t **ptr);
typedef int SND_PCM_HW_PARAMS_ANY(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
typedef int SND_PCM_HW_PARAMS_SET_ACCESS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
typedef int SND_PCM_HW_PARAMS_SET_FORMAT(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
typedef int SND_PCM_HW_PARAMS_SET_CHANNELS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
typedef int SND_PCM_HW_PARAMS_SET_RATE_NEAR(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
typedef int SND_PCM_HW_PARAMS_SET_BUFFER_SIZE_NEAR(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
typedef int SND_PCM_HW_PARAMS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
typedef int SND_PCM_HW_PARAMS_GET_CHANNELS(const snd_pcm_hw_params_t *params, unsigned int *channelCount);
typedef int SND_PCM_HW_PARAMS_GET_RATE(const snd_pcm_hw_params_t *params, unsigned int *sampleRate, int *dir);
typedef int SND_PCM_HW_PARAMS_GET_FORMAT(const snd_pcm_hw_params_t *params, snd_pcm_format_t *format);
typedef int SND_PCM_HW_PARAMS_GET_ACCESS(const snd_pcm_hw_params_t *params, snd_pcm_access_t *access);
typedef int SND_PCM_HW_PARAMS_GET_PERIOD_TIME(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int SND_PCM_HW_PARAMS_GET_PERIOD_SIZE(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
typedef int SND_PCM_HW_PARAMS_GET_BUFFER_SIZE(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
typedef int SND_PCM_WAIT(snd_pcm_t *pcm, int timeout);
typedef int SND_PCM_AVAIL_DELAY(snd_pcm_t *pcm, snd_pcm_sframes_t *availp, snd_pcm_sframes_t *delayp);
typedef snd_pcm_sframes_t SND_PCM_WRITEI(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
typedef int SND_PCM_RECOVER(snd_pcm_t *pcm, int err, int silent);
typedef int SND_PCM_PREPARE(snd_pcm_t *pcm);
typedef int SND_PCM_CLOSE(snd_pcm_t *pcm);
typedef int SND_PCM_DRAIN(snd_pcm_t *pcm);

static SND_STRERROR* FP_snd_strerror;
static SND_PCM_OPEN* FP_snd_pcm_open;
static SND_PCM_HW_PARAMS_MALLOC* FP_snd_pcm_hw_params_malloc;
static SND_PCM_HW_PARAMS_ANY* FP_snd_pcm_hw_params_any;
static SND_PCM_HW_PARAMS_SET_ACCESS* FP_snd_pcm_hw_params_set_access;
static SND_PCM_HW_PARAMS_SET_FORMAT* FP_snd_pcm_hw_params_set_format;
static SND_PCM_HW_PARAMS_SET_CHANNELS* FP_snd_pcm_hw_params_set_channels;
static SND_PCM_HW_PARAMS_SET_RATE_NEAR* FP_snd_pcm_hw_params_set_rate_near;
static SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR* FP_snd_pcm_hw_params_set_period_size_near;
static SND_PCM_HW_PARAMS_SET_BUFFER_SIZE_NEAR* FP_snd_pcm_hw_params_set_buffer_size_near;
static SND_PCM_HW_PARAMS* FP_snd_pcm_hw_params;
static SND_PCM_HW_PARAMS_GET_CHANNELS* FP_snd_pcm_hw_params_get_channels;
static SND_PCM_HW_PARAMS_GET_RATE* FP_snd_pcm_hw_params_get_rate;
static SND_PCM_HW_PARAMS_GET_FORMAT* FP_snd_pcm_hw_params_get_format;
static SND_PCM_HW_PARAMS_GET_ACCESS* FP_snd_pcm_hw_params_get_access;
static SND_PCM_HW_PARAMS_GET_PERIOD_TIME* FP_snd_pcm_hw_params_get_period_time;
static SND_PCM_HW_PARAMS_GET_PERIOD_SIZE* FP_snd_pcm_hw_params_get_period_size;
static SND_PCM_HW_PARAMS_GET_BUFFER_SIZE* FP_snd_pcm_hw_params_get_buffer_size;
static SND_PCM_WAIT* FP_snd_pcm_wait;
static SND_PCM_AVAIL_DELAY* FP_snd_pcm_avail_delay;
static SND_PCM_WRITEI* FP_snd_pcm_writei;
static SND_PCM_RECOVER* FP_snd_pcm_recover;
static SND_PCM_PREPARE* FP_snd_pcm_prepare;
static SND_PCM_CLOSE* FP_snd_pcm_close;
static SND_PCM_DRAIN* FP_snd_pcm_drain;

static bool InitializeAudioDevice(Platform &platform)
{
	AudioDevice &audio = platform.audio;

	audioLibrary = OpenLibrary("libasound.so");

	if (audioLibrary)
	{
		DynamicLibrary alsa = audioLibrary;

		// Load functions
		FP_snd_strerror = (SND_STRERROR*) LoadSymbol(alsa, "snd_strerror");
		FP_snd_pcm_open = (SND_PCM_OPEN*) LoadSymbol(alsa, "snd_pcm_open");
		FP_snd_pcm_hw_params_malloc = (SND_PCM_HW_PARAMS_MALLOC*) LoadSymbol(alsa, "snd_pcm_hw_params_malloc");
		FP_snd_pcm_hw_params_any = (SND_PCM_HW_PARAMS_ANY*) LoadSymbol(alsa, "snd_pcm_hw_params_any");
		FP_snd_pcm_hw_params_set_access = (SND_PCM_HW_PARAMS_SET_ACCESS*) LoadSymbol(alsa, "snd_pcm_hw_params_set_access");
		FP_snd_pcm_hw_params_set_format = (SND_PCM_HW_PARAMS_SET_FORMAT*) LoadSymbol(alsa, "snd_pcm_hw_params_set_format");
		FP_snd_pcm_hw_params_set_channels = (SND_PCM_HW_PARAMS_SET_CHANNELS*) LoadSymbol(alsa, "snd_pcm_hw_params_set_channels");
		FP_snd_pcm_hw_params_set_rate_near = (SND_PCM_HW_PARAMS_SET_RATE_NEAR*) LoadSymbol(alsa, "snd_pcm_hw_params_set_rate_near");
		FP_snd_pcm_hw_params_set_period_size_near = (SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR*) LoadSymbol(alsa, "snd_pcm_hw_params_set_period_size_near");
		FP_snd_pcm_hw_params_set_buffer_size_near = (SND_PCM_HW_PARAMS_SET_BUFFER_SIZE_NEAR*) LoadSymbol(alsa, "snd_pcm_hw_params_set_buffer_size_near");
		FP_snd_pcm_hw_params = (SND_PCM_HW_PARAMS*) LoadSymbol(alsa, "snd_pcm_hw_params");
		FP_snd_pcm_hw_params_get_channels = (SND_PCM_HW_PARAMS_GET_CHANNELS*) LoadSymbol(alsa, "snd_pcm_hw_params_get_channels");
		FP_snd_pcm_hw_params_get_rate = (SND_PCM_HW_PARAMS_GET_RATE*) LoadSymbol(alsa, "snd_pcm_hw_params_get_rate");
		FP_snd_pcm_hw_params_get_format = (SND_PCM_HW_PARAMS_GET_FORMAT*) LoadSymbol(alsa, "snd_pcm_hw_params_get_format");
		FP_snd_pcm_hw_params_get_access = (SND_PCM_HW_PARAMS_GET_ACCESS*) LoadSymbol(alsa, "snd_pcm_hw_params_get_access");
		FP_snd_pcm_hw_params_get_period_time = (SND_PCM_HW_PARAMS_GET_PERIOD_TIME*) LoadSymbol(alsa, "snd_pcm_hw_params_get_period_time");
		FP_snd_pcm_hw_params_get_period_size = (SND_PCM_HW_PARAMS_GET_PERIOD_SIZE*) LoadSymbol(alsa, "snd_pcm_hw_params_get_period_size");
		FP_snd_pcm_hw_params_get_buffer_size = (SND_PCM_HW_PARAMS_GET_BUFFER_SIZE*) LoadSymbol(alsa, "snd_pcm_hw_params_get_buffer_size");
		FP_snd_pcm_wait = (SND_PCM_WAIT*) LoadSymbol(alsa, "snd_pcm_wait");
		FP_snd_pcm_avail_delay = (SND_PCM_AVAIL_DELAY*) LoadSymbol(alsa, "snd_pcm_avail_delay");
		FP_snd_pcm_writei = (SND_PCM_WRITEI*) LoadSymbol(alsa, "snd_pcm_writei");
		FP_snd_pcm_recover = (SND_PCM_RECOVER*) LoadSymbol(alsa, "snd_pcm_recover");
		FP_snd_pcm_prepare = (SND_PCM_PREPARE*) LoadSymbol(alsa, "snd_pcm_prepare");
		FP_snd_pcm_close = (SND_PCM_CLOSE*) LoadSymbol(alsa, "snd_pcm_close");
		FP_snd_pcm_drain = (SND_PCM_DRAIN*) LoadSymbol(alsa, "snd_pcm_drain");

		// Open PCM device
		int res = FP_snd_pcm_open(&audioPcm, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
		if (res == 0)
		{
			int dir = 0; // direction of approximate values
			unsigned int sampleRate = audio.samplesPerSecond; // frames/second (CD quality)
			unsigned int channelCount = audio.channelCount;
			unsigned int bytesPerSample = audio.bytesPerSample;
			// snd_pcm_wait only blocks once the ring buffer is nearly full: it returns as soon
			// as one period of space is free. So the buffer has to be sized to what we actually
			// intend to keep queued, otherwise there is always free space and the wait spins
			// instead of pacing us. The period sets the wake-up granularity, a few wake-ups
			// per write-ahead window.
			const u32 periodMillis = Max(1u, audio.writeAheadMillis / 3);
			snd_pcm_uframes_t periodFrames = audio.samplesPerSecond * periodMillis / 1000;
			snd_pcm_uframes_t bufferFrames = audio.samplesPerSecond * 2 * audio.writeAheadMillis / 1000;

			// Allocate and configure hardware parameters
			snd_pcm_hw_params_t *params;
			FP_snd_pcm_hw_params_malloc(&params);
			FP_snd_pcm_hw_params_any(audioPcm, params); // default values
			FP_snd_pcm_hw_params_set_channels(audioPcm, params, channelCount);
			FP_snd_pcm_hw_params_set_rate_near(audioPcm, params, &sampleRate, &dir);
			FP_snd_pcm_hw_params_set_format(audioPcm, params, SND_PCM_FORMAT_S16_LE); // 16 bit little endian
			FP_snd_pcm_hw_params_set_access(audioPcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
			FP_snd_pcm_hw_params_set_period_size_near(audioPcm, params, &periodFrames, &dir);
			FP_snd_pcm_hw_params_set_buffer_size_near(audioPcm, params, &bufferFrames);

			// Write the parameters to the driver
			res = FP_snd_pcm_hw_params(audioPcm, params);
			if (res == 0)
			{
				// The sizes above are only hints, so report what the driver actually granted.
				snd_pcm_uframes_t actualPeriodFrames = 0;
				snd_pcm_uframes_t actualBufferFrames = 0;
				FP_snd_pcm_hw_params_get_period_size(params, &actualPeriodFrames, &dir);
				FP_snd_pcm_hw_params_get_buffer_size(params, &actualBufferFrames);
				LOG(Info, "- PCM period: %lu frames / buffer: %lu frames\n",
					(unsigned long)actualPeriodFrames, (unsigned long)actualBufferFrames);

				LOG(Info, "- PCM is playing...\n");
				audio.state = AudioStatePlaying;
			}
			else
			{
				LOG(Error, "- Error setting PCM HW parameters: %s\n", FP_snd_strerror(res));
			}

			unsigned int finalSampleRate = 0;
			FP_snd_pcm_hw_params_get_rate(params, &finalSampleRate, &dir);
		}
		else
		{
			LOG(Error, "- Error opening PCM device: %s\n", FP_snd_strerror(res));
		}
	}
	else
	{
		LOG(Error, "- Error loading libasound.so\n");
	}

	return audio.state != AudioStateUninitialized;
}

// Blocks until the device has at least one period of free space (or timeout)
static void WaitForAudioDevice(Platform &platform)
{
	const int timeoutMillis = 100;
	const int res = FP_snd_pcm_wait(audioPcm, timeoutMillis);

	// Error recovery
	if ( res < 0 )
	{
		FP_snd_pcm_recover(audioPcm, res, 1);
	}
}

static void UpdateAudioDevice(Platform &platform)
{
	AudioDevice &audio = platform.audio;

	for (u32 i = 0; i < 2; ++i)
	{
		snd_pcm_sframes_t availableFrames;
		snd_pcm_sframes_t delayFrames;
		int res = FP_snd_pcm_avail_delay(audioPcm, &availableFrames, &delayFrames);
		//LOG(Debug, "avail %u / delay %u\n", availableFrames, delayFrames);

		if ( res == 0 )
		{
			// Keep up to two write-ahead windows queued. Widening the window makes underruns
			// less likely, at the cost of latency before a new sound is heard.
			const float time = 2.0f * (float)audio.writeAheadMillis / 1000.0f;
			const snd_pcm_uframes_t maxFramesToRender = (snd_pcm_uframes_t)(audio.samplesPerSecond * time);

			snd_pcm_uframes_t framesToRender = maxFramesToRender - delayFrames;

			framesToRender = framesToRender < availableFrames ? framesToRender : availableFrames;

			if ( framesToRender > 0 )
			{
				SoundBuffer soundBuffer = {};
				soundBuffer.samplesPerSecond = audio.samplesPerSecond;
				soundBuffer.sampleCount = framesToRender;
				soundBuffer.samples = audio.outputSamples;
				platform.OnPlatformRenderAudio(platform.host, soundBuffer);

				res = FP_snd_pcm_writei(audioPcm, soundBuffer.samples, framesToRender);

				if ( res < 0 )
				{
					LOG(Error, "Error calling snd_pcm_writei: %s\n", FP_snd_strerror(res));
				}
			}
		}
		else
		{
			LOG(Error, "Error calling snd_pcm_avail_delay: %s\n", FP_snd_strerror(res));
		}

		// Error recovery
		if (res == -EPIPE || res == -ESTRPIPE || res == -EINTR) {
			FP_snd_pcm_recover(audioPcm, res, 0);
			continue;
		}

		// All good or unrecoverable error (no need to try a second time)
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int argc, char **argv)
{
	Main(argc, argv);
	return 0;
}
