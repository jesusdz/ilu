/*
 * ilu_array_pool.h
 * Author: Jesus Diaz Garcia
 *
 * A pool of arrays of one element type, recycled by element count: every count up to
 * ARRAY_POOL_SIZE_CLASS_COUNT has its own free list. Longer arrays come from the arena
 * too, but are never recycled.
 *
 * No include guard: include it once per pool type, defining these first:
 * - ARRAY_POOL_NAME              The pool struct to declare, e.g. PropertyDescPool
 * - ARRAY_POOL_TYPE              The element type, e.g. PropertyDesc
 * - ARRAY_POOL_SIZE_CLASS_COUNT  The longest array that gets recycled
 * All three are undefined again at the end of this file.
 *
 * Each pool also declares <ARRAY_POOL_TYPE>Array, e.g. PropertyDescArray, holding an array,
 * its count and its capacity, and gets its own overloads, picked by the pool's type:
 * - AllocArray        count elements, all in use
 * - AllocEmptyArray   room for capacity elements, none in use, filled with PushArrayElement
 * - PushArrayElement  the next unused element, or null once the array is full
 * - FreeArray         files the block under its capacity, blocks carry no header
 *
 * Requires ilu_core.h.
 */

#ifndef ARRAY_POOL_NAME
#error "ARRAY_POOL_NAME must be defined before including ilu_array_pool.h"
#endif
#ifndef ARRAY_POOL_TYPE
#error "ARRAY_POOL_TYPE must be defined before including ilu_array_pool.h"
#endif
#ifndef ARRAY_POOL_SIZE_CLASS_COUNT
#error "ARRAY_POOL_SIZE_CLASS_COUNT must be defined before including ilu_array_pool.h"
#endif

#ifndef ILU_ARRAY_POOL_H
#define ILU_ARRAY_POOL_H

// Struct used to store a linked list of free blocks
struct ArrayPoolBlock
{
	ArrayPoolBlock *next;
};

// ## only pastes inside a macro, and only after one more expansion does it paste the
// element type's name rather than the literal token ARRAY_POOL_TYPE
#define ARRAY_POOL_CONCAT_(a, b) a ## b
#define ARRAY_POOL_CONCAT(a, b) ARRAY_POOL_CONCAT_(a, b)

#endif // ILU_ARRAY_POOL_H

#define ARRAY_POOL_ARRAY ARRAY_POOL_CONCAT(ARRAY_POOL_TYPE, Array)

CT_ASSERT(sizeof(ARRAY_POOL_TYPE) >= sizeof(ArrayPoolBlock));

struct ARRAY_POOL_ARRAY
{
	ARRAY_POOL_TYPE *array;
	u32 count;
	u32 capacity;

	ARRAY_POOL_TYPE &operator[](u32 index) { ASSERT( index < count ); return array[index]; }
	const ARRAY_POOL_TYPE &operator[](u32 index) const { ASSERT( index < count ); return array[index]; }
};

struct ARRAY_POOL_NAME
{
	Arena *arena;
	ArrayPoolBlock *freeLists[ARRAY_POOL_SIZE_CLASS_COUNT];
};

static ARRAY_POOL_ARRAY AllocEmptyArray(ARRAY_POOL_NAME &pool, u32 capacity)
{
	ARRAY_POOL_ARRAY array = {};
	if ( capacity == 0 ) {
		return array;
	}

	array.capacity = capacity;

	if ( capacity <= ARRAY_POOL_SIZE_CLASS_COUNT )
	{
		const u32 sizeClass = capacity - 1;
		if ( ArrayPoolBlock *block = pool.freeLists[sizeClass] )
		{
			pool.freeLists[sizeClass] = block->next;
			MemSet(block, capacity * sizeof(ARRAY_POOL_TYPE), 0);
			array.array = (ARRAY_POOL_TYPE*) block;
			return array;
		}
	}

	array.array = PushZeroArray(*pool.arena, ARRAY_POOL_TYPE, capacity);
	return array;
}

static ARRAY_POOL_ARRAY AllocArray(ARRAY_POOL_NAME &pool, u32 count)
{
	ARRAY_POOL_ARRAY array = AllocEmptyArray(pool, count);
	array.count = count;
	return array;
}

static ARRAY_POOL_TYPE *PushArrayElement(ARRAY_POOL_ARRAY &array)
{
	if ( array.count == array.capacity ) {
		return nullptr;
	}
	return &array.array[array.count++];
}

static void FreeArray(ARRAY_POOL_NAME &pool, ARRAY_POOL_ARRAY array)
{
	// A block from another pool would join this one's free lists and be handed out again after
	// its own memory is gone. Transient pools are never freed into, their arena drops everything.
	ASSERT( !array.array || ( (byte*)array.array >= pool.arena->base && (byte*)array.array < pool.arena->base + pool.arena->size ) );

	if ( array.array && array.capacity > 0 && array.capacity <= ARRAY_POOL_SIZE_CLASS_COUNT )
	{
		ArrayPoolBlock *block = (ArrayPoolBlock*) array.array;
		block->next = pool.freeLists[array.capacity - 1];
		pool.freeLists[array.capacity - 1] = block;
	}
}

#undef ARRAY_POOL_ARRAY
#undef ARRAY_POOL_NAME
#undef ARRAY_POOL_TYPE
#undef ARRAY_POOL_SIZE_CLASS_COUNT
