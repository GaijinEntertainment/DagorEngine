// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/validateLods.h>
#include <libTools/util/iLogWriter.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_bounds3.h>

bool LodValidationSettings::loadValidationSettings(const DataBlock &blk)
{
  validate = blk.getBool("validate", false);
  checkLodsGoLesser = blk.getBool("checkLodsGoLesser", false);
  faceDensPerSqPixThres = blk.getReal("faceDensPerSqPixThres", 10.0f);
  warnOnly = blk.getBool("warnOnly", false);
  return true;
}

bool LodValidationSettings::checkLodValid(unsigned lod_idx, unsigned faces, unsigned prev_lod_faces, float lod_range,
  const BBox3 &bbox, unsigned lods_count, ILogWriter &log) const
{
  if (!validate)
    return true;
  bool validation_passed = true;
  if (checkLodsGoLesser && faces >= prev_lod_faces)
  {
    log.addMessage(warnOnly ? ILogWriter::WARNING : ILogWriter::ERROR, "LOD[%d].faces=%d >= LOD[%d].faces=%d", lod_idx, faces,
      lod_idx - 1, prev_lod_faces);
    if (!warnOnly)
      validation_passed = false;
  }
  float pixel_vol =
    max(bbox.width().x, 1e-3f) * max(bbox.width().y, 1e-3f) * max(bbox.width().z, 1e-3f) * powf(1920 / 2 / lod_range, 3);
  float dens = faces / pixel_vol;
  // debug("LOD[%d].faces=%d dens=%g (pixel_vol=%g  %gp/m)", lod_idx, faces, dens, pixel_vol, 1920/2/lod_range);
  if (dens > faceDensPerSqPixThres && lod_idx != lods_count && lods_count > 1)
  {
    log.addMessage(warnOnly ? ILogWriter::WARNING : ILogWriter::ERROR, "LOD[%d].faces=%d .range=%g .dens=%g > %g (suggested range=%g)",
      lod_idx, faces, lod_range, dens, faceDensPerSqPixThres, lod_range * (faceDensPerSqPixThres + 1e-3) / dens);
    if (!warnOnly)
      validation_passed = false;
  }
  return validation_passed;
}
