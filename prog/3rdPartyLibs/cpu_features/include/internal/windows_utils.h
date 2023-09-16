// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CPU_FEATURES_INCLUDE_INTERNAL_WINDOWS_UTILS_H_
#define CPU_FEATURES_INCLUDE_INTERNAL_WINDOWS_UTILS_H_

#include "cpu_features_macros.h"

#ifdef CPU_FEATURES_OS_WINDOWS

#include <windows.h>  // IsProcessorFeaturePresent

// modern WinSDK winnt.h contains newer features detection definitions
#if !defined(PF_SSSE3_INSTRUCTIONS_AVAILABLE)
#define PF_SSSE3_INSTRUCTIONS_AVAILABLE 36
#endif

#if !defined(PF_SSE4_1_INSTRUCTIONS_AVAILABLE)
#define PF_SSE4_1_INSTRUCTIONS_AVAILABLE 37
#endif

#if !defined(PF_SSE4_2_INSTRUCTIONS_AVAILABLE)
#define PF_SSE4_2_INSTRUCTIONS_AVAILABLE 38
#endif

#endif  // CPU_FEATURES_OS_WINDOWS
#endif  // CPU_FEATURES_INCLUDE_INTERNAL_WINDOWS_UTILS_H_
