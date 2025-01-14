#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2020 Nicholas Frechette & Animation Compression Library contributors
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

// Included only once from compression_settings.h

#include "acl/version.h"
#include "acl/core/error_result.h"
#include "acl/core/hash.h"
#include "acl/core/track_formats.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	inline uint32_t compression_database_settings::get_hash() const
	{
		uint32_t hash_value = 0;
		hash_value = hash_combine(hash_value, hash32(max_chunk_size));
		hash_value = hash_combine(hash_value, hash32(medium_importance_tier_proportion));
		hash_value = hash_combine(hash_value, hash32(low_importance_tier_proportion));
		return hash_value;
	}

	inline error_result compression_database_settings::is_valid() const
	{
		if (max_chunk_size < 4 * 1024)
			return error_result("max_chunk_size must be greater or equal to 4 KB");

		if (align_to(max_chunk_size, 4 * 1024) != max_chunk_size)
			return error_result("max_chunk_size must be a multiple of 4 KB");

		if (!rtm::scalar_is_finite(medium_importance_tier_proportion) || medium_importance_tier_proportion < 0.0F || medium_importance_tier_proportion > 1.0F)
			return error_result("medium_importance_tier_proportion must be in the range [0.0, 1.0]");

		if (!rtm::scalar_is_finite(low_importance_tier_proportion) || low_importance_tier_proportion < 0.0F || low_importance_tier_proportion > 1.0F)
			return error_result("low_importance_tier_proportion must be in the range [0.0, 1.0]");

		// Add an epsilon to account for arithmetic imprecision
		const float database_proportion = low_importance_tier_proportion + medium_importance_tier_proportion;
		const float epsilon = 1.0e-5F;
		if (database_proportion < epsilon || database_proportion > (1.0F + epsilon))
			return error_result("The sum of medium_importance_tier_proportion + low_importance_tier_proportion must be in the range [0.0, 1.0]");

		return error_result();
	}

	inline uint32_t compression_metadata_settings::get_hash() const
	{
		uint32_t hash_value = 0;
		hash_value = hash_combine(hash_value, hash32(include_track_list_name));
		hash_value = hash_combine(hash_value, hash32(include_track_names));
		hash_value = hash_combine(hash_value, hash32(include_parent_track_indices));
		hash_value = hash_combine(hash_value, hash32(include_track_descriptions));
		hash_value = hash_combine(hash_value, hash32(include_contributing_error));

		return hash_value;
	}

	inline error_result compression_metadata_settings::is_valid() const
	{
		return error_result();
	}

	inline bool compression_keyframe_stripping_settings::is_enabled() const
	{
		return proportion > 0.0F || threshold > 0.0F || strip_trivial;
	}

	inline uint32_t compression_keyframe_stripping_settings::get_hash() const
	{
		uint32_t hash_value = 0;
		hash_value = hash_combine(hash_value, hash32(proportion));
		hash_value = hash_combine(hash_value, hash32(threshold));
		hash_value = hash_combine(hash_value, hash32(strip_trivial));

		return hash_value;
	}

	inline error_result compression_keyframe_stripping_settings::is_valid() const
	{
		if (!rtm::scalar_is_finite(proportion) || proportion < 0.0F || proportion > 1.0F)
			return error_result("proportion must be in the range [0.0, 1.0]");

		if (!rtm::scalar_is_finite(threshold) || threshold < 0.0F)
			return error_result("threshold must be positive definite");

		return error_result();
	}

	inline uint32_t compression_settings::get_hash() const
	{
		uint32_t hash_value = 0;
		hash_value = hash_combine(hash_value, hash32(level));
		hash_value = hash_combine(hash_value, hash32(rotation_format));
		hash_value = hash_combine(hash_value, hash32(translation_format));
		hash_value = hash_combine(hash_value, hash32(scale_format));

		if (error_metric != nullptr)
			hash_value = hash_combine(hash_value, error_metric->get_hash());

		hash_value = hash_combine(hash_value, enable_database_support);
		hash_value = hash_combine(hash_value, optimize_loops);
		hash_value = hash_combine(hash_value, keyframe_stripping.get_hash());
		hash_value = hash_combine(hash_value, metadata.get_hash());

		return hash_value;
	}

	inline error_result compression_settings::is_valid() const
	{
		if (error_metric == nullptr)
			return error_result("error_metric cannot be NULL");

		const error_result keyframe_stripping_result = keyframe_stripping.is_valid();
		if (keyframe_stripping_result.any())
			return keyframe_stripping_result;

		const error_result metadata_result = metadata.is_valid();
		if (metadata_result.any())
			return metadata_result;

		if (keyframe_stripping.is_enabled() && enable_database_support)
			return error_result("Cannot enable keyframe stripping with database support");

		return error_result();
	}

	inline compression_settings get_raw_compression_settings()
	{
		return compression_settings();
	}

	inline compression_settings get_default_compression_settings()
	{
		compression_settings settings;
		settings.level = compression_level8::automatic;
		settings.rotation_format = rotation_format8::quatf_drop_w_variable;
		settings.translation_format = vector_format8::vector3f_variable;
		settings.scale_format = vector_format8::vector3f_variable;
		settings.optimize_loops = true;
		settings.keyframe_stripping.strip_trivial = true;
		return settings;
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
