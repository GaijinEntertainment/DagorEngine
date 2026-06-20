//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#ifndef CLUSTERED_LIGHTS_GRID_INCLUDED
#define CLUSTERED_LIGHTS_GRID_INCLUDED 1

#ifndef __cplusplus
  #define CLG_unsigned_int32 uint
  #define CLG_signed_int32 int
  #define CLG_float_4 float4
  #define CLG_INLINE
#else
  #define CLG_unsigned_int32 uint32_t
  #define CLG_signed_int32 int32_t
  #define CLG_float_4 Point4
  #define CLG_INLINE static inline
#endif

// CLG_FrustumScreenRect packs four cluster-grid coordinates into one uint32:
//   bits [ 7: 0] = min_x  (8 bits, 0..CLUSTERS_W-1)
//   bits [15: 8] = max_x  (8 bits, 0..CLUSTERS_W-1)
//   bits [23:16] = min_y  (8 bits, 0..CLUSTERS_H-1)
//   bits [31:24] = max_y  (8 bits, 0..CLUSTERS_H-1)
// Invalid sentinel: min_x=1, max_x=0  ->  packed_values == 1u
struct CLG_FrustumScreenRect
{
  CLG_unsigned_int32 packed_values;
  CLG_signed_int32 center_y;
};

// CLG_ItemRect3D packs z-slice range into one uint32:
//   bits [15: 0] = zmin  (16 bits, 0..CLUSTERS_D-1)
//   bits [31:16] = zmax  (16 bits, 0..CLUSTERS_D-1)
struct CLG_ItemRect3D
{
  CLG_FrustumScreenRect fsRect;
  CLG_unsigned_int32 z_bounds;
};

struct CLG_LightData
{
  CLG_ItemRect3D rect3d;
  CLG_unsigned_int32 pad0;
  CLG_float_4 viewSpaceBoundSpherePosRadius;
};

CLG_INLINE CLG_unsigned_int32 clg_zmin(CLG_unsigned_int32 z_bounds) { return  z_bounds        & 0xFFFFu; }
CLG_INLINE CLG_unsigned_int32 clg_zmax(CLG_unsigned_int32 z_bounds) { return (z_bounds >> 16) & 0xFFFFu; }

CLG_INLINE CLG_unsigned_int32 clg_rect_min_x(CLG_FrustumScreenRect r) { return  r.packed_values        & 0xFFu; };
CLG_INLINE CLG_unsigned_int32 clg_rect_max_x(CLG_FrustumScreenRect r) { return (r.packed_values >>  8) & 0xFFu; };
CLG_INLINE CLG_unsigned_int32 clg_rect_min_y(CLG_FrustumScreenRect r) { return (r.packed_values >> 16) & 0xFFu; };
CLG_INLINE CLG_unsigned_int32 clg_rect_max_y(CLG_FrustumScreenRect r) { return (r.packed_values >> 24) & 0xFFu; };

#ifndef __cplusplus

uint clg_pack_z_bounds(uint zmin, uint zmax) { return (zmin & 0xFFFFu) | ((zmax & 0xFFFFu) << 16); }

CLG_FrustumScreenRect clg_pack_frustrum_screen_rect(uint min_x, uint max_x, uint min_y, uint max_y, int center_y)
{
  CLG_FrustumScreenRect r;
  r.center_y = center_y;
  r.packed_values = (min_x & 0xFFu) | ((max_x & 0xFFu) << 8) | ((min_y & 0xFFu) << 16) | ((max_y & 0xFFu) << 24);
  return r;
};

bool is_frustrum_screen_rect_valid(CLG_FrustumScreenRect rect)
{
  return clg_rect_min_x(rect) <= clg_rect_max_x(rect) &&
         clg_rect_min_y(rect) <= clg_rect_max_y(rect);
};

void make_frustrum_screen_invalid(inout CLG_FrustumScreenRect rect)
{
  rect.packed_values = 1u; // min_x=1 > max_x=0
};

#endif

#undef CLG_INLINE
#undef CLG_unsigned_int32
#undef CLG_signed_int32
#undef CLG_float_4
#endif