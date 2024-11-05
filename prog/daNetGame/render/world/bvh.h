// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class Point3;

void initBVH();
void closeBVH();
void prepareFXForBVH(const Point3 &cameraPos);
bool is_bvh_enabled();
void draw_rtr_validation();