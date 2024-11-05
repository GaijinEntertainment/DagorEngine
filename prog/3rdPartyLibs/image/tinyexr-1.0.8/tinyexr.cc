#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#define TINYEXR_USE_MINIZ 0

#include <zlib.h>

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"
