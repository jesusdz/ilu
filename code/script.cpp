
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
			LOG(Warning, "RegisterScript: <%s> property <%s> of type <%s> cannot be stored\n", type->name, member.name, PropertyTypeToString(member.reflexId));
		} else if ( IsIDProperty(member.reflexId) && PropertyIDKind(member) == IDKind_None ) {
			LOG(Warning, "RegisterScript: <%s> property <%s> has hint <%s>, which names no ID kind (e.g. ILU_PROPERTY(Sprite)), so nothing can be assigned to it in the editor\n", type->name, member.name, member.hint ? member.hint : "");
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
	// From 1: slot 0 is the null script, which no name resolves to
	for (u32 i = 1; i < scriptRegistry.scriptCount; ++i)
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

void RegisterScripts(Engine &engine)
{
	// Reserved slot 0. For script components without a script assigned
	static const ReflexStruct nullScriptType = { .name = "None" };
	RegisterScript(&nullScriptType, nullptr, nullptr, nullptr, nullptr);

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
	for (u32 i = 0; i < scene.componentPools[ComponentType_Script].count; ++i)
	{
		ScriptComponent &component = scene.scriptComponents[i];

		if ( !component.name ) { // Added, but no script dropped on it yet
			continue;
		}

		const u32 structIndex = FindScriptIndex(component.name);
		if ( structIndex == U32_MAX ) {
			LOG(Warning, "Script <%s> is gone after the reload, its instance stops running.\n", component.name);
			component.structIndex = NULL_SCRIPT;
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

	ScriptComponent *component = (ScriptComponent*)AddComponentSlot(scene, entityId, ComponentType_Script);
	return component;
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
		ApplyDesc(*component, desc);
	}
	return component;
}

ScriptComponentDesc MakeDesc(const ScriptComponent &comp, ComponentDescPool &pool)
{
	ScriptComponentDesc desc = {};
	desc.name = comp.name;
	desc.properties = pool.properties + pool.propertyCount;

	const ReflexStruct *type = scriptRegistry.scripts[comp.structIndex].type;
	for (u32 p = 0; p < type->memberCount; ++p)
	{
		const ReflexMember &member = type->members[p];
		if ( IsStorableProperty(member) )
		{
			if ( pool.propertyCount == pool.propertyCapacity ) {
				LOG(Warning, "Script <%s> drops property <%s>, the property pool is full.\n", desc.name, member.name);
				break;
			}

			ScriptPropertyDesc &propertyDesc = pool.properties[pool.propertyCount++];
			propertyDesc.name = member.name;
			propertyDesc.value = GetPropertyValue(member, comp.data);
			desc.propertyCount++;
		}
	}

	return desc;
}

void ApplyDesc(ScriptComponent &comp, const ScriptComponentDesc &desc)
{
	const ReflexStruct *type = scriptRegistry.scripts[comp.structIndex].type;

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
			SetPropertyValue(*member, comp.data, propertyDesc.value);
		}
	}
}

void RemoveScript(Engine &engine, ID entityId)
{
	Scene &scene = engine.scene;

	if ( !HasComponents(scene, entityId, Component_Script) ) {
		return;
	}

	ScriptComponent &component = GetScript(scene, entityId);

	if ( engine.game.state == GameStateRunning ) {
		RunScriptHook(engine, entityId, component, ScriptHook_Stop);
	}

	FreeScriptData(engine, component.data, component.dataSize);

	RemoveComponentSlot(scene, entityId, ComponentType_Script);
}

void RunScriptHooks(Engine &engine, ScriptHookType hook)
{
	Scene &scene = engine.scene;
	for (u32 i = 0; i < scene.componentPools[ComponentType_Script].count; ++i)
	{
		const ID entityId = scene.scriptComponents[i].entityId;
		RunScriptHook(engine, entityId, scene.scriptComponents[i], hook);
	}
}

