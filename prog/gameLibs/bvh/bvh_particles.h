// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

class Sbuffer;

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
} // namespace bvh::particles
