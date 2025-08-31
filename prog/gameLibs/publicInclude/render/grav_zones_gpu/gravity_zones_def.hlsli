#define GRAV_ZONE_SHAPE_SPHERE 0
#define GRAV_ZONE_SHAPE_BOX 1
#define GRAV_ZONE_SHAPE_CYLINDER 2

#define GRAVITY_ZONE_TYPE_LINEAR 0
#define GRAVITY_ZONE_TYPE_RADIAL 1
#define GRAVITY_ZONE_TYPE_RADIAL_INVERTED 2
#define GRAVITY_ZONE_TYPE_CYLINDRICAL 3

#define MAX_GRAV_ZONES 128

#ifndef FLT_EPSILON
#define FLT_EPSILON 1.192092896e-07F
#endif

#define GRAV_ZONES_USE_AABB 0


struct GravZonesPacked
{
  // Cannot pack the itm, or center due to precision reqs
  uint3 size_dir; uint extra__weight;
  float3 itm0; float center_x;
  float3 itm1; float center_y;
  float3 itm2; float center_z;

#ifdef __cplusplus

  GravZonesPacked(const TMatrix &transform, Point3 size, uint shape, uint type, float weight)
  {
    const float3 &dir = (type == GRAVITY_ZONE_TYPE_LINEAR) ? transform.col[1] : transform.col[2];
    const Matrix3 itm = inverse(reinterpret_cast<const Matrix3 &>(transform));
    itm0 = itm.col[0];
    itm1 = itm.col[1];
    itm2 = itm.col[2];
    size_dir = uint3((float_to_half(size.x) << 16u) | float_to_half(dir.x),
                    (float_to_half(size.y) << 16u) | float_to_half(dir.y),
                    (float_to_half(size.z) << 16u) | float_to_half(dir.z));
    extra__weight = (uint(shape) << 18u) | uint(type << 16u) | float_to_half(weight);
    center_x = transform.col[3][0];
    center_y = transform.col[3][1];
    center_z = transform.col[3][2];
  }
#endif
};

struct GravZonesPackedPrecise
{
  // Cannot pack the itm, or center due to precision reqs
  float3 center; uint extra;
  uint3 size_dir; float weight;
  float3 itm0; uint box_TLS_BLS_x;
  float3 itm1; uint box_TLS_BLS_y;
  float3 itm2; uint box_TLS_BLS_z;

#ifdef __cplusplus

  // Boxes
  GravZonesPackedPrecise(const TMatrix &transform, Point3 size, Point3 scale, uint shape, uint type, Point2 borderX, Point2 borderY, Point2 borderZ, float weight, float level) :
    center(transform.col[3]), weight(weight)
  {
    const float3 &dir = (type == GRAVITY_ZONE_TYPE_LINEAR) ? transform.col[1] : transform.col[2];
    const Matrix3 itm = inverse(reinterpret_cast<const Matrix3 &>(transform));
    itm0 = itm.col[0];
    itm1 = itm.col[1];
    itm2 = itm.col[2];
    size_dir = uint3((float_to_half(size.x * 0.5f) << 16u) | float_to_half(dir.x),
                    (float_to_half(size.y * 0.5f) << 16u) | float_to_half(dir.y),
                    (float_to_half(size.z * 0.5f) << 16u) | float_to_half(dir.z));
    extra = (uint(shape) << 18u) | uint(type << 16u) | float_to_half(level);

    // The max border is artist request
    Point3 topLSInv = Point3(scale.x / max(borderX.y, 0.5f), scale.y / max(borderY.y, 0.5f), scale.z / max(borderZ.y, 0.5f));
    Point3 botLSInv = Point3(scale.x / max(borderX.x, 0.5f), scale.y / max(borderY.x, 0.5f), scale.z / max(borderZ.x, 0.5f));
    box_TLS_BLS_x = (float_to_half(topLSInv.x) << 16u) | float_to_half(botLSInv.x);
    box_TLS_BLS_y = (float_to_half(topLSInv.y) << 16u) | float_to_half(botLSInv.y);
    box_TLS_BLS_z = (float_to_half(topLSInv.z) << 16u) | float_to_half(botLSInv.z);

  }
  // Spheres and Cylinders
  GravZonesPackedPrecise(const TMatrix &transform, Point3 size, uint shape, uint type, float weight, float level) :
    center(transform.col[3]), weight(weight)
  {
    const float3 &dir = (type == GRAVITY_ZONE_TYPE_LINEAR) ? transform.col[1] : transform.col[2];
    const Matrix3 itm = inverse(reinterpret_cast<const Matrix3 &>(transform));
    itm0 = itm.col[0];
    itm1 = itm.col[1];
    itm2 = itm.col[2];
    size_dir = uint3((float_to_half(size.x) << 16u) | float_to_half(dir.x),
                    (float_to_half(size.y) << 16u) | float_to_half(dir.y),
                    (float_to_half(size.z) << 16u) | float_to_half(dir.z));

    extra = (uint(shape) << 18u) | uint(type << 16u) | float_to_half(level);
    box_TLS_BLS_x = 0u;
    box_TLS_BLS_y = 0u;
    box_TLS_BLS_z = 0u;
  }
#endif
};

#define CBUFF_STRUCTURE_gravity_zones GravZonesPacked grav_zones [MAX_GRAV_ZONES];

