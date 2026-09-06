#include "ilu_core.h"

#define USE_EDITOR ( PLATFORM_LINUX || PLATFORM_WINDOWS )
#define USE_UI ( PLATFORM_LINUX || PLATFORM_WINDOWS )
#define USE_DATA_BUILD ( PLATFORM_LINUX || PLATFORM_WINDOWS )

#if USE_UI

#define STB_RECT_PACK_IMPLEMENTATION
#include "libs/stb/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb/stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_LINEAR // Only stbi_load_from_memory (LDR) is used, never stbi_loadf (HDR) - this also drops stb_image.h's own <math.h> include, only needed by the linear/HDR path.
#define STBI_NO_HDR
#include "libs/stb/stb_image.h"

#endif // #if USE_UI

#define TOOLS_GFX_FUNCTION_POINTERS
#include "ilu_gfx.h"

#define PLATFORM_API
#include "platform.h"

#if USE_UI
#include "ilu_ui.h"
#endif

#define ILU_PROFILE_GPU
#include "ilu_profile.h"

#include "ilu_id.h"


// C/HLSL shared types and bindings
#include "shaders/types.hlsl"
#include "shaders/bindings.hlsl"


#include "engine.h"

#if USE_EDITOR
#include "editor.h"
#endif


#if USE_DATA_BUILD
static constexpr bool sLoadShadersFromText = true;
#else
static constexpr bool sLoadShadersFromText = false;
#endif

// Access to singletons
inline Host &GetHost() { return *sHost; }
inline Window &GetWindow() { return *sHost->window; }
inline Engine &GetEngine() { return *sHost->engine; }
inline Game &GetGame() { return sHost->engine->game; }
#if USE_EDITOR
inline Editor &GetEditor() { return *sHost->editor; }
#endif // USE_EDITOR



static const char * InternString(const char *str)
{
	const char *intern = MakeStringIntern(sHost->stringInterning, str);
	return intern;
}


////////////////////////////////////////////////////////////////////////
// Game loop

static void GameSetInput(Game &game, const InputAccumulator &input)
{
	game.input = {};

	// Keyboard

	const Keyboard &keyboard = input.keyboard;
	game.input.move.x += KeyPressed(keyboard, K_D) ? 1.0f : 0.0f;
	game.input.move.x -= KeyPressed(keyboard, K_A) ? 1.0f : 0.0f;
	game.input.move.y += KeyPressed(keyboard, K_W) ? 1.0f : 0.0f;
	game.input.move.y -= KeyPressed(keyboard, K_S) ? 1.0f : 0.0f;
	game.input.jump.press = KeyPress(keyboard, K_SPACE);
	game.input.jump.pressed = KeyPressed(keyboard, K_SPACE);
	game.input.jump.release = KeyRelease(keyboard, K_SPACE);

	// Gamepad

	const Gamepad &gamepad = input.gamepad;
	game.input.move += gamepad.leftAxis;
	game.input.jump.press |= ButtonPress(gamepad.a);
	game.input.jump.pressed |= ButtonPressed(gamepad.a);
	game.input.jump.release |= ButtonRelease(gamepad.a);
}

static void GameStop(Engine &engine)
{
	Game &game = engine.game;

	if ( game.state != GameStateStopped )
	{
		// If starting never reached its Start hooks, there is nothing to stop
		if ( game.state != GameStateStarting ) {
			RunScriptHooks(engine, ScriptHook_Stop);
		}

		AudioStopAll(engine.audio);
		ClearParticles(engine.scene);

		GfxWaitDeviceIdle(engine.gfx);
		DestroyRenderTargets(engine.gfx, engine.gfx.renderTargets);
		CreateRenderTargets(engine.gfx);

		game.state = GameStateStopped;
	}
}

void GameUpdate(Engine &engine, const Host &host)
{
	Game &game = engine.game;

	if (game.state == GameStateStarting)
	{
		GfxWaitDeviceIdle(engine.gfx);
		DestroyRenderTargets(engine.gfx, engine.gfx.renderTargets);
		CreateRenderTargets(engine.gfx, SCENE_WIDTH, SCENE_HEIGHT);

		game.accumulatedInput = {};
		game.accumulatedSeconds = 0.0f;

		StartParticles(engine.scene);
		RunScriptHooks(engine, ScriptHook_Start);
		game.state = GameStateRunning;
	}

	if (game.state == GameStateRunning)
	{
		constexpr f32 fixedStepSeconds = SIMULATE_SECONDS;
		constexpr f32 maxFrameSeconds = 0.25f; // avoid catch-up bursts after stalls (hot-reload, shader compiles...)

		InputAccumulate(game.accumulatedInput, *host.gamepad, host.window->keyboard);

		game.accumulatedSeconds += Min(engine.gfx.deltaSeconds, maxFrameSeconds);

		while (game.accumulatedSeconds >= fixedStepSeconds)
		{
			game.deltaSeconds = fixedStepSeconds;
			GameSetInput(game, game.accumulatedInput);
			RunScriptHooks(engine, ScriptHook_Simulate);
			SimulateParticles(engine.scene, fixedStepSeconds);
			InputConsume(game.accumulatedInput);
			game.accumulatedSeconds -= fixedStepSeconds;
		}

		RunScriptHooks(engine, ScriptHook_Update);
	}

	if (game.state == GameStateStopping)
	{
		GameStop(engine);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// UI

#if USE_UI

static void UIBeginFrameRecording(Engine &engine)
{
	UI &ui = engine.ui;
	const Window &window = GetWindow();
	Graphics &gfx = engine.gfx;

	UI_SetInputState(ui, window.keyboard, window.mouse, window.chars);
	UI_SetViewportSize(ui, uint2{window.width, window.height});

	UI_BeginFrame(ui);
}

static void UIEndFrameRecording(Engine &engine)
{
	UI &ui = engine.ui;
	UI_EndFrame(ui);
}

#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Dynamic library interface

#if PLATFORM_WINDOWS
#define ENGINE_API extern "C" __declspec(dllexport)
#else
#define ENGINE_API extern "C"
#endif

// Bumped by hand whenever the layout of the retained engine state changes in a way its size
// does not capture, for instance reordering fields that happen to have the same size.
#define ENGINE_STATE_VERSION 1

// Signature to let the platform know if the memory layout of engine data changed
ENGINE_API u32 OnPlatformGetStateSignature()
{
	const u32 layout[] = {
		ENGINE_STATE_VERSION,
		sizeof(Engine),
		sizeof(Graphics),
		sizeof(Scene),
		sizeof(Audio),
		sizeof(Game),
		sizeof(Script),
#if USE_UI
		sizeof(UI),
#endif
#if USE_EDITOR
		sizeof(Editor),
#endif
	};

	const u32 signature = HashFNV(layout, sizeof(layout));
	return signature;
}

ENGINE_API void OnPlatformLoadEngine(Host &host)
{
	SetHost(host);
	SetGraphicsAPI(&host.graphicsAPI);
	PROFILE_INIT();

	const bool firstLoad = ( host.engine == nullptr );

	if ( firstLoad )
	{
		host.engine = PushZeroStruct(GlobalArena, Engine);
		Engine &engine = GetEngine();

		// The ID pool lives in the retained engine state, so it is only reset once
		InitializeIDPool();

		engine.scriptData.arena = PushSubArena(GlobalArena, SCRIPT_DATA_MEMORY, "Script component data");

#if USE_DATA_BUILD
		bool buildAssets = false;
		bool exitAfterBuild = false;
		for ( u32 i = 0; i < host.argc; ++i ) {
			if ( StrEq(host.argv[i], "--build-assets") ) {
				buildAssets = true;
				exitAfterBuild = true;
			}
		}

		const FilePath assetsFilepath = MakePath(DataDir, "assets.dat");
		if ( !ExistsFile(assetsFilepath.str) ) {
			buildAssets = true;
		}

		if ( buildAssets ) {
			const FilePath shadersFilepath = MakePath(DataDir, "shaders.dat");
			BuildShaders(engine, shadersFilepath.str);
			const FilePath descriptorsFilepath = MakePath(AssetDir, "assets.txt");
			BuildAssetsFromTxt(engine, descriptorsFilepath.str, assetsFilepath.str);
			if (exitAfterBuild) {
				PlatformQuit();
			}
		}
#endif // USE_DATA_BUILD

#if USE_EDITOR
		host.editor = PushZeroStruct(GlobalArena, Editor);
#endif // USE_EDITOR
	}

	Engine &engine = GetEngine();

	RegisterScripts();
	RebindScripts(engine);

	if ( !firstLoad )
	{
		UI_ResetStyle(engine.ui);

		PROFILE_GPU_INIT(engine.gfx.device);
	}

}

ENGINE_API void OnPlatformUnloadEngine(Host &host)
{
	Graphics &gfx = host.engine->gfx;

	if ( IsValidGraphicsDevice(gfx.device) )
	{
		WaitDeviceIdle(gfx.device);
		PROFILE_GPU_CLEANUP(gfx.device);
	}
}

ENGINE_API bool OnPlatformInit(Host &host)
{
	Engine &engine = GetEngine();

	Graphics &gfx = engine.gfx;
	Game &game = engine.game;

#if USE_EDITOR
	CompileModifiedShaders();
	engine.settings.hotReload = true;
#endif

	// Initialize graphics
	if ( !InitializeGraphicsDriver(gfx.device, GlobalArena) )
	{
		// TODO: Actually we could throw a system error and exit...
		LOG(Error, "InitializeGraphicsDriver failed!\n");
		return false;
	}

	// Initialize sound system
	if ( !InitializeAudio(engine.audio, GlobalArena) )
	{
		LOG(Error, "InitializeAudio failed!\n");
		return false;
	}

	return true;
}

ENGINE_API bool OnPlatformWindowInit(Host &host)
{
	Engine &engine = GetEngine();
	Graphics &gfx = engine.gfx;

	if ( !InitializeGraphicsSurface(gfx.device, *host.window) )
	{
		// TODO: Actually we could throw a system error and exit...
		LOG(Error, "InitializeGraphicsSurface failed!\n");
		return false;
	}

	if ( gfx.deviceInitialized )
	{
		// TODO: Check the current device still supports the new surface
	}
	else
	{
		if (!sLoadShadersFromText) {
			LoadShadersFromBin(engine);
		}

		if ( !InitializeGraphics(engine, GlobalArena) )
		{
			// TODO: Actually we could throw a system error and exit...
			LOG(Error, "InitializeGraphics failed!\n");
			return false;
		}

		gfx.camera = {
			.projectionType = ProjectionOrthographic,
			.znear = -10.0f,
			.zfar = 10.0f,
			.height = 8.0f,
		};


		InitializeScene(engine);

#if USE_EDITOR
		EditorInitialize(engine);
#else
		LoadSceneFromBin(engine);
#endif
	}

	return true;
}

ENGINE_API void OnPlatformUpdate(Host &host)
{
	PROFILE_THREAD("UpdateAndRender");
	PROFILE_FRAME();
	PROFILE_BLOCK(Update);

	Engine &engine = GetEngine();
	Graphics &gfx = engine.gfx;

	const Clock begin = GetClock();

#if PLATFORM_ANDROID
	// TODO(jesus): This is test code
	static bool firstUpdate = true;
	if ( firstUpdate )
	{
		firstUpdate = false;
		//LoadSceneFromBin(engine);
		// Plays whichever module came first out of the asset file. LoadSceneFromBin ran
		// back in OnPlatformWindowInit, so the pool is already populated here.
		if ( engine.audio.musicFileCount > 0 )
		{
			MusicPlay(engine, engine.audio.musicFiles[0].desc.id);
		}
	}
#endif

#if USE_UI
	UIBeginFrameRecording(engine);
#endif

#if USE_EDITOR
	if (engine.settings.hotReload && host.fileChangesDetected)
	{
		if ( CompileModifiedShaders() )
		{
			// NOTE(jesus): Recompiling all pipelines here even if likely only a shader was recompiled :-S
			WaitDeviceIdle(gfx.device);
			Scratch scratch;
			RecompilePipelines(engine, scratch.arena);
		}

		RecreateModifiedTextures(engine);
	}

	EditorUpdate(engine);
#endif

	{
		PROFILE_BLOCK(GameUpdate);
		GameUpdate(engine, host);
	}

#if USE_UI
	UIEndFrameRecording(engine);
#endif

	// The audio pools are missing on purpose, CompactAudio runs on the mixing thread.
	CompactRooms(engine.scene);
	CompactEntities(engine.scene);
	CompactSprites(engine.scene);
	CompactParticleEffects(engine.scene);
	CompactMaterials(engine.gfx);
	CompactTextures(engine.gfx);
}

ENGINE_API void OnPlatformRenderGraphics(Host &host)
{
	PROFILE_BLOCK(RenderGraphics);

	Engine &engine = GetEngine();
	Graphics &gfx = engine.gfx;

	static Clock lastClock = GetClock();
	Clock currentClock = GetClock();
	gfx.deltaSeconds = GetSecondsElapsed(lastClock, currentClock);
	lastClock = currentClock;

	if ( !gfx.deviceInitialized )
	{
		return;
	}

	if ( !IsValidSwapchain(gfx.device) )
	{
		GfxWaitDeviceIdle(gfx);
		DestroyRenderTargets(gfx, gfx.renderTargets);
		if ( host.window->width != 0 && host.window->height != 0 )
		{
			RecreateSwapchain(gfx.device, *host.window);

			char debugName[16];
			for (u32 i = 0; i < ARRAY_COUNT(gfx.device.swapchain.imageHandles); ++i) {
				SPrintf(debugName, "swapchain_%u", i);
				SetObjectNameImage(gfx.device, gfx.device.swapchain.imageHandles[i], debugName);
			}

			u32 sceneWidth = 0;
			u32 sceneHeight = 0;
			if ( engine.game.state != GameStateStopped ) {
				sceneWidth = SCENE_WIDTH;
				sceneHeight = SCENE_HEIGHT;
			}
			CreateRenderTargets(gfx, sceneWidth, sceneHeight);
		}
	}

	if ( IsValidSwapchain(gfx.device) )
	{
		RenderGraphics(engine);

#if USE_EDITOR
		EditorPostRender(engine);
#endif
	}
}

ENGINE_API void OnPlatformPreRenderAudio(Host &host)
{
	PROFILE_THREAD("Audio");
	PROFILE_FLUSH();
	PROFILE_BLOCK(PreRenderAudio);
	Engine &engine = GetEngine();
	PreRenderAudio(engine.audio);
}

ENGINE_API void OnPlatformRenderAudio(Host &host, SoundBuffer &soundBuffer)
{
	PROFILE_FLUSH();
	PROFILE_BLOCK(RenderAudio);
	Engine &engine = GetEngine();
	RenderAudio(engine, soundBuffer);
}

ENGINE_API void OnPlatformWindowCleanup(Host &host)
{
	Engine &engine = GetEngine();
	Graphics &gfx = engine.gfx;

	GfxWaitDeviceIdle(gfx);
	DestroyRenderTargets(gfx, gfx.renderTargets);
	DestroySwapchain(gfx.device);
	CleanupGraphicsSurface(gfx.device);
}

ENGINE_API void OnPlatformCleanup(Host &host)
{
	Engine &engine = GetEngine();
	Graphics &gfx = engine.gfx;
	Game &game = engine.game;

	GfxWaitDeviceIdle(gfx);

#if USE_UI
	UI_Cleanup(engine.ui);
#endif

	CleanupGraphics(gfx);
}


////////////////////////////////////////////////////////////////////////
// Game API

Entity &GetSelf()
{
	Game &game = GetGame();
	Entity &entity = GetEntity(game.currentEntity);
	return entity;
}

void SetCamera(const Camera &camera)
{
	Engine &engine = GetEngine();
	engine.gfx.camera = camera;
}

ID FindRoom(const char *name)
{
	Engine &engine = GetEngine();
	for (u32 i = 0; i < engine.scene.roomCount; ++i)
	{
		const Room &room = engine.scene.rooms[i];
		if ( room.id && StrEq(room.name, name) ) {
			return room.id;
		}
	}
	return {};
}

// Nullable counterpart to GetRoom, which asserts. Game code holds an ID across frames
// and has to cope with the room going away.
Room *TryGetRoom(ID roomId)
{
	Room *roomPtr = nullptr;
	if ( roomId ) {
		roomPtr = &GetRoom(roomId);
	}
	return roomPtr;
}

ID FindEntity(const char *name)
{
	Engine &engine = GetEngine();
	for (u32 i = 0; i < engine.scene.entityCount; ++i)
	{
		const Entity &entity = engine.scene.entities[i];
		if ( entity.id && StrEq(entity.name, name) ) {
			return entity.id;
		}
	}
	return {};
}

// See TryGetRoom
Entity *TryGetEntity(ID entityId)
{
	Entity *ent = nullptr;
	if ( entityId ) {
		ent = &GetEntity(entityId);
	}
	return ent;
}

ID FindSprite(const char *name)
{
	Engine &engine = GetEngine();
	ID id = FindSprite(engine.scene, name);
	return id;
}

ID GetAudioClip(const char *name)
{
	Engine &engine = GetEngine();
	for (u32 i = 0; i < engine.audio.clipCount; ++i)
	{
		const AudioClipDesc &desc = engine.audio.clips[i].desc;
		if ( desc.id && StrEq(desc.name, name) ) {
			return desc.id;
		}
	}
	return {};
}

u32 PlayAudioClip(ID clipId)
{
	Engine &engine = GetEngine();
	u32 ret = PlayAudioClip(engine.audio, clipId);
	return ret;
}

ID GetMusic(const char *name)
{
	Engine &engine = GetEngine();
	for (u32 i = 0; i < engine.audio.musicFileCount; ++i)
	{
		const MusicFileDesc &desc = engine.audio.musicFiles[i].desc;
		if ( desc.id && StrEq(desc.name, name) ) {
			return desc.id;
		}
	}
	return {};
}

void PlayMusic(ID musicId)
{
	Engine &engine = GetEngine();
	MusicPlay(engine, musicId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementations

#define PLATFORM_API_IMPLEMENTATION
#include "platform.h"

#include "graphics.cpp"
#include "scene.cpp"
#include "script.cpp"
#include "render.cpp"

#include "data.cpp"
#include "audio.cpp"

#define ILU_PROFILE_IMPLEMENTATION
#include "ilu_profile.h"

#if USE_EDITOR
#include "editor.cpp"
#endif

#include "libs/ibxm/ibxm.c"

#include "game.cpp"

#define ILU_ID_POOL GetEngine().idPool
#define ILU_ID_IMPLEMENTATION
#include "ilu_id.h"

// Reflection declarations are put at the end of the engine so information about all types is available
#include "reflex.generated.h"

// TODO:
// - [ ] Instead of binding descriptors per entity, group entities by material and perform a multi draw call for each material group.
// - [ ] GPU culling: Modify the compute to perform frustum culling and save the result in the buffer.
// - [ ] Text rendering
// - [ ] Include directive in the C AST parsing code.
//
// DONE:
// - [X] Avoid using push constants and put transformation matrices in buffers instead.
// - [X] Investigate how to write descriptors in a more elegant manner (avoid hardcoding).
// - [X] Put all the geometry in the same buffer.
// - [X] GPU culling: Add a "hello world" compute shader that writes some numbers into a buffer.
// - [X] GPU time queries
// - [X] GPU culling: As a first step, perform frustum culling in the CPU.
// - [X] Avoid duplicated global descriptor sets.
// - [X] Have a single descripor set for global info that only changes once per frame
