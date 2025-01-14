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

// Included only once from track_error.h

#include "acl/version.h"
#include "acl/core/compressed_tracks.h"
#include "acl/core/error.h"
#include "acl/core/error_result.h"
#include "acl/core/iallocator.h"
#include "acl/core/impl/debug_track_writer.h"
#include "acl/compression/track_array.h"
#include "acl/compression/transform_error_metrics.h"
#include "acl/compression/impl/track_list_context.h"
#include "acl/decompression/decompress.h"

#include <rtm/scalarf.h>
#include <rtm/vector4f.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		inline rtm::vector4f RTM_SIMD_CALL get_scalar_track_error(track_type8 track_type, uint32_t raw_track_index, uint32_t lossy_track_index, const debug_track_writer& raw_tracks_writer, const debug_track_writer& lossy_tracks_writer)
		{
			rtm::vector4f error;
			switch (track_type)
			{
			case track_type8::float1f:
			{
				const float raw_value = raw_tracks_writer.read_float1(raw_track_index);
				const float lossy_value = lossy_tracks_writer.read_float1(lossy_track_index);
				error = rtm::vector_set(rtm::scalar_abs(raw_value - lossy_value));
				break;
			}
			case track_type8::float2f:
			{
				const rtm::vector4f raw_value = raw_tracks_writer.read_float2(raw_track_index);
				const rtm::vector4f lossy_value = lossy_tracks_writer.read_float2(lossy_track_index);
				error = rtm::vector_abs(rtm::vector_sub(raw_value, lossy_value));
				error = rtm::vector_mix<rtm::mix4::x, rtm::mix4::y, rtm::mix4::c, rtm::mix4::d>(error, rtm::vector_zero());
				break;
			}
			case track_type8::float3f:
			{
				const rtm::vector4f raw_value = raw_tracks_writer.read_float3(raw_track_index);
				const rtm::vector4f lossy_value = lossy_tracks_writer.read_float3(lossy_track_index);
				error = rtm::vector_abs(rtm::vector_sub(raw_value, lossy_value));
				error = rtm::vector_mix<rtm::mix4::x, rtm::mix4::y, rtm::mix4::z, rtm::mix4::d>(error, rtm::vector_zero());
				break;
			}
			case track_type8::float4f:
			{
				const rtm::vector4f raw_value = raw_tracks_writer.read_float4(raw_track_index);
				const rtm::vector4f lossy_value = lossy_tracks_writer.read_float4(lossy_track_index);
				error = rtm::vector_abs(rtm::vector_sub(raw_value, lossy_value));
				break;
			}
			case track_type8::vector4f:
			{
				const rtm::vector4f raw_value = raw_tracks_writer.read_vector4(raw_track_index);
				const rtm::vector4f lossy_value = lossy_tracks_writer.read_vector4(lossy_track_index);
				error = rtm::vector_abs(rtm::vector_sub(raw_value, lossy_value));
				break;
			}
			case track_type8::qvvf:
			default:
				ACL_ASSERT(false, "Unsupported track type");
				error = rtm::vector_zero();
				break;
			}

			return error;
		}

		struct ierror_calculation_adapter
		{
			virtual ~ierror_calculation_adapter() = default;

			// Scalar and transforms, mandatory
			virtual void sample_tracks0(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) = 0;
			virtual void sample_tracks1(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) = 0;

			// Scalar and transforms, optional
			virtual uint32_t get_output_index(uint32_t track_index) { return track_index; }

			// Transforms only, mandatory
			virtual void initialize_bind_pose(debug_track_writer& tracks_writer0, debug_track_writer& tracks_writer1) { (void)tracks_writer0; (void)tracks_writer1; }
			virtual uint32_t get_parent_index(uint32_t track_index) { (void)track_index; return k_invalid_track_index; }
			virtual float get_shell_distance(uint32_t track_index) { (void)track_index; return 0.0F; }

			// Transforms only, optional
			virtual void remap_output(debug_track_writer& track_writer0, debug_track_writer& track_writer1, debug_track_writer& track_writer_remapped)
			{
				(void)track_writer0;
				std::memcpy(track_writer_remapped.tracks_typed.qvvf, track_writer1.tracks_typed.qvvf, sizeof(rtm::qvvf) * track_writer1.num_tracks);
			}

			// Transform only, optional, additive base support
			virtual bool has_additive_base() const { return false; }
			virtual void sample_tracks_base(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer)
			{
				(void)sample_time;
				(void)rounding_policy;
				(void)track_writer;
			}
		};

		struct calculate_track_error_args
		{
			explicit calculate_track_error_args(ierror_calculation_adapter& adapter_)
				: adapter(adapter_)
			{
			}

			// Cannot copy or move
			calculate_track_error_args(const calculate_track_error_args&) = delete;
			calculate_track_error_args& operator=(const calculate_track_error_args&) = delete;

			// Scalar and transforms
			ierror_calculation_adapter& adapter;
			uint32_t num_samples = 0;
			uint32_t num_tracks = 0;
			float duration = 0.0F;
			float sample_rate = 0.0F;
			track_type8 track_type = track_type8::float1f;
			sample_rounding_policy rounding_policy = sample_rounding_policy::nearest;

			// Transforms only
			const itransform_error_metric* error_metric = nullptr;

			// Optional
			uint32_t base_num_samples = 0;
			float base_duration = 0.0F;
		};

		inline track_error calculate_scalar_track_error(iallocator& allocator, const calculate_track_error_args& args)
		{
			const uint32_t num_samples = args.num_samples;
			if (num_samples == 0)
				return track_error();	// Cannot measure any error

			const uint32_t num_tracks = args.num_tracks;
			if (num_tracks == 0)
				return track_error();	// Cannot measure any error

			track_error result;
			result.error = -1.0F;		// Can never have a negative error, use -1 so the first sample is used

			const float duration = args.duration;
			const float sample_rate = args.sample_rate;
			const track_type8 track_type = args.track_type;
			const sample_rounding_policy rounding_policy = args.rounding_policy;

			debug_track_writer tracks_writer0(allocator, track_type, num_tracks);
			debug_track_writer tracks_writer1(allocator, track_type, num_tracks);

			ACL_ASSERT(track_type != track_type8::qvvf, "Don't use calculate_scalar_track_error on track_type8::qvvf");

			// Measure our error
			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				const float sample_time = rtm::scalar_min(float(sample_index) / sample_rate, duration);

				args.adapter.sample_tracks0(sample_time, rounding_policy, tracks_writer0);
				args.adapter.sample_tracks1(sample_time, rounding_policy, tracks_writer1);

				// Validate decompress_tracks
				for (uint32_t track_index = 0; track_index < num_tracks; ++track_index)
				{
					const uint32_t output_index = args.adapter.get_output_index(track_index);
					if (output_index == k_invalid_track_index)
						continue;	// Track is being stripped, ignore it

					const rtm::vector4f error = get_scalar_track_error(track_type, track_index, output_index, tracks_writer0, tracks_writer1);

					const float max_error = rtm::vector_get_max_component(error);
					if (max_error > result.error)
					{
						result.error = max_error;
						result.index = track_index;
						result.sample_time = sample_time;
					}
				}
			}

			return result;
		}

		inline track_error calculate_transform_track_error(iallocator& allocator, const calculate_track_error_args& args)
		{
			ACL_ASSERT(args.error_metric != nullptr, "Must have an error metric");

			const uint32_t num_samples = args.num_samples;
			if (num_samples == 0)
				return track_error();	// Cannot measure any error

			const uint32_t num_tracks = args.num_tracks;
			if (num_tracks == 0)
				return track_error();	// Cannot measure any error

			const float clip_duration = args.duration;
			const float sample_rate = args.sample_rate;
			const itransform_error_metric& error_metric = *args.error_metric;
			const uint32_t additive_num_samples = args.base_num_samples;
			const float additive_duration = args.base_duration;
			const sample_rounding_policy rounding_policy = args.rounding_policy;
			const bool has_additive_base = args.adapter.has_additive_base();

			// Always calculate the error with scale, slower but we don't need to know if we have scale or not
			const bool has_scale = true;

			debug_track_writer tracks_writer0(allocator, track_type8::qvvf, num_tracks);
			debug_track_writer tracks_writer1(allocator, track_type8::qvvf, num_tracks);

			args.adapter.initialize_bind_pose(tracks_writer0, tracks_writer1);

			debug_track_writer tracks_writer1_remapped(allocator, track_type8::qvvf, num_tracks);
			debug_track_writer tracks_writer_base(allocator, track_type8::qvvf, num_tracks);

			const size_t transform_size = error_metric.get_transform_size(has_scale);
			const bool needs_conversion = error_metric.needs_conversion(has_scale);
			uint8_t* raw_local_pose_converted = nullptr;
			uint8_t* base_local_pose_converted = nullptr;
			uint8_t* lossy_local_pose_converted = nullptr;
			if (needs_conversion)
			{
				raw_local_pose_converted = allocate_type_array_aligned<uint8_t>(allocator, num_tracks * transform_size, 64);
				base_local_pose_converted = allocate_type_array_aligned<uint8_t>(allocator, num_tracks * transform_size, 64);
				lossy_local_pose_converted = allocate_type_array_aligned<uint8_t>(allocator, num_tracks * transform_size, 64);
			}

			uint8_t* raw_object_pose = allocate_type_array_aligned<uint8_t>(allocator, num_tracks * transform_size, 64);
			uint8_t* lossy_object_pose = allocate_type_array_aligned<uint8_t>(allocator, num_tracks * transform_size, 64);

			uint32_t* parent_transform_indices = allocate_type_array<uint32_t>(allocator, num_tracks);
			uint32_t* self_transform_indices = allocate_type_array<uint32_t>(allocator, num_tracks);

			for (uint32_t transform_index = 0; transform_index < num_tracks; ++transform_index)
			{
				const uint32_t parent_index = args.adapter.get_parent_index(transform_index);
				parent_transform_indices[transform_index] = parent_index;
				self_transform_indices[transform_index] = transform_index;
			}

			void* raw_local_pose_ = needs_conversion ? (void*)raw_local_pose_converted : (void*)tracks_writer0.tracks_typed.qvvf;
			const void* base_local_pose_ = needs_conversion ? (void*)base_local_pose_converted : (void*)tracks_writer_base.tracks_typed.qvvf;
			void* lossy_local_pose_ = needs_conversion ? (void*)lossy_local_pose_converted : (void*)tracks_writer1_remapped.tracks_typed.qvvf;

			itransform_error_metric::convert_transforms_args convert_transforms_args_raw;
			convert_transforms_args_raw.dirty_transform_indices = self_transform_indices;
			convert_transforms_args_raw.num_dirty_transforms = num_tracks;
			convert_transforms_args_raw.transforms = tracks_writer0.tracks_typed.qvvf;
			convert_transforms_args_raw.num_transforms = num_tracks;
			convert_transforms_args_raw.sample_index = 0;
			convert_transforms_args_raw.is_lossy = false;
			convert_transforms_args_raw.is_additive_base = false;

			itransform_error_metric::convert_transforms_args convert_transforms_args_base = convert_transforms_args_raw;
			convert_transforms_args_base.transforms = tracks_writer_base.tracks_typed.qvvf;
			convert_transforms_args_base.is_additive_base = true;

			itransform_error_metric::convert_transforms_args convert_transforms_args_lossy = convert_transforms_args_raw;
			convert_transforms_args_lossy.transforms = tracks_writer1_remapped.tracks_typed.qvvf;
			convert_transforms_args_lossy.is_lossy = true;

			itransform_error_metric::apply_additive_to_base_args apply_additive_to_base_args_raw;
			apply_additive_to_base_args_raw.dirty_transform_indices = self_transform_indices;
			apply_additive_to_base_args_raw.num_dirty_transforms = num_tracks;
			apply_additive_to_base_args_raw.local_transforms = raw_local_pose_;
			apply_additive_to_base_args_raw.base_transforms = base_local_pose_;
			apply_additive_to_base_args_raw.num_transforms = num_tracks;

			itransform_error_metric::apply_additive_to_base_args apply_additive_to_base_args_lossy = apply_additive_to_base_args_raw;
			apply_additive_to_base_args_lossy.local_transforms = lossy_local_pose_;

			itransform_error_metric::local_to_object_space_args local_to_object_space_args_raw;
			local_to_object_space_args_raw.dirty_transform_indices = self_transform_indices;
			local_to_object_space_args_raw.num_dirty_transforms = num_tracks;
			local_to_object_space_args_raw.parent_transform_indices = parent_transform_indices;
			local_to_object_space_args_raw.local_transforms = raw_local_pose_;
			local_to_object_space_args_raw.num_transforms = num_tracks;

			itransform_error_metric::local_to_object_space_args local_to_object_space_args_lossy = local_to_object_space_args_raw;
			local_to_object_space_args_lossy.local_transforms = lossy_local_pose_;

			track_error result;
			result.error = -1.0F;		// Can never have a negative error, use -1 so the first sample is used

			for (uint32_t sample_index = 0; sample_index < num_samples; ++sample_index)
			{
				const float sample_time = rtm::scalar_min(float(sample_index) / sample_rate, clip_duration);

				// Sample our tracks
				args.adapter.sample_tracks0(sample_time, rounding_policy, tracks_writer0);
				args.adapter.sample_tracks1(sample_time, rounding_policy, tracks_writer1);

				// Maybe remap them
				args.adapter.remap_output(tracks_writer0, tracks_writer1, tracks_writer1_remapped);

				if (needs_conversion)
				{
					convert_transforms_args_raw.sample_index = sample_index;
					convert_transforms_args_lossy.sample_index = sample_index;
					error_metric.convert_transforms(convert_transforms_args_raw, raw_local_pose_converted);
					error_metric.convert_transforms(convert_transforms_args_lossy, lossy_local_pose_converted);
				}

				if (has_additive_base)
				{
					const float normalized_sample_time = additive_num_samples > 1 ? (sample_time / clip_duration) : 0.0F;
					const float additive_sample_time = additive_num_samples > 1 ? (normalized_sample_time * additive_duration) : 0.0F;
					args.adapter.sample_tracks_base(additive_sample_time, rounding_policy, tracks_writer_base);

					if (needs_conversion)
					{
						const uint32_t nearest_base_sample_index = static_cast<uint32_t>(rtm::scalar_round_bankers(normalized_sample_time * float(additive_num_samples)));
						convert_transforms_args_base.sample_index = nearest_base_sample_index;
						error_metric.convert_transforms(convert_transforms_args_base, base_local_pose_converted);
					}

					error_metric.apply_additive_to_base(apply_additive_to_base_args_raw, raw_local_pose_);
					error_metric.apply_additive_to_base(apply_additive_to_base_args_lossy, lossy_local_pose_);
				}

				error_metric.local_to_object_space(local_to_object_space_args_raw, raw_object_pose);
				error_metric.local_to_object_space(local_to_object_space_args_lossy, lossy_object_pose);

				for (uint32_t bone_index = 0; bone_index < num_tracks; ++bone_index)
				{
					const float shell_distance = args.adapter.get_shell_distance(bone_index);

					itransform_error_metric::calculate_error_args calculate_error_args;
					calculate_error_args.transform0 = raw_object_pose + (bone_index * transform_size);
					calculate_error_args.transform1 = lossy_object_pose + (bone_index * transform_size);
					calculate_error_args.construct_sphere_shell(shell_distance);

					const float error = rtm::scalar_cast(error_metric.calculate_error(calculate_error_args));

					if (error > result.error)
					{
						result.error = error;
						result.index = bone_index;
						result.sample_time = sample_time;
					}
				}
			}

			deallocate_type_array(allocator, raw_local_pose_converted, num_tracks * transform_size);
			deallocate_type_array(allocator, base_local_pose_converted, num_tracks * transform_size);
			deallocate_type_array(allocator, lossy_local_pose_converted, num_tracks * transform_size);
			deallocate_type_array(allocator, raw_object_pose, num_tracks * transform_size);
			deallocate_type_array(allocator, lossy_object_pose, num_tracks * transform_size);
			deallocate_type_array(allocator, parent_transform_indices, num_tracks);
			deallocate_type_array(allocator, self_transform_indices, num_tracks);

			return result;
		}

		inline track_error invalid_track_error()
		{
			track_error result;
			result.index = ~0U;
			result.error = -1.0F;
			result.sample_time = -1.0F;
			return result;
		}
	}

	template<class decompression_context_type, acl_impl::is_decompression_context<decompression_context_type>>
	inline track_error calculate_compression_error(iallocator& allocator, const track_array& raw_tracks, decompression_context_type& context)
	{
		using namespace acl_impl;

		ACL_ASSERT(raw_tracks.is_valid().empty(), "Raw tracks are invalid");
		ACL_ASSERT(context.is_initialized(), "Context isn't initialized");

		if (raw_tracks.get_track_type() == track_type8::qvvf)
			return invalid_track_error();	// Only supports scalar tracks

		struct error_calculation_adapter final : public ierror_calculation_adapter
		{
			const track_array& raw_tracks_;
			decompression_context_type& context_;

			error_calculation_adapter(const track_array& raw_tracks__, decompression_context_type& context__)
				: raw_tracks_(raw_tracks__)
				, context_(context__)
			{
			}

			// Cannot copy or move
			error_calculation_adapter(const error_calculation_adapter&) = delete;
			error_calculation_adapter& operator=(const error_calculation_adapter&) = delete;

			virtual void sample_tracks0(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				raw_tracks_.sample_tracks(sample_time, rounding_policy, track_writer);
			}

			virtual void sample_tracks1(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				context_.seek(sample_time, rounding_policy);
				context_.decompress_tracks(track_writer);
			}

			virtual uint32_t get_output_index(uint32_t track_index) override
			{
				const track& track_ = raw_tracks_[track_index];
				return track_.get_output_index();
			}
		};

		error_calculation_adapter adapter(raw_tracks, context);

		calculate_track_error_args args(adapter);
		args.num_samples = raw_tracks.get_num_samples_per_track();
		args.num_tracks = raw_tracks.get_num_tracks();
		args.duration = raw_tracks.get_finite_duration();
		args.sample_rate = raw_tracks.get_sample_rate();
		args.track_type = raw_tracks.get_track_type();

		// We use the nearest sample to accurately measure the loss that happened, if any but only if all data is loaded
		// If we have a database with some data missing, we can't use the nearest samples, we have to interpolate
		// TODO: Check if all the data is loaded, always interpolate for now
		const compressed_tracks& tracks = *context.get_compressed_tracks();
		if (tracks.has_database() || tracks.has_stripped_keyframes())
			args.rounding_policy = sample_rounding_policy::none;
		else
			args.rounding_policy = sample_rounding_policy::nearest;

		return calculate_scalar_track_error(allocator, args);
	}

	template<class decompression_context_type, acl_impl::is_decompression_context<decompression_context_type>>
	inline track_error calculate_compression_error(iallocator& allocator, const track_array& raw_tracks, decompression_context_type& context, const itransform_error_metric& error_metric)
	{
		using namespace acl_impl;

		ACL_ASSERT(raw_tracks.is_valid().empty(), "Raw tracks are invalid");
		ACL_ASSERT(context.is_initialized(), "Context isn't initialized");

		struct error_calculation_adapter final : public ierror_calculation_adapter
		{
			const track_array& raw_tracks_;
			decompression_context_type& context_;
			iallocator& allocator_;

			uint32_t* output_bone_mapping;
			uint32_t num_output_bones;

			error_calculation_adapter(iallocator& allocator__, const track_array& raw_tracks__, decompression_context_type& context__)
				: raw_tracks_(raw_tracks__)
				, context_(context__)
				, allocator_(allocator__)
			{
				output_bone_mapping = create_output_track_mapping(allocator__, raw_tracks__, num_output_bones);
			}

			virtual ~error_calculation_adapter() override
			{
				deallocate_type_array(allocator_, output_bone_mapping, num_output_bones);
			}

			// Cannot copy or move
			error_calculation_adapter(const error_calculation_adapter&) = delete;
			error_calculation_adapter& operator=(const error_calculation_adapter&) = delete;

			virtual void initialize_bind_pose(debug_track_writer& track_writer0, debug_track_writer& tracks_writer1) override
			{
				(void)track_writer0;
				tracks_writer1.initialize_with_defaults(raw_tracks_);
			}

			virtual void sample_tracks0(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				raw_tracks_.sample_tracks(sample_time, rounding_policy, track_writer);
			}

			virtual void sample_tracks1(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				context_.seek(sample_time, rounding_policy);
				context_.decompress_tracks(track_writer);
			}

			virtual uint32_t get_output_index(uint32_t track_index) override
			{
				const track& track_ = raw_tracks_[track_index];
				return track_.get_output_index();
			}

			virtual uint32_t get_parent_index(uint32_t track_index) override
			{
				const track_qvvf& track_ = track_cast<track_qvvf>(raw_tracks_[track_index]);
				return track_.get_description().parent_index;
			}

			virtual float get_shell_distance(uint32_t track_index) override
			{
				const track_qvvf& track_ = track_cast<track_qvvf>(raw_tracks_[track_index]);
				return track_.get_description().shell_distance;
			}

			virtual void remap_output(debug_track_writer& track_writer0, debug_track_writer& track_writer1, debug_track_writer& track_writer_remapped) override
			{
				// Perform remapping by copying the raw pose first and we overwrite with the decompressed pose if
				// the data is available
				std::memcpy(track_writer_remapped.tracks_typed.qvvf, track_writer0.tracks_typed.qvvf, sizeof(rtm::qvvf) * track_writer_remapped.num_tracks);
				for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
				{
					const uint32_t bone_index = output_bone_mapping[output_index];
					track_writer_remapped.tracks_typed.qvvf[bone_index] = track_writer1.tracks_typed.qvvf[output_index];
				}
			}
		};

		error_calculation_adapter adapter(allocator, raw_tracks, context);

		calculate_track_error_args args(adapter);
		args.num_samples = raw_tracks.get_num_samples_per_track();
		args.num_tracks = raw_tracks.get_num_tracks();
		args.duration = raw_tracks.get_finite_duration();
		args.sample_rate = raw_tracks.get_sample_rate();
		args.track_type = raw_tracks.get_track_type();

		// We use the nearest sample to accurately measure the loss that happened, if any but only if all data is loaded
		// If we have a database with some data missing, we can't use the nearest samples, we have to interpolate
		// TODO: Check if all the data is loaded, always interpolate for now
		const compressed_tracks& tracks = *context.get_compressed_tracks();
		if (tracks.has_database() || tracks.has_stripped_keyframes())
			args.rounding_policy = sample_rounding_policy::none;
		else
			args.rounding_policy = sample_rounding_policy::nearest;

		if (raw_tracks.get_track_type() != track_type8::qvvf)
			return calculate_scalar_track_error(allocator, args);

		args.error_metric = &error_metric;

		return calculate_transform_track_error(allocator, args);
	}

	template<class decompression_context_type, acl_impl::is_decompression_context<decompression_context_type>>
	inline track_error calculate_compression_error(iallocator& allocator, const track_array_qvvf& raw_tracks, decompression_context_type& context, const itransform_error_metric& error_metric, const track_array_qvvf& additive_base_tracks)
	{
		using namespace acl_impl;

		ACL_ASSERT(raw_tracks.is_valid().empty(), "Raw tracks are invalid");
		ACL_ASSERT(context.is_initialized(), "Context isn't initialized");

		struct error_calculation_adapter final : public ierror_calculation_adapter
		{
			const track_array& raw_tracks_;
			decompression_context_type& context_;
			const track_array_qvvf& additive_base_tracks_;

			iallocator& allocator_;
			uint32_t* output_bone_mapping;
			uint32_t num_output_bones;

			error_calculation_adapter(iallocator& allocator__, const track_array& raw_tracks__, decompression_context_type& context__, const track_array_qvvf& additive_base_tracks__)
				: raw_tracks_(raw_tracks__)
				, context_(context__)
				, additive_base_tracks_(additive_base_tracks__)
				, allocator_(allocator__)
			{
				output_bone_mapping = create_output_track_mapping(allocator__, raw_tracks__, num_output_bones);
			}

			virtual ~error_calculation_adapter() override
			{
				deallocate_type_array(allocator_, output_bone_mapping, num_output_bones);
			}

			// Cannot copy or move
			error_calculation_adapter(const error_calculation_adapter&) = delete;
			error_calculation_adapter& operator=(const error_calculation_adapter&) = delete;

			virtual void initialize_bind_pose(debug_track_writer& track_writer0, debug_track_writer& tracks_writer1) override
			{
				(void)track_writer0;
				tracks_writer1.initialize_with_defaults(raw_tracks_);
			}

			virtual void sample_tracks0(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				raw_tracks_.sample_tracks(sample_time, rounding_policy, track_writer);
			}

			virtual void sample_tracks1(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				context_.seek(sample_time, rounding_policy);
				context_.decompress_tracks(track_writer);
			}

			virtual uint32_t get_output_index(uint32_t track_index) override
			{
				const track& track_ = raw_tracks_[track_index];
				return track_.get_output_index();
			}

			virtual uint32_t get_parent_index(uint32_t track_index) override
			{
				const track_qvvf& track_ = track_cast<track_qvvf>(raw_tracks_[track_index]);
				return track_.get_description().parent_index;
			}

			virtual float get_shell_distance(uint32_t track_index) override
			{
				const track_qvvf& track_ = track_cast<track_qvvf>(raw_tracks_[track_index]);
				return track_.get_description().shell_distance;
			}

			virtual void remap_output(debug_track_writer& track_writer0, debug_track_writer& track_writer1, debug_track_writer& track_writer_remapped) override
			{
				// Perform remapping by copying the raw pose first and we overwrite with the decompressed pose if
				// the data is available
				std::memcpy(track_writer_remapped.tracks_typed.qvvf, track_writer0.tracks_typed.qvvf, sizeof(rtm::qvvf) * track_writer_remapped.num_tracks);
				for (uint32_t output_index = 0; output_index < num_output_bones; ++output_index)
				{
					const uint32_t bone_index = output_bone_mapping[output_index];
					track_writer_remapped.tracks_typed.qvvf[bone_index] = track_writer1.tracks_typed.qvvf[output_index];
				}
			}

			virtual bool has_additive_base() const override { return !additive_base_tracks_.is_empty(); }
			virtual void sample_tracks_base(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				additive_base_tracks_.sample_tracks(sample_time, rounding_policy, track_writer);
			}
		};

		error_calculation_adapter adapter(allocator, raw_tracks, context, additive_base_tracks);

		calculate_track_error_args args(adapter);
		args.num_samples = raw_tracks.get_num_samples_per_track();
		args.num_tracks = raw_tracks.get_num_tracks();
		args.duration = raw_tracks.get_finite_duration();
		args.sample_rate = raw_tracks.get_sample_rate();
		args.track_type = raw_tracks.get_track_type();

		// We use the nearest sample to accurately measure the loss that happened, if any but only if all data is loaded
		// If we have a database with some data missing, we can't use the nearest samples, we have to interpolate
		// TODO: Check if all the data is loaded, always interpolate for now
		const compressed_tracks& tracks = *context.get_compressed_tracks();
		if (tracks.has_database() || tracks.has_stripped_keyframes())
			args.rounding_policy = sample_rounding_policy::none;
		else
			args.rounding_policy = sample_rounding_policy::nearest;

		args.error_metric = &error_metric;

		if (!additive_base_tracks.is_empty())
		{
			args.base_num_samples = additive_base_tracks.get_num_samples_per_track();
			args.base_duration = additive_base_tracks.get_finite_duration();
		}

		return calculate_transform_track_error(allocator, args);
	}

	template<class decompression_context_type0, class decompression_context_type1, acl_impl::is_decompression_context<decompression_context_type0>, acl_impl::is_decompression_context<decompression_context_type1>>
	inline track_error calculate_compression_error(iallocator& allocator, decompression_context_type0& context0, decompression_context_type1& context1)
	{
		using namespace acl_impl;

		ACL_ASSERT(context0.is_initialized(), "Context isn't initialized");
		ACL_ASSERT(context1.is_initialized(), "Context isn't initialized");

		const compressed_tracks* tracks0 = context0.get_compressed_tracks();

		if (tracks0->get_track_type() == track_type8::qvvf)
			return invalid_track_error();	// Only supports scalar tracks

		struct error_calculation_adapter final : public ierror_calculation_adapter
		{
			decompression_context_type0& context0_;
			decompression_context_type1& context1_;

			error_calculation_adapter(decompression_context_type0& context0__, decompression_context_type1& context1__)
				: context0_(context0__)
				, context1_(context1__)
			{
			}

			// Cannot copy or move
			error_calculation_adapter(const error_calculation_adapter&) = delete;
			error_calculation_adapter& operator=(const error_calculation_adapter&) = delete;

			virtual void sample_tracks0(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				context0_.seek(sample_time, rounding_policy);
				context0_.decompress_tracks(track_writer);
			}

			virtual void sample_tracks1(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				context1_.seek(sample_time, rounding_policy);
				context1_.decompress_tracks(track_writer);
			}
		};

		error_calculation_adapter adapter(context0, context1);

		calculate_track_error_args args(adapter);
		args.num_samples = tracks0->get_num_samples_per_track();
		args.num_tracks = tracks0->get_num_tracks();
		args.duration = tracks0->get_finite_duration();
		args.sample_rate = tracks0->get_sample_rate();
		args.track_type = tracks0->get_track_type();

		// We use the nearest sample to accurately measure the loss that happened, if any but only if all data is loaded
		// If we have a database with some data missing, we can't use the nearest samples, we have to interpolate
		// TODO: Check if all the data is loaded, always interpolate for now
		const compressed_tracks* tracks1 = context1.get_compressed_tracks();
		if (tracks0->has_database() || tracks1->has_database() || tracks0->has_stripped_keyframes() || tracks1->has_stripped_keyframes())
			args.rounding_policy = sample_rounding_policy::none;
		else
			args.rounding_policy = sample_rounding_policy::nearest;

		return calculate_scalar_track_error(allocator, args);
	}

	inline track_error calculate_compression_error(iallocator& allocator, const track_array& raw_tracks0, const track_array& raw_tracks1)
	{
		using namespace acl_impl;

		ACL_ASSERT(raw_tracks0.is_valid().empty(), "Raw tracks are invalid");
		ACL_ASSERT(raw_tracks1.is_valid().empty(), "Raw tracks are invalid");

		if (raw_tracks0.get_track_type() == track_type8::qvvf)
			return invalid_track_error();	// Only supports scalar tracks

		struct error_calculation_adapter final : public ierror_calculation_adapter
		{
			const track_array& raw_tracks0_;
			const track_array& raw_tracks1_;

			error_calculation_adapter(const track_array& raw_tracks0__, const track_array& raw_tracks1__)
				: raw_tracks0_(raw_tracks0__)
				, raw_tracks1_(raw_tracks1__)
			{
			}

			// Cannot copy or move
			error_calculation_adapter(const error_calculation_adapter&) = delete;
			error_calculation_adapter& operator=(const error_calculation_adapter&) = delete;

			virtual void sample_tracks0(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				raw_tracks0_.sample_tracks(sample_time, rounding_policy, track_writer);
			}

			virtual void sample_tracks1(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				raw_tracks1_.sample_tracks(sample_time, rounding_policy, track_writer);
			}
		};

		error_calculation_adapter adapter(raw_tracks0, raw_tracks1);

		calculate_track_error_args args(adapter);
		args.num_samples = raw_tracks0.get_num_samples_per_track();
		args.num_tracks = raw_tracks0.get_num_tracks();
		args.duration = raw_tracks0.get_finite_duration();
		args.sample_rate = raw_tracks0.get_sample_rate();
		args.track_type = raw_tracks0.get_track_type();

		// We use the nearest sample to accurately measure the loss that happened, if any
		args.rounding_policy = sample_rounding_policy::nearest;

		return calculate_scalar_track_error(allocator, args);
	}

	inline track_error calculate_compression_error(iallocator& allocator, const track_array& raw_tracks0, const track_array& raw_tracks1, const itransform_error_metric& error_metric)
	{
		using namespace acl_impl;

		ACL_ASSERT(raw_tracks0.is_valid().empty(), "Raw tracks are invalid");
		ACL_ASSERT(raw_tracks1.is_valid().empty(), "Raw tracks are invalid");

		struct error_calculation_adapter final : public ierror_calculation_adapter
		{
			const track_array& raw_tracks0_;
			const track_array& raw_tracks1_;

			error_calculation_adapter(const track_array& raw_tracks0__, const track_array& raw_tracks1__)
				: raw_tracks0_(raw_tracks0__)
				, raw_tracks1_(raw_tracks1__)
			{
			}

			// Cannot copy or move
			error_calculation_adapter(const error_calculation_adapter&) = delete;
			error_calculation_adapter& operator=(const error_calculation_adapter&) = delete;

			virtual void sample_tracks0(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				raw_tracks0_.sample_tracks(sample_time, rounding_policy, track_writer);
			}

			virtual void sample_tracks1(float sample_time, sample_rounding_policy rounding_policy, debug_track_writer& track_writer) override
			{
				raw_tracks1_.sample_tracks(sample_time, rounding_policy, track_writer);
			}

			virtual uint32_t get_parent_index(uint32_t track_index) override
			{
				const track_qvvf& track_ = track_cast<track_qvvf>(raw_tracks0_[track_index]);
				return track_.get_description().parent_index;
			}

			virtual float get_shell_distance(uint32_t track_index) override
			{
				const track_qvvf& track_ = track_cast<track_qvvf>(raw_tracks0_[track_index]);
				return track_.get_description().shell_distance;
			}
		};

		error_calculation_adapter adapter(raw_tracks0, raw_tracks1);

		calculate_track_error_args args(adapter);
		args.num_samples = raw_tracks0.get_num_samples_per_track();
		args.num_tracks = raw_tracks0.get_num_tracks();
		args.duration = raw_tracks0.get_finite_duration();
		args.sample_rate = raw_tracks0.get_sample_rate();
		args.track_type = raw_tracks0.get_track_type();

		// We use the nearest sample to accurately measure the loss that happened, if any
		args.rounding_policy = sample_rounding_policy::nearest;

		if (raw_tracks0.get_track_type() != track_type8::qvvf)
			return calculate_scalar_track_error(allocator, args);

		args.error_metric = &error_metric;

		return calculate_transform_track_error(allocator, args);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
