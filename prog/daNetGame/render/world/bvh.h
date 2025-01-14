// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class Point3;

void initBVH();
void closeBVH();
void prepareFXForBVH(const Point3 &cameraPos);
bool is_bvh_enabled();
bool is_rtsm_enabled();
bool is_rtr_enabled();
bool is_rtao_enabled();
bool is_rt_water_enabled();
bool is_denoiser_enabled();
void draw_rtr_validation();
void bvh_cables_changed();