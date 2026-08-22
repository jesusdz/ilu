#ifndef GAME_H
#define GAME_H

////////////////////////////////////////////////////////////////////////
// Engine Types
////////////////////////////////////////////////////////////////////////

struct Entity;

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

//struct Box
//{
//	float2 pos;
//	float2 size;
//	float4 color;
//};

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
	const char *name;
	u16 offset;
	PropertyType type;
};

struct Game
{
	GameState state;

	f32 deltaSeconds;
	f32 accumulatedSeconds;

	GameInput input;

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

	u32 propertyCount;
	Property properties[32];
};

////////////////////////////////////////////////////////////////////////
// Engine -> Game interface
////////////////////////////////////////////////////////////////////////

void GameRegisterProperties(Game &game);
void GameSetInput(Game &game, const Keyboard &, const Mouse &, const Gamepad &);
void GameStart(Game &game);
void GameSimulate(Game &game);
void GameUpdate(Game &game);
void GameStop(Game &game);

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
