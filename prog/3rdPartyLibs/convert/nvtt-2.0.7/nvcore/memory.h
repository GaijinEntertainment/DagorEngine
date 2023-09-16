// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_MEMORY_H
#define NV_CORE_MEMORY_H

#include <nvcore/nvcore.h>

// Custom memory allocator
namespace nv
{
	namespace mem 
	{
		inline void * malloc(size_t size) { return memalloc_default(size); }
		inline void free(const void * ptr) { memfree_default((void*)ptr); }
		inline void * realloc(void * ptr, size_t size) { return memrealloc_default(ptr, size); }
	} // mem namespace
	
} // nv namespace

#endif // NV_CORE_MEMORY_H
