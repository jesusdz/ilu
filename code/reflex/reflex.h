/*
 * reflex.h
 * Author: Jesus Diaz Garcia
 */

#ifndef TOOLS_REFLEX_H
#define TOOLS_REFLEX_H

#define REFLEX_MAX_STRUCTS 64
#define REFLEX_MAX_ENUMS 32

typedef u16 ReflexID;

enum // ReflexID
{
	// Trivial type IDs
	ReflexID_Void,
	ReflexID_Bool,
	ReflexID_Char,
	ReflexID_UnsignedChar,
	ReflexID_Int,
	ReflexID_ShortInt,
	ReflexID_LongInt,
	ReflexID_LongLongInt,
	ReflexID_UnsignedInt,
	ReflexID_UnsignedShortInt,
	ReflexID_UnsignedLongInt,
	ReflexID_UnsignedLongLongInt,
	ReflexID_Float,
	ReflexID_Double,
	// Trivial type IDs range
	ReflexID_TrivialCount,
	ReflexID_TrivialBegin = ReflexID_Void,
	ReflexID_TrivialEnd = ReflexID_TrivialBegin + ReflexID_TrivialCount,
	// Struct type IDs range
	ReflexID_StructCount = REFLEX_MAX_STRUCTS,
	ReflexID_StructBegin = ReflexID_TrivialEnd,
	ReflexID_StructEnd = ReflexID_StructBegin + ReflexID_StructCount,
	// Enum type IDs range
	ReflexID_EnumCount = REFLEX_MAX_ENUMS,
	ReflexID_EnumBegin = ReflexID_StructEnd,
	ReflexID_EnumEnd = ReflexID_EnumBegin + ReflexID_EnumCount,
};

struct ReflexTrivial
{
	u8 reflexId : 4;
	u8 size : 4;
};

struct ReflexEnumerator
{
	const char *name;
	i32 value;
};

struct ReflexEnum
{
	const char *name;
	const ReflexEnumerator *enumerators;
	u16 enumeratorCount;
};

struct ReflexMember
{
	const char *name;
	u16 isConst : 1;
	u16 pointerCount : 2;
	u16 isArray : 1;
	u16 arrayDim : 12; // 4096 values
	u16 reflexId;
	u16 offset;
};

struct ReflexStruct
{
	const char *name;
	const ReflexMember *members;
	u16 memberCount;
	u16 size;
};


static const ReflexStruct *gReflexStructs[REFLEX_MAX_STRUCTS] = {};
static const ReflexEnum *gReflexEnums[REFLEX_MAX_ENUMS] = {};


static bool ReflexIsTrivial(ReflexID id)
{
	const bool isTrivial = id >= ReflexID_TrivialBegin && id < ReflexID_TrivialEnd;
	return isTrivial;
}

static bool ReflexIsStruct(ReflexID id)
{
	const bool isStruct = id >= ReflexID_StructBegin && id < ReflexID_StructEnd;
	return isStruct;
}

static bool ReflexIsEnum(ReflexID id)
{
	const bool isEnum = id >= ReflexID_EnumBegin && id < ReflexID_EnumEnd;
	return isEnum;
}

static const ReflexStruct* ReflexGetStruct(ReflexID id)
{
	ASSERT(ReflexIsStruct(id));
	ReflexID index = id - ReflexID_StructBegin;
	const ReflexStruct *reflexStruct = gReflexStructs[index];
	return reflexStruct;
}

static const ReflexStruct* ReflexGetStructFromName(const char *name)
{
	const ReflexStruct **rstruct = gReflexStructs;
	const ReflexStruct **end = gReflexStructs + ReflexID_StructCount;
	while (*rstruct && rstruct != end && (*rstruct)->name) {
		if (StrEq((*rstruct)->name, name) ) {
			return *rstruct;
		}
		++rstruct;
	}
	return 0;
}

static const ReflexEnum* ReflexGetEnum(ReflexID id)
{
	ASSERT(ReflexIsEnum(id));
	ReflexID index = id - ReflexID_EnumBegin;
	const ReflexEnum *reflexEnum = gReflexEnums[index];
	return reflexEnum;
}

static ReflexID ReflexRegisterStruct(const ReflexStruct *reflexStruct)
{
	static ReflexID sReflexIdCounter = 0;
	ASSERT(sReflexIdCounter < ReflexID_StructCount);
	gReflexStructs[sReflexIdCounter] = reflexStruct;
	ReflexID reflexId = ReflexID_StructBegin + sReflexIdCounter++;
	return reflexId;
}

static ReflexID ReflexRegisterEnum(const ReflexEnum *reflexEnum)
{
	static ReflexID sReflexIdCounter = 0;
	ASSERT(sReflexIdCounter < ReflexID_EnumCount);
	gReflexEnums[sReflexIdCounter] = reflexEnum;
	ReflexID reflexId = ReflexID_EnumBegin + sReflexIdCounter++;
	return reflexId;
}

static i32 ReflexGetEnumValue(const ReflexEnum *reflexEnum, const char *enumeratorName)
{
	for (u32 i = 0; i < reflexEnum->enumeratorCount; ++i) {
		const ReflexEnumerator *enumerator = reflexEnum->enumerators + i;
		if (StrEq(enumerator->name, enumeratorName)) {
			return enumerator->value;
		}
	}
	return 0;
}

static const void *ReflexGetMemberPtr(const void *structBase, const ReflexMember *member)
{
	const void *memberPtr = (u8*)structBase + member->offset;
	return memberPtr;
}

static const ReflexTrivial* ReflexGetTrivial(ReflexID id)
{
	ASSERT(ReflexIsTrivial(id));
	static const ReflexTrivial trivials[] = {
		{ .reflexId = ReflexID_Void, .size = 0 },
		{ .reflexId = ReflexID_Bool, .size = sizeof(bool) },
		{ .reflexId = ReflexID_Char, .size = sizeof(char) },
		{ .reflexId = ReflexID_UnsignedChar, .size = sizeof(unsigned char) },
		{ .reflexId = ReflexID_Int, .size = sizeof(int) },
		{ .reflexId = ReflexID_ShortInt, .size = sizeof(short int) },
		{ .reflexId = ReflexID_LongInt, .size = sizeof(long int) },
		{ .reflexId = ReflexID_LongLongInt, .size = sizeof(long long int) },
		{ .reflexId = ReflexID_UnsignedInt, .size = sizeof(unsigned int) },
		{ .reflexId = ReflexID_UnsignedShortInt, .size = sizeof(unsigned short int) },
		{ .reflexId = ReflexID_UnsignedLongInt, .size = sizeof(unsigned long int) },
		{ .reflexId = ReflexID_UnsignedLongLongInt, .size = sizeof(unsigned long long int) },
		{ .reflexId = ReflexID_Float, .size = sizeof(float) },
		{ .reflexId = ReflexID_Double, .size = sizeof(double) },
	};
	CT_ASSERT(ARRAY_COUNT(trivials) == ReflexID_TrivialCount);
	return &trivials[id];
}

static u32 ReflexGetTypeSize(ReflexID id)
{
	if (ReflexIsTrivial(id))
	{
		const ReflexTrivial *trivial = ReflexGetTrivial(id);
		const u32 size = trivial->size;
		return size;
	}
	else if (ReflexIsStruct(id))
	{
		const ReflexStruct* rstruct = ReflexGetStruct(id);
		const u32 size = rstruct->size;
		return size;
	}
	else if (ReflexIsEnum(id))
	{
		// TODO: Enums can specify their base type which may vary its size
		return sizeof(int);
	}
	else
	{
		INVALID_CODE_PATH();
		return 0;
	}
}

static u32 ReflexGetElemCount( const void *data, const ReflexStruct *rstruct, const char *memberName )
{
	for (u32 i = 0; i < rstruct->memberCount; ++i)
	{
		const ReflexMember *member = &rstruct->members[i];
		const bool isPointer = member->pointerCount > 0;
		const u32 reflexId = member->reflexId;

		if ( !isPointer && reflexId == ReflexID_UnsignedInt )
		{
			const char *cursor = member->name; // current member name

			// NOTE: This solution is quite ad-hoc. We are searching for a member that's
			// called memberNameCount (e.g. for "textures" we look for "texturesCount").
			if ( ( cursor = StrConsume( cursor, memberName ) ) &&
					( cursor = StrConsume( cursor, "Count" ) ) && *cursor == 0 )
			{
				const void *memberPtr = (u8*)data + member->offset;
				const u32 count = *(u32*)memberPtr;
				return count;
			}
		}
	}
	return 0;
}

#endif // #ifndef TOOLS_REFLEX_H

