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

// Needed before ilu_ui.h
struct ImagePixels
{
	stbi_uc* pixels;
	i32 width;
	i32 height;
	i32 channelCount;
	bool constPixels;
};

#define TOOLS_GFX_FUNCTION_POINTERS
#include "ilu_gfx.h"

#define PLATFORM_API
#include "platform.h"

#if USE_UI
#include "ilu_ui.h"
#endif

#define ILU_PROFILE_GPU
#include "ilu_profile.h"


// C/HLSL shared types and bindings
#include "shaders/types.hlsl"
#include "shaders/bindings.hlsl"

#include "handle_pool.h"
#include "data.h"
#include "audio.h"
#include "graphics.h"
#include "scene.h"
#include "render.h"
#include "game.h"
#if USE_EDITOR
#include "editor.h"
#endif
#include "engine.h"


#if USE_DATA_BUILD
static constexpr bool sLoadShadersFromText = true;
#else
static constexpr bool sLoadShadersFromText = false;
#endif

static Engine *engine = nullptr;


static const char * InternString(const char *str)
{
	const char *intern = MakeStringIntern(sPlatform->stringInterning, str);
	return intern;
}

u32 U32FromChars(char a, char b, char c, char d)
{
	const u32 res =
		(u32)a << 0 |
		(u32)b << 8 |
		(u32)c << 16 |
		(u32)d << 24 ;
	return res;
}

Engine &GetEngine(Plat &platform)
{
	Engine *engine = platform.engine;
	ASSERT(engine != NULL);
	return *engine;
}

#if USE_UI
UI &GetUI(Engine &engine)
{
	return engine.ui;
}
#endif


AssetDescriptors GetAssetDescriptors(Engine &engine, Arena &arena)
{
	static TextureDesc textureDescs[MAX_TEXTURES];
	u32 textureCount = 0;
	for (u16 i = 0; i < HandleCount(engine.gfx.textureHandles); ++i) {
		textureDescs[textureCount] = engine.gfx.textureDescs[i];
		if ( !( textureDescs[textureCount].flags & AssetFlag_Ghost ) ) {
			textureCount++;
		}
	}

	static SpriteDesc spriteDescs[MAX_SPRITES];
	u32 spriteCount = 0;
	for (u16 i = 0; i < HandleCount(engine.scene.spriteHandles); ++i) {
		spriteDescs[spriteCount++] = GetSpriteDesc(engine.scene, GetHandleAt(engine.scene.spriteHandles, i));
	}

	static MaterialDesc materialDescs[MAX_MATERIALS];
	u32 materialCount = 0;
	for (u16 i = 0; i < HandleCount(engine.gfx.materialHandles); ++i) {
		materialDescs[materialCount] = engine.gfx.materialDescs[i];
		if ( !( materialDescs[materialCount].flags & AssetFlag_Ghost ) ) {
			materialCount++;
		}
	}

	static EntityDesc entityDescs[MAX_ENTITIES];
	u32 entityCount = 0;
	for (u16 i = 0; i < HandleCount(engine.scene.entityHandles); ++i) {
		entityDescs[entityCount++] = GetEntityDesc(engine.scene, GetHandleAt(engine.scene.entityHandles, i));
	}

	static RoomDesc roomDescs[MAX_ROOMS];
	u32 roomCount = 0;
	for (u16 roomIndex = 0; roomIndex < HandleCount(engine.scene.roomHandles); ++roomIndex) {
		const Room &room = engine.scene.rooms[roomIndex];
		RoomDesc &desc = roomDescs[roomCount++];
		desc = {};
		desc.name = room.name;
		desc.pos = room.pos;
		for (u32 l = 0; l < ARRAY_COUNT(room.layers); ++l) {
			const Layer &layer = room.layers[l];
			if (!layer.initialized) {
				continue;
			}
			LayerDesc &layerDesc = desc.layers[desc.layerCount++];
			layerDesc.name = layer.name;
			layerDesc.isBase = layer.isBase;
			layerDesc.visible = layer.visible;
			layerDesc.isCollider = layer.isCollider;
			layerDesc.size = layer.size;
			layerDesc.tiles = (TileDesc*)(arena.base + arena.used);
			layerDesc.tileCount = 0;
			if (layer.isCollider)
			{
				for (u32 x = 0; x < layer.size.x; ++x) {
					for (u32 y = 0; y < layer.size.y; ++y) {
						const u16 collider = (u16)layer.cells[x][y].collider;
						if (collider == 0) continue;
						TileDesc &tile = *PushStruct(arena, TileDesc);
						tile.x = (u16)x;
						tile.y = (u16)y;
						tile.collider = collider;
						layerDesc.tileCount++;
					}
				}
			}
			else
			{
				// Sprites
				for (u32 x = 0; x < layer.size.x; ++x) {
					for (u32 y = 0; y < layer.size.y; ++y) {
						const SpriteH spriteH = layer.cells[x][y].handle;
						if (!spriteH) continue;
						TileDesc &tile = *PushStruct(arena, TileDesc);
						tile.x = (u16)x;
						tile.y = (u16)y;
						tile.spriteIndex = GetSpriteIndex(engine.scene, spriteH);
						layerDesc.tileCount++;
					}
				}
			}
		}
	}

	static AudioClipDesc audioClipDescs[MAX_AUDIO_CLIPS];
	u32 audioClipCount = 0;
	for (u16 i = 0; i < HandleCount(engine.audio.clipHandles); ++i) {
		audioClipDescs[audioClipCount] = engine.audio.clipDescs[i];
	}

	const AssetDescriptors assetDescs = {
		.shaderDescs = nullptr, // shaderSourceDescs,
		.shaderDescCount = 0, //ARRAY_COUNT(shaderSourceDescs),
		.textureDescs = textureDescs,
		.textureDescCount = textureCount,
		.spriteDescs = spriteDescs,
		.spriteDescCount = spriteCount,
		.materialDescs = materialDescs,
		.materialDescCount = materialCount,
		.entityDescs = entityDescs,
		.entityDescCount = entityCount,
		.roomDescs = roomDescs,
		.roomDescCount = roomCount,
		.audioClipDescs = audioClipDescs,
		.audioClipDescCount = audioClipCount,
	};

	return assetDescs;
}

static bool PushDataArenaState(Engine &engine)
{
	const bool ok = engine.dataArenaStateCount < ARRAY_COUNT(engine.dataArenaStates);
	if (ok)
	{
		engine.dataArenaStates[engine.dataArenaStateCount++] = DataArena;
	}
	return ok;
}

static bool PopDataArenaState(Engine &engine)
{
	const bool ok = engine.dataArenaStateCount > 0;
	if (ok)
	{
		DataArena = engine.dataArenaStates[--engine.dataArenaStateCount];
	}
	return ok;
}


#if USE_DATA_BUILD
void LoadSceneFromTxt(Engine &engine, const char *filepath)
{
	if ( PushDataArenaState(engine) )
	{
		Arena &dataArena = DataArena;
		AssetDescriptors assetDescriptors = ParseDescriptors(filepath, dataArena);

		// Textures
		for (u32 i = 0; i < assetDescriptors.textureDescCount; ++i)
		{
			CreateTexture(engine.gfx, assetDescriptors.textureDescs[i]);
		}

		// Materials
		for (u32 i = 0; i < assetDescriptors.materialDescCount; ++i)
		{
			CreateMaterial(engine.gfx, assetDescriptors.materialDescs[i]);
		}

		// Sprites (must be before entities and rooms)
		static SpriteH spriteHandles[MAX_SPRITES];
		for (u32 i = 0; i < assetDescriptors.spriteDescCount; ++i)
		{
			spriteHandles[i] = CreateSprite(engine, assetDescriptors.spriteDescs[i]);
		}

		// Entities
		for (u32 i = 0; i < assetDescriptors.entityDescCount; ++i)
		{
			CreateEntity(engine, assetDescriptors.entityDescs[i]);
		}

		// Rooms (layer cells reference sprites by index into the sprite list)
		if (assetDescriptors.roomDescCount > 0)
		{
			for (u32 i = 0; i < assetDescriptors.roomDescCount; ++i)
			{
				CreateRoom(engine, assetDescriptors.roomDescs[i], spriteHandles, assetDescriptors.spriteDescCount);
			}
		}

		// Audio clips
		for (u32 i = 0; i < assetDescriptors.audioClipDescCount; ++i)
		{
			CreateAudioClip(engine, assetDescriptors.audioClipDescs[i]);
		}

		// Music files
		for (u32 i = 0; i < assetDescriptors.musicFileDescCount; ++i)
		{
			CreateMusicFile(engine, assetDescriptors.musicFileDescs[i]);
		}

		UploadMaterialData(engine.gfx);
		LinkHandles(engine.gfx);
	}
}

void SaveSceneToTxt(Engine &engine, const char *filepath)
{
	Scratch scratch(MB(16)); // holds the tile lists of all rooms
	const AssetDescriptors assetDescs = GetAssetDescriptors(engine, scratch.arena);

	SaveAssetDescriptors(filepath, assetDescs);
}
#endif // USE_DATA_BUILD

void LoadShadersFromBin(Engine &engine)
{
	const FilePath filepath = MakePath(DataDir, "shaders.dat");
	engine.shaderAssets = OpenAssets(DataArena, filepath.str);
}

void LoadSceneFromBin(Engine &engine)
{
	if (PushDataArenaState(engine))
	{
		const FilePath filepath = MakePath(DataDir, "assets.dat");
		engine.assets = OpenAssets(DataArena, filepath.str);

		// Textures
		for (u32 i = 0; i < engine.assets.header.imageCount; ++i)
		{
			CreateTexture(engine.gfx, engine.assets.images[i]);
		}

		// Materials
		for (u32 i = 0; i < engine.assets.header.materialCount; ++i)
		{
			CreateMaterial(engine.gfx, *engine.assets.materials[i].desc);
		}

		// Sprites (must be before entities and rooms)
		static SpriteH spriteHandles[MAX_SPRITES];
		for (u32 i = 0; i < engine.assets.header.spriteCount; ++i)
		{
			spriteHandles[i] = CreateSprite(engine, *engine.assets.sprites[i].desc);
		}

		// Entities
		for (u32 i = 0; i < engine.assets.header.entityCount; ++i)
		{
			CreateEntity(engine, *engine.assets.entities[i].desc);
		}

		// Rooms (layer cells reference sprites by index into the sprite list)
		if (engine.assets.header.roomCount > 0)
		{
			for (u32 i = 0; i < engine.assets.header.roomCount; ++i)
			{
				CreateRoom(engine, engine.assets.rooms[i], spriteHandles, engine.assets.header.spriteCount);
			}
		}

		// Audio clips
		for (u32 i = 0; i < engine.assets.header.audioClipCount; ++i)
		{
			CreateAudioClip(engine, engine.assets.audioClips[i]);
		}

		// Music files
		for (u32 i = 0; i < engine.assets.header.musicFileCount; ++i)
		{
			CreateMusicFile(engine, engine.assets.musicFiles[i]);
		}

		UploadMaterialData(engine.gfx);
		LinkHandles(engine.gfx);
	}
}


#if USE_DATA_BUILD
void BuildShaders(Engine &engine, const char *outBinFilepath)
{
	CompileShaders();

	Arena scratch = MakeSubArena(DataArena, "Scratch - BuildShaders");
	AssetDescriptors assetDescriptors = {};
	assetDescriptors.shaderDescs = GetShaderSourceDescs();
	assetDescriptors.shaderDescCount = GetShaderSourceDescCount();
	BuildAssets(assetDescriptors, outBinFilepath, scratch);
}

void BuildAssetsFromTxt(Engine &engine, const char *inTxtFilepath, const char *outBinFilepath)
{
	Arena scratch = MakeSubArena(DataArena, "Scratch - BuildAssetsFromTxt");
	AssetDescriptors assetDescriptors = ParseDescriptors(inTxtFilepath, scratch);
	BuildAssets(assetDescriptors, outBinFilepath, scratch);
}
#endif // USE_DATA_BUILD


static bool sKeyPendingRelease[K_COUNT];
static bool sGamepadButtonPendingRelease[ARRAY_COUNT(Gamepad::buttons)];

struct PlatformInput
{
	Gamepad gamepad;
	Keyboard keyboard;
	Mouse mouse;
};

static void InputAccumulate(PlatformInput &input, const PlatformInput &newInput)
{
	for (u32 i = 0; i < K_COUNT; ++i)
	{
		const KeyState newState = newInput.keyboard.keys[i];
		KeyState &state = input.keyboard.keys[i];

		if (newState == KEY_STATE_PRESS)
		{
			// Latch the edge until a fixed step consumes it
			state = KEY_STATE_PRESS;
			sKeyPendingRelease[i] = false;
		}
		else if (newState == KEY_STATE_RELEASE)
		{
			if (state == KEY_STATE_PRESS) {
				// The press was not consumed yet: keep it and release right after
				sKeyPendingRelease[i] = true;
			} else {
				state = KEY_STATE_RELEASE;
			}
		}
	}
	for (u32 i = 0; i < ARRAY_COUNT(input.gamepad.buttons); ++i)
	{
		const ButtonState newState = newInput.gamepad.buttons[i];
		ButtonState &state = input.gamepad.buttons[i];

		if (newState == BUTTON_STATE_PRESS)
		{
			// Latch the edge until a fixed step consumes it
			state = BUTTON_STATE_PRESS;
			sGamepadButtonPendingRelease[i] = false;
		}
		else if (newState == BUTTON_STATE_RELEASE)
		{
			if (state == BUTTON_STATE_PRESS) {
				// The press was not consumed yet: keep it and release right after
				sGamepadButtonPendingRelease[i] = true;
			} else {
				state = BUTTON_STATE_RELEASE;
			}
		}
	}
	input.gamepad.leftTrigger = newInput.gamepad.leftTrigger;
	input.gamepad.rightTrigger = newInput.gamepad.rightTrigger;
	input.gamepad.leftAxis = newInput.gamepad.leftAxis;
	input.gamepad.rightAxis = newInput.gamepad.rightAxis;
}

static void InputConsume(PlatformInput &input)
{
	for (u32 i = 0; i < K_COUNT; ++i)
	{
		KeyState &state = input.keyboard.keys[i];

		if (state == KEY_STATE_PRESS)
		{
			state = sKeyPendingRelease[i] ? KEY_STATE_RELEASE : KEY_STATE_PRESSED;
			sKeyPendingRelease[i] = false;
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
			state = sGamepadButtonPendingRelease[i] ? BUTTON_STATE_RELEASE : BUTTON_STATE_PRESSED;
			sGamepadButtonPendingRelease[i] = false;
		}
		else if (state == BUTTON_STATE_RELEASE)
		{
			state = BUTTON_STATE_IDLE;
		}
	}
}

void GameUpdate(Engine &engine, const Plat &platform)
{
	Game &game = engine.game;

	static PlatformInput accumulatedInput = {};
	static f32 accumulatedSeconds = 0.0f;

	if (game.state == GameStateStarting)
	{
		EngineWaitDeviceIdle(engine.gfx);
		DestroyRenderTargets(engine.gfx, engine.gfx.renderTargets);
		CreateRenderTargets(engine.gfx, SCENE_WIDTH, SCENE_HEIGHT);

		accumulatedInput = {};
		accumulatedSeconds = 0.0f;
		MemSet(sKeyPendingRelease, sizeof(sKeyPendingRelease), 0);

		GameStart(game);
		game.state = GameStateRunning;
	}

	if (game.state == GameStateRunning)
	{
		constexpr f32 fixedStepSeconds = 1.0f / 60.0f;
		constexpr f32 maxFrameSeconds = 0.25f; // avoid catch-up bursts after stalls (hot-reload, shader compiles...)

		const PlatformInput platformInput = {
			.gamepad = *platform.gamepad,
			.keyboard = platform.window->keyboard,
			.mouse = platform.window->mouse,
		};

		InputAccumulate(accumulatedInput, platformInput);

		accumulatedSeconds += Min(engine.gfx.deltaSeconds, maxFrameSeconds);

		//u32 count = 0;

		while (accumulatedSeconds >= fixedStepSeconds)
		{
			game.deltaSeconds = fixedStepSeconds;
			GameSetInput(game, accumulatedInput.keyboard, accumulatedInput.mouse, accumulatedInput.gamepad);
			GameSimulate(game);
			InputConsume(accumulatedInput);
			accumulatedSeconds -= fixedStepSeconds;
			//count++;
		}

		//LOG(Info, "Update count: %u\n", count);

		GameUpdate(game);
	}

	if (game.state == GameStateStopping)
	{
		GameStop(game);

		AudioStopAll(engine);

		EngineWaitDeviceIdle(engine.gfx);
		DestroyRenderTargets(engine.gfx, engine.gfx.renderTargets);
		CreateRenderTargets(engine.gfx);

		game.state = GameStateStopped;
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// UI

#if USE_UI

void UIBeginFrameRecording(Engine &engine)
{
	UI &ui = GetUI(engine);
	const Window &window = *sPlatform->window;
	Graphics &gfx = engine.gfx;

	UI_SetInputState(ui, window.keyboard, window.mouse, window.chars);
	UI_SetViewportSize(ui, uint2{window.width, window.height});

	UI_BeginFrame(ui);
}

void UIEndFrameRecording(Engine &engine)
{
	UI &ui = GetUI(engine);
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

ENGINE_API void OnPlatformLoadEngine(Plat &platform)
{
	SetPlatformAPI(platform);
	SetGraphicsAPI(&platform.graphicsAPI);
	PROFILE_INIT();

	if ( platform.engine )
	{
		UI_ResetStyle(platform.engine->ui);

		// Profile state does not survive the reload, so GPU profiling starts fresh
		PROFILE_GPU_INIT(platform.engine->gfx.device);
	}
}

ENGINE_API void OnPlatformUnloadEngine(Plat &platform)
{
	Graphics &gfx = platform.engine->gfx;
	WaitDeviceIdle(gfx.device);
	PROFILE_GPU_CLEANUP(gfx.device);
}

ENGINE_API bool OnPlatformPreInit(Plat &platform)
{
	::engine = PushZeroStruct(GlobalArena, Engine);
	platform.engine = ::engine;

	Engine &engine = *::engine;

#if USE_DATA_BUILD
	bool buildAssets = false;
	bool exitAfterBuild = false;
	for ( u32 i = 0; i < platform.argc; ++i ) {
		if ( StrEq(platform.argv[i], "--build-assets") ) {
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

	return true;
}

ENGINE_API bool OnPlatformInit(Plat &platform)
{
	SetGraphicsStringInterning(platform.stringInterning);

	Engine &engine = GetEngine(platform);

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

ENGINE_API bool OnPlatformWindowInit(Plat &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	if ( !InitializeGraphicsSurface(gfx.device, *platform.window) )
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

		Initialize(engine.scene.roomHandles, GlobalArena, MAX_ROOMS);
		Initialize(engine.scene.entityHandles, GlobalArena, MAX_ENTITIES);
		Initialize(engine.scene.spriteHandles, GlobalArena, MAX_SPRITES);

#if USE_EDITOR
		EditorInitialize(engine);
#else
		engine.mode = EngineModeGame3D;
		LoadSceneFromBin(engine);
#endif
	}

	return true;
}

ENGINE_API void OnPlatformUpdate(Plat &platform)
{
	PROFILE_THREAD("UpdateAndRender");
	PROFILE_FRAME();
	PROFILE_BLOCK(Update);

	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	::engine = &engine;

	const Clock begin = GetClock();

#if PLATFORM_ANDROID
	// TODO(jesus): This is test code
	static bool firstUpdate = true;
	if ( firstUpdate )
	{
		firstUpdate = false;
		//LoadSceneFromBin(engine);
		MusicPlay(engine, 0);
	}
#endif

#if USE_UI
	UIBeginFrameRecording(engine);
#endif

#if USE_EDITOR
	if (engine.settings.hotReload)
	{
		static Clock lastClock = GetClock();
		const Clock currentClock = GetClock();
		const f32 secondsSinceLastCheck  = GetSecondsElapsed(lastClock, currentClock);

		if ( secondsSinceLastCheck > 0.2 )
		{
			lastClock = currentClock;

			if ( CompileModifiedShaders() )
			{
				// NOTE(jesus): Recompiling all pipelines here even if likely only a shader was recompiled :-S
				WaitDeviceIdle(gfx.device);
				Scratch scratch;
				RecompilePipelines(engine, scratch.arena);
			}

			RecreateModifiedTextures(engine);
		}
	}

	EditorUpdate(engine);
#endif

	{
		PROFILE_BLOCK(GameUpdate);
		GameUpdate(engine, platform);
	}

#if USE_UI
	UIEndFrameRecording(engine);
#endif

	// The audio pools are missing on purpose, CompactAudio runs on the mixing thread.
	CompactRooms(engine.scene);
	CompactEntities(engine.scene);
	CompactSprites(engine.scene);
	CompactMaterials(engine.gfx);
	CompactTextures(engine.gfx);
}

ENGINE_API void OnPlatformRenderGraphics(Plat &platform)
{
	PROFILE_BLOCK(RenderGraphics);

	Engine &engine = GetEngine(platform);
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
		EngineWaitDeviceIdle(gfx);
		DestroyRenderTargets(gfx, gfx.renderTargets);
		if ( platform.window->width != 0 && platform.window->height != 0 )
		{
			RecreateSwapchain(gfx.device, *platform.window);

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

ENGINE_API void OnPlatformPreRenderAudio(Plat &platform)
{
	PROFILE_THREAD("Audio");
	PROFILE_FLUSH();
	PROFILE_BLOCK(PreRenderAudio);
	Engine &engine = GetEngine(platform);
	PreRenderAudio(engine);
}

ENGINE_API void OnPlatformRenderAudio(Plat &platform, SoundBuffer &soundBuffer)
{
	PROFILE_FLUSH();
	PROFILE_BLOCK(RenderAudio);
	Engine &engine = GetEngine(platform);
	RenderAudio(engine, soundBuffer);
}

ENGINE_API void OnPlatformWindowCleanup(Plat &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	EngineWaitDeviceIdle(gfx);
	DestroyRenderTargets(gfx, gfx.renderTargets);
	DestroySwapchain(gfx.device);
	CleanupGraphicsSurface(gfx.device);
}

ENGINE_API void OnPlatformCleanup(Plat &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;
	Game &game = engine.game;

	EngineWaitDeviceIdle(gfx);

#if USE_UI
	UI_Cleanup(engine.ui);
#endif

	CleanupGraphics(gfx);
}


////////////////////////////////////////////////////////////////////////
// Game API

void SetCamera(const Camera &camera)
{
	engine->gfx.camera = camera;
}

RoomH FindRoom(const char *name)
{
	for (u16 i = 0; i < HandleCount(engine->scene.roomHandles); ++i)
	{
		if ( StrEq(engine->scene.rooms[i].name, name) ) {
			return GetHandleAt(engine->scene.roomHandles, i);
		}
	}
	return InvalidHandle;
}

Room *GetRoom(RoomH handle)
{
	Room *roomPtr = nullptr;
	if ( handle ) {
		roomPtr = &GetRoom(engine->scene, handle);
	}
	return roomPtr;
}

EntityH FindEntity(const char *name)
{
	for (u16 i = 0; i < HandleCount(engine->scene.entityHandles); ++i)
	{
		if ( StrEq(engine->scene.entities[i].name, name) ) {
			return GetHandleAt(engine->scene.entityHandles, i);
		}
	}
	return InvalidHandle;
}

Entity *GetEntity(EntityH handle)
{
	Entity *ent = nullptr;
	if ( handle ) {
		ent = &GetEntity(engine->scene, handle);
	}
	return ent;
}

AudioClipH GetAudioClip(const char *name)
{
	AudioClipH handle = InvalidHandle;
	for (u16 i = 0; i < HandleCount(engine->audio.clipHandles); ++i)
	{
		const AudioClipDesc &desc = engine->audio.clipDescs[i];
		if ( StrEq(desc.name, name) ) {
			handle = GetHandleAt(engine->audio.clipHandles, i);
			break;
		}
	}
	return handle;
}

u32 PlayAudioClip(AudioClipH handle)
{
	u32 ret = PlayAudioClip(*engine, handle);
	return ret;
}

MusicH GetMusic(const char *name)
{
	MusicH handle = InvalidHandle;
	for (u16 i = 0; i < HandleCount(engine->audio.musicHandles); ++i)
	{
		const MusicFileDesc &desc = engine->audio.musicFileDescs[i];
		if ( StrEq(desc.name, name) ) {
			handle = GetHandleAt(engine->audio.musicHandles, i);
			break;
		}
	}
	return handle;
}

void PlayMusic(MusicH handle)
{
	MusicPlay(*engine, handle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementations

#define PLATFORM_API_IMPLEMENTATION
#include "platform.h"

#include "graphics.cpp"
#include "scene.cpp"
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
