#include "logger.h"

void to_lower_case(char *msgBegin)
{
    while (char c = *(msgBegin++))
    {
        if (c >= 'A' && c <= 'Z')
        {
            *(msgBegin - 1) = c + 32;
        }
    }
}

uint32_t str_length(const char *string)
{
    uint32_t size = 0;

    if (string)
    {
        while (char c = *(string++))
        {
            size++;
        }
    }

    return size;
}

bool str_in_str(char *hay, char *needle)
{
    CAKEZ_ASSERT(hay, "No hay supplied!");
    CAKEZ_ASSERT(needle, "No needle supplied!");

    bool result = false;
    uint32_t index = 0;

    while (char c = *(hay++))
    {
        if (!needle[index] && index > 0)
        {
            result = true;
            break;
        }

        if (needle[index] == c)
        {
            index++;
        }
        else
        {
            index = 0;
        }
    }

    return result;
}