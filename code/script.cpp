
// Reflected type information lives in the reflex registry, generated from the
// tagged structs. This one only binds the hooks reflex cannot see to those types.
struct ScriptRegistry
{
	u32 scriptCount;
	Script scripts[MAX_SCRIPTS];
};

static ScriptRegistry scriptRegistry = {};

static void RegisterScript(const ReflexStruct *type, ScriptHook start, ScriptHook simulate, ScriptHook update, ScriptHook stop)
{
	if ( scriptRegistry.scriptCount == ARRAY_COUNT(scriptRegistry.scripts) ) {
		LOG(Warning, "RegisterScript: the script registry is full (%u), <%s> is dropped.\n", MAX_SCRIPTS, type->name);
		return;
	}

	// Every reflected member was tagged on purpose, so one the engine cannot
	// store is a mistake worth reporting instead of silently dropping it
	for (u32 i = 0; i < type->memberCount; ++i)
	{
		const ReflexMember &member = type->members[i];
		if ( !IsStorableProperty(member) ) {
			LOG(Warning, "RegisterScript: <%s> property <%s> of type <%s> cannot be stored, does its tag need a hint (e.g. ILU_PROPERTY(Sprite))?\n", type->name, member.name, PropertyTypeToString(member.reflexId));
		}
	}

	Script &script = scriptRegistry.scripts[scriptRegistry.scriptCount++];
	script = {
		.type = type,
		.hooks = { start, simulate, update, stop },
	};
}

static u32 FindScriptIndex(const char *name)
{
	for (u32 i = 0; i < scriptRegistry.scriptCount; ++i)
	{
		if ( StrEq( ScriptName(scriptRegistry.scripts[i]), name ) ) {
			return i;
		}
	}
	return U32_MAX;
}

static u32 ScriptSizeClass(u32 size)
{
	ASSERT( size > 0 && size % SCRIPT_DATA_ALIGN == 0 );
	return size / SCRIPT_DATA_ALIGN - 1;
}

// Hands back a zeroed block of exactly `size` bytes, reusing a freed one when the bucket
// has any. Blocks past the last bucket come straight from the arena and never return.
static byte *AllocScriptData(Engine &engine, u32 size)
{
	ScriptDataPool &pool = engine.scriptData;

	if ( size <= MAX_POOLED_SCRIPT_DATA_SIZE )
	{
		const u32 sizeClass = ScriptSizeClass(size);
		if ( ScriptDataBlock *block = pool.freeLists[sizeClass] )
		{
			pool.freeLists[sizeClass] = block->next;
			MemSet(block, size, 0);
			return (byte*)block;
		}
	}

	// PushSize is a plain bump, so the alignment the script data needs is taken here
	const u32 misalignment = (u32)( (u64)(pool.arena.base + pool.arena.used) % SCRIPT_DATA_ALIGN );
	if ( misalignment ) {
		PushSize(pool.arena, SCRIPT_DATA_ALIGN - misalignment);
	}

	return PushZeroSize(pool.arena, size);
}

// The freed block holds the link to the next one, so a bucket costs nothing but its head
static void FreeScriptData(Engine &engine, byte *data, u32 size)
{
	if ( !data || size > MAX_POOLED_SCRIPT_DATA_SIZE ) {
		return;
	}

	ScriptDataPool &pool = engine.scriptData;
	const u32 sizeClass = ScriptSizeClass(size);

	ScriptDataBlock *block = (ScriptDataBlock*)data;
	block->next = pool.freeLists[sizeClass];
	pool.freeLists[sizeClass] = block;
}

static void RunScriptHook(Engine &engine, ID entityId, ScriptComponent &component, ScriptHookType hook)
{
	if ( component.structIndex >= scriptRegistry.scriptCount ) {
		return;
	}

	const Script &script = scriptRegistry.scripts[component.structIndex];

	if ( ScriptHook hookFn = script.hooks[hook] )
	{
		Game &game = engine.game;
		const ID prevEntity = game.currentEntity;
		game.currentEntity = entityId;

		hookFn(component.data);

		game.currentEntity = prevEntity;
	}
}

// Writes the saved property values over the script data, skipping any that no longer
// match the script's reflected members.
static void ApplyScriptDesc(ScriptComponent &component, const ScriptComponentDesc &desc)
{
	if ( component.structIndex >= scriptRegistry.scriptCount ) {
		return;
	}

	const ReflexStruct *type = scriptRegistry.scripts[component.structIndex].type;

	for (u32 i = 0; i < desc.propertyCount; ++i)
	{
		const ScriptPropertyDesc &propertyDesc = desc.properties[i];

		const ReflexMember *member = nullptr;
		for (u32 p = 0; p < type->memberCount; ++p)
		{
			const ReflexMember &currMember = type->members[p];
			if ( StrEq( currMember.name, propertyDesc.name ) ) {
				member = &currMember;
				break;
			}
		}

		if ( !member ) {
			LOG(Warning, "Script <%s> has no property named <%s>, its saved value is dropped.\n", desc.name, propertyDesc.name);
		} else if ( member->reflexId != propertyDesc.value.type ) {
			LOG(Warning, "Script <%s> property <%s> changed type, its saved value is dropped.\n", desc.name, propertyDesc.name);
		} else {
			SetPropertyValue(*member, component.data, propertyDesc.value);
		}
	}
}

void RegisterScripts(Engine &engine)
{
	for (u32 i = ReflexID_StructBegin; i < ReflexID_StructEnd; ++i)
	{
		const ReflexStruct* rstruct = ReflexGetStruct(i);
		if (rstruct && rstruct->hint && StrEq(rstruct->hint, SCRIPT_HINT))
		{
			ScriptHook start = ReflexGetFunctor(rstruct->name, "Start");
			ScriptHook simulate = ReflexGetFunctor(rstruct->name, "Simulate");
			ScriptHook update = ReflexGetFunctor(rstruct->name, "Update");
			ScriptHook stop = ReflexGetFunctor(rstruct->name, "Stop");
			RegisterScript(rstruct, start, simulate, update, stop);
		}
	}

	// A reload rebuilds the registry, so every component re-resolves its index by name. A
	// struct whose size changed gets a fresh block, but one that only moved its members
	// around keeps its data and reads it back under the new layout.
	Scene &scene = engine.scene;
	for (u32 i = 0; i < scene.scriptComponentCount; ++i)
	{
		ScriptComponent &component = scene.scriptComponents[i];

		if ( !component.name ) { // Added, but no script dropped on it yet
			continue;
		}

		const u32 structIndex = FindScriptIndex(component.name);
		if ( structIndex == U32_MAX ) {
			LOG(Warning, "Script <%s> is gone after the reload, its instance stops running.\n", component.name);
			component.structIndex = U16_MAX;
			continue;
		}

		component.structIndex = (u16)structIndex;

		// The block is exactly the size the old struct was, so a struct that changed
		// across the reload needs a new one. Its contents cannot be carried over.
		const u32 dataSize = ScriptDataSize(scriptRegistry.scripts[structIndex]);
		if ( dataSize != component.dataSize )
		{
			LOG(Warning, "Script <%s> changed size across the reload, its instance is reset.\n", component.name);
			FreeScriptData(engine, component.data, component.dataSize);
			component.dataSize = dataSize;
			component.data = AllocScriptData(engine, dataSize);
		}
	}
}

u32 ScriptCount()
{
	return scriptRegistry.scriptCount;
}

const Script &GetScriptAt(u32 index)
{
	ASSERT( index < scriptRegistry.scriptCount );
	return scriptRegistry.scripts[index];
}

// Attaches a script component with nothing assigned to it yet. Hooks skip it and the
// inspector offers a drop target until AddScript points it at a script.
ScriptComponent *AddScript(Engine &engine, ID entityId)
{
	Scene &scene = engine.scene;

	if ( !Valid(entityId) ) {
		LOG(Warning, "AddScript: entity ID %u does not exist.\n", entityId.slot);
		return nullptr;
	}

	if ( HasComponents(scene, entityId, Component_Script) ) {
		LOG(Warning, "AddScript: the entity already has a script component.\n");
		return nullptr;
	}

	if ( scene.scriptComponentCount == MAX_SCRIPT_COMPONENTS ) {
		LOG(Warning, "AddScript: the script pool is full.\n");
		return nullptr;
	}

	const u16 entityIndex = GetEntityIndex(scene, entityId);
	const u16 slot = (u16)scene.scriptComponentCount++;
	scene.entityComponentIndex[entityIndex][ComponentType_Script] = slot;

	ScriptComponent &component = scene.scriptComponents[slot];
	component = {};
	component.entityId = entityId;
	component.structIndex = U16_MAX;

	return &component;
}

// Points the entity's script component at scriptName, adding the component if it has none
// and starting the instance over if it was already running something.
ScriptComponent *AddScript(Engine &engine, ID entityId, const char *scriptName)
{
	Scene &scene = engine.scene;

	if ( !Valid(entityId) ) {
		LOG(Warning, "AddScript: script <%s> refers to entity ID %u, which does not exist.\n", scriptName, entityId.slot);
		return nullptr;
	}

	const u32 structIndex = FindScriptIndex(scriptName);
	if ( structIndex == U32_MAX ) {
		LOG(Warning, "AddScript: no script named <%s> is registered.\n", scriptName);
		return nullptr;
	}

	if ( !HasComponents(scene, entityId, Component_Script) && !AddScript(engine, entityId) ) {
		return nullptr;
	}

	ScriptComponent &component = GetScript(scene, entityId);

	if ( engine.game.state == GameStateRunning ) {
		RunScriptHook(engine, entityId, component, ScriptHook_Stop);
	}

	FreeScriptData(engine, component.data, component.dataSize);

	const u32 dataSize = ScriptDataSize(scriptRegistry.scripts[structIndex]);

	component = {};
	component.entityId = entityId;
	component.name = InternString(scriptName);
	component.structIndex = (u16)structIndex;
	component.dataSize = dataSize;
	component.data = AllocScriptData(engine, dataSize);

	if ( engine.game.state == GameStateRunning ) {
		RunScriptHook(engine, entityId, component, ScriptHook_Start);
	}

	return &component;
}

ScriptComponent* AddScript(Engine &engine, ID entityId, const ScriptComponentDesc &desc)
{
	ScriptComponent *component = AddScript(engine, entityId, desc.name);
	if ( component ) {
		ApplyScriptDesc(*component, desc);
	}
	return component;
}

// Snapshots entityId's live script into outScript. False when it has none.
bool GatherEntityScriptDesc(const Scene &scene, ID entityId, ScriptComponentDesc &outScript, ComponentDescPool &pool)
{
	outScript = {};

	if ( !HasComponents(scene, entityId, Component_Script) ) {
		return false;
	}

	const ScriptComponent &component = GetScript(scene, entityId);
	if ( component.structIndex >= scriptRegistry.scriptCount ) {
		return false;
	}

	outScript.name = component.name;
	outScript.properties = pool.properties + pool.propertyCount;

	const ReflexStruct *type = scriptRegistry.scripts[component.structIndex].type;
	for (u32 p = 0; p < type->memberCount; ++p)
	{
		const ReflexMember &member = type->members[p];
		if ( !IsStorableProperty(member) ) {
			continue;
		}

		if ( pool.propertyCount == pool.propertyCapacity ) {
			LOG(Warning, "Script <%s> drops property <%s>, the property pool is full.\n",
					outScript.name, member.name);
			break;
		}

		ScriptPropertyDesc &propertyDesc = pool.properties[pool.propertyCount++];
		propertyDesc.name = member.name;
		propertyDesc.value = GetPropertyValue(member, component.data);
		outScript.propertyCount++;
	}

	return true;
}

void RemoveScript(Engine &engine, ID entityId)
{
	Scene &scene = engine.scene;

	if ( !HasComponents(scene, entityId, Component_Script) ) {
		return;
	}

	const u16 entityIndex = GetEntityIndex(scene, entityId);
	const u16 slot = scene.entityComponentIndex[entityIndex][ComponentType_Script];
	ScriptComponent &component = scene.scriptComponents[slot];

	if ( engine.game.state == GameStateRunning ) {
		RunScriptHook(engine, entityId, component, ScriptHook_Stop);
	}

	FreeScriptData(engine, component.data, component.dataSize);

	const u16 last = (u16)(--scene.scriptComponentCount);
	if ( slot != last )
	{
		scene.scriptComponents[slot] = scene.scriptComponents[last];
		scene.entityComponentIndex[ GetEntityIndex(scene, scene.scriptComponents[slot].entityId) ][ComponentType_Script] = slot;
	}

	scene.entityComponentIndex[entityIndex][ComponentType_Script] = NO_COMPONENT;
}

void RunScriptHooks(Engine &engine, ScriptHookType hook)
{
	Scene &scene = engine.scene;
	for (u32 i = 0; i < scene.scriptComponentCount; ++i)
	{
		const ID entityId = scene.scriptComponents[i].entityId;
		RunScriptHook(engine, entityId, scene.scriptComponents[i], hook);
	}
}

