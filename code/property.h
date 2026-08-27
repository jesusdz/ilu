#ifndef PROPERTY_H
#define PROPERTY_H

////////////////////////////////////////////////////////////////////////
// Reflected properties

#define ILU_STRUCT(...)
#define ILU_PROPERTY(...)
#define ILU_ENUM(...)

// List of custom types for reflex
#define REFLEX_ID_CUSTOM_TYPES \
	ReflexID_IDBegin, \
	ReflexID_IDEntity, \
	ReflexID_IDTexture, \
	ReflexID_IDSprite, \
	ReflexID_IDSoundClip, \
	ReflexID_IDMusicFile, \
	ReflexID_IDEnd, \
	ReflexID_u32,

#include "reflex\reflex.h"

typedef ReflexID PropertyType;

typedef ID IDEntity;
typedef ID IDTexture;
typedef ID IDSprite;
typedef ID IDSoundClip;
typedef ID IDMusicFile;

struct PropertyValue
{
	PropertyType type;
	union
	{
		u32 uValue;
		ID idValue;
	};
};

inline bool IsIDProperty(PropertyType type)
{
	const bool res = type >= ReflexID_IDBegin && type <= ReflexID_IDEnd;
	return res;
}

// Whether a reflected member holds a value the engine knows how to read, write
// and serialize. Pointers, arrays and types outside the property set are not
// storable, and every reflected member was tagged on purpose, so callers report
// these instead of silently dropping them.
inline bool IsStorableProperty(const ReflexMember &member)
{
	if ( member.pointerCount > 0 || member.isArray ) {
		return false;
	}

	const bool res = member.reflexId == ReflexID_u32 || IsIDProperty(member.reflexId);
	return res;
}

inline const char *PropertyTypeToString(PropertyType type)
{
	const char *str = ReflexGetTypeName(type);
	return str;
}

inline PropertyType StringToPropertyType(const char *str)
{
	const PropertyType type = ReflexGetTypeFromName(str);
	return type;
}

inline PropertyType StringToPropertyType(String str)
{
	char buffer[128];
	StrCopy(buffer, str);
	const PropertyType type = ReflexGetTypeFromName(buffer);
	return type;
}

inline PropertyValue GetPropertyValue(const ReflexMember &member, const void *base)
{
	if ( !IsStorableProperty(member) ) {
		const PropertyValue none = { .type = ReflexID_Null };
		return none;
	}

	const byte *field = (const byte *)base + member.offset;

	PropertyValue value = { .type = member.reflexId };

	if (member.reflexId == ReflexID_u32) {
		value.uValue = *(const u32*)field;
	} else if (IsIDProperty(member.reflexId)) {
		value.idValue = *(const ID*)field;
	}

	return value;
}

inline void SetPropertyValue(const ReflexMember &member, void *base, PropertyValue value)
{
	if ( !IsStorableProperty(member) || value.type != member.reflexId ) {
		return;
	}

	byte *field = (byte *)base + member.offset;

	if (member.reflexId == ReflexID_u32) {
		*(u32*)field = value.uValue;
	} else if (IsIDProperty(member.reflexId)) {
		*(ID*)field = value.idValue;
	}
}

#endif // PROPERTY_H
