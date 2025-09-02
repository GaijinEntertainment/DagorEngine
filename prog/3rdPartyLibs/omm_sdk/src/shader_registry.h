/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

// SHADERS =================================================================================

#ifdef OMM_ENABLE_PRECOMPILED_SHADERS_DXIL
	#include "omm_clear_buffer.cs.dxil.h"
	#include "omm_render_target_clear.ps.dxil.h"

	#include "omm_init_buffers_cs.cs.dxil.h"
	#include "omm_init_buffers_gfx.cs.dxil.h"
	#include "omm_work_setup_cs.cs.dxil.h"
	#include "omm_work_setup_bake_only_cs.cs.dxil.h"
	#include "omm_work_setup_gfx.cs.dxil.h"
	#include "omm_work_setup_bake_only_gfx.cs.dxil.h"
	#include "omm_post_build_info.cs.dxil.h"
	#include "omm_desc_patch.cs.dxil.h"
	#include "omm_index_write.cs.dxil.h"
	#include "omm_rasterize.vs.dxil.h"
	#include "omm_rasterize.gs.dxil.h"
	#include "omm_rasterize_ps_r.ps.dxil.h"
	#include "omm_rasterize_ps_g.ps.dxil.h"
	#include "omm_rasterize_ps_b.ps.dxil.h"
	#include "omm_rasterize_ps_a.ps.dxil.h"
	#include "omm_rasterize_cs_r.cs.dxil.h"
	#include "omm_rasterize_cs_g.cs.dxil.h"
	#include "omm_rasterize_cs_b.cs.dxil.h"
	#include "omm_rasterize_cs_a.cs.dxil.h"
	#include "omm_compress.cs.dxil.h"

	#include "omm_rasterize_debug.vs.dxil.h"
	#include "omm_rasterize_debug.ps.dxil.h"

#endif

#ifdef OMM_ENABLE_PRECOMPILED_SHADERS_SPIRV
	#include "omm_clear_buffer.cs.spirv.h"
	#include "omm_render_target_clear.ps.spirv.h"

	#include "omm_init_buffers_cs.cs.spirv.h"
	#include "omm_init_buffers_gfx.cs.spirv.h"
	#include "omm_work_setup_cs.cs.spirv.h"
	#include "omm_work_setup_bake_only_cs.cs.spirv.h"
	#include "omm_work_setup_gfx.cs.spirv.h"
	#include "omm_work_setup_bake_only_gfx.cs.spirv.h"
	#include "omm_post_build_info.cs.spirv.h"
	#include "omm_desc_patch.cs.spirv.h"
	#include "omm_index_write.cs.spirv.h"
	#include "omm_rasterize.vs.spirv.h"
	#include "omm_rasterize.gs.spirv.h"
	#include "omm_rasterize_ps_r.ps.spirv.h"
	#include "omm_rasterize_ps_g.ps.spirv.h"
	#include "omm_rasterize_ps_b.ps.spirv.h"
	#include "omm_rasterize_ps_a.ps.spirv.h"
	#include "omm_rasterize_cs_r.cs.spirv.h"
	#include "omm_rasterize_cs_g.cs.spirv.h"
	#include "omm_rasterize_cs_b.cs.spirv.h"
	#include "omm_rasterize_cs_a.cs.spirv.h"
	#include "omm_compress.cs.spirv.h"

	#include "omm_rasterize_debug.vs.spirv.h"
	#include "omm_rasterize_debug.ps.spirv.h"
#endif

// GLOBAL CONSTANT BUFFER =================================================================================

#define OMM_CONSTANTS_START(name) struct name {
#define OMM_CONSTANT(type, name) type name;
#define OMM_CONSTANTS_END(name, registerIndex) };

#define OMM_PUSH_CONSTANTS_START(name) struct name {
#define OMM_PUSH_CONSTANT(type, name) type name;
#define OMM_PUSH_CONSTANTS_END(name, registerIndex) };
#include "omm_global_cb.hlsli"
OMM_DECLARE_GLOBAL_CONSTANT_BUFFER

// SHADER BINDINGS =========================================================================
BEGIN_DECLARE_SHADER_BINDINGS(omm_clear_buffer_cs_bindings)
#include "omm_clear_buffer.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_init_buffers_cs_cs_bindings)
#include "omm_init_buffers_cs.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_init_buffers_gfx_cs_bindings)
#include "omm_init_buffers_gfx.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_work_setup_cs_cs_bindings)
#include "omm_work_setup_cs.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_work_setup_bake_only_cs_cs_bindings)
#include "omm_work_setup_bake_only_cs.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_work_setup_bake_only_gfx_cs_bindings)
#include "omm_work_setup_bake_only_gfx.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_work_setup_gfx_cs_bindings)
#include "omm_work_setup_gfx.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_post_build_info_cs_bindings)
#include "omm_post_build_info.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_rasterize_bindings)
#include "omm_rasterize.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_rasterize_cs_bindings)
#include "omm_rasterize.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_compress_cs_bindings)
#include "omm_compress.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_desc_patch_cs_bindings)
#include "omm_desc_patch.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_index_write_cs_bindings)
#include "omm_index_write.cs.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************
BEGIN_DECLARE_SHADER_BINDINGS(omm_render_target_clear_bindings)
#include "omm_render_target_clear.ps.resources.hlsli"
#include "shader_bindings_expand.h"
END_DECLARE_SHADER_BINDINGS
/// **********************************************************************************