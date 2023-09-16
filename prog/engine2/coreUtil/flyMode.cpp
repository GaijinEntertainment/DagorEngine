#include <util/dag_flyMode.h>
#include <math/dag_math3d.h>

class FlyModeBlackBoxImp : public FlyModeBlackBox
{
public:
  FlyModeBlackBoxImp()
  {
    resetSettings();
    resetDynamic();
  }

  virtual void resetSettings()
  {
    angSpdScaleH = 0.01f;
    angSpdScaleV = 0.01f;
    angSpdBank = 0.01f;
    moveSpd = 4.0f;
    turboSpd = 50.0f;
    moveInertia = .95f;
    stopInertia = .1f;
    angStopInertiaH = .15f;
    angStopInertiaV = .15f;
    angInertiaH = .5f, angInertiaV = .5f;
  }

  virtual void resetDynamic()
  {
    itm.identity();
    tmValid = false;

    linVel = Point3(0, 0, 0);
    ang = Point3(0, 0, 0);
    angVel = Point3(0, 0, 0);
    memset(&keys, 0, sizeof(keys));
  }

  virtual void integrate(real dt, real and_dx, real ang_dy)
  {
    real hspeed = -and_dx * angSpdScaleH;
    real vspeed = -ang_dy * angSpdScaleV;
    real hin = (fabsf(angVel.x) > fabsf(hspeed) || angVel.x * hspeed < 0) ? angStopInertiaH : angInertiaH;
    real vin = (fabsf(angVel.y) > fabsf(vspeed) || angVel.y * vspeed < 0) ? angStopInertiaV : angInertiaV;

    angVel.x = angVel.x * hin + hspeed * (1 - hin);
    angVel.y = angVel.y * vin + vspeed * (1 - vin);

    if (keys.bankLeft)
      angVel.z = -angSpdBank;
    else if (keys.bankRight)
      angVel.z = angSpdBank;
    else
      angVel.z = 0.f;

    if (fabsf(angVel.x) > 1.0e-6 || fabsf(angVel.y) > 1.0e-6 || fabsf(angVel.z) > 1.0e-6)
    {
      ang.x += angVel.x;
      ang.y += angVel.y;
      ang.z += angVel.z;
      if (ang.y < -HALFPI)
        ang.y = -HALFPI;
      else if (ang.y > HALFPI)
        ang.y = HALFPI;

      const Point3 p = itm.getcol(3);
      itm = rotyTM(ang.x) * rotxTM(ang.y) * rotzTM(ang.z);
      itm.setcol(3, p);

      tmValid = false;
    }

    real v = moveSpd;
    Point3 mov(0, 0, 0);

    if (keys.turbo)
      v = turboSpd;
    if (keys.fwd)
      mov += itm.getcol(2);
    if (keys.back)
      mov -= itm.getcol(2);
    if (keys.left)
      mov -= itm.getcol(0);
    if (keys.right)
      mov += itm.getcol(0);
    if (keys.cameraUp)
      mov += itm.getcol(1);
    if (keys.cameraDown)
      mov -= itm.getcol(1);

    if (keys.worldUp)
      mov += Point3(0, 1, 0);
    if (keys.worldDown)
      mov -= Point3(0, 1, 0);

    real in = (mov == Point3(0, 0, 0) ? stopInertia : moveInertia);
    linVel = linVel * in + mov * (v * (1 - in));
    if (lengthSq(linVel) > 1.0E-8)
    {
      itm.setcol(3, itm.getcol(3) + linVel * dt);
      tmValid = false;
    }
  }

  virtual void getViewMatrix(TMatrix &out_tm)
  {
    if (!tmValid)
    {
      tm = inverse(itm);
      tmValid = true;
    }
    out_tm = tm;
  }
  virtual void getInvViewMatrix(TMatrix &out_itm) { out_itm = itm; }

  virtual void setViewMatrix(const TMatrix &in_itm)
  {
    itm = in_itm;
    tm = inverse(itm);
    tmValid = true;
    Point3 dir = itm.getcol(2);
    ang.x = -safe_atan2(dir.x, dir.z);
    ang.y = safe_asin(dir.y);
    ang.z = 0;
  }

protected:
  TMatrix itm, tm;
  Point3 linVel;
  Point3 ang, angVel;
  bool tmValid;
};


FlyModeBlackBox *create_flymode_bb() { return new (midmem) FlyModeBlackBoxImp; }
