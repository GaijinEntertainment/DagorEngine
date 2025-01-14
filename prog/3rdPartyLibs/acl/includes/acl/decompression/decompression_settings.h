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

#include "acl/version.h"
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/track_formats.h"
#include "acl/core/track_types.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/decompression/database/database_settings.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	/// <summary>
	/// Describes when rotations should be normalized during decompression.
	/// When lossy rotations are unpacked, the quaternions are typically very close to having
	/// a length of 1.0 but this can deviate due to square root precision.
	/// When strict normalization is required, make sure to always normalize.
	/// Rotations can also drift considerably following linear interpolation and
	/// as such it is recommended to always normalize lerp outputs.
	/// Normalization should only be disabled if the caller handles it outside ACL
	/// explicitly.
	/// </summary>
	enum class rotation_normalization_policy_t
	{
		// Never normalize rotations
		never		= 0,

		// Only normalize following linear interpolation (recommended default value)
		lerp_only	= 1,

		// Always normalize rotations
		always		= 2,
	};

	//////////////////////////////////////////////////////////////////////////
	// Deriving from this struct and overriding these constexpr functions
	// allow you to control which code is stripped for maximum performance.
	// With these, you can:
	//    - Support only a subset of the formats and statically strip the rest
	//    - Force a single format and statically strip the rest
	//    - Etc.
	//
	// By default, all formats are supported.
	//////////////////////////////////////////////////////////////////////////
	struct decompression_settings
	{
		//////////////////////////////////////////////////////////////////////////
		// Common decompression settings

		//////////////////////////////////////////////////////////////////////////
		// Whether or not to clamp the sample time when `seek(..)` is called. Defaults to true.
		// Must be static constexpr!
		static constexpr bool clamp_sample_time() { return true; }

		//////////////////////////////////////////////////////////////////////////
		// Whether or not the specified track type is supported. Defaults to true.
		// If a track type is statically known not to be supported, the compiler can strip
		// the associated code.
		// Must be static constexpr!
		static constexpr bool is_track_type_supported(track_type8 /*type*/) { return true; }

		//////////////////////////////////////////////////////////////////////////
		// Whether to explicitly disable floating point exceptions during decompression.
		// This has a cost, exceptions are usually disabled globally and do not need to be
		// explicitly disabled during decompression.
		// We assume that floating point exceptions are already disabled by the caller.
		// Must be static constexpr!
		static constexpr bool disable_fp_exeptions() { return false; }

		//////////////////////////////////////////////////////////////////////////
		// Which version we should optimize for.
		// If 'any' is specified, the decompression context will support every single version
		// with full backwards compatibility.
		// Using a specific version allows the compiler to statically strip code for all other
		// versions. This allows the creation of context objects specialized for specific
		// versions which yields optimal performance.
		// Must be static constexpr!
		static constexpr compressed_tracks_version16 version_supported() { return compressed_tracks_version16::any; }

		//////////////////////////////////////////////////////////////////////////
		// Transform decompression settings

		//////////////////////////////////////////////////////////////////////////
		// Whether the specified rotation/translation/scale format are supported or not.
		// Use this to strip code related to formats you do not need.
		// Must be static constexpr!
		static constexpr bool is_rotation_format_supported(rotation_format8 /*format*/) { return true; }
		static constexpr bool is_translation_format_supported(vector_format8 /*format*/) { return true; }
		static constexpr bool is_scale_format_supported(vector_format8 /*format*/) { return true; }

		//////////////////////////////////////////////////////////////////////////
		// Whether rotations should be normalized before being output or not. Some animation
		// runtimes will normalize in a separate step and do not need the explicit normalization.
		// Enabled by default for safety.
		// Must be static constexpr!
		//ACL_DEPRECATED("Override get_rotation_normalization_policy instead; to be removed in v3.0")
		static constexpr bool normalize_rotations() { return true; }

		//////////////////////////////////////////////////////////////////////////
		// The rotation normalization policy controls when rotations are normalized by ACL.
		// By default, we always normalize to ensure safe output no matter what.
		// Must be static constexpr!
		static constexpr rotation_normalization_policy_t get_rotation_normalization_policy() { return rotation_normalization_policy_t::always; }

		//////////////////////////////////////////////////////////////////////////
		// Whether safety checks are performed when we initialize our context.
		// When safety checks are disabled, initialization never fails even if it is invalid.
		// This is meant as a performance optimization for shipping executables with all
		// unnecessary checks removed.
		// ENABLE AT YOUR OWN RISK!
		// Disabled by default for safety.
		// Must be static constexpr!
		static constexpr bool skip_initialize_safety_checks() { return false; }

		//////////////////////////////////////////////////////////////////////////
		// Whether or not to enable support for the wrap looping policy.
		// When wrapping isn't supported, we will use the clamp policy.
		// Required when loops are optimized during compression.
		// See 'sample_looping_policy' for details.
		// Enabled by default.
		// Must be static constexpr!
		static constexpr bool is_wrapping_supported() { return true; }

		//////////////////////////////////////////////////////////////////////////
		// Whether or not to enable 'sample_rounding_policy::per_track' support.
		// If support is disabled and that value is provided to the seek(..) function,
		// the runtime will assert.
		// See 'sample_rounding_policy' for details.
		// Enabled by default.
		// Must be static constexpr!
		static constexpr bool is_per_track_rounding_supported() { return true; }

		//////////////////////////////////////////////////////////////////////////
		// The database settings to use when decompressing.
		// By default, the database isn't supported.
		using database_settings_type = null_database_settings;
	};

	//////////////////////////////////////////////////////////////////////////
	// These are debug settings, everything is enabled and nothing is stripped.
	// It will have the worst performance but allows every feature.
	//////////////////////////////////////////////////////////////////////////
	struct debug_scalar_decompression_settings : public decompression_settings
	{
		//////////////////////////////////////////////////////////////////////////
		// Only support scalar tracks
		static constexpr bool is_track_type_supported(track_type8 type) { return type != track_type8::qvvf; }
	};

	//////////////////////////////////////////////////////////////////////////
	// These are debug settings, everything is enabled and nothing is stripped.
	// It will have the worst performance but allows every feature.
	//////////////////////////////////////////////////////////////////////////
	struct debug_transform_decompression_settings : public decompression_settings
	{
		//////////////////////////////////////////////////////////////////////////
		// Only support transform tracks
		static constexpr bool is_track_type_supported(track_type8 type) { return type == track_type8::qvvf; }
	};

	//////////////////////////////////////////////////////////////////////////
	// These are the default settings. Only the generally optimal settings
	// are enabled and will offer the overall best performance.
	// Supports every version.
	//////////////////////////////////////////////////////////////////////////
	struct default_scalar_decompression_settings : public decompression_settings
	{
		//////////////////////////////////////////////////////////////////////////
		// Only support scalar tracks
		static constexpr bool is_track_type_supported(track_type8 type) { return type != track_type8::qvvf; }

		//////////////////////////////////////////////////////////////////////////
		// Disabled by default since it is an uncommon feature
		static constexpr bool is_per_track_rounding_supported() { return false; }
	};

	//////////////////////////////////////////////////////////////////////////
	// These are the default settings. Only the generally optimal settings
	// are enabled and will offer the overall best performance.
	// Supports every version.
	//////////////////////////////////////////////////////////////////////////
	struct default_transform_decompression_settings : public decompression_settings
	{
		//////////////////////////////////////////////////////////////////////////
		// Only support transform tracks
		static constexpr bool is_track_type_supported(track_type8 type) { return type == track_type8::qvvf; }

		//////////////////////////////////////////////////////////////////////////
		// By default, we only support the variable bit rates as they are generally optimal
		static constexpr bool is_rotation_format_supported(rotation_format8 format) { return format == rotation_format8::quatf_drop_w_variable; }
		static constexpr bool is_translation_format_supported(vector_format8 format) { return format == vector_format8::vector3f_variable; }
		static constexpr bool is_scale_format_supported(vector_format8 format) { return format == vector_format8::vector3f_variable; }

		//////////////////////////////////////////////////////////////////////////
		// By default, we only normalize following linear interpolation as this is the biggest
		// source of deviation. Non interpolated values will typically be normalized for all
		// intents and purposes. If safety is required, override this value.
		static constexpr rotation_normalization_policy_t get_rotation_normalization_policy() { return rotation_normalization_policy_t::lerp_only; }

		//////////////////////////////////////////////////////////////////////////
		// Disabled by default since it is an uncommon feature
		static constexpr bool is_per_track_rounding_supported() { return false; }
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
