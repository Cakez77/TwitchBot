#pragma once

#include "defines.h"
#include "logger.h"

uint32_t constexpr MAX_ALLOCATED_SOUNDS = 20;
uint32_t constexpr MAX_PLAYING_SOUNDS = 5;

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

struct SoundState
{
    uint32_t allocatedSoundCount;
    Sound allocatedSounds[MAX_ALLOCATED_SOUNDS];

    uint32_t playingSoundCount;
    Sound playingSounds[MAX_PLAYING_SOUNDS];
    uint32_t oldPlayCursor;
    uint32_t oldWriteCursor;

    uint32_t bufferSize;
    uint32_t allocatedByteCount;
    char *soundBuffer;
};

void play_sound(
    SoundState *soundState,
    Sound sound,
    bool loop = false,
    float volume = 1.0f)
{
    if (sound.data)
    {
        if (soundState->playingSoundCount < MAX_PLAYING_SOUNDS)
        {
            soundState->playingSounds[soundState->playingSoundCount++] = sound;
        }
    }
    else
    {
        // CAKEZ_ASSERT(0, "Reached maximum playing Sounds");
        // CAKEZ_ERROR("Reached maximum playing Sounds");
    }
}