//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_carray.h>

#include <math/dag_Point2.h>
#include <math/dag_interpolator.h>

class DataBlock;

namespace gamephys
{

// polar math

struct Polares
{
  // variable
  float cl0;
  float cd0, indCoeff;
  float clLineCoeff;
  float cyCritH, cyCritL;
  float aoaCritH, aoaCritL;
  float aoaLineH, aoaLineL;
  float parabCyCoeffH, parabCyCoeffL;
  float aerCenterOffset;
  Point2 clToCm;

  // constant
  float parabAngle;
  float declineCoeff;
  float maxDistAng;
  float cdAfterCoeff;
  float clAfterCritL, clAfterCritH;

  // mach multipliers
  float kq;
  float clKq;

  // multipliers
  float cyMult;
};

// direct calculations

// calc Cx and Cy for angle of attack (AOA)
Point2 calc_c(const Polares &polares, float aoa, float ang, float cl_add = 0.0f, float cd_coeff = 1.f);

// calc Cl for angle of attack (AOA)
float calc_cl(const Polares &polares, float aoa);

// calc Cd for angle of attack (AOA)
float calc_cd(const Polares &polares, float aoa);

// calc Cs for sliding angle (AOS)
float calc_cs(const Polares &polares, float aos);

// performance & reverse calculations

bool calc_aoa(const Polares &polares, float cy, float &out_aoa);

bool calc_aoa(const Polares &polares, float cy, float ang, bool convert_aoa, float &out_aoa);

// props

struct PolaresProps
{
  static constexpr int MAX_CL_TO_CM = 8;
  // normal polar
  float lambda;
  float clLineCoeff;

  // after crit polar
  float parabAngle;
  float declineCoeff;
  float maxDistAng;
  float cdAfterCoeff;
  float clAfterCritL;
  float clAfterCritH;

  // no flaps polar
  float cl0;
  float aoaCritH;
  float aoaCritL;
  float clCritH;
  float clCritL;
  float cd0;

  // others
  float span;
  float area;

  // mach factor
  enum MachFactorMode
  {
    MACH_FACTOR_MODE_NONE,
    MACH_FACTOR_MODE_SIMPLE,
    MACH_FACTOR_MODE_SIMPLE_PROPELLER,
    MACH_FACTOR_MODE_ADVANCED
  } machFactorMode;
  enum
  {
    MACH_FACTOR_CURVE_CX0,
    MACH_FACTOR_CURVE_AUX,
    MACH_FACTOR_CURVE_CL_LINE_COEFF = MACH_FACTOR_CURVE_AUX,
    MACH_FACTOR_CURVE_CY_MAX,
    MACH_FACTOR_CURVE_AOA_CRIT,
    MACH_FACTOR_CURVE_IND_COEFF,
    MACH_FACTOR_CURVE_AER_CENTER_OFFSET,
    MACH_FACTOR_CURVE_CY0,
    MACH_FACTOR_CURVE_MAX
  };
  struct MachFactor
  {
    float machCrit;
    float machMax;
    float multMachMax;
    float multLineCoeff;
    float multLimit;
    carray<float, 4> machCoeffs;
  };
  carray<MachFactor, MACH_FACTOR_CURVE_MAX> machFactor;
  bool combinedCl;
  InterpolateFixedTab<Point2, MAX_CL_TO_CM> clToCmByMach;
};

void reset(PolaresProps &props);

extern PolaresProps defaultPolarProps;

void load_save_override_blk(gamephys::PolaresProps &props, DataBlock *blk1, DataBlock *blk2, float span, float area, float cl_to_cm,
  bool load);

inline void load_save_override_blk(gamephys::PolaresProps &props, DataBlock *blk, float span, float area, float cl_to_cm, bool load)
{
  load_save_override_blk(props, blk, blk, span, area, cl_to_cm, load);
}

int apply_modifications_1(gamephys::PolaresProps &props, const DataBlock &modBlk, const char *suffix, float mod_effect,
  float whole_mod_effect, float aspect_ratio);

int apply_modifications_2_no_flaps(gamephys::PolaresProps &props, const DataBlock &modBlk, const char *suffix, float mod_effect,
  float whole_mod_effect, float aspect_ratio);

inline int apply_modifications(gamephys::PolaresProps &props, const DataBlock &modBlk, const char *suffix, float mod_effect,
  float whole_mod_effect, float aspect_ratio)
{
  return apply_modifications_1(props, modBlk, suffix, mod_effect, whole_mod_effect, aspect_ratio) +
         apply_modifications_2_no_flaps(props, modBlk, suffix, mod_effect, whole_mod_effect, aspect_ratio);
}

void interpolate(const gamephys::PolaresProps &a, const gamephys::PolaresProps &b, float k, gamephys::PolaresProps &out);

void recalc_mach_factor(PolaresProps &props);

// props utils

float calc_mach_mult_derrivative(const PolaresProps &props, float mach, unsigned int index);

// props -> polares

void fill_polares(const gamephys::PolaresProps &props, gamephys::Polares &out_polares);

void calc_polares(const gamephys::PolaresProps &props, float mach, gamephys::Polares &out_polares);

void calc_polares(const gamephys::PolaresProps &props, float cy_mult, float mach, gamephys::Polares &out_polares);

} // namespace gamephys
