// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_tab.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_commands.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>

#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>

#include <drv/3d/dag_driver.h>

#include <validate_sbuf_flags.h>

#include "render.h"
#include "drv_log_defs.h"
#include "drv_assert_defs.h"

using namespace drv3d_metal;

extern bool dirty_render_target();

VDECL d3d::create_vdecl(VSDTYPE* d)
{
  return render.createVDdecl(d);
}

void d3d::delete_vdecl(VDECL vdecl)
{
  render.deleteVDecl(vdecl);
}

bool d3d::setvdecl(VDECL vdecl)
{
  render.setVDecl(vdecl);
  return true;
}

Sbuffer *d3d::create_vb(int size, int flg, const char* name)
{
  flg |= SBCF_BIND_VERTEX;
  validate_sbuffer_flags(flg, name);
  return new Buffer(size, 0, flg, 0, name);
}

Sbuffer *d3d::create_ib(int size, int flg, const char *name)
{
  flg |= SBCF_BIND_INDEX;
  validate_sbuffer_flags(flg, name);
  return new Buffer(size, 0, flg, 0, name);
}

Sbuffer *d3d::create_sbuffer(int struct_size, int elements, unsigned flags, unsigned format, const char* name)
{
  validate_sbuffer_flags(flags, name);
  return new Buffer(elements, struct_size, flags, format, name);
}

bool set_buffer_ex(unsigned shader_stage, BufferType buf_type, int slot, Buffer *vb, int offset, int stride)
{
  Buffer* bufMetal = (Buffer*)vb;
  // yet another special case, don't invalidate readback frame if the buffer has readback flag
  // it should be read back by async readback lock magic call
  if (bufMetal && buf_type == RW_BUFFER && (bufMetal->bufFlags & SBCF_USAGE_READ_BACK) == 0)
    bufMetal->last_locked_submit = render.submits_scheduled;
  render.setBuff(shader_stage, buf_type, slot, bufMetal, offset, stride);

  return true;
}

bool d3d::setvsrc_ex(int slot, Sbuffer *vb, int offset, int stride)
{
  return set_buffer_ex(STAGE_VS, GEOM_BUFFER, slot, (Buffer*)vb, offset, stride);
}

bool d3d::set_const_buffer(unsigned stage, unsigned slot, Sbuffer* buffer, uint32_t consts_offset, uint32_t consts_size)
{
  D3D_CONTRACT_ASSERT_RETURN(!consts_offset && !consts_size, false); //not implemented, not tested
  return set_buffer_ex(stage, CONST_BUFFER, slot, (Buffer*)buffer, 0, 0);
}

bool d3d::set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer)
{
  int offset = 0;
  Buffer* buf = (Buffer*)buffer;
  return set_buffer_ex(shader_stage, STRUCT_BUFFER, slot, buf, offset, 0);
}

bool d3d::set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer)
{
  return set_buffer_ex(shader_stage, RW_BUFFER, slot, (Buffer*)buffer, 0, 0);
}

bool d3d::setind(Sbuffer *ib)
{
  render.setIBuff((Buffer*)ib);

  return true;
}

static inline uint32_t nprim_to_nverts(uint32_t prim_type, uint32_t numprim)
{
  //table look-up: 4 bits per entry [2b mul 2bit add]
  static const uint64_t table =
    (0x0ULL << (4 * PRIM_POINTLIST))  // 1+0 00/00
    | (0x4ULL << (4 * PRIM_LINELIST))   // 2+0 01/00
    | (0x1ULL << (4 * PRIM_LINESTRIP))  // 1+1 00/01
    | (0x8ULL << (4 * PRIM_TRILIST))  // 3+0 10/00
    | (0x2ULL << (4 * PRIM_TRISTRIP))   // 1+2 00/10
    | (0x8ULL << (4 * PRIM_TRIFAN))   // 1+2 00/10
                      //| (0xcLL << 4*PRIM_QUADLIST)   // 4+0 11/00
                      //| (0x3LL << 4*PRIM_QUADSTRIP);   // 1+3 00/11
    ;

  const uint32_t code = uint32_t((table >> (prim_type * 4)) & 0x0f);
  return numprim * ((code >> 2) + 1) + (code & 3);
}

void check_primtype(const int prim_type)
{
  D3D_CONTRACT_ASSERT(prim_type >= 0 && prim_type < PRIM_COUNT && prim_type != PRIM_4_CONTROL_POINTS);
}

bool d3d::draw_base(int prim_type, int start_vertex, int numprim, uint32_t num_inst, uint32_t start_instance)
{
  dirty_render_target();
  check_primtype(prim_type);
  return render.draw(prim_type, start_vertex, nprim_to_nverts(prim_type, numprim), num_inst, start_instance);
}

bool d3d::drawind_base(int prim_type, int startind, int numprim, int base_vertex, uint32_t num_inst, uint32_t start_instance)
{
  dirty_render_target();
  check_primtype(prim_type);
  return render.drawIndexed(prim_type, startind, nprim_to_nverts(prim_type, numprim), base_vertex, num_inst, start_instance);
}

bool d3d::draw_indirect(int prim_type, Sbuffer* buffer, uint32_t offset)
{
  dirty_render_target();
  check_primtype(prim_type);
  return render.drawIndirect(prim_type, buffer, offset);
}

bool d3d::draw_indexed_indirect(int prim_type, Sbuffer* buffer, uint32_t offset)
{
  dirty_render_target();
  check_primtype(prim_type);
  return render.drawIndirectIndexed(prim_type, buffer, offset);
}

bool d3d::multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  bool res = true;
  for (int i = 0; i < draw_count; ++i, byte_offset+=stride_bytes)
    res &= d3d::draw_indirect(prim_type, args, byte_offset);
  return res;
}

bool d3d::multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  bool res = true;
  for (int i = 0; i < draw_count; ++i, byte_offset+=stride_bytes)
    res &= d3d::draw_indexed_indirect(prim_type, args, byte_offset);
  return res;
}

bool d3d::draw_up(int prim_type, int numprim, const void* ptr, int stride_bytes)
{
  dirty_render_target();
  check_primtype(prim_type);
  return render.drawUP(prim_type, nprim_to_nverts(prim_type, numprim), NULL, ptr, nprim_to_nverts(prim_type, numprim), stride_bytes);
}

/// draw indexed primitives from user pointer (rather slow)
bool d3d::drawind_up(int prim_type, int minvert, int numvert, int numprim, const uint16_t *ind,
                     const void* ptr, int stride_bytes)
{
  dirty_render_target();
  check_primtype(prim_type);
  return render.drawUP(prim_type, nprim_to_nverts(prim_type, numprim), ind, ptr, numvert, stride_bytes);
}
#define IMMEDIATE_CB_NAMESPACE namespace drv3d_metal

bool d3d::set_immediate_const(unsigned shader_stage, const uint32_t *data, unsigned num_words)
{
  render.setImmediateConst(shader_stage, data, num_words);

  return true;
}

namespace d3d
{

struct ResUpdateBuffer
{
  drv3d_metal::Texture *texture = nullptr;
  id<MTLBuffer> buffer = nullptr;
  uint32_t level = 0;
  uint32_t face = 0;
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t z = 0;
  uint32_t w = 0;
  uint32_t h = 0;
  uint32_t d = 0;
  size_t pitch = 0;
  size_t slicePitch = 0;
};

ResUpdateBuffer *allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip, unsigned dest_slice,
                                                            unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth)
{
  G_ASSERT_RETURN(dest_base_texture, nullptr);

  ResUpdateBuffer *rub = new ResUpdateBuffer;
  rub->texture = (drv3d_metal::Texture *)dest_base_texture;
  rub->level = dest_mip;
  rub->face = dest_slice;
  rub->x = offset_x;
  rub->y = offset_y;
  rub->z = offset_z;
  rub->w = width;
  rub->h = height;
  rub->d = depth;

  TextureInfo base_ti;
  dest_base_texture->getinfo(base_ti, 0);

  G_ASSERT_RETURN(dest_mip < base_ti.mipLevels, nullptr);
  G_ASSERT_RETURN(dest_slice < base_ti.a, nullptr);

  int row_pitch = 0, slice_pitch = 0;
  rub->texture->getStride(rub->texture->base_format, width, height, 0, row_pitch, slice_pitch);

  rub->pitch = row_pitch;
  rub->slicePitch = slice_pitch;

  String name;
  name.printf(0, "upload for %s %llu", rub->texture->getName(), render.frame);
  rub->buffer = render.createBuffer(slice_pitch, MTLResourceStorageModeShared, name);

  return rub;
}

ResUpdateBuffer *allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice)
{
  G_ASSERT_RETURN(dest_tex, nullptr);

  TextureInfo base_ti;
  dest_tex->getinfo(base_ti, dest_mip);

  ResUpdateBuffer *rub = new ResUpdateBuffer;
  rub->texture = (drv3d_metal::Texture *)dest_tex;
  rub->level = dest_mip;
  rub->face = dest_slice;
  rub->x = 0;
  rub->y = 0;
  rub->z = 0;
  rub->w = base_ti.w;
  rub->h = base_ti.h;
  rub->d = base_ti.d;

  G_ASSERT_RETURN(dest_mip < base_ti.mipLevels, nullptr);
  G_ASSERT_RETURN(dest_slice < base_ti.a, nullptr);

  int row_pitch = 0, slice_pitch = 0;
  rub->texture->getStride(rub->texture->base_format, rub->w, rub->h, 0, row_pitch, slice_pitch);

  int d = rub->texture->type == D3DResourceType::VOLTEX ? rub->d : 1;

  rub->pitch = row_pitch;
  rub->slicePitch = slice_pitch;

  String name;
  name.printf(0, "upload for %s %llu", rub->texture->getName(), render.frame);
  rub->buffer = render.createBuffer(slice_pitch * d, MTLResourceStorageModeShared, name);

  return rub;
}

void release_update_buffer(ResUpdateBuffer *&rub)
{
  if (rub == nullptr)
    return;

  render.queueResourceForDeletion(rub->buffer);

  delete rub;
  rub = nullptr;
}

char *get_update_buffer_addr_for_write(ResUpdateBuffer *rub)
{
  return rub && rub->buffer ? (char *)rub->buffer.contents : nullptr;
}

size_t get_update_buffer_size(ResUpdateBuffer *rub)
{
  return rub && rub->buffer ? rub->slicePitch : 0;
}

size_t get_update_buffer_pitch(ResUpdateBuffer *rub)
{
  return rub ? rub->pitch : 0;
}

size_t get_update_buffer_slice_pitch(ResUpdateBuffer *rub)
{
  return rub ? rub->slicePitch : 0;
}

bool update_texture_and_release_update_buffer(ResUpdateBuffer *&src_rub)
{
  if (src_rub == nullptr)
    return false;

  render.uploadTexture(src_rub->texture, src_rub->texture->apiTex->texture, src_rub->level, src_rub->face, src_rub->x, src_rub->y, src_rub->z, src_rub->w, src_rub->h, src_rub->d, src_rub->buffer, src_rub->pitch, src_rub->slicePitch);

  d3d::release_update_buffer(src_rub);
  return true;
}

}
