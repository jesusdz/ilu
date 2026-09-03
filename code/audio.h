#ifndef AUDIO_H
#define AUDIO_H

//#include "tools_mod.h"
#include "libs/ibxm/ibxm.h"


// Shared by every asset kind below. They live here because audio.h is the first
// header that needs them, not because they are an audio concern.

////////////////////////////////////////////////////////////////////////
// Asset flags

enum AssetFlags
{
	// Not serialized and hidden from the editor's asset lists. Transient previews are ghosts, and so
	// are the builtins, which the engine recreates on its own.
	AssetFlag_Ghost = 1 << 0,
	// Owned by the engine, not by the scene, so CleanScene must leave it alone. These assets hold the
	// shared images bound in the global bind group, which nothing recreates after initialization.
	AssetFlag_Builtin = 1 << 1,
};

// The desc fields below are typed AssetFlags, but combining two enumerators yields an int that C++
// will not convert back to the enum on its own, so give the type the operator it is used as if it had.
inline AssetFlags operator|(AssetFlags a, AssetFlags b) { return (AssetFlags)((u32)a | (u32)b); }

////////////////////////////////////////////////////////////////////////
// Binary data

// Types

#pragma pack(push, 1)

struct BinLocation
{
	u32 offset;
	u32 size;
};

#pragma pack(pop)


#define MAX_AUDIO_CLIPS 16
#define MAX_AUDIO_SOURCES 16
#define AUDIO_CHUNK_SAMPLE_COUNT (48000u/4u)

#define MAX_MUSIC_FILES 16

////////////////////////////////////////////////////////////////////////
// Types

enum AudioClipLoadSource
{
	AUDIO_CLIP_LOAD_SOURCE_WAV,
	//AUDIO_CLIP_LOAD_SOURCE_MOD,
	AUDIO_CLIP_LOAD_SOURCE_ASSETS,
};

struct AudioClipDesc
{
	ID id;
	const char *name;
	const char *filename;
	AssetFlags flags;
};

struct AudioClip
{
	AudioClipDesc desc;
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	AudioClipLoadSource loadSource;
	union
	{
		BinLocation location;
		const char *filename;
	};
};

enum AudioState
{
	AUDIO_STATE_IDLE,
	AUDIO_STATE_PLAYING,
	AUDIO_STATE_PAUSED,
};

struct AudioSource
{
	ID clip;
	u32 lastWriteSampleIndex = 0;
	AudioState state;
};

struct AudioChunk
{
	ID clipId;
	u32 index;
	i16 samples[AUDIO_CHUNK_SAMPLE_COUNT];
	AudioChunk *prev;
	AudioChunk *next;
};

enum LoadSource
{
	LOAD_SOURCE_MOD_FILE,
	LOAD_SOURCE_ASSET_FILE,
};

struct MusicFileDesc
{
	ID id;
	const char *name;
	const char *filename;
	AssetFlags flags;
};

struct MusicFile
{
	MusicFileDesc desc;
	LoadSource loadSource;
	union
	{
		BinLocation location;
		const char *filename;
	};
};

struct Audio
{
	// Compact, no holes, like the rest of the pools. Unlike the rest, these are read
	// by the mixing thread, so CompactAudio is what closes the gaps and it runs from
	// PreRenderAudio rather than from the frame loop. See CompactAudio.
	u32 clipCount;
	AudioClip clips[MAX_AUDIO_CLIPS] = {};

	AudioSource sources[MAX_AUDIO_SOURCES] = {};

	// Circular list of audio chunks
	AudioChunk audioChunkSentinel;

	// Music ring buffer
	i16 *musicBuffer;
	u32 musicBufferSampleCount; // Mono samples count

	// Music play state
	AudioState musicState;
	u32 musicBufferReadSampleIndex;
	u32 musicBufferWriteSampleIndex;

	u32 musicFileCount;
	MusicFile musicFiles[MAX_MUSIC_FILES] = {};

	ID musicFile; // Music file being played

	// MOD tracks
	Arena moduleArena;
	struct module *module;
	u32 moduleSampleCount;
	struct replay *moduleReplay;

	bool initialized;
};


#pragma pack(push, 1)

struct BinAudioClipDesc
{
	ID id;
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	BinLocation location;
};

struct BinMusicFileDesc
{
	ID id;
	const char *name;
	BinLocation location;
};

struct BinAudioClip
{
	BinAudioClipDesc *desc;
};

struct BinMusicFile
{
	BinMusicFileDesc *desc;
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////
// Functions

struct Engine;

// Each function takes the narrowest thing it touches, so a signature says how far the
// call can reach:
// - ID only        Resolved through the ID pool, no subsystem state read (see ilu_id.h).
// - nothing        Only queues an AudioCmd, which the mixing thread applies later.
// - Audio &        Reads or writes the audio pools.
// - Engine &       Streams from engine.assets, so it needs more than the audio state.

bool InitializeAudio(Audio &audio, Arena &globalArena);

bool LoadAudioClipFromWAVFile(const char *filename, Arena &arena, AudioClip &audioClip, void **outSamples);
bool LoadSamplesFromWAVFile(const char *filename, void *samples, u32 firstSampleIndex, u32 sampleCount);

AudioClip &GetAudioClip(ID clipId);
ID CreateAudioClip(Audio &audio, const BinAudioClip &binAudioClip);
ID CreateAudioClip(Audio &audio, const AudioClipDesc &audioClipDesc);
ID GetOrCreateAudioClip(Audio &audio, const AudioClipDesc &audioClipDesc);
void RemoveAudioClip(ID clipId); // Deferred, takes effect on the next CompactAudio
void CompactAudio(Audio &audio);
u32 PlayAudioClip(Audio &audio, ID clipId);
bool IsActiveAudioSource(const Audio &audio, u32 audioSourceIndex);
bool IsPausedAudioSource(const Audio &audio, u32 audioSourceIndex);
void PauseAudioSource(u32 audioSourceIndex);
void ResumeAudioSource(u32 audioSourceIndex);
void StopAudioSource(u32 audioSourceIndex);

void PreRenderAudio(Audio &audio);
void RenderAudio(Engine &engine, SoundBuffer &soundBuffer); // Streams clips from engine.assets

MusicFile &GetMusicFile(ID musicId);
ID CreateMusicFile(Audio &audio, const BinMusicFile &binMusicFile);
ID CreateMusicFile(Audio &audio, const MusicFileDesc &musicFileDesc);
ID GetOrCreateMusicFile(Audio &audio, const MusicFileDesc &musicFileDesc);
void DestroyMusicFile(ID musicId);
void MusicPlay(Engine &engine, ID musicId); // Streams the module from engine.assets
void MusicPause();
void MusicStop(Audio &audio);
bool MusicIsPlaying(const Audio &audio);

void AudioStopAll(Audio &audio);

#endif // AUDIO_H
