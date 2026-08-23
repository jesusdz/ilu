
#define USE_UPDATE_THREAD 0
#define USE_AUDIO_THREAD 0 // Do not set to 1, Android invokes AAudioFillAudioBuffer from its own audio thread


////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform implementation types and state

struct WindowImpl
{
	ANativeWindow *nativeWindow;
};

static WindowImpl windowImpl;

static struct android_app *androidApp;

static AAudioStream *audioStream;

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
	// TODO: Don't hardcode this path here and get it from Android API.
	DataDir = "/sdcard/Android/data/com.tools.game/files";
	BinDir = DataDir;
	ProjectDir = "";

	PublishDirectories(platform);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Window and events

/**
 * Process the next main command.
 * enum NativeAppGlueAppCmd {
 *   UNUSED_APP_CMD_INPUT_CHANGED = 0
 *   APP_CMD_INIT_WINDOW = 1
 *   APP_CMD_TERM_WINDOW = 2
 *   APP_CMD_WINDOW_RESIZED = 3
 *   APP_CMD_WINDOW_REDRAW_NEEDED = 4
 *   APP_CMD_CONTENT_RECT_CHANGED = 5
 *   APP_CMD_GAINED_FOCUS = 6
 *   APP_CMD_LOST_FOCUS = 7
 *   APP_CMD_CONFIG_CHANGED = 8
 *   APP_CMD_LOW_MEMORY = 9
 *   APP_CMD_START = 10
 *   APP_CMD_RESUME = 11
 *   APP_CMD_SAVE_STATE = 12
 *   APP_CMD_PAUSE = 13
 *   APP_CMD_STOP = 14
 *   APP_CMD_DESTROY = 15
 *   APP_CMD_WINDOW_INSETS_CHANGED = 16
 * }
 */
static void AndroidHandleAppCommand(struct android_app *app, int32_t cmd)
{
	Platform *platform = (Platform*)app->userData;

	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
			// The window is being shown, get it ready.
			ASSERT(app->window != NULL);
			if (app->window && app->window != windowImpl.nativeWindow)
			{
				windowImpl.nativeWindow = app->window;
				const PlatformEvent event = { .type = PlatformEventTypeWindowWasCreated };
				SendPlatformEvent(*platform, event);
			}
			break;
		case APP_CMD_TERM_WINDOW:
			// The window is being hidden or closed, clean it up.
			platform->window = {};
			platform->window.impl = &windowImpl;
			windowImpl.nativeWindow = NULL;
			{
				const PlatformEvent event = { .type = PlatformEventTypeWindowWillDestroy };
				SendPlatformEvent(*platform, event);
			}
			break;
		case APP_CMD_WINDOW_RESIZED:
			{
				const PlatformEvent event = {
					.type = PlatformEventTypeWindowResize,
					.windowResize = {
						.width = (u16)ANativeWindow_getWidth(app->window),
						.height = (u16)ANativeWindow_getHeight(app->window),
					},
				};
				SendPlatformEvent(*platform, event);
			}
			break;
		//case APP_CMD_WINDOW_REDRAW_NEEDED: break;
		//case APP_CMD_CONTENT_RECT_CHANGED: break;
		//case APP_CMD_GAINED_FOCUS: break;
		//case APP_CMD_LOST_FOCUS: break;
		//case APP_CMD_CONFIG_CHANGED: break;
		//case APP_CMD_LOW_MEMORY: break;
		//case APP_CMD_START: break;
		//case APP_CMD_RESUME: break;
		//case APP_CMD_SAVE_STATE: break;
		case APP_CMD_PAUSE:
			{
				// Gets activated at APP_CMD_INIT_WINDOW
			}
			break;
		//case APP_CMD_STOP: break;
		//case APP_CMD_DESTROY: break;
		//case APP_CMD_WINDOW_INSETS_CHANGED: break;
		default:
			//LOG( Info, "UNKNOWN ANDROID COMMAND: %d\n", cmd);
			break;
	}
	//LOG( Info, "ANDROID APP COMMAND: %d\n", cmd);
}

static int32_t AndroidHandleInputEvent(struct android_app *app, AInputEvent *event)
{
	Platform *platform = (Platform*)app->userData;

	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	{
		const int32_t actionAndPointer = AMotionEvent_getAction( event );
		const uint32_t action = actionAndPointer & AMOTION_EVENT_ACTION_MASK;
		const uint32_t pointerIndex = (actionAndPointer & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		const uint32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
		const uint32_t pointerCount = AMotionEvent_getPointerCount(event);
		const float x = AMotionEvent_getX(event, pointerIndex);
		const float y = AMotionEvent_getY(event, pointerIndex);

		if (pointerId < ARRAY_COUNT(platform->window.touches))
		{
			Touch *touches = platform->window.touches;

			switch( action )
			{
				case AMOTION_EVENT_ACTION_DOWN:
				case AMOTION_EVENT_ACTION_POINTER_DOWN:
					{
						touches[pointerId].state = TOUCH_STATE_PRESS;
						touches[pointerId].x0 = x;
						touches[pointerId].y0 = y;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
				case AMOTION_EVENT_ACTION_UP:
				case AMOTION_EVENT_ACTION_POINTER_UP:
					{
						touches[pointerId].state = TOUCH_STATE_RELEASE;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
				case AMOTION_EVENT_ACTION_MOVE:
					// On move ements, we are meant to handle all pointers in the gesture
					for (u32 pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex)
					{
						const float x = AMotionEvent_getX(event, pointerIndex);
						const float y = AMotionEvent_getY(event, pointerIndex);
						const uint32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
						touches[pointerId].dx = x - touches[pointerId].x;
						touches[pointerId].dy = y - touches[pointerId].y;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
			}
		}
		return 1;
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

	// The native window is provided later through the APP_CMD_INIT_WINDOW app command.

	return true;
}

static void CleanupWindow(Window &window)
{
}

static void ShowPlatformWindow(Window &window)
{
}

static void PlatformWakeMainThread()
{
	// No-op: the event loop polls without blocking (ALooper_pollAll with a zero
	// timeout), so the main loop re-checks keepRunning every iteration on its own.
}

static void PlatformUpdateEventLoop(Platform &platform)
{
	// Read all pending events.
	int ident;
	int events;
	struct android_poll_source* source;

	const int kWaitForever = -1;
	const int kDontWait = 0;
	while ((ident=ALooper_pollAll(kDontWait, NULL, &events, (void**)&source)) >= 0)
	{
		// Process this event.
		if (source != NULL)
		{
			source->process(androidApp, source);
		}

		// Check if we are exiting.
		if (androidApp->destroyRequested != 0)
		{
			LOG(Info, "androidApp->destroyRequesteds\n");
			PlatformQuit();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Gamepad

static bool InitializeGamepad(Platform &platform)
{
	LOG(Info, "Input system initialization:\n");

	platform.pub.gamepad = &platform.gamepad;

	LOG(Info, "- Missing implementation\n");

	return false;
}

static void UpdateGamepad(Platform &platform)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// File watching

static bool WatchDirectory(Platform &platform, const char *path, bool recursive)
{
	LOG(Info, "- Missing implementation\n");

	return false;
}

static void UpdateFileWatcher(Platform &platform)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Audio

static aaudio_data_callback_result_t AAudioFillAudioBuffer(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{
	Platform &platform = *(Platform*)userData;
	AudioDevice &audio = platform.audio;

	SoundBuffer soundBuffer = {};
	soundBuffer.samplesPerSecond = audio.samplesPerSecond;
	soundBuffer.sampleCount = numFrames;
	soundBuffer.samples = (i16*)audioData;
	platform.OnPlatformRenderAudio(platform.pub, soundBuffer);

	return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static bool InitializeAudioDevice(Platform &platform)
{
	AudioDevice &audio = platform.audio;

	AAudioStreamBuilder *builder;
	aaudio_result_t result = AAudio_createStreamBuilder(&builder);
	if ( result == AAUDIO_OK )
	{
		const int32_t deviceId = 0;
		AAudioStreamBuilder_setDeviceId(builder, deviceId);
		AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
		AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
		AAudioStreamBuilder_setSampleRate(builder, audio.samplesPerSecond);
		AAudioStreamBuilder_setChannelCount(builder, audio.channelCount);
		AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
		AAudioStreamBuilder_setBufferCapacityInFrames(builder, audio.samplesPerSecond * audio.writeAheadMillis / 1000);
		AAudioStreamBuilder_setDataCallback(builder, AAudioFillAudioBuffer, &platform);

		AAudioStream *stream;
		result = AAudioStreamBuilder_openStream(builder, &stream);
		if ( result == AAUDIO_OK )
		{
			LOG(Info, "- AAudioStream created successfully!\n");

			// TODO(jesus): Perform checks?
			// aaudio_format_t dataFormat = AAudioStream_getDataFormat(stream);
			// if (dataFormat == AAUDIO_FORMAT_PCM_I16) { }

			result = AAudioStream_requestStart(stream);
			if ( result == AAUDIO_OK )
			{
				LOG(Info, "- AAudioStream is playing...\n");
				audioStream = stream;
				audio.initialized = true;
				audio.isPlaying = true;
			}
			else
			{
				LOG(Error, "- Error starting AAudioStream\n");
			}
		}
		else
		{
			LOG(Error, "- Error creating an AAudioStream\n");
		}

		AAudioStreamBuilder_delete(builder);
	}
	else
	{
		LOG(Error, "- Error creating an AAudioStreamBuilder\n");
	}

	return audio.initialized;
}

static void WaitForAudioDevice(Platform &platform)
{
	// Unused: AAudio drives playback from its own thread, so USE_AUDIO_THREAD
	// is 0 here and there is no audio thread of ours to pace.
}

static void UpdateAudioDevice(Platform &platform)
{
	// NOTE(jesus): AAudio makes an async call to AAudioFillAudioBuffer.
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point

void android_main(struct android_app* app)
{
	androidApp = app;
	app->onAppCmd = AndroidHandleAppCommand;
	app->onInputEvent = AndroidHandleInputEvent;
	app->userData = &platform;

	Main(0, nullptr);

	ANativeActivity_finish(app->activity);
}
