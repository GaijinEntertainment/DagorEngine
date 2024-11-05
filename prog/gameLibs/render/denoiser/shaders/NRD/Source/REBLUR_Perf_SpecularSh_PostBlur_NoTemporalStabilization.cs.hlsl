/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NRD.hlsli"
#include "STL.hlsli"

#define REBLUR_PERFORMANCE_MODE
#define REBLUR_SPECULAR
#define REBLUR_SH
#define REBLUR_NO_TEMPORAL_STABILIZATION

#include "REBLUR_Config.hlsli"
#include "REBLUR_PostBlur.resources.hlsli"

#include "Common.hlsli"
#include "REBLUR_Common.hlsli"
#include "REBLUR_PostBlur.hlsli"
