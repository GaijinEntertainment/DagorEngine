// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/occlusion/occlusionMaskApplier.h>
#include <perfMon/dag_statDrv.h>
#include <scene/dag_occlusion.h>


void OcclusionMaskApplier::apply(Occlusion &occlusion)
{
  if (!nearPlaneWithHoleTask)
    return;

  TIME_PROFILE(OcclusionMaskApplier_apply);
  // if the pixel outside the canonical Ellipse => return near plane
  // else returns max
  //
  // uv = cell center
  // dr = (uv.x - center.x, uv.y * yScale - center.y)
  // r2 = dot(d, xAxis)^2 + dot(d, yAxis)^2
  // inside = r2 <= 1 |  (x/A)^2 + (y/b)^2 <= 1

  const OcclusionEllipse &ellipseHole = nearPlaneWithHoleTask->ellipseHole;
  const float zn = nearPlaneWithHoleTask->zn;

  const vec4f axisXx = v_splat_x(ellipseHole.axes);
  const vec4f axisXy = v_splat_y(ellipseHole.axes);
  const vec4f axisYx = v_splat_z(ellipseHole.axes);
  const vec4f axisYy = v_splat_w(ellipseHole.axes);
  const vec4f centerX = v_splat_x(ellipseHole.centerScale);
  const vec4f centerY = v_splat_y(ellipseHole.centerScale);
  const vec4f yScale = v_splat_z(ellipseHole.centerScale);

  const vec4f forcedNearZ = v_splats(OcclusionZBuffer::Z_SCALE / zn);
  const float rcpW = 1.0f / (float)OcclusionZBuffer::WIDTH;
  const float rcpH = 1.0f / (float)OcclusionZBuffer::HEIGHT;

  const vec4f uvxBase = v_mul(v_make_vec4f(0.5f, 1.5f, 2.5f, 3.5f), v_splats(rcpW));
  const vec4f uvxStep = v_splats(4.0f * rcpW);

  const int rowMin = max(0, (int)v_extract_y(ellipseHole.cellBox));
  const int rowMax = min((int)OcclusionZBuffer::HEIGHT, (int)v_extract_w(ellipseHole.cellBox) + 1);
  const int colMin = max(0, (int)v_extract_x(ellipseHole.cellBox));
  const int colMax = min((int)OcclusionZBuffer::WIDTH, (int)v_extract_z(ellipseHole.cellBox) + 1);
  constexpr int rowVecs = OcclusionZBuffer::WIDTH / 4;

  vec4f *zbuf = (vec4f *)occlusion.getOcclusionZBuffer().getZbuffer();
  for (int y = 0; y < OcclusionZBuffer::HEIGHT; y++)
  {
    if (y < rowMin || y >= rowMax)
    {
      for (int i = 0; i < rowVecs; i++)
        zbuf[i] = forcedNearZ;
      zbuf += rowVecs;
      continue;
    }
    const vec4f dY = v_sub(v_mul(v_splats((y + 0.5f) * rcpH), yScale), centerY);
    vec4f uvx = uvxBase;
    for (int x = 0; x < OcclusionZBuffer::WIDTH; x += 4, zbuf++, uvx = v_add(uvx, uvxStep))
    {
      if (x + 4 <= colMin || x >= colMax)
      {
        *zbuf = forcedNearZ;
        continue;
      }
      const vec4f dX = v_sub(uvx, centerX);
      const vec4f px = v_madd(dX, axisXx, v_mul(dY, axisXy));
      const vec4f py = v_madd(dX, axisYx, v_mul(dY, axisYy));
      const vec4f r2 = v_madd(px, px, v_mul(py, py));
      const vec4f inside = v_cmp_ge(v_splats(1.0f), r2);

      const vec4f curVal = *zbuf;
      *zbuf = v_sel(forcedNearZ, curVal, inside);
    }
  }

  nearPlaneWithHoleTask.reset();
}
