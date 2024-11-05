// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum class CustomSkyRequest
{
  RENDER_IF_EXISTS,
  CHECK_ONLY
};

struct OcclusionData
{
  mat44f closeGeometryPrevToCurrFrameTransform = {}; // Describes movement of the gun between the previous and the current frame.
  float closeGeometryBoundingRadius = 0.f;
  bool occlusionAvailable = false;
  explicit operator bool() const { return occlusionAvailable; }
};

bool has_custom_sky_render(CustomSkyRequest request);

// returns if it rendered anything
bool render_custom_envi_probe(const ManagedTex *cubeTarget, int face_num);
eastl::vector<Color4> get_custom_envi_probe_spherical_hamornics();
void log_custom_envi_probe_spherical_harmonics(const Color4 *result);

// These are defined in skiesSettingsES.cpp.inl
bool is_panorama_forced();
void validate_volumetric_clouds_settings();

bool needs_water_heightmap(Color4 underwaterFade);
OcclusionData gather_occlusion_data();
bool use_foam_tex();
bool use_wfx_textures();
bool is_camera_inside_custom_gi_zone();
BBox3 get_custom_gi_zone_bbox();