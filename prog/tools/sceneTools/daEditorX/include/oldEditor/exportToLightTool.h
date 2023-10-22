//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

struct LightingSettings;
class StaticGeometryContainer;
class CtlWndObject;
class ILogWriter;
class IGenericProgressIndicator;
class IGenLoad;

namespace StaticSceneBuilder
{
class StdTonemapper;
class Splitter;
class IAdditionalLTExport;
} // namespace StaticSceneBuilder

namespace DagorEDStaticSceneBuilder
{
bool export_envi_to_new_light_tool(const char *dag_file, const char *lms_file, ILogWriter &rep, IGenericProgressIndicator &bar);

bool export_to_light_tool(StaticGeometryContainer &geom, StaticGeometryContainer &envi_geom,
  StaticSceneBuilder::IAdditionalLTExport *add, LightingSettings &lt_stg, const char *lms_file, int tex_quality, int lm_size,
  int target_lm_num, float pack_fidelity_ratio, StaticSceneBuilder::StdTonemapper &tone_mapper, StaticSceneBuilder::Splitter &splitter,
  bool create_bld, real &lm_scale);
} // namespace DagorEDStaticSceneBuilder
