#ifndef PROPERTY_H
#define PROPERTY_H

////////////////////////////////////////////////////////////////////////
// Reflected properties

#define ILU_STRUCT(...)
#define ILU_PROPERTY(...)
#define ILU_ENUM(...)

enum PropertyType : u8
{
	Property_U32,
	Property_Entity,
	Property_Sprite,
	Property_Texture,
	PropertyTypeCount,
};

constexpr const char *PropertyTypeName[] = {
	"U32",
	"Entity",
	"Sprite",
	"Texture",
};

CT_ASSERT(ARRAY_COUNT(PropertyTypeName) == PropertyTypeCount);

inline const char *PropertyTypeToString(PropertyType type)
{
	ASSERT(type < PropertyTypeCount);
	const char *str = PropertyTypeName[type];
	return str;
}

inline PropertyType StringToPropertyType(const char *str)
{
	for (u32 i = 0; i < PropertyTypeCount; ++i)
	{
		if (StrEq(PropertyTypeName[i], str)) {
			PropertyType res = (PropertyType)i;
			return res;
		}
	}
	return PropertyTypeCount;
}

inline PropertyType StringToPropertyType(const String str)
{
	for (u32 i = 0; i < PropertyTypeCount; ++i)
	{
		if (StrEq(str, PropertyTypeName[i])) {
			PropertyType res = (PropertyType)i;
			return res;
		}
	}
	return PropertyTypeCount;
}

struct PropertyValue
{
	PropertyType type;
	union
	{
		u32 uValue;
		ID idValue;
	};
};

// Which kind of property a reflected member is, if any. The C++ type is not
// enough: sprites, textures and entities are all IDs, and only the hint in the
// tag macro (e.g. ILU_PROPERTY(Sprite)) tells them apart.
inline PropertyType MemberPropertyType(const ReflexMember &member)
{
	if ( member.pointerCount > 0 || member.isArray ) {
		return PropertyTypeCount;
	}

	if ( member.hint ) {
		const PropertyType type = StringToPropertyType(member.hint);
		if ( type != PropertyTypeCount ) {
			return type;
		}
	}

	if ( member.reflexId == ReflexID_UnsignedInt ) {
		return Property_U32;
	}

	return PropertyTypeCount;
}

inline PropertyValue GetPropertyValue(const ReflexMember &member, const void *base)
{
	const PropertyType type = MemberPropertyType(member);

	const byte *field = (const byte *)base + member.offset;

	PropertyValue value = { .type = type };

	switch (type)
	{
		case Property_U32: value.uValue = *(const u32*)field; break;
		case Property_Entity:
		case Property_Sprite:
		case Property_Texture:
			value.idValue = *(const ID*)field; break;
		default:;
	}

	return value;
}

inline void SetPropertyValue(const ReflexMember &member, void *base, PropertyValue value)
{
	if ( value.type != MemberPropertyType(member) ) {
		return;
	}

	byte *field = (byte *)base + member.offset;

	switch (value.type)
	{
		case Property_U32: *(u32*)field = value.uValue; break;
		case Property_Entity:
		case Property_Sprite:
		case Property_Texture:
			*(ID*)field = value.idValue; break;
		default:;
	}
}

inline bool IsIDProperty(PropertyType type)
{
	const bool res = type >= Property_Entity && type <= Property_Texture;
	return res;
}

#endif // PROPERTY_H
