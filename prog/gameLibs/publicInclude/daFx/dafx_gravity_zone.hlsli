#ifndef DAFX_GRAVITY_ZONE_HLSLI
#define DAFX_GRAVITY_ZONE_HLSLI

#define GRAVITY_ZONE_DEFAULT 0
#define GRAVITY_ZONE_DISABLED 1
#define GRAVITY_ZONE_PER_EMITTER 2
#define GRAVITY_ZONE_PER_PARTICLE 3

#define GRAVITY_ZONE_SHAPE_SPHERE 0
#define GRAVITY_ZONE_SHAPE_BOX 1
#define GRAVITY_ZONE_SHAPE_CYLINDER 2

#define GRAVITY_ZONE_TYPE_LINEAR 0
#define GRAVITY_ZONE_TYPE_RADIAL 1
#define GRAVITY_ZONE_TYPE_RADIAL_INVERTED 2
#define GRAVITY_ZONE_TYPE_CYLINDRICAL 3

struct GravityZoneDescriptor {
  uint3 center__size; float dir_x;
  uint3 itm0__itm1; float dir_y;
  uint3 itm2__extra; float dir_z;

#if __cplusplus
  GravityZoneDescriptor(const TMatrix &transform, const Point3 &size, uint32_t shape, uint32_t type, float weight)
  {
    G_STATIC_ASSERT(sizeof(GravityZoneDescriptor) % 16 == 0);

    const float3 &center = transform.col[3];
    const float3 &dir = (type == GRAVITY_ZONE_TYPE_LINEAR) ? transform.col[1] : transform.col[2];
    const Matrix3 itm = inverse(reinterpret_cast<const Matrix3 &>(transform));
    center__size = uint3((float_to_half_unsafe(center.x) << 16u) | float_to_half_unsafe(size.x),
      (float_to_half_unsafe(center.y) << 16u) | float_to_half_unsafe(size.y),
      (float_to_half_unsafe(center.z) << 16u) | float_to_half_unsafe(size.z));
    itm0__itm1 = uint3((float_to_half_unsafe(itm.col[0].x) << 16u) | float_to_half_unsafe(itm.col[1].x),
      (float_to_half_unsafe(itm.col[0].y) << 16u) | float_to_half_unsafe(itm.col[1].y),
      (float_to_half_unsafe(itm.col[0].z) << 16u) | float_to_half_unsafe(itm.col[1].z));
    itm2__extra = uint3((float_to_half_unsafe(itm.col[2].x) << 16u) | shape,
      (float_to_half_unsafe(itm.col[2].y) << 16u) | type, (float_to_half_unsafe(itm.col[2].z) << 16u) | float_to_half_unsafe(weight));
    dir_x = dir.x;
    dir_y = dir.y;
    dir_z = dir.z;
  }
#endif
};

#ifdef __cplusplus
#define GravityZoneDescriptor_cref const GravityZoneDescriptor &
#define GravityZoneDescriptor_buffer const GravityZoneDescriptor *
#else
#define GravityZoneDescriptor_cref GravityZoneDescriptor
#endif

// NOTE: if update here, don't forget to update in dafx_helpers.dshl
#define GRAVITY_ZONE_MAX_COUNT 8

#endif