#pragma once
// this is basically version of _COMPILER_ itself (usually dll)
#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_METAL
static const uint32_t sha1_cache_version = 15;
#elif _CROSS_TARGET_SPIRV
static const uint32_t sha1_cache_version = 19;
#elif _CROSS_TARGET_EMPTY
static const uint32_t sha1_cache_version = 2;
#elif _CROSS_TARGET_DX12
static const uint32_t sha1_cache_version = 20;
#elif _CROSS_TARGET_DX11 //_TARGET_PC is also defined
static const uint32_t sha1_cache_version = 5;
#endif
