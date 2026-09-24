#include "../ilu_core.h"
#include "reflex.h"
#include "../../assets/assets.h"
#include <cstddef> // offsetof

////////////////////////////////////////////////////////////////////////
// Print utils

#define Printf( format, ... ) LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ );
#define PrintBeginScope( format, ... ) LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ ); IndentationIncrease();
#define PrintEndScope( format, ... ) IndentationDecrease(); LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ );
#define PrintNewLine() LOG(Info, "\n"); indent.apply = true;

struct IndentationState
{
	bool apply;
	u32 pos;
	char padding[64];
};

static IndentationState indent = {
	.apply = true,
	.pos = 0,
	.padding = "",
};

static void IndentationIncrease()
{
	indent.padding[indent.pos++] = '\t';
}

static void IndentationDecrease()
{
	ASSERT(indent.pos > 0);
	indent.padding[--indent.pos] = '\0';
}

static const char *Indentation()
{
	const char *padding = indent.apply ? indent.padding : "";
	indent.apply = false;
	return padding;
}


////////////////////////////////////////////////////////////////////////
// Automatic implementation (using reflection)

#define REFLEX_ID(TypeName) ReflexID_ ## TypeName

#include "assets.reflex.h"

void PrintTrivial(const void *data, const ReflexID id)
{
	const ReflexTrivial *trivial = ReflexGetTrivial(id);

	if (id == ReflexID_Bool) {
		const bool val = *((bool*)data);
		Printf("%d", val ? 1 : 0);
	} else if (id == ReflexID_CString) {
		const char *val = *((const char **)data);
		Printf("\"%s\"", val);
	} else if (id == ReflexID_Char) {
		const char val = *((char*)data);
		Printf("%c", val);
	} else if (id == ReflexID_Float) {
		const float val = *((float*)data);
		Printf("%f", val);
	} else if (id == ReflexID_Double) {
		const double val = *((double*)data);
		Printf("%lf", val);
	} else if (id == ReflexID_Int) {
		const int val = *((int*)data);
		Printf("%d", val);
	} else if (id == ReflexID_ShortInt) {
		const short int val = *((int*)data);
		Printf("%hd", val);
	} else if (id == ReflexID_LongInt) {
		const long int val = *((int*)data);
		Printf("%ld", val);
	} else if (id == ReflexID_LongLongInt) {
		const long long int val = *((int*)data);
		Printf("%lld", val);
	} else if (id == ReflexID_UnsignedInt) {
		const unsigned int val = *((unsigned int*)data);
		Printf("%u", val);
	} else if (id == ReflexID_UnsignedShortInt) {
		const unsigned short int val = *((unsigned int*)data);
		Printf("%hu", val);
	} else if (id == ReflexID_UnsignedLongInt) {
		const unsigned long int val = *((unsigned int*)data);
		Printf("%lu", val);
	} else if (id == ReflexID_UnsignedLongLongInt) {
		const unsigned long long int val = *((unsigned int*)data);
		Printf("%llu", val);
	} else {
		INVALID_CODE_PATH();
	}
}

void PrintEnum(const void *data, const ReflexID id)
{
	//const ReflexEnum *renum = ReflexGetEnum(id);
	const void *enumPtr = data;
	const int val = *((int*)data);
	Printf("%d", val);
}

void PrintStruct(const void *data, const ReflexID id)
{
	const ReflexStruct *rstruct = ReflexGetStruct(id);
	const void *structPtr = data;

	PrintBeginScope("{");
	PrintNewLine();

	for (u32 i = 0; i < rstruct->memberCount; ++i)
	{
		const ReflexMember *member = &rstruct->members[i];
		const ReflexID reflexId = member->reflexId;
		const bool isTrivial = ReflexIsTrivial(reflexId);
		const bool isStruct = ReflexIsStruct(reflexId);
		const bool isEnum = ReflexIsEnum(reflexId);
		const bool isArray = member->isArray;
		const bool isPointer = member->pointerCount > 0;
		const bool isDoublePointer = member->pointerCount > 1;
		const u16 pointerCount = member->pointerCount;
		const u16 arrayDim = member->arrayDim;

		Printf("\"%s\" : ", member->name);

		const u32 typeSize = ReflexGetTypeSize(reflexId);
		const u32 elemSize =
			isArray ? (isPointer ? sizeof(void*) : typeSize):
			isPointer? (isDoublePointer ? sizeof(void*) : typeSize):
			typeSize;

		const u32 elemCount =
			isArray ? arrayDim :
			isPointer ? ReflexGetElemCount(structPtr, rstruct, member->name) :
			0;

		if ( elemCount > 0 ) {
			PrintBeginScope("[");
			if (!isTrivial) {
				PrintNewLine();
			}
		}

		const void *memberPtr = ReflexGetMemberPtr(structPtr, member);

		u32 indirections = pointerCount;

		// If the member is a pointed array (no fixed length), we dereference the pointer
		// to get the base address of the pointed array. For fixed length arrays (isArray)
		// there's no need for dereferencing, memberPtr already points to the base address.
		if ( !isArray && isPointer ) {
			memberPtr = *(void**)memberPtr;
			indirections--;
		}

		for (u32 elemIndex = 0; elemIndex == 0 || elemIndex < elemCount; ++elemIndex)
		{
			const void *elemPtr = (u8*)memberPtr + elemIndex * elemSize;

			// We apply the remaining indirections until getting the value address.
			for (u32 i = 0; i < indirections; ++i) {
				elemPtr = *(void**)elemPtr;
			}

			if (isTrivial) {
				PrintTrivial(elemPtr, reflexId);
			} else if (isEnum) {
				PrintEnum(elemPtr, reflexId);
			} else if (isStruct) {
				PrintStruct(elemPtr, reflexId);
			} else {
				Printf("<?>");
			}

			if (elemCount > 0 && elemIndex + 1 < elemCount) {
				Printf(",");
				if (isTrivial) {
					Printf(" ");
				} else {
					PrintNewLine();
				}
			}
		}

		if ( elemCount > 0 ) {
			if (!isTrivial) {
				PrintNewLine();
			}
			PrintEndScope("]");
		}

		if ( i + 1 < rstruct->memberCount ) {
			Printf(",");
		}
		PrintNewLine();

	}

	PrintEndScope("}");
}


////////////////////////////////////////////////////////////////////////
//  Main function

int main(int argc, char **argv)
{
	PrintStruct(&gAssets, REFLEX_ID(Assets));

	return 0;
}

