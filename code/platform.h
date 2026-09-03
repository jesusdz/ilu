
#ifndef PLATFORM_H
#define PLATFORM_H

#include "ilu_core.h"

#ifndef ILU_GFX_H
#error "ilu_gfx.h must be included before this header"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform types

enum Key
{
	K_NULL,
	K_LEFT, K_RIGHT, K_UP, K_DOWN,
	K_ESCAPE,
	K_SPACE,
	K_BACKSPACE,
	K_DELETE,
	K_RETURN,
	K_TAB,
	K_CONTROL,
	K_SHIFT,
	K_ALT,
	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_0, K_1, K_2,
	K_3, K_4, K_5,
	K_6, K_7, K_8, K_9,
	K_A, K_B, K_C, K_D,
	K_E, K_F, K_G, K_H,
	K_I, K_J, K_K, K_L,
	K_M, K_N, K_O, K_P,
	K_Q, K_R, K_S, K_T,
	K_U, K_V, K_W, K_X,
	K_Y, K_Z,
	K_PERIOD,
	K_COUNT,
};

enum MouseButton
{
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_COUNT,
};

enum KeyState
{
	KEY_STATE_IDLE,
	KEY_STATE_PRESS,
	KEY_STATE_PRESSED,
	KEY_STATE_RELEASE,
};

enum ButtonState
{
	BUTTON_STATE_IDLE,
	BUTTON_STATE_PRESS,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_RELEASE,
};

enum TouchState
{
	TOUCH_STATE_IDLE,
	TOUCH_STATE_PRESS,
	TOUCH_STATE_PRESSED,
	TOUCH_STATE_RELEASE,
};

struct Keyboard
{
	KeyState keys[K_COUNT];
};

struct Mouse
{
	int2 pos;
	int2 delta;
	int2 wheel;
	int2 click;
	int2 drag;
	ButtonState buttons[MOUSE_BUTTON_COUNT];
};

#define MAX_INPUT_CHARS 16

struct Chars
{
	char chars[MAX_INPUT_CHARS];
	u32 charCount;
	bool shift;
	bool ctrl;
	bool alt;
};

struct Touch
{
	f32 x0, y0;
	f32 x, y;
	f32 dx, dy;
	TouchState state;
};

inline bool KeyPress(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_PRESS;
}

inline bool KeyPressed(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return KeyPress(keyboard, key) || keyboard.keys[key] == KEY_STATE_PRESSED;
}

inline bool KeyRelease(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_RELEASE;
}

inline bool ButtonPress(ButtonState state)
{
	return state == BUTTON_STATE_PRESS;
}

inline bool ButtonPressed(ButtonState state)
{
	return state == BUTTON_STATE_PRESSED;
}

inline bool ButtonRelease(ButtonState state)
{
	return state == BUTTON_STATE_RELEASE;
}

inline bool MouseMoved(const Mouse &mouse)
{
	return mouse.delta.x != 0 || mouse.delta.y != 0;
}

inline bool MouseButtonPress(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonPress(mouse.buttons[button]);
}

inline bool MouseButtonPressed(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonPressed(mouse.buttons[button]);
}

inline bool MouseButtonRelease(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonRelease(mouse.buttons[button]);
}

inline bool MouseButtonChanged(const Mouse &mouse, MouseButton button)
{
	return MouseButtonPress(mouse, button) || MouseButtonRelease(mouse, button);
}

inline bool MouseChanged(const Mouse &mouse)
{
	return MouseMoved(mouse)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_LEFT)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_RIGHT)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_MIDDLE);
}

struct WindowImpl;

struct Window
{
	u32 width;
	u32 height;
	u32 flags;

	Keyboard keyboard;
	Mouse mouse;
	Chars chars;
	Touch touches[2];

	WindowImpl *impl;
};

struct Gamepad
{
	union {
		struct {
			ButtonState start;
			ButtonState back;
			ButtonState up;
			ButtonState down;
			ButtonState left;
			ButtonState right;
			ButtonState a;
			ButtonState b;
			ButtonState x;
			ButtonState y;
			ButtonState leftShoulder;
			ButtonState rightShoulder;
		};
		ButtonState buttons[12];
	};
	float leftTrigger;
	float rightTrigger;
	float2 leftAxis;
	float2 rightAxis;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Input edge latching
//
// This is a method to accumulate and retain input state so that Simulate, which can
// be executed more or less often than the platform update, doesn't miss input events.

struct InputAccumulator
{
	Gamepad gamepad;
	Keyboard keyboard;

	bool keyPendingRelease[K_COUNT];
	bool buttonPendingRelease[ARRAY_COUNT(Gamepad::buttons)];
};

inline void InputAccumulate(InputAccumulator &input, const Gamepad &newGamepad, const Keyboard &newKeyboard)
{
	for (u32 i = 0; i < K_COUNT; ++i)
	{
		const KeyState newState = newKeyboard.keys[i];
		KeyState &state = input.keyboard.keys[i];

		if (newState == KEY_STATE_PRESS)
		{
			// Latch the edge until a fixed step consumes it
			state = KEY_STATE_PRESS;
			input.keyPendingRelease[i] = false;
		}
		else if (newState == KEY_STATE_RELEASE)
		{
			if (state == KEY_STATE_PRESS) {
				// The press was not consumed yet: keep it and release right after
				input.keyPendingRelease[i] = true;
			} else {
				state = KEY_STATE_RELEASE;
			}
		}
	}
	for (u32 i = 0; i < ARRAY_COUNT(input.gamepad.buttons); ++i)
	{
		const ButtonState newState = newGamepad.buttons[i];
		ButtonState &state = input.gamepad.buttons[i];

		if (newState == BUTTON_STATE_PRESS)
		{
			// Latch the edge until a fixed step consumes it
			state = BUTTON_STATE_PRESS;
			input.buttonPendingRelease[i] = false;
		}
		else if (newState == BUTTON_STATE_RELEASE)
		{
			if (state == BUTTON_STATE_PRESS) {
				// The press was not consumed yet: keep it and release right after
				input.buttonPendingRelease[i] = true;
			} else {
				state = BUTTON_STATE_RELEASE;
			}
		}
	}
	input.gamepad.leftTrigger = newGamepad.leftTrigger;
	input.gamepad.rightTrigger = newGamepad.rightTrigger;
	input.gamepad.leftAxis = newGamepad.leftAxis;
	input.gamepad.rightAxis = newGamepad.rightAxis;
}

inline void InputConsume(InputAccumulator &input)
{
	for (u32 i = 0; i < K_COUNT; ++i)
	{
		KeyState &state = input.keyboard.keys[i];

		if (state == KEY_STATE_PRESS)
		{
			state = input.keyPendingRelease[i] ? KEY_STATE_RELEASE : KEY_STATE_PRESSED;
			input.keyPendingRelease[i] = false;
		}
		else if (state == KEY_STATE_RELEASE)
		{
			state = KEY_STATE_IDLE;
		}
	}
	for (u32 i = 0; i < ARRAY_COUNT(input.gamepad.buttons); ++i)
	{
		ButtonState &state = input.gamepad.buttons[i];

		if (state == BUTTON_STATE_PRESS)
		{
			state = input.buttonPendingRelease[i] ? BUTTON_STATE_RELEASE : BUTTON_STATE_PRESSED;
			input.buttonPendingRelease[i] = false;
		}
		else if (state == BUTTON_STATE_RELEASE)
		{
			state = BUTTON_STATE_IDLE;
		}
	}
}


struct SoundBuffer
{
	u16 samplesPerSecond;
	u16 sampleCount;
	i16* samples;
};


typedef void (*PFN_PlatformQuit)();
typedef u32  (*PFN_AcquireScratchArena)(Arena &outArena, u32 minSize);
typedef void (*PFN_ReleaseScratchArena)(u32 index);

struct PlatformAPI
{
	PFN_PlatformQuit        PlatformQuit;
	PFN_AcquireScratchArena AcquireScratchArena;
	PFN_ReleaseScratchArena ReleaseScratchArena;
};

struct Engine;

struct Plat
{
	PlatformAPI api;
	GraphicsAPI graphicsAPI;

	i32 argc;
	char **argv;

	const char *BinDir;
	const char *DataDir;
	const char *AssetDir;
	const char *ProjectDir;

	Arena *globalArena;
	Arena *frameArena;
	Arena *stringArena;
	Arena *dataArena;

	StringInterning *stringInterning;

	Window *window;
	Gamepad *gamepad;

	bool fileChangesDetected; // true for the one frame in which a watched directory changed

	Engine *engine;
};

#endif // PLATFORM_H



////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform API

#if defined(PLATFORM_API)
#ifndef PLATAFORM_API_INCLUDED
#define PLATAFORM_API_INCLUDED

extern Plat *sPlatform;

#define GlobalArena (*sPlatform->globalArena)
#define FrameArena (*sPlatform->frameArena)
#define StringArena (*sPlatform->stringArena)
#define DataArena (*sPlatform->dataArena)

extern const char *BinDir;
extern const char *DataDir;
extern const char *AssetDir;
extern const char *ProjectDir;

extern PFN_PlatformQuit        PlatformQuit;
extern PFN_AcquireScratchArena AcquireScratchArena;
extern PFN_ReleaseScratchArena ReleaseScratchArena;

inline void SetPlatformAPI(Plat &platform)
{
	sPlatform = &platform;

	BinDir       = platform.BinDir;
	DataDir      = platform.DataDir;
	AssetDir     = platform.AssetDir;
	ProjectDir   = platform.ProjectDir;

	PlatformQuit        = platform.api.PlatformQuit;
	AcquireScratchArena = platform.api.AcquireScratchArena;
	ReleaseScratchArena = platform.api.ReleaseScratchArena;
}

struct Scratch
{
	Arena arena;
	u32 lockedBit;

	Scratch(u32 size = MB(1)) { lockedBit = AcquireScratchArena(arena, size); }
	~Scratch() { ReleaseScratchArena(lockedBit); }
};

#endif //  PLATAFORM_API_INCLUDED
#endif // PLATFORM_API



////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform API implementation

#if defined(PLATFORM_API_IMPLEMENTATION)
#ifndef PLATFORM_API_IMPLEMENTATION_INCLUDED
#define PLATFORM_API_IMPLEMENTATION_INCLUDED

Plat *sPlatform = nullptr;

const char *BinDir = "";
const char *DataDir = "";
const char *AssetDir = "";
const char *ProjectDir = "";

PFN_PlatformQuit        PlatformQuit        = nullptr;
PFN_AcquireScratchArena AcquireScratchArena = nullptr;
PFN_ReleaseScratchArena ReleaseScratchArena = nullptr;

#endif // PLATFORM_API_IMPLEMENTATION_INCLUDED

#endif // PLATFORM_API_IMPLEMENTATION

