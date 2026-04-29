//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_meshShaders.h>
#include <util/dag_generationRefId.h>
#include <util/dag_globDef.h>
#include <drv/3d/dag_shaderModelVersion.h>

class MaterialData;

struct ShaderBindumpHandleDummy
{};
using ShaderBindumpHandle = GenerationRefId<8, ShaderBindumpHandleDummy>;

inline constexpr ShaderBindumpHandle MAIN_BINDUMP_HANDLE = ShaderBindumpHandle::make(uint32_t(-2), 0);
inline constexpr ShaderBindumpHandle SEC_EXP_BINDUMP_HANDLE = ShaderBindumpHandle::make(uint32_t(-3), 0);
inline constexpr ShaderBindumpHandle INVALID_BINDUMP_HANDLE = ShaderBindumpHandle{ShaderBindumpHandle::INVALID_ID};

ShaderMaterial *new_shader_material(ShaderBindumpHandle hnd, MaterialData const &m, bool do_log = true);
ShaderMaterial *new_shader_material_by_name_optional(ShaderBindumpHandle hnd, char const *shader_name,
  char const *mat_script = nullptr);
ShaderMaterial *new_shader_material_by_name(ShaderBindumpHandle hnd, char const *shader_name, char const *mat_script = nullptr);
ComputeShaderElement *new_compute_shader(ShaderBindumpHandle hnd, char const *shader_name, bool optional = false);
MeshShaderElement *new_mesh_shader(ShaderBindumpHandle hnd, char const *shader_name, bool optional = false);

bool shader_exists(ShaderBindumpHandle hnd, char const *shader_name);
char const *get_shader_class_name_by_material_name(ShaderBindumpHandle hnd, char const *mat_name);

ShaderBindumpHandle load_additional_shaders_bindump(dag::ConstSpan<uint8_t> dump_data, char const *dump_name);
ShaderBindumpHandle load_additional_shaders_bindump(char const *src_filename, d3d::shadermodel::Version shader_model_version);
ShaderBindumpHandle load_additional_shaders_bindump(char const *src_filename, char const *name_override,
  d3d::shadermodel::Version shader_model_version);
bool reload_shaders_bindump(ShaderBindumpHandle hnd, dag::ConstSpan<uint8_t> dump_data, char const *dump_name);
bool reload_shaders_bindump(ShaderBindumpHandle hnd, char const *src_filename, d3d::shadermodel::Version shader_model_version);
bool reload_shaders_bindump(ShaderBindumpHandle hnd, char const *src_filename, char const *name_override,
  d3d::shadermodel::Version shader_model_version);
void unload_shaders_bindump(ShaderBindumpHandle hnd);

uint32_t get_shaders_bindump_generation(ShaderBindumpHandle hnd);
