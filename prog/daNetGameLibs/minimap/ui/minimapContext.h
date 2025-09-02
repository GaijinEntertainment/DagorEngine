// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/event.h>
#include <math/dag_TMatrix.h>
#include <EASTL/vector_set.h>

#include <sqrat.h>
#include <gui/dag_stdGuiRender.h>
#include <3d/dag_picMgr.h>

#include "ecs/game/generic/team.h"
#include "render/renderEvent.h"

class MinimapContext
{
public:
  MinimapContext();
  ~MinimapContext();

  const TMatrix &getCurViewItm() const { return curViewItm; }

  void setup(Sqrat::Object cfg);
  float limitVisibleRadius(float radius);
  float maxVisibleRadius();
  Point3 clampPan(const Point3 &pan, float vis_radius);

  static void clamp_square(float &x, float &y, float r);

  static const float def_visible_radius;

public:
  TMatrix curViewItm = TMatrix::IDENT;

  TEXTUREID mapTexId, backTexId;
  d3d::SamplerHandle mapSampler, backSampler;
  Point2 worldLeftTop;
  Point2 worldRightBottom;
  Point2 backWorldLeftTop;
  Point2 backWorldRightBottom;
  float northAngle;

  TEXTUREID maskTexId;
  TEXTUREID blendMaskTexId;
  d3d::SamplerHandle maskSampler, blendMaskSampler;

  E3DCOLOR colorMap;
  E3DCOLOR colorFov;

  PictureManager::PicDesc picTick;
  PictureManager::PicDesc picN, picE, picS, picW;

  bool isPersp = false;
  float perspWk = 0.0f;
};

extern void minimap_on_render_ui(const RenderEventUI &evt);
