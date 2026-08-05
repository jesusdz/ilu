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
#define ILU_ID_MAX_SLOTS U16_MAX // At 64 bits per slot, this is 512KB
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

union IDSlot
{
	void *object;
	u32 nextFree;
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

void InitializeIDPool();

ID NewID();
void ReserveID(ID id);
bool IsBuiltin(ID id);
bool Valid(ID id);
void Invalidate(ID id);
void SetObject(ID id, void *ptr);
void *GetObject(ID id);


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
	GetIDPool().slots[id.slot].object = object;
}

void *GetObject(ID id)
{
	void *object = GetIDPool().slots[id.slot].object;
	return object;
}

ID::operator bool() const { return Valid(*this); }

#endif // ILU_ID_IMPLEMENTATION

