//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace darg
{

class Transform
{
public:
  Transform() :
    pivot(0.5f, 0.5f),
    translate(0, 0),
    scale(1.0f, 1.0f),
    rotate(0),
    curTranslate(0, 0),
    curScale(1.0f, 1.0f),
    curRotate(0),
    haveCurTranslate(false),
    haveCurScale(false),
    haveCurRotate(false)
  {}

  Point2 getCurTranslate() const { return haveCurTranslate ? curTranslate : translate; }
  Point2 getCurScale() const { return haveCurScale ? curScale : scale; }
  float getCurRotate() const { return haveCurRotate ? curRotate : rotate; }

public:
  Point2 pivot;

  Point2 translate;
  Point2 scale;
  float rotate;

  bool haveCurTranslate, haveCurScale, haveCurRotate;

  Point2 curTranslate;
  Point2 curScale;
  float curRotate;
};

} // namespace darg
