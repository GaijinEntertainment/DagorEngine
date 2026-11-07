// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bvh_context.h"

// Shared bvh_particle_data: fx slots [0, fxCapacity), smoke-tracer slots [fxCapacity,
// fxCapacity + tracerCapacity). ensure_capacity must run once per frame before writers;
// writers must check is_ready() (false if the buffer could not grow that frame). The
// alignments are exposed so each subsystem's HW-instance buffer can match its data region,
// keeping the TLAS instance index aligned with bvh_particle_data slot.
namespace bvh::particles
{
static constexpr int FX_CAPACITY_ALIGN = 256;
static constexpr int TRACER_CAPACITY_ALIGN = 64;

void ensure_capacity(int fx_max, int smoke_tracer_max);
bool is_ready();
Sbuffer *get_data_buf();
int get_fx_capacity();
int get_smoke_tracer_capacity();
int64_t get_size_bytes();

struct BillboardVertex
{
  Point3 position;
  Point2 texcoord;
};

UniqueBLAS create_billboard_blas(const BillboardVertex (&vertices)[8], UniqueBuf &vb, UniqueBuf &ib, const char *vb_name,
  const char *ib_name);
UniqueBLAS build_billboard_blas(Sbuffer *vb, Sbuffer *ib, uint32_t quad_count);
// meta must be fresh from allocateMetaRegion
void fill_billboard_meta(MeshMeta &meta, uint32_t material_type, uint32_t bindless_indices_index, uint32_t bindless_vertices_index);
} // namespace bvh::particles
