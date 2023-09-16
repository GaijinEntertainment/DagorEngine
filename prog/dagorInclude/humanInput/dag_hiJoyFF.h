//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <humanInput/dag_hiDecl.h>

namespace HumanInput
{
// common effect creation structures and types
enum
{
  JFF_COORD_TYPE_Polar = 1,
  JFF_COORD_TYPE_Sperical,
  JFF_COORD_TYPE_Cartesian,
};

struct JoyFfCreateParams
{
  // coordinate system settings
  int coordType;
  int axisNum;
  int axisId[3];
  float dir[3];
  // duration in seconds
  float dur;
};


// effect subtypes
enum
{
  // Condition force feedback effect
  JFF_COND_Friction = 1,
  JFF_COND_Damper,
  JFF_COND_Inertia,
  JFF_COND_Spring,

  // Constant force feedback effect (no subtypes)

  // Periodic force feedback effect
  JFF_PERIOD_Square = 1,
  JFF_PERIOD_Sine,
  JFF_PERIOD_Triangle,
  JFF_PERIOD_SawtoothUp,
  JFF_PERIOD_SawtoothDown,

  // Ramp force feedback effect (no subtypes)
};


// generic effect interface
class IGenJoyFfEffect
{
public:
  virtual void destroy() = 0;

  virtual void download() = 0;
  virtual bool start(int count, bool solo) = 0;
  virtual bool isPlaying() = 0;
  virtual void stop() = 0;

  virtual void apply() = 0;

  virtual void restore() = 0;

  // direction setup
  virtual void setDirPolar(float ang) = 0;
  virtual void setDirSpherical(float ang_x, float ang_y = 0, float ang_z = 0) = 0;
  virtual void setDirCartesian(float x, float y = 0, float z = 0) = 0;

  // start variants
  inline bool startSingle(bool solo = false) { return start(1, solo); }
  inline bool startContinuous(bool solo = false) { return start(0, solo); }
};


// Condition force feedback effect
class IJoyFfCondition : public IGenJoyFfEffect
{
public:
  virtual int getType() = 0;

  // generic parameters setup
  virtual void setOffsetEx(int axis_idx, float p) = 0;
  virtual void setCoefEx(int axis_idx, bool neg, float p) = 0;
  virtual void setSaturationEx(int axis_idx, bool neg, float maxp) = 0;
  virtual void setDeadzoneEx(int axis_idx, float dp) = 0;

  inline void setOffset(float p) { setOffsetEx(0, p); }
  inline void setCoef(bool neg, float p) { setCoefEx(0, neg, p); }
  inline void setSaturation(bool neg, float maxp) { setSaturationEx(0, neg, maxp); }
  inline void setDeadzone(float dp) { setDeadzoneEx(0, dp); }

  // aliases for concrete effect types
  inline void setSpringCenter(float p) { setOffset(p); }
  inline void setSpringCoefs(float neg_p, float pos_p)
  {
    setCoef(false, pos_p);
    setCoef(true, neg_p);
  }
  inline void setSpringSymmCoef(float p)
  {
    setCoef(false, p);
    setCoef(true, p);
  }
  inline void setSpringSaturation(float neg_maxp, float pos_maxp)
  {
    setSaturation(false, pos_maxp);
    setSaturation(true, neg_maxp);
  }

  inline void setSpringCenterEx(int axis_idx, float p) { setOffsetEx(axis_idx, p); }
  inline void setSpringCoefsEx(int axis_idx, float neg_p, float pos_p)
  {
    setCoefEx(axis_idx, false, pos_p);
    setCoefEx(axis_idx, true, neg_p);
  }
  inline void setSpringSymmCoefEx(int axis_idx, float p)
  {
    setCoefEx(axis_idx, false, p);
    setCoefEx(axis_idx, true, p);
  }
  inline void setSpringSaturationEx(int axis_idx, float neg_maxp, float pos_maxp)
  {
    setSaturationEx(axis_idx, false, pos_maxp);
    setSaturationEx(axis_idx, true, neg_maxp);
  }

  inline void setNoDamperMaxVel(float p) { setOffset(p); }
};


// Constant force feedback effect
class IJoyFfConstForce : public IGenJoyFfEffect
{
public:
  // generic parameters setup
  virtual void setForce(float p) = 0;
};


// Periodic force feedback effect
class IJoyFfPeriodic : public IGenJoyFfEffect
{
public:
  virtual int getType() = 0;

  // generic parameters setup
  virtual void setAmplitude(float dp) = 0;
  virtual void setBaseline(float p0) = 0;
  virtual void setPeriod(float t) = 0;
  virtual void setPhase(float ang) = 0;

  // aliases for concrete effect types
  inline void setFrequency(float v) { setPeriod(1 / v); }
};


// Ramp force (linear changing force) feedback effect
class IJoyFfRampForce : public IGenJoyFfEffect
{
public:
  // generic parameters setup
  virtual void setStartForce(float p) = 0;
  virtual void setEndForce(float p) = 0;

  // aliases for concrete effect types
  inline void setForces(float pstart, float pend)
  {
    setStartForce(pstart);
    setEndForce(pend);
  }
};
} // namespace HumanInput
