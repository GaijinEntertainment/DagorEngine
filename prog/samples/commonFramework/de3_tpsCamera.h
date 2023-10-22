#pragma once

/** \addtogroup de3Common
  @{
*/

#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include "de3_rayTracer.h"
#include "de3_ICamera.h"

/**
Callback interface for third person camera targets.
*/

class ITpsCameraTarget
{
public:
  /**
  \brief Retrieves target position.

  \return #Point3

  @see Point3 getDirection() getTm()
  */
  virtual Point3 getPos() const = 0;

  /**
  \brief Retrieves target direction.

  \return #Point3

  @see Point3 getPos() getTm()
  */
  virtual Point3 getDirection() const = 0;

  /**
  \brief Retrieves target matrix.

  \param[out] tm matrix to save target matrix to.

  @see TMatrix getDirection() getPos()
  */
  virtual void getTm(TMatrix &tm) const = 0;
};


/**
\brief Third person camera class.
*/

class TpsCamera : public IGameCamera
{
public:
  /**
  \brief Constructor
  */
  TpsCamera(float zn = 0.1f, float zf = 2000.0f);

  /**
  \brief Constructor

  \param[in] newTarget to look at.

  @see ITpsCameraTarget
  */
  TpsCamera(ITpsCameraTarget *newTarget) { target = newTarget; }

  /**
  \brief Destructor
  */
  virtual ~TpsCamera() {}

  /**
  \brief Sets a target to TPS camera.

  \param[in] newTarget new target to look at.

  @see ITpsCameraTarget
  */
  void setTarget(ITpsCameraTarget *newTarget) { target = newTarget; }

  /**
  \brief Sets a minimal distance between camera and target.

  \param[in] dist distance.

  */
  void setMinTargetDistance(real dist) { minDistance = dist; }

  /**
  \brief Locks camera. When locked, camera's Tm is hard linked with target Tm.

  \param[in] lckd locked.

  */
  void setLocked(bool lckd) { locked = lckd; }

  /**
  \brief Sets a ray tracer to TPS camera.

  \param[in] raytracer ray tracer to trace.

  @see IRayTracer
  */
  void setRayTracer(IRayTracer *raytracer) { tracer = raytracer; }

  virtual void setView()
  {
    if (!locked)
    {
      Point3 v, u, vv = normalize(targetPos - pos);
      u = up % vv;

      u.normalize();

      v = vv % u;
      v.normalize();

      TMatrix t;
      t.setcol(0, u.x, v.x, vv.x);
      t.setcol(1, u.y, v.y, vv.y);
      t.setcol(2, u.z, v.z, vv.z);
      t.setcol(3, -pos * u, -pos * v, -pos * vv);


      itm = inverse(t);
    }

    ::grs_cur_view.itm = itm;

    tm = inverse(::grs_cur_view.itm);
    ::grs_cur_view.tm = tm;
    ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);

    d3d::settm(TM_VIEW, tm);
    int w, h;
    d3d::get_target_size(w, h);
    d3d::setpersp(Driver3dPerspective(1.3, 1.3 * w / h, zNear, zFar, 0, 0));
    d3d::setview(0, 0, w, h, 0, 1);
  }

  /**
  \brief If invert, inverts camera direction.

  \param[in] invert inverted direction flag.

  */
  virtual void invertDir(bool invert) { invert_dir = invert; }

  virtual void act();

  virtual void getInvViewMatrix(TMatrix &out_itm) const { out_itm = itm; }

  /**
  \brief Retrieves view matrix.

  See #setViewMatrix().

  \param[out] out_tm matrix to save view matrix to.

  @see TMatrix setViewMatrix() getInvViewMatrix() setInvViewMatrix()
  */
  virtual void getViewMatrix(TMatrix &out_tm)
  {
    tm = inverse(itm);
    out_tm = tm;
  }

  /**
  \brief Sets view matrix.

  See #getViewMatrix().

  \param[in] in_tm view matrix.

  @see TMatrix getViewMatrix() setInvViewMatrix() getInvViewMatrix()
  */
  virtual void setViewMatrix(const TMatrix &in_tm)
  {
    tm = in_tm;
    itm = inverse(tm);
    pos = itm.getcol(3);
    vel = Point3(0, 0, 0);
  }

  virtual void setInvViewMatrix(const TMatrix &in_itm)
  {
    itm = in_itm;
    tm = inverse(itm);
    pos = itm.getcol(3);
    vel = Point3(0, 0, 0);
  }

  /**
  \brief Sets camera origin offset.

  \param[in] x offset value.
  \param[in] y offset value.
  \param[in] z offset value.

  */
  inline void setOriginShift(float x, float y, float z) { origin = Point3(x, y, z); }

  inline void setDistOverTarget(real h) { distOverTarget = h; }

  int setCamDiff(int const &diff);

  inline int getCamDiff() { return camDiff; }

protected:
  TMatrix itm = TMatrix::IDENT, tm = TMatrix::IDENT;
  Point3 vel = Point3(0, 0, 0);

  Point3 pos = Point3(0, 0, 0), targetPos = Point3(0, 0, 0), up = Point3(0, 1, 0), viewVector = Point3(1, 0, 0),
         origin = Point3(0, 0, 0), right = Point3(0, 0, 1);
  real minDistance = 1.0f;
  real distOverTarget = 0.0f;
  float zNear = 0, zFar = 0;
  bool locked = false, invert_dir = false;
  ITpsCameraTarget *target;
  IRayTracer *tracer = nullptr;

  int camDiff = 0;
};
/** @} */
