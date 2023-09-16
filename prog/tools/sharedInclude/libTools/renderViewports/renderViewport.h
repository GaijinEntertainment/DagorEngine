#ifndef __DAGOR_TOOLS_RENDER_VIEWPORT_H__
#define __DAGOR_TOOLS_RENDER_VIEWPORT_H__


#include <math/dag_math3d.h>


class RenderViewport
{
protected:
  TMatrix viewMatrix;
  TMatrix4 projectionMatrix;
  Point2 lt, rb;

public:
  const TMatrix &getViewMatrix() const { return viewMatrix; }
  void setViewMatrix(const TMatrix &tm) { viewMatrix = tm; }
  void setProjectionMatrix(const TMatrix4 &tm) { projectionMatrix = tm; }
  const TMatrix4 &getProjectionMatrix() const { return projectionMatrix; }
  const Point2 &getlt() const { return lt; }
  const Point2 &getrb() const { return rb; }
  void setlt(const Point2 &lt1) { lt = lt1; }
  void setrb(const Point2 &rb1) { rb = rb1; }

  bool wireframe;


  RenderViewport();

  // sets VIEW and PROJ matrices; used internally
  // can be used externally when scene is rendered manually
  void setViewProjTms();

  void setView(real minz = 0, real maxz = 1);

  // setups d3d for each viewport before rendering
  void setViewAndTms(real minz = 0, real maxz = 1)
  {
    setView(minz, maxz);
    setViewProjTms();
  }

  static void resetView();


  void setPersp(real wk, real hk, real zn, real zf);

  // set perspective projection tm with specified horizontal FOV
  void setPerspHFov(real fov_angle_radians, real zn, real zf);

  // sets lt, rb to cover all screen
  void setFullScreenViewRect();
};


#endif
