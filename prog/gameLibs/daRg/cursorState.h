// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_mathBase.h>
#include <gui/dag_stdGuiRender.h>


namespace darg
{

// Move pos one step toward target at a screen-size-scaled speed.
// Returns true if pos changed (false when already within 1px of target).
inline bool smooth_cursor_step(Point2 &pos, Point2 target, float dt)
{
  if (lengthSq(pos - target) < 1.0f)
    return false;

  const float sh = StdGuiRender::screen_height();
  const float sw = StdGuiRender::screen_width();

  Point2 delta = target - pos;
  float dist = length(delta);

  float scale = sqrtf(sh * sh + sw * sw) / sqrtf(1080.f * 1080.f + 1920.f * 1920.f);
  float speed = ::clamp(dist / 0.1f, 8000.f * scale, 20000.f * scale);

  float maxD = dt * speed;
  if (dist > maxD)
    delta *= maxD / dist;
  Point2 dest = pos + delta;
  if (lengthSq(dest - target) < 1.0f)
    dest = target;
  pos = dest;
  return true;
}


// Shared position state for mouse pointer and gamepad cursor.
// Owns pos/targetPos and the deferred-jump mechanism (nextFramePos).
// Each device subclass provides its own moveToTarget() for smooth animation.
struct CursorPosState
{
  Point2 pos = Point2(0, 0);
  Point2 targetPos = Point2(0, 0);

  inline void requestNextFramePos(const Point2 &p)
  {
    nextFramePos = p;
    needToMoveOnNextFrame = true;
  }

  inline void requestTargetPos(const Point2 &p)
  {
    needToMoveOnNextFrame = false;
    targetPos = p;
  }

  inline void applyNextFramePos()
  {
    if (needToMoveOnNextFrame)
    {
      pos = nextFramePos;
      targetPos = pos;
      needToMoveOnNextFrame = false;
    }
  }

  bool hasNextFramePos() const { return needToMoveOnNextFrame; }

  // Copy position from another pointer (for device switching).
  inline void syncPosFrom(const CursorPosState &other)
  {
    pos = other.pos;
    targetPos = pos;
    needToMoveOnNextFrame = false;
  }

  // Debug visualization: render nextFramePos marker and targetPos box in the given colors.
  inline void debugRenderState(StdGuiRender::GuiContext *ctx, E3DCOLOR frame_color, E3DCOLOR target_color) const
  {
    if (needToMoveOnNextFrame)
    {
      ctx->set_color(frame_color);
      ctx->render_frame(nextFramePos.x - 5, nextFramePos.y - 5, nextFramePos.x + 5, nextFramePos.y + 5, 2);
      ctx->render_frame(nextFramePos.x - 9, nextFramePos.y - 9, nextFramePos.x + 9, nextFramePos.y + 9, 2);
      ctx->render_frame(nextFramePos.x - 13, nextFramePos.y - 13, nextFramePos.x + 13, nextFramePos.y + 13, 2);
    }
    ctx->set_color(target_color);
    ctx->render_box(targetPos.x - 4, targetPos.y - 4, targetPos.x + 4, targetPos.y + 4);
  }

protected:
  Point2 nextFramePos = Point2(0, 0);
  bool needToMoveOnNextFrame = false;
};

} // namespace darg
