#ifndef PROPERTY_H
#define PROPERTY_H

////////////////////////////////////////////////////////////////////////
// Reflected properties

enum PropertyType : u8
{
	Property_U32,
	Property_ID,
	PropertyTypeCount,
};

constexpr const char *PropertyTypeName[] = {
	"U32",
	"ID",
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

struct Property
{
	PropertyType type;
	const char *name;
	u16 offset;
};

struct PropertyValue
{
	PropertyType type;
	union
	{
		u32 uValue;
		ID idValue;
	};
};

inline PropertyValue GetPropertyValue(const Property &property, const void *base)
{
	const byte *field = (const byte *)base + property.offset;

	PropertyValue value = { .type = property.type };

	switch (property.type)
	{
		case Property_U32: value.uValue = *(const u32*)field; break;
		case Property_ID: value.idValue = *(const ID*)field; break;
	}

	return value;
}

inline void SetPropertyValue(const Property &property, void *base, PropertyValue value)
{
	if ( value.type != property.type ) {
		return;
	}

	byte *field = (byte *)base + property.offset;

	switch (property.type)
	{
		case Property_U32: *(u32*)field = value.uValue; break;
		case Property_ID: *(ID*)field = value.idValue; break;
	}
}

#endif // PROPERTY_H
