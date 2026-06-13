// Copyright  © 2023 Advanced Micro Devices, Inc.
// Copyright  © 2024-2025 Arm Limited.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/// @defgroup FfxGPU GPU
/// The FidelityFX SDK GPU References
///
/// @ingroup ffxSDK

/// @defgroup FfxHLSL HLSL References
/// FidelityFX SDK HLSL GPU References
///
/// @ingroup FfxGPU

/// @defgroup FfxGLSL GLSL References
/// FidelityFX SDK GLSL GPU References
///
/// @ingroup FfxGPU

/// @defgroup FfxGPUEffects FidelityFX GPU References
/// FidelityFX Effect GPU Reference Documentation
///
/// @ingroup FfxGPU

/// @defgroup GPUCore GPU Core
/// GPU defines and functions
///
/// @ingroup FfxGPU

#if !defined(FFXM_CORE_H)
#define FFXM_CORE_H

#include "ffxm_common_types.h"

#if defined(FFXM_CPU)
#include "ffxm_core_cpu.h"
#endif // #if defined(FFXM_CPU)

#if defined(FFXM_GLSL) && defined(FFXM_GPU)
#include "ffxm_core_glsl.h"
#endif // #if defined(FFXM_GLSL) && defined(FFXM_GPU)

#if defined(FFXM_HLSL) && defined(FFXM_GPU)
#include "ffxm_core_hlsl.h"
#endif // #if defined(FFXM_HLSL) && defined(FFXM_GPU)

#if defined(FFXM_GPU)
#include "ffxm_core_gpu_common.h"
#include "ffxm_core_gpu_common_half.h"
#include "ffxm_core_portability.h"
#endif // #if defined(FFXM_GPU)
#endif // #if !defined(FFXM_CORE_H)
