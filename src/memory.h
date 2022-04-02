#pragma once
#include "defines.h"

constexpr uint32_t MAX_ALLOCATIONS = 100;

struct Allocation
{
    uint32_t size;
    uint32_t used;
    uint8_t *memory;
};

struct Memory
{
    uint32_t size;
    uint32_t used;
    uint8_t *memory;

    uint32_t allocationCount;
    Allocation allocations[MAX_ALLOCATIONS];
};

uint8_t *allocate_memory(Memory *memory, uint32_t size);
void free_memory(Memory *memory, uint8_t *location);