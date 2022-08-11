#pragma once

#include "defines.h"
#include "logger.h"
#include "memory.h"

enum SoundBufferType
{
	SOUND_BUFFER_TYPE_NORMAL,
	SOUND_BUFFER_TYPE_CHIPMUNK,
};

struct Sound
{
	bool loop;
	float volume;
	uint32_t size;
	bool playing;
	uint32_t playCursor;
	char *data;
	int16_t *samples;
	uint32_t sampleIdx;
	uint32_t sampleCount;
};

void play_sound(Sound sound, bool loop = false, float volume = 1.0f);
void play_sound(char* mp3File, uint32_t fileSize, 
								bool loop = false, float volume = 1.0f);

// Util
int sound_get_playing_sound_count();
void platform_change_sound_buffer(SoundBufferType type);