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

#include "acl/version.h"
#include "acl/core/additive_utils.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/hash.h"
#include "acl/core/track_types.h"

#include <rtm/matrix3x4f.h>
#include <rtm/qvvf.h>
#include <rtm/scalarf.h>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Interface for all skeletal error metrics.
	// An error metric is responsible for a few things:
	//    - converting from rtm::qvvf into whatever transform type the metric uses (optional)
	//    - applying local space transforms on top of base transforms (optional)
	//    - transforming local space transforms into object space
	//    - evaluating the error function
	//
	// Most functions require two implementations: with and without scale support.
	// This is entirely for performance reasons as most clips do not have any scale.
	//////////////////////////////////////////////////////////////////////////
	class itransform_error_metric
	{
	public:
		virtual ~itransform_error_metric() = default;

		//////////////////////////////////////////////////////////////////////////
		// Returns the unique name of the error metric.
		virtual const char* get_name() const = 0;

		//////////////////////////////////////////////////////////////////////////
		// Returns a unique hash to represent the error metric.
		virtual uint32_t get_hash() const { return hash32(get_name()); }

		//////////////////////////////////////////////////////////////////////////
		// Returns the transform size used by the error metric.
		virtual size_t get_transform_size(bool has_scale) const = 0;

		//////////////////////////////////////////////////////////////////////////
		// Returns whether or not the error metric uses a transform that isn't rtm::qvvf.
		// If this is the case, we need to convert from rtm::qvvf into the transform type
		// used by the error metric.
		virtual bool needs_conversion(bool has_scale) const { (void)has_scale; return false; }

		//////////////////////////////////////////////////////////////////////////
		// Input arguments for the 'convert_transforms*' functions.
		//////////////////////////////////////////////////////////////////////////
		struct convert_transforms_args
		{
			//////////////////////////////////////////////////////////////////////////
			// A list of transform indices that are dirty and need conversion.
			const uint32_t* dirty_transform_indices;

			//////////////////////////////////////////////////////////////////////////
			// The number of dirty transforms that need conversion.
			uint32_t num_dirty_transforms;

			//////////////////////////////////////////////////////////////////////////
			// The input transforms in rtm::qvvf format to be converted.
			const rtm::qvvf* transforms;

			//////////////////////////////////////////////////////////////////////////
			// The number of transforms in the input and output buffers.
			uint32_t num_transforms;

			//////////////////////////////////////////////////////////////////////////
			// The sample index these transforms come from.
			uint32_t sample_index;

			//////////////////////////////////////////////////////////////////////////
			// True if these transforms are being converted from lossy data.
			// False if these transforms are being converted from the original uncompressed data.
			bool is_lossy;

			//////////////////////////////////////////////////////////////////////////
			// True if these transforms are being converted from the additive base pose.
			// False if these transforms are being converted from the additive pose.
			// If not using a base, this value is always false.
			bool is_additive_base;
		};

		//////////////////////////////////////////////////////////////////////////
		// Converts from rtm::qvvf into the transform type used by the error metric.
		// Called when 'needs_conversion' returns true.
		virtual void convert_transforms(const convert_transforms_args& args, void* out_transforms) const
		{
			(void)args;
			(void)out_transforms;
			ACL_ASSERT(false, "Not implemented");
		}

		//////////////////////////////////////////////////////////////////////////
		// Converts from rtm::qvvf into the transform type used by the error metric.
		// Called when 'needs_conversion' returns true.
		virtual void convert_transforms_no_scale(const convert_transforms_args& args, void* out_transforms) const
		{
			(void)args;
			(void)out_transforms;
			ACL_ASSERT(false, "Not implemented");
		}

		//////////////////////////////////////////////////////////////////////////
		// Input arguments for the 'local_to_object_space*' functions.
		//////////////////////////////////////////////////////////////////////////
		struct local_to_object_space_args
		{
			//////////////////////////////////////////////////////////////////////////
			// A list of transform indices that are dirty and need transformation.
			const uint32_t* dirty_transform_indices;

			//////////////////////////////////////////////////////////////////////////
			// The number of dirty transforms that need transformation.
			uint32_t num_dirty_transforms;

			//////////////////////////////////////////////////////////////////////////
			// A list of parent transform indices for every transform.
			// An index of 0xFFFF represents a root transform with no parent.
			const uint32_t* parent_transform_indices;

			//////////////////////////////////////////////////////////////////////////
			// The input transforms in the type expected by the error metric to be transformed.
			const void* local_transforms;

			//////////////////////////////////////////////////////////////////////////
			// The number of transforms in the input and output buffers.
			uint32_t num_transforms;
		};

		//////////////////////////////////////////////////////////////////////////
		// Takes local space transforms into object space.
		virtual void local_to_object_space(const local_to_object_space_args& args, void* out_object_transforms) const
		{
			(void)args;
			(void)out_object_transforms;
			ACL_ASSERT(false, "Not implemented");
		}

		//////////////////////////////////////////////////////////////////////////
		// Takes local space transforms into object space.
		virtual void local_to_object_space_no_scale(const local_to_object_space_args& args, void* out_object_transforms) const
		{
			(void)args;
			(void)out_object_transforms;
			ACL_ASSERT(false, "Not implemented");
		}

		//////////////////////////////////////////////////////////////////////////
		// Input arguments for the 'apply_additive_to_base*' functions.
		//////////////////////////////////////////////////////////////////////////
		struct apply_additive_to_base_args
		{
			//////////////////////////////////////////////////////////////////////////
			// A list of transform indices that are dirty and need the base applied.
			const uint32_t* dirty_transform_indices;

			//////////////////////////////////////////////////////////////////////////
			// The number of dirty transforms that need the base applied.
			uint32_t num_dirty_transforms;

			//////////////////////////////////////////////////////////////////////////
			// The input local space transforms in the type expected by the error metric.
			const void* local_transforms;

			//////////////////////////////////////////////////////////////////////////
			// The input base transforms in the type expected by the error metric.
			const void* base_transforms;

			//////////////////////////////////////////////////////////////////////////
			// The number of transforms in the input and output buffers.
			uint32_t num_transforms;
		};

		//////////////////////////////////////////////////////////////////////////
		// Applies local space transforms on top of base transforms.
		// This is called when a clip has an additive base.
		virtual void apply_additive_to_base(const apply_additive_to_base_args& args, void* out_transforms) const
		{
			(void)args;
			(void)out_transforms;
			ACL_ASSERT(false, "Not implemented");
		}

		//////////////////////////////////////////////////////////////////////////
		// Applies local space transforms on top of base transforms.
		// This is called when a clip has an additive base.
		virtual void apply_additive_to_base_no_scale(const apply_additive_to_base_args& args, void* out_transforms) const
		{
			(void)args;
			(void)out_transforms;
			ACL_ASSERT(false, "Not implemented");
		}

		//////////////////////////////////////////////////////////////////////////
		// Input arguments for the 'calculate_error*' functions.
		//////////////////////////////////////////////////////////////////////////
		struct calculate_error_args
		{
			//////////////////////////////////////////////////////////////////////////
			// A point on our rigid shell along the X axis.
			rtm::vector4f shell_point_x;

			//////////////////////////////////////////////////////////////////////////
			// A point on our rigid shell along the Y axis.
			rtm::vector4f shell_point_y;

			//////////////////////////////////////////////////////////////////////////
			// A point on our rigid shell along the Z axis.
			rtm::vector4f shell_point_z;

			//////////////////////////////////////////////////////////////////////////
			// The first transform used to measure the error.
			// In the type expected by the error metric.
			// Could be in local or object space (same space as lossy).
			const void* transform0;

			//////////////////////////////////////////////////////////////////////////
			// The second transform used to measure the error.
			// In the type expected by the error metric.
			// Could be in local or object space (same space as raw).
			const void* transform1;

			//////////////////////////////////////////////////////////////////////////
			// We measure the error on a rigid shell around each transform.
			// This shell takes the form of a sphere at a certain distance.
			// When no scale is present, measuring any two points is sufficient
			// but when there is scale, measuring all three is necessary.
			// See ./docs/error_metrics.md for details.
			void construct_sphere_shell(float shell_distance)
			{
				shell_point_x = rtm::vector_set(shell_distance, 0.0F, 0.0F, 0.0F);
				shell_point_y = rtm::vector_set(0.0F, shell_distance, 0.0F, 0.0F);
				shell_point_z = rtm::vector_set(0.0F, 0.0F, shell_distance, 0.0F);
			}
		};

		//////////////////////////////////////////////////////////////////////////
		// Measures the error between a raw and lossy transform.
		virtual rtm::scalarf RTM_SIMD_CALL calculate_error(const calculate_error_args& args) const = 0;

		//////////////////////////////////////////////////////////////////////////
		// Measures the error between a raw and lossy transform.
		virtual rtm::scalarf RTM_SIMD_CALL calculate_error_no_scale(const calculate_error_args& args) const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Uses rtm::qvvf arithmetic for local and object space error.
	// Note that this can cause inaccuracy when dealing with shear/skew.
	//////////////////////////////////////////////////////////////////////////
	class qvvf_transform_error_metric : public itransform_error_metric
	{
	public:
		virtual const char* get_name() const override { return "qvvf_transform_error_metric"; }

		virtual size_t get_transform_size(bool has_scale) const override { (void)has_scale; return sizeof(rtm::qvvf); }
		virtual bool needs_conversion(bool has_scale) const override { (void)has_scale; return false; }

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK void local_to_object_space(const local_to_object_space_args& args, void* out_object_transforms) const override
		{
			const uint32_t* dirty_transform_indices = args.dirty_transform_indices;
			const uint32_t* parent_transform_indices = args.parent_transform_indices;
			const rtm::qvvf* local_transforms_ = static_cast<const rtm::qvvf*>(args.local_transforms);
			rtm::qvvf* out_object_transforms_ = static_cast<rtm::qvvf*>(out_object_transforms);

			const uint32_t num_dirty_transforms = args.num_dirty_transforms;
			for (uint32_t dirty_transform_index = 0; dirty_transform_index < num_dirty_transforms; ++dirty_transform_index)
			{
				const uint32_t transform_index = dirty_transform_indices[dirty_transform_index];
				const uint32_t parent_transform_index = parent_transform_indices[transform_index];

				rtm::qvvf obj_transform;
				if (parent_transform_index == k_invalid_track_index)
					obj_transform = local_transforms_[transform_index];	// Just copy the root as-is, it has no parent and thus local and object space transforms are equal
				else
					obj_transform = rtm::qvv_normalize(rtm::qvv_mul(local_transforms_[transform_index], out_object_transforms_[parent_transform_index]));

				out_object_transforms_[transform_index] = obj_transform;
			}
		}

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK void local_to_object_space_no_scale(const local_to_object_space_args& args, void* out_object_transforms) const override
		{
			const uint32_t* dirty_transform_indices = args.dirty_transform_indices;
			const uint32_t* parent_transform_indices = args.parent_transform_indices;
			const rtm::qvvf* local_transforms_ = static_cast<const rtm::qvvf*>(args.local_transforms);
			rtm::qvvf* out_object_transforms_ = static_cast<rtm::qvvf*>(out_object_transforms);

			const uint32_t num_dirty_transforms = args.num_dirty_transforms;
			for (uint32_t dirty_transform_index = 0; dirty_transform_index < num_dirty_transforms; ++dirty_transform_index)
			{
				const uint32_t transform_index = dirty_transform_indices[dirty_transform_index];
				const uint32_t parent_transform_index = parent_transform_indices[transform_index];

				rtm::qvvf obj_transform;
				if (parent_transform_index == k_invalid_track_index)
					obj_transform = local_transforms_[transform_index];	// Just copy the root as-is, it has no parent and thus local and object space transforms are equal
				else
					obj_transform = rtm::qvv_normalize(rtm::qvv_mul_no_scale(local_transforms_[transform_index], out_object_transforms_[parent_transform_index]));

				out_object_transforms_[transform_index] = obj_transform;
			}
		}

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::scalarf RTM_SIMD_CALL calculate_error(const calculate_error_args& args) const override
		{
			const rtm::qvvf& raw_transform_ = *static_cast<const rtm::qvvf*>(args.transform0);
			const rtm::qvvf& lossy_transform_ = *static_cast<const rtm::qvvf*>(args.transform1);

			// Note that because we have scale, we must measure all three axes
			const rtm::vector4f vtx0 = args.shell_point_x;
			const rtm::vector4f vtx1 = args.shell_point_y;
			const rtm::vector4f vtx2 = args.shell_point_z;

			const rtm::vector4f raw_vtx0 = rtm::qvv_mul_point3(vtx0, raw_transform_);
			const rtm::vector4f raw_vtx1 = rtm::qvv_mul_point3(vtx1, raw_transform_);
			const rtm::vector4f raw_vtx2 = rtm::qvv_mul_point3(vtx2, raw_transform_);

			const rtm::vector4f lossy_vtx0 = rtm::qvv_mul_point3(vtx0, lossy_transform_);
			const rtm::vector4f lossy_vtx1 = rtm::qvv_mul_point3(vtx1, lossy_transform_);
			const rtm::vector4f lossy_vtx2 = rtm::qvv_mul_point3(vtx2, lossy_transform_);

			const rtm::scalarf vtx0_error = rtm::vector_distance3_as_scalar(raw_vtx0, lossy_vtx0);
			const rtm::scalarf vtx1_error = rtm::vector_distance3_as_scalar(raw_vtx1, lossy_vtx1);
			const rtm::scalarf vtx2_error = rtm::vector_distance3_as_scalar(raw_vtx2, lossy_vtx2);

			return rtm::scalar_max(rtm::scalar_max(vtx0_error, vtx1_error), vtx2_error);
		}

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::scalarf RTM_SIMD_CALL calculate_error_no_scale(const calculate_error_args& args) const override
		{
			const rtm::qvvf& raw_transform_ = *static_cast<const rtm::qvvf*>(args.transform0);
			const rtm::qvvf& lossy_transform_ = *static_cast<const rtm::qvvf*>(args.transform1);

			const rtm::vector4f vtx0 = args.shell_point_x;
			const rtm::vector4f vtx1 = args.shell_point_y;

			const rtm::vector4f raw_vtx0 = rtm::qvv_mul_point3_no_scale(vtx0, raw_transform_);
			const rtm::vector4f raw_vtx1 = rtm::qvv_mul_point3_no_scale(vtx1, raw_transform_);

			const rtm::vector4f lossy_vtx0 = rtm::qvv_mul_point3_no_scale(vtx0, lossy_transform_);
			const rtm::vector4f lossy_vtx1 = rtm::qvv_mul_point3_no_scale(vtx1, lossy_transform_);

			const rtm::scalarf vtx0_error = rtm::vector_distance3_as_scalar(raw_vtx0, lossy_vtx0);
			const rtm::scalarf vtx1_error = rtm::vector_distance3_as_scalar(raw_vtx1, lossy_vtx1);

			return rtm::scalar_max(vtx0_error, vtx1_error);
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Uses a mix of rtm::qvvf and rtm::matrix3x4f arithmetic.
	// The local space error is always calculated with rtm::qvvf arithmetic.
	// The object space error is calculated with rtm::qvvf arithmetic if there is no scale
	// and with rtm::matrix3x4f arithmetic if there is scale.
	// Note that this can cause inaccuracy issues if there are very large or very small
	// scale values.
	//////////////////////////////////////////////////////////////////////////
	class qvvf_matrix3x4f_transform_error_metric : public qvvf_transform_error_metric
	{
	public:
		virtual const char* get_name() const override { return "qvvf_matrix3x4f_transform_error_metric"; }

		virtual size_t get_transform_size(bool has_scale) const override { return has_scale ? sizeof(rtm::matrix3x4f) : sizeof(rtm::qvvf); }
		virtual bool needs_conversion(bool has_scale) const override { return has_scale; }

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK void convert_transforms(const convert_transforms_args& args, void* out_transforms) const override
		{
			const uint32_t* dirty_transform_indices = args.dirty_transform_indices;
			const rtm::qvvf* transforms_ = args.transforms;
			rtm::matrix3x4f* out_transforms_ = static_cast<rtm::matrix3x4f*>(out_transforms);

			const uint32_t num_dirty_transforms = args.num_dirty_transforms;
			for (uint32_t dirty_transform_index = 0; dirty_transform_index < num_dirty_transforms; ++dirty_transform_index)
			{
				const uint32_t transform_index = dirty_transform_indices[dirty_transform_index];

				const rtm::qvvf& transform_qvv = transforms_[transform_index];
				rtm::matrix3x4f transform_mtx = rtm::matrix_from_qvv(transform_qvv);

				out_transforms_[transform_index] = transform_mtx;
			}
		}

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK void local_to_object_space(const local_to_object_space_args& args, void* out_object_transforms) const override
		{
			const uint32_t* dirty_transform_indices = args.dirty_transform_indices;
			const uint32_t* parent_transform_indices = args.parent_transform_indices;
			const rtm::matrix3x4f* local_transforms_ = static_cast<const rtm::matrix3x4f*>(args.local_transforms);
			rtm::matrix3x4f* out_object_transforms_ = static_cast<rtm::matrix3x4f*>(out_object_transforms);

			const uint32_t num_dirty_transforms = args.num_dirty_transforms;
			for (uint32_t dirty_transform_index = 0; dirty_transform_index < num_dirty_transforms; ++dirty_transform_index)
			{
				const uint32_t transform_index = dirty_transform_indices[dirty_transform_index];
				const uint32_t parent_transform_index = parent_transform_indices[transform_index];

				rtm::matrix3x4f obj_transform;
				if (parent_transform_index == k_invalid_track_index)
					obj_transform = local_transforms_[transform_index];	// Just copy the root as-is, it has no parent and thus local and object space transforms are equal
				else
					obj_transform = rtm::matrix_mul(local_transforms_[transform_index], out_object_transforms_[parent_transform_index]);

				out_object_transforms_[transform_index] = obj_transform;
			}
		}

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::scalarf RTM_SIMD_CALL calculate_error(const calculate_error_args& args) const override
		{
			const rtm::matrix3x4f& raw_transform_ = *static_cast<const rtm::matrix3x4f*>(args.transform0);
			const rtm::matrix3x4f& lossy_transform_ = *static_cast<const rtm::matrix3x4f*>(args.transform1);

			// Note that because we have scale, we must measure all three axes
			const rtm::vector4f vtx0 = args.shell_point_x;
			const rtm::vector4f vtx1 = args.shell_point_y;
			const rtm::vector4f vtx2 = args.shell_point_z;

			const rtm::vector4f raw_vtx0 = rtm::matrix_mul_point3(vtx0, raw_transform_);
			const rtm::vector4f raw_vtx1 = rtm::matrix_mul_point3(vtx1, raw_transform_);
			const rtm::vector4f raw_vtx2 = rtm::matrix_mul_point3(vtx2, raw_transform_);

			const rtm::vector4f lossy_vtx0 = rtm::matrix_mul_point3(vtx0, lossy_transform_);
			const rtm::vector4f lossy_vtx1 = rtm::matrix_mul_point3(vtx1, lossy_transform_);
			const rtm::vector4f lossy_vtx2 = rtm::matrix_mul_point3(vtx2, lossy_transform_);

			const rtm::scalarf vtx0_error = rtm::vector_distance3_as_scalar(raw_vtx0, lossy_vtx0);
			const rtm::scalarf vtx1_error = rtm::vector_distance3_as_scalar(raw_vtx1, lossy_vtx1);
			const rtm::scalarf vtx2_error = rtm::vector_distance3_as_scalar(raw_vtx2, lossy_vtx2);

			return rtm::scalar_max(rtm::scalar_max(vtx0_error, vtx1_error), vtx2_error);
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Uses rtm::qvvf arithmetic for local and object space error.
	// This error metric should be used whenever a clip is additive or relative.
	// Note that this can cause inaccuracy when dealing with shear/skew.
	//////////////////////////////////////////////////////////////////////////
	template<additive_clip_format8 additive_format>
	class additive_qvvf_transform_error_metric : public qvvf_transform_error_metric
	{
	public:
		virtual const char* get_name() const override
		{
			switch (additive_format)
			{
			default:
			case additive_clip_format8::none:			return "additive_qvvf_transform_error_metric<none>";
			case additive_clip_format8::relative:		return "additive_qvvf_transform_error_metric<relative>";
			case additive_clip_format8::additive0:		return "additive_qvvf_transform_error_metric<additive0>";
			case additive_clip_format8::additive1:		return "additive_qvvf_transform_error_metric<additive1>";
			}
		}

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK void apply_additive_to_base(const apply_additive_to_base_args& args, void* out_transforms) const override
		{
			const uint32_t* dirty_transform_indices = args.dirty_transform_indices;
			const rtm::qvvf* local_transforms_ = static_cast<const rtm::qvvf*>(args.local_transforms);
			const rtm::qvvf* base_transforms_ = static_cast<const rtm::qvvf*>(args.base_transforms);
			rtm::qvvf* out_transforms_ = static_cast<rtm::qvvf*>(out_transforms);

			const uint32_t num_dirty_transforms = args.num_dirty_transforms;
			for (uint32_t dirty_transform_index = 0; dirty_transform_index < num_dirty_transforms; ++dirty_transform_index)
			{
				const uint32_t transform_index = dirty_transform_indices[dirty_transform_index];

				const rtm::qvvf& local_transform = local_transforms_[transform_index];
				const rtm::qvvf& base_transform = base_transforms_[transform_index];
				const rtm::qvvf transform = acl::apply_additive_to_base(additive_format, base_transform, local_transform);

				out_transforms_[transform_index] = transform;
			}
		}

		virtual RTM_DISABLE_SECURITY_COOKIE_CHECK void apply_additive_to_base_no_scale(const apply_additive_to_base_args& args, void* out_transforms) const override
		{
			const uint32_t* dirty_transform_indices = args.dirty_transform_indices;
			const rtm::qvvf* local_transforms_ = static_cast<const rtm::qvvf*>(args.local_transforms);
			const rtm::qvvf* base_transforms_ = static_cast<const rtm::qvvf*>(args.base_transforms);
			rtm::qvvf* out_transforms_ = static_cast<rtm::qvvf*>(out_transforms);

			const uint32_t num_dirty_transforms = args.num_dirty_transforms;
			for (uint32_t dirty_transform_index = 0; dirty_transform_index < num_dirty_transforms; ++dirty_transform_index)
			{
				const uint32_t transform_index = dirty_transform_indices[dirty_transform_index];

				const rtm::qvvf& local_transform = local_transforms_[transform_index];
				const rtm::qvvf& base_transform = base_transforms_[transform_index];
				const rtm::qvvf transform = acl::apply_additive_to_base_no_scale(additive_format, base_transform, local_transform);

				out_transforms_[transform_index] = transform;
			}
		}
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
