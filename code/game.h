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

struct Game
{
	GameState state;
	GameInput input;

	f32 deltaSeconds;
	f32 accumulatedSeconds;

	u32 scriptInstanceCount;
	ScriptInstance scriptInstances[MAX_SCRIPT_INSTANCES];

	u32 scriptInstanceDataUsed;
	alignas(SCRIPT_INSTANCE_ALIGN) byte scriptInstanceData[SCRIPT_INSTANCE_DATA_SIZE];

	ID currentEntity;
};

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
