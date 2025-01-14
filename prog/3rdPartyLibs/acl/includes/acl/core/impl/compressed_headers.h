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
#include "acl/core/algorithm_types.h"
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/ptr_offset.h"
#include "acl/core/quality_tiers.h"
#include "acl/core/range_reduction_types.h"
#include "acl/core/track_formats.h"
#include "acl/core/track_types.h"
#include "acl/core/impl/atomic.impl.h"
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	class compressed_tracks;

	namespace acl_impl
	{
		// Common header to all binary formats
		struct raw_buffer_header
		{
			// Total size in bytes of the raw buffer.
			uint32_t		size;

			// Hash of the raw buffer.
			uint32_t		hash;
		};

		// Header for 'compressed_tracks'
		struct tracks_header
		{
			// Serialization tag used to distinguish raw buffer types.
			uint32_t						tag;

			// Serialization version used to compress the tracks.
			compressed_tracks_version16		version;

			// Algorithm type used to compress the tracks.
			algorithm_type8					algorithm_type;

			// Type of the tracks contained in this compressed stream.
			track_type8						track_type;

			// The total number of tracks.
			uint32_t						num_tracks;

			// The total number of samples per track.
			uint32_t						num_samples;

			// The sample rate our tracks use.
			float							sample_rate;					// TODO: Store duration as float instead?

			// Miscellaneous packed values
			uint32_t						misc_packed;

			//////////////////////////////////////////////////////////////////////////
			// Accessors for 'misc_packed'

			// Scalar tracks use it like this (listed from LSB):
			// Bits [0, 31): unused (31 bits)
			// Bit 30: is wrap optimized? See sample_looping_policy for details.
			// Bit 31: has metadata?

			// Transform tracks use it like this (listed from LSB):
			// Bit 0: has scale?
			// Bit 1: default scale: 0,0,0 or 1,1,1 (bool/bit)
			// Bit 2: scale format
			// Bit 3: translation format
			// Bits [4, 8): rotation format (4 bits)
			// Bit 8: has database?
			// Bit 9: has trivial default values? Non-trivial default values indicate that extra data beyond the clip will be needed at decompression (e.g. bind pose)
			// Bit 10: has stripped keyframes?
			// Bits [11, 30): unused (19 bits)
			// Bit 30: is wrap optimized? See sample_looping_policy for details.
			// Bit 31: has metadata?

			// Transform only
			bool get_has_scale() const { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); return (misc_packed & 1) != 0; }
			void set_has_scale(bool has_scale) { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); misc_packed = (misc_packed & ~1) | static_cast<uint32_t>(has_scale); }
			int32_t get_default_scale() const { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); return static_cast<int32_t>((misc_packed >> 1) & 1); }
			void set_default_scale(uint32_t scale) { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); ACL_ASSERT(scale == 0 || scale == 1, "Invalid default scale"); misc_packed = (misc_packed & ~(1 << 1)) | (scale << 1); }
			vector_format8 get_scale_format() const { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); return static_cast<vector_format8>((misc_packed >> 2) & 1); }
			void set_scale_format(vector_format8 format) { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); misc_packed = (misc_packed & ~(1 << 2)) | (static_cast<uint32_t>(format) << 2); }
			vector_format8 get_translation_format() const { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); return static_cast<vector_format8>((misc_packed >> 3) & 1); }
			void set_translation_format(vector_format8 format) { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); misc_packed = (misc_packed & ~(1 << 3)) | (static_cast<uint32_t>(format) << 3); }
			rotation_format8 get_rotation_format() const { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); return static_cast<rotation_format8>((misc_packed >> 4) & 15); }
			void set_rotation_format(rotation_format8 format) { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); misc_packed = (misc_packed & ~(15 << 4)) | (static_cast<uint32_t>(format) << 4); }
			bool get_has_database() const { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); return (misc_packed & (1 << 8)) != 0; }
			void set_has_database(bool has_database) { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); misc_packed = (misc_packed & ~(1 << 8)) | (static_cast<uint32_t>(has_database) << 8); }
			bool get_has_trivial_default_values() const { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); return (misc_packed & (1 << 9)) != 0; }
			void set_has_trivial_default_values(bool has_trivial_default_values) { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); misc_packed = (misc_packed & ~(1 << 9)) | (static_cast<uint32_t>(has_trivial_default_values) << 9); }
			bool get_has_stripped_keyframes() const { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); return (misc_packed & (1 << 10)) != 0; }
			void set_has_stripped_keyframes(bool has_stripped_keyframes) { ACL_ASSERT(track_type == track_type8::qvvf, "Transform tracks only"); misc_packed = (misc_packed & ~(1 << 10)) | (static_cast<uint32_t>(has_stripped_keyframes) << 10); }

			// Common
			bool get_is_wrap_optimized() const { return (misc_packed & (1 << 30)) != 0; }
			void set_is_wrap_optimized(bool is_wrap_optimized) { misc_packed = (misc_packed & ~(1 << 30)) | (static_cast<uint32_t>(is_wrap_optimized) << 30); }
			bool get_has_metadata() const { return (misc_packed >> 31) != 0; }
			void set_has_metadata(bool has_metadata) { misc_packed = (misc_packed & ~(1 << 31)) | (static_cast<uint32_t>(has_metadata) << 31); }
		};

		// Scalar track metadata
		struct track_metadata
		{
			uint8_t			bit_rate;
		};

		// Header for scalar 'compressed_tracks'
		struct scalar_tracks_header
		{
			// The number of bits used for a whole frame of data.
			// The sum of one sample per track with all bit rates taken into account.
			uint32_t						num_bits_per_frame;

			// Various data offsets relative to the start of this header.
			ptr_offset32<track_metadata>	metadata_per_track;
			ptr_offset32<float>				track_constant_values;
			ptr_offset32<float>				track_range_values;
			ptr_offset32<uint8_t>			track_animated_values;

			//////////////////////////////////////////////////////////////////////////

			track_metadata*					get_track_metadata() { return metadata_per_track.add_to(this); }
			const track_metadata*			get_track_metadata() const { return metadata_per_track.add_to(this); }

			float*							get_track_constant_values() { return track_constant_values.add_to(this); }
			const float*					get_track_constant_values() const { return track_constant_values.add_to(this); }

			float*							get_track_range_values() { return track_range_values.add_to(this); }
			const float*					get_track_range_values() const { return track_range_values.add_to(this); }

			uint8_t*						get_track_animated_values() { return track_animated_values.add_to(this); }
			const uint8_t*					get_track_animated_values() const { return track_animated_values.add_to(this); }
		};

		////////////////////////////////////////////////////////////////////////////////
		// A compressed clip segment header. Each segment is built from a uniform number
		// of samples per track. A clip is split into one or more segments.
		////////////////////////////////////////////////////////////////////////////////
		struct segment_header
		{
			// Number of bits used by a fully animated pose (excludes default/constant tracks).
			uint32_t						animated_pose_bit_size;

			// Number of bits used by a fully animated pose per sub-track type (excludes default/constant tracks).
			uint32_t						animated_rotation_bit_size;
			uint32_t						animated_translation_bit_size;

			// Offset to the animated segment data, relative to the start of the transform_tracks_header
			// Segment data is partitioned as follows:
			//    - format per variable track (no alignment)
			//    - range data per variable track (only when more than one segment) (2 byte alignment)
			//    - track data sorted per sample then per track (4 byte alignment)
			ptr_offset32<uint8_t>			segment_data;
		};

		////////////////////////////////////////////////////////////////////////////////
		// A compressed clip segment header. Each segment is built from a uniform number
		// of samples per track. A clip is split into one or more segments.
		// Only valid when a clip is split into a database or when keyframe stripping is enabled.
		////////////////////////////////////////////////////////////////////////////////
		struct stripped_segment_header_t : segment_header
		{
			// Bit set of which sample indices are stored in this clip (database tier 0).
			uint32_t						sample_indices;
		};

		struct database_runtime_clip_header;	// Forward declare

		// Header for database related metadata in 'compressed_tracks'
		struct tracks_database_header
		{
			// Offset of the runtime clip header to update when we stream in/out.
			ptr_offset32<database_runtime_clip_header>		clip_header_offset;

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			database_runtime_clip_header*					get_clip_header(void* base) { return clip_header_offset.add_to(base); }
			const database_runtime_clip_header*				get_clip_header(const void* base) const { return clip_header_offset.add_to(base); }
		};

		//////////////////////////////////////////////////////////////////////////
		// A 32 bit integer that contains packed sub-track types.
		// Each sub-track type is packed on 2 bits starting with the MSB.
		// 0 = default/padding, 1 = constant, 2 = animated
		//////////////////////////////////////////////////////////////////////////
		struct packed_sub_track_types
		{
			uint32_t types;
		};

		const uint32_t k_num_sub_tracks_per_packed_entry = 16;	// 2 bits each within a 32 bit entry

		// Header for transform 'compressed_tracks'
		struct transform_tracks_header
		{
			transform_tracks_header() = delete;	// Not needed

			// The number of segments contained.
			uint32_t						num_segments;

			// The number of animated rot/trans/scale tracks.
			uint32_t						num_animated_variable_sub_tracks;		// Might be padded with dummy tracks for alignment
			uint32_t						num_animated_rotation_sub_tracks;
			uint32_t						num_animated_translation_sub_tracks;
			uint32_t						num_animated_scale_sub_tracks;

			// The number of constant sub-track samples stored, does not include default samples
			uint32_t						num_constant_rotation_samples;
			uint32_t						num_constant_translation_samples;
			uint32_t						num_constant_scale_samples;

			// Offset to the database metadata header.
			ptr_offset32<tracks_database_header>	database_header_offset;

			// Offset to the segment headers data.
			union
			{
				ptr_offset32<segment_header>			segment_headers_offset;
				ptr_offset32<stripped_segment_header_t>	stripped_segment_headers_offset;
			};

			// Offset to the packed sub-track types.
			ptr_offset32<packed_sub_track_types>	sub_track_types_offset;

			// Offset to the constant tracks data.
			ptr_offset32<uint8_t>			constant_track_data_offset;

			// Offset to the clip range data.
			ptr_offset32<uint8_t>			clip_range_data_offset;					// TODO: Make this offset optional? Only present if using variable bit rate

			//////////////////////////////////////////////////////////////////////////

			bool							has_multiple_segments() const { return num_segments > 1; }

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			uint32_t*						get_segment_start_indices() { ACL_ASSERT(has_multiple_segments(), "Must have multiple segments to contain segment indices"); return add_offset_to_ptr<uint32_t>(this, align_to(sizeof(transform_tracks_header), 4)); }
			const uint32_t*					get_segment_start_indices() const { ACL_ASSERT(has_multiple_segments(), "Must have multiple segments to contain segment indices"); return add_offset_to_ptr<const uint32_t>(this, align_to(sizeof(transform_tracks_header), 4)); }

			tracks_database_header*			get_database_header() { return database_header_offset.safe_add_to(this); }
			const tracks_database_header*	get_database_header() const { return database_header_offset.safe_add_to(this); }

			segment_header*					get_segment_headers() { return segment_headers_offset.add_to(this); }
			const segment_header*			get_segment_headers() const { return segment_headers_offset.add_to(this); }

			stripped_segment_header_t*			get_stripped_segment_headers() { return stripped_segment_headers_offset.add_to(this); }
			const stripped_segment_header_t*	get_stripped_segment_headers() const { return stripped_segment_headers_offset.add_to(this); }

			packed_sub_track_types*			get_sub_track_types() { return sub_track_types_offset.add_to(this); }
			const packed_sub_track_types*	get_sub_track_types() const { return sub_track_types_offset.add_to(this); }

			uint8_t*						get_constant_track_data() { return constant_track_data_offset.add_to(this); }
			const uint8_t*					get_constant_track_data() const { return constant_track_data_offset.add_to(this); }

			uint8_t*						get_clip_range_data() { return clip_range_data_offset.add_to(this); }
			const uint8_t*					get_clip_range_data() const { return clip_range_data_offset.add_to(this); }

			template<class segment_header_type>
			void							get_segment_data(const segment_header_type& header, uint8_t*& out_format_per_track_data, uint8_t*& out_range_data, uint8_t*& out_animated_data)
			{
				uint8_t* segment_data = header.segment_data.add_to(this);

				uint8_t* format_per_track_data = segment_data;

				uint8_t* range_data = align_to(format_per_track_data + num_animated_variable_sub_tracks, 2);
				const uint32_t range_data_size = has_multiple_segments() ? (k_segment_range_reduction_num_bytes_per_component * 6 * num_animated_variable_sub_tracks) : 0;

				uint8_t* animated_data = align_to(range_data + range_data_size, 4);

				out_format_per_track_data = format_per_track_data;
				out_range_data = range_data;
				out_animated_data = animated_data;
			}

			template<class segment_header_type>
			void							get_segment_data(const segment_header_type& header, const uint8_t*& out_format_per_track_data, const uint8_t*& out_range_data, const uint8_t*& out_animated_data) const
			{
				const uint8_t* segment_data = header.segment_data.add_to(this);

				const uint8_t* format_per_track_data = segment_data;

				const uint8_t* range_data = align_to(format_per_track_data + num_animated_variable_sub_tracks, 2);
				const uint32_t range_data_size = has_multiple_segments() ? (k_segment_range_reduction_num_bytes_per_component * 6 * num_animated_variable_sub_tracks) : 0;

				const uint8_t* animated_data = align_to(range_data + range_data_size, 4);

				out_format_per_track_data = format_per_track_data;
				out_range_data = range_data;
				out_animated_data = animated_data;
			}
		};

		//////////////////////////////////////////////////////////////////////////
		// How much error each frame contributes to at most
		// Frames that cannot be removed have infinite error (e.g. first and last frame of the clip)
		// Note: old structure used prior to ACL 2.1
		struct frame_contributing_error
		{
			uint32_t index;		// Segment relative frame index
			float error;		// Contributing error
		};

		//////////////////////////////////////////////////////////////////////////
		// Keyframe stripping related metadata information for each keyframe
		struct keyframe_stripping_metadata_t
		{
			// Clip relative keyframe index, from 0 to num clip keyframes
			uint32_t keyframe_index = 0;

			// Segment index this keyframe belongs to, from 0 to num clip segments
			uint32_t segment_index = 0;

			// In which order to strip keyframes, from 0 to num clip keyframes, ~0 if we cannot strip this keyframe
			uint32_t stripping_index = 0;

			// How much error remains if this keyframe is removed, inf if we cannot strip this keyframe
			float stripping_error = 0.0F;

			// Whether or not the error at every track is below its precision threshold if this keyframe is removed
			uint8_t is_keyframe_trivial = false;

			keyframe_stripping_metadata_t() = default;
			keyframe_stripping_metadata_t(uint32_t keyframe_index_, uint32_t segment_index_, uint32_t stripping_index_, float stripping_error_, bool is_keyframe_trivial_)
				: keyframe_index(keyframe_index_)
				, segment_index(segment_index_)
				, stripping_index(stripping_index_)
				, stripping_error(stripping_error_)
				, is_keyframe_trivial(is_keyframe_trivial_)
			{}
		};

		// Header for optional track metadata, must be at least 15 bytes
		struct optional_metadata_header
		{
			ptr_offset32<char>						track_list_name;
			ptr_offset32<uint32_t>					track_name_offsets;
			ptr_offset32<uint32_t>					parent_track_indices;
			ptr_offset32<uint8_t>					track_descriptions;

			// In ACL 2.0, the contributing error is of type frame_contributing_error and is sorted by segment
			// Each keyframe of segment 0, followed by segment 1, etc and each segment is sorted by the stripping error
			// In ACL 2.1, the contributing error is of type keyframe_stripping_metadata_t and is sorted in
			// stripping order
			ptr_offset32<uint8_t>					contributing_error;

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			char*									get_track_list_name(compressed_tracks& tracks) { return track_list_name.safe_add_to(&tracks); }
			const char*								get_track_list_name(const compressed_tracks& tracks) const { return track_list_name.safe_add_to(&tracks); }
			uint32_t*								get_track_name_offsets(compressed_tracks& tracks) { return track_name_offsets.safe_add_to(&tracks); }
			const uint32_t*							get_track_name_offsets(const compressed_tracks& tracks) const { return track_name_offsets.safe_add_to(&tracks); }
			uint32_t*								get_parent_track_indices(compressed_tracks& tracks) { return parent_track_indices.safe_add_to(&tracks); }
			const uint32_t*							get_parent_track_indices(const compressed_tracks& tracks) const { return parent_track_indices.safe_add_to(&tracks); }
			uint8_t*								get_track_descriptions(compressed_tracks& tracks) { return track_descriptions.safe_add_to(&tracks); }
			const uint8_t*							get_track_descriptions(const compressed_tracks& tracks) const { return track_descriptions.safe_add_to(&tracks); }
			uint8_t*								get_contributing_error(compressed_tracks& tracks) { return contributing_error.safe_add_to(&tracks); }
			const uint8_t*							get_contributing_error(const compressed_tracks& tracks) const { return contributing_error.safe_add_to(&tracks); }
		};

		static_assert(sizeof(optional_metadata_header) >= 15, "Optional metadata must be at least 15 bytes");

		//////////////////////////////////////////////////////////////////////////
		// Runtime metadata has the following layout:
		// [ database_runtime_clip_header, database_runtime_segment_header+, database_runtime_clip_header, database_runtime_segment_header+, ... ]
		// Each clip header is followed by a list of segment headers, one for each segment contained in the clip.
		// The pattern repeats for every clip contained in the database.

		// Header for runtime database segments
		struct database_runtime_segment_header
		{
			// Can't copy or move due to atomic
			database_runtime_segment_header(const database_runtime_segment_header&) = delete;
			database_runtime_segment_header& operator=(const database_runtime_segment_header&) = delete;

			// Each segment can be split into at most 3 tiers with tier 0 being in the compressed clip itself.
			// As such, each segment can be split into at most 2 tiers within the database, each with it's own
			// chunk. Each segment contains at most 32 samples. Tiers are sorted in order from most important
			// to least important and as such should stream in that order.

			// For thread safety reasons when streaming in asynchronously, we use a 64 bit atomic value per tier.
			// This ensures that when we read and write to it, both the offset and the indices are updated in lock
			// step. This avoids the need for fences on 64 bit systems.
			// Each tier value contains: (sample offset << 32) | sample indices
			// Sample indices is a bit set of which sample indices are stored in our chunk
			// Sample offset to the data. Zero if the data isn't used or streamed in. Relative to start of bulk data.
			std::atomic<uint64_t>			tier_metadata[2];
		};

		// Header for runtime database clips, 8 byte alignment to match database_runtime_segment_header
		struct alignas(8) database_runtime_clip_header
		{
			// Hash of the compressed clip stored in this entry
			uint32_t						clip_hash;

			uint32_t						padding;

			// Segment headers follow in memory

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			database_runtime_segment_header*			get_segment_headers() { return add_offset_to_ptr<database_runtime_segment_header>(this, sizeof(database_runtime_clip_header)); }
			const database_runtime_segment_header*		get_segment_headers() const { return add_offset_to_ptr<const database_runtime_segment_header>(this, sizeof(database_runtime_clip_header)); }
		};

		//////////////////////////////////////////////////////////////////////////
		// Chunk data has the following layout:
		// [ database_chunk_header, database_chunk_segment_header+, sample data ... ]
		// Each chunk starts with a header for itself and one header per segment it contains.
		// Segments might belong to different clips within the same chunk.
		// The actual sample data follows afterwards.

		// Header for compressed database chunk segment header
		struct database_chunk_segment_header
		{
			// Hash of the compressed clip stored in this segment
			uint32_t										clip_hash;

			// Bit set of which sample indices are stored in this chunk.
			uint32_t										sample_indices;

			// Offset in the chunk where the segment sample data begins. Relative to start of bulk data.
			ptr_offset32<uint8_t>							samples_offset;

			// Offset of the runtime clip header to update when we stream in/out
			ptr_offset32<database_runtime_clip_header>		clip_header_offset;

			// Offset of the runtime segment header to update when we stream in/out
			ptr_offset32<database_runtime_segment_header>	segment_header_offset;

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			database_runtime_clip_header*					get_clip_header(void* base) const { return clip_header_offset.add_to(base); }
			const database_runtime_clip_header*				get_clip_header(const void* base) const { return clip_header_offset.add_to(base); }

			database_runtime_segment_header*				get_segment_header(void* base) const { return segment_header_offset.add_to(base); }
			const database_runtime_segment_header*			get_segment_header(const void* base) const { return segment_header_offset.add_to(base); }
		};

		static_assert(alignof(database_chunk_segment_header), "Alignment must be 4 bytes");

		// Header for compressed database bulk data chunks
		struct database_chunk_header
		{
			// Each chunk contains multiple segments and is at most 1 MB in size.

			// Chunk index.
			uint32_t						index;

			// Chunk size in bytes
			uint32_t						size;

			// Number of segments stored in this chunk.
			uint32_t						num_segments;

			// Chunk segment headers and sample data follows in memory

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			database_chunk_segment_header*			get_segment_headers() { return add_offset_to_ptr<database_chunk_segment_header>(this, sizeof(database_chunk_header)); }
			const database_chunk_segment_header*	get_segment_headers() const { return add_offset_to_ptr<const database_chunk_segment_header>(this, sizeof(database_chunk_header)); }
		};

		static_assert((sizeof(database_chunk_header) % 4) == 0, "Header size must be a multiple of 4 bytes");

		//////////////////////////////////////////////////////////////////////////
		// Standalone database metadata headers

		// Header for compressed database chunk descriptions
		struct database_chunk_description
		{
			// Size in bytes of this chunk.
			uint32_t								size;

			// Offset in bulk data for this chunk, relative to the start of the bulk data.
			ptr_offset32<database_chunk_header>		offset;

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			database_chunk_header*					get_chunk_header(void* base) const { return offset.add_to(base); }
			const database_chunk_header*			get_chunk_header(const void* base) const { return offset.add_to(base); }
		};

		struct database_clip_metadata
		{
			// Hash of the compressed clip stored in this entry
			uint32_t										clip_hash;

			// Offset of the runtime clip header to update when we stream in/out
			ptr_offset32<database_runtime_clip_header>		clip_header_offset;

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			database_runtime_clip_header*					get_clip_header(void* base) const { return clip_header_offset.add_to(base); }
			const database_runtime_clip_header*				get_clip_header(const void* base) const { return clip_header_offset.add_to(base); }
		};

		// Header for 'compressed_database'
		// We use arrays so we can index with (tier - 1) as our index
		// Index 0 = medium importance tier, index 1 = low importance
		struct database_header
		{
			// Serialization tag used to distinguish raw buffer types.
			uint32_t						tag;

			// Serialization version used to compress the database.
			compressed_tracks_version16		version;

			// Misc packed data.
			uint16_t						misc_packed;

			// Number of chunks stored in the bulk data for each tier.
			uint32_t						num_chunks[k_num_database_tiers];

			// Max chunk size contained within
			uint32_t						max_chunk_size;

			// Number of clips stored in the database.
			uint32_t						num_clips;

			// Number of segments stored in the database.
			uint32_t						num_segments;

			// Offset to a list of clip metadata contained in this database.
			ptr_offset32<database_clip_metadata>	clip_metadata_offset;

			// Size in bytes of the bulk data for each tier.
			uint32_t						bulk_data_size[k_num_database_tiers];

			// Offset to the bulk data for each tier (optional, omitted if not inline).
			ptr_offset32<uint8_t>			bulk_data_offset[k_num_database_tiers];

			// Hash of the bulk data for each tier.
			uint32_t						bulk_data_hash[k_num_database_tiers];

			// Chunk descriptions follow in memory

			//////////////////////////////////////////////////////////////////////////
			// Accessors for 'misc_packed'

			// Listed from LSB:
			// Bit 0: is bulk data inline?
			// Bits [1, 16): unused (15 bits)

			bool get_is_bulk_data_inline() const { return (misc_packed & (1 << 0)) != 0; }
			void set_is_bulk_data_inline(bool is_inline) { misc_packed = static_cast<uint16_t>((misc_packed & ~(static_cast<uint16_t>(1) << 0)) | (static_cast<uint16_t>(is_inline) << 0)); }

			//////////////////////////////////////////////////////////////////////////
			// Utility functions that return pointers from their respective offsets.

			// Follows the header
			database_chunk_description*				get_chunk_descriptions_medium() { return add_offset_to_ptr<database_chunk_description>(this, align_to(sizeof(database_header), 4)); }
			const database_chunk_description*		get_chunk_descriptions_medium() const { return add_offset_to_ptr<const database_chunk_description>(this, align_to(sizeof(database_header), 4)); }

			// Follows the medium descriptions
			database_chunk_description*				get_chunk_descriptions_low() { return add_offset_to_ptr<database_chunk_description>(this, align_to(align_to(sizeof(database_header), 4) + num_chunks[0] * sizeof(database_chunk_description), 4)); }
			const database_chunk_description*		get_chunk_descriptions_low() const { return add_offset_to_ptr<const database_chunk_description>(this, align_to(align_to(sizeof(database_header), 4) + num_chunks[0] * sizeof(database_chunk_description), 4)); }

			database_clip_metadata*					get_clip_metadatas() { return clip_metadata_offset.add_to(this); }
			const database_clip_metadata*			get_clip_metadatas() const { return clip_metadata_offset.add_to(this); }

			uint8_t*								get_bulk_data_medium() { return bulk_data_offset[0].safe_add_to(this); }
			const uint8_t*							get_bulk_data_medium() const { return bulk_data_offset[0].safe_add_to(this); }

			uint8_t*								get_bulk_data_low() { return bulk_data_offset[1].safe_add_to(this); }
			const uint8_t*							get_bulk_data_low() const { return bulk_data_offset[1].safe_add_to(this); }
		};
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
