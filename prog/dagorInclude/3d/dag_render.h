//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

// global render states
extern bool grs_draw_wire;

// compute and set gamma correction
void set_gamma_shadervar(float p);

// returns current gamma
float get_current_gamma();

#include <supp/dag_undef_KRNLIMP.h>
