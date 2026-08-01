/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#ifndef OMM_ASSERT_ENABLE
#define OMM_ASSERT_ENABLE (1)
#endif

#if OMM_ASSERT_ENABLE
#include <assert.h>
#define OMM_ASSERT(...) do { assert(__VA_ARGS__); } while(false)
#else
#define OMM_ASSERT(...) do {} while(false)
#endif
