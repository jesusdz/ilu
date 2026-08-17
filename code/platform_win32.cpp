
#include <WindowsX.h>
#include <xinput.h>
#include <mmsystem.h> // audio
#include <dsound.h>   // audio
#include <direct.h>   // _getcwd


#define USE_UPDATE_THREAD 1
#define USE_AUDIO_THREAD 1


////////////////////////////////////////////////////////////////////////////////////////////////////
// Win32 key mappings

// Window virtual key codes taken from: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
internal Key Win32KeyMappings[] = {
K_NULL, // -						0x00	Undefined
K_NULL, // VK_LBUTTON				0x01	Left mouse button
K_NULL, // VK_RBUTTON				0x02	Right mouse button
K_NULL, // VK_CANCEL				0x03	Control-break processing
K_NULL, // VK_MBUTTON				0x04	Middle mouse button (three-button mouse)
K_NULL, // VK_XBUTTON1			0x05	X1 mouse button
K_NULL, // VK_XBUTTON2			0x06	X2 mouse button
K_NULL, // -						0x07	Undefined
K_BACKSPACE, // VK_BACK			0x08	BACKSPACE key
K_TAB, // VK_TAB					0x09	TAB key
K_NULL, // -						0x0A
K_NULL, // -						0x0B	Reserved
K_NULL, // VK_CLEAR				0x0C	CLEAR key
K_RETURN, // VK_RETURN			0x0D	ENTER key
K_NULL, // -						0x0E
K_NULL, // -						0x0F	Undefined
K_SHIFT, // VK_SHIFT				0x10	SHIFT key
K_CONTROL, // VK_CONTROL			0x11	CTRL key
K_ALT, // VK_MENU					0x12	ALT key
K_NULL, // VK_PAUSE				0x13	PAUSE key
K_NULL, // VK_CAPITAL				0x14	CAPSLOCK key
K_NULL, // VK_KANA				0x15	IME Kana,Hanguel,Hangul modes
K_NULL, // VK_IME_ON				0x16	IME On
K_NULL, // VK_JUNJA				0x17	IME Junja mode
K_NULL, // VK_FINAL				0x18	IME final mode
K_NULL, // VK_HANJA				0x19	IME Hanja,Kanji modes
K_NULL, // VK_IME_OFF				0x1A	IME Off
K_ESCAPE, // VK_ESCAPE			0x1B	ESC key
K_NULL, // VK_CONVERT				0x1C	IME convert
K_NULL, // VK_NONCONVERT			0x1D	IME non convert
K_NULL, // VK_ACCEPT				0x1E	IME accept
K_NULL, // VK_MODECHANGE			0x1F	IME mode change request
K_SPACE, // VK_SPACE				0x20	SPACEBAR
K_NULL, // VK_PRIOR				0x21	PAGEUP key
K_NULL, // VK_NEXT				0x22	PAGEDOWN key
K_NULL, // VK_END					0x23	END key
K_NULL, // VK_HOME				0x24	HOME key
K_LEFT, // VK_LEFT				0x25	LEFTARROW key
K_UP,   // VK_UP					0x26	UPARROW key
K_RIGHT,// VK_RIGHT				0x27	RIGHTARROW key
K_DOWN, // VK_DOWN				0x28	DOWNARROW key
K_NULL, // VK_SELECT				0x29	SELECT key
K_NULL, // VK_PRINT				0x2A	PRINT key
K_NULL, // VK_EXECUTE				0x2B	EXECUTE key
K_NULL, // VK_SNAPSHOT			0x2C	PRINTSCREEN key
K_NULL, // VK_INSERT				0x2D	INS key
K_DELETE, // VK_DELETE			0x2E	DEL key
K_NULL, // VK_HELP				0x2F	HELP key
K_0, // 							0x30	0 key
K_1, // 							0x31	1 key
K_2, // 							0x32	2 key
K_3, // 							0x33	3 key
K_4, // 							0x34	4 key
K_5, // 							0x35	5 key
K_6, // 							0x36	6 key
K_7, // 							0x37	7 key
K_8, // 							0x38	8 key
K_9, // 							0x39	9 key
K_NULL, // -						0x3A	Undefined
K_NULL, // -						0x3B	Undefined
K_NULL, // -						0x3C	Undefined
K_NULL, // -						0x3D	Undefined
K_NULL, // -						0x3E	Undefined
K_NULL, // -						0x3F	Undefined
K_NULL, // -						0x40	Undefined
K_A, // 							0x41	A key
K_B, // 							0x42	B key
K_C, // 							0x43	C key
K_D, // 							0x44	D key
K_E, // 							0x45	E key
K_F, // 							0x46	F key
K_G, // 							0x47	G key
K_H, // 							0x48	H key
K_I, // 							0x49	I key
K_J, // 							0x4A	J key
K_K, // 							0x4B	K key
K_L, // 							0x4C	L key
K_M, // 							0x4D	M key
K_N, // 							0x4E	N key
K_O, // 							0x4F	O key
K_P, // 							0x50	P key
K_Q, // 							0x51	Q key
K_R, // 							0x52	R key
K_S, // 							0x53	S key
K_T, // 							0x54	T key
K_U, // 							0x55	U key
K_V, // 							0x56	V key
K_W, // 							0x57	W key
K_X, // 							0x58	X key
K_Y, // 							0x59	Y key
K_Z, // 							0x5A	Z key
K_NULL, // VK_LWIN				0x5B	Left Windows key(Natural keyboard)
K_NULL, // VK_RWIN				0x5C	Right Windows key(Natural keyboard)
K_NULL, // VK_APPS				0x5D	Applications key(Natural keyboard)
K_NULL, // -						0x5E	Reserved
K_NULL, // VK_SLEEP				0x5F	Computer Sleep key
K_0, // VK_NUMPAD0				0x60	Numeric keypad0 key
K_1, // VK_NUMPAD1				0x61	Numeric keypad1 key
K_2, // VK_NUMPAD2				0x62	Numeric keypad2 key
K_3, // VK_NUMPAD3				0x63	Numeric keypad3 key
K_4, // VK_NUMPAD4				0x64	Numeric keypad4 key
K_5, // VK_NUMPAD5				0x65	Numeric keypad5 key
K_6, // VK_NUMPAD6				0x66	Numeric keypad6 key
K_7, // VK_NUMPAD7				0x67	Numeric keypad7 key
K_8, // VK_NUMPAD8				0x68	Numeric keypad8 key
K_9, // VK_NUMPAD9				0x69	Numeric keypad9 key
K_NULL, // VK_MULTIPLY			0x6A	Multiply key
K_NULL, // VK_ADD					0x6B	Add key
K_NULL, // VK_SEPARATOR			0x6C	Separator key
K_NULL, // VK_SUBTRACT			0x6D	Subtract key
K_NULL, // VK_DECIMAL				0x6E	Decimal key
K_NULL, // VK_DIVIDE				0x6F	Divide key
K_F1, // VK_F1					0x70	F1 key
K_F2, // VK_F2					0x71	F2 key
K_F3, // VK_F3					0x72	F3 key
K_F4, // VK_F4					0x73	F4 key
K_F5, // VK_F5					0x74	F5 key
K_F6, // VK_F6					0x75	F6 key
K_F7, // VK_F7					0x76	F7 key
K_F8, // VK_F8					0x77	F8 key
K_F9, // VK_F9					0x78	F9 key
K_F10, // VK_F10					0x79	F10 key
K_F11, // VK_F11					0x7A	F11 key
K_F12, // VK_F12					0x7B	F12 key
K_NULL, // VK_F13					0x7C	F13 key
K_NULL, // VK_F14					0x7D	F14 key
K_NULL, // VK_F15					0x7E	F15 key
K_NULL, // VK_F16					0x7F	F16 key
K_NULL, // VK_F17					0x80	F17 key
K_NULL, // VK_F18					0x81	F18 key
K_NULL, // VK_F19					0x82	F19 key
K_NULL, // VK_F20					0x83	F20 key
K_NULL, // VK_F21					0x84	F21 key
K_NULL, // VK_F22					0x85	F22 key
K_NULL, // VK_F23					0x86	F23 key
K_NULL, // VK_F24					0x87	F24 key
K_NULL, // -						0x88	Unassigned
K_NULL, // -						0x89	Unassigned
K_NULL, // -						0x8A	Unassigned
K_NULL, // -						0x8B	Unassigned
K_NULL, // -						0x8C	Unassigned
K_NULL, // -						0x8D	Unassigned
K_NULL, // -						0x8E	Unassigned
K_NULL, // -						0x8F	Unassigned
K_NULL, // VK_NUMLOCK				0x90	NUMLOCK key
K_NULL, // VK_SCROLL				0x91	SCROLLLOCK key
K_NULL, // -						0x92	OEM specific
K_NULL, // -						0x93	OEM specific
K_NULL, // -						0x94	OEM specific
K_NULL, // -						0x95	OEM specific
K_NULL, // -						0x96	OEM specific
K_NULL, // -						0x97	Unassigned
K_NULL, // -						0x98	Unassigned
K_NULL, // -						0x99	Unassigned
K_NULL, // -						0x9A	Unassigned
K_NULL, // -						0x9B	Unassigned
K_NULL, // -						0x9C	Unassigned
K_NULL, // -						0x9D	Unassigned
K_NULL, // -						0x9E	Unassigned
K_NULL, // -						0x9F	Unassigned
K_NULL, // VK_LSHIFT				0xA0	Left SHIFT key
K_NULL, // VK_RSHIFT				0xA1	Right SHIFT key
K_NULL, // VK_LCONTROL			0xA2	Left CONTROL key
K_NULL, // VK_RCONTROL			0xA3	Right CONTROL key
K_NULL, // VK_LMENU				0xA4	Left ALT key
K_NULL, // VK_RMENU				0xA5	Right ALT key
K_NULL, // VK_BROWSER_BACK		0xA6	BrowserBack key
K_NULL, // VK_BROWSER_FORWARD		0xA7	BrowserForward key
K_NULL, // VK_BROWSER_REFRESH		0xA8	BrowserRefresh key
K_NULL, // VK_BROWSER_STOP		0xA9	BrowserStop key
K_NULL, // VK_BROWSER_SEARCH		0xAA	BrowserSearch key
K_NULL, // VK_BROWSER_FAVORITES	0xAB	BrowserFavorites key
K_NULL, // VK_BROWSER_HOME		0xAC	BrowserStart and Home key
K_NULL, // VK_VOLUME_MUTE			0xAD	VolumeMute key
K_NULL, // VK_VOLUME_DOWN			0xAE	VolumeDown key
K_NULL, // VK_VOLUME_UP			0xAF	VolumeUp key
K_NULL, // VK_MEDIA_NEXT_TRACK	0xB0	NextTrack key
K_NULL, // VK_MEDIA_PREV_TRACK	0xB1	PreviousTrack key
K_NULL, // VK_MEDIA_STOP			0xB2	StopMedia key
K_NULL, // VK_MEDIA_PLAY_PAUSE	0xB3	Play/PauseMedia key
K_NULL, // VK_LAUNCH_MAIL			0xB4	StartMail key
K_NULL, // VK_LAUNCH_MEDIA_SELECT	0xB5	SelectMedia key
K_NULL, // VK_LAUNCH_APP1			0xB6	StartApplication1 key
K_NULL, // VK_LAUNCH_APP2			0xB7	StartApplication2 key
K_NULL, // -						0xB8	Reserved
K_NULL, // -						0xB9	Reserved
K_NULL, // VK_OEM_1				0xBA	Miscellaneous
K_NULL, // VK_OEM_PLUS			0xBB	For any country/region, the '+' key
K_NULL, // VK_OEM_COMMA			0xBC	For any country/region, the ',' key
K_NULL, // VK_OEM_MINUS			0xBD	For any country/region, the '-' key
K_PERIOD, // VK_OEM_PERIOD			0xBE	For any country/region, the '.' key
K_NULL, // VK_OEM_2				0xBF	Miscellaneous
K_NULL, // VK_OEM_3				0xC0	Miscellaneous
K_NULL, // -						0xC1	Reserved
K_NULL, // -						0xC2	Reserved
K_NULL, // -						0xC3	Reserved
K_NULL, // -						0xC4	Reserved
K_NULL, // -						0xC5	Reserved
K_NULL, // -						0xC6	Reserved
K_NULL, // -						0xC7	Reserved
K_NULL, // -						0xC8	Reserved
K_NULL, // -						0xC9	Reserved
K_NULL, // -						0xCA	Reserved
K_NULL, // -						0xCB	Reserved
K_NULL, // -						0xCC	Reserved
K_NULL, // -						0xCD	Reserved
K_NULL, // -						0xCE	Reserved
K_NULL, // -						0xCF	Reserved
K_NULL, // -						0xD0	Reserved
K_NULL, // -						0xD1	Reserved
K_NULL, // -						0xD2	Reserved
K_NULL, // -						0xD3	Reserved
K_NULL, // -						0xD4	Reserved
K_NULL, // -						0xD5	Reserved
K_NULL, // -						0xD6	Reserved
K_NULL, // -						0xD7	Reserved
K_NULL, // -						0xD8	Unassigned
K_NULL, // -						0xD9	Unassigned
K_NULL, // -						0xDA	Unassigned
K_NULL, // VK_OEM_4				0xDB	Miscellaneous
K_NULL, // VK_OEM_5				0xDC	Miscellaneous
K_NULL, // VK_OEM_6				0xDD	Miscellaneous
K_NULL, // VK_OEM_7				0xDE	Miscellaneous
K_NULL, // VK_OEM_8				0xDF	Miscellaneous
K_NULL, // -						0xE0	Reserved
K_NULL, // 						0xE1	OEM specific
K_NULL, // VK_OEM_102				0xE2	The <> keys on the US standard keyboard, or the \\| key on the non-US102-key keyboard
K_NULL, // 						0xE3
K_NULL, // 						0xE4	OEM specific
K_NULL, // VK_PROCESSKEY			0xE5	IMEPROCESS key
K_NULL, // 						0xE6	OEM specific
K_NULL, // VK_PACKET				0xE7	Used to pass Unicode characters...
K_NULL, // -						0xE8	Unassigned
K_NULL, // 						0xE9	OEM specific
K_NULL, // 						0xEA	OEM specific
K_NULL, // 						0xEB	OEM specific
K_NULL, // 						0xEC	OEM specific
K_NULL, // 						0xED	OEM specific
K_NULL, // 						0xEE	OEM specific
K_NULL, // 						0xEF	OEM specific
K_NULL, // -						0xF0	OEM specific
K_NULL, // -						0xF1	OEM specific
K_NULL, // -						0xF2	OEM specific
K_NULL, // -						0xF3	OEM specific
K_NULL, // -						0xF4	OEM specific
K_NULL, // -						0xF5	OEM specific
K_NULL, // VK_ATTN				0xF6	Attn key
K_NULL, // VK_CRSEL				0xF7	CrSel key
K_NULL, // VK_EXSEL				0xF8	ExSel key
K_NULL, // VK_EREOF				0xF9	EraseEOF key
K_NULL, // VK_PLAY				0xFA	Play key
K_NULL, // VK_ZOOM				0xFB	Zoom key
K_NULL, // VK_NONAME				0xFC	Reserved
K_NULL, // VK_PA1					0xFD	PA1 key
K_NULL, // VK_OEM_CLEAR			0xFE	Clear key
K_NULL, // -						0xFF	Unassigned
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform implementation types and state

struct WindowImpl
{
	HINSTANCE hInstance;
	HWND hWnd;
	WINDOWPLACEMENT windowPlacement = { sizeof(WINDOWPLACEMENT) };
};

static WindowImpl windowImpl;

static DynamicLibrary gamepadLibrary;

static DynamicLibrary audioLibrary;
static LPDIRECTSOUNDBUFFER audioBuffer;

typedef HRESULT FP_DirectSoundCreate( LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter );

static const char *engineLibFilename = "engine.dll";
static const char *engineLibTmpFilename = "engine.tmp.dll";


////////////////////////////////////////////////////////////////////////////////////////////////////
// Directories

static bool IsAbsolutePath(const char *path)
{
	const bool res = path[1] == ':' && path[0] >= 'A' && path[0] <= 'Z';
	return res;
}

static void InitializeDirectories(Platform &platform)
{
	char buffer[MAX_PATH_LENGTH];
	char *workingDir = _getcwd(buffer, ARRAY_COUNT(buffer));
	InitializeDirectoriesFromWorkingDir(platform, workingDir);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Window and events

static void ToggleFullscreen(HWND hWnd)
{
	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	if (style & WS_OVERLAPPEDWINDOW)
	{
		MONITORINFO mi = { sizeof(mi) };
		if (GetWindowPlacement(hWnd, &windowImpl.windowPlacement) &&
			GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mi))
		{
			SetWindowLong(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(hWnd, HWND_TOP,
				mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else
	{
		SetWindowLong(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(hWnd, &windowImpl.windowPlacement);
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

static void UpdateAndRender(Platform &platform);

static u32 capturedButtons = 0; // bitmask: bit i set means MOUSE_BUTTON_i is currently pressed

static void CaptureButton(MouseButton button, Window &window)
{
	if (!capturedButtons) {
		SetCapture(window.impl->hWnd);
	}
	capturedButtons |= (1 << button);
}

static void ReleaseButton(MouseButton button)
{
	capturedButtons &= ~(1 << button);
	if (!capturedButtons) {
		ReleaseCapture();
	}
}

static void ReleaseCapturedButtons(Platform &platform, Window &window)
{
	for (u32 i = 0; i < MOUSE_BUTTON_COUNT; ++i)
	{
		if (capturedButtons & (1 << i))
		{
			const PlatformEvent event = {
				.type = PlatformEventTypeMouseClick,
				.mouseClick = { .button = (MouseButton)i, .state = BUTTON_STATE_RELEASE },
			};
			SendPlatformEvent(platform, event);
		}
	}
	capturedButtons = 0;
}

// Timer used to keep rendering while the window is being moved or resized
#define SIZEMOVE_TIMER_ID 1

static LRESULT CALLBACK Win32WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool processMouseEvents = true;
	bool processKeyboardEvents = true;

	switch (uMsg)
	{
		case WM_KEYDOWN:
		case WM_KEYUP:
			if ( processKeyboardEvents )
			{
				WPARAM keyCode = wParam;
				ASSERT( keyCode < ARRAY_COUNT(Win32KeyMappings) );
				const Key mapping = Win32KeyMappings[ keyCode ];
				ASSERT( mapping < K_COUNT );
				const KeyState state = uMsg == WM_KEYDOWN ? KEY_STATE_PRESS : KEY_STATE_RELEASE;
				const PlatformEvent event = {
					.type = PlatformEventTypeKeyPress,
					.keyPress = {
						.code = mapping,
						.state = state,
					},
				};
				SendPlatformEvent(platform, event);
			}
			break;

		case WM_SYSKEYDOWN:
			if (wParam == VK_RETURN)
				ToggleFullscreen(hWnd);
			else
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			break;

		case WM_SYSCHAR:
			// Handled to suppress the system notification sound on Alt+Enter.
			break;

		case WM_LBUTTONDOWN:
			if ( processMouseEvents )
			{
				CaptureButton(MOUSE_BUTTON_LEFT, platform.window);
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_LEFT,
						.state = BUTTON_STATE_PRESS,
					},
				};
				SendPlatformEvent(platform, event);
			}
			break;
		case WM_LBUTTONUP:
			if ( processMouseEvents )
			{
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_LEFT,
						.state = BUTTON_STATE_RELEASE,
					},
				};
				SendPlatformEvent(platform, event);
				ReleaseButton(MOUSE_BUTTON_LEFT);
			}
			break;
		case WM_RBUTTONDOWN:
			if ( processMouseEvents )
			{
				CaptureButton(MOUSE_BUTTON_RIGHT, platform.window);
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_RIGHT,
						.state = BUTTON_STATE_PRESS,
					},
				};
				SendPlatformEvent(platform, event);
			}
			break;
		case WM_RBUTTONUP:
			if ( processMouseEvents )
			{
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_RIGHT,
						.state = BUTTON_STATE_RELEASE,
					},
				};
				SendPlatformEvent(platform, event);
				ReleaseButton(MOUSE_BUTTON_RIGHT);
			}
			break;
		case WM_MBUTTONDOWN:
			if ( processMouseEvents )
			{
				CaptureButton(MOUSE_BUTTON_MIDDLE, platform.window);
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_MIDDLE,
						.state = BUTTON_STATE_PRESS,
					},
				};
				SendPlatformEvent(platform, event);
			}
			break;
		case WM_MBUTTONUP:
			if ( processMouseEvents )
			{
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_MIDDLE,
						.state = BUTTON_STATE_RELEASE,
					},
				};
				SendPlatformEvent(platform, event);
				ReleaseButton(MOUSE_BUTTON_MIDDLE);
			}
			break;

		case WM_MOUSEWHEEL:
			if ( processMouseEvents )
			{
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseWheel,
					.mouseWheel = { .delta = { .x = 0, .y = -GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA } },
				};
				SendPlatformEvent(platform, event);
			}
			break;

		case WM_MOUSEHWHEEL:
			if ( processMouseEvents )
			{
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseWheel,
					.mouseWheel = { .delta = { .x = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA, .y = 0} },
				};
				SendPlatformEvent(platform, event);
			}
			break;

		case WM_MOUSEMOVE:
			if ( processMouseEvents )
			{
				i16 xPos = Max(0, Min((i32)platform.window.width, GET_X_LPARAM(lParam)));
				i16 yPos = Max(0, Min((i32)platform.window.height, GET_Y_LPARAM(lParam)));
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseMove,
					.mouseMove = { .pos = { .x = xPos, .y = yPos } }
				};
				SendPlatformEvent(platform, event);
				//LOG( Info, "Mouse at position (%d, %d)\n", xPos, yPos );
			}
			break;

		case WM_CAPTURECHANGED:
			{
				ReleaseCapturedButtons(platform, platform.window);
				break;
			}

		case WM_MOUSEHOVER:
		case WM_MOUSELEAVE:
			{
				// These events are disabled by default. See documentation if needed:
				// https://learn.microsoft.com/en-us/windows/win32/learnwin32/other-mouse-operations
				//LOG( Info, "Mouse %s the window\n", uMsg == WM_MOUSEHOVER ? "entered" : "left" );
				break;
			}

		case WM_PAINT:
			// Only render from here while the update thread is parked during a
			// modal size/move loop; outside of it the update thread presents
			// continuously and this paint would be redundant. It would also
			// deadlock at startup: ShowWindow is called from the update thread
			// with the render lock held, and blocks until this thread processes
			// the sync-paint it sends, which would wait on that same lock.
			if ( platform.inSizeMove && platform.windowInitialized )
			{
				UpdateAndRender(platform);
			}
			// Must always validate, even when not rendering here: an unvalidated
			// WM_PAINT is regenerated by Windows on every message check, which
			// would starve PlatformUpdateEventLoop's PeekMessage loop forever.
			ValidateRect(hWnd, NULL);
			return 0;

		case WM_SIZE:
			{
				const u16 width = LOWORD(lParam);
				const u16 height = HIWORD(lParam);
				// The window size is applied by ProcessPlatformEvents on whichever
				// thread renders next. Writing it directly here would race with the
				// update thread clearing window flags and could lose resizes.
				const PlatformEvent event = {
					.type = PlatformEventTypeWindowResize,
					.windowResize = { .width = width, .height = height },
				};
				SendPlatformEvent(platform, event);
				break;
			}

		case WM_ENTERSIZEMOVE:
			// While the modal size/move loop blocks this thread's message pump,
			// the update thread pauses and rendering is driven from here instead:
			// WM_PAINT fires after each size change, and the timer keeps frames
			// coming while the window is only being moved (moves generate no paints).
			platform.inSizeMove = 1;
			SetTimer(hWnd, SIZEMOVE_TIMER_ID, 10, NULL);
			break;

		case WM_EXITSIZEMOVE:
			KillTimer(hWnd, SIZEMOVE_TIMER_ID);
			platform.inSizeMove = 0;
			break;

		case WM_TIMER:
			if ( wParam == SIZEMOVE_TIMER_ID && platform.inSizeMove && platform.windowInitialized )
			{
				UpdateAndRender(platform);
			}
			break;

		case WM_SYSCOMMAND:
			{
				WPARAM param = ( wParam & 0xFFF0 );
				AudioDevice &audio = platform.audio;

				if (param == SC_MINIMIZE)
				{
					if ( audio.initialized && audio.isPlaying ) {
						audioBuffer->Stop();
						audio.isPlaying = false;
					}
				}
				else if (param == SC_RESTORE)
				{
					if ( audio.initialized && !audio.isPlaying ) {
						audioBuffer->Play(0, 0, DSBPLAY_LOOPING);
						audio.isPlaying = true;
						platform.audio.soundIsValid = false;
					}
				}

				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			};

		case WM_CREATE:
			{
				const PlatformEvent event = { .type = PlatformEventTypeWindowWasCreated };
				SendPlatformEvent(platform, event);
				break;
			}

		case WM_CLOSE:
			{
				// If we want to show a dialog to ask the user for confirmation before
				// closing the window, it should be done here. Zero should be returned
				// to indicate that we handled this message.
				// Otherwise, calling DefWindowProc will internally call DestroyWindow
				// and will internally send the WM_DESTROY message.
				const PlatformEvent event = { .type = PlatformEventTypeWindowWillDestroy, };
				SendPlatformEvent(platform, event);
				DestroyWindow(hWnd);
				break;
			}

		case WM_DESTROY:
			{
				PlatformQuit();
				// This inserts a WM_QUIT message in the queue, which will in turn cause
				// GetMessage to return zero. We will exit the main loop when that happens.
				// On the other hand, PeekMessage has to handle WM_QUIT messages explicitly.
				PostQuitMessage(0);
				break;
			}

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

static bool InitializeWindow(Window &window, u32 width, u32 height, const char *title)
{
	ZeroStruct(&window);
	window.width = width;
	window.height = height;

	ZeroStruct(&windowImpl);
	window.impl = &windowImpl;

	FilePath iconPath = MakePath(ProjectDir, "editor/ilu.ico");

	// Register the window class.
	const char CLASS_NAME[]  = "Ilu Class";
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = Win32WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon         = (HICON)LoadImage(NULL, iconPath.str, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
	//wc.hbrBackground = GetSysColorBrush(COLOR_GRAYTEXT); //(HBRUSH)GetStockObject(GRAY_BRUSH); //CreateSolidBrush(RGB(20,20,20));
	ATOM classAtom = RegisterClassA(&wc);

	if (classAtom == 0)
	{
		Win32ReportError("InitializeWindow - RegisterClassA");
		return false;
	}

	// Given the desired client window size, get the full size
	RECT windowRect = { 0, 0, (int)width, (int)height };
	AdjustWindowRect( &windowRect, WS_OVERLAPPEDWINDOW, FALSE );
	int fullWidth = windowRect.right - windowRect.left;
	int fullHeight = windowRect.bottom - windowRect.top;

	// Ignore display scaling
	SetProcessDPIAware();

	HWND hWnd = CreateWindowExA(
			0,                              // Optional window styles.
			CLASS_NAME,                     // Window class
			title,                          // Window text
			WS_OVERLAPPEDWINDOW,            // Window style
			CW_USEDEFAULT, CW_USEDEFAULT,   // Position
			fullWidth, fullHeight,          // Size
			NULL,                           // Parent window
			NULL,                           // Menu
			hInstance,                      // Instance handle
			NULL                            // Additional application data
			);

	if (hWnd == NULL)
	{
		Win32ReportError("InitializeWindow - CreateWindowExA");
		return false;
	}

	window.impl->hInstance = hInstance;
	window.impl->hWnd = hWnd;

	return true;
}

static void CleanupWindow(Window &window)
{
	DestroyWindow(window.impl->hWnd);
}

static void ShowPlatformWindow(Window &window)
{
	ShowWindow(window.impl->hWnd, SW_SHOW);
	SetForegroundWindow(window.impl->hWnd);
}

static void PlatformWakeMainThread()
{
	if ( windowImpl.hWnd )
	{
		PostMessageA(windowImpl.hWnd, WM_CLOSE, 0, 0);
	}
}

static void PlatformUpdateEventLoop(Platform &platform)
{
	BOOL ret = 1;
	MSG msg = { };
#if USE_UPDATE_THREAD
	while ( (ret = GetMessage( &msg, NULL, 0, 0 )) != 0 ) // ret == 0 means WM_QUIT
#else
	while ( PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ) )
#endif
	{
		if ( ret == -1 )
		{
			LOG(Error, "Fatal error returned by GetMessage\n");
			break;
		}
		else if ( LOWORD( msg.message ) == WM_QUIT )
		{
			// TODO: Send platform event?
			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// File watching

#define MAX_FILE_WATCHES 4

struct FileWatch
{
	HANDLE handle;
	OVERLAPPED overlapped;
	DWORD buffer[4096]; // DWORD-aligned scratch required by ReadDirectoryChangesW; contents unused
	bool pending;
};

static FileWatch fileWatches[MAX_FILE_WATCHES];
static u32 fileWatchCount = 0;

static bool IssueReadDirectoryChanges(FileWatch &watch)
{
	watch.overlapped = {};
	watch.pending = ReadDirectoryChangesW(
			watch.handle,
			watch.buffer, sizeof(watch.buffer),
			FALSE, // do not watch the subtree, flat only
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			NULL, &watch.overlapped, NULL) != 0;
	return watch.pending;
}

static bool WatchDirectory(Platform &platform, const char *path, bool recursive)
{
	ASSERT(!recursive); // Recursive watches are not supported yet

	if (fileWatchCount == MAX_FILE_WATCHES)
	{
		LOG(Warning, "WatchDirectory: no free watch slots for %s\n", path);
		return false;
	}

	FileWatch &watch = fileWatches[fileWatchCount];
	watch = {};

	watch.handle = CreateFileA(path,
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL);

	if (watch.handle == INVALID_HANDLE_VALUE)
	{
		Win32ReportError("WatchDirectory - CreateFileA");
		return false;
	}

	if (!IssueReadDirectoryChanges(watch))
	{
		Win32ReportError("WatchDirectory - ReadDirectoryChangesW");
		CloseHandle(watch.handle);
		return false;
	}

	fileWatchCount++;
	return true;
}

static void UpdateFileWatcher(Platform &platform)
{
	for (u32 i = 0; i < fileWatchCount; ++i)
	{
		FileWatch &watch = fileWatches[i];

		if (!watch.pending)
		{
			continue;
		}

		DWORD transferred = 0;
		if (GetOverlappedResult(watch.handle, &watch.overlapped, &transferred, FALSE))
		{
			// We don't care what changed or what the buffer contains, only that something did
			platform.pub.fileChangesDetected = true;

			IssueReadDirectoryChanges(watch);
		}
		else if (GetLastError() != ERROR_IO_INCOMPLETE)
		{
			Win32ReportError("UpdateFileWatcher - GetOverlappedResult");
			watch.pending = false;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamepad

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD userIndex, XINPUT_STATE *state)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD userIndex, XINPUT_VIBRATION *vibration)

typedef X_INPUT_GET_STATE(XInputGetState_t);
typedef X_INPUT_SET_STATE(XInputSetState_t);

static X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_INVALID_FUNCTION;
}

static X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_INVALID_FUNCTION;
}

static XInputGetState_t *FP_XInputGetState = XInputGetStateStub;
static XInputSetState_t *FP_XInputSetState = XInputSetStateStub;

static bool InitializeGamepad(Platform &platform)
{
	LOG(Info, "Input system initialization:\n");

	platform.pub.gamepad = &platform.gamepad;

	bool found = false;

	const char *libraryNames[] = {
		"xinput1_4.dll",
		"xinput9_1_0.dll",
		"xinput1_3.dll",
	};

	const char *libraryName = nullptr;
	for (u32 i = 0; i < ARRAY_COUNT(libraryNames); ++i) {
		libraryName = libraryNames[i];
		gamepadLibrary = OpenLibrary(libraryName);
		if (gamepadLibrary) {
			break;
		}
	}

	if (gamepadLibrary)
	{
		LOG(Info, "- Loaded %s successfully\n", libraryName);

		XInputGetState_t* getState = (XInputGetState_t*)LoadSymbol(gamepadLibrary, "XInputGetState");
		XInputSetState_t* setState = (XInputSetState_t*)LoadSymbol(gamepadLibrary, "XInputSetState");

		if ( getState != nullptr ) {
			LOG(Info, "- XInputGetState symbol loaded successfully\n");
			FP_XInputGetState = getState;
			found = true;
		} else {
			LOG(Warning, "- Error loading XInputGetState symbol\n");
			FP_XInputGetState = XInputGetStateStub;
			found = false;
		}
		if ( setState != nullptr ) {
			LOG(Info, "- XInputSetState symbol loaded successfully\n");
			FP_XInputSetState = setState;
			found = found && true;
		} else {
			LOG(Warning, "- Error loading XInputSetState\n");
			FP_XInputSetState = XInputSetStateStub;
			found = false;
		}
	}

	return found;
}

static ButtonState ButtonStateFromXInput(WORD prevButtonMask, WORD currButtonMask, WORD buttonBit)
{
	const u32 wasDown = ( prevButtonMask & buttonBit ) ? 1 : 0;
	const u32 isDown = ( currButtonMask & buttonBit ) ? 1 : 0;
	constexpr ButtonState stateMatrix[2][2] = {
		// isUp,  isDown
		{ BUTTON_STATE_IDLE , BUTTON_STATE_PRESS }, // wasUp
		{ BUTTON_STATE_RELEASE ,BUTTON_STATE_PRESSED }, // wasDown
	};
	const ButtonState state = stateMatrix[wasDown][isDown];
	return state;
}

static f32 TriggerFromXInput(BYTE trigger)
{
	const f32 res = (f32)trigger/255.0f;
	return res;
}

static f32 AxisFromXInput(SHORT axis, SHORT deadzoneThreshold)
{
	f32 normalizedAxis = 0.0f;
	if (axis < -deadzoneThreshold) {
		normalizedAxis = (f32)(axis + deadzoneThreshold)/(32768.0f - deadzoneThreshold);
	} else if (axis > deadzoneThreshold) {
		normalizedAxis = (f32)(axis - deadzoneThreshold)/(32767.0f - deadzoneThreshold);
	}
	return normalizedAxis;
}

static void UpdateGamepad(Platform &platform)
{
	Gamepad &gamepad = platform.gamepad;

	for ( DWORD i = 0; i < XUSER_MAX_COUNT; ++i )
	{
		XINPUT_STATE controllerState;
		if (FP_XInputGetState(i, &controllerState) == ERROR_SUCCESS)
		{
			static XINPUT_GAMEPAD prevXPad = {};

			//int packetNumber = controllerState.dwPacketNumber;
			const XINPUT_GAMEPAD &xpad = controllerState.Gamepad;

			gamepad.start = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_START);
			gamepad.back = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_BACK);
			gamepad.up = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_UP);
			gamepad.down = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
			gamepad.left = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
			gamepad.right = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
			gamepad.a = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_A);
			gamepad.b = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_B);
			gamepad.x = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_X);
			gamepad.y = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_Y);
			gamepad.leftShoulder = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
			gamepad.rightShoulder = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
			gamepad.leftTrigger = TriggerFromXInput(xpad.bLeftTrigger);
			gamepad.rightTrigger = TriggerFromXInput(xpad.bRightTrigger);
			gamepad.leftAxis.x = AxisFromXInput(xpad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad.leftAxis.y = AxisFromXInput(xpad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad.rightAxis.x = AxisFromXInput(xpad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			gamepad.rightAxis.y = AxisFromXInput(xpad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

			prevXPad = xpad;

			// Only one gamepad supported
			break;
		}
		else
		{
			// Controller not available
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Audio

static void Win32FillAudioBuffer(AudioDevice &audio, DWORD writeOffset, DWORD writeSize, const i16 *audioSamples)
{
	ASSERT(writeSize <= audio.bufferSize);

	void *region1;
	DWORD region1Size;
	void *region2;
	DWORD region2Size;

	if (audioBuffer->Lock(writeOffset, writeSize, &region1, &region1Size, &region2, &region2Size, 0) == DS_OK)
	{
		const i16 *srcSample = audioSamples;

		i16 *dstSample = (i16*)region1;
		const u32 region1SampleCount = region1Size / (audio.bytesPerSample * audio.channelCount);
		for (u32 i = 0; i < region1SampleCount; ++i)
		{
			*dstSample++ = *srcSample++;
			*dstSample++ = *srcSample++;
			audio.runningSampleIndex++;
		}

		dstSample = (i16*)region2;
		const u32 region2SampleCount = region2Size / (audio.bytesPerSample * audio.channelCount);
		for (u32 i = 0; i < region2SampleCount; ++i)
		{
			*dstSample++ = *srcSample++;
			*dstSample++ = *srcSample++;
			audio.runningSampleIndex++;
		}

		audioBuffer->Unlock(region1, region1Size, region2, region2Size);
	}
	else
	{
		LOG(Warning, "Failed to Lock sound buffer.\n");
		audio.soundIsValid = false;
	}
}

static bool InitializeAudioDevice(Platform &platform)
{
	AudioDevice &audio = platform.audio;
	const Window &window = platform.window;

	audioLibrary = OpenLibrary("dsound.dll");

	if (audioLibrary)
	{
		LOG(Info, "- Loaded dsound.dll successfully\n");

		FP_DirectSoundCreate* CreateAudioDevice = (FP_DirectSoundCreate*)LoadSymbol(audioLibrary, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if (CreateAudioDevice && SUCCEEDED(CreateAudioDevice(0, &directSound, 0)))
		{
			LOG(Info, "- Audio device created successfully\n");

			if (SUCCEEDED(directSound->SetCooperativeLevel(window.impl->hWnd, DSSCL_PRIORITY)))
			{
				// We create a primary buffer just to set the wanted format and avoid the API resample sounds to whatever rate it uses by default
				DSBUFFERDESC primaryBufferDesc = {};
				primaryBufferDesc.dwSize = sizeof(primaryBufferDesc);
				primaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&primaryBufferDesc, &primaryBuffer, 0)))
				{
					LOG(Info, "- Primary buffer created successfully\n");

					WAVEFORMATEX waveFormat = {};
					waveFormat.wFormatTag = WAVE_FORMAT_PCM;
					waveFormat.nChannels = audio.channelCount;
					waveFormat.nSamplesPerSec = audio.samplesPerSecond;
					waveFormat.nBlockAlign = audio.channelCount * audio.bytesPerSample;
					waveFormat.nAvgBytesPerSec = audio.samplesPerSecond * waveFormat.nBlockAlign;
					waveFormat.wBitsPerSample = audio.bytesPerSample * 8;
					waveFormat.cbSize = 0;
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
					{
						LOG(Info, "- Primary buffer format set successfully\n");

						// After setting the primary buffer format, we create the secondary buffer where we will be actually writing to
						DSBUFFERDESC secondaryBufferDesc = {};
						secondaryBufferDesc.dwSize = sizeof(secondaryBufferDesc);
						// TODO(jesus): Set the DSBCAPS_GETCURRENTPOSITION2 flag?
						// TODO(jesus): Set the DSBCAPS_GLOBALFOCUS flag?
						secondaryBufferDesc.dwFlags = 0;
						secondaryBufferDesc.dwBufferBytes = audio.bufferSize;
						secondaryBufferDesc.lpwfxFormat = &waveFormat;
						LPDIRECTSOUNDBUFFER secondaryBuffer;
						if (SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDesc, &secondaryBuffer, 0)))
						{
							LOG(Info, "- Secondary buffer created successfully\n");

							audioBuffer = secondaryBuffer;

							MemSet(audio.outputSamples, audio.bufferSize, 0);
							Win32FillAudioBuffer(platform.audio, 0, platform.audio.bufferSize, audio.outputSamples);

							if (SUCCEEDED(audioBuffer->Play(0, 0, DSBPLAY_LOOPING)))
							{
								LOG(Info, "- Secondary buffer is playing...\n");
								audio.initialized = true;
								audio.isPlaying = true;
								audio.soundIsValid = false;
							}
							else
							{
								LOG(Error, "- Error playing secondaryBuffer.\n");
							}
						}
						else
						{
							LOG(Error, "- Error calling CreateSoundBuffer for secondaryBuffer\n");
						}
					}
					else
					{
						LOG(Error, "- Error setting primary buffer format\n");
					}
				}
				else
				{
					LOG(Error, "- Error calling CreateSoundBuffer for primaryBuffer\n");
				}
			}
			else
			{
				LOG(Error, "- Error setting DirectSound priority cooperative level\n");
			}
		}
		else
		{
			LOG(Error, "- Error loading DirectSoundCreate symbol\n");
		}
	}
	else
	{
		LOG(Error, "- Error loading dsound.dll\n");
	}

	return audio.initialized;
}

// DirectSound exposes no readiness primitive to block on, so this backend is still paced by a
// fixed sleep. Moving to WASAPI in event driven mode would replace this with a wait on the
// event the device signals when it wants more samples.
static void WaitForAudioDevice(Platform &platform)
{
	SleepMillis(10);
}

static void UpdateAudioDevice(Platform &platform)
{
	AudioDevice &audio = platform.audio;

	DWORD playCursor;
	DWORD writeCursor;
	if (audioBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
	{
		// Audio just started playing
		if (!audio.soundIsValid) {
			audio.runningSampleIndex = writeCursor / (audio.bytesPerSample * audio.channelCount);
			audio.soundIsValid = true;
		}

		const DWORD byteToLock = (audio.runningSampleIndex * audio.bytesPerSample * audio.channelCount) % audio.bufferSize;

		// Bytes of audio the device consumes during one write-ahead window.
		const DWORD bytesPerSecond = audio.samplesPerSecond * audio.channelCount * audio.bytesPerSample;
		const DWORD expectedBytesPerWindow = (bytesPerSecond * audio.writeAheadMillis) / 1000;
		const DWORD expectedWindowBoundaryByte = playCursor + expectedBytesPerWindow;

		DWORD safeWriteCursor = writeCursor;
		if (safeWriteCursor < playCursor) {
			safeWriteCursor += audio.bufferSize;
		}
		ASSERT(safeWriteCursor >= playCursor);

		const bool audioCardIsLowLatency = safeWriteCursor < expectedWindowBoundaryByte;

		DWORD targetCursor = 0;
		if (audioCardIsLowLatency) {
			targetCursor = expectedWindowBoundaryByte + expectedBytesPerWindow;
		} else {
			targetCursor = writeCursor + expectedBytesPerWindow + audio.safetyBytes;
		}
		targetCursor = targetCursor % audio.bufferSize;

		DWORD bytesToWrite = 0;
		if (byteToLock > targetCursor) {
			bytesToWrite = targetCursor + (audio.bufferSize - byteToLock);
		} else {
			bytesToWrite = targetCursor - byteToLock;
		}

		#if 0 // Debug code to print audio latency
		// Latency calculation
		DWORD unwrappedWriteCursor = writeCursor;
		if (writeCursor < playCursor) {
			unwrappedWriteCursor += audio.bufferSize;
		}
		const DWORD latencySize = unwrappedWriteCursor - playCursor;
		const DWORD latencySamples = latencySize / ( audio.bytesPerSample * audio.channelCount );
		const f32 latencySeconds = (f32)latencySamples / (f32)audio.samplesPerSecond;
		LOG(Debug, "latency: %u bytes (%fs)\n", latencySize, latencySeconds);
		#endif

		SoundBuffer soundBuffer = {};
		soundBuffer.samplesPerSecond = audio.samplesPerSecond;
		soundBuffer.sampleCount = bytesToWrite / (audio.bytesPerSample * audio.channelCount);
		soundBuffer.samples = audio.outputSamples;
		platform.RenderAudioCallback(platform.pub, soundBuffer);

		Win32FillAudioBuffer(audio, byteToLock, bytesToWrite, soundBuffer.samples);
	}
	else
	{
		LOG(Warning, "Failed to GetCurrentPosition for sound buffer.\n");
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

int main(int argc, char **argv)
{
	Main(argc, argv);
	return 0;
}
