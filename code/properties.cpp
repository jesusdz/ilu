////////////////////////////////////////////////////////////////////////
// Numeric field access

// A machine type reaches REFLEX_OPS under every name C spells it with, so these
// work off the field's width and spare the ops one implementation per spelling

static i64 ReadSignedField(const void *field, u32 size)
{
	switch ( size )
	{
		case 1: return *(const i8*)field;
		case 2: return *(const i16*)field;
		case 4: return *(const i32*)field;
		case 8: return *(const i64*)field;
		default:;
	}
	INVALID_CODE_PATH();
	return 0;
}

static u64 ReadUnsignedField(const void *field, u32 size)
{
	switch ( size )
	{
		case 1: return *(const u8*)field;
		case 2: return *(const u16*)field;
		case 4: return *(const u32*)field;
		case 8: return *(const u64*)field;
		default:;
	}
	INVALID_CODE_PATH();
	return 0;
}

static f64 ReadFloatField(const void *field, u32 size)
{
	switch ( size )
	{
		case 4: return *(const f32*)field;
		case 8: return *(const f64*)field;
		default:;
	}
	INVALID_CODE_PATH();
	return 0.0;
}

static void WriteSignedField(void *field, u32 size, i64 value)
{
	switch ( size )
	{
		case 1: *(i8*)field = (i8)( value < I8_MIN ? I8_MIN : ( value > I8_MAX ? I8_MAX : value ) ); break;
		case 2: *(i16*)field = (i16)( value < I16_MIN ? I16_MIN : ( value > I16_MAX ? I16_MAX : value ) ); break;
		case 4: *(i32*)field = (i32)( value < I32_MIN ? I32_MIN : ( value > I32_MAX ? I32_MAX : value ) ); break;
		case 8: *(i64*)field = value; break;
		default: INVALID_CODE_PATH();
	}
}

static void WriteUnsignedField(void *field, u32 size, u64 value)
{
	switch ( size )
	{
		case 1: *(u8*)field = (u8)( value > U8_MAX ? U8_MAX : value ); break;
		case 2: *(u16*)field = (u16)( value > U16_MAX ? U16_MAX : value ); break;
		case 4: *(u32*)field = (u32)( value > U32_MAX ? U32_MAX : value ); break;
		case 8: *(u64*)field = value; break;
		default: INVALID_CODE_PATH();
	}
}

static void WriteFloatField(void *field, u32 size, f64 value)
{
	switch ( size )
	{
		case 4: *(f32*)field = (f32)value; break;
		case 8: *(f64*)field = value; break;
		default: INVALID_CODE_PATH();
	}
}

#if USE_EDITOR

static const char *EditorObjectName(ID id)
{
	if ( !id ) {
		return "<none>";
	}

	switch ( GetIDType(id) )
	{
		case ReflexID_Entity:         return GetEntity(id).name;
		case ReflexID_Texture:        return GetTexture(id).desc.name;
		case ReflexID_Material:       return GetMaterial(id).desc.name;
		case ReflexID_Sprite:         return GetSprite(id).desc.name;
		case ReflexID_ParticleEffect: return GetParticleEffect(id).desc.name;
		case ReflexID_Layer:          return GetLayer(id).name;
		case ReflexID_Room:           return GetRoom(id).name;
		case ReflexID_Prefab:         return GetPrefab(id).name;
		case ReflexID_AudioClip:      return GetAudioClip(id).desc.name;
		case ReflexID_MusicFile:      return GetMusicFile(id).desc.name;
		default:;
	}

	return "<unknown>";
}

static bool HasFlag(const ReflexMember &member, ReflexMetaFlags flag)
{
	return (member.meta.flags & flag) != 0;
}

static bool EditProperty_Bool(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	return UI_Checkbox(ui, member.name, (bool*)field);
}

static bool EditProperty_Int(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	const u32 size = ReflexGetTypeSize(member.reflexId);

	i64 value = ReadSignedField(field, size);
	const bool changed = UI_InputI64(ui, member.name, &value);
	if ( changed ) {
		WriteSignedField(field, size, value);
	}
	return changed;
}

static bool EditProperty_UInt(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	const u32 size = ReflexGetTypeSize(member.reflexId);

	if ( HasFlag(member, ReflexMetaFlag_Bool) ) {
		bool value = ReadUnsignedField(field, size) != 0;
		const bool changed = UI_Checkbox(ui, member.name, &value);
		WriteUnsignedField(field, size, value ? 1 : 0);
		return changed;
	}

	// UI_InputI64 has no way to carry a value past I64_MAX, so one that big shows up
	// wrapped and is written back only once the field is really edited
	i64 value = (i64)ReadUnsignedField(field, size);
	const bool changed = UI_InputI64(ui, member.name, &value);
	if ( changed ) {
		WriteUnsignedField(field, size, value < 0 ? 0 : (u64)value);
	}
	return changed;
}

static bool EditProperty_Float(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	const u32 size = ReflexGetTypeSize(member.reflexId);

	f32 value = (f32)ReadFloatField(field, size);
	const bool changed = UI_InputFloat(ui, member.name, &value);
	if ( changed ) {
		WriteFloatField(field, size, value);
	}
	return changed;
}

static bool EditProperty_int2(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	return UI_InputInt2(ui, member.name, (int2*)field);
}

static bool EditProperty_uint2(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	return UI_InputUInt2(ui, member.name, (uint2*)field);
}

static bool EditProperty_float3(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;

	if ( HasFlag(member, ReflexMetaFlag_Color) )
	{
		// The picker edits a copy until it is closed, and only one can be open, so the field
		// it belongs to is what tells the open picker apart from every other color row
		static const void *openField = nullptr;
		static float4 color = {};

		float3 &value = *(float3*)field;

		if ( UI_ColorButton(ui, member.name, Float4(value, 1.0f)) ) {
			openField = field;
			color = Float4(value, 1.0f);
		}

		if ( openField != field ) {
			return false;
		}

		bool isOpen = true;
		UI_ColorPicker(ui, &color, &isOpen);
		const bool changed = value.x != color.x || value.y != color.y || value.z != color.z;
		value = color.xyz;
		if ( !isOpen ) {
			openField = nullptr;
		}
		return changed;
	}

	return UI_InputFloat3(ui, member.name, (float3*)field);
}

static bool EditProperty_CString(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	const char *value = *(const char **)field;
	UI_Text(ui, member.name, "%s", value ? value : "");
	return false;
}

// Today's body of EditorUpdateUI_Property, working on the field directly
static bool EditProperty_ID(const ReflexMember &member, void *field)
{
	UI &ui = GetEngine().ui;
	ID &id = *(ID*)field;
	UI_Text(ui, member.name, "%s", EditorObjectName(id));

	const ReflexID type = PropertyIDType(member);
	if ( type != ReflexID_Null && UI_DragAndDropTarget(ui, ReflexGetTypeName(type)) ) {
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

static void WriteProperty_Bool(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteText(ctx, "%u", *(const bool*)field ? 1u : 0u);
}

// The lexer holds no sign, so a negative value arrives as a minus token of its own
static bool ParseProperty_Bool(DParser &parser, const ReflexMember &member, void *field)
{
	const bool negative = DParser_TryConsume(parser, TOKEN_MINUS);
	const i64 value = StrToI64(DParser_ConsumeLexeme(parser));
	*(bool*)field = !negative && value != 0;
	return true;
}

static void WriteProperty_Int(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteText(ctx, "%lld", ReadSignedField(field, ReflexGetTypeSize(member.reflexId)));
}

static bool ParseProperty_Int(DParser &parser, const ReflexMember &member, void *field)
{
	const bool negative = DParser_TryConsume(parser, TOKEN_MINUS);
	const i64 value = StrToI64(DParser_ConsumeLexeme(parser));
	WriteSignedField(field, ReflexGetTypeSize(member.reflexId), negative ? -value : value);
	return true;
}

static void WriteProperty_UInt(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteText(ctx, "%llu", ReadUnsignedField(field, ReflexGetTypeSize(member.reflexId)));
}

// An unsigned field has no negative to hold, so it takes the minus and reads zero
static bool ParseProperty_UInt(DParser &parser, const ReflexMember &member, void *field)
{
	const bool negative = DParser_TryConsume(parser, TOKEN_MINUS);
	const i64 value = StrToI64(DParser_ConsumeLexeme(parser));
	WriteUnsignedField(field, ReflexGetTypeSize(member.reflexId), negative || value < 0 ? 0 : (u64)value);
	return true;
}

static void WriteProperty_Float(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteText(ctx, "%f", ReadFloatField(field, ReflexGetTypeSize(member.reflexId)));
}

// StrToFloat is the only float parser there is, so a double round trips through f32 precision
static bool ParseProperty_Float(DParser &parser, const ReflexMember &member, void *field)
{
	const bool negative = DParser_TryConsume(parser, TOKEN_MINUS);
	const f32 value = StrToFloat(DParser_ConsumeLexeme(parser));
	WriteFloatField(field, ReflexGetTypeSize(member.reflexId), negative ? -value : value);
	return true;
}

static void WriteProperty_CString(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	const char *value = *(const char *const *)field;
	WriteText(ctx, "\"%s\"", value ? value : "");
}

static bool ParseProperty_CString(DParser &parser, const ReflexMember &member, void *field)
{
	if ( !DParser_IsNextToken(parser, TOKEN_STRING) ) {
		DParser_Consume(parser);
		return false;
	}
	*(const char **)field = PushString(*parser.arena, DParser_ConsumeString(parser));
	return true;
}

static void WriteProperty_int2(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	const int2 &value = *(const int2*)field;
	WriteText(ctx, "{%d, %d}", value.x, value.y);
}

static bool ParseProperty_int2(DParser &parser, const ReflexMember &member, void *field)
{
	*(int2*)field = DParser_ConsumeInt2(parser);
	return true;
}

static void WriteProperty_uint2(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	const uint2 &value = *(const uint2*)field;
	WriteText(ctx, "{%u, %u}", value.x, value.y);
}

static bool ParseProperty_uint2(DParser &parser, const ReflexMember &member, void *field)
{
	*(uint2*)field = DParser_ConsumeUint2(parser);
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

#define PROPERTY_OPS_AS(Name, Type) static const ReflexOps PropertyOps_##Name = { PROPERTY_EDIT(Type) PROPERTY_SERIALIZATION(Type) };
#define PROPERTY_OPS(Type) PROPERTY_OPS_AS(Type, Type)

PROPERTY_OPS(Bool)
PROPERTY_OPS(Int)
PROPERTY_OPS(UInt)
PROPERTY_OPS(Float)
PROPERTY_OPS(CString)
PROPERTY_OPS(ID)
PROPERTY_OPS(int2)
PROPERTY_OPS(uint2)
PROPERTY_OPS(float3)
PROPERTY_OPS(Enum)

// Aliases
PROPERTY_OPS_AS(Char, Int)
PROPERTY_OPS_AS(ShortInt, Int)
PROPERTY_OPS_AS(LongInt, Int)
PROPERTY_OPS_AS(LongLongInt, Int)
PROPERTY_OPS_AS(i8, Int)
PROPERTY_OPS_AS(i16, Int)
PROPERTY_OPS_AS(i32, Int)
PROPERTY_OPS_AS(i64, Int)
PROPERTY_OPS_AS(UnsignedChar, UInt)
PROPERTY_OPS_AS(UnsignedShortInt, UInt)
PROPERTY_OPS_AS(UnsignedInt, UInt)
PROPERTY_OPS_AS(UnsignedLongInt, UInt)
PROPERTY_OPS_AS(UnsignedLongLongInt, UInt)
PROPERTY_OPS_AS(u8, UInt)
PROPERTY_OPS_AS(u16, UInt)
PROPERTY_OPS_AS(u32, UInt)
PROPERTY_OPS_AS(u64, UInt)
PROPERTY_OPS_AS(Double, Float)
PROPERTY_OPS_AS(f32, Float)
PROPERTY_OPS_AS(f64, Float)

#undef PROPERTY_OPS
#undef PROPERTY_OPS_AS
#undef PROPERTY_SERIALIZATION
#undef PROPERTY_EDIT
