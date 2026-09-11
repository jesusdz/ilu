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

#define PROPERTY_OPS(Type) static const ReflexOps PropertyOps_##Type = { .edit = EditProperty_##Type };
#else
#define PROPERTY_OPS(Type) static const ReflexOps PropertyOps_##Type = {};
#endif

PROPERTY_OPS(ID)
PROPERTY_OPS(u32)
PROPERTY_OPS(u8)
PROPERTY_OPS(f32)
PROPERTY_OPS(float3)
PROPERTY_OPS(Enum)

#undef PROPERTY_OPS

