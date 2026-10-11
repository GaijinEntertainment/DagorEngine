//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

inline constexpr char RENDER_PASS_SPLIT_MODE_INTERVAL[] = "render_pass_split_mode";
inline constexpr int RENDER_PASS_SPLIT_MODE_NATIVE = 0;
inline constexpr int RENDER_PASS_SPLIT_MODE_SPLIT = 1;
inline constexpr int RENDER_PASS_MAX_TARGETS = 16;
inline constexpr int RENDER_PASS_T_REGISTER_COUNT = 32;

inline constexpr int subpass_read_register(int max_t_registers, int read_index) { return max_t_registers - 1 - read_index; }
inline constexpr int subpass_read_window_base(int max_t_registers, int read_count) { return max_t_registers - read_count; }
inline constexpr bool subpass_read_fits_window(int max_t_registers, int read_index)
{
  return read_index >= 0 && read_index < max_t_registers;
}

inline constexpr int render_pass_backend_color_slot(bool is_metal, int target_index, int color_pos)
{
  return is_metal ? target_index : color_pos;
}
