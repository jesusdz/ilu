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

struct Box
{
	float2 pos;
	float2 size;
	float4 color;
};

enum PlayerState
{
	OnFloor,
	OnPlatform,
	OnAir,
};

struct Game
{
	GameState state;

	f32 deltaSeconds;
	f32 accumulatedSeconds;

	GameInput input;

	Box box1;
	float2 speed;

	Box box2;
	float2 speed2;
	float accel2;
	PlayerState playerState;

	ID entId;

	AudioClipH sndJump;
	MusicH modEquinox;

	bool playingMusic;

	Camera camera;

	ID roomId;
};

////////////////////////////////////////////////////////////////////////
// Engine -> Game interface
////////////////////////////////////////////////////////////////////////

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
