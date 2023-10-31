//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <math/integer/dag_IPoint4.h>

class Sbuffer;

enum LandClassType
{
  LC_SIMPLE,
  LC_DETAILED,
  LC_DETAILED_R,
  LC_MEGA_NO_NORMAL,
  LC_MEGA_DETAILED,
  LC_TRIVIAL,
  LC_COUNT,
  LC_CUSTOM = LC_COUNT
}; // LC_MEGA,
struct LandClassDetailTextures
{
  enum
  {
    ALBEDO,
    NORMAL,
    REFLECTANCE,
    NUM_TEXTURES_ST = 3
  };

  char name[128];
  int editorId = -1;
  Point2 offset;
  float tile;

  LandClassType lcType;
  TEXTUREID colormapId;
  TEXTUREID grassMaskTexId = BAD_TEXTUREID; // to be removed

  Tab<TEXTUREID> lcTextures;
  carray<Tab<int16_t>, NUM_TEXTURES_ST> lcDetailTextures;
  Tab<Point4> lcDetailParamsVS; // just CB
  Tab<Point4> lcDetailParamsPS; // just CB
  Tab<Point4> lcDetails;        // just CB
  Tab<const char *> lcDetailGroupNames;
  Tab<int> lcDetailGroupIndices;

  TEXTUREID flowmapTex;

  Point4 displacementMin;
  Point4 displacementMax;
  Point4 bumpScales;
  Point4 compatibilityDiffuseScales;
  Point4 randomFlowmapParams;
  Point4 flowmapMask;
  Point4 waterDecalBumpScale;
  // todo: we can just use dfferent Constant Buffers
  char shader_name[64] = {0};

  IPoint4 physmatIds;

  enum
  {
    DETAIL_RED = 0,
    DETAIL_GREEN = 1,
    DETAIL_BLUE = 2,
    DETAIL_BLACK = 3,
    // DETAIL_BLACK = 4,
    MAX_DETAILS = 4
  };


  void reset()
  {
    name[0] = 0;
    name[123] = 0;
    shader_name[0] = 0;
    editorId = -1;
    lcType = LC_SIMPLE;
    offset = Point2(0, 0);
    tile = 1;
    grassMaskTexId = colormapId = BAD_TEXTUREID; // normalMapId =
    detailsCB = 0;
    clear_and_shrink(lcDetailParamsVS);
    clear_and_shrink(lcDetailParamsPS);
    clear_and_shrink(lcDetails);
    for (int i = 0; i < lcDetailTextures.size(); i++)
      clear_and_shrink(lcDetailTextures[i]);
    flowmapTex = BAD_TEXTUREID;
  }
  Sbuffer *detailsCB = 0;

  void resetGrassMask(const DataBlock &grassBlk, const char *color_name, const char *info_grass_mask_name);
  void close();
  LandClassDetailTextures() { reset(); }
};
