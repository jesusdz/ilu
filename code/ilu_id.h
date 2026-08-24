/*
 * ilu_id.h
 * Author: Jesus Diaz Garcia
 *
 * Mandatory: define ILU_ID_POOL
 * - The IDPool must be provided externally by user code.
 * - To do that, define ILU_ID_POOL somewhere in your code.
 * - E.g.: #define ILU_ID_POOL sEngine->idPool
 * - Where `sEngine->idPool` is a field of type IDPool in your sEngine object.
 *
 * Optional: define ILU_ID_MAX_SLOTS
 * - Either way its U16_MAX by default.
 *
 * Optional: define ILU_ID_FIRST_DYNAMIC_SLOT
 * - Either way its 1000 by default.
 *
 * Slots are split in three ranges:
 * - 0                                      Invalid. No object ever lives here.
 * - [1, ILU_ID_FIRST_DYNAMIC_SLOT)         Builtin. User code picks these by hand, from
 *                                          a fixed enum, so a given builtin resource
 *                                          lands on the same slot every run. That is
 *                                          what lets saved data refer to one.
 * - [ILU_ID_FIRST_DYNAMIC_SLOT, MAX_SLOTS) Dynamic. Handed out by NewID, and persisted
 *                                          with whatever created them.
 */

#ifndef ILU_ID_H
#define ILU_ID_H

#ifndef ILU_ID_MAX_SLOTS
#define ILU_ID_MAX_SLOTS U16_MAX // At 8 bytes per slot, this is 512KB
#endif // ILU_ID_MAX_SLOTS

#ifndef ILU_ID_FIRST_DYNAMIC_SLOT
#define ILU_ID_FIRST_DYNAMIC_SLOT 1000
#endif // ILU_ID_FIRST_DYNAMIC_SLOT

// ID type is used to index IDPool Slots
struct ID
{
	u32 slot;

	operator bool() const;
};

struct IDSlot
{
	void *object;
};

struct IDPool
{
	u32 idCounter = 0;
	IDSlot slots[ILU_ID_MAX_SLOTS];
	u32 firstFree;
};

inline bool operator==(ID a, ID b)
{
	const bool res = a.slot == b.slot;
	return res;
}

inline bool operator!=(ID a, ID b)
{
	const bool res = a.slot != b.slot;
	return res;
}

void InitializeIDPool();

ID NewID();
void ReserveID(ID id);
bool IsBuiltin(ID id);
bool Valid(ID id);
void Invalidate(ID id);
void BindID(ID *idPtr, void *object);
void SetObject(ID id, void *object);
void *GetObject(ID id);


////////////////////////////////////////////////////////////////////////
// Array compaction

#define COMPACT_MOVE(name) void name(u32 dstIndex, u32 srcIndex, void *data)
typedef COMPACT_MOVE(CompactMoveFunc);

#define COMPACT_REMOVE(name) void name(u32 index, void *data)
typedef COMPACT_REMOVE(CompactRemoveFunc);

bool CompactArrayByID(void *elements, u32 elementSize, u32 &count, u32 idOffset,
		CompactMoveFunc *onMove, CompactRemoveFunc *onRemove, void *data);

// Spell the element type once instead of a sizeof and an offsetof at every call site
#define COMPACT_ARRAY_BY_ID(Type, array, count, idMember, onMove, onRemove, data) \
	CompactArrayByID(array, sizeof(Type), count, OFFSET_OF(Type, idMember), onMove, onRemove, data)


#endif // ILU_ID_H

#ifdef ILU_ID_IMPLEMENTATION

#ifdef ILU_ID_IMPLEMENTATION_INCLUDED
#error "ILU_ID_IMPLEMENTATION cannot be included twice"
#endif // ILU_ID_IMPLEMENTATION_INCLUDED
#define ILU_ID_IMPLEMENTATION_INCLUDED

#ifndef ILU_ID_POOL
#error "ILU_ID_POOL must get a reference to an IDPool object somewhere in code"
#endif // ILU_ID_POOL

static IDPool &GetIDPool()
{
	return ILU_ID_POOL;
}

void InitializeIDPool()
{
	// The builtin range is skipped entirely. Those slots belong to user code, which
	// assigns them by hand
	GetIDPool().idCounter = ILU_ID_FIRST_DYNAMIC_SLOT - 1;
	GetIDPool().firstFree = 0;
	ZeroArray(GetIDPool().slots);
}

ID NewID()
{
	ASSERT( GetIDPool().idCounter + 1 < ARRAY_COUNT(GetIDPool().slots) );
	const ID id = { .slot = ++GetIDPool().idCounter };
	return id;
}

// For IDs that come from outside the pool (loaded from an asset file, say). Bumps the
// counter past them so a later NewID cannot hand out a slot that is already spoken for.
void ReserveID(ID id)
{
	ASSERT( id.slot < ARRAY_COUNT(GetIDPool().slots) );

	// Builtin slots are fixed by user code and sit below the dynamic range, so the
	// counter never has to move for them
	if ( id.slot >= ILU_ID_FIRST_DYNAMIC_SLOT && GetIDPool().idCounter < id.slot ) {
		GetIDPool().idCounter = id.slot;
	}
}

// Points a slot at `object`, minting the ID when the caller has none. Objects loaded from
// an asset file come with an ID already, and keeping it is what makes references stored
// in saved data still resolve on the next run.
void BindID(ID *idPtr, void *object)
{
	ASSERT( idPtr );

	ID &id = *idPtr;

	if ( id.slot == 0 ) {
		id = NewID();
	} else {
		ReserveID(id);
	}

	GetIDPool().slots[id.slot].object = object;
}

bool IsBuiltin(ID id)
{
	const bool builtin = id.slot != 0 && id.slot < ILU_ID_FIRST_DYNAMIC_SLOT;
	return builtin;
}

bool Valid(ID id)
{
	const bool valid = GetIDPool().slots[id.slot].object != nullptr;
	return valid;
}

void Invalidate(ID id)
{
	GetIDPool().slots[id.slot].object = nullptr;
}

void SetObject(ID id, void *object)
{
	if ( id )
	{
		GetIDPool().slots[id.slot].object = object;
	}
}

void *GetObject(ID id)
{
	void *object = GetIDPool().slots[id.slot].object;
	return object;
}

ID::operator bool() const { return Valid(*this); }

static ID ElementID(const void *elements, u32 elementSize, u32 idOffset, u32 index)
{
	const ID id = *(const ID*)((const byte*)elements + index*elementSize + idOffset);
	return id;
}

bool CompactArrayByID(void *elements, u32 elementSize, u32 &count, u32 idOffset, CompactMoveFunc *onMove, CompactRemoveFunc *onRemove, void *data)
{
	byte *base = (byte*)elements;

	u32 storeIndex = U32_MAX;
	for (u32 i = 0; i < count; ++i)
	{
		if ( !ElementID(base, elementSize, idOffset, i) ) {
			storeIndex = i;
			break;
		}
	}

	if ( storeIndex == U32_MAX ) {
		return false;
	}

	for (u32 readIndex = storeIndex; readIndex < count; ++readIndex)
	{
		if ( !ElementID(base, elementSize, idOffset, readIndex) )
		{
			if ( onRemove ) {
				onRemove(readIndex, data);
			}
			continue;
		}

		// The sweep starts at a hole, so the write index trails the read index from
		// here on and a survivor never lands on an element still waiting to be visited
		const u32 writeIndex = storeIndex++;
		byte *writeElement = base + writeIndex*elementSize;
		MemCopy(writeElement, base + readIndex*elementSize, elementSize);

		if ( onMove ) {
			onMove(writeIndex, readIndex, data);
		}

		SetObject(ElementID(base, elementSize, idOffset, writeIndex), writeElement);
	}

	// Whatever the survivors left behind, so the tail reads as empty
	MemSet(base + storeIndex*elementSize, (count - storeIndex)*elementSize, 0);

	count = storeIndex;
	return true;
}

#endif // ILU_ID_IMPLEMENTATION

