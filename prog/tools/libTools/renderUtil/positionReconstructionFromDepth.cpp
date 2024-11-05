// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texMgr.h>


namespace position_reconstruction_from_depth
{

float linearize_z(float raw_z, Point2 decode_depth) { return 1.0f / (decode_depth.x + decode_depth.y * raw_z); }

float linearize_z(float raw_z, float near_z, float far_z)
{
  return linearize_z(raw_z, Point2(safediv(1.0f, far_z), safediv(far_z - near_z, near_z * far_z)));
}

Point3 reconstruct_ws_position_from_raw_depth(const TMatrix &view_tm, const TMatrix4 &proj_tm, Point2 sample_pos, float raw_z)
{
  TMatrix4 viewProjInv = inverse44(TMatrix4(view_tm) * proj_tm);
  Point4 viewVec = Point4(sample_pos.x * 2 - 1, 1 - sample_pos.y * 2, raw_z, 1) * viewProjInv;
  viewVec /= viewVec.w;
  return Point3(viewVec.x, viewVec.y, viewVec.z);
}

Point3 reconstruct_ws_position_from_linear_depth(const TMatrix &view_tm, const TMatrix4 &proj_tm, Point2 sample_pos, float linear_z)
{
  Point3 worldPos = inverse(view_tm).getcol(3);
  TMatrix4 viewRot = TMatrix4(view_tm);
  viewRot.setrow(3, 0, 0, 0, 1);
  TMatrix4 viewRotProjInv = inverse44(viewRot * proj_tm);
  Point3 viewVec = Point3(sample_pos.x * 2 - 1, 1 - sample_pos.y * 2, 1) * viewRotProjInv;
  return viewVec * linear_z + worldPos;
}

// potentially VERY slow, should only be used in tools
// depth texture type must be R32 (float)
bool get_position_from_depth(const TMatrix &view_tm, const TMatrix4 &proj_tm, TEXTUREID depth_texture_id, const Point2 &sample_pos,
  Point3 &out_world_pos)
{
  bool success = false;
  BaseTexture *depthTex = depth_texture_id != BAD_TEXTUREID ? acquire_managed_tex(depth_texture_id) : nullptr;
  if (depthTex)
  {
    float *data; // R32 (float)
    int stride;
    if (depthTex->lockimgEx(&data, stride, 0, TEXLOCK_READ))
    {
      TextureInfo ti;
      depthTex->getinfo(ti);
      int sampleXi = clamp(static_cast<int>(round(sample_pos.x * ti.w)), 0, ti.w - 1);
      int sampleYi = clamp(static_cast<int>(round(sample_pos.y * ti.h)), 0, ti.h - 1);
      float rawZ = data[sampleXi + sampleYi * stride / 4];
      if (rawZ > 0) // to avoid hitting the skybox
      {
        out_world_pos = reconstruct_ws_position_from_raw_depth(view_tm, proj_tm, sample_pos, rawZ);
        success = true;
      }
      depthTex->unlockimg(); // TODO: should use some wrapper with dtor for these
    }
    release_managed_tex(depth_texture_id);
  }
  return success;
}

} // namespace position_reconstruction_from_depth
