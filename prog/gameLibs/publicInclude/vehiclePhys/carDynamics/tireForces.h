//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>

class DataBlock;

class TireForceModel2
{
public:
  TireForceModel2();

  void load(const DataBlock *blk, bool left_wheel);

  // Attribs
  void setCamber(float _camber) { gamma = _camber; }
  void setSlipAngle(float sa) { a_s = -tanf(sa); }
  void setSlipAstar(float tana_mul_sgn_vcx) { a_s = tana_mul_sgn_vcx; }
  void setSlipRatio(float sr) { k = sr; }
  void setNormalForce(float force) { Fz = force; }
  void setFriction(float mux_s, float muy_s, float muv)
  {
    mxs = mux_s;
    mys = muy_s;
    laMuV = muv;
  }
  void setContactVel(float vs, float abs_vcx, float sgn)
  {
    Vs = vs;
    absVcx = abs_vcx;
    sgnVcx = sgn;
  }
  void setCosAquote(float cosa_q) { cosAq = cosa_q; }
  void setPhy(float phy_t, float phy_full)
  {
    phyT = phy_t;
    phy = phy_full;
  }

  // Physics
  void computeForces(bool calc_mz = false);

public:
  float getFx() { return Fx; }
  float getFy() { return Fy; }
  float getMz() { return Mz; }
  float getFz() { return Fz; }
  float getCamber() { return gamma; }
  float getSlipAstar() const { return a_s; }

  // Extras
  float getMaxLongForce() const { return 100000; }
  float getMaxLatForce() const { return 100000; }
  float getGxa() const { return Gxa * Fz; }
  float getGyk() const { return Gyk * Fz; }

private:
  // user scaling factors
  float laFz0, laMux, laMuy, laMuV, laKxk, laKya, laCx, laEx, laHx, laVx, laCy, laEy, laHy, laVy, laKyg, laKzg, lat, laMr;

  float laxa, layk, laVyk, las;

  float laCz, laMx, laMy;

  float rEx1, rEx2, rEy1, rEy2;

  // Input parameters
  float gamma, // Angle between wheel-up and ground normal (rad)
    a_s,       // Slip angle (rad)
    k,         // Slip ratio (-1=full lock, 0=no slip)
    Fz,        // Normal force (N)
    Vs,        // magnitude of slip velocity (for wet roads)
    absVcx,    // |Vcx|
    sgnVcx,    // sgn(Vcx)
    cosAq,     // Vcx/(Vc+0.1)
    phyT,      // -psi./(Vc+0.1)
    phy,       // -(psi. - (1-eps)*w*sin(gamma))/(Vc+0.1),  eps=0.7+
    mxs, mys;  // friction scale for contact point material

  // Output
  float Fx, Fy, Mz; // Longitudinal/lateral/aligning moment
  float Gxa, Gyk;
};
