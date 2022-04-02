#include "memory.h"

uint8_t *allocate_memory(Memory *memory, uint32_t size)
{
    uint8_t *buffer = 0;
    for (uint32_t i = 0; i < memory->allocationCount; i++)
    {
        Allocation *a = &memory->allocations[i];

        if (a->size < size && (a->size - a->used) > size)
        {
            buffer = a->memory + a->used;
            a->used += size;
            break;
        }
    }

    if (!buffer)
    {
        if (memory->allocationCount < MAX_ALLOCATIONS && memory->used + size < memory->size)
        {
            buffer = memory->memory + memory->used;

            // Increase memory used
            memory->used += size;

            // Add Allocation
            Allocation *a = &memory->allocations[memory->allocationCount];
            a->size = size;
            a->used = size;
            a->memory = buffer;

            // Increase Allocations
            memory->allocationCount++;
        }
    }

    return buffer;
}

void free_memory(Memory *memory, uint8_t *location)
{
    for (uint32_t i = 0; i < memory->allocationCount; i++)
    {
        Allocation *a = &memory->allocations[i];

        if (a->memory == location)
        {
            a->used = 0;
            break;
        }
    }
}