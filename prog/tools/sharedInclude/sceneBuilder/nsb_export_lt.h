// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// forward declarations for external classes
class ILogWriter;
class IGenericProgressIndicator;
class StaticGeometryContainer;
class IGenLoad;
class IGenSave;

namespace StaticSceneBuilder
{
class IAdditionalLTExport
{
public:
  virtual void write(IGenSave &cb) = 0;
};
bool export_to_light_tool(const char *ltinput, const char *lmsfile, int tex_quality, int lm_size, int target_lm_num,
  float pack_fidelity_ratio, StaticGeometryContainer &geom, StaticGeometryContainer &envi_geom, IAdditionalLTExport *add,
  IGenericProgressIndicator &pbar, ILogWriter &log, float &lm_scale);

bool export_envi_to_LMS1(StaticGeometryContainer &geom, const char *lms_file, IGenericProgressIndicator &pbar, ILogWriter &log);
} // namespace StaticSceneBuilder
