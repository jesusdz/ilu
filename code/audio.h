#ifndef AUDIO_H
#define AUDIO_H

//#include "tools_mod.h"
#include "libs/ibxm/ibxm.h"


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


////////////////////////////////////////////////////////////////////////
// Functions

struct Engine;

bool InitializeAudio(Audio &audio, Arena &globalArena);

bool LoadAudioClipFromWAVFile(const char *filename, Arena &arena, AudioClip &audioClip, void **outSamples);
bool LoadSamplesFromWAVFile(const char *filename, void *samples, u32 firstSampleIndex, u32 sampleCount);

AudioClip &GetAudioClip(ID clipId);
ID CreateAudioClip(Engine &engine, const BinAudioClip &binAudioClip);
ID CreateAudioClip(Engine &engine, const AudioClipDesc &audioClipDesc);
ID GetOrCreateAudioClip(Engine &engine, const AudioClipDesc &audioClipDesc);
void RemoveAudioClip(Engine &engine, ID clipId); // Deferred, takes effect on the next CompactAudio
void CompactAudio(Audio &audio);
u32 PlayAudioClip(Engine &engine, ID clipId);
bool IsActiveAudioSource(Engine &engine, u32 audioSourceIndex);
bool IsPausedAudioSource(Engine &engine, u32 audioSourceIndex);
void PauseAudioSource(Engine &engine, u32 audioSourceIndex);
void ResumeAudioSource(Engine &engine, u32 audioSourceIndex);
void StopAudioSource(Engine &engine, u32 audioSourceIndex);

void PreRenderAudio(Engine &engine);
void RenderAudio(Engine &engine, SoundBuffer &soundBuffer);

MusicFile &GetMusicFile(ID musicId);
ID CreateMusicFile(Engine &engine, const BinMusicFile &binMusicFile);
ID CreateMusicFile(Engine &engine, const MusicFileDesc &musicFileDesc);
ID GetOrCreateMusicFile(Engine &engine, const MusicFileDesc &musicFileDesc);
void DestroyMusicFile(Engine &engine, ID musicId);
void MusicPlay(Engine &engine, ID musicId);
void MusicPause(Engine &engine);
void MusicStop(Engine &engine);
bool MusicIsPlaying(Engine &engine);

void AudioStopAll(Engine &engine);

#endif // AUDIO_H
