// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// See https://gaijinentertainment.github.io/DagorEngine/dagor-tools/shader-compiler/contributing_to_compiler.html#versioning
// this is basically version of _COMPILER_ itself (usually dll),
// increase it if hlsl may produce a bytecode different from the previous version
#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_METAL
static const uint32_t sha1_cache_version = 28;
#elif _CROSS_TARGET_SPIRV
static const uint32_t sha1_cache_version = 31;
#elif _CROSS_TARGET_EMPTY
static const uint32_t sha1_cache_version = 4;
#elif _CROSS_TARGET_DX12
static const uint32_t sha1_cache_version = 31;
#elif _CROSS_TARGET_DX11 //_TARGET_PC is also defined
static const uint32_t sha1_cache_version = 7;
#endif
