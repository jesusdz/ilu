#ifndef SCRIPT_H
#define SCRIPT_H

constexpr u32 MAX_SCRIPTS = 64;
constexpr u32 MAX_SCRIPT_PROPERTIES = 32;
constexpr u32 MAX_ENTITY_SCRIPTS = 4;
constexpr u32 MAX_PROPERTIES = MAX_SCRIPTS * MAX_SCRIPT_PROPERTIES;
constexpr u32 MAX_SCRIPT_INSTANCES = 1024;
constexpr u32 SCRIPT_INSTANCE_ALIGN = 16;
constexpr u32 SCRIPT_INSTANCE_DATA_SIZE = MAX_SCRIPT_INSTANCES * 128; // 128K

enum ScriptHookType
{
	ScriptHook_Start,
	ScriptHook_Simulate,
	ScriptHook_Update,
	ScriptHook_Stop,
	ScriptHook_Count,
};

typedef void (*ScriptHook)(void *instance);

struct ScriptPropertyDesc
{
	const char *name;
	PropertyValue value;
};

struct ScriptDesc
{
	ID entity; // Owner entity
	const char *name;
	u32 propertyCount;
	ScriptPropertyDesc properties[MAX_SCRIPT_PROPERTIES];
};

struct Script
{
	const char *name;
	u32 propertyFirst;
	u32 propertyCount;
	u32 instanceSize;
	ScriptHook hooks[ScriptHook_Count];
};

struct ScriptInstance
{
	ID entity; // Owner entity (invalid means remove the instance)
	const char *scriptName;
	u32 offset; // Offset into the data blob
	u32 size; // To compare against new hot-reloaded data
	u16 scriptIndex;
};

////////////////////////////////////////////////////////////////////////
// Binary data

#pragma pack(push, 1)

struct BinScriptPropertyDesc
{
	const char *name;
	PropertyType type;
	u32 value; // Raw view of PropertyValue, whichever member the type selects
};

struct BinScriptDesc
{
	ID entity;
	const char *name;
	BinLocation properties;
};

struct BinScript
{
	BinScriptDesc *desc;
	BinScriptPropertyDesc *properties;
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////
// Registration

#define SCRIPT_THUNK(StructName) \
	static void StructName##_Start(void *instance) { Start(*(StructName*)instance); } \
	static void StructName##_Simulate(void *instance) { Simulate(*(StructName*)instance); } \
	static void StructName##_Update(void *instance) { Update(*(StructName*)instance); } \
	static void StructName##_Stop(void *instance) { Stop(*(StructName*)instance); }

#define SCRIPT_BEGIN(StructName) \
	typedef StructName ScriptType; \
	Script &script = AllocateScript(game) = { \
		.name = #StructName, \
		.propertyFirst = game.propertyCount, \
		.instanceSize = AlignUp((u32)sizeof(StructName), SCRIPT_INSTANCE_ALIGN), \
		.hooks = { \
			StructName##_Start, \
			StructName##_Simulate, \
			StructName##_Update, \
			StructName##_Stop, \
		}, \
	}

#define PROPERTY(FieldType, Name) \
	AllocateProperty(game) = { \
		.type = Property_##FieldType, \
		.name = #Name, \
		.offset = OFFSET_OF(ScriptType, Name), \
	};

#define SCRIPT_END() { \
	script.propertyCount = game.propertyCount - script.propertyFirst; \
	ASSERT(script.propertyCount <= MAX_SCRIPT_PROPERTIES); \
}

#endif // SCRIPT_H
