#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
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

#if defined(ACL_USE_SJSON)

#include "acl/version.h"
#include "acl/io/clip_reader_error.h"
#include "acl/compression/compression_settings.h"
#include "acl/compression/track_array.h"
#include "acl/core/algorithm_types.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/iallocator.h"
#include "acl/core/string.h"
#include "acl/core/track_desc.h"
#include "acl/core/unique_ptr.h"
#include "acl/math/quatf.h"

#include <rtm/quatd.h>
#include <rtm/quatf.h>
#include <rtm/vector4d.h>
#include <rtm/vector4f.h>
#include <rtm/qvvd.h>
#include <rtm/qvvf.h>

#include <sjson/parser.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Enum to describe each type of raw content that an SJSON ACL file might contain.
	enum class sjson_file_type
	{
		unknown,
		raw_clip,
		raw_track_list,
	};

	// Disable warning for implicit constructor using deprecated members
	// Also remove warning suppression added for RTM_COMPILER_MSVC_2015 in acl_compressor.cpp and test_reader_writer.cpp
#if defined(RTM_COMPILER_CLANG)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

	//////////////////////////////////////////////////////////////////////////
	// Raw transform tracks
	struct sjson_raw_clip
	{
		track_array_qvvf track_list;

		track_array_qvvf additive_base_track_list;
		additive_clip_format8 additive_format = additive_clip_format8::none;

		ACL_DEPRECATED("No longer used, see track description, to be removed in v3.0")
		track_qvvf bind_pose;

		bool has_settings = false;
		compression_settings settings;

		sjson_raw_clip() = default;

		// Can't copy
		sjson_raw_clip(const sjson_raw_clip&) = delete;
		sjson_raw_clip& operator=(const sjson_raw_clip&) = delete;

		// Can move
		sjson_raw_clip(sjson_raw_clip&&) = default;
		sjson_raw_clip& operator=(sjson_raw_clip&&) = default;
	};

	//////////////////////////////////////////////////////////////////////////
	// Raw scalar tracks
	struct sjson_raw_track_list
	{
		track_array track_list;

		ACL_DEPRECATED("No longer used, see track description, to be removed in v3.0")
		track_qvvf bind_pose;

		bool has_settings = false;
		compression_settings settings;

		sjson_raw_track_list() = default;

		// Can't copy
		sjson_raw_track_list(const sjson_raw_track_list&) = delete;
		sjson_raw_track_list& operator=(const sjson_raw_track_list&) = delete;

		// Can move
		sjson_raw_track_list(sjson_raw_track_list&&) = default;
		sjson_raw_track_list& operator=(sjson_raw_track_list&&) = default;
	};

#if defined(RTM_COMPILER_CLANG)
	#pragma clang diagnostic pop
#elif defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic pop
#endif

	//////////////////////////////////////////////////////////////////////////
	// An SJSON ACL file reader.
	class clip_reader
	{
	public:
		clip_reader(iallocator& allocator, const char* sjson_input, size_t input_length)
			: m_allocator(allocator)
			, m_parser(sjson_input, input_length)
		{
		}

		~clip_reader()
		{
			deallocate_type_array(m_allocator, m_bone_names, m_num_bones);
		}

		sjson_file_type get_file_type()
		{
			reset_state();

			if (!read_version())
				return sjson_file_type::unknown;

			if (m_parser.object_begins("clip"))
				return sjson_file_type::raw_clip;

			if (m_parser.object_begins("track_list"))
				return sjson_file_type::raw_track_list;

			return sjson_file_type::unknown;
		}

		bool read_raw_clip(sjson_raw_clip& out_data)
		{
			reset_state();

			if (!read_version())
				return false;

			if (!read_raw_clip_header())
				return false;

			if (!read_settings(&out_data.has_settings, &out_data.settings))
				return false;

			if (!create_skeleton(out_data.track_list))
				return false;

			if (!read_tracks(out_data.track_list, out_data.additive_base_track_list))
				return false;

			out_data.additive_format = !out_data.additive_base_track_list.is_empty() ? m_additive_format : additive_clip_format8::none;

			return nothing_follows();
		}

		bool read_raw_track_list(sjson_raw_track_list& out_data)
		{
			reset_state();

			if (!read_version())
				return false;

			if (!read_raw_track_list_header())
				return false;

			if (!read_settings(&out_data.has_settings, &out_data.settings))
				return false;

			if (!create_track_list(out_data.track_list))
				return false;

			return nothing_follows();
		}

		clip_reader_error get_error() const { return m_error; }

	private:
		clip_reader(const clip_reader&) = delete;
		clip_reader& operator=(const clip_reader&) = delete;

		iallocator& m_allocator;
		sjson::Parser m_parser;
		clip_reader_error m_error;

		uint32_t m_version								= 0;
		uint32_t m_num_samples							= 0;
		float m_sample_rate								= 0.0F;
		sjson::StringView m_clip_name;
		bool m_is_binary_exact							= false;
		additive_clip_format8 m_additive_format			= additive_clip_format8::none;
		sjson::StringView m_additive_base_name;
		uint32_t m_additive_base_num_samples			= 0;
		float m_additive_base_sample_rate				= 0.0F;

		bool m_has_settings								= false;
		float m_error_threshold							= 0.0F;

		sjson::StringView* m_bone_names					= nullptr;
		uint32_t m_num_bones							= 0;

		string convert_string(const sjson::StringView& str) const
		{
			return string(m_allocator, str.c_str(), str.size());
		}

		void reset_state()
		{
			m_parser.reset_state();
			set_error(clip_reader_error::None);
		}

		bool read_version()
		{
			if (!m_parser.read("version", m_version))
			{
				m_error = m_parser.get_error();
				return false;
			}

			if (m_version > 5)
			{
				set_error(clip_reader_error::UnsupportedVersion);
				return false;
			}

			return true;
		}

		bool read_raw_clip_header()
		{
			sjson::StringView additive_format;

			if (!m_parser.object_begins("clip"))
				goto error;

			m_parser.try_read("name", m_clip_name, "");

			double num_samples;
			if (!m_parser.read("num_samples", num_samples))
				goto error;

			m_num_samples = static_cast<uint32_t>(num_samples);
			if (static_cast<double>(m_num_samples) != num_samples)
			{
				set_error(clip_reader_error::UnsignedIntegerExpected);
				return false;
			}

			double sample_rate;
			if (!m_parser.read("sample_rate", sample_rate))
				goto error;

			m_sample_rate = static_cast<float>(sample_rate);
			if (m_sample_rate <= 0.0F)
			{
				set_error(clip_reader_error::PositiveValueExpected);
				return false;
			}

			// Version 1 had an error_threshold field, skip it
			double error_threshold;
			if (m_version == 1 && !m_parser.read("error_threshold", error_threshold))
				goto error;

			// Optional value
			m_parser.try_read("is_binary_exact", m_is_binary_exact, false);

			// Optional value
			m_parser.try_read("additive_format", additive_format, "none");
			if (!get_additive_clip_format(additive_format.c_str(), m_additive_format))
			{
				set_error(clip_reader_error::InvalidAdditiveClipFormat);
				return false;
			}

			m_parser.try_read("additive_base_name", m_additive_base_name, "");
			m_parser.try_read("additive_base_num_samples", num_samples, 1.0);
			m_additive_base_num_samples = static_cast<uint32_t>(num_samples);
			if (static_cast<double>(m_additive_base_num_samples) != num_samples || m_additive_base_num_samples == 0)
			{
				set_error(clip_reader_error::UnsignedIntegerExpected);
				return false;
			}
			m_parser.try_read("additive_base_sample_rate", sample_rate, 30.0);
			m_additive_base_sample_rate = static_cast<float>(sample_rate);
			if (m_additive_base_sample_rate <= 0.0F)
			{
				set_error(clip_reader_error::PositiveValueExpected);
				return false;
			}

			if (!m_parser.object_ends())
				goto error;

			return true;

		error:
			m_error = m_parser.get_error();
			return false;
		}

		bool read_raw_track_list_header()
		{
			if (!m_parser.object_begins("track_list"))
				goto error;

			m_parser.try_read("name", m_clip_name, "");

			double num_samples;
			if (!m_parser.read("num_samples", num_samples))
				goto error;

			m_num_samples = static_cast<uint32_t>(num_samples);
			if (static_cast<double>(m_num_samples) != num_samples)
			{
				set_error(clip_reader_error::UnsignedIntegerExpected);
				return false;
			}

			double sample_rate;
			if (!m_parser.read("sample_rate", sample_rate))
				goto error;

			m_sample_rate = static_cast<float>(sample_rate);
			if (m_sample_rate <= 0.0F)
			{
				set_error(clip_reader_error::PositiveValueExpected);
				return false;
			}

			// Optional value
			m_parser.try_read("is_binary_exact", m_is_binary_exact, false);

			if (!m_parser.object_ends())
				goto error;

			return true;

		error:
			m_error = m_parser.get_error();
			return false;
		}

		bool read_settings(bool* out_has_settings, compression_settings* out_settings)
		{
			m_has_settings = false;

			if (!m_parser.try_object_begins("settings"))
			{
				if (out_has_settings != nullptr)
					*out_has_settings = false;

				// Settings are optional, all good
				return true;
			}

			compression_settings default_settings;

			sjson::StringView algorithm_name;
			sjson::StringView compression_level;
			sjson::StringView rotation_format;
			sjson::StringView translation_format;
			sjson::StringView scale_format;
			bool rotation_range_reduction;
			bool translation_range_reduction;
			bool scale_range_reduction;
			double constant_rotation_threshold_angle;
			double constant_translation_threshold;
			double constant_scale_threshold;
			double error_threshold;

			double segmenting_ideal_num_samples = 0.0;	// Legacy, no longer used
			double segmenting_max_num_samples = 0.0;	// Legacy, no longer used

			m_parser.try_read("algorithm_name", algorithm_name, get_algorithm_name(algorithm_type8::uniformly_sampled));
			m_parser.try_read("level", compression_level, get_compression_level_name(default_settings.level));
			m_parser.try_read("rotation_format", rotation_format, get_rotation_format_name(default_settings.rotation_format));
			m_parser.try_read("translation_format", translation_format, get_vector_format_name(default_settings.translation_format));
			m_parser.try_read("scale_format", scale_format, get_vector_format_name(default_settings.scale_format));
			m_parser.try_read("rotation_range_reduction", rotation_range_reduction, false);			// Legacy, no longer used
			m_parser.try_read("translation_range_reduction", translation_range_reduction, false);	// Legacy, no longer used
			m_parser.try_read("scale_range_reduction", scale_range_reduction, false);				// Legacy, no longer used

			if (m_parser.try_object_begins("segmenting"))
			{
				bool segmenting_enabled;
				bool segmenting_rotation_range_reduction;
				bool segmenting_translation_range_reduction;
				bool segmenting_scale_range_reduction;
				m_parser.try_read("enabled", segmenting_enabled, false);											// Legacy, no longer used
				m_parser.try_read("ideal_num_samples", segmenting_ideal_num_samples, 0.0);							// Legacy, no longer used
				m_parser.try_read("max_num_samples", segmenting_max_num_samples, 0.0);								// Legacy, no longer used
				m_parser.try_read("rotation_range_reduction", segmenting_rotation_range_reduction, false);			// Legacy, no longer used
				m_parser.try_read("translation_range_reduction", segmenting_translation_range_reduction, false);	// Legacy, no longer used
				m_parser.try_read("scale_range_reduction", segmenting_scale_range_reduction, false);				// Legacy, no longer used

				if (!m_parser.is_valid() || !m_parser.object_ends())
					goto parsing_error;
			}

			// Skip deprecated values
			m_parser.try_read("constant_rotation_threshold_angle", constant_rotation_threshold_angle, 0.00284714461);
			m_parser.try_read("constant_translation_threshold", constant_translation_threshold, 0.001);
			m_parser.try_read("constant_scale_threshold", constant_scale_threshold, 0.00001);
			m_parser.try_read("error_threshold", error_threshold, 0.01);

			if (!m_parser.is_valid() || !m_parser.object_ends())
				goto parsing_error;

			if (out_has_settings != nullptr)
			{
				*out_has_settings = true;

				if (!get_compression_level(compression_level.c_str(), out_settings->level))
					goto invalid_value_error;

				if (!get_rotation_format(rotation_format.c_str(), out_settings->rotation_format))
					goto invalid_value_error;

				if (!get_vector_format(translation_format.c_str(), out_settings->translation_format))
					goto invalid_value_error;

				if (!get_vector_format(scale_format.c_str(), out_settings->scale_format))
					goto invalid_value_error;

				m_error_threshold = float(error_threshold);
			}

			m_has_settings = true;
			return true;

		parsing_error:
			m_error = m_parser.get_error();
			return false;

		invalid_value_error:
			m_parser.get_position(m_error.line, m_error.column);
			m_error.error = clip_reader_error::InvalidCompressionSetting;
			return false;
		}

		bool create_skeleton(track_array_qvvf& track_list)
		{
			sjson::ParserState before_bones = m_parser.save_state();

			uint32_t num_bones;
			if (!process_each_bone(nullptr, num_bones))
				return false;

			m_parser.restore_state(before_bones);

			m_num_bones = num_bones;
			m_bone_names = allocate_type_array<sjson::StringView>(m_allocator, num_bones);

			track_list = track_array_qvvf(m_allocator, num_bones);
			track_list.set_name(convert_string(m_clip_name));

			const uint32_t num_allocated_bones = num_bones;

			if (!process_each_bone(&track_list, num_bones))
				return false;

			(void)num_allocated_bones;
			ACL_ASSERT(num_bones == num_allocated_bones, "Number of bones read mismatch");

			return true;
		}

		static double hex_to_double(const sjson::StringView& value)
		{
			union UInt64ToDouble
			{
				uint64_t u64;
				double dbl;

				constexpr explicit UInt64ToDouble(uint64_t u64_value) : u64(u64_value) {}
			};

			ACL_ASSERT(value.size() <= 16, "Invalid binary exact double value");
			uint64_t value_u64 = acl_impl::strtoull(value.c_str(), nullptr, 16);
			return UInt64ToDouble(value_u64).dbl;
		}

		static float hex_to_float(const sjson::StringView& value)
		{
			union UInt32ToFloat
			{
				uint32_t u32;
				float flt;

				constexpr explicit UInt32ToFloat(uint32_t u32_value) : u32(u32_value) {}
			};

			ACL_ASSERT(value.size() <= 8, "Invalid binary exact float value");
			uint32_t value_u32 = safe_static_cast<uint32_t>(std::strtoul(value.c_str(), nullptr, 16));
			return UInt32ToFloat(value_u32).flt;
		}

		static rtm::quatd hex_to_quat(const sjson::StringView values[4])
		{
			return rtm::quat_set(hex_to_double(values[0]), hex_to_double(values[1]), hex_to_double(values[2]), hex_to_double(values[3]));
		}

		static rtm::vector4d hex_to_vector3(const sjson::StringView values[3])
		{
			return rtm::vector_set(hex_to_double(values[0]), hex_to_double(values[1]), hex_to_double(values[2]));
		}

		static rtm::float4f hex_to_float4f(const sjson::StringView values[4], uint32_t num_components)
		{
			ACL_ASSERT(num_components <= 4, "Invalid number of components");

			rtm::float4f result = { 0.0F, 0.0F, 0.0F, 0.0F };
			float* result_ptr = &result.x;

			for (uint32_t component_index = 0; component_index < num_components; ++component_index)
				result_ptr[component_index] = hex_to_float(values[component_index]);

			return result;
		}

		bool process_each_bone(track_array_qvvf* tracks, uint32_t& num_bones)
		{
			bool counting = tracks == nullptr;
			num_bones = 0;

			if (!m_parser.array_begins("bones"))
				goto error;

			for (uint32_t i = 0; !m_parser.try_array_ends(); ++i)
			{
				if (!m_parser.object_begins())
					goto error;

				sjson::StringView name;
				if (!m_parser.read("name", name))
					goto error;

				if (tracks != nullptr)
					m_bone_names[i] = name;

				sjson::StringView parent;
				if (!m_parser.read("parent", parent))
					goto error;

				uint32_t parent_index = k_invalid_track_index;
				if (tracks != nullptr && !parent.empty())
				{
					parent_index = find_bone(parent);
					if (parent_index == k_invalid_track_index)
					{
						set_error(clip_reader_error::NoParentBoneWithThatName);
						return false;
					}
				}

				float vertex_distance;
				if (!m_parser.read("vertex_distance", vertex_distance))
					goto error;

				rtm::qvvd bind_transform = rtm::qvv_identity();
				if (m_is_binary_exact)
				{
					sjson::StringView rotation[4];
					if (m_parser.try_read("bind_rotation", rotation, 4, nullptr) && !counting)
						bind_transform.rotation = hex_to_quat(rotation);

					sjson::StringView translation[3];
					if (m_parser.try_read("bind_translation", translation, 3, nullptr) && !counting)
						bind_transform.translation = hex_to_vector3(translation);

					sjson::StringView scale[3];
					if (m_parser.try_read("bind_scale", scale, 3, nullptr) && !counting)
						bind_transform.scale = hex_to_vector3(scale);
				}
				else
				{
					double rotation[4] = { 0.0, 0.0, 0.0, 0.0 };
					if (m_parser.try_read("bind_rotation", rotation, 4, 0.0) && !counting)
						bind_transform.rotation = rtm::quat_load(&rotation[0]);

					double translation[3] = { 0.0, 0.0, 0.0 };
					if (m_parser.try_read("bind_translation", translation, 3, 0.0) && !counting)
						bind_transform.translation = rtm::vector_load3(&translation[0]);

					double scale[3] = { 0.0, 0.0, 0.0 };
					if (m_parser.try_read("bind_scale", scale, 3, 0.0) && !counting)
						bind_transform.scale = rtm::vector_load3(&scale[0]);
				}

				if (tracks != nullptr)
				{
					track_desc_transformf desc;
					desc.parent_index = parent_index;
					desc.shell_distance = vertex_distance;

					if (m_has_settings)
						desc.precision = m_error_threshold;

					// Create a dummy track for now to hold our arguments
					(*tracks)[i] = track_qvvf::make_ref(desc, nullptr, 0, 30.0F);
					(*tracks)[i].set_name(convert_string(name));

					rtm::qvvf bind_transform_ = rtm::qvv_cast(bind_transform);
					bind_transform_.rotation = rtm::quat_normalize_deterministic(bind_transform_.rotation);

					(*tracks)[i].get_description().default_value = bind_transform_;
				}

				if (!m_parser.object_ends())
					goto error;

				++num_bones;
			}

			return true;

		error:
			m_error = m_parser.get_error();
			return false;
		}

		uint32_t find_bone(const sjson::StringView& name) const
		{
			for (uint32_t i = 0; i < m_num_bones; ++i)
			{
				if (name == m_bone_names[i])
					return i;
			}

			return k_invalid_track_index;
		}

		float read_optional_float(const char* key, float default_value)
		{
			float value = default_value;
			if (m_is_binary_exact)
			{
				sjson::StringView value_str;
				if (m_parser.try_read(key, value_str, ""))
					value = hex_to_float(value_str);
			}
			else
				m_parser.try_read(key, value, default_value);

			return value;
		}

		bool read_qvv_sample(rtm::qvvf& out_sample)
		{
			rtm::qvvf sample = rtm::qvv_identity();
			sjson::StringView values_str[4];
			double values[4] = { 0.0, 0.0, 0.0, 0.0 };

			{
				if (!m_parser.array_begins())
					return false;

				rtm::float4f value;
				if (m_is_binary_exact)
				{
					if (!m_parser.read(values_str, 4))
						return false;

					value = hex_to_float4f(values_str, 4);
				}
				else
				{
					if (!m_parser.read(values, 4))
						return false;

					value = { static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]), static_cast<float>(values[3])};
				}

				sample.rotation = rtm::quat_load(&value);
				if (!rtm::quat_is_finite(sample.rotation))
					return false;

				if (!m_parser.array_ends())
					return false;
			}

			if (!m_parser.read_comma())
				return false;

			{
				if (!m_parser.array_begins())
					return false;

				rtm::float4f value;
				if (m_is_binary_exact)
				{
					if (!m_parser.read(values_str, 3))
						return false;

					value = hex_to_float4f(values_str, 4);
				}
				else
				{
					if (!m_parser.read(values, 3))
						return false;

					value = { static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]), 0.0F };
				}

				sample.translation = vector_load(&value);
				if (!rtm::vector_is_finite3(sample.translation))
					return false;

				if (!m_parser.array_ends())
					return false;
			}

			if (!m_parser.read_comma())
				return false;

			{
				if (!m_parser.array_begins())
					return false;

				rtm::float4f value;
				if (m_is_binary_exact)
				{
					if (!m_parser.read(values_str, 3))
						return false;

					value = hex_to_float4f(values_str, 4);
				}
				else
				{
					if (!m_parser.read(values, 3))
						return false;

					value = { static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]), 0.0F };
				}

				sample.scale = vector_load(&value);
				if (!rtm::vector_is_finite3(sample.scale))
					return false;

				if (!m_parser.array_ends())
					return false;
			}

			out_sample = sample;
			return true;
		}

		bool process_track_list(track* tracks, uint32_t& num_tracks)
		{
			const bool counting = tracks == nullptr;
			track dummy;
			track_type8 track_list_type = track_type8::float1f;

			num_tracks = 0;

			if (!m_parser.array_begins("tracks"))
				goto error;

			for (uint32_t i = 0; !m_parser.try_array_ends(); ++i)
			{
				track& track_ = counting ? dummy : tracks[i];

				if (!m_parser.object_begins())
					goto error;

				sjson::StringView name;
				m_parser.try_read("name", name, "");

				sjson::StringView type;
				if (!m_parser.read("type", type))
					goto error;

				track_type8 track_type;
				if (!get_track_type(type.c_str(), track_type))
				{
					m_error.error = clip_reader_error::InvalidTrackType;
					return false;
				}

				if (num_tracks == 0)
				{
					// Found our first track, this is the type we'll use for every track
					track_list_type = track_type;
				}
				else if (track_type != track_list_type)
				{
					// Track type doesn't match the type of the first track
					m_error.error = clip_reader_error::InvalidTrackType;
					return false;
				}

				const uint32_t num_components = get_track_num_sample_elements(track_type);
				ACL_ASSERT((num_components > 0 && num_components <= 4) || num_components == 12, "Must have between 1 and 4 components or 12");

				track_desc_scalarf scalar_desc;
				track_desc_transformf transform_desc;

				float precision = read_optional_float("precision", -1.0F);

				// Deprecated, no longer used
				read_optional_float("constant_threshold", -1.0F);

				uint32_t output_index;
				m_parser.try_read("output_index", output_index, i);

				m_parser.try_read("parent_index", transform_desc.parent_index, k_invalid_track_index);

				transform_desc.shell_distance = read_optional_float("shell_distance", transform_desc.shell_distance);

				// Deprecated, no longer used
				read_optional_float("constant_rotation_threshold_angle", -1.0F);
				read_optional_float("constant_translation_threshold", -1.0F);
				read_optional_float("constant_scale_threshold", -1.0F);

				scalar_desc.output_index = output_index;
				transform_desc.output_index = output_index;

				if (track_type == track_type8::qvvf)
					transform_desc.precision = precision < 0.0F ? transform_desc.precision : precision;
				else
					scalar_desc.precision = precision < 0.0F ? scalar_desc.precision : precision;

				rtm::qvvf bind_transform = rtm::qvv_identity();
				if (m_is_binary_exact)
				{
					sjson::StringView rotation[4];
					if (m_parser.try_read("bind_rotation", rotation, 4, nullptr) && !counting)
					{
						const rtm::float4f value = hex_to_float4f(rotation, 4);
						bind_transform.rotation = rtm::quat_load(&value);
					}

					sjson::StringView translation[3];
					if (m_parser.try_read("bind_translation", translation, 3, nullptr) && !counting)
					{
						const rtm::float4f value = hex_to_float4f(translation, 3);
						bind_transform.translation = rtm::vector_load(&value);
					}

					sjson::StringView scale[3];
					if (m_parser.try_read("bind_scale", scale, 3, nullptr) && !counting)
					{
						const rtm::float4f value = hex_to_float4f(scale, 3);
						bind_transform.scale = rtm::vector_load(&value);
					}
				}
				else
				{
					double rotation[4] = { 0.0, 0.0, 0.0, 0.0 };
					if (m_parser.try_read("bind_rotation", rotation, 4, 0.0) && !counting)
						bind_transform.rotation = rtm::quat_normalize_deterministic(rtm::quat_cast(rtm::quat_load(&rotation[0])));

					double translation[3] = { 0.0, 0.0, 0.0 };
					if (m_parser.try_read("bind_translation", translation, 3, 0.0) && !counting)
						bind_transform.translation = rtm::vector_cast(rtm::vector_load3(&translation[0]));

					double scale[3] = { 0.0, 0.0, 0.0 };
					if (m_parser.try_read("bind_scale", scale, 3, 0.0) && !counting)
						bind_transform.scale = rtm::vector_cast(rtm::vector_load3(&scale[0]));
				}

				transform_desc.default_value = bind_transform;

				if (!m_parser.array_begins("data"))
					goto error;

				union track_samples_ptr_union
				{
					void*			any;
					float*			float1f;
					rtm::float2f*	float2f;
					rtm::float3f*	float3f;
					rtm::float4f*	float4f;
					rtm::vector4f*	vector4f;
					rtm::qvvf*		qvvf;
				};
				track_samples_ptr_union track_samples_typed = { nullptr };

				switch (track_type)
				{
				case track_type8::float1f:
					track_samples_typed.float1f = allocate_type_array<float>(m_allocator, m_num_samples);
					break;
				case track_type8::float2f:
					track_samples_typed.float2f = allocate_type_array<rtm::float2f>(m_allocator, m_num_samples);
					break;
				case track_type8::float3f:
					track_samples_typed.float3f = allocate_type_array<rtm::float3f>(m_allocator, m_num_samples);
					break;
				case track_type8::float4f:
					track_samples_typed.float4f = allocate_type_array<rtm::float4f>(m_allocator, m_num_samples);
					break;
				case track_type8::vector4f:
					track_samples_typed.vector4f = allocate_type_array<rtm::vector4f>(m_allocator, m_num_samples);
					break;
				case track_type8::qvvf:
					track_samples_typed.qvvf = allocate_type_array<rtm::qvvf>(m_allocator, m_num_samples);
					break;
				default:
					ACL_ASSERT(false, "Unsupported track type");
					// Initialize our value to itself, we'll never use it
					// This silences an MSVC static analysis warning
					track_samples_typed.any = &track_samples_typed;
					break;
				}

				bool has_error = false;
				for (uint32_t sample_index = 0; sample_index < m_num_samples; ++sample_index)
				{
					if (!m_parser.array_begins())
					{
						has_error = true;
						break;
					}

					if (m_is_binary_exact)
					{
						switch (track_type)
						{
						case track_type8::float1f:
						case track_type8::float2f:
						case track_type8::float3f:
						case track_type8::float4f:
						case track_type8::vector4f:
						{
							sjson::StringView values[4];
							if (m_parser.read(values, num_components))
							{
								const rtm::float4f value = hex_to_float4f(values, num_components);
								std::memcpy(track_samples_typed.float1f + (size_t(sample_index) * num_components), &value, sizeof(float) * num_components);
							}
							else
								has_error = true;
							break;
						}
						case track_type8::qvvf:
						{
							rtm::qvvf sample;
							if (!read_qvv_sample(sample))
								has_error = true;
							else
								std::memcpy(track_samples_typed.qvvf + sample_index, &sample, sizeof(rtm::qvvf));
							break;
						}
						default:
							ACL_ASSERT(false, "Unsupported track type");
							break;
						}

						if (has_error)
							break;
					}
					else
					{
						switch (track_type)
						{
						case track_type8::float1f:
						case track_type8::float2f:
						case track_type8::float3f:
						case track_type8::float4f:
						case track_type8::vector4f:
						{
							double values[4] = { 0.0, 0.0, 0.0, 0.0 };
							if (m_parser.read(values, num_components))
							{
								const rtm::float4f value = { static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]), static_cast<float>(values[3]) };
								std::memcpy(track_samples_typed.float1f + (size_t(sample_index) * num_components), &value, sizeof(float) * num_components);
							}
							else
								has_error = true;
							break;
						}
						case track_type8::qvvf:
						{
							rtm::qvvf sample;
							if (!read_qvv_sample(sample))
								has_error = true;
							else
								std::memcpy(track_samples_typed.qvvf + sample_index, &sample, sizeof(rtm::qvvf));
							break;
						}
						default:
							ACL_ASSERT(false, "Unsupported track type");
							break;
						}

						if (has_error)
							break;
					}

					if (!has_error && !m_parser.array_ends())
					{
						has_error = true;
						break;
					}
				}

				if (!has_error && !m_parser.array_ends())
					has_error = true;

				if (!has_error && !m_parser.object_ends())
					has_error = true;

				if (has_error)
				{
					switch (track_type)
					{
					case track_type8::float1f:
						deallocate_type_array<float>(m_allocator, track_samples_typed.float1f, m_num_samples);
						break;
					case track_type8::float2f:
						deallocate_type_array<rtm::float2f>(m_allocator, track_samples_typed.float2f, m_num_samples);
						break;
					case track_type8::float3f:
						deallocate_type_array<rtm::float3f>(m_allocator, track_samples_typed.float3f, m_num_samples);
						break;
					case track_type8::float4f:
						deallocate_type_array<rtm::float4f>(m_allocator, track_samples_typed.float4f, m_num_samples);
						break;
					case track_type8::vector4f:
						deallocate_type_array<rtm::vector4f>(m_allocator, track_samples_typed.vector4f, m_num_samples);
						break;
					case track_type8::qvvf:
						deallocate_type_array<rtm::qvvf>(m_allocator, track_samples_typed.qvvf, m_num_samples);
						break;
					default:
						ACL_ASSERT(false, "Unsupported track type");
						break;
					}

					goto error;
				}
				else
				{
					switch (track_type)
					{
					case track_type8::float1f:
						track_ = track_float1f::make_owner(scalar_desc, m_allocator, track_samples_typed.float1f, m_num_samples, m_sample_rate);
						break;
					case track_type8::float2f:
						track_ = track_float2f::make_owner(scalar_desc, m_allocator, track_samples_typed.float2f, m_num_samples, m_sample_rate);
						break;
					case track_type8::float3f:
						track_ = track_float3f::make_owner(scalar_desc, m_allocator, track_samples_typed.float3f, m_num_samples, m_sample_rate);
						break;
					case track_type8::float4f:
						track_ = track_float4f::make_owner(scalar_desc, m_allocator, track_samples_typed.float4f, m_num_samples, m_sample_rate);
						break;
					case track_type8::vector4f:
						track_ = track_vector4f::make_owner(scalar_desc, m_allocator, track_samples_typed.vector4f, m_num_samples, m_sample_rate);
						break;
					case track_type8::qvvf:
						track_ = track_qvvf::make_owner(transform_desc, m_allocator, track_samples_typed.qvvf, m_num_samples, m_sample_rate);
						break;
					default:
						ACL_ASSERT(false, "Unsupported track type");
						break;
					}
				}

				track_.set_name(convert_string(name));

				num_tracks++;
			}

			return true;

		error:
			m_error = m_parser.get_error();
			return false;
		}

		bool create_track_list(track_array& track_list)
		{
			const sjson::ParserState before_tracks = m_parser.save_state();

			uint32_t num_tracks;
			if (!process_track_list(nullptr, num_tracks))
				return false;

			m_parser.restore_state(before_tracks);

			track_list = track_array(m_allocator, num_tracks);
			track_list.set_name(convert_string(m_clip_name));

			if (!process_track_list(track_list.begin(), num_tracks))
				return false;

			ACL_ASSERT(num_tracks == track_list.get_num_tracks(), "Number of tracks read mismatch");

			return true;
		}

		bool read_tracks(track_array_qvvf& track_list, track_array_qvvf& additive_base_track_list)
		{
			const uint32_t num_transforms = track_list.get_num_tracks();

			if (m_parser.try_array_begins("base_tracks"))
			{
				// Copy our metadata from the actual clip
				additive_base_track_list = track_array_qvvf(m_allocator, num_transforms);
				additive_base_track_list.set_name(string(m_allocator, "additive base"));
				for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
					additive_base_track_list[transform_index].get_description() = track_list[transform_index].get_description();

				while (!m_parser.try_array_ends())
				{
					if (!m_parser.object_begins())
						goto error;

					sjson::StringView name;
					if (!m_parser.read("name", name))
						goto error;

					const uint32_t bone_index = find_bone(name);
					if (bone_index == k_invalid_track_index)
					{
						set_error(clip_reader_error::NoBoneWithThatName);
						return false;
					}

					track_desc_transformf desc = additive_base_track_list[bone_index].get_description();	// Copy our description
					desc.output_index = bone_index;

					track_qvvf track = track_qvvf::make_reserve(desc, m_allocator, m_additive_base_num_samples, m_additive_base_sample_rate);
					track.set_name(convert_string(name));

					if (m_parser.try_array_begins("rotations"))
					{
						if (!read_track_rotations(track, m_additive_base_num_samples) || !m_parser.array_ends())
							goto error;
					}
					else
					{
						for (uint32_t sample_index = 0; sample_index < m_additive_base_num_samples; ++sample_index)
							track[sample_index].rotation = rtm::quat_identity();
					}

					if (m_parser.try_array_begins("translations"))
					{
						if (!read_track_translations(track, m_additive_base_num_samples) || !m_parser.array_ends())
							goto error;
					}
					else
					{
						for (uint32_t sample_index = 0; sample_index < m_additive_base_num_samples; ++sample_index)
							track[sample_index].translation = rtm::vector_zero();
					}

					if (m_parser.try_array_begins("scales"))
					{
						if (!read_track_scales(track, m_additive_base_num_samples) || !m_parser.array_ends())
							goto error;
					}
					else
					{
						for (uint32_t sample_index = 0; sample_index < m_additive_base_num_samples; ++sample_index)
							track[sample_index].scale = rtm::vector_set(1.0F);
					}

					additive_base_track_list[bone_index] = std::move(track);

					if (!m_parser.object_ends())
						goto error;
				}
			}

			if (!m_parser.array_begins("tracks"))
				goto error;

			while (!m_parser.try_array_ends())
			{
				if (!m_parser.object_begins())
					goto error;

				sjson::StringView name;
				if (!m_parser.read("name", name))
					goto error;

				const uint32_t bone_index = find_bone(name);
				if (bone_index == k_invalid_track_index)
				{
					set_error(clip_reader_error::NoBoneWithThatName);
					return false;
				}

				track_desc_transformf desc = track_list[bone_index].get_description();	// Copy our description
				desc.output_index = bone_index;

				track_qvvf track = track_qvvf::make_reserve(desc, m_allocator, m_num_samples, m_sample_rate);
				track.set_name(convert_string(name));

				if (m_parser.try_array_begins("rotations"))
				{
					if (!read_track_rotations(track, m_num_samples) || !m_parser.array_ends())
						goto error;
				}
				else
				{
					for (uint32_t sample_index = 0; sample_index < m_num_samples; ++sample_index)
						track[sample_index].rotation = rtm::quat_identity();
				}

				if (m_parser.try_array_begins("translations"))
				{
					if (!read_track_translations(track, m_num_samples) || !m_parser.array_ends())
						goto error;
				}
				else
				{
					for (uint32_t sample_index = 0; sample_index < m_num_samples; ++sample_index)
						track[sample_index].translation = rtm::vector_zero();
				}

				if (m_parser.try_array_begins("scales"))
				{
					if (!read_track_scales(track, m_num_samples) || !m_parser.array_ends())
						goto error;
				}
				else
				{
					for (uint32_t sample_index = 0; sample_index < m_num_samples; ++sample_index)
						track[sample_index].scale = rtm::vector_set(1.0F);
				}

				track_list[bone_index] = std::move(track);

				if (!m_parser.object_ends())
					goto error;
			}

			return true;

		error:
			m_error = m_parser.get_error();
			return false;
		}

		bool read_track_rotations(track_qvvf& track, uint32_t num_samples_expected)
		{
			for (uint32_t i = 0; i < num_samples_expected; ++i)
			{
				if (!m_parser.array_begins())
					return false;

				rtm::quatd rotation;

				if (m_is_binary_exact)
				{
					sjson::StringView values[4];
					if (!m_parser.read(values, 4))
						return false;

					rotation = hex_to_quat(values);
				}
				else
				{
					double values[4] = { 0.0, 0.0, 0.0, 0.0 };
					if (!m_parser.read(values, 4))
						return false;

					rotation = rtm::quat_load(values);
				}

				if (!m_parser.array_ends())
					return false;

				track[i].rotation = rtm::quat_normalize_deterministic(rtm::quat_cast(rotation));
			}

			return true;
		}

		bool read_track_translations(track_qvvf& track, uint32_t num_samples_expected)
		{
			for (uint32_t i = 0; i < num_samples_expected; ++i)
			{
				if (!m_parser.array_begins())
					return false;

				rtm::vector4d translation;

				if (m_is_binary_exact)
				{
					sjson::StringView values[3];
					if (!m_parser.read(values, 3))
						return false;

					translation = hex_to_vector3(values);
				}
				else
				{
					double values[3];
					if (!m_parser.read(values, 3))
						return false;

					translation = rtm::vector_load3(values);
				}

				if (!m_parser.array_ends())
					return false;

				track[i].translation = rtm::vector_cast(translation);
			}

			return true;
		}

		bool read_track_scales(track_qvvf& track, uint32_t num_samples_expected)
		{
			for (uint32_t i = 0; i < num_samples_expected; ++i)
			{
				if (!m_parser.array_begins())
					return false;

				rtm::vector4d scale;

				if (m_is_binary_exact)
				{
					sjson::StringView values[3];
					if (!m_parser.read(values, 3))
						return false;

					scale = hex_to_vector3(values);
				}
				else
				{
					double values[3];
					if (!m_parser.read(values, 3))
						return false;

					scale = rtm::vector_load3(values);
				}

				if (!m_parser.array_ends())
					return false;

				track[i].scale = rtm::vector_cast(scale);
			}

			return true;
		}

		bool nothing_follows()
		{
			if (!m_parser.remainder_is_comments_and_whitespace())
			{
				m_error = m_parser.get_error();
				return false;
			}

			return true;
		}

		void set_error(uint32_t reason)
		{
			m_parser.get_position(m_error.line, m_error.column);
			m_error.error = reason;
		}
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP

#endif	// #if defined(ACL_USE_SJSON)
