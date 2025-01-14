#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2019 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/iallocator.h"
#include "acl/core/track_formats.h"
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/variable_bit_rates.h"
#include "acl/compression/impl/sample_streams.h"
#include "acl/compression/impl/track_stream.h"

#include <rtm/quatf.h>
#include <rtm/qvvf.h>
#include <rtm/vector4f.h>

#include <cstdint>

// 0 = disabled, 1 = enabled
#define ACL_IMPL_DEBUG_DATABASE_IMPL 0

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
		class track_bit_rate_database;

		class hierarchical_track_query
		{
		public:
			explicit hierarchical_track_query(iallocator& allocator)
				: m_allocator(allocator)
				, m_database(nullptr)
				, m_track_index(0xFFFFFFFFU)
				, m_bit_rates(nullptr)
				, m_indices(nullptr)
				, m_num_transforms(0)
			{}

			~hierarchical_track_query()
			{
				deallocate_type_array(m_allocator, m_indices, m_num_transforms);
			}

			void bind(track_bit_rate_database& database);
			void build(uint32_t track_index, const transform_bit_rates* bit_rates, const transform_streams* bone_streams);

		private:
			hierarchical_track_query(const hierarchical_track_query&) = delete;
			hierarchical_track_query& operator=(const hierarchical_track_query&) = delete;

			struct transform_indices
			{
				uint32_t	rotation_cache_index;
				uint32_t	translation_cache_index;
				uint32_t	scale_cache_index;
			};

			iallocator&						m_allocator;
			track_bit_rate_database*		m_database;
			uint32_t						m_track_index;
			const transform_bit_rates*				m_bit_rates;
			transform_indices*				m_indices;
			uint32_t						m_num_transforms;

			friend track_bit_rate_database;
		};

		class single_track_query
		{
		public:
			single_track_query()
				: m_database(nullptr)
				, m_track_index(0xFFFFFFFFU)
				, m_rotation_cache_index(0xFFFFFFFFU)
				, m_translation_cache_index(0xFFFFFFFFU)
				, m_scale_cache_index(0xFFFFFFFFU)
			{}

			inline uint32_t get_track_index() const { return m_track_index; }
			inline const transform_bit_rates& get_bit_rates() const { return m_bit_rates; }

			void bind(track_bit_rate_database& database);
			void build(uint32_t track_index, const transform_bit_rates& bit_rates);

		private:
			single_track_query(const single_track_query&) = delete;
			single_track_query& operator=(const single_track_query&) = delete;

			track_bit_rate_database*		m_database;
			uint32_t						m_track_index;
			transform_bit_rates				m_bit_rates;

			uint32_t						m_rotation_cache_index;
			uint32_t						m_translation_cache_index;
			uint32_t						m_scale_cache_index;

			friend track_bit_rate_database;
		};

		class every_track_query
		{
		public:
			explicit every_track_query(iallocator& allocator)
				: m_allocator(allocator)
				, m_database(nullptr)
				, m_bit_rates(nullptr)
				, m_indices(nullptr)
				, m_num_transforms(0)
			{}

			~every_track_query()
			{
				deallocate_type_array(m_allocator, m_indices, m_num_transforms);
			}

			void bind(track_bit_rate_database& database);
			void build(const transform_bit_rates* bit_rates);

		private:
			every_track_query(const every_track_query&) = delete;
			every_track_query& operator=(const every_track_query&) = delete;

			struct transform_indices
			{
				uint32_t	rotation_cache_index;
				uint32_t	translation_cache_index;
				uint32_t	scale_cache_index;
			};

			iallocator&						m_allocator;
			track_bit_rate_database*		m_database;
			const transform_bit_rates*		m_bit_rates;
			transform_indices*				m_indices;
			uint32_t						m_num_transforms;

			friend track_bit_rate_database;
		};

		union bit_rates_union
		{
			// We are 0xFF, 0xFF, 0xFF, 0xFF if we are uninitialized and 0xFF, 0xFF, 0xFF, 0x00 if all three tracks are not variable
			uint32_t value;
			uint8_t bit_rates[4];

			bit_rates_union() : value(0xFFFFFFFFU) {}
			explicit bit_rates_union(const transform_bit_rates& input) : bit_rates{ input.rotation, input.translation, input.scale, 0 } {}

			inline bool operator==(bit_rates_union other) const { return value == other.value; }
			inline bool operator!=(bit_rates_union other) const { return value != other.value; }
		};

		//////////////////////////////////////////////////////////////////////////
		// This class manages bit rate queries against tracks.
		// It will cache recently requested bit rates to speed up repeating queries.
		//////////////////////////////////////////////////////////////////////////
		class track_bit_rate_database
		{
		public:
			track_bit_rate_database(iallocator& allocator, rotation_format8 rotation_format, vector_format8 translation_format, vector_format8 scale_format, const transform_streams* bone_streams, const transform_streams* raw_bone_steams, uint32_t num_transforms, uint32_t num_samples_per_track);
			~track_bit_rate_database();

			void set_segment(const transform_streams* bone_streams, uint32_t num_transforms, uint32_t num_samples_per_track);

			void sample(const single_track_query& query, float sample_time, rtm::qvvf* out_transforms, uint32_t num_transforms);
			void sample(const hierarchical_track_query& query, float sample_time, rtm::qvvf* out_transforms, uint32_t num_transforms);
			void sample(const every_track_query& query, float sample_time, rtm::qvvf* out_transforms, uint32_t num_transforms);

			size_t get_allocated_size() const;

		private:
			track_bit_rate_database(const track_bit_rate_database&) = delete;
			track_bit_rate_database& operator=(const track_bit_rate_database&) = delete;

			void find_cache_entries(uint32_t track_index, const transform_bit_rates& bit_rates, uint32_t& out_rotation_cache_index, uint32_t& out_translation_cache_index, uint32_t& out_scale_cache_index);

			RTM_FORCE_INLINE rtm::quatf RTM_SIMD_CALL sample_rotation(const sample_context& context, uint32_t rotation_cache_index);
			RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL sample_translation(const sample_context& context, uint32_t translation_cache_index);
			RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL sample_scale(const sample_context& context, uint32_t scale_cache_index);

			struct transform_cache_entry
			{
				// Each transform has a rotation/translation/scale.
				// We cache up to 4 different bit rates for each.
				// We also keep a generation id to determine the least recently used bit rates to evict from the cache.

				bit_rates_union		rotation_bit_rates;
				uint32_t			rotation_generation_ids[4] = { 0, 0, 0, 0 };

				bit_rates_union		translation_bit_rates;
				uint32_t			translation_generation_ids[4] = { 0, 0, 0, 0 };

				bit_rates_union		scale_bit_rates;
				uint32_t			scale_generation_ids[4] = { 0, 0, 0, 0 };

				static int32_t find_bit_rate_index(const bit_rates_union& bit_rates, uint32_t search_bit_rate);
			};

			iallocator&					m_allocator;
			const transform_streams*	m_mutable_bone_streams;
			const transform_streams*	m_raw_bone_streams;

			uint32_t			m_num_transforms;
			uint32_t			m_num_samples_per_track;
			uint32_t			m_num_entries_per_transform;
			uint32_t			m_track_size;

			bitset_description	m_bitset_desc;
			bitset_index_ref	m_bitref_constant;
			rotation_format8	m_rotation_format;
			vector_format8		m_translation_format;
			vector_format8		m_scale_format;
			bool				m_is_rotation_variable;
			bool				m_is_translation_variable;
			bool				m_is_scale_variable;
			bool				m_has_scale;

			uint32_t			m_generation_id;

			transform_cache_entry*	m_transforms;

			uint32_t*			m_track_entry_bitsets;
			size_t				m_track_bitsets_size;

			uint8_t*			m_data;
			size_t				m_data_size;
			size_t				m_num_cached_tracks;

			static constexpr uint32_t k_num_bit_rates_cached_per_track = 4;

			friend single_track_query;
			friend hierarchical_track_query;
			friend every_track_query;
		};

		//////////////////////////////////////////////////////////////////////////
		// Implementation

		inline void hierarchical_track_query::bind(track_bit_rate_database& database)
		{
			ACL_ASSERT(m_database == nullptr, "Query already bound");
			m_database = &database;

			m_indices = allocate_type_array<transform_indices>(database.m_allocator, database.m_num_transforms);
			m_num_transforms = database.m_num_transforms;
		}

		inline void hierarchical_track_query::build(uint32_t track_index, const transform_bit_rates* bit_rates, const transform_streams* bone_streams)
		{
			ACL_ASSERT(m_database != nullptr, "Query not bound to a database");
			ACL_ASSERT(track_index < m_num_transforms, "Invalid track index");

			m_track_index = track_index;
			m_bit_rates = bit_rates;

			uint32_t current_track_index = track_index;
			while (current_track_index != k_invalid_track_index)
			{
				const transform_bit_rates& current_bit_rates = bit_rates[current_track_index];
				transform_indices& indices = m_indices[current_track_index];

				m_database->find_cache_entries(current_track_index, current_bit_rates, indices.rotation_cache_index, indices.translation_cache_index, indices.scale_cache_index);

				const transform_streams& bone_stream = bone_streams[current_track_index];
				current_track_index = bone_stream.parent_bone_index;
			}
		}

		inline void single_track_query::bind(track_bit_rate_database& database)
		{
			ACL_ASSERT(m_database == nullptr, "Query already bound");
			m_database = &database;
		}

		inline void single_track_query::build(uint32_t track_index, const transform_bit_rates& bit_rates)
		{
			ACL_ASSERT(m_database != nullptr, "Query not bound to a database");

			m_track_index = track_index;
			m_bit_rates = bit_rates;

			m_database->find_cache_entries(track_index, bit_rates, m_rotation_cache_index, m_translation_cache_index, m_scale_cache_index);
		}

		inline void every_track_query::bind(track_bit_rate_database& database)
		{
			ACL_ASSERT(m_database == nullptr, "Query already bound");
			m_database = &database;

			m_indices = allocate_type_array<transform_indices>(database.m_allocator, database.m_num_transforms);
			m_num_transforms = database.m_num_transforms;
		}

		inline void every_track_query::build(const transform_bit_rates* bit_rates)
		{
			ACL_ASSERT(m_database != nullptr, "Query not bound to a database");

			m_bit_rates = bit_rates;

			for (uint32_t transform_index = 0; transform_index < m_num_transforms; ++transform_index)
			{
				const transform_bit_rates& current_bit_rates = bit_rates[transform_index];
				transform_indices& indices = m_indices[transform_index];

				m_database->find_cache_entries(transform_index, current_bit_rates, indices.rotation_cache_index, indices.translation_cache_index, indices.scale_cache_index);
			}
		}

		inline int32_t track_bit_rate_database::transform_cache_entry::find_bit_rate_index(const bit_rates_union& bit_rates, uint32_t search_bit_rate)
		{
			for (int32_t i = 0; i < 4; ++i)
			{
				if (bit_rates.bit_rates[i] == search_bit_rate)
					return i;
			}

			return -1;
		}

		inline track_bit_rate_database::track_bit_rate_database(iallocator& allocator, rotation_format8 rotation_format, vector_format8 translation_format, vector_format8 scale_format, const transform_streams* bone_streams, const transform_streams* raw_bone_steams, uint32_t num_transforms, uint32_t num_samples_per_track)
			: m_allocator(allocator)
			, m_mutable_bone_streams(bone_streams)
			, m_raw_bone_streams(raw_bone_steams)
			, m_num_transforms(num_transforms)
			, m_num_samples_per_track(num_samples_per_track)
		{
			m_transforms = allocate_type_array<transform_cache_entry>(allocator, num_transforms);

			const bool has_scale = raw_bone_steams->segment->clip->has_scale;
			m_has_scale = has_scale;

			const uint32_t num_tracks_per_transform = has_scale ? 3U : 2U;
			const uint32_t num_entries_per_transform = num_tracks_per_transform * k_num_bit_rates_cached_per_track;
			m_num_entries_per_transform = num_entries_per_transform;

			const uint32_t num_cached_tracks = num_transforms * m_num_entries_per_transform;

			m_bitset_desc = bitset_description::make_from_num_bits(num_samples_per_track);
			m_bitref_constant = num_samples_per_track != 0 ? bitset_index_ref(m_bitset_desc, 0) : bitset_index_ref();

			m_rotation_format = rotation_format;
			m_translation_format = translation_format;
			m_scale_format = scale_format;
			m_is_rotation_variable = is_rotation_format_variable(rotation_format);
			m_is_translation_variable = is_vector_format_variable(translation_format);
			m_is_scale_variable = is_vector_format_variable(scale_format);

			m_generation_id = 1;

			const uint32_t track_bitsets_size = m_bitset_desc.get_size() * num_cached_tracks;
			m_track_entry_bitsets = allocate_type_array<uint32_t>(allocator, track_bitsets_size);
			m_track_bitsets_size = track_bitsets_size;

			// We allocate a single float buffer to accommodate 4 bit rates for every rot/trans/scale track of each transform.
			// Each track is padded and aligned to ensure that it starts on a cache line boundary.
			const uint32_t track_size = align_to<uint32_t>(sizeof(rtm::vector4f) * num_samples_per_track, 64);
			m_track_size = track_size;

			const uint32_t data_size = track_size * num_cached_tracks;
			m_data = bit_cast<uint8_t*>(allocator.allocate(data_size, 64));
			m_data_size = data_size;
			m_num_cached_tracks = num_cached_tracks;
		}

		inline track_bit_rate_database::~track_bit_rate_database()
		{
			deallocate_type_array(m_allocator, m_transforms, m_num_transforms);
			deallocate_type_array(m_allocator, m_track_entry_bitsets, m_track_bitsets_size);
			m_allocator.deallocate(m_data, m_data_size);
		}

		inline void track_bit_rate_database::set_segment(const transform_streams* bone_streams, uint32_t num_transforms, uint32_t num_samples_per_track)
		{
			ACL_ASSERT(bone_streams != nullptr, "Bone streams cannot be null");
			ACL_ASSERT(num_transforms == m_num_transforms, "The number of transforms isn't consistent, we will corrupt the heap");
			ACL_ASSERT(num_samples_per_track <= m_num_samples_per_track, "Not enough memory has been reserved, we will corrupt the heap");
			(void)num_transforms;
			(void)num_samples_per_track;
			(void)m_num_samples_per_track;

			m_mutable_bone_streams = bone_streams;

			// Reset our cache
			for (uint32_t transform_index = 0; transform_index < m_num_transforms; ++transform_index)
				m_transforms[transform_index] = transform_cache_entry();

#if ACL_IMPL_DEBUG_DATABASE_IMPL
			printf("Switching segment, resetting the database...\n");
#endif
		}

		inline void track_bit_rate_database::find_cache_entries(uint32_t track_index, const transform_bit_rates& bit_rates, uint32_t& out_rotation_cache_index, uint32_t& out_translation_cache_index, uint32_t& out_scale_cache_index)
		{
			// Memory layout:
			//    track 0
			//        rotation
			//            entry 0		0
			//            entry 1		1
			//            entry 2		2
			//            entry 3		3
			//        translation
			//            entry 0		4
			//            entry 1		5
			//            entry 2		6
			//            entry 3		7
			//        scale
			//            entry 0		8
			//            entry 1		9
			//            entry 2		10
			//            entry 3		11
			//    track 1
			// ...

			const uint32_t num_entries_per_transform = m_num_entries_per_transform;
			const uint32_t base_track_offset = track_index * num_entries_per_transform;
			const uint32_t base_rotation_offset = base_track_offset + (0 * k_num_bit_rates_cached_per_track);
			const uint32_t base_translation_offset = base_track_offset + (1 * k_num_bit_rates_cached_per_track);

			const size_t bitset_size = m_bitset_desc.get_size();

			transform_cache_entry& entry = m_transforms[track_index];

			uint32_t rotation_cache_index;
			if (bit_rates.rotation == k_invalid_bit_rate)
			{
				// Constant/default tracks or tracks that do not use a variable bit rate can use a single slot
				rotation_cache_index = base_rotation_offset;
				ACL_ASSERT(rotation_cache_index < m_num_cached_tracks, "Invalid cache index");

				if (entry.rotation_generation_ids[0] == 0)
				{
					// The first time around, we invalidate all our cached samples and they will remain valid until we change segment
					uint32_t* validity_bitset = m_track_entry_bitsets + (bitset_size * rotation_cache_index);
					bitset_reset(validity_bitset, m_bitset_desc, false);

					entry.rotation_generation_ids[0] = m_generation_id++;

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Reserved cache index %u for rotation track %u...\n", rotation_cache_index, track_index);
#endif
				}
			}
			else
			{
				const int32_t slot_index = entry.find_bit_rate_index(entry.rotation_bit_rates, bit_rates.rotation);
				if (slot_index >= 0)
					rotation_cache_index = base_rotation_offset + slot_index;
				else
				{
					// Failed to find a cached entry for this track and bit rate, clear the oldest entry
					uint32_t oldest_generation_id = entry.rotation_generation_ids[0];
					uint32_t oldest_index = 0;
					for (uint32_t i = 1; i < 4; ++i)
					{
						if (entry.rotation_generation_ids[i] < oldest_generation_id)
						{
							oldest_generation_id = entry.rotation_generation_ids[i];
							oldest_index = i;
						}
					}

					rotation_cache_index = base_rotation_offset + oldest_index;
					ACL_ASSERT(rotation_cache_index < m_num_cached_tracks, "Invalid cache index");

					entry.rotation_bit_rates.bit_rates[oldest_index] = bit_rates.rotation;
					entry.rotation_generation_ids[oldest_index] = m_generation_id++;

					uint32_t* validity_bitset = m_track_entry_bitsets + (bitset_size * rotation_cache_index);
					bitset_reset(validity_bitset, m_bitset_desc, false);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Reserved cache index %u for rotation track %u...\n", rotation_cache_index, track_index);
#endif
				}
			}

			out_rotation_cache_index = rotation_cache_index;

			uint32_t translation_cache_index;
			if (bit_rates.translation == k_invalid_bit_rate)
			{
				// Constant/default tracks or tracks that do not use a variable bit rate can use a single slot
				translation_cache_index = base_translation_offset;
				ACL_ASSERT(translation_cache_index < m_num_cached_tracks, "Invalid cache index");

				if (entry.translation_generation_ids[0] == 0)
				{
					// The first time around, we invalidate all our cached samples and they will remain valid until we change segment
					uint32_t* validity_bitset = m_track_entry_bitsets + (bitset_size * translation_cache_index);
					bitset_reset(validity_bitset, m_bitset_desc, false);

					entry.translation_generation_ids[0] = m_generation_id++;

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Reserved cache index %u for translation track %u...\n", translation_cache_index, track_index);
#endif
				}
			}
			else
			{
				const int32_t slot_index = entry.find_bit_rate_index(entry.translation_bit_rates, bit_rates.translation);
				if (slot_index >= 0)
					translation_cache_index = base_translation_offset + slot_index;
				else
				{
					// Failed to find a cached entry for this track and bit rate, clear the oldest entry
					uint32_t oldest_generation_id = entry.translation_generation_ids[0];
					uint32_t oldest_index = 0;
					for (uint32_t i = 1; i < 4; ++i)
					{
						if (entry.translation_generation_ids[i] < oldest_generation_id)
						{
							oldest_generation_id = entry.translation_generation_ids[i];
							oldest_index = i;
						}
					}

					translation_cache_index = base_translation_offset + oldest_index;
					ACL_ASSERT(translation_cache_index < m_num_cached_tracks, "Invalid cache index");

					entry.translation_bit_rates.bit_rates[oldest_index] = bit_rates.translation;
					entry.translation_generation_ids[oldest_index] = m_generation_id++;

					uint32_t* validity_bitset = m_track_entry_bitsets + (bitset_size * translation_cache_index);
					bitset_reset(validity_bitset, m_bitset_desc, false);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Reserved cache index %u for translation track %u...\n", translation_cache_index, track_index);
#endif
				}
			}

			out_translation_cache_index = translation_cache_index;

			uint32_t scale_cache_index;
			if (!m_has_scale)
				scale_cache_index = 0xFFFFFFFFU;
			else
			{
				const uint32_t base_scale_offset = base_track_offset + (2 * k_num_bit_rates_cached_per_track);

				if (bit_rates.scale == k_invalid_bit_rate)
				{
					// Constant/default tracks or tracks that do not use a variable bit rate can use a single slot
					scale_cache_index = base_scale_offset;
					ACL_ASSERT(scale_cache_index < m_num_cached_tracks, "Invalid cache index");

					if (entry.scale_generation_ids[0] == 0)
					{
						// The first time around, we invalidate all our cached samples and they will remain valid until we change segment
						uint32_t* validity_bitset = m_track_entry_bitsets + (bitset_size * scale_cache_index);
						bitset_reset(validity_bitset, m_bitset_desc, false);

						entry.scale_generation_ids[0] = m_generation_id++;

#if ACL_IMPL_DEBUG_DATABASE_IMPL
						printf("Reserved cache index %u for scale track %u...\n", scale_cache_index, track_index);
#endif
					}
				}
				else
				{
					const int32_t slot_index = entry.find_bit_rate_index(entry.scale_bit_rates, bit_rates.scale);
					if (slot_index >= 0)
						scale_cache_index = base_scale_offset + slot_index;
					else
					{
						// Failed to find a cached entry for this track and bit rate, clear the oldest entry
						uint32_t oldest_generation_id = entry.scale_generation_ids[0];
						uint32_t oldest_index = 0;
						for (uint32_t i = 1; i < 4; ++i)
						{
							if (entry.scale_generation_ids[i] < oldest_generation_id)
							{
								oldest_generation_id = entry.scale_generation_ids[i];
								oldest_index = i;
							}
						}

						scale_cache_index = base_scale_offset + oldest_index;
						ACL_ASSERT(scale_cache_index < m_num_cached_tracks, "Invalid cache index");

						entry.scale_bit_rates.bit_rates[oldest_index] = bit_rates.scale;
						entry.scale_generation_ids[oldest_index] = m_generation_id++;

						uint32_t* validity_bitset = m_track_entry_bitsets + (bitset_size * scale_cache_index);
						bitset_reset(validity_bitset, m_bitset_desc, false);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
						printf("Reserved cache index %u for scale track %u...\n", scale_cache_index, track_index);
#endif
					}
				}
			}

			out_scale_cache_index = scale_cache_index;

			ACL_ASSERT(m_generation_id < (0xFFFFFFFFU - 8), "Generation ID is about to wrap, bad things will happen");
		}

		RTM_FORCE_INLINE rtm::quatf RTM_SIMD_CALL track_bit_rate_database::sample_rotation(const sample_context& context, uint32_t rotation_cache_index)
		{
			const uint32_t track_index = context.track_index;
			const transform_streams& bone_stream = m_mutable_bone_streams[track_index];
			const size_t rotation_cache_index_ = rotation_cache_index;

			rtm::quatf rotation;
			if (bone_stream.is_rotation_default)
				rotation = bone_stream.default_value.rotation;
			else if (bone_stream.is_rotation_constant)
			{
				uint32_t* validity_bitset = m_track_entry_bitsets + (m_bitset_desc.get_size() * rotation_cache_index_);
				rtm::quatf* cached_samples = safe_ptr_cast<rtm::quatf>(m_data + (m_track_size * rotation_cache_index_));

				if (bitset_test(validity_bitset, m_bitref_constant))
				{
					// Cached
					rotation = cached_samples[0];

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Hit cache for constant sample for rotation track %u...\n", track_index);
#endif
				}
				else
				{
					// Not cached
					if (m_is_rotation_variable)
						rotation = get_rotation_sample(m_raw_bone_streams[track_index], 0);
					else
						rotation = get_rotation_sample(m_raw_bone_streams[track_index], 0, m_rotation_format);

					rotation = rtm::quat_normalize(rotation);

					cached_samples[0] = rotation;
					bitset_set(validity_bitset, m_bitref_constant, true);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Cached constant sample for rotation track %u (%.5f, %.5f, %.5f, %.5f)...\n", track_index, quat_get_x(rotation), quat_get_y(rotation), quat_get_z(rotation), quat_get_w(rotation));
#endif
				}
			}
			else
			{
				const transform_streams& raw_bone_stream = m_raw_bone_streams[track_index];

				uint32_t* validity_bitset = m_track_entry_bitsets + (m_bitset_desc.get_size() * rotation_cache_index_);
				rtm::quatf* cached_samples = safe_ptr_cast<rtm::quatf>(m_data + (m_track_size * rotation_cache_index_));

				const bitset_index_ref bitref0(m_bitset_desc, context.sample_key);
				if (bitset_test(validity_bitset, bitref0))
				{
					// Cached
					rotation = cached_samples[context.sample_key];

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Hit cache for sample %u for rotation track %u at bit rate %u...\n", key0, track_index, context.bit_rates.rotation);
#endif
				}
				else
				{
					// Not cached
					if (m_is_rotation_variable)
						rotation = get_rotation_sample(bone_stream, raw_bone_stream, context.sample_key, context.bit_rates.rotation);
					else
						rotation = get_rotation_sample(bone_stream, context.sample_key, m_rotation_format);

					rotation = rtm::quat_normalize(rotation);

					cached_samples[context.sample_key] = rotation;
					bitset_set(validity_bitset, bitref0, true);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Cached sample %u for rotation track %u at bit rate %u (%.5f, %.5f, %.5f, %.5f)...\n", context.sample_key, track_index, context.bit_rates.rotation, quat_get_x(rotation), quat_get_y(rotation), quat_get_z(rotation), quat_get_w(rotation));
#endif
				}
			}

			return rotation;
		}

		RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL track_bit_rate_database::sample_translation(const sample_context& context, uint32_t translation_cache_index)
		{
			const uint32_t track_index = context.track_index;
			const transform_streams& bone_stream = m_mutable_bone_streams[track_index];
			const size_t translation_cache_index_ = translation_cache_index;

			rtm::vector4f translation;
			if (bone_stream.is_translation_default)
				translation = bone_stream.default_value.translation;
			else if (bone_stream.is_translation_constant)
			{
				uint32_t* validity_bitset = m_track_entry_bitsets + (m_bitset_desc.get_size() * translation_cache_index_);
				rtm::vector4f* cached_samples = safe_ptr_cast<rtm::vector4f>(m_data + (m_track_size * translation_cache_index_));

				if (bitset_test(validity_bitset, m_bitref_constant))
				{
					// Cached
					translation = cached_samples[0];

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Hit cache for constant sample for translation track %u...\n", track_index);
#endif
				}
				else
				{
					// Not cached
					translation = get_translation_sample(m_raw_bone_streams[track_index], 0, vector_format8::vector3f_full);

					cached_samples[0] = translation;
					bitset_set(validity_bitset, m_bitref_constant, true);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Cached constant sample for translation track %u (%.5f, %.5f, %.5f)...\n", track_index, vector_get_x(translation), vector_get_y(translation), vector_get_z(translation));
#endif
				}
			}
			else
			{
				const transform_streams& raw_bone_stream = m_raw_bone_streams[track_index];

				uint32_t* validity_bitset = m_track_entry_bitsets + (m_bitset_desc.get_size() * translation_cache_index_);
				rtm::vector4f* cached_samples = safe_ptr_cast<rtm::vector4f>(m_data + (m_track_size * translation_cache_index_));

				const bitset_index_ref bitref0(m_bitset_desc, context.sample_key);
				if (bitset_test(validity_bitset, bitref0))
				{
					// Cached
					translation = cached_samples[context.sample_key];

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Hit cache for sample %u for translation track %u at bit rate %u...\n", key0, track_index, context.bit_rates.translation);
#endif
				}
				else
				{
					// Not cached
					if (m_is_translation_variable)
						translation = get_translation_sample(bone_stream, raw_bone_stream, context.sample_key, context.bit_rates.translation);
					else
						translation = get_translation_sample(bone_stream, context.sample_key, m_translation_format);

					cached_samples[context.sample_key] = translation;
					bitset_set(validity_bitset, bitref0, true);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Cached sample %u for translation track %u at bit rate %u (%.5f, %.5f, %.5f)...\n", context.sample_key, track_index, context.bit_rates.translation, vector_get_x(translation), vector_get_y(translation), vector_get_z(translation));
#endif
				}
			}

			return translation;
		}

		RTM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL track_bit_rate_database::sample_scale(const sample_context& context, uint32_t scale_cache_index)
		{
			const uint32_t track_index = context.track_index;
			const transform_streams& bone_stream = m_mutable_bone_streams[track_index];
			const size_t scale_cache_index_ = scale_cache_index;

			rtm::vector4f scale;
			if (bone_stream.is_scale_default)
				scale = bone_stream.default_value.scale;
			else if (bone_stream.is_scale_constant)
			{
				uint32_t* validity_bitset = m_track_entry_bitsets + (m_bitset_desc.get_size() * scale_cache_index_);
				rtm::vector4f* cached_samples = safe_ptr_cast<rtm::vector4f>(m_data + (m_track_size * scale_cache_index_));

				if (bitset_test(validity_bitset, m_bitref_constant))
				{
					// Cached
					scale = cached_samples[0];

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Hit cache for constant sample for scale track %u...\n", track_index);
#endif
				}
				else
				{
					// Not cached
					scale = get_scale_sample(m_raw_bone_streams[track_index], 0, vector_format8::vector3f_full);

					cached_samples[0] = scale;
					bitset_set(validity_bitset, m_bitref_constant, true);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Cached constant sample for scale track %u (%.5f, %.5f, %.5f)...\n", track_index, vector_get_x(scale), vector_get_y(scale), vector_get_z(scale));
#endif
				}
			}
			else
			{
				const transform_streams& raw_bone_stream = m_raw_bone_streams[track_index];

				uint32_t* validity_bitset = m_track_entry_bitsets + (m_bitset_desc.get_size() * scale_cache_index_);
				rtm::vector4f* cached_samples = safe_ptr_cast<rtm::vector4f>(m_data + (m_track_size * scale_cache_index_));

				const bitset_index_ref bitref0(m_bitset_desc, context.sample_key);
				if (bitset_test(validity_bitset, bitref0))
				{
					// Cached
					scale = cached_samples[context.sample_key];

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Hit cache for sample %u for translation track %u at bit rate %u...\n", key0, track_index, context.bit_rates.scale);
#endif
				}
				else
				{
					// Not cached
					if (m_is_scale_variable)
						scale = get_scale_sample(bone_stream, raw_bone_stream, context.sample_key, context.bit_rates.scale);
					else
						scale = get_scale_sample(bone_stream, context.sample_key, m_scale_format);

					cached_samples[context.sample_key] = scale;
					bitset_set(validity_bitset, bitref0, true);

#if ACL_IMPL_DEBUG_DATABASE_IMPL
					printf("Cached sample %u for translation track %u at bit rate %u (%.5f, %.5f, %.5f)...\n", context.sample_key, track_index, context.bit_rates.scale, vector_get_x(scale), vector_get_y(scale), vector_get_z(scale));
#endif
				}
			}

			return scale;
		}

		inline void track_bit_rate_database::sample(const single_track_query& query, float sample_time, rtm::qvvf* out_local_pose, uint32_t num_transforms)
		{
			ACL_ASSERT(query.m_database == this, "Query has not been built for this database");
			ACL_ASSERT(out_local_pose != nullptr, "Cannot write to null output local pose");
			ACL_ASSERT(num_transforms > 0, "Cannot write to empty output local pose");
			(void)num_transforms;

			const segment_context* segment_context = m_mutable_bone_streams->segment;

			const uint32_t sample_key = get_uniform_sample_key(*segment_context, sample_time);

			sample_context context;
			context.track_index = query.m_track_index;
			context.sample_key = sample_key;
			context.sample_time = sample_time;
			context.bit_rates = query.m_bit_rates;

			const rtm::quatf rotation = sample_rotation(context, query.m_rotation_cache_index);
			const rtm::vector4f translation = sample_translation(context, query.m_translation_cache_index);
			const rtm::vector4f scale = sample_scale(context, query.m_scale_cache_index);

			out_local_pose[query.m_track_index] = rtm::qvv_set(rotation, translation, scale);
		}

		inline void track_bit_rate_database::sample(const hierarchical_track_query& query, float sample_time, rtm::qvvf* out_local_pose, uint32_t num_transforms)
		{
			ACL_ASSERT(out_local_pose != nullptr, "Cannot write to null output local pose");
			ACL_ASSERT(num_transforms > 0, "Cannot write to empty output local pose");
			(void)num_transforms;

			const segment_context* segment_context = m_mutable_bone_streams->segment;

			const uint32_t sample_key = get_uniform_sample_key(*segment_context, sample_time);

			sample_context context;
			context.sample_key = sample_key;
			context.sample_time = sample_time;

			uint32_t current_track_index = query.m_track_index;
			while (current_track_index != k_invalid_track_index)
			{
				const transform_streams& bone_stream = m_mutable_bone_streams[current_track_index];
				const hierarchical_track_query::transform_indices& indices = query.m_indices[current_track_index];

				context.track_index = current_track_index;
				context.bit_rates = query.m_bit_rates[current_track_index];

				const rtm::quatf rotation = sample_rotation(context, indices.rotation_cache_index);
				const rtm::vector4f translation = sample_translation(context, indices.translation_cache_index);
				const rtm::vector4f scale = sample_scale(context, indices.scale_cache_index);

				out_local_pose[current_track_index] = rtm::qvv_set(rotation, translation, scale);
				current_track_index = bone_stream.parent_bone_index;
			}
		}

		inline void track_bit_rate_database::sample(const every_track_query& query, float sample_time, rtm::qvvf* out_local_pose, uint32_t num_transforms)
		{
			ACL_ASSERT(out_local_pose != nullptr, "Cannot write to null output local pose");
			ACL_ASSERT(num_transforms > 0, "Cannot write to empty output local pose");
			ACL_ASSERT(num_transforms == m_num_transforms, "Expected the same number of pose transforms");
			(void)num_transforms;

			const segment_context* segment_context = m_mutable_bone_streams->segment;

			const uint32_t sample_key = get_uniform_sample_key(*segment_context, sample_time);

			sample_context context;
			context.sample_key = sample_key;
			context.sample_time = sample_time;

			for (uint32_t transform_index = 0; transform_index < num_transforms; ++transform_index)
			{
				const every_track_query::transform_indices& indices = query.m_indices[transform_index];

				context.track_index = transform_index;
				context.bit_rates = query.m_bit_rates[transform_index];

				const rtm::quatf rotation = sample_rotation(context, indices.rotation_cache_index);
				const rtm::vector4f translation = sample_translation(context, indices.translation_cache_index);
				const rtm::vector4f scale = sample_scale(context, indices.scale_cache_index);

				out_local_pose[transform_index] = rtm::qvv_set(rotation, translation, scale);
			}
		}

		inline size_t track_bit_rate_database::get_allocated_size() const
		{
			size_t cache_size = 0;
			cache_size += sizeof(transform_cache_entry) * m_num_transforms;
			cache_size += m_track_bitsets_size;
			cache_size += m_data_size;
			return cache_size;
		}
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
