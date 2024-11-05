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

#define RELAX_DIFFUSE
#define RELAX_SH

#include "RELAX_Config.hlsli"
#include "RELAX_PrePass.resources.hlsli"

#include "Common.hlsli"
#include "RELAX_Common.hlsli"
#include "RELAX_PrePass.hlsli"
