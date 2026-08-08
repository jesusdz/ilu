
#define TOOLS_GFX_FUNCTION_PROTOTYPES
#include "ilu_gfx.h"

#include "platform.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform types

struct AudioDevice
{
	// Config
	u32 channelCount;
	u32 bytesPerSample;
	u32 samplesPerSecond;

	// How far ahead of the write cursor each update fills the ring buffer, plus the extra
	// slack added when the device does not report a low latency write cursor. Both are in
	// milliseconds, so audio latency no longer depends on an assumed game frame rate.
	u32 writeAheadMillis;
	u32 safetyMillis;

	u32 safetyBytes;
	u32 runningSampleIndex;
	u32 bufferSize;

	bool initialized;
	bool isPlaying;
	bool soundIsValid;

	i16 *outputSamples;
};

enum PlatformEventType
{
	PlatformEventTypeWindowWasCreated,
	PlatformEventTypeWindowWillDestroy,
	PlatformEventTypeWindowResize,
	PlatformEventTypeKeyPress,
	PlatformEventTypeMouseClick,
	PlatformEventTypeMouseMove,
	PlatformEventTypeMouseWheel,
	PlatformEventTypeCount,
};

struct PlatformEventWindowResize
{
	u16 width, height;
};

struct PlatformEventKeyPress
{
	Key code;
	KeyState state;
};

struct PlatformEventMouseClick
{
	MouseButton button;
	ButtonState state;
};

struct PlatformEventMouseMove
{
	int2 pos;
};

struct PlatformEventMouseWheel
{
	int2 delta;
};

struct PlatformEvent
{
	PlatformEventType type;
	union
	{
		PlatformEventWindowResize windowResize;
		PlatformEventKeyPress keyPress;
		PlatformEventMouseClick mouseClick;
		PlatformEventMouseMove mouseMove;
		PlatformEventMouseWheel mouseWheel;
	};
};

#define MAX_SCRATCH_ARENAS 8

struct Platform
{
	u32 globalMemorySize = MB(64);
	u32 frameMemorySize = MB(16);
	u32 stringMemorySize = KB(16);
	u32 dataMemorySize = MB(16);

	void (*LoadEngineCallback)(Plat &);
	void (*UnloadEngineCallback)(Plat &);
	bool (*PreInitCallback)(Plat &);
	bool (*InitCallback)(Plat &);
	void (*UpdateCallback)(Plat &);
	void (*RenderGraphicsCallback)(Plat &);
	void (*PreRenderAudioCallback)(Plat &);
	void (*RenderAudioCallback)(Plat &, SoundBuffer &soundBuffer);
	void (*CleanupCallback)(Plat &);
	bool (*WindowInitCallback)(Plat &);
	void (*WindowCleanupCallback)(Plat &);
	u32 (*GetStateSignatureCallback)();

	void *userData;

	// Platform components

	Arena globalArena;
	Arena frameArena;
	Arena stringArena;
	Arena dataArena;

	Arena scratchArenas[MAX_SCRATCH_ARENAS];
	volatile_u32 scratchArenaLockMask;

	DynamicLibrary engineLib;

	u32 engineStateSignature;
	bool engineStateSignatureInitialized;

	StringInterning stringInterning;
	Window window;
	Gamepad gamepad;
	AudioDevice audio;
	f32 deltaSeconds;
	f32 totalSeconds;

	volatile_i64 eventTail;
	volatile_i64 eventHead;
	PlatformEvent events[128];

	bool paused;
	bool keepRunning;
	bool windowInitialized;
	volatile_u32 inSizeMove; // Main thread is inside a modal size/move loop

	Semaphore updateThreadFinishSemaphore;
	Mutex renderLock;

	bool audioPaused;
	Semaphore audioThreadPauseSemaphore;
	Semaphore audioThreadFinishSemaphore;

	// API exposed to the engine
	Plat pub;
};

static Platform platform = {};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread IDs

#define WORK_QUEUE_WORKER_COUNT 8

enum ThreadID
{
	THREAD_ID_UPDATE,
	THREAD_ID_AUDIO,
	THREAD_ID_WORKER_0,
	THREAD_ID_WORKER_LAST = THREAD_ID_WORKER_0 + WORK_QUEUE_WORKER_COUNT - 1,
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Per-platform interface
//
// These functions are implemented by the platform-specific file included further below
// (platform_win32.cpp, platform_linux.cpp or platform_android.cpp). Each platform file
// additionally provides:
// - The USE_UPDATE_THREAD and USE_AUDIO_THREAD macros
// - The WindowImpl struct
// - The engineLibFilename / engineLibTmpFilename constants
// - The program entry point (main / android_main), which calls Main below

static bool IsAbsolutePath(const char *path);
static void InitializeDirectories(Platform &platform);
static bool InitializeWindow(Window &window, u32 width = 1280, u32 height = 720, const char *title = "ILU Engine");
static void CleanupWindow(Window &window);
static void ShowPlatformWindow(Window &window);
static void PlatformUpdateEventLoop(Platform &platform);
static bool InitializeGamepad(Platform &platform);
static void UpdateGamepad(Platform &platform);
static bool InitializeAudioDevice(Platform &platform);
static void UpdateAudioDevice(Platform &platform);
static void WaitForAudioDevice(Platform &platform);
static void PlatformQuit();
static void PlatformWakeMainThread();

// Common entry point called by the platform entry points
static void Main(int argc, char **argv);


////////////////////////////////////////////////////////////////////////////////////////////////////
// Directories

static const char *BinDir = "";
static const char *DataDir = "";
static const char *AssetDir = "";
static const char *ProjectDir = "";

static void CanonicalizePath(char *path)
{
	struct PathPart
	{
		char *str;
		int len;
	};

	PathPart parts[32] = {};
	u32 partCount = 0;

	char *ptr = path;

	PathPart *currentPart = &parts[partCount++];
	currentPart->str = ptr;

	const bool addRootSeparator = (*ptr == '/');

	// Split path in parts
	// NOTE: This replaces separators by '\0' for easier string comparisons later
	while (*ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			if (currentPart->len > 0) {
				ASSERT(partCount < ARRAY_COUNT(parts));
				currentPart = &parts[partCount++];
			}
			currentPart->str = ptr + 1;
		} else {
			currentPart->len++;
		}
		ptr++;
	}

	//LOG(Debug, "Directory parts:\n");
	//for (u32 i = 0; i < partCount; ++i) {
	//	LOG(Debug, "- %.*s\n", parts[i].len,  parts[i].str);
	//}

	// Canonicalize
	u32 finalParts[32] = {};
	u32 finalPartCount = 0;
	for (u32 partIndex = 0; partIndex < partCount; ++partIndex)
	{
		const PathPart &part = parts[partIndex];
		if ( StrEq(part.str, ".") ) {
			// Do nothing
		} else if ( StrEq(part.str, "..") ) {
			// Remove previous part
			ASSERT(finalPartCount > 0);
			finalPartCount--;
		} else if ( part.len > 0 ) {
			finalParts[finalPartCount++] = partIndex;
		}
	}

	// Copy string back to buffer
	ptr = path;
	for (u32 i = 0; i < finalPartCount; ++i)
	{
		const u32 partIndex = finalParts[i];
		const PathPart &part = parts[partIndex];
		if ( i > 0 || addRootSeparator ) {
			*ptr++ = '/';
		}
		for (u32 c = 0; c < part.len; ++c) {
			*ptr++ = part.str[c];
		}
	}
	*ptr = 0;
}

static void PublishDirectories(Platform &platform)
{
	platform.pub.BinDir    = BinDir;
	platform.pub.DataDir   = DataDir;
	platform.pub.AssetDir  = AssetDir;
	platform.pub.ProjectDir = ProjectDir;

	LOG(Info, "Directories:\n");
	LOG(Info, "- BinDir: %s\n", BinDir);
	LOG(Info, "- DataDir: %s\n", DataDir);
	LOG(Info, "- AssetDir: %s\n", AssetDir);
	LOG(Info, "- ProjectDir: %s\n", ProjectDir);
}

static void InitializeDirectoriesFromWorkingDir(Platform &platform, char *workingDir)
{
	StrReplace(workingDir, '\\', '/'); // Make all separators '/'

	char exeDir[MAX_PATH_LENGTH] = {};
	if (platform.pub.argc > 0)
	{
		StrReplace(platform.pub.argv[0], '\\', '/'); // Make all separators '/'
		const char *exePath = platform.pub.argv[0];
		const char *lastSeparator = StrCharR(exePath, '/');
		const u32 length = lastSeparator ? lastSeparator - exePath : 0;
		StrCopyN(exeDir, exePath, length);
	}

	char directory[MAX_PATH_LENGTH] = {};
	if ( !IsAbsolutePath(exeDir) )
	{
		StrCopy(directory, workingDir);
		StrCat(directory, "/");
	}
	StrCat(directory, exeDir);
	CanonicalizePath(directory);

	DataDir = PushString(platform.stringArena, directory);
	BinDir = DataDir;

	StrCat(directory, "/..");
	CanonicalizePath(directory);
	ProjectDir = PushString(platform.stringArena, directory);

	StrCopy(directory, ProjectDir);
	StrCat(directory, "/assets");
	AssetDir = PushString(platform.stringArena, directory);

	PublishDirectories(platform);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Generic platform events

static void SendPlatformEvent(Platform &platform, PlatformEvent event)
{
	if (platform.eventHead - platform.eventTail < ARRAY_COUNT(platform.events))
	{
		const i32 eventIndex = platform.eventHead % ARRAY_COUNT(platform.events);
		platform.events[eventIndex] = event;

		AtomicIncrement(&platform.eventHead);
	}
	else
	{
		LOG(Warning, "Event %d was lost\n", event.type);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Input state helpers

static void TransitionInputStatesSinceLastFrame(Window &window)
{
	// Transition key states
	for ( u32 i = 0; i < K_COUNT; ++i ) {
		if ( window.keyboard.keys[i] == KEY_STATE_PRESS ) {
			window.keyboard.keys[i] = KEY_STATE_PRESSED;
		} else if ( window.keyboard.keys[i] == KEY_STATE_RELEASE ) {
			window.keyboard.keys[i] = KEY_STATE_IDLE;
		}
	}

	// Transition mouse button states
	for ( u32 i = 0; i < MOUSE_BUTTON_COUNT; ++i ) {
		if ( window.mouse.buttons[i] == BUTTON_STATE_PRESS ) {
			window.mouse.buttons[i] = BUTTON_STATE_PRESSED;
		} else if ( window.mouse.buttons[i] == BUTTON_STATE_RELEASE ) {
			window.mouse.buttons[i] = BUTTON_STATE_IDLE;
		}
	}

	window.mouse.delta = {0, 0};
	window.mouse.wheel = {0, 0};

	window.chars.charCount = 0;

	// Transition touch states
	for ( u32 i = 0; i < ARRAY_COUNT(window.touches); ++i ) {
		if ( window.touches[i].state == TOUCH_STATE_PRESS ) {
			window.touches[i].state = TOUCH_STATE_PRESSED;
		} else if ( window.touches[i].state == TOUCH_STATE_RELEASE ) {
			window.touches[i].state = TOUCH_STATE_IDLE;
		}
		window.touches[i].dx = 0.0f;
		window.touches[i].dy = 0.0f;
	}
}

static void UpdateKeyModifiers(Window &window)
{
	// Update key modifiers
	window.chars.shift = KeyPressed(window.keyboard, K_SHIFT);
	window.chars.ctrl = KeyPressed(window.keyboard, K_CONTROL);
	window.chars.alt = KeyPressed(window.keyboard, K_ALT);

	for ( u32 i = 0; i < K_COUNT; ++i )
	{
		if ( KeyPress(window.keyboard, (Key)i) )
		{
			char character = 0;

			if ( i >= K_A && i <= K_Z ) {
				character = window.chars.shift ? 'A' + (i - K_A) : 'a' + (i - K_A);
			} else if ( i >= K_0 && i <= K_9 ) {
				character = '0' + (i - K_0);
			} else if ( i == K_SPACE ) {
				character = ' ';
			} else if ( i == K_PERIOD ) {
				character = '.';
			}

			if (character)
			{
				window.chars.chars[window.chars.charCount++] = character;
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform-specific implementation

#if PLATFORM_WINDOWS
#include "platform_win32.cpp"
#elif PLATFORM_LINUX
#include "platform_linux.cpp"
#elif PLATFORM_ANDROID
#include "platform_android.cpp"
#else
#error "Unsupported platform"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// Audio

static void UpdateAudio(Platform &platform)
{
	UpdateAudioDevice(platform);

	if ( platform.PreRenderAudioCallback )
	{
		platform.PreRenderAudioCallback(platform.pub);
	}
}

static bool InitializeAudio(Platform &platform)
{
	LOG(Info, "Sound system initialization:\n");

	if ( platform.RenderAudioCallback == nullptr )
	{
		LOG(Info, "- RenderAudioCallback not provided, sound system not required\n");
		return true;
	}

	if ( platform.PreRenderAudioCallback == nullptr )
	{
		LOG(Info, "- PreRenderAudioCallback not provided (you might want it to pre-render costly stuff)\n");
	}

	AudioDevice &audio = platform.audio;

	// Audio configuration
	audio.channelCount = 2;
	audio.bytesPerSample = 2; // 4 in HH
	audio.samplesPerSecond = 48000; // per channel

	// One second of audio. The device plays from this ring buffer while we write ahead of it.
	audio.bufferSize = audio.channelCount * audio.samplesPerSecond * audio.bytesPerSample;

	// Tuning knobs for latency vs. underrun resistance: widening these makes dropouts less
	// likely but delays how soon a newly triggered sound is heard.
	audio.writeAheadMillis = 33;
	audio.safetyMillis = 11;

	const u32 bytesPerSecond = audio.samplesPerSecond * audio.channelCount * audio.bytesPerSample;
	audio.safetyBytes = (bytesPerSecond * audio.safetyMillis) / 1000;

	// Allocate buffer to output samples from the engine
	audio.outputSamples = (i16*)AllocateVirtualMemory(audio.bufferSize);

	InitializeAudioDevice(platform);

	return audio.initialized;
}

#if USE_AUDIO_THREAD
static THREAD_FUNCTION(AudioThread) // void *WorkQueueThread(void* arguments)
{
	const ThreadInfo *threadInfo = (const ThreadInfo *)arguments;

	while ( platform.keepRunning )
	{
		if ( platform.audio.isPlaying )
		{
			UpdateAudio(platform);

			// Let the device pace this loop where it can, instead of a fixed sleep.
			WaitForAudioDevice(platform);
		}
		else
		{
			SleepMillis(10);
		}

		if ( platform.paused )
		{
			platform.audioPaused = true;
			WaitSemaphore( platform.audioThreadPauseSemaphore );
			platform.audioPaused = false;
		}
	}

	SignalSemaphore(platform.audioThreadFinishSemaphore);

	return 0;
}

static bool InitializeAudioThread(AudioDevice &audio)
{
	if ( audio.initialized )
	{
		if ( !CreateSemaphore( platform.audioThreadFinishSemaphore, 0, 1 ) )
		{
			return false;
		}

		if ( !CreateSemaphore( platform.audioThreadPauseSemaphore, 0, 1 ) )
		{
			return false;
		}

		static const ThreadInfo threadInfo = {
			.globalIndex = THREAD_ID_AUDIO,
		};
		if ( !CreateDetachedThread(AudioThread, threadInfo) )
		{
			audio.initialized = false;
		}
	}

	return audio.initialized;
}
#endif // USE_AUDIO_THREAD



////////////////////////////////////////////////////////////////////////////////////////////////////
// Work queue abstraction

struct WorkQueueEntry
{
	WorkQueueCallback *callback;
	void *data;
};

struct WorkQueue
{
	Semaphore workSemaphore;
	Semaphore pauseSemaphore;
	Semaphore finishSemaphore;

	volatile_u32 workerPaused[WORK_QUEUE_WORKER_COUNT];

	volatile_i64 head;
	volatile_i64 tail;

	WorkQueueEntry entries[128] = {};
};

static WorkQueue workQueue;

static void WorkQueuePush(WorkQueueEntry entry)
{
	ASSERT(workQueue.head - workQueue.tail < ARRAY_COUNT(workQueue.entries));
	ASSERT(entry.callback);
	ASSERT(entry.data);

	const u32 index = workQueue.head % ARRAY_COUNT(workQueue.entries);
	workQueue.entries[index] = entry;

	FullWriteBarrier();

	AtomicIncrement(&workQueue.head);

	SignalSemaphore(workQueue.workSemaphore);
}

static bool WorkQueueEmpty()
{
	const bool empty = workQueue.head == workQueue.tail;
	return empty;
}

static bool WorkQueueProcess(const ThreadInfo &threadInfo)
{
	bool thereIsPendingWork = false;

	const u64 tail = workQueue.tail;

	if (tail < workQueue.head)
	{
		if ( AtomicSwap(&workQueue.tail, tail, tail+1) )
		{
			FullReadBarrier();

			const u32 workIndex = tail % ARRAY_COUNT(workQueue.entries);
			WorkQueueEntry &entry = workQueue.entries[workIndex];
			entry.callback(threadInfo, entry.data);
		}

		thereIsPendingWork = true;
	}

	return thereIsPendingWork;
}

static THREAD_FUNCTION(WorkQueueThread) // void *WorkQueueThread(void* arguments)
{
	const ThreadInfo *threadInfo = (const ThreadInfo *)arguments;
	const u32 workerIndex = threadInfo->globalIndex - THREAD_ID_WORKER_0;

	while ( platform.keepRunning )
	{
		if ( platform.paused )
		{
			workQueue.workerPaused[workerIndex] = 1;
			WaitSemaphore(workQueue.pauseSemaphore);
			workQueue.workerPaused[workerIndex] = 0;
			continue;
		}

		if ( !WorkQueueProcess(*threadInfo) )
		{
			WaitSemaphore(workQueue.workSemaphore);
		}
	}

	SignalSemaphore(workQueue.finishSemaphore);

	return 0;
}

#if 0 // Threading test code
static void PrintString(const ThreadInfo &threadInfo, void *data)
{
	const char *work = (const char *)data;
	LOG(Debug, "Thread %u work: %s\n", threadInfo.globalIndex, work);
}

static void WorkQueuePushString(const char *str)
{
	WorkQueueEntry entry = {
		.callback = PrintString,
		.data = (void*)str
	};
	WorkQueuePush(entry);
}
#endif

static bool InitializeWorkQueue(Platform &platform)
{
	static ThreadInfo threadInfos[WORK_QUEUE_WORKER_COUNT];
	constexpr u32 threadCount = ARRAY_COUNT(threadInfos);

	// The work semaphore is signalled once per queued job, and once more per worker when
	// pausing them, so its maximum count has to cover a full queue plus one wake-up each.
	const u32 iniCount = 0;
	const u32 maxCount = (u32)ARRAY_COUNT(workQueue.entries) + threadCount;
	if ( !CreateSemaphore( workQueue.workSemaphore, iniCount, maxCount ) )
	{
		return false;
	}

	if ( !CreateSemaphore( workQueue.pauseSemaphore, iniCount, threadCount ) )
	{
		return false;
	}

	if ( !CreateSemaphore( workQueue.finishSemaphore, iniCount, threadCount ) )
	{
		return false;
	}

	workQueue.head = 0;
	workQueue.tail = 0;

	for (u32 i = 0; i < threadCount; ++i)
	{
		ThreadInfo &threadInfo = threadInfos[i];
		threadInfo.globalIndex = THREAD_ID_WORKER_0 + i;
		if ( !CreateDetachedThread(WorkQueueThread, threadInfo) )
		{
			return false;
		}
	}

	return true;

#if 0 // Threading test code
	WorkQueuePushString("A0");
	WorkQueuePushString("A1");
	WorkQueuePushString("A2");
	WorkQueuePushString("A3");
	WorkQueuePushString("A4");
	WorkQueuePushString("A5");
	WorkQueuePushString("A6");
	WorkQueuePushString("A7");
	WorkQueuePushString("A8");
	WorkQueuePushString("A9");
	WorkQueuePushString("B0");
	WorkQueuePushString("B1");
	WorkQueuePushString("B2");
	WorkQueuePushString("B3");
	WorkQueuePushString("B4");
	WorkQueuePushString("B5");
	WorkQueuePushString("B6");
	WorkQueuePushString("B7");
	WorkQueuePushString("B8");
	WorkQueuePushString("B9");
	WorkQueuePushString("C0");
	WorkQueuePushString("C1");
	WorkQueuePushString("C2");
	WorkQueuePushString("C3");
	WorkQueuePushString("C4");
	WorkQueuePushString("C5");
	WorkQueuePushString("C6");
	WorkQueuePushString("C7");
	WorkQueuePushString("C8");
	WorkQueuePushString("C9");

	ThreadInfo threadInfo = { 16 };
	while (!WorkQueueEmpty()) {
		WorkQueueProcess(threadInfo);
	}
#endif

	return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Update thread

static void ProcessPlatformEvents(Platform &platform)
{
	Window &window = platform.window;

	TransitionInputStatesSinceLastFrame(window);

	while (platform.eventTail < platform.eventHead)
	{
		const i64 uncappedIndex = AtomicPreIncrement(&platform.eventTail);
		const i32 eventIndex = uncappedIndex % ARRAY_COUNT(platform.events);
		const PlatformEvent &event = platform.events[eventIndex];

		switch (event.type)
		{
			case PlatformEventTypeWindowWasCreated:
			{
				platform.WindowInitCallback(platform.pub);
				platform.windowInitialized = true;
				ShowPlatformWindow(platform.window);
				break;
			};
			case PlatformEventTypeWindowWillDestroy:
			{
				platform.windowInitialized = false;
				platform.WindowCleanupCallback(platform.pub);
				CleanupWindow(platform.window);
				break;
			};
			case PlatformEventTypeWindowResize:
			{
				const u16 width = event.windowResize.width;
				const u16 height = event.windowResize.height;
				if ( window.width != width || window.height != height )
				{
					window.width = Max(width, 0);
					window.height = Max(height, 0);
					InvalidateSwapchain();
				}
				break;
			};
			case PlatformEventTypeKeyPress:
			{
				window.keyboard.keys[event.keyPress.code] = event.keyPress.state;
				break;
			};
			case PlatformEventTypeMouseClick:
			{
				window.mouse.buttons[event.mouseClick.button] = event.mouseClick.state;
				if (event.mouseClick.button == MOUSE_BUTTON_LEFT)
				{
					window.mouse.click = window.mouse.pos;
				}
				break;
			};
			case PlatformEventTypeMouseMove:
			{
				window.mouse.delta += event.mouseMove.pos - window.mouse.pos;
				window.mouse.pos = event.mouseMove.pos;
				break;
			};
			case PlatformEventTypeMouseWheel:
			{
				window.mouse.wheel += event.mouseWheel.delta;
				break;
			};
		}
	}

	UpdateKeyModifiers(window);
}

static void CheckEngineHotReload(Platform &platform);

// Runs a whole frame iteration: event processing, update and render. It is
// called by the update thread normally, and by the main thread while it is
// blocked in a modal size/move loop. The render lock makes both exclusive,
// including their access to the event queue, window state and frame arena.
static void UpdateAndRender(Platform &platform)
{
#if USE_UPDATE_THREAD
	MutexScope renderScope(platform.renderLock);
#endif

	ResetArena(platform.frameArena);

	ProcessPlatformEvents(platform);

	UpdateGamepad(platform);

	if ( platform.windowInitialized )
	{
		platform.UpdateCallback(platform.pub);
		platform.RenderGraphicsCallback(platform.pub);

		platform.window.flags = 0;
	}

	CheckEngineHotReload(platform);
}

#if USE_UPDATE_THREAD

static THREAD_FUNCTION(UpdateThread) // void *WorkQueueThread(void* arguments)
{
	const ThreadInfo *threadInfo = (const ThreadInfo *)arguments;

	Clock lastClock = GetClock();

	while ( platform.keepRunning )
	{
		if ( platform.inSizeMove )
		{
			// The main thread drives update and render during modal size/move loops
			Yield();
		}
		else
		{
			UpdateAndRender(platform);
		}
	}

	SignalSemaphore(platform.updateThreadFinishSemaphore);

	return 0;
}

static bool InitializeUpdateThread(Platform &platform)
{
	bool ok = true;

	if ( !CreateSemaphore( platform.updateThreadFinishSemaphore, 0, 1 ) )
	{
		return false;
	}

	static const ThreadInfo threadInfo = {
		.globalIndex = THREAD_ID_UPDATE,
	};
	if ( !CreateDetachedThread(UpdateThread, threadInfo) )
	{
		ok = false;
	}

	return ok;
}

#endif // USE_UPDATE_THREAD



////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform

static bool InitializeArenas(Platform &platform)
{
	platform.stringMemorySize = KB(16);
	platform.dataMemorySize = MB(64);
	platform.globalMemorySize = MB(64);
	platform.frameMemorySize = MB(16);

	byte *globalMemory = (byte*)AllocateVirtualMemory(platform.globalMemorySize);
	platform.globalArena = MakeArena(globalMemory, platform.globalMemorySize, "Global arena");

	byte *frameMemory = (byte*)AllocateVirtualMemory(platform.frameMemorySize);
	platform.frameArena = MakeArena(frameMemory, platform.frameMemorySize, "Frame arena");

	byte *stringMemory = (byte*)AllocateVirtualMemory(platform.stringMemorySize);
	platform.stringArena = MakeArena(stringMemory, platform.stringMemorySize, "String arena");
	platform.stringInterning = StringInterningCreate(&platform.stringArena);

	byte *dataMemory = (byte*)AllocateVirtualMemory(platform.dataMemorySize);
	platform.dataArena = MakeArena(dataMemory, platform.dataMemorySize, "Data arena");

	platform.pub.stringInterning = &platform.stringInterning;
	platform.pub.globalArena = &platform.globalArena;
	platform.pub.stringArena = &platform.stringArena;
	platform.pub.frameArena = &platform.frameArena;
	platform.pub.dataArena = &platform.dataArena;

	return true;
}

static void PlatformQuit()
{
	platform.keepRunning = false;
	PlatformWakeMainThread();
}

static void ReleaseScratchArena(u32 index)
{
	u32 oldValue, newValue;
	do
	{
		oldValue = platform.scratchArenaLockMask;
		newValue = oldValue & ~(1<<index);
	}
	while (!AtomicSwap(&platform.scratchArenaLockMask, oldValue, newValue));
}

static u32 AcquireScratchArena(Arena &outArena, u32 minSize)
{
	u32 rejectedMask = 0; // Slots already found too small in this call; skip them on retry.
	for (u32 i = 0; i < MAX_SCRATCH_ARENAS; ++i)
	{
		const u32 oldValue = platform.scratchArenaLockMask;
		const u32 index = FBZ(oldValue | rejectedMask);
		ASSERT(index < MAX_SCRATCH_ARENAS);

		const u32 newValue = oldValue | (1<<index);
		if (AtomicSwap(&platform.scratchArenaLockMask, oldValue, newValue))
		{
			Arena &arena = platform.scratchArenas[index];

			if (arena.base && arena.size < minSize)
			{
				// Too small; release it and keep looking rather than growing (and leaking) it.
				ReleaseScratchArena(index);
				rejectedMask |= (1<<index);
				continue;
			}

			if (!arena.base)
			{
				arena.base = (byte*)AllocateVirtualMemory(minSize);
				arena.size = minSize;
			}
			outArena = arena;
			outArena.used = 0;
			return index;
		}
	}
	INVALID_CODE_PATH();
	return U32_MAX;
}

static FilePath sEngineLibPath = {};
static FilePath sEngineTmpLibPath = {};

static bool LoadEngineDLL(Platform &platform)
{
	LOG(Info, "Loading engine DLL:\n");

	sEngineLibPath = MakePath(BinDir, engineLibFilename);
	sEngineTmpLibPath = MakePath(BinDir, engineLibTmpFilename);

	if (!CopyFile(sEngineLibPath.str, sEngineTmpLibPath.str)) {
		LOG(Warning, "- Couldn't copy from %s to %s\n", engineLibFilename, engineLibTmpFilename);
	}

	platform.engineLib = OpenLibrary(sEngineTmpLibPath.str);
	if ( !platform.engineLib ) {
		LOG(Error, "- Couldn't load %s\n", engineLibTmpFilename);
		return false;
	}

	LOG(Info, "- Loaded %s successfully\n", engineLibTmpFilename);

	// Engine interface exposed to platform

	platform.LoadEngineCallback = (void (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformLoadEngine");
	if( !platform.LoadEngineCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformLoadEngine\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformLoadEngine\n");

	platform.UnloadEngineCallback = (void (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformUnloadEngine");
	if( !platform.UnloadEngineCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformUnloadEngine\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformUnloadEngine\n");

	platform.PreInitCallback = (bool (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformPreInit");
	if( !platform.PreInitCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformPreInit\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformPreInit\n");

	platform.InitCallback = (bool (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformInit");
	if( !platform.InitCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformInit\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformInit\n");

	platform.UpdateCallback = (void (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformUpdate");
	if( !platform.UpdateCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformUpdate\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformUpdate\n");

	platform.RenderGraphicsCallback = (void (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformRenderGraphics");
	if( !platform.RenderGraphicsCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformRenderGraphics\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformRenderGraphics\n");

	platform.CleanupCallback = (void (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformCleanup");
	if( !platform.CleanupCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformCleanup\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformCleanup\n");

	platform.WindowInitCallback = (bool (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformWindowInit");
	if( !platform.WindowInitCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformWindowInit\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformWindowInit\n");

	platform.WindowCleanupCallback = (void (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformWindowCleanup");
	if( !platform.WindowCleanupCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformWindowCleanup\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformWindowCleanup\n");

	platform.GetStateSignatureCallback = (u32 (*)()) LoadSymbol(platform.engineLib, "OnPlatformGetStateSignature");
	if( !platform.GetStateSignatureCallback ) {
		LOG(Error, "- Couldn't load symbol: OnPlatformGetStateSignature\n");
		return false;
	}
	LOG(Info, "- Symbol loaded: OnPlatformGetStateSignature\n");

	const u32 stateSignature = platform.GetStateSignatureCallback();
	if ( !platform.engineStateSignatureInitialized )
	{
		platform.engineStateSignature = stateSignature;
		platform.engineStateSignatureInitialized = true;
	}
	else if ( stateSignature != platform.engineStateSignature )
	{
		LOG(Error, "- Engine state layout changed (0x%x -> 0x%x)\n", platform.engineStateSignature, stateSignature);
		LOG(Error, "- The retained engine state cannot be reused by this module. Restart the engine.\n");
		return false;
	}

	platform.RenderAudioCallback = (void (*)(Plat &, SoundBuffer &)) LoadSymbol(platform.engineLib, "OnPlatformRenderAudio");
	if( !platform.RenderAudioCallback ) { LOG(Error, "- Couldn't load symbol: OnPlatformRenderAudio\n"); }
	else { LOG( Info, "- Symbol loaded: OnPlatformRenderAudio\n" ); }

	platform.PreRenderAudioCallback = (void (*)(Plat &)) LoadSymbol(platform.engineLib, "OnPlatformPreRenderAudio");
	if( !platform.PreRenderAudioCallback ) { LOG(Error, "- Couldn't load symbol: OnPlatformPreRenderAudio\n"); }
	else { LOG( Info, "- Symbol loaded: OnPlatformPreRenderAudio\n" ); }

	// Platform interface exposed to engine
	platform.pub.api.PlatformQuit        = PlatformQuit;
	platform.pub.api.AcquireScratchArena = AcquireScratchArena;
	platform.pub.api.ReleaseScratchArena = ReleaseScratchArena;

	platform.LoadEngineCallback(platform.pub);

	return true;
}

static void UnloadEngineDLL(Platform &platform)
{
	if (platform.engineLib)
	{
		platform.UnloadEngineCallback(platform.pub);
		CloseLibrary(platform.engineLib);
		platform.engineLib = 0;
	}
}

static void PauseThreads(Platform &platform)
{
	platform.paused = true;

#if USE_AUDIO_THREAD
	while ( !platform.audioPaused && platform.keepRunning )
	{
		Yield();
	}
#endif

	// Wake up threads blocked waiting for work...
	for (u32 i = 0; i < WORK_QUEUE_WORKER_COUNT; ++i)
	{
		SignalSemaphore(workQueue.workSemaphore);
	}

	// And wait until all of them are paused
	for (u32 i = 0; i < WORK_QUEUE_WORKER_COUNT; ++i)
	{
		while ( !workQueue.workerPaused[i] && platform.keepRunning )
		{
			Yield();
		}
	}
}

static void ResumeThreads(Platform &platform)
{
	platform.paused = false;

	for (u32 i = 0; i < WORK_QUEUE_WORKER_COUNT; ++i)
	{
		SignalSemaphore(workQueue.pauseSemaphore);
	}

#if USE_AUDIO_THREAD
	SignalSemaphore(platform.audioThreadPauseSemaphore);
#endif
}

static void CheckEngineHotReload(Platform &platform)
{
	static Clock lastClock = GetClock();
	const Clock currentClock = GetClock();
	const f32 secondsSinceLastCheck = GetSecondsElapsed(lastClock, currentClock);

	if ( secondsSinceLastCheck > 0.1f )
	{
		lastClock = currentClock;

		u64 timestampBase = 0;
		u64 timestampCopy = 0;
		const bool success1 = GetFileLastWriteTimestamp(sEngineLibPath.str, timestampBase);
		const bool success2 = GetFileLastWriteTimestamp(sEngineTmpLibPath.str, timestampCopy);
		const bool someError = !success1 || !success2;

		// In case of error recovering the timestamp, we reload
		if (someError || timestampBase > timestampCopy)
		{
			LOG(Info, "Engine DLL was updated. Reloading...\n");
			PauseThreads(platform);
			UnloadEngineDLL(platform);
			const bool reloaded = LoadEngineDLL(platform);
			ResumeThreads(platform);

			if ( !reloaded )
			{
				// The previous module is already gone and the new one is unusable
				LOG(Error, "Engine hot reload failed. Quitting.\n");
				PlatformQuit();
			}
		}
	}
}

static bool Run(Platform &platform)
{
	platform.paused = false;
	platform.keepRunning = true;

	if ( !CreateMutex( platform.renderLock ) )
	{
		return false;
	}

	if ( !platform.PreInitCallback(platform.pub) )
	{
		return false;
	}

	if ( !InitializeWindow(platform.window) )
	{
		return false;
	}

	platform.pub.window = &platform.window;

#if PLATFORM_LINUX
	const PlatformEvent event = { .type = PlatformEventTypeWindowWasCreated };
	SendPlatformEvent(platform, event);
#endif

	if ( !InitializeGamepad(platform) )
	{
		// Do nothing
	}

	if ( !InitializeAudio(platform) )
	{
		LOG(Error, "InitializeAudio failed!\n");
		//return false; // Allow running without audio
	}

	if ( !InitializeWorkQueue(platform) )
	{
		return false;
	}

	if ( !platform.InitCallback(platform.pub) )
	{
		return false;
	}

#if USE_AUDIO_THREAD
	if ( !InitializeAudioThread(platform.audio) )
	{
		LOG(Error, "InitializeAudioThread failed!\n");
		//return false; // Allow running without audio
	}
#endif // USE_AUDIO_THREAD

#if USE_UPDATE_THREAD
	if ( !InitializeUpdateThread(platform) )
	{
		return false;
	}
#endif // USE_UPDATE_THREAD

	Clock lastFrameClock = GetClock();

	const Clock firstFrameClock = GetClock();

	while ( platform.keepRunning )
	{
		const Clock currentFrameBeginClock = GetClock();
		platform.deltaSeconds = GetSecondsElapsed(lastFrameClock, currentFrameBeginClock);
		platform.totalSeconds = GetSecondsElapsed(firstFrameClock, currentFrameBeginClock);
		lastFrameClock = currentFrameBeginClock;

		PlatformUpdateEventLoop(platform);

#if !USE_UPDATE_THREAD
		UpdateAndRender(platform);
#endif // !USE_UPDATE_THREAD

#if !USE_AUDIO_THREAD
		if ( platform.audio.isPlaying )
		{
			UpdateAudio(platform);
		}
#endif // USE_AUDIO_THREAD
	}

	for (u32 i = 0; i < WORK_QUEUE_WORKER_COUNT; ++i) {
		SignalSemaphore(workQueue.workSemaphore);
		SignalSemaphore(workQueue.pauseSemaphore);
	}
	for (u32 i = 0; i < WORK_QUEUE_WORKER_COUNT; ++i) {
		WaitSemaphore(workQueue.finishSemaphore);
	}

#if USE_UPDATE_THREAD
	WaitSemaphore(platform.updateThreadFinishSemaphore);
#endif
#if USE_AUDIO_THREAD
	WaitSemaphore(platform.audioThreadFinishSemaphore);
#endif

	if ( platform.windowInitialized )
	{
		platform.WindowCleanupCallback(platform.pub);
	}

	platform.CleanupCallback(platform.pub);
	// TODO: Cleanup window and audio

	return false;
}


static void Main( int argc, char **argv )
{
	// Input args
	platform.pub.argc = argc;
	platform.pub.argv = argv;

	InitializeArenas(platform);

	InitializeDirectories(platform);

	GetGraphicsAPI(&platform.pub.graphicsAPI);

	LoadEngineDLL(platform);

	Run(platform);

	UnloadEngineDLL(platform);
}


#define TOOLS_GFX_IMPLEMENTATION
#include "ilu_gfx.h"

