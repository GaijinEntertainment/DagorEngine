//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

void enable_diffuse_mipmaps_debug(bool en);
void dump_unused_shader_variants();
void dump_shader_statistics(const char *filename);
void enable_assert_on_shader_mat_vars(bool en);

void dump_exec_stcode_time();
void enable_overdraw_to_stencil(bool enable);
bool is_overdraw_to_stencil_enabled();
