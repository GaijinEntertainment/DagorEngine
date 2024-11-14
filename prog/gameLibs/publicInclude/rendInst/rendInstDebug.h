//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Point3;
class TMatrix;

namespace rendinst
{
struct RendInstDesc;

void draw_rendinst_info(const Point3 &intersection_pos, const TMatrix &cam_tm, const RendInstDesc &desc);
} // namespace rendinst
