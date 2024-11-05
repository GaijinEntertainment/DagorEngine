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
  uint3 center__size; uint un00;
  uint3 rot0__rot1; uint un01;
  uint3 rot2__extra; uint un02;

#if __cplusplus
  GravityZoneDescriptor(const TMatrix &transform, const Point3 &size, uint32_t shape, uint32_t type)
  {
    G_STATIC_ASSERT(sizeof(GravityZoneDescriptor) % 16 == 0);

    const float3 &center = transform.col[3];
    center__size = uint3((float_to_half_unsafe(center.x) << 16u) | float_to_half_unsafe(size.x),
      (float_to_half_unsafe(center.y) << 16u) | float_to_half_unsafe(size.y),
      (float_to_half_unsafe(center.z) << 16u) | float_to_half_unsafe(size.z));
    rot0__rot1 = uint3((float_to_half_unsafe(transform.col[0].x) << 16u) | float_to_half_unsafe(transform.col[1].x),
      (float_to_half_unsafe(transform.col[0].y) << 16u) | float_to_half_unsafe(transform.col[1].y),
      (float_to_half_unsafe(transform.col[0].z) << 16u) | float_to_half_unsafe(transform.col[1].z));
    rot2__extra = uint3((float_to_half_unsafe(transform.col[2].x) << 16u) | shape,
      (float_to_half_unsafe(transform.col[2].y) << 16u) | type, (float_to_half_unsafe(transform.col[2].z) << 16u));

    un00 = un01 = un02 = 0u;
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