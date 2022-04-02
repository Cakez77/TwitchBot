//TODO: Maybe use my own defines?
#include "logger.h"
#include <cstdint>
#define BIT(x) (1 << x)

#define internal static
#define global_variable static

#define ArraySize(arr) sizeof((arr)) / sizeof((arr)[0])

#define KB(x) ((uint64_t)1024 * x)
#define MB(x) ((uint64_t)1024 * KB(x))
#define GB(x) ((uint64_t)1024 * MB(x))

#define INVALID_IDX UINT32_MAX

// clang-format on
// clang-format off