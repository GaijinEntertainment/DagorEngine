#pragma once

#include <3d/dag_tex3d.h>

namespace rubgeneric
{
struct ResUpdateBuffer
{
  char *ptr;
  size_t size;
  BaseTexture *staging;
  size_t pitch, slicePitch;
  uint16_t x, y, z;
  uint16_t w, h, d;
  unsigned cflg;
  BaseTexture *destTex;
  uint16_t destMip, destSlice;
};

ResUpdateBuffer *allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip, unsigned dest_slice,
  unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth);
ResUpdateBuffer *allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice);
void release_update_buffer(ResUpdateBuffer *&rub);

inline char *get_update_buffer_addr_for_write(ResUpdateBuffer *rub) { return rub ? rub->ptr : nullptr; }
inline size_t get_update_buffer_size(ResUpdateBuffer *rub) { return rub ? rub->size : 0; }
inline size_t get_update_buffer_pitch(ResUpdateBuffer *rub) { return rub ? rub->pitch : 0; }
inline size_t get_update_buffer_slice_pitch(ResUpdateBuffer *rub) { return rub ? rub->slicePitch : 0; }

bool update_texture_and_release_update_buffer(ResUpdateBuffer *&src_rub);
} // namespace rubgeneric

#define IMPLEMENT_D3D_RUB_API_USING_GENERIC()                                                                                      \
  d3d::ResUpdateBuffer *d3d::allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip,              \
    unsigned dest_slice, unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth) \
  {                                                                                                                                \
    return reinterpret_cast<d3d::ResUpdateBuffer *>(rubgeneric::allocate_update_buffer_for_tex_region(dest_base_texture, dest_mip, \
      dest_slice, offset_x, offset_y, offset_z, width, height, depth));                                                            \
  }                                                                                                                                \
  d3d::ResUpdateBuffer *d3d::allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice)                   \
  {                                                                                                                                \
    return reinterpret_cast<d3d::ResUpdateBuffer *>(rubgeneric::allocate_update_buffer_for_tex(dest_tex, dest_mip, dest_slice));   \
  }                                                                                                                                \
  void d3d::release_update_buffer(d3d::ResUpdateBuffer *&rub)                                                                      \
  {                                                                                                                                \
    rubgeneric::release_update_buffer(reinterpret_cast<rubgeneric::ResUpdateBuffer *&>(rub));                                      \
  }                                                                                                                                \
  char *d3d::get_update_buffer_addr_for_write(d3d::ResUpdateBuffer *rub)                                                           \
  {                                                                                                                                \
    return rubgeneric::get_update_buffer_addr_for_write(reinterpret_cast<rubgeneric::ResUpdateBuffer *>(rub));                     \
  }                                                                                                                                \
  size_t d3d::get_update_buffer_size(d3d::ResUpdateBuffer *rub)                                                                    \
  {                                                                                                                                \
    return rubgeneric::get_update_buffer_size(reinterpret_cast<rubgeneric::ResUpdateBuffer *>(rub));                               \
  }                                                                                                                                \
  size_t d3d::get_update_buffer_pitch(d3d::ResUpdateBuffer *rub)                                                                   \
  {                                                                                                                                \
    return rubgeneric::get_update_buffer_pitch(reinterpret_cast<rubgeneric::ResUpdateBuffer *>(rub));                              \
  }                                                                                                                                \
  size_t d3d::get_update_buffer_slice_pitch(d3d::ResUpdateBuffer *rub)                                                             \
  {                                                                                                                                \
    return rubgeneric::get_update_buffer_slice_pitch(reinterpret_cast<rubgeneric::ResUpdateBuffer *>(rub));                        \
  }                                                                                                                                \
  bool d3d::update_texture_and_release_update_buffer(d3d::ResUpdateBuffer *&rub)                                                   \
  {                                                                                                                                \
    return rubgeneric::update_texture_and_release_update_buffer(reinterpret_cast<rubgeneric::ResUpdateBuffer *&>(rub));            \
  }
