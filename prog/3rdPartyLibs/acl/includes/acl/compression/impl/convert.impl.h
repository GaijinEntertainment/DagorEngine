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

// Included only once from convert.h

#include "acl/version.h"
#include "acl/compression/compress.h"
#include "acl/compression/track_array.h"
#include "acl/core/compressed_tracks.h"
#include "acl/core/error_result.h"
#include "acl/core/iallocator.h"
#include "acl/core/track_formats.h"
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/debug_track_writer.h"
#include "acl/decompression/decompress.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		struct raw_sampling_decompression_settings final : public decompression_settings
		{
			// Disable normalization. This is only safe if we know the input data is already normalized!
			static constexpr rotation_normalization_policy_t get_rotation_normalization_policy() { return rotation_normalization_policy_t::never; }

			// Only support raw formats. This ensures we don't interpolate when we can avoid it to maintain the original
			// accuracy and it strips the code we don't need for faster decompression.
			static constexpr bool is_rotation_format_supported(rotation_format8 format) { return format == rotation_format8::quatf_full; }
			static constexpr bool is_translation_format_supported(vector_format8 format) { return format == vector_format8::vector3f_full; }
			static constexpr bool is_scale_format_supported(vector_format8 format) { return format == vector_format8::vector3f_full; }
		};
	}

	inline error_result convert_track_list(iallocator& allocator, const track_array& track_list, compressed_tracks*& out_compressed_tracks)
	{
		compression_settings settings = get_raw_compression_settings();

		qvvf_transform_error_metric error_metric;
		settings.error_metric = &error_metric;

		// Include all the metadata
		settings.metadata.include_track_list_name = true;
		settings.metadata.include_track_names = true;
		settings.metadata.include_parent_track_indices = true;
		settings.metadata.include_track_descriptions = true;

		output_stats stats;
		return compress_track_list(allocator, track_list, settings, out_compressed_tracks, stats);
	}

	inline error_result convert_track_list(iallocator& allocator, const compressed_tracks& tracks, track_array& out_track_list)
	{
		error_result error = tracks.is_valid(false);
		if (error.any())
			return error;

		const uint32_t num_tracks = tracks.get_num_tracks();
		const track_type8 track_type = tracks.get_track_type();
		const uint32_t num_samples = tracks.get_num_samples_per_track();
		const float sample_rate = tracks.get_sample_rate();
		const float duration = tracks.get_finite_duration();

		track_array result(allocator, num_tracks);
		result.set_name(string(allocator, tracks.get_name()));

		// Setup our track metadata and allocate memory
		bool success = true;
		for (uint32_t track_index = 0; track_index < num_tracks; ++track_index)
		{
			track& track_ = result[track_index];

			track_desc_scalarf desc_scalar;
			track_desc_transformf desc_transform;

			bool got_description = false;
			switch (track_type)
			{
			case track_type8::float1f:
				got_description = tracks.get_track_description(track_index, desc_scalar);
				track_ = track_float1f::make_reserve(desc_scalar, allocator, num_samples, sample_rate);
				break;
			case track_type8::float2f:
				got_description = tracks.get_track_description(track_index, desc_scalar);
				track_ = track_float2f::make_reserve(desc_scalar, allocator, num_samples, sample_rate);
				break;
			case track_type8::float3f:
				got_description = tracks.get_track_description(track_index, desc_scalar);
				track_ = track_float3f::make_reserve(desc_scalar, allocator, num_samples, sample_rate);
				break;
			case track_type8::float4f:
				got_description = tracks.get_track_description(track_index, desc_scalar);
				track_ = track_float4f::make_reserve(desc_scalar, allocator, num_samples, sample_rate);
				break;
			case track_type8::vector4f:
				got_description = tracks.get_track_description(track_index, desc_scalar);
				track_ = track_vector4f::make_reserve(desc_scalar, allocator, num_samples, sample_rate);
				break;
			case track_type8::qvvf:
				got_description = tracks.get_track_description(track_index, desc_transform);
				track_ = track_qvvf::make_reserve(desc_transform, allocator, num_samples, sample_rate);
				break;
			default:
				ACL_ASSERT(false, "Unexpected track type");
				break;
			}

			success &= got_description;

			track_.set_name(string(allocator, tracks.get_track_name(track_index)));
		}

		if (!success)
			return error_result("Metadata was missing from the input");

		// Disable floating point exceptions since decompression assumes it
		scope_disable_fp_exceptions fp_off;

		// Decompress and populate our track data
		const acl_impl::tracks_header& header = acl_impl::get_tracks_header(tracks);
		if (track_type == track_type8::qvvf &&
			header.get_rotation_format() == rotation_format8::quatf_full &&
			header.get_translation_format() == vector_format8::vector3f_full &&
			header.get_scale_format() == vector_format8::vector3f_full)
		{
			// Our input transform data uses full precision, retain it
			decompression_context<acl_impl::raw_sampling_decompression_settings> context;
			context.initialize(tracks);

			acl_impl::debug_track_writer writer(allocator, track_type, num_tracks);
			writer.initialize_with_defaults(result);

			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				const float sample_time = rtm::scalar_min(float(sample_index) / sample_rate, duration);

				// Round to nearest to land directly on a sample
				context.seek(sample_time, sample_rounding_policy::nearest);

				context.decompress_tracks(writer);

				for (uint32_t track_index = 0; track_index < num_tracks; ++track_index)
				{
					track& track_ = result[track_index];

					switch (track_type)
					{
					case track_type8::float1f:
						*acl_impl::bit_cast<float*>(track_[sample_index]) = writer.read_float1(track_index);
						break;
					case track_type8::float2f:
						rtm::vector_store2(writer.read_float2(track_index), acl_impl::bit_cast<rtm::float2f*>(track_[sample_index]));
						break;
					case track_type8::float3f:
						rtm::vector_store3(writer.read_float3(track_index), acl_impl::bit_cast<rtm::float3f*>(track_[sample_index]));
						break;
					case track_type8::float4f:
						rtm::vector_store(writer.read_float4(track_index), acl_impl::bit_cast<rtm::float4f*>(track_[sample_index]));
						break;
					case track_type8::vector4f:
						*acl_impl::bit_cast<rtm::vector4f*>(track_[sample_index]) = writer.read_vector4(track_index);
						break;
					case track_type8::qvvf:
						*acl_impl::bit_cast<rtm::qvvf*>(track_[sample_index]) = writer.read_qvv(track_index);
						break;
					default:
						ACL_ASSERT(false, "Unexpected track type");
						break;
					}
				}
			}
		}
		else
		{
			decompression_context<decompression_settings> context;
			context.initialize(tracks);

			acl_impl::debug_track_writer writer(allocator, track_type, num_tracks);

			if (track_type == track_type8::qvvf)
				writer.initialize_with_defaults(result);

			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				const float sample_time = rtm::scalar_min(float(sample_index) / sample_rate, duration);

				// Round to nearest to land directly on a sample
				context.seek(sample_time, sample_rounding_policy::nearest);

				context.decompress_tracks(writer);

				for (uint32_t track_index = 0; track_index < num_tracks; ++track_index)
				{
					track& track_ = result[track_index];

					switch (track_type)
					{
					case track_type8::float1f:
						*acl_impl::bit_cast<float*>(track_[sample_index]) = writer.read_float1(track_index);
						break;
					case track_type8::float2f:
						rtm::vector_store2(writer.read_float2(track_index), acl_impl::bit_cast<rtm::float2f*>(track_[sample_index]));
						break;
					case track_type8::float3f:
						rtm::vector_store3(writer.read_float3(track_index), acl_impl::bit_cast<rtm::float3f*>(track_[sample_index]));
						break;
					case track_type8::float4f:
						rtm::vector_store(writer.read_float4(track_index), acl_impl::bit_cast<rtm::float4f*>(track_[sample_index]));
						break;
					case track_type8::vector4f:
						*acl_impl::bit_cast<rtm::vector4f*>(track_[sample_index]) = writer.read_vector4(track_index);
						break;
					case track_type8::qvvf:
						*acl_impl::bit_cast<rtm::qvvf*>(track_[sample_index]) = writer.read_qvv(track_index);
						break;
					default:
						ACL_ASSERT(false, "Unexpected track type");
						break;
					}
				}
			}
		}

		out_track_list = std::move(result);
		return error_result();
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
