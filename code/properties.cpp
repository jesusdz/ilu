#if USE_EDITOR

static const char *EditorObjectName(ID id)
{
	if ( !id ) {
		return "<none>";
	}

	switch ( GetIDKind(id) )
	{
		case IDKind_Entity:         return GetEntity(id).name;
		case IDKind_Texture:        return GetTexture(id).desc.name;
		case IDKind_Material:       return GetMaterial(id).desc.name;
		case IDKind_Sprite:         return GetSprite(id).desc.name;
		case IDKind_ParticleEffect: return GetParticleEffect(id).desc.name;
		case IDKind_Layer:          return GetLayer(id).name;
		case IDKind_Room:           return GetRoom(id).name;
		case IDKind_Prefab:         return GetPrefab(id).name;
		case IDKind_AudioClip:      return GetAudioClip(id).desc.name;
		case IDKind_MusicFile:      return GetMusicFile(id).desc.name;
		default:;
	}

	return "<unknown>";
}

static bool EditProperty_u32(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	return UI_InputUInt(ui, member.name, (u32*)field);
}

static bool EditProperty_u8(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	u32 value = *(u8*)field;
	const bool changed = UI_InputUInt(ui, member.name, &value);
	*(u8*)field = (u8)Min(value, 255u);
	return changed;
}

static bool EditProperty_f32(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	return UI_InputFloat(ui, member.name, (f32*)field);
}

static bool EditProperty_float3(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	return UI_InputFloat3(ui, member.name, (float3*)field);
}

// Today's body of EditorUpdateUI_Property, working on the field directly
static bool EditProperty_ID(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	ID &id = *(ID*)field;
	UI_Text(ui, member.name, "%s", EditorObjectName(id));

	const IDKind kind = PropertyIDKind(member);
	if ( kind != IDKind_None && UI_DragAndDropTarget(ui, IDKindNames[kind]) ) {
		id = { UI_DragAndDropPayload(ui).uvalue };
		return true;
	}
	return false;
}

// Reflex numbers enumerators from 0, so a value is also its index in the list
static bool EditProperty_Enum(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	const ReflexEnum *reflexEnum = ReflexGetEnum(member.reflexId);

	const char *names[64];
	ASSERT( reflexEnum->enumeratorCount <= ARRAY_COUNT(names) );
	for (u32 i = 0; i < reflexEnum->enumeratorCount; ++i) {
		names[i] = reflexEnum->enumerators[i].name;
	}

	i32 &value = *(i32*)field;
	u32 index = (u32)value;
		UI_Combo(ui, member.name, names, reflexEnum->enumeratorCount, &index);
	const bool changed = index != (u32)value;
	value = (i32)index;
	return changed;
}

#define PROPERTY_EDIT(Type) .edit = EditProperty_##Type,
#else
#define PROPERTY_EDIT(Type)
#endif // USE_EDITOR

#if USE_DATA_BUILD

static void WriteProperty_u32(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteText(ctx, "%u", *(const u32*)field);
}

static bool ParseProperty_u32(DParser &parser, const ReflexMember &member, void *field)
{
	*(u32*)field = DParser_ConsumeU32(parser);
	return true;
}

static void WriteProperty_u8(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteText(ctx, "%u", (u32)*(const u8*)field);
}

static bool ParseProperty_u8(DParser &parser, const ReflexMember &member, void *field)
{
	*(u8*)field = DParser_ConsumeU8(parser);
	return true;
}

static void WriteProperty_f32(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteText(ctx, "%f", *(const f32*)field);
}

static bool ParseProperty_f32(DParser &parser, const ReflexMember &member, void *field)
{
	const bool negative = DParser_TryConsume(parser, TOKEN_MINUS);
	const f32 value = DParser_ConsumeF32(parser);
	*(f32*)field = negative ? -value : value;
	return true;
}

static void WriteProperty_float3(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	const float3 &value = *(const float3*)field;
	WriteText(ctx, "{%f, %f, %f}", value.x, value.y, value.z);
}

static bool ParseProperty_float3(DParser &parser, const ReflexMember &member, void *field)
{
	*(float3*)field = DParser_ConsumeFloat3(parser);
	return true;
}

static void WriteProperty_ID(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteText(ctx, "%u", ((const ID*)field)->slot);
}

static bool ParseProperty_ID(DParser &parser, const ReflexMember &member, void *field)
{
	*(ID*)field = DParser_ConsumeID(parser);
	return true;
}

// Enums are written by enumerator name, so reordering an enum does not change saved data
static void WriteProperty_Enum(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	const ReflexEnum *reflexEnum = ReflexGetEnum(member.reflexId);
	const i32 value = *(const i32*)field;

	for (u32 i = 0; i < reflexEnum->enumeratorCount; ++i) {
		if ( reflexEnum->enumerators[i].value == value ) {
			WriteText(ctx, "%s", reflexEnum->enumerators[i].name);
			return;
		}
	}

	LOG(Warning, "Property <%s> holds %d, which is no <%s> enumerator.\n", member.name, value, reflexEnum->name);
	WriteText(ctx, "%s", reflexEnum->enumerators[0].name);
}

static bool ParseProperty_Enum(DParser &parser, const ReflexMember &member, void *field)
{
	const ReflexEnum *reflexEnum = ReflexGetEnum(member.reflexId);
	const String name = DParser_ConsumeLexeme(parser);

	for (u32 i = 0; i < reflexEnum->enumeratorCount; ++i) {
		if ( StrEq(name, reflexEnum->enumerators[i].name) ) {
			*(i32*)field = reflexEnum->enumerators[i].value;
			return true;
		}
	}
	return false;
}

#define PROPERTY_SERIALIZATION(Type) .write = WriteProperty_##Type, .parse = ParseProperty_##Type,
#else
#define PROPERTY_SERIALIZATION(Type)
#endif // USE_DATA_BUILD

#define PROPERTY_OPS(Type) static const ReflexOps PropertyOps_##Type = { PROPERTY_EDIT(Type) PROPERTY_SERIALIZATION(Type) };

PROPERTY_OPS(ID)
PROPERTY_OPS(u32)
PROPERTY_OPS(u8)
PROPERTY_OPS(f32)
PROPERTY_OPS(float3)
PROPERTY_OPS(Enum)

#undef PROPERTY_OPS
#undef PROPERTY_SERIALIZATION
#undef PROPERTY_EDIT

