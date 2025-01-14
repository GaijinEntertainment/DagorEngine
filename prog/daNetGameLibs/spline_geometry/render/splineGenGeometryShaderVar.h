// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

extern int dynamicSceneBlockId, dynamicDepthSceneBlockId;

#define SPLINE_GEN_VARS_LIST            \
  VAR(spline_gen_instancing_buffer)     \
  VAR(spline_gen_spline_buffer)         \
  VAR(spline_gen_indirection_buffer)    \
  VAR(spline_gen_vertex_buffer)         \
  VAR(spline_gen_prev_vertex_buffer)    \
  VAR(spline_gen_stripes)               \
  VAR(spline_gen_slices)                \
  VAR(spline_gen_vertex_count)          \
  VAR(spline_gen_texture_d)             \
  VAR(spline_gen_texture_n)             \
  VAR(spline_gen_attachment_data)       \
  VAR(spline_gen_prev_attachment_data)  \
  VAR(spline_gen_attachment_batch_size) \
  VAR(spline_gen_attachment_max_no)     \
  VAR(spline_gen_instance_count)        \
  VAR(spline_gen_active_instance_count) \
  VAR(spline_gen_culled_buffer)         \
  VAR(spline_gen_obj_elem_count)        \
  VAR(spline_gen_obj_batch_id_buffer)

#define VAR(a) extern int a##VarId;
SPLINE_GEN_VARS_LIST
#undef VAR