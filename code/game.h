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

struct Script
{
	const char *name;
	u32 propertyOffset;
	u32 propertyCount;
};

constexpr u32 MAX_SCRIPTS = 64;
constexpr u32 MAX_PROPERTIES = MAX_SCRIPTS * 64;

struct Game
{
	GameState state;
	GameInput input;

	f32 deltaSeconds;
	f32 accumulatedSeconds;

	u32 scriptCount;
	Script scripts[MAX_SCRIPTS];

	u32 propertyCount;
	Property properties[MAX_PROPERTIES];
};

////////////////////////////////////////////////////////////////////////
// Engine -> Game interface
////////////////////////////////////////////////////////////////////////

struct ScriptPlayerController
{
	PlayerState playerState;

	ID entPlayer;
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

void RegisterScripts(Game &game);
void Start(ScriptPlayerController &script);
void Simulate(ScriptPlayerController &script, Game &game);
void Update(ScriptPlayerController &script);
void Stop(ScriptPlayerController &script);

////////////////////////////////////////////////////////////////////////
// Game -> Engine interface
////////////////////////////////////////////////////////////////////////

ID FindEntity(const char *name);
Entity *TryGetEntity(ID entityId); // Null once the entity is gone
ID FindRoom(const char *name);
Room *TryGetRoom(ID roomId);       // Null once the room is gone
void EntitySetPosition(Entity &entity, float3 position);
void DrawBox(float2 pos, float2 size, float4 color);
bool IsColliderAtWorldPos(float2 worldPos);
bool IsColliderInBox(float2 pos, float2 size);

#endif // GAME_H
