//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_color.h>


TMatrix make_brightness_color_matrix(const Color3 &scale);
TMatrix make_contrast_color_matrix(const Color3 &scale, const Color3 &pivot);
TMatrix make_saturation_color_matrix(const Color3 &scale, const Color3 &gray_color);
TMatrix make_hue_shift_color_matrix(real degrees);

TMatrix make_blend_to_color_matrix(const Color3 &to_color, real blend_factor);
