//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

// global render states
extern bool grs_draw_wire;

// compute and set gamma correction
void set_gamma(float p);
void set_gamma_shadervar(float p);

// returns current gamma
float get_current_gamma();

#include <supp/dag_undef_COREIMP.h>
