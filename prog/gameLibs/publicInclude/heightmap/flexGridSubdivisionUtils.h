//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct FlexGridSubdivisionUtils
{
  static uint32_t get_level_size(uint32_t level);
  static uint32_t get_level_offset(uint32_t level);
  static uint32_t get_chain_index(uint32_t level, uint32_t x, uint32_t y);
  static uint32_t get_chain_index(uint64_t patch_key);

  static uint64_t get_patch_key(uint32_t level, uint32_t x, uint32_t y);
  static uint32_t get_patch_level(uint64_t page_key);
  static uint32_t get_patch_x(uint64_t page_key);
  static uint32_t get_patch_y(uint64_t page_key);
};

inline uint32_t FlexGridSubdivisionUtils::get_level_size(uint32_t level) { return (1 << level); }

inline uint32_t FlexGridSubdivisionUtils::get_level_offset(uint32_t level) { return uint32_t((1u << (level << 1u)) - 1u) / 3u; }

inline uint32_t FlexGridSubdivisionUtils::get_chain_index(uint32_t level, uint32_t x, uint32_t y)
{
  return FlexGridSubdivisionUtils::get_level_offset(level) + (y << level) + x;
}

inline uint32_t FlexGridSubdivisionUtils::get_chain_index(uint64_t patch_key)
{
  return get_chain_index(get_patch_level(patch_key), get_patch_x(patch_key), get_patch_y(patch_key));
}
