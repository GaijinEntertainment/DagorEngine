//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

// Differential term
class Damper
{
public:
  Damper() : coeff(0.0f), tauInv(0.0f), prevVal(0.0f), rate(0.0f) {}
  inline void setParams(float coeff_in, float tau_in)
  {
    coeff = coeff_in;
    tauInv = safeinv(tau_in);
  }
  inline void getParams(float &out_coeff, float &out_tau) const
  {
    out_coeff = coeff;
    out_tau = safeinv(tauInv);
  }
  inline void setState(float prev_val, float value_in)
  {
    prevVal = prev_val;
    rate = safediv(value_in, coeff);
  }
  inline float simulate(float val, float rate_in, float dt)
  {
    rate += (rate_in - rate) * dt * tauInv;
    prevVal = val;
    return get();
  }
  inline float simulate(float val, float dt)
  {
    G_ASSERT(rabs(dt) > VERY_SMALL_NUMBER);
    const float step = val - prevVal;
    rate += (step - rate * dt) * tauInv;
    prevVal = val;
    return get();
  }
  inline void getState(float &out_prev, float &out_derr) const
  {
    out_prev = prevVal;
    out_derr = get();
  }
  inline float get() const { return rate * coeff; }

private:
  float coeff;
  float tauInv;
  float prevVal;
  float rate;
};

// Integral term
class Integrator
{
public:
  Integrator() : coeff(0.0f), limit(0.0f), result(0.0f) {}
  inline void setParams(float coeff_in, float lim_in)
  {
    coeff = coeff_in;
    limit = lim_in;
  }
  inline void getParams(float &out_coeff, float &out_lim) const
  {
    out_coeff = coeff;
    out_lim = limit;
  }
  inline void setState(float val) { result = val; }
  inline float simulate(float val, float dt)
  {
    G_ASSERT(rabs(dt) > VERY_SMALL_NUMBER);
    result = clamp(result + coeff * val * dt, -limit, limit);
    return result;
  }
  inline float get() const { return result; }

private:
  float coeff;
  float limit;
  float result;
};

// Proportional-integral-differential term
class PidController
{
public:
  PidController() : coeffProp(0.0f) { ; }
  inline void setParams(float coeff_prop_in, float coeff_integral_in, float intergrator_lim, float coeff_dumper_in, float dumper_tau)
  {
    coeffProp = coeff_prop_in;
    integrator.setParams(coeff_integral_in, intergrator_lim);
    damper.setParams(coeff_dumper_in, dumper_tau);
  }
  inline void getParams(float &out_coeff_prop, float &out_coeff_integral, float &out_intergrator_lim, float &out_coeff_dumper,
    float &out_dumper_tau)
  {
    out_coeff_prop = coeffProp;
    integrator.getParams(out_coeff_integral, out_intergrator_lim);
    damper.getParams(out_coeff_dumper, out_dumper_tau);
  }
  inline void setState(float integrator_val, float damper_prev, float dumper_val)
  {
    integrator.setState(integrator_val);
    damper.setState(damper_prev, dumper_val);
  }
  inline void getState(float &out_intg, float &out_prev_val, float &out_derr) const
  {
    out_intg = integrator.get();
    damper.getState(out_prev_val, out_derr);
  }
  inline float simulate(float val, float rate, float dt)
  {
    G_ASSERT(rabs(dt) > VERY_SMALL_NUMBER);
    const float p = coeffProp * val;
    const float i = integrator.simulate(val, dt);
    const float d = damper.simulate(val, rate, dt);
    return p + i + d;
  }
  inline float simulate(float val, float dt)
  {
    G_ASSERT(rabs(dt) > VERY_SMALL_NUMBER);
    const float p = coeffProp * val;
    const float i = integrator.simulate(val, dt);
    const float d = damper.simulate(val, dt);
    return p + i + d;
  }

private:
  float coeffProp;
  Integrator integrator;
  Damper damper;
};

// Value rate limiter
class RateLimit
{
public:
  RateLimit() : range(Point2(-VERY_BIG_NUMBER, VERY_BIG_NUMBER)), prevVal(0.0f) {}
  inline void setRange(const Point2 &range_in) { range = range_in; }
  inline const Point2 &getRange() const { return range; }
  inline void setState(float val) { prevVal = val; }
  inline float simulate(float val, float rate_in, float dt)
  {
    G_ASSERT(rabs(dt) > VERY_SMALL_NUMBER);
    prevVal = limitRate(val, rate_in, dt);
    return prevVal;
  }
  inline float simulate(float val, float dt)
  {
    G_ASSERT(rabs(dt) > VERY_SMALL_NUMBER);
    const float rate = (val - prevVal) / dt;
    prevVal = limitRate(val, rate, dt);
    return prevVal;
  }

private:
  float limitRate(float val, float rate, float dt) const
  {
    const float limitedRate = clamp(rate, range.x, range.y);
    const float result = (rabs(rate - limitedRate) > VERY_SMALL_NUMBER) ? prevVal + fsel(rate, range.y, range.x) * dt : val;
    return result;
  }
  Point2 range;
  float prevVal;
};

// First Order Filter
template <typename Value, typename Time>
class FirstOrderFilter
{
public:
  FirstOrderFilter() : invTimeConst(1.0), result() { ; }
  inline void setTimeConstant(Time time_const_in)
  {
    G_ASSERT(rabs(time_const_in) > VERY_SMALL_NUMBER);
    invTimeConst = 1.0f / time_const_in;
  }
  inline Time getTimeConstant() const { return 1.0f / invTimeConst; }
  inline void setState(Value val) { result = val; }
  inline Value simulate(Value val, Time dt)
  {
    G_ASSERT(rabs(dt) > VERY_SMALL_NUMBER);
    return result += (val - result) * dt * invTimeConst;
  }
  inline Value get() const { return result; }

private:
  Time invTimeConst;
  Value result;
};

// Alpha-Betta-filter

template <typename T, typename K>
inline void simulate_ab_filter(K alpha, K betta, K dt, const T &in, T &in_out_value, T &in_out_rate)
{
  in_out_value += in_out_rate * dt;
  const T diff = in - in_out_value;
  in_out_value += diff * alpha;
  in_out_rate += diff * safeinv(dt) * betta;
}

template <typename T, typename K>
inline T clamp_ab_filter_delta(const T &delta, K lim_min, K lim_max)
{
  return clamp(delta, lim_min, lim_max);
}

template <typename V, typename K>
inline V clamp_ab_filter_delta_vec(const V &delta, K /*lim_min*/, K lim_max)
{
  const K deltaLenSq = delta.lengthSq();
  const K limSq = sqr(lim_max);
  if (deltaLenSq > limSq)
    return delta * sqrtf(limSq / deltaLenSq);
  else
    return delta;
}

template <>
inline Point2 clamp_ab_filter_delta<Point2, float>(const Point2 &delta, float lim_min, float lim_max)
{
  return clamp_ab_filter_delta_vec(delta, lim_min, lim_max);
}

template <>
inline Point3 clamp_ab_filter_delta<Point3, float>(const Point3 &delta, float lim_min, float lim_max)
{
  return clamp_ab_filter_delta_vec(delta, lim_min, lim_max);
}

template <typename T>
inline float ab_filter_scalar(const T &in)
{
  return (float)in;
}

template <typename T>
inline float ab_filter_scalar_vec(const T &in)
{
  return in.length();
}

template <>
inline float ab_filter_scalar<Point2>(const Point2 &in)
{
  return ab_filter_scalar_vec(in);
}

template <>
inline float ab_filter_scalar<Point3>(const Point3 &in)
{
  return ab_filter_scalar_vec(in);
}

template <typename T, typename K>
inline void simulate_ab_filter(K alpha, K betta, K rate_lim, K accel_lim, K dt, const T &in, T &in_out_value, T &in_out_rate)
{
  const K diffRateMax = accel_lim * dt;

  in_out_value += in_out_rate * dt;
  const T diff = in - in_out_value;

  T delta = diff * alpha;
  const K accelDeltaMax = diffRateMax * dt * 0.5f;
  delta = clamp_ab_filter_delta(delta, -accelDeltaMax, accelDeltaMax);
  const float rateLen = ab_filter_scalar(in_out_rate);
  delta = clamp_ab_filter_delta(delta, (-rate_lim - rateLen) * dt, (rate_lim - rateLen) * dt);
  in_out_value += delta;

  in_out_rate += clamp_ab_filter_delta(diff * safeinv(dt) * betta, -diffRateMax, diffRateMax);
  in_out_rate = clamp_ab_filter_delta(in_out_rate, -rate_lim, rate_lim);
}
