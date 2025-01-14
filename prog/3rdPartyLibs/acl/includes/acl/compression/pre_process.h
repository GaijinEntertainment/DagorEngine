#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2023 Nicholas Frechette & Animation Compression Library contributors
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
////////////////////////////////////////////////////////////////////////////////

#include "acl/version.h"
#include "acl/core/additive_utils.h"
#include "acl/core/enum_utils.h"
#include "acl/core/error_result.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/compression/transform_error_metrics.h"
#include "acl/compression/track_array.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// pre_process_action represents various actions that can be executed when
	// pre-processing raw data. Multiple actions can be combined with binary operators.
	// When multiple actions are used, they execute in the order defined here.
	enum class pre_process_actions
	{
		// No action to perform
		none = 0x00,

		// Ensures every rotation quaternion is normalized.
		// A quaternion will be normalized only if it isn't already normalized to preserve
		// its original value where possible.
		// QVV transform tracks only.
		// This is lossy.
		normalize_rotations = 0x01,

		// Ensures every rotation quaternion has a positive W component.
		// This forces all quaternions into the same side of the hyper-sphere to
		// facilitate interpolation by avoiding wrapping around.
		// Because both halves of the hyper-sphere represent equivalent rotations,
		// this operation is considered lossless.
		// QVV transform tracks only.
		// This is lossless.
		ensure_quat_w_positive = 0x02,

		// Ensures that if the first and last keyframes match well enough to satisfy the
		// precision requirements, the last keyframe is overwritten with the first keyframe
		// to make sure they match perfectly.
		// This is lossy.
		optimize_looping = 0x04,

		// Sanitizes every constant track (and sub-track) by duplicating the optimal sample.
		// This is lossy.
		sanitize_constant_tracks = 0x08,

		// Sanitizes every default track (and sub-track) by duplicating the default sample.
		// This is lossy.
		sanitize_default_tracks = 0x10,

		// Performs all recommended pre-process actions
		recommended = normalize_rotations | optimize_looping | sanitize_constant_tracks | sanitize_default_tracks,
	};

	ACL_IMPL_ENUM_FLAGS_OPERATORS(pre_process_actions)

	////////////////////////////////////////////////////////////////////////////////
	// pre_process_precision_policy controls whether we should be lossless or lossy
	enum class pre_process_precision_policy
	{
		// Pre-processing will use binary equality and binary equivalent transformations where possible
		lossless,

		// Pre-processing will use the error metric and precision values provided
		lossy,
	};

	////////////////////////////////////////////////////////////////////////////////
	// Various settings to control pre-processing
	struct pre_process_settings_t
	{
		// The list of actions to perform
		pre_process_actions actions = pre_process_actions::recommended;

		// The precision policy to follow
		pre_process_precision_policy precision_policy = pre_process_precision_policy::lossless;

		// The optional error metric to use with transform tracks when lossy
		// Lossy pre-processing only
		// Transform tracks only
		const itransform_error_metric* error_metric = nullptr;

		// The optional additive track list to use with transform tracks
		// Lossy pre-processing only
		// Transform tracks only
		track_array_qvvf* additive_base = nullptr;
		additive_clip_format8 additive_format = additive_clip_format8::none;
	};

	//////////////////////////////////////////////////////////////////////////
	// Pre-processes a track array by performing various operations to improve
	// compression.
	//
	// ACL avoids modifying the raw data during compression to ensure that the caller
	// can still measure the same error that the internal algorithm sees. However,
	// this can lead to sub-optimal compression in certain cases. Rotation format
	// conversion and constant/default sub-track collapsing can yield values that
	// the optimizing algorithm can never fully reach, leading it to try too hard
	// for little to no gain. To avoid this issue, the raw data can be pre-processed.
	// This allows the caller to control everything. Raw data can be pre-processed
	// once following import (e.g. from FBX).
	//
	//    allocator:				The allocator instance to use for intermediary allocations
	//    settings:					The settings to use during pre-processing.
	//    track_list:				The track list to pre-process.
	//
	// This function returns an error result. If an error occurred, the raw data
	// may have mutated partially.
	//////////////////////////////////////////////////////////////////////////
	error_result pre_process_track_list(iallocator& allocator, const pre_process_settings_t& settings, track_array& track_list);

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/compression/impl/pre_process.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
