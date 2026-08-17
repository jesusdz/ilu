
////////////////////////////////////////////////////////////////////////
// Audio command queue

#define AUDIO_CMD_QUEUE_CAPACITY 64

enum AudioCmdType : u32
{
	AudioCmd_SourcePlay,
	AudioCmd_SourcePause,
	AudioCmd_SourceStop,
	AudioCmd_MusicPlay,
	AudioCmd_MusicPause,
	AudioCmd_MusicStop,
	AudioCmd_StopAll,
};

struct AudioCmd
{
	AudioCmdType type;
	u32 sourceIndex; // For active audio sources
};

struct AudioCmdQueue
{
	AudioCmd slots[AUDIO_CMD_QUEUE_CAPACITY];
	volatile_u32 writePos;
	volatile_u32 readPos;
};

static AudioCmdQueue sAudioCmdQueue = {};

static void AudioCmdQueue_Push(AudioCmd cmd)
{
	AudioCmdQueue &queue = sAudioCmdQueue;
	ASSERT( queue.writePos - queue.readPos < AUDIO_CMD_QUEUE_CAPACITY );
	queue.slots[queue.writePos % AUDIO_CMD_QUEUE_CAPACITY] = cmd;
	FullWriteBarrier();
	queue.writePos = queue.writePos + 1;
}

static void AudioCmdQueue_ProcessCommand(Audio &audio, AudioCmd cmd)
{
	switch (cmd.type)
	{
		case AudioCmd_SourcePlay:
			audio.sources[cmd.sourceIndex].state = AUDIO_STATE_PLAYING;
			break;
		case AudioCmd_SourcePause:
			audio.sources[cmd.sourceIndex].state = AUDIO_STATE_PAUSED;
			break;
		case AudioCmd_SourceStop:
			audio.sources[cmd.sourceIndex] = {};
			break;
		case AudioCmd_MusicPlay:
			audio.musicState = AUDIO_STATE_PLAYING;
			break;
		case AudioCmd_MusicPause:
			audio.musicState = AUDIO_STATE_PAUSED;
			break;
		case AudioCmd_MusicStop:
			audio.musicState = AUDIO_STATE_IDLE;
			audio.musicBufferReadSampleIndex = 0;
			audio.musicBufferWriteSampleIndex = 0;
			break;
		case AudioCmd_StopAll:
			break;
	};
}

static void AudioCmdQueue_Process(Audio &audio)
{
	AudioCmdQueue &queue = sAudioCmdQueue;
    while (queue.readPos != queue.writePos)
    {
        const AudioCmd &cmd = queue.slots[queue.readPos % AUDIO_CMD_QUEUE_CAPACITY];
        AudioCmdQueue_ProcessCommand(audio, cmd);
        ++queue.readPos;
    }
}


////////////////////////////////////////////////////////////////////////
// Audio system init

// At 48000 Hz, 2 channels, 2 bytes per mono sample, 1MB is about 6 seconds of audio

#define AUDIO_CHUNK_MEMORY MB(4)
#define AUDIO_MUSIC_MEMORY MB(4)
#define AUDIO_MODULE_MEMORY MB(2)

bool InitializeAudio(Audio &audio, Arena &globalArena)
{
	// Allocate audio chunks

	const u32 totalChunkCount = AUDIO_CHUNK_MEMORY / sizeof(AudioChunk);

	// Make sure we at least have as many audio chunks as twice the number of
	// simultaneos audio sources. This is needed because every sound is split
	// in sequences of chunks, so we play one and prefetch the next one.
	if (totalChunkCount < MAX_AUDIO_SOURCES * 2)
	{
		LOG(Error, "- totalChunkCount (%u) must be >= MAX_AUDIO_SOURCES * 2 (%u)\n", totalChunkCount, MAX_AUDIO_SOURCES * 2);
		return false;
	}

	AudioChunk *chunks = PushArray(globalArena, AudioChunk, totalChunkCount);
	if ( chunks == nullptr )
	{
		LOG(Error, "- Could not allocate memory for %u audio chunks\n", totalChunkCount);
		return false;
	}

	// Initialize doubly linked list of chunks

	for (u32 i = 0; i < totalChunkCount; ++i)
	{
		chunks[i].next = &chunks[i+1];
		chunks[i].prev = &chunks[i-1];
	}

	AudioChunk *first = &chunks[0];
	AudioChunk *last = &chunks[totalChunkCount-1];
	first->prev = &audio.audioChunkSentinel;
	last->next = &audio.audioChunkSentinel;
	audio.audioChunkSentinel.next = first;
	audio.audioChunkSentinel.prev = last;

	// Allocate music buffer

	const u32 musicMonoSampleCount = AUDIO_MUSIC_MEMORY / sizeof(i16);
	audio.musicBuffer = PushArray(globalArena, i16, musicMonoSampleCount);
	audio.musicBufferSampleCount = musicMonoSampleCount;

	audio.musicBufferReadSampleIndex = 0;
	audio.musicBufferWriteSampleIndex = 0;

	// Mod track

	audio.moduleArena = PushSubArena(globalArena, AUDIO_MODULE_MEMORY, "MOD arena");

	audio.musicFile = {};

	// Initialized!

	FullWriteBarrier();
	audio.initialized = true;

	return true;
}



////////////////////////////////////////////////////////////////////////
// WAV file loading

#define QUAD_CHAR(a,b,c,d) (a) | (b<<8) | (c<<16) | (d<<24)

enum RIFFCode
{
	RIFF_RIFF = QUAD_CHAR('R', 'I', 'F', 'F'),
	RIFF_WAVE = QUAD_CHAR('W', 'A', 'V', 'E'),
	RIFF_fmt  = QUAD_CHAR('f', 'm', 't', ' '),
	RIFF_data = QUAD_CHAR('d', 'a', 't', 'a'),
};

#pragma pack(push, 1)

struct WAVE_header
{
	u32 ChunkID;
	u32 ChunkSize;
	u32 Format;
};

struct WAVE_chunk
{
	u32 ID;
	u32 Size;
};

struct WAVE_fmt
{
	u16 AudioFormat;
	u16 NumChannels;
	u32 SampleRate;
	u32 ByteRate;
	u16 BlockAlign;
	u16 BitsPerSample;
	u16 cbSize;
	u16 ValidBitsPerSample;
	u32 ChannelMask;
	u8  SubFormat[16];
};

#pragma pack(pop)

bool LoadAudioClipFromWAVFile(const char *filename, Arena &arena, AudioClip &audioClip, void **outSamples)
{
	// TODO(jesus): maybe revisit where MakePath is needed?
	FilePath path = MakePath(AssetDir, filename);

	FILE *file = fopen(path.str, "rb");

	if ( file == nullptr ) { // try if filename is a filepath per se
		file = fopen(filename, "rb");
	}

	if (file != nullptr)
	{
		WAVE_header Header;
		WAVE_chunk Chunk;
		WAVE_fmt Fmt;
		u32 dataSize;
		void *data = nullptr;

		fread(&Header, sizeof(Header), 1, file);
		ASSERT(Header.ChunkID == RIFF_RIFF); // "RIFF"
		ASSERT(Header.Format == RIFF_WAVE); // "WAVE"

		while (1)
		{
			fread(&Chunk, sizeof(Chunk), 1, file);
			if (feof(file)) {
				break;
			}

			switch (Chunk.ID)
			{
				case RIFF_fmt:
					fread(&Fmt, Chunk.Size, 1, file);
					ASSERT(Fmt.AudioFormat == 1); // 1 means PCM
					ASSERT(Fmt.SampleRate == 48000);
					ASSERT(Fmt.NumChannels == 2);
					ASSERT(Fmt.BitsPerSample == 16);
					break;
				case RIFF_data:
					ASSERT(data == nullptr);
					dataSize = Chunk.Size;
					if (outSamples != nullptr)
					{
						data = PushSize(arena, dataSize);
						ASSERT(data != nullptr);
						fread(data, dataSize, 1, file);
						*outSamples = data;
					}
					else
					{
						fseek(file, dataSize, SEEK_CUR);
					}
					break;
				default:
					ASSERT(Chunk.Size > 0);
					fseek(file, Chunk.Size, SEEK_CUR);
					break;
			}
		}

		audioClip.filename = filename;
		audioClip.sampleSize = Fmt.BitsPerSample / 8;
		audioClip.samplingRate = Fmt.SampleRate;
		audioClip.channelCount = Fmt.NumChannels;
		audioClip.sampleCount = dataSize / audioClip.sampleSize;

		fclose(file);
		return true;
	}
	else
	{
		LOG(Warning, "Could not load sound file %s\n", filename);
		return false;
	}
}

bool LoadAudioClipFromWAVFile(const char *filename, AudioClip &audioClip)
{
	Arena dummyArena = {};
	return LoadAudioClipFromWAVFile(filename, dummyArena, audioClip, nullptr);
}

bool LoadSamplesFromWAVFile(const char *filename, void *samples, u32 firstSampleIndex, u32 sampleCount)
{
	FilePath path = MakePath(AssetDir, filename);

	FILE *file = fopen(path.str, "rb");
	if (file != nullptr)
	{
		WAVE_header Header;
		WAVE_chunk Chunk;
		WAVE_fmt Fmt;
		const u32 dataOffset = firstSampleIndex * sizeof(i16);
		const u32 dataSize = sampleCount * sizeof(i16);;

		fread(&Header, sizeof(Header), 1, file);
		ASSERT(Header.ChunkID == RIFF_RIFF); // "RIFF"
		ASSERT(Header.Format == RIFF_WAVE); // "WAVE"

		bool keepReading = true;

		while (keepReading)
		{
			fread(&Chunk, sizeof(Chunk), 1, file);
			if (feof(file)) {
				break;
			}

			switch (Chunk.ID)
			{
				case RIFF_fmt:
					fread(&Fmt, Chunk.Size, 1, file);
					ASSERT(Fmt.AudioFormat == 1); // 1 means PCM
					ASSERT(Fmt.SampleRate == 48000);
					ASSERT(Fmt.NumChannels == 2);
					ASSERT(Fmt.BitsPerSample == 16);
					break;
				case RIFF_data:
					ASSERT( dataOffset + dataSize <= Chunk.Size );
					fseek(file, dataOffset, SEEK_CUR);
					fread(samples, dataSize, 1, file);
					keepReading = false;
					break;
				default:
					ASSERT(Chunk.Size > 0);
					fseek(file, Chunk.Size, SEEK_CUR);
					break;
			}
		}

		fclose(file);
		return true;
	}
	else
	{
		LOG(Warning, "Could not load sound file %s\n", filename);
		return false;
	}
}

static i32 mixedSamples[48000/3];

void LoadSamplesFromModFile(struct replay *replay, void *samples, u32 firstSampleIndex, u32 sampleCount)
{
	ASSERT(firstSampleIndex % 2 == 0);
	ASSERT(sampleCount % 2 == 0);
	i32 stereoSampleCount = U32ToI32(sampleCount/2);
	i32 firstStereoSample = U32ToI32(firstSampleIndex/2);
	i32 lastStereoSample = firstStereoSample + stereoSampleCount;

	//i32 currentStereoSample = ModuleReplaySetSamplePos(replay, firstStereoSample);
	i32 currentStereoSample = replay_seek(replay, firstStereoSample);
	ASSERT(currentStereoSample <= firstStereoSample);

	LOG(Info, "SetPos(%d) vs. WantPos(%u)\n", currentStereoSample, firstStereoSample);

	i16 *dstSamples = (i16*)samples;

	while (currentStereoSample < lastStereoSample)
	{
		i32 *srcSamples = mixedSamples;
		i32 renderedStereoSampleCount = replay_get_audio(replay, srcSamples, 0 );
		LOG(Info, "- Rendered %u stereo samples\n", renderedStereoSampleCount);

		for (u32 i = 0; i < renderedStereoSampleCount && currentStereoSample < lastStereoSample; ++i)
		{
			if ( currentStereoSample >= firstStereoSample && currentStereoSample < lastStereoSample )
			{
				*dstSamples++ = ClipI32ToI16( *srcSamples++ );
				*dstSamples++ = ClipI32ToI16( *srcSamples++ );
			}
			++currentStereoSample;
		}
	}
}

// returns the number of mono samples loaded
u32 LoadSamplesFromModFile(struct replay *replay, void *samples, u32 sampleCount)
{
	i16 *dstSamples = (i16*)samples;
	i32 *srcSamples = mixedSamples;
	i32 renderedStereoSampleCount = replay_get_audio(replay, srcSamples, 0);
	ASSERT(renderedStereoSampleCount * 2 <= sampleCount);
	ASSERT(renderedStereoSampleCount * 2 <= ARRAY_COUNT(mixedSamples));

	for (u32 i = 0; i < renderedStereoSampleCount; ++i)
	{
		*dstSamples++ = ClipI32ToI16( *srcSamples++ );
		*dstSamples++ = ClipI32ToI16( *srcSamples++ );
	}

	return renderedStereoSampleCount * 2;
}



////////////////////////////////////////////////////////////////////////
// AudioClip and AudioSource management

AudioClip &GetAudioClip(ID id)
{
	ASSERT( Valid(id) );
	AudioClip &audioClip = *((AudioClip*)GetObject(id));
	return audioClip;
}

// Appends an audio clip and gives it its ID. Null when the array is full.
static AudioClip *PushAudioClip(Audio &audio, const AudioClipDesc &desc)
{
	if ( audio.clipCount == MAX_AUDIO_CLIPS )
	{
		LOG(Warning, "Could not create audio clip, the audio clip array is full.\n");
		return nullptr;
	}

	AudioClip &audioClip = audio.clips[audio.clipCount++];
	audioClip = {};
	audioClip.desc = desc;

	BindID(&audioClip.desc.id, &audioClip);

	return &audioClip;
}

ID CreateAudioClip(Audio &audio, const BinAudioClip &binAudioClip)
{
	const BinAudioClipDesc &desc = *binAudioClip.desc;

	AudioClip *audioClip = PushAudioClip(audio, { .id = desc.id });
	if ( !audioClip ) {
		return {};
	}

	audioClip->sampleSize = desc.sampleSize;
	audioClip->samplingRate = desc.samplingRate;
	audioClip->channelCount = desc.channelCount;
	audioClip->sampleCount = desc.sampleCount;
	audioClip->loadSource = AUDIO_CLIP_LOAD_SOURCE_ASSETS;
	audioClip->location = desc.location;

	return audioClip->desc.id;
}

ID CreateAudioClip(Audio &audio, const AudioClipDesc &audioClipDesc)
{
	AudioClip *audioClip = PushAudioClip(audio, audioClipDesc);
	if ( !audioClip ) {
		LOG(Warning, "Could not load audio clip %s (no more space left for audio clips)\n", audioClipDesc.filename);
		return {};
	}

	if ( !LoadAudioClipFromWAVFile(audioClipDesc.filename, *audioClip) )
	{
		LOG(Warning, "Could not load audio clip %s (not enough memory for audio clips)\n", audioClipDesc.filename);
		RemoveAudioClip(audioClip->desc.id);
		return {};
	}

	audioClip->loadSource = AUDIO_CLIP_LOAD_SOURCE_WAV;

	return audioClip->desc.id;
}

ID GetOrCreateAudioClip(Audio &audio, const AudioClipDesc &desc)
{
	ID id = {};
	for (u32 i = 0; i < audio.clipCount; ++i)
	{
		const AudioClipDesc &clipDesc = audio.clips[i].desc;
		if ( !( desc.flags & AssetFlag_Ghost ) && StrEq(desc.name, clipDesc.name)) {
			id = clipDesc.id;
			break;
		}
	}

	if ( !id )
	{
		id = CreateAudioClip(audio, desc);
	}
	return id;
}

void RemoveAudioClip(ID id)
{
	if (id)
	{
		// Marks only. The clip keeps its samples until CompactAudio, which runs on the
		// mixing thread, so anything mid-playback still has something valid to read.
		GetAudioClip(id).desc.id = {};
		Invalidate(id);
	}
}

// Closing the gaps moves clip storage, and RenderAudio reads that storage by index,
// so this must run on the thread that mixes rather than from the frame loop.
// PreRenderAudio calls it, right before the mixer would next look anything up.
//
// TODO(jesus): that holds where USE_AUDIO_THREAD is 1 (win32, linux), where
// PreRenderAudio and RenderAudio are both driven by AudioThread. On Android
// USE_AUDIO_THREAD is 0 and AAudio calls RenderAudio from its own thread while
// PreRenderAudio runs on the main one, so the two can overlap there.
void CompactAudio(Audio &audio)
{
	u32 storeIndex = U32_MAX;
	for (u32 i = 0; i < audio.clipCount; ++i)
	{
		if ( !audio.clips[i].desc.id ) { storeIndex = i; break; }
	}
	if ( storeIndex != U32_MAX )
	{
		for (u32 readIndex = storeIndex; readIndex < audio.clipCount; ++readIndex)
		{
			AudioClip &readClip = audio.clips[readIndex];
			if ( !readClip.desc.id ) { readClip = {}; continue; }

			AudioClip &writeClip = audio.clips[storeIndex++];
			writeClip = readClip;
			readClip = {};

			SetObject(writeClip.desc.id, &writeClip);
		}
		audio.clipCount = storeIndex;
	}

	storeIndex = U32_MAX;
	for (u32 i = 0; i < audio.musicFileCount; ++i)
	{
		if ( !audio.musicFiles[i].desc.id ) { storeIndex = i; break; }
	}
	if ( storeIndex != U32_MAX )
	{
		for (u32 readIndex = storeIndex; readIndex < audio.musicFileCount; ++readIndex)
		{
			MusicFile &readFile = audio.musicFiles[readIndex];
			if ( !readFile.desc.id ) { readFile = {}; continue; }

			MusicFile &writeFile = audio.musicFiles[storeIndex++];
			writeFile = readFile;
			readFile = {};

			SetObject(writeFile.desc.id, &writeFile);
		}
		audio.musicFileCount = storeIndex;
	}
}

#define INVALID_AUDIO_CLIP U32_MAX
#define INVALID_AUDIO_SOURCE U32_MAX

//u32 FindAudioClipIndex(Engine &engine, const char *name)
//{
//	u32 audioClipIndex = INVALID_AUDIO_CLIP;
//	for (u32 i = 0; i < audio.clipCount; ++i)
//	{
//		const AudioClip &audioClip = audio.clips[i];
//		if (StrEq(audioClip.name, name))
//		{
//			audioClipIndex = INVALID_AUDIO_CLIP;
//			break;
//		}
//	}
//	return audioClipIndex;
//}

u32 FindFreeAudioSource(const Audio &audio)
{
	for (u32 i = 0; i < ARRAY_COUNT(audio.sources); ++i)
	{
		if (audio.sources[i].state == AUDIO_STATE_IDLE)
		{
			return i;
		}
	}

	return INVALID_AUDIO_SOURCE;
}

u32 PlayAudioClip(Audio &audio, ID clipId)
{
	const u32 audioSourceIndex = FindFreeAudioSource(audio);

	if (audioSourceIndex == INVALID_AUDIO_SOURCE)
	{
		LOG(Warning, "No available source to play audio clip\n");
	}
	else
	{
		AudioSource &audioSource = audio.sources[audioSourceIndex];
		audioSource.clip = clipId;
		audioSource.lastWriteSampleIndex = 0;


		AudioCmd cmd = { .type = AudioCmd_SourcePlay, .sourceIndex = audioSourceIndex };
		AudioCmdQueue_Push(cmd);
	}

	return audioSourceIndex;
}

bool IsActiveAudioSource(const Audio &audio, u32 audioSourceIndex)
{
	bool active = false;
	if (audioSourceIndex < ARRAY_COUNT(audio.sources)) {
		const AudioSource &audioSource = audio.sources[audioSourceIndex];
		active = audioSource.state != AUDIO_STATE_IDLE;
	}
	return active;
}

bool IsPausedAudioSource(const Audio &audio, u32 audioSourceIndex)
{
	bool paused = false;
	if (audioSourceIndex < ARRAY_COUNT(audio.sources)) {
		const AudioSource &audioSource = audio.sources[audioSourceIndex];
		paused = audioSource.state == AUDIO_STATE_PAUSED;
	}
	return paused;
}

void PauseAudioSource(u32 audioSourceIndex)
{
	AudioCmd cmd = { .type = AudioCmd_SourcePause, .sourceIndex = audioSourceIndex };
	AudioCmdQueue_Push(cmd);
}

void ResumeAudioSource(u32 audioSourceIndex)
{
	AudioCmd cmd = { .type = AudioCmd_SourcePlay, .sourceIndex = audioSourceIndex };
	AudioCmdQueue_Push(cmd);
}

void StopAudioSource(u32 audioSourceIndex)
{
	if ( audioSourceIndex < MAX_AUDIO_SOURCES )
	{
		AudioCmd cmd = { .type = AudioCmd_SourceStop, .sourceIndex = audioSourceIndex };
		AudioCmdQueue_Push(cmd);
	}
}



////////////////////////////////////////////////////////////////////////
// Music pre-render

void PreRenderAudio(Audio &audio)
{
	CompactAudio(audio);

	Clock beginClock = GetClock();

	if ( audio.musicState == AUDIO_STATE_PLAYING )
	{
		struct replay *replay = audio.moduleReplay;

		const float budgetSeconds = 0.007f;
		const float elapsedSeconds = GetSecondsElapsed(beginClock, GetClock());

		while (elapsedSeconds < budgetSeconds)
		{
			u32 writtenSampleCount = audio.musicBufferWriteSampleIndex - audio.musicBufferReadSampleIndex;
			ASSERT(writtenSampleCount <= audio.musicBufferSampleCount);

			// If music is not being read, don't load any more samples
			if ( writtenSampleCount > audio.musicBufferSampleCount * 0.7 )
			{
				//LOG(Info, "Break\n");
				break;
			}

			// If we are writting far in the music buffer... place samples back at the beginning
			if (audio.musicBufferWriteSampleIndex > audio.musicBufferSampleCount * 0.7 )
			{
				//LOG(Info, "Copy\n");
				const u32 prevReadSampleIndex = audio.musicBufferReadSampleIndex;
				for (u32 i = 0; i < writtenSampleCount; ++i)
				{
					audio.musicBuffer[i] = audio.musicBuffer[prevReadSampleIndex + i];
				}
				audio.musicBufferReadSampleIndex = 0;
				audio.musicBufferWriteSampleIndex = writtenSampleCount;
			}

			void *samples = audio.musicBuffer + audio.musicBufferWriteSampleIndex;
			u32 sampleCount = audio.musicBufferSampleCount - audio.musicBufferWriteSampleIndex;

			static u32 counter = 0;
			//LOG(Info, "%u - LoadSamplesFromModFile %d\n", counter++, sampleCount);

			u32 loadedSampleCount = LoadSamplesFromModFile(replay, samples, sampleCount);
			audio.musicBufferWriteSampleIndex += loadedSampleCount;

			if ( loadedSampleCount == 0 )
			{
				audio.musicState = AUDIO_STATE_IDLE;
				break;
			}
		}
	}

	if (audio.musicState == AUDIO_STATE_IDLE)
	{
		audio.musicBufferWriteSampleIndex = 0;
		audio.musicBufferReadSampleIndex = 0;
	}
}

////////////////////////////////////////////////////////////////////////
// Audio mixer

void RenderAudio(Engine &engine, SoundBuffer &soundBuffer)
{
	Audio &audio = engine.audio;

	if (!audio.initialized) {
		return;
	}

	if ( soundBuffer.sampleCount == 0 ) {
		return;
	}

#if 1
	AudioCmdQueue_Process(audio);

	Scratch scratch;

	f32 *realSamples = PushArray(scratch.arena, f32, soundBuffer.sampleCount * 2.0f );

	// Clear sound buffer
	f32 *samplePtr = realSamples;
	for (u32 i = 0; i < soundBuffer.sampleCount; ++i)
	{
		*samplePtr++ = 0.0f;
		*samplePtr++ = 0.0f;
	}

	// Render music
	if ( audio.musicState == AUDIO_STATE_PLAYING )
	{
		u32 availableMusicSampleCount = audio.musicBufferWriteSampleIndex - audio.musicBufferReadSampleIndex;
		u32 musicSampleCount = Min((u32)soundBuffer.sampleCount * 2, availableMusicSampleCount);
		//LOG(Info, "%u / %u (%u)\n", audio.musicBufferReadSampleIndex, audio.musicBufferWriteSampleIndex, availableMusicSampleCount);
		i16 *srcSample = audio.musicBuffer + audio.musicBufferReadSampleIndex;
		f32 *dstSample = realSamples;
		for (u32 i = 0; i < musicSampleCount; ++i)
		{
			*dstSample++ = (f32)*srcSample++;
		}
		audio.musicBufferReadSampleIndex += musicSampleCount;
	}


	// Render audio clips
	for (u32 i = 0; i < ARRAY_COUNT(audio.sources); ++i)
	{
		AudioSource &audioSource = audio.sources[i];

		bool audioSourceIsValid = (bool)audioSource.clip;

		if ( audioSourceIsValid && audioSource.state == AUDIO_STATE_PLAYING )
		{
			AudioClip &audioClip = GetAudioClip(audioSource.clip);

			const u32 chunkCount = (audioClip.sampleCount - 1) / AUDIO_CHUNK_SAMPLE_COUNT + 1;
			const u32 currChunkIndex = audioSource.lastWriteSampleIndex / AUDIO_CHUNK_SAMPLE_COUNT;
			const u32 nextChunkIndex = Min(currChunkIndex + 1, chunkCount - 1);

			const u32 chunkIndices[] = { currChunkIndex, nextChunkIndex };
			const u32 prefetchChunkCount = nextChunkIndex - currChunkIndex + 1;
			ASSERT(prefetchChunkCount <= 2);

			// soundBuffer.sampleCount is for stereo samples (each stereo sample is 2 mono samples)
			u32 requestedSampleCount = soundBuffer.sampleCount * 2;

			f32 *dstSample = realSamples;

			for (u32 i = 0; i < prefetchChunkCount; ++i)
			{
				const u32 chunkIndex = chunkIndices[i];
				const u32 firstSampleIndex = chunkIndex * AUDIO_CHUNK_SAMPLE_COUNT;

				// Search chunk
				AudioChunk *chunk = audio.audioChunkSentinel.next;
				AudioChunk *end = &audio.audioChunkSentinel;
				while (chunk != end)
				{
					if ( audioSource.clip == chunk->clipId && chunkIndex == chunk->index ) {
						break;
					}
					chunk = chunk->next;
				}

				// No chunk found, get LRU and populate it
				if ( chunk == end )
				{
					chunk = audio.audioChunkSentinel.prev;
					chunk->clipId = audioSource.clip;
					chunk->index = chunkIndex;

					const u32 chunkSampleCount = (chunkIndex == chunkCount - 1) ? audioClip.sampleCount % AUDIO_CHUNK_SAMPLE_COUNT : AUDIO_CHUNK_SAMPLE_COUNT;
					if ( audioClip.loadSource == AUDIO_CLIP_LOAD_SOURCE_ASSETS )
					{
						FileSeek(engine.assets.file, audioClip.location.offset + firstSampleIndex * sizeof(i16));
						ReadFromFile(engine.assets.file, chunk->samples, chunkSampleCount * sizeof(i16));
					}
					else // id ( audioClip.loadSource == AUDIO_CLIP_LOAD_SOURCE_WAV )
					{
						LoadSamplesFromWAVFile(audioClip.filename, chunk->samples, firstSampleIndex, chunkSampleCount);
					}
				}

				// Remove chunk from the list
				chunk->next->prev = chunk->prev;
				chunk->prev->next = chunk->next;
				// Put it back first in the list
				chunk->next = audio.audioChunkSentinel.next;
				chunk->prev = &audio.audioChunkSentinel;
				chunk->next->prev = chunk;
				chunk->prev->next = chunk;

				// Copy requested samples from chunk to real samples
				const i16 *srcSample = chunk->samples + audioSource.lastWriteSampleIndex - firstSampleIndex;

				const u32 remainingClipSampleCount = audioClip.sampleCount - audioSource.lastWriteSampleIndex;
				const u32 remainingChunkSampleCount = firstSampleIndex + AUDIO_CHUNK_SAMPLE_COUNT - audioSource.lastWriteSampleIndex;
				const u32 remainingSampleCount = Min(remainingClipSampleCount, remainingChunkSampleCount);
				const u32 sampleCount = Min(remainingSampleCount, requestedSampleCount);

				for (u32 sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
				{
					*dstSample += (f32)*srcSample;
					dstSample++;
					srcSample++;
				}

				audioSource.lastWriteSampleIndex += sampleCount;
				if (audioSource.lastWriteSampleIndex >= audioClip.sampleCount)
				{
					audioSourceIsValid = false;
				}

				requestedSampleCount -= sampleCount;
			}

			FullWriteBarrier();
		}

		if ( !audioSourceIsValid )
		{
			// This implies AUDIO_STATE_IDLE
			audioSource = {};
		}
	}

	// Convert f32 samples back to i16 samples
	f32 *srcSample = realSamples;
	i16 *dstSample = soundBuffer.samples;
	for (u32 i = 0; i < soundBuffer.sampleCount; ++i)
	{
		*dstSample++ = (i16)*srcSample++;
		*dstSample++ = (i16)*srcSample++;
	}
#else
	// Wave parameters
	const u32 ToneHz = 256;
	const i32 ToneVolume = 4000;
	const u32 WavePeriod = soundBuffer.samplesPerSecond / ToneHz;
	static f32 tSine = 0.0f;

	//LOG(Debug, "bitrate:%u, wavePeriod:%u\n", soundBuffer.samplesPerSecond, WavePeriod);

	samplePtr = soundBuffer.samples;
	for (u32 i = 0; i < soundBuffer.sampleCount; ++i)
	{
		// Sine wave
		tSine += TwoPi/(f32)WavePeriod;
		while ( tSine >= TwoPi ) { tSine -= TwoPi; }
		const f32 sinValue = Sin(tSine);
		const i16 sample = (i16)(sinValue * ToneVolume);

		*samplePtr++ = sample;
		*samplePtr++ = sample;
	}
#endif

}


////////////////////////////////////////////////////////////////////////
// MOD music  tracks

MusicFile &GetMusicFile(ID id)
{
	ASSERT( Valid(id) );
	MusicFile &musicFile = *((MusicFile*)GetObject(id));
	return musicFile;
}

// Appends a music file and gives it its ID. Null when the array is full.
static MusicFile *PushMusicFile(Audio &audio, const MusicFileDesc &desc)
{
	if ( audio.musicFileCount == MAX_MUSIC_FILES )
	{
		LOG(Warning, "Could not create music file, the music file array is full.\n");
		return nullptr;
	}

	MusicFile &musicFile = audio.musicFiles[audio.musicFileCount++];
	musicFile = {};
	musicFile.desc = desc;

	BindID(&musicFile.desc.id, &musicFile);

	return &musicFile;
}

ID CreateMusicFile(Audio &audio, const BinMusicFile &binMusicFile)
{
	const BinMusicFileDesc &desc = *binMusicFile.desc;

	MusicFile *musicFile = PushMusicFile(audio, { .id = desc.id, .name = desc.name });
	if ( !musicFile ) {
		return {};
	}

	musicFile->loadSource = LOAD_SOURCE_ASSET_FILE;
	musicFile->location = desc.location;

	return musicFile->desc.id;
}

ID CreateMusicFile(Audio &audio, const MusicFileDesc &musicFileDesc)
{
	MusicFile *musicFile = PushMusicFile(audio, musicFileDesc);
	if ( !musicFile ) {
		LOG(Warning, "Could not load music file %s (no more space left for music files)\n", musicFileDesc.filename);
		return {};
	}

	musicFile->loadSource = LOAD_SOURCE_MOD_FILE;
	musicFile->filename = musicFileDesc.filename;

	return musicFile->desc.id;
}

ID GetOrCreateMusicFile(Audio &audio, const MusicFileDesc &desc)
{
	ID id = {};
	for (u32 i = 0; i < audio.musicFileCount; ++i)
	{
		const MusicFileDesc &musicDesc = audio.musicFiles[i].desc;
		if ( !( desc.flags & AssetFlag_Ghost ) && StrEq(desc.name, musicDesc.name)) {
			id = musicDesc.id;
			break;
		}
	}

	if ( !id )
	{
		id = CreateMusicFile(audio, desc);
	}
	return id;
}

void DestroyMusicFile(ID id)
{
	if ( id )
	{
		// Marks only, see RemoveAudioClip
		GetMusicFile(id).desc.id = {};
		Invalidate(id);
	}
}

void MusicPlay(Engine &engine, ID musicId)
{
	Audio &audio = engine.audio;

	if (!musicId) {
		return;
	}

	if (musicId != audio.musicFile)
	{
		ResetArena( audio.moduleArena );

		MusicFile &musicFile = GetMusicFile(musicId);
		DataChunk chunk = {};

		if (musicFile.loadSource == LOAD_SOURCE_MOD_FILE)
		{
			const char *filename = musicFile.filename;
			FilePath filePath = MakePath( AssetDir, filename );
			chunk = *PushFile( audio.moduleArena, filePath.str );

			if ( chunk.bytes != nullptr ) {
				LOG(Info, "%s loaded correctly from disk\n", filename);
			}
		}
		else if (musicFile.loadSource == LOAD_SOURCE_ASSET_FILE)
		{
			FileSeek(engine.assets.file, musicFile.location.offset);
			chunk.size = musicFile.location.size;
			chunk.bytes = PushArray(audio.moduleArena, byte, chunk.size);
			ReadFromFile(engine.assets.file, chunk.bytes, chunk.size);
		}

		if ( chunk.bytes != nullptr )
		{
			char message[64];
			struct data data;
			data.buffer = (char*)chunk.bytes;
			data.length = chunk.size;
			struct module *module = module_load(&data, message, &audio.moduleArena);

			if (module)
			{
				struct replay *replay = new_replay(module, 48000, 0, &audio.moduleArena);

				if (replay)
				{
					audio.module = module;
					audio.moduleReplay = replay;
					audio.musicFile = musicId;
					audio.moduleSampleCount = replay_calculate_duration( audio.moduleReplay ) * 2;
				}
				else
				{
					LOG(Warning, "Could not create music file replay\n");
					ResetArena(audio.moduleArena);
					audio.module = nullptr;
				}
			}
			else
			{
				LOG(Warning, "Could not load music file: %s\n", message);
				ResetArena(audio.moduleArena);
			}
		}
	}

	if ( audio.moduleReplay != nullptr )
	{
		AudioCmd cmd = { .type = AudioCmd_MusicPlay };
		AudioCmdQueue_Push(cmd);
	}
}

void MusicPause()
{
	AudioCmd cmd = { .type = AudioCmd_MusicPause };
	AudioCmdQueue_Push(cmd);
}

void MusicStop(Audio &audio)
{
	AudioCmd cmd = { .type = AudioCmd_MusicStop };
	AudioCmdQueue_Push(cmd);

	if ( audio.moduleReplay != nullptr ) {
		replay_set_sequence_pos( audio.moduleReplay, 0 );
	}
}

bool MusicIsPlaying(const Audio &audio)
{
	return audio.musicState == AUDIO_STATE_PLAYING;
}

void AudioStopAll(Audio &audio)
{
	MusicStop(audio);

	for (u32 i = 0; i < MAX_AUDIO_SOURCES; ++i)
	{
		StopAudioSource(i);
	}
}

