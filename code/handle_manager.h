#ifndef HANDLE_MANAGER_H
#define HANDLE_MANAGER_H

union Handle
{
	struct {
	u16 gen;
	u16 idx;
	};
	u32 num;
};

constexpr Handle InvalidHandle = {};

#define CHECK_HANDLE(name) bool name(Handle, const char *, void *)
typedef CHECK_HANDLE(CheckHandle);

#define PROCESS_HANDLE(name) void name(Handle, void *)
typedef PROCESS_HANDLE(ProcessHandleFunc);

struct HandleFinder
{
	CheckHandle *checkHandle;
	const char *name;
	void *data;
};

struct HandleManager
{
	Handle *handles;
	u16 *indices; // First all used indices, then all free indices
	u16 handleCount;
	u16 handleLimit;
	bool initialized;
};

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

void FreeAllHandles(HandleManager &manager)
{
	manager.handleCount = 0;

	for (u32 i = 0; i < manager.handleLimit; ++i)
	{
		manager.indices[i] = i;
		manager.handles[i].idx = i;
		manager.handles[i].gen++;
	}
}

void Initialize(HandleManager &manager, Arena &arena, u16 handleLimit)
{
	manager.handles = PushArray(arena, Handle, handleLimit);
	manager.indices = PushArray(arena, u16, handleLimit);
	manager.handleLimit = handleLimit;
	manager.handleCount = 0;

	for (u32 i = 0; i < manager.handleLimit; ++i)
	{
		manager.indices[i] = i;
		manager.handles[i].idx = i;
		manager.handles[i].gen = 1;
	}

	manager.initialized = true;
}

bool IsValidHandle(const HandleManager &manager, Handle handle)
{
	// Bounds check first: a zeroed or garbage handle can carry any slot index
	if ( handle.idx >= manager.handleLimit ) {
		return false;
	}
	Handle storedHandle = manager.handles[handle.idx];
	const bool valid = handle == storedHandle;
	return valid;
}

Handle NewHandle(HandleManager &manager)
{
	ASSERT(manager.initialized);
	ASSERT(manager.handleCount < manager.handleLimit);

	Handle handle = InvalidHandle;
	if ( manager.handleCount < manager.handleLimit ) {
		u16 index = manager.indices[manager.handleCount++];
		handle = manager.handles[index];
		ASSERT(handle.idx == index);
	}
	return handle;
}

void FreeHandle(HandleManager &manager, Handle handle)
{
	ASSERT(IsValidHandle(manager, handle));
	ASSERT(manager.handleCount > 0);

	if ( IsValidHandle(manager, handle) && manager.handleCount > 0 )
	{
		manager.handles[handle.idx].gen++;

		manager.handleCount--;
		bool found = false;
		for (u16 i = 0; i < manager.handleCount; ++i) {
			found = found || handle.idx == manager.indices[i];
			if (found) { // compact indices from found index onwards
				manager.indices[i] = manager.indices[i+1];
			}
		}
		// Insert the freed index at the end
		manager.indices[manager.handleCount] = handle.idx;
	}
}

Handle GetHandleAt(const HandleManager &manager, u16 index)
{
	u16 handleIndex = manager.indices[index];
	const Handle handle = manager.handles[handleIndex];
	return handle;
}

Handle FindHandle(const HandleManager &manager, const HandleFinder &finder)
{
	for (u16 i = 0; i < manager.handleCount; ++i)
	{
		u16 index = manager.indices[i];
		const Handle handle = manager.handles[index];
		if ( finder.checkHandle(handle, finder.name, finder.data) )
		{
			return handle;
		}
	}
	LOG(Warning, "Could not find handle <%s>.\n", finder.name);
	return InvalidHandle;
}

void ForAllHandles(const HandleManager &manager, ProcessHandleFunc *processHandle, void *data)
{
	// In reverse order we don't have to worry if FreeHandle is called by processHandle
	u16 handleCount = manager.handleCount;
	for (i32 i = handleCount - 1; i >= 0; --i)
	{
		i16 index = manager.indices[i];
		processHandle(manager.handles[index], data);
	}
}


// Iterator ////////////////////////////////////////////////////////////

struct HandleIter
{
	const HandleManager *manager;
	u16 elementIndex;

	Handle operator*() {
		Handle handle = InvalidHandle;
		if (elementIndex < manager->handleCount) {
			i16 handleIndex = manager->indices[elementIndex];
			return manager->handles[handleIndex];
		}
		return handle;
	}

	HandleIter& operator++(int) {
		elementIndex++;
		return *this;
	}

	operator bool() const {
		return elementIndex < manager->handleCount;
	}
};

HandleIter BeginIter(const HandleManager &manager)
{
	HandleIter it = { .manager = &manager };
	return it;
}

#endif // HANDLE_MANAGER_H

