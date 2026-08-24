#ifndef GAME_H
#define GAME_H

////////////////////////////////////////////////////////////////////////
// Game Types
////////////////////////////////////////////////////////////////////////

enum GameState
{
	GameStateStopped,
	GameStateStarting,
	GameStateRunning,
	GameStateStopping,
	GameStateCount,
};

union InputButton
{
	u8 press : 1;
	u8 pressed : 1;
	u8 release : 1;
};

struct GameInput
{
	float2 move;
	InputButton jump;
};

enum PlayerState
{
	OnFloor,
	OnPlatform,
	OnAir,
};

enum PropertyType : u8
{
	Property_U32,
	Property_ID,
};

struct Property
{
	PropertyType type;
	const char *name;
	u16 offset;
};

enum ScriptHookType
{
	ScriptHook_Start,
	ScriptHook_Simulate,
	ScriptHook_Update,
	ScriptHook_Stop,
	ScriptHook_Count,
};

typedef void (*ScriptHook)(void *instance);

struct Script
{
	const char *name;
	u32 propertyFirst;
	u32 propertyCount;
	u32 instanceSize;
	ScriptHook hooks[ScriptHook_Count];
};

constexpr u32 MAX_SCRIPTS = 64;
constexpr u32 MAX_PROPERTIES = MAX_SCRIPTS * 64;
constexpr u32 MAX_SCRIPT_INSTANCES = 1024;
constexpr u32 SCRIPT_INSTANCE_ALIGN = 16;
constexpr u32 SCRIPT_INSTANCE_DATA_SIZE = MAX_SCRIPT_INSTANCES * 128; // 128K

struct ScriptInstance
{
	ID entity; // Owner entity (invalid means remove the instance)
	const char *scriptName;
	u32 offset; // Offset into the data blob
	u32 size; // To compare against new hot-reloaded data
	u16 scriptIndex;
};

struct Game
{
	GameState state;
	GameInput input;

	f32 deltaSeconds;
	f32 accumulatedSeconds;

	u32 propertyCount;
	Property properties[MAX_PROPERTIES];

	u32 scriptCount;
	Script scripts[MAX_SCRIPTS];

	u32 scriptInstanceCount;
	ScriptInstance scriptInstances[MAX_SCRIPT_INSTANCES];

	u32 scriptInstanceDataUsed;
	alignas(SCRIPT_INSTANCE_ALIGN) byte scriptInstanceData[SCRIPT_INSTANCE_DATA_SIZE];

	ID currentEntity;
};

////////////////////////////////////////////////////////////////////////
// Engine -> Game interface
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Registration

void RegisterScripts(Game &game);

////////////////////////////////////////////////////////////////////////
// Game scripts

struct ScriptPlayerController
{
	PlayerState playerState;

	ID sprPlayerIdle;
	ID sprPlayerRun;
	ID sprPlayerJump;
	ID sprPlayerFall;

	ID sndJump;
	ID modEquinox;

	bool playingMusic;

	Camera camera;

	ID roomId;
};

void Start(ScriptPlayerController &script);
void Simulate(ScriptPlayerController &script);
void Update(ScriptPlayerController &script);
void Stop(ScriptPlayerController &script);

////////////////////////////////////////////////////////////////////////
// Game -> Engine interface
////////////////////////////////////////////////////////////////////////

Entity &GetSelf();
ID FindEntity(const char *name);
Entity *TryGetEntity(ID entityId); // Null once the entity is gone
ID FindRoom(const char *name);
Room *TryGetRoom(ID roomId);       // Null once the room is gone
void EntitySetPosition(Entity &entity, float3 position);
void DrawBox(float2 pos, float2 size, float4 color);
bool IsColliderAtWorldPos(float2 worldPos);
bool IsColliderInBox(float2 pos, float2 size);

#endif // GAME_H
