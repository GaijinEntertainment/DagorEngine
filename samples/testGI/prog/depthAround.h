// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// todo: cascades, toroidal
class DepthAround
{
public:
  template <class Cb>
  void render(const Point3 &pos, const Cb &cb)
  {
    if (!depth_above)
      return;
    const IPoint2 alignedPos = ipoint2(floor(Point2::xz(pos) / texelSize + Point2(0.5, 0.5)));
    if (abs(alignedPos.x - origin.x) <= 1 && abs(alignedPos.y - origin.y) <= 1)
      return;
    origin = alignedPos;

    DA_PROFILE_GPU;
    const Point2 lt = point2(origin) * texelSize - Point2(w / 2, w / 2) * texelSize;
    const float d = w * texelSize;
    ShaderGlobal::set_color4(get_shader_variable_id("world_to_depth_above", true), 1. / d, -lt.x / d, -lt.y / d, d);
    SCOPE_VIEW_PROJ_MATRIX;
    SCOPE_RENDER_TARGET;
    d3d::set_render_target();
    d3d::set_render_target(nullptr, 0);
    d3d::set_depth(depth_above.getTex2D(), 0, DepthAccess::RW);
    d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
    TMatrix4 view;
    view.setcol(0, 1, 0, 0, 0);
    view.setcol(1, 0, 0, 1, 0);
    view.setcol(2, 0, 1, 0, 0);
    view.setcol(3, 0, 0, 0, 1);
    cb(view, BBox2(lt, lt + Point2(d, d)));
  }
  void init(int w_)
  {
    if (w == w_)
      return;
    w = w_;
    depth_above.close();
    depth_above = dag::create_tex(NULL, w, w, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "depth_above");
    texelSize = dist / w;
    invalidate();
  }
  void setDist(float dist_)
  {
    if (dist == dist_)
      return;
    dist = dist_;
    texelSize = dist / w;
    invalidate();
  }
  BBox2 getArea() const
  {
    const Point2 lt = point2(origin) * texelSize - Point2(w / 2, w / 2) * texelSize;
    return BBox2(lt, Point2(w * texelSize, w * texelSize) + lt);
  }
  void invalidate() { origin = IPoint2(-100000, 100000); }

protected:
  UniqueTexHolder depth_above;
  IPoint2 origin = {-100000, 100000};
  float texelSize = 1, dist = 1024;
  int w = 1;
};