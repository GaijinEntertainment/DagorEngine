#pragma once

#include <malloc.h>

#define malloc  memalloc_default
#define calloc  memcalloc_default
#define free    memfree_default
#define realloc memrealloc_default

#include <dxtlib/dxtlib.h>

#undef malloc
#undef calloc
#undef free
#undef realloc
