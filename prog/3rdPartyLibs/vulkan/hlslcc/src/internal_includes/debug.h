#ifndef DEBUG_H_
#define DEBUG_H_

#if defined(_DEBUG)
#include "assert.h"
#define ASSERT(expr) if(!(expr)) { printf("%s:%u: assertion failed: %s\n", __FILE__, __LINE__, #expr); }
#else
#define ASSERT(expr)
#endif

#endif
