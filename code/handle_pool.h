#ifndef HANDLE_POOL_H
#define HANDLE_POOL_H

////////////////////////////////////////////////////////////////////////
// Handle pool
//
// Stable ids for elements that live in a compact array. Two index spaces,
// never mix them:
//
//   handle.slot               -> slot in the sparse table. Stable for the whole
//                                life of the handle. Not an array index!
//   GetHandleIndex(pool, h)   -> index into the element array. Changes whenever
//                                the pool compacts.
//
// Removal happens at a single point in the frame. FreeHandle only marks: the handle
// stays valid and the element stays where it is for the rest of the frame, so
// everything referencing it goes on working and removing while iterating is safe.
// CompactPool is what removes: it calls removeElement for each element it drops
// (the place to release whatever the element owns), closes the gaps they leave in
// one stable sweep (relative order of the survivors is preserved), invalidates
// their handles, and calls moveElement for every element it displaced so the caller
// can move its element arrays along with it.
//
// So the array never has holes at any point a caller can observe, and a plain
// `for (u16 i = 0; i < HandleCount(pool); ++i)` is always a valid traversal.

union Handle
{
	struct {
	u16 gen;
	u16 slot;
	};
	u32 num;
};

constexpr Handle InvalidHandle = {};

constexpr u16 InvalidHandleSlot = 0xFFFF;

inline bool operator==(Handle a, Handle b)
{
	const bool equal = a.num == b.num;
	return equal;
}

inline bool operator!=(Handle a, Handle b)
{
	const bool nequal = a.num != b.num;
	return nequal;
}

// A distinct type per pool, so a MusicH cannot stand in for an AudioClipH. Conversion
// to and from the untyped Handle the pool trades in is implicit, which keeps the pool
// API usable without casts everywhere; C++ allows only one user-defined conversion in
// a sequence, so MusicH -> Handle -> AudioClipH is still rejected and passing a handle
// to the wrong accessor will not compile.
//
// `operator bool` is only declared here. Each type defines it next to the pool that
// issued it, since that is what holds the generation to check against: unlike a raw
// pointer, `if (handle)` asks whether the element is still alive, not just whether the
// handle was ever filled in.
#define DECLARE_HANDLE_TYPE(Name)                                                  \
struct Name                                                                        \
{                                                                                  \
	u16 gen;                                                                       \
	u16 slot;                                                                      \
                                                                                   \
	Name() = default;                                                              \
	Name(Handle handle) : gen(handle.gen), slot(handle.slot) {}                    \
	operator Handle() const { Handle h = {}; h.gen = gen; h.slot = slot; return h; } \
                                                                                   \
	explicit operator bool() const;                                                \
}

struct HandleSlot
{
	u16 gen;             // Odd while the slot is live, even while it is free
	u16 index;           // Index into the dense array while live, next free slot while free
	bool pendingRemoval; // Freed this frame, dropped by the next CompactPool
};

struct HandlePool
{
	HandleSlot *slots;       // Sparse: slot -> element index
	Handle *dense;           // Dense: element index -> handle (back reference, needed to compact)
	u16 count;               // Elements in the array
	u16 pendingRemovalCount; // Of those, how many are on their way out
	u16 freeSlot;            // Head of the free slot list
	u16 limit;
	bool initialized;
};

#define MOVE_ELEMENT(name) void name(u16 dstIndex, u16 srcIndex, void *data)
typedef MOVE_ELEMENT(MoveElementFunc);

#define REMOVE_ELEMENT(name) void name(u16 index, void *data)
typedef REMOVE_ELEMENT(RemoveElementFunc);

// A slot only holds a live element while its generation is odd. This is what
// keeps a zeroed or fabricated handle (InvalidHandle included, generation 0)
// from validating against a slot that was never handed out.
static bool IsLiveSlot(const HandleSlot &slot)
{
	const bool live = (slot.gen & 1) != 0;
	return live;
}

void Initialize(HandlePool &pool, Arena &arena, u16 limit)
{
	ASSERT(limit > 0 && limit < InvalidHandleSlot);

	pool.slots = PushArray(arena, HandleSlot, limit);
	pool.dense = PushArray(arena, Handle, limit);
	pool.limit = limit;
	pool.count = 0;
	pool.pendingRemovalCount = 0;
	pool.freeSlot = 0;

	for (u16 i = 0; i < limit; ++i)
	{
		pool.slots[i].gen = 0; // even: free
		pool.slots[i].index = (u16)(i + 1);
		pool.slots[i].pendingRemoval = false;
	}
	pool.slots[limit - 1].index = InvalidHandleSlot;

	pool.initialized = true;
}

// Every element in [0, HandleCount) is addressable this frame, the ones on their
// way out included
u16 HandleCount(const HandlePool &pool)
{
	return pool.count;
}

// Elements on their way out still take up room: capacity comes back at the next
// CompactPool, not at FreeHandle
bool IsFull(const HandlePool &pool)
{
	const bool full = pool.count >= pool.limit;
	return full;
}

bool IsValidHandle(const HandlePool &pool, Handle handle)
{
	if ( handle.slot >= pool.limit ) {
		return false;
	}
	const HandleSlot &slot = pool.slots[handle.slot];
	const bool valid = IsLiveSlot(slot) && slot.gen == handle.gen;
	return valid;
}

// Index of the element the handle refers to. Only valid until the next compaction.
u16 GetHandleIndex(const HandlePool &pool, Handle handle)
{
	ASSERT(IsValidHandle(pool, handle));
	const u16 index = pool.slots[handle.slot].index;
	return index;
}

Handle GetHandleAt(const HandlePool &pool, u16 index)
{
	ASSERT(index < pool.count);
	const Handle handle = pool.dense[index];
	return handle;
}

Handle NewHandle(HandlePool &pool)
{
	ASSERT(pool.initialized);
	ASSERT(!IsFull(pool));

	Handle handle = InvalidHandle;
	if ( !IsFull(pool) )
	{
		const u16 slotIndex = pool.freeSlot;
		ASSERT(slotIndex != InvalidHandleSlot);

		HandleSlot &slot = pool.slots[slotIndex];
		pool.freeSlot = slot.index;

		slot.gen++; // even -> odd, the slot is live now
		slot.index = pool.count;

		handle.gen = slot.gen;
		handle.slot = slotIndex;
		pool.dense[pool.count++] = handle;
	}
	return handle;
}

void FreeHandle(HandlePool &pool, Handle handle)
{
	ASSERT(IsValidHandle(pool, handle));

	if ( IsValidHandle(pool, handle) )
	{
		HandleSlot &slot = pool.slots[handle.slot];
		if ( !slot.pendingRemoval ) // freeing twice in the same frame removes one element
		{
			slot.pendingRemoval = true;
			pool.pendingRemovalCount++;
		}
	}
}

bool IsPendingRemoval(const HandlePool &pool, Handle handle)
{
	ASSERT(IsValidHandle(pool, handle));
	const bool pending = pool.slots[handle.slot].pendingRemoval;
	return pending;
}

u16 CompactPool(HandlePool &pool, MoveElementFunc *moveElement, RemoveElementFunc *removeElement, void *data)
{
	if ( pool.pendingRemovalCount == 0 ) {
		return 0;
	}

	u16 write = 0;
	for (u16 read = 0; read < pool.count; ++read)
	{
		const Handle handle = pool.dense[read];
		HandleSlot &slot = pool.slots[handle.slot];

		if ( slot.pendingRemoval )
		{
			if ( removeElement ) {
				removeElement(read, data);
			}

			// This is where the handle actually dies. The slot can only go back in
			// the free list now: recycling it earlier would have given two live
			// handles the same element.
			slot.pendingRemoval = false;
			slot.gen++; // odd -> even, the slot is free
			slot.index = pool.freeSlot;
			pool.freeSlot = handle.slot;
			continue;
		}

		if ( write != read )
		{
			pool.dense[write] = handle;
			slot.index = write;
			if ( moveElement ) {
				moveElement(write, read, data);
			}
		}
		write++;
	}

	const u16 removedCount = pool.count - write;
	pool.count = write;
	pool.pendingRemovalCount = 0;
	return removedCount;
}

#endif // HANDLE_POOL_H
