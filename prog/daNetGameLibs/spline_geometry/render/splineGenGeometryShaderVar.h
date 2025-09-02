// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVariableInfo.h>

extern ShaderBlockIdHolder dynamicSceneBlockId, dynamicDepthSceneBlockId;

namespace var
{
static ShaderVariableInfo spline_gen_instancing_buffer("spline_gen_instancing_buffer", true);
static ShaderVariableInfo spline_gen_spline_buffer("spline_gen_spline_buffer", true);
static ShaderVariableInfo spline_gen_prev_spline_buffer("spline_gen_prev_spline_buffer", true);
static ShaderVariableInfo spline_gen_indirection_buffer("spline_gen_indirection_buffer", true);
static ShaderVariableInfo spline_gen_stripes("spline_gen_stripes", true);
static ShaderVariableInfo spline_gen_slices("spline_gen_slices", true);
static ShaderVariableInfo spline_gen_vertex_count("spline_gen_vertex_count", true);
static ShaderVariableInfo spline_gen_texture_d("spline_gen_texture_d", true);
static ShaderVariableInfo spline_gen_texture_d_samplerstate("spline_gen_texture_d_samplerstate", true);
static ShaderVariableInfo spline_gen_texture_n("spline_gen_texture_n", true);
static ShaderVariableInfo spline_gen_texture_n_samplerstate("spline_gen_texture_n_samplerstate", true);
static ShaderVariableInfo spline_gen_texture_emissive_mask("spline_gen_texture_emissive_mask", true);
static ShaderVariableInfo spline_gen_texture_emissive_mask_samplerstate("spline_gen_texture_emissive_mask_samplerstate", true);
static ShaderVariableInfo spline_gen_texture_skin_ao("spline_gen_texture_skin_ao", true);
static ShaderVariableInfo spline_gen_texture_skin_ao_samplerstate("spline_gen_texture_skin_ao_samplerstate", true);
static ShaderVariableInfo spline_gen_attachment_data("spline_gen_attachment_data", true);
static ShaderVariableInfo spline_gen_prev_attachment_data("spline_gen_prev_attachment_data", true);
static ShaderVariableInfo spline_gen_attachment_batch_size("spline_gen_attachment_batch_size", true);
static ShaderVariableInfo spline_gen_attachment_max_no("spline_gen_attachment_max_no", true);
static ShaderVariableInfo spline_gen_instance_count("spline_gen_instance_count", true);
static ShaderVariableInfo spline_gen_active_instance_count("spline_gen_active_instance_count", true);
static ShaderVariableInfo spline_gen_culled_buffer("spline_gen_culled_buffer", true);
static ShaderVariableInfo spline_gen_obj_elem_count("spline_gen_obj_elem_count", true);
static ShaderVariableInfo spline_gen_obj_batch_id_buffer("spline_gen_obj_batch_id_buffer", true);
} // namespace var