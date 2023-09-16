//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <ioSys/dag_baseIo.h>
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include <math/dag_frustum.h>
#include <memory/dag_framemem.h>
#include <scene/dag_tiledScene.h>


class dtPathCorridor;

namespace covers
{
struct Cover
{
  mat44f calcTm() const;

  Point3 dir;
  Point3 groundLeft, groundRight;
  float hLeft, hRight;

  Point3 shootLeft;
  Point3 shootRight;

  bool hasLeftPos, hasRightPos;

  BBox3 visibleBox;
};


bool load(IGenLoad &crd, Tab<Cover> &covers_out);
bool build(float max_tile_size, uint32_t split, const Tab<Cover> &covers, scene::TiledScene &scene_out);

void draw_cover(const Cover &cover, uint8_t alpha);
void draw_debug(const Tab<Cover> &covers);
void draw_debug(const scene::TiledScene &scene);
void draw_debug(const scene::TiledScene &scene, mat44f_cref globtm);
void draw_debug(const Tab<Cover> &covers, const scene::TiledScene &scene, mat44f_cref globtm, Point3 *viewPos = nullptr,
  float distSq = 6400, uint8_t alpha = 255);
}; // namespace covers
