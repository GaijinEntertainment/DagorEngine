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

// Included only once from pre_process.h

#include "acl/version.h"
#include "acl/core/floating_point_exceptions.h"
#include "acl/compression/impl/pre_process.scalar.h"
#include "acl/compression/impl/pre_process.transform.h"

#include <rtm/quatf.h>
#include <rtm/scalarf.h>
#include <rtm/vector4f.h>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline void pre_process_optimize_looping(pre_process_context_t& context)
		{
			if (context.settings.precision_policy == pre_process_precision_policy::lossless)
				return;	// This pre-process action is lossy, do nothing

			track_array& track_list = context.track_list;

			if (track_list.get_looping_policy() == sample_looping_policy::wrap)
				return;	// This track list has already been optimized, the wrapping keyframe has been stripped

			if (track_list.get_num_samples_per_track() <= 1)
				return;	// We need at least two keyframes to detect if we loop or not

			const track_category8 track_category = track_list.get_track_category();
			const track_type8 track_type = track_list.get_track_type();

			if (track_category == track_category8::scalarf)
			{
				// Scalar tracks are looping if the first and last keyframes are within the precision threshold

				switch (track_type)
				{
				case track_type8::float1f:
					pre_process_optimize_looping_scalar<track_type8::float1f>(track_list);
					break;
				case track_type8::float2f:
					pre_process_optimize_looping_scalar<track_type8::float2f>(track_list);
					break;
				case track_type8::float3f:
					pre_process_optimize_looping_scalar<track_type8::float3f>(track_list);
					break;
				case track_type8::float4f:
					pre_process_optimize_looping_scalar<track_type8::float4f>(track_list);
					break;
				case track_type8::vector4f:
					pre_process_optimize_looping_scalar<track_type8::vector4f>(track_list);
					break;
				case track_type8::qvvf:
				default:
					ACL_ASSERT(false, "Unexpected track type");
					break;
				}
			}
			else if (track_category == track_category8::transformf)
			{
				ACL_ASSERT(track_type == track_type8::qvvf, "Expected qvvf");

				// Transform tracks are looping if replacing the last keyframe with the first satisfies
				// the error metric.

				pre_process_optimize_looping_transform(context);
			}
		}

		// Compacts constant sub-tracks
		// A sub-track is constant if every sample can be replaced by a single unique sample without exceeding
		// our error threshold.
		// Constant sub-tracks will retain the first sample.
		// A constant sub-track is a default sub-track if its unique sample can be replaced by the default value
		// without exceeding our error threshold.
		inline void pre_process_sanitize_constant_tracks(pre_process_context_t& context)
		{
			const pre_process_settings_t& settings = context.settings;

			if (settings.precision_policy == pre_process_precision_policy::lossless)
				return;	// This pre-process action is lossy, do nothing

			track_array& track_list = context.track_list;
			const track_category8 track_category = track_list.get_track_category();
			const track_type8 track_type = track_list.get_track_type();

			if (track_category == track_category8::scalarf)
			{
				// Scalar tracks are constant if their range extent is less than the precision threshold

				for (track& track_ : track_list)
				{
					switch (track_type)
					{
					case track_type8::float1f:
						pre_process_sanitize_constant_tracks_scalar<track_type8::float1f>(track_);
						break;
					case track_type8::float2f:
						pre_process_sanitize_constant_tracks_scalar<track_type8::float2f>(track_);
						break;
					case track_type8::float3f:
						pre_process_sanitize_constant_tracks_scalar<track_type8::float3f>(track_);
						break;
					case track_type8::float4f:
						pre_process_sanitize_constant_tracks_scalar<track_type8::float4f>(track_);
						break;
					case track_type8::vector4f:
						pre_process_sanitize_constant_tracks_scalar<track_type8::vector4f>(track_);
						break;
					case track_type8::qvvf:
					default:
						ACL_ASSERT(false, "Unexpected track type");
						break;
					}
				}
			}
			else if (track_category == track_category8::transformf)
			{
				ACL_ASSERT(track_type == track_type8::qvvf, "Expected qvvf");

				// Transform tracks are constant when the error metric is satisfied when the first
				// sample is repeated

				pre_process_sanitize_constant_tracks_transform(context);
			}
		}

		// See pre_process_sanitize_constant_tracks(..) above
		inline void pre_process_sanitize_default_tracks(pre_process_context_t& context)
		{
			if (context.settings.precision_policy == pre_process_precision_policy::lossless)
				return;	// This pre-process action is lossy, do nothing

			const track_array& track_list = context.track_list;
			const track_category8 track_category = track_list.get_track_category();

			if (track_category == track_category8::scalarf)
			{
				ACL_ASSERT(false, "Not implemented");
			}
			else if (track_category == track_category8::transformf)
			{
				ACL_ASSERT(track_list.get_track_type() == track_type8::qvvf, "Expected qvvf");

				// Transform tracks are default when the error metric is satisfied when the default
				// sample is repeated

				pre_process_sanitize_default_tracks_transform(context);
			}
		}
	}

	inline error_result pre_process_track_list(
		iallocator& allocator,
		const pre_process_settings_t& settings,
		track_array& track_list)
	{
		using namespace acl_impl;

		if (track_list.is_empty())
			return error_result();	// No tracks, nothing to do

		if (track_list.get_num_samples_per_track() == 0)
			return error_result();	// No samples, nothing to do

		if (settings.additive_format != additive_clip_format8::none)
		{
			if (track_list.get_track_type() != track_type8::qvvf)
				return error_result("'additive_format' is only supported with transform tracks");

			if (settings.additive_base == nullptr)
				return error_result("Missing 'additive_base' when 'additive_format' is used");

			if (settings.additive_base->is_empty())
				return error_result("'additive_base' cannot be empty when 'additive_format' is used");
		}

		// Disable floating point exceptions during compression because we leverage all SIMD lanes
		// and we might intentionally divide by zero, etc.
		scope_disable_fp_exceptions fp_off;

		pre_process_context_t context(allocator, settings, track_list);

		const track_type8 track_type = track_list.get_track_type();

		if (are_any_enum_flags_set(settings.actions, pre_process_actions::normalize_rotations))
		{
			if (track_type == track_type8::qvvf)
				pre_process_normalize_rotations(context);
		}

		if (are_any_enum_flags_set(settings.actions, pre_process_actions::ensure_quat_w_positive))
		{
			if (track_type == track_type8::qvvf)
				pre_process_ensure_quat_w_positive(context);
		}

		if (are_any_enum_flags_set(settings.actions, pre_process_actions::optimize_looping))
		{
			if (track_type == track_type8::qvvf &&
				settings.precision_policy == pre_process_precision_policy::lossy &&
				settings.error_metric == nullptr)
				return error_result("'error_metric' is required when optimizing looping transform tracks");

			pre_process_optimize_looping(context);
		}

		if (are_any_enum_flags_set(settings.actions, pre_process_actions::sanitize_constant_tracks))
		{
			if (track_type == track_type8::qvvf &&
				settings.precision_policy == pre_process_precision_policy::lossy &&
				settings.error_metric == nullptr)
				return error_result("'error_metric' is required when sanitizing lossy constant transform tracks");

			pre_process_sanitize_constant_tracks(context);
		}

		if (are_any_enum_flags_set(settings.actions, pre_process_actions::sanitize_default_tracks))
		{
			if (track_type == track_type8::qvvf &&
				settings.precision_policy == pre_process_precision_policy::lossy &&
				settings.error_metric == nullptr)
				return error_result("'error_metric' is required when sanitizing lossy default transform tracks");

			if (track_type == track_type8::qvvf)
				pre_process_sanitize_default_tracks(context);
		}

		// We are done!
		return error_result();
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
