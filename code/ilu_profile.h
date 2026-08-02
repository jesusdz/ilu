#ifndef ILU_PROFILE
#define ILU_PROFILE

#ifndef USE_PROFILE
#define USE_PROFILE 1
#endif

#ifndef USE_PROFILE_GPU
#define USE_PROFILE_GPU 1
#endif

#ifdef USE_PROFILE_GPU
#ifndef ILU_GFX_H
#error "Profiling GPU requires ilu_gfx.h included before this header."
#endif
#endif

#if USE_PROFILE

constexpr u32 MAX_PROFILE_EVENTS = 1024;
constexpr u32 MAX_PROFILE_NODES = 512;
constexpr u32 MAX_PROFILE_STACKED_EVENTS = 16;
constexpr u32 MAX_PROFILE_NAMES = 256;
constexpr u32 MAX_PROFILE_NAME_SLOTS = 512;
constexpr u32 MAX_PROFILE_STRING_CHARS = KB(8);
constexpr u32 MAX_PROFILE_FRAMES = 64;
constexpr u32 MAX_PROFILE_THREADS = 8;
constexpr u16 PROFILE_NODE_NONE = 0xFFFF;
constexpr u32 PROFILE_THREAD_NONE = 0xFFFFFFFF;

// Balanced begin/end pairs produce one node per two events
CT_ASSERT(MAX_PROFILE_NODES >= MAX_PROFILE_EVENTS / 2);
CT_ASSERT(MAX_PROFILE_NODES < PROFILE_NODE_NONE); // Node indices must fit in ProfileNode::parentIndex
CT_ASSERT((MAX_PROFILE_NAME_SLOTS & (MAX_PROFILE_NAME_SLOTS - 1)) == 0); // Is power of 2
CT_ASSERT(MAX_PROFILE_NAME_SLOTS >= 2 * MAX_PROFILE_NAMES);

typedef u64 ProfileTime;

enum ProfileEventType : u16
{
	ProfileEventType_Begin,
	ProfileEventType_End,
};

struct ProfileEvent
{
	ProfileTime time;
	ProfileEventType type;
	u16 nameId;
};

struct ProfileNode
{
	ProfileTime begin;
	ProfileTime end;
	u16 nameId;
	u16 parentIndex; // PROFILE_NODE_NONE if root
};

struct ProfileFrame
{
	u32 nodeCount;
	u32 droppedEventCount;
	ProfileNode nodes[MAX_PROFILE_NODES];
	ProfileTime begin;
	ProfileTime end;
	u64 index;
};

struct ProfileThread
{
	const char *name;
	ProfileEvent events[MAX_PROFILE_EVENTS];
	u32 eventCount;
	u32 droppedEventCount;
	ProfileTime frameBegin;
	// u32 threadId; // optional, for the UI later

	// History of frame data
	ProfileFrame frames[MAX_PROFILE_FRAMES]; // Ring buffer
	u64 frameIndex;
};

struct Profile
{
	// Per thread event recorders
	ProfileThread threads[MAX_PROFILE_THREADS];
	volatile_i64 threadClaimCount;
	u32 frameThreadIndex;

	// Ad-hoc string interning for names
	Mutex nameMutex;
	const char *names[MAX_PROFILE_NAMES];
	u32 nameCount;
	u16 nameSlots[MAX_PROFILE_NAME_SLOTS];
	char stringArena[MAX_PROFILE_STRING_CHARS];
	u32 stringArenaSize;
};

void ProfileInit();
u16 ProfileRegisterName(const char *name);
const char *ProfileGetName(u16 nameId);
void ProfileFlush();
void ProfileNewFrame();
void ProfileBeginEvent(u16 nameId);
void ProfileEndEvent(u16 nameId);

void ProfileRegisterThread(const char *name);
const char *ProfileGetThreadName(u32 threadIndex);
u32 ProfileGetThreadCount();
u32 ProfileGetThreadFrameCount(u32 threadIndex);
ProfileFrame ProfileGetThreadFrame(u32 threadIndex, u32 age);
// Convenience functions
u32 ProfileGetFrameCount();
ProfileFrame ProfileGetFrame(u32 age);

#endif // USE_PROFILE

#if USE_PROFILE_GPU

constexpr u32 MAX_PROFILE_GPU_QUERIES = 128; // Timestamps per frame, the actual limit for events
constexpr u32 MAX_PROFILE_GPU_EVENTS = MAX_PROFILE_GPU_QUERIES;
constexpr u32 MAX_PROFILE_GPU_NODES = MAX_PROFILE_GPU_EVENTS / 2;
constexpr u32 MAX_PROFILE_GPU_STACKED_EVENTS = MAX_PROFILE_STACKED_EVENTS;
constexpr u32 MAX_PROFILE_GPU_FRAMES = MAX_PROFILE_FRAMES;
constexpr u32 PROFILE_GPU_SLOT_NONE = 0xFFFFFFFF;

CT_ASSERT(MAX_PROFILE_GPU_NODES < PROFILE_NODE_NONE); // Node indices must fit in ProfileNode::parentIndex
CT_ASSERT((MAX_PROFILE_GPU_FRAMES & (MAX_PROFILE_GPU_FRAMES - 1)) == 0); // Is power of 2

struct ProfileGpuEvent
{
	u16 nameId;
	u16 queryIndex;
	ProfileEventType type;
};

struct ProfileGpuFrame
{
	u32 nodeCount;
	u32 droppedEventCount;
	ProfileNode nodes[MAX_PROFILE_GPU_NODES];
	ProfileTime begin; // Always 0: nodes are relative to the beginning of the GPU frame
	ProfileTime end;
	u64 index;
};

// One recording slot per frame in flight. Its query pool can neither be read nor reset until the
// GPU is done with the frame that wrote into it.
struct ProfileGpuSlot
{
	TimestampPool pool;
	ProfileGpuEvent events[MAX_PROFILE_GPU_EVENTS];
	u32 eventCount;
	u32 droppedEventCount;
	u32 openCount;  // Events began but not ended yet
	u32 dropDepth;  // Begin events without query, so their end events must be dropped too
	u32 frameBeginQuery;
	u32 frameEndQuery;
	u64 frameIndex;
	bool pending;   // Frame was recorded and is waiting for the GPU to be done with it
};

struct ProfileGpu
{
	// Recording state
	ProfileGpuSlot slots[MAX_FRAMES_IN_FLIGHT];
	u32 recordSlot; // PROFILE_GPU_SLOT_NONE when not recording
	u64 recordIndex;

	// History of resolved frame data
	ProfileGpuFrame frames[MAX_PROFILE_GPU_FRAMES]; // Ring buffer
	u64 frameIndex;

	bool enabled;
};

void ProfileGpuInit(const GraphicsDevice &device);
void ProfileGpuCleanup(const GraphicsDevice &device);
bool ProfileGpuResolveFrame(const GraphicsDevice &device); // True if a frame was resolved
void ProfileGpuBeginFrame(const CommandList &commandList);
void ProfileGpuEndFrame(const CommandList &commandList);
void ProfileGpuBeginEvent(const CommandList &commandList, u16 nameId);
void ProfileGpuEndEvent(const CommandList &commandList, u16 nameId);
u32 ProfileGpuGetFrameCount();
ProfileGpuFrame ProfileGpuGetFrame(u32 age);

#endif // USE_PROFILE_GPU


////////////////////////////////////////////////////////////////////////////////////////////////////
// Profile macros

#if USE_PROFILE

struct ProfileBlock
{
	u16 id;
	ProfileBlock(u16 nameId) : id(nameId) { ProfileBeginEvent(nameId); }
	~ProfileBlock() { ProfileEndEvent(id); }
};

#define PROFILE_INIT() ProfileInit()
#define PROFILE_THREAD(name) ProfileRegisterThread(name)
#define PROFILE_FLUSH() ProfileFlush()
#define PROFILE_FRAME() ProfileNewFrame()
#define PROFILE_BLOCK(name) \
	static const u16 profileNameId_##name = ProfileRegisterName(#name); \
	ProfileBlock profileBlock_##name(profileNameId_##name)

#else // !USE_PROFILE

#define PROFILE_INIT()
#define PROFILE_THREAD(name)
#define PROFILE_FLUSH()
#define PROFILE_FRAME()
#define PROFILE_BLOCK(name)

#endif // USE_PROFILE

#if USE_PROFILE_GPU

struct ProfileGpuBlock
{
	const CommandList &cmd;
	u16 id;
	ProfileGpuBlock(const CommandList &commandList, u16 nameId) : cmd(commandList), id(nameId) { ProfileGpuBeginEvent(commandList, nameId); }
	~ProfileGpuBlock() { ProfileGpuEndEvent(cmd, id); }
};

#define PROFILE_GPU_INIT(device) ProfileGpuInit(device)
#define PROFILE_GPU_CLEANUP(device) ProfileGpuCleanup(device)
#define PROFILE_GPU_RESOLVE(device) ProfileGpuResolveFrame(device)
#define PROFILE_GPU_FRAME_BEGIN(cmd) ProfileGpuBeginFrame(cmd)
#define PROFILE_GPU_FRAME_END(cmd) ProfileGpuEndFrame(cmd)

#define PROFILE_GPU_BLOCK(cmd, name) \
	static const u16 profileGpuNameId_##name = ProfileRegisterName(#name); \
	ProfileGpuBlock profileGpuBlock_##name(cmd, profileGpuNameId_##name)

#else // !USE_PROFILE_GPU

#define PROFILE_GPU_INIT(device)
#define PROFILE_GPU_CLEANUP(device)
#define PROFILE_GPU_RESOLVE(device) false
#define PROFILE_GPU_FRAME_BEGIN(cmd)
#define PROFILE_GPU_FRAME_END(cmd)
#define PROFILE_GPU_BLOCK(cmd, name)

#endif // USE_PROFILE_GPU

#endif // ILU_PROFILE


////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation

#if USE_PROFILE
#ifdef ILU_PROFILE_IMPLEMENTATION

static Profile sProfile = {};
thread_local u32 tThreadIndex = PROFILE_THREAD_NONE;

void ProfileInit()
{
	CreateMutex(sProfile.nameMutex);
}

static u32 ProfileThreadIndex()
{
	if (tThreadIndex == PROFILE_THREAD_NONE) {
		const i64 claimed = AtomicPreIncrement(&sProfile.threadClaimCount);
		tThreadIndex = claimed < MAX_PROFILE_THREADS ? (u32)claimed : PROFILE_THREAD_NONE;
	}
	return tThreadIndex;
}

u16 ProfileRegisterName(const char *name)
{
	MutexScope lock(sProfile.nameMutex);

	Profile &p = sProfile;

	// Lazy init of reserved/invalid id 0
	if (p.nameCount == 0) {
		p.names[0] = "<?>";
		p.nameCount = 1;
	}

	// Find existing id
	const u32 hash = HashStringFNV(name);
	u32 slot = hash & (MAX_PROFILE_NAME_SLOTS - 1);
	while (p.nameSlots[slot] != 0)
	{
		const u16 id = p.nameSlots[slot];
		if (StrEq(p.names[id], name)) {
			return id;
		}
		slot = (slot + 1) & (MAX_PROFILE_NAME_SLOTS - 1);
	}

	// If no space for the new name...
	const u32 len = StrLen(name);
	if (p.nameCount >= MAX_PROFILE_NAMES ||
			p.stringArenaSize + len + 1 > MAX_PROFILE_STRING_CHARS) {
		return 0;
	}

	// Copy string
	char *allocatedName = p.stringArena + p.stringArenaSize;
	MemCopy(allocatedName, name, len + 1);
	p.stringArenaSize += len + 1;

	// Return new id
	const u16 id = (u16)p.nameCount++;
	p.names[id] = allocatedName;
	p.nameSlots[slot] = id;

	return id;
}

const char *ProfileGetName(u16 nameId)
{
	const char *name = nameId < sProfile.nameCount ? sProfile.names[nameId] : "<?>";
	return name;
}

void ProfileFlush()
{
	// Build tree from previous frame (nodes are emitted at begin events, so they are stored in tree pre-order)
	u32 stack[MAX_PROFILE_STACKED_EVENTS] = {};
	u16 stackSize = 0;
	u32 skippedDepth = 0; // Begin events without node (capacity exceeded), so their end events must be consumed too

	const u32 index = ProfileThreadIndex();
	if (index == PROFILE_THREAD_NONE) { return; }
	ProfileThread &thread = sProfile.threads[index];

	ProfileFrame &frame = thread.frames[thread.frameIndex & (MAX_PROFILE_FRAMES - 1)];
	frame.nodeCount = 0;

	for (u32 i = 0; i < thread.eventCount; ++i)
	{
		ProfileEvent *event = &thread.events[i];
		if (event->type == ProfileEventType_Begin)
		{
			if (skippedDepth > 0 ||
					stackSize >= MAX_PROFILE_STACKED_EVENTS ||
					frame.nodeCount >= MAX_PROFILE_NODES)
			{
				skippedDepth++;
				thread.droppedEventCount++;
				continue;
			}

			const u16 parentIndex = stackSize > 0 ? (u16)stack[stackSize - 1] : PROFILE_NODE_NONE;
			stack[stackSize] = frame.nodeCount;
			frame.nodes[frame.nodeCount++] = {
				.begin = event->time,
				.end = event->time,
				.nameId = event->nameId,
				.parentIndex = parentIndex,
			};
			stackSize++;
		}
		else if (skippedDepth > 0)
		{
			skippedDepth--;
		}
		else if (stackSize == 0)
		{
			if (thread.droppedEventCount == 0) {
				LOG(Warning, "ProfileEndEvent('%s') does not match any begin event\n", ProfileGetName(event->nameId));
			}
		}
		else
		{
			ProfileNode *node = &frame.nodes[stack[--stackSize]];
			if ( event->nameId != node->nameId && thread.droppedEventCount == 0 )
			{
				LOG(Warning, "ProfileEndEvent('%s') does not match ProfileBeginEvent('%s')\n",
						ProfileGetName(event->nameId), ProfileGetName(node->nameId));
			}
			node->end = event->time;
		}
	}

	const ProfileTime now = GetTicks();

	// Close nodes left open
	while (stackSize > 0)
	{
		stackSize--;
		ProfileNode &node = frame.nodes[stack[stackSize]];
		node.end = now;
		if (thread.droppedEventCount == 0) {
			LOG(Warning, "ProfileBeginEvent('%s') does not match any end event\n", ProfileGetName(frame.nodes[stack[stackSize]].nameId));
		}
	}

	frame.begin = thread.frameBegin > 0 ? thread.frameBegin : now;
	frame.end = now;
	frame.index = thread.frameIndex;
	frame.droppedEventCount = thread.droppedEventCount; // build-time drops (loop) + record-time drops

	// Restart event count for the new frame
	thread.frameBegin = now;
	thread.eventCount = 0;
	thread.droppedEventCount = 0;
	thread.frameIndex++;
}

void ProfileNewFrame()
{
	ProfileFlush();
	sProfile.frameThreadIndex = ProfileThreadIndex();
}

void ProfileBeginEvent(u16 nameId)
{
	const u32 index = ProfileThreadIndex();
	if (index != PROFILE_THREAD_NONE)
	{
		ProfileThread &thread = sProfile.threads[index];
		if (thread.eventCount >= MAX_PROFILE_EVENTS) {
			thread.droppedEventCount++;
			return;
		}

		thread.events[thread.eventCount++] = {
			.time = GetTicks(),
			.type = ProfileEventType_Begin,
			.nameId = nameId,
		};
	}
}

void ProfileEndEvent(u16 nameId)
{
	const u32 index = ProfileThreadIndex();
	if (index != PROFILE_THREAD_NONE)
	{
		ProfileThread &thread = sProfile.threads[index];
		if (thread.eventCount >= MAX_PROFILE_EVENTS) {
			thread.droppedEventCount++;
			return;
		}

		thread.events[thread.eventCount++] = {
			.time = GetTicks(),
			.type = ProfileEventType_End,
			.nameId = nameId,
		};
	}
}

void ProfileRegisterThread(const char *name)
{
    const u32 index = ProfileThreadIndex();
    if (index != PROFILE_THREAD_NONE) {
		sProfile.threads[index].name = name;
	}
}

const char *ProfileGetThreadName(u32 threadIndex)
{
	const char *name = "Thread";
    if (threadIndex < MAX_PROFILE_THREADS && sProfile.threads[threadIndex].name) {
        name = sProfile.threads[threadIndex].name;
	}
    return name;
}

u32 ProfileGetThreadCount()
{
	const u32 c = (u32)sProfile.threadClaimCount;
	const u32 count = c < MAX_PROFILE_THREADS ? c : MAX_PROFILE_THREADS;
	return count;
}

u32 ProfileGetThreadFrameCount(u32 threadIndex)
{
	u32 frameCount = 0;
	if (threadIndex < MAX_PROFILE_THREADS)
	{
		const u64 built = sProfile.threads[threadIndex].frameIndex;
		frameCount = built < MAX_PROFILE_FRAMES ? (u32)built :  MAX_PROFILE_FRAMES;
	}
	return frameCount;
}

ProfileFrame ProfileGetThreadFrame(u32 threadIndex, u32 age)
{
	ProfileFrame frame = {};
	if (threadIndex < MAX_PROFILE_THREADS && age < ProfileGetThreadFrameCount(threadIndex))
	{
		const ProfileThread &thread = sProfile.threads[threadIndex];
		const u64 index = thread.frameIndex - 1 - age;
		frame = thread.frames[index & (MAX_PROFILE_FRAMES - 1)];
	}
	return frame;
}

// Convenience functions

u32 ProfileGetFrameCount()
{
	const u32 frameCount = ProfileGetThreadFrameCount(sProfile.frameThreadIndex);
	return frameCount;
}

ProfileFrame ProfileGetFrame(u32 age)
{
	const ProfileFrame frame = ProfileGetThreadFrame(sProfile.frameThreadIndex, age);
	return frame;
}

#endif // ILU_PROFILE_IMPLEMENTATION
#endif // USE_PROFILE


////////////////////////////////////////////////////////////////////////////////////////////////////
// GPU profiling implementation

#if USE_PROFILE_GPU
#ifdef ILU_PROFILE_IMPLEMENTATION

static ProfileGpu sProfileGpu = {};

void ProfileGpuInit(const GraphicsDevice &device)
{
	ProfileGpu &p = sProfileGpu;

	ProfileGpuCleanup(device);

	p.recordSlot = PROFILE_GPU_SLOT_NONE;
	p.recordIndex = 0;
	p.frameIndex = 0;
	p.enabled = device.support.timestampQueries;

	if (!p.enabled) {
		LOG(Warning, "GPU profiling disabled: the device does not support timestamp queries\n");
		return;
	}

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		p.slots[i].pool = CreateTimestampPool(device, MAX_PROFILE_GPU_QUERIES);
	}
}

void ProfileGpuCleanup(const GraphicsDevice &device)
{
	ProfileGpu &p = sProfileGpu;

	if (!p.enabled) { return; }

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		DestroyTimestampPool(device, p.slots[i].pool);
		p.slots[i] = {};
	}

	p.recordSlot = PROFILE_GPU_SLOT_NONE;
	p.enabled = false;
}

// GPU timestamps come in millis from an arbitrary GPU origin, so they are converted to CPU tick
// units relative to the beginning of the frame. That way the same code that presents CPU nodes
// works for GPU nodes.
static ProfileTime ProfileGpuTicks(const Timestamp *timestamps, u32 queryIndex, f64 originMillis)
{
	const f64 millis = timestamps[queryIndex].millis - originMillis;
	const f64 ticks = millis * (f64)GetTicksPerSecond() / 1000.0;
	return ticks > 0.0 ? (ProfileTime)ticks : 0;
}

bool ProfileGpuResolveFrame(const GraphicsDevice &device)
{
	ProfileGpu &p = sProfileGpu;

	if (!p.enabled) { return false; }

	// The slot to be recorded this frame is the one recorded MAX_FRAMES_IN_FLIGHT frames ago. Its
	// fence was just waited at BeginFrame, so its queries are readable now, and only now: the pool
	// is reset again as soon as this frame starts recording.
	ProfileGpuSlot &slot = p.slots[device.frameIndex];
	if (!slot.pending) { return false; }
	slot.pending = false;

	Timestamp timestamps[MAX_PROFILE_GPU_QUERIES];
	const u32 queryCount = slot.frameEndQuery + 1;
	if ( !ReadTimestamps(slot.pool, 0, queryCount, timestamps) ) {
		LOG(Warning, "ProfileGpu: timestamps of frame %llu were not available\n", slot.frameIndex);
		return false;
	}

	// Everything is relative to the first timestamp of the frame
	const f64 originMillis = timestamps[slot.frameBeginQuery].millis;

	ProfileGpuFrame &frame = p.frames[p.frameIndex & (MAX_PROFILE_GPU_FRAMES - 1)];
	frame.nodeCount = 0;
	frame.begin = 0;
	frame.end = ProfileGpuTicks(timestamps, slot.frameEndQuery, originMillis);
	frame.index = slot.frameIndex;
	frame.droppedEventCount = slot.droppedEventCount;

	// Build the tree (nodes are emitted at begin events, so they are stored in tree pre-order)
	u32 stack[MAX_PROFILE_GPU_STACKED_EVENTS] = {};
	u32 stackSize = 0;

	for (u32 i = 0; i < slot.eventCount; ++i)
	{
		const ProfileGpuEvent &event = slot.events[i];
		const ProfileTime time = ProfileGpuTicks(timestamps, event.queryIndex, originMillis);

		if (event.type == ProfileEventType_Begin)
		{
			// Unbalanced events are already discarded at record time
			ASSERT(stackSize < MAX_PROFILE_GPU_STACKED_EVENTS);
			ASSERT(frame.nodeCount < MAX_PROFILE_GPU_NODES);

			const u16 parentIndex = stackSize > 0 ? (u16)stack[stackSize - 1] : PROFILE_NODE_NONE;
			stack[stackSize++] = frame.nodeCount;
			frame.nodes[frame.nodeCount++] = {
				.begin = time,
				.end = time,
				.nameId = event.nameId,
				.parentIndex = parentIndex,
			};
		}
		else
		{
			ASSERT(stackSize > 0);
			ProfileNode &node = frame.nodes[stack[--stackSize]];
			ASSERT(node.nameId == event.nameId);
			node.end = time;
		}
	}

	p.frameIndex++;

	return true;
}

void ProfileGpuBeginFrame(const CommandList &commandList)
{
	ProfileGpu &p = sProfileGpu;

	if (!p.enabled) { return; }

	const u32 slotIndex = commandList.device->frameIndex;
	ProfileGpuSlot &slot = p.slots[slotIndex];

	slot.eventCount = 0;
	slot.droppedEventCount = 0;
	slot.openCount = 0;
	slot.dropDepth = 0;
	slot.frameIndex = p.recordIndex;

	ResetTimestampPool(commandList, slot.pool);
	slot.frameBeginQuery = WriteTimestamp(commandList, slot.pool, PipelineStageTop);
	slot.frameEndQuery = slot.frameBeginQuery;

	p.recordSlot = slotIndex;
}

void ProfileGpuEndFrame(const CommandList &commandList)
{
	ProfileGpu &p = sProfileGpu;

	if (!p.enabled || p.recordSlot == PROFILE_GPU_SLOT_NONE) { return; }

	ProfileGpuSlot &slot = p.slots[p.recordSlot];

	if (slot.openCount > 0 || slot.dropDepth > 0) {
		LOG(Warning, "ProfileGpuBeginEvent does not match any end event\n");
	}

	slot.frameEndQuery = WriteTimestamp(commandList, slot.pool, PipelineStageBottom);
	slot.pending = true;

	p.recordSlot = PROFILE_GPU_SLOT_NONE;
	p.recordIndex++;
}

void ProfileGpuBeginEvent(const CommandList &commandList, u16 nameId)
{
	ProfileGpu &p = sProfileGpu;

	if (!p.enabled || p.recordSlot == PROFILE_GPU_SLOT_NONE) { return; }

	ProfileGpuSlot &slot = p.slots[p.recordSlot];

	// Besides this begin, there must be room for the end of every open event and for the frame end.
	// Events are dropped in whole subtrees so the tree stays balanced at resolve time.
	const u32 requiredQueries = slot.pool.queryCount + slot.openCount + 3;

	if (slot.dropDepth > 0 ||
			slot.openCount >= MAX_PROFILE_GPU_STACKED_EVENTS ||
			slot.eventCount >= MAX_PROFILE_GPU_EVENTS ||
			requiredQueries > slot.pool.maxQueries)
	{
		slot.dropDepth++;
		slot.droppedEventCount++;
		return;
	}

	const u32 queryIndex = WriteTimestamp(commandList, slot.pool, PipelineStageTop);
	slot.events[slot.eventCount++] = {
		.nameId = nameId,
		.queryIndex = (u16)queryIndex,
		.type = ProfileEventType_Begin,
	};
	slot.openCount++;
}

void ProfileGpuEndEvent(const CommandList &commandList, u16 nameId)
{
	ProfileGpu &p = sProfileGpu;

	if (!p.enabled || p.recordSlot == PROFILE_GPU_SLOT_NONE) { return; }

	ProfileGpuSlot &slot = p.slots[p.recordSlot];

	if (slot.dropDepth > 0) {
		slot.dropDepth--;
		return;
	}

	if (slot.openCount == 0) {
		LOG(Warning, "ProfileGpuEndEvent('%s') does not match any begin event\n", ProfileGetName(nameId));
		return;
	}

	const u32 queryIndex = WriteTimestamp(commandList, slot.pool, PipelineStageBottom);
	slot.events[slot.eventCount++] = {
		.nameId = nameId,
		.queryIndex = (u16)queryIndex,
		.type = ProfileEventType_End,
	};
	slot.openCount--;
}

u32 ProfileGpuGetFrameCount()
{
	const u64 resolved = sProfileGpu.frameIndex;
	const u32 frameCount = resolved < MAX_PROFILE_GPU_FRAMES ? (u32)resolved : MAX_PROFILE_GPU_FRAMES;
	return frameCount;
}

ProfileGpuFrame ProfileGpuGetFrame(u32 age)
{
	ProfileGpuFrame frame = {};
	if (age < ProfileGpuGetFrameCount())
	{
		const u64 index = sProfileGpu.frameIndex - 1 - age;
		frame = sProfileGpu.frames[index & (MAX_PROFILE_GPU_FRAMES - 1)];
	}
	return frame;
}

#endif // ILU_PROFILE_IMPLEMENTATION
#endif // USE_PROFILE_GPU

