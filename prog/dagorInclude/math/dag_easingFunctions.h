//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>

// see https://easings.net/ for cheatsheet
// copy of https://github.com/warrenm/AHEasing

// Modeled after the line y = x
inline float linear(float p) { return p; }

// Modeled after the parabola y = x^2
inline float inQuad(float p) { return p * p; }

// Modeled after the parabola y = -x^2 + 2x
inline float outQuad(float p) { return -(p * (p - 2.f)); }

// Modeled after the piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
inline float inOutQuad(float p)
{
  if (p < 0.5f)
  {
    return 2.f * p * p;
  }
  else
  {
    return (-2.f * p * p) + (4.f * p) - 1.f;
  }
}

// Modeled after the cubic y = x^3
inline float inCubic(float p) { return p * p * p; }

// Modeled after the cubic y = (x - 1)^3 + 1
inline float outCubic(float p)
{
  float f = (p - 1.f);
  return f * f * f + 1.f;
}

// Modeled after the piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
inline float inOutCubic(float p)
{
  if (p < 0.5f)
  {
    return 4.f * p * p * p;
  }
  else
  {
    float f = ((2.f * p) - 2.f);
    return 0.5f * f * f * f + 1.f;
  }
  // return (3.f - 2.f * p) * p * p; //originally in gameMath/easing
}

// Modeled after the quartic x^4
inline float inQuart(float p) { return p * p * p * p; }

// Modeled after the quartic y = 1 - (x - 1)^4
inline float outQuart(float p)
{
  float f = (p - 1.f);
  return f * f * f * (1.f - p) + 1.f;
}

// Modeled after the piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
inline float inOutQuart(float p)
{
  if (p < 0.5f)
  {
    return 8.f * p * p * p * p;
  }
  else
  {
    float f = (p - 1.f);
    return -8.f * f * f * f * f + 1.f;
  }
}


// Quintic easing; p^5


// Modeled after the quintic y = x^5
inline float inQuintic(float p) { return p * p * p * p * p; }

// Modeled after the quintic y = (x - 1)^5 + 1
inline float outQuintic(float p)
{
  float f = (p - 1.f);
  return f * f * f * f * f + 1.f;
}

// Modeled after the piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
inline float inOutQuintic(float p)
{
  if (p < 0.5f)
  {
    return 16.f * p * p * p * p * p;
  }
  else
  {
    float f = ((2.f * p) - 2.f);
    return 0.5f * f * f * f * f * f + 1.f;
  }
  // return (6.f * p * p - 15.f * p + 10) * p * p * p; //originally in gameMath/easing
}


// Sine wave easing; ins(p * PI/2)


// Modeled after quarter-cycle of sine wave
inline float inSine(float p) { return sinf((p - 1.f) * TWOPI) + 1.f; }

// Modeled after quarter-cycle of sine wave (different phase)
inline float outSine(float p) { return sinf(p * TWOPI); }

// Modeled after half sine wave
inline float inOutSine(float p) { return 0.5f * (1.f - cosf(p * PI)); }


// Circular easing; sqrt(1 - p^2)


// Modeled after shifted quadrant IV of unit circle
inline float inCirc(float p) { return 1.f - sqrtf(1.f - (p * p)); }

// Modeled after shifted quadrant II of unit circle
inline float outCirc(float p) { return sqrtf((2.f - p) * p); }

// Modeled after the piecewise circular function
// y = (1/2)(1 - sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
inline float inOutCirc(float p)
{
  if (p < 0.5f)
    return 0.5f * (1.f - sqrtf(1.f - 4.f * (p * p)));
  else
    return 0.5f * (sqrtf(-((2.f * p) - 3.f) * ((2.f * p) - 1.f)) + 1.f);
}


// Exponential easing, base 2


// Modeled after the exponential function y = 2^(10(x - 1))
inline float inExp(float p) { return (p == 0.f) ? p : powf(2.f, 10.f * (p - 1.f)); }

// Modeled after the exponential function y = -2^(-10x) + 1
inline float outExp(float p) { return (p == 1.f) ? p : 1.f - powf(2.f, -10.f * p); }

// Modeled after the piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
inline float inOutExp(float p)
{
  if (p == 0.f || p == 1.f)
    return p;

  if (p < 0.5f)
  {
    return 0.5f * powf(2.f, (20.f * p) - 10.f);
  }
  else
  {
    return -0.5f * powf(2.f, (-20.f * p) + 10.f) + 1.f;
  }
}


// Exponentially-damped sine wave easing

// Modeled after the damped sine wave y = sinf(13pi/2*x)*powf(2, 10 * (x - 1))
inline float inElastic(float p) { return sinf(13.f * TWOPI * p) * powf(2.f, 10.f * (p - 1.f)); }

// Modeled after the damped sine wave y = sinf(-13pi/2*(x + 1))*powf(2, -10x) + 1
inline float outElastic(float p) { return sinf(-13.f * TWOPI * (p + 1.f)) * powf(2.f, -10.f * p) + 1.f; }

// Modeled after the piecewise exponentially-damped sine wave:
// y = (1/2)*sinf(13pi/2*(2*x))*powf(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(sinf(-13pi/2*((2x-1)+1))*powf(2,-10(2*x-1)) + 2) ; [0.5, 1]
inline float inOutElastic(float p)
{
  if (p < 0.5f)
  {
    return 0.5f * sinf(13.f * TWOPI * (2.f * p)) * powf(2.f, 10.f * ((2.f * p) - 1.f));
  }
  else
  {
    return 0.5f * (sinf(-13.f * TWOPI * ((2.f * p - 1.f) + 1.f)) * powf(2.f, -10.f * (2.f * p - 1.f)) + 2.f);
  }
}


// Overshooting cubic easing;

// Modeled after the overshooting cubic y = x^3-x*sinf(x*pi)
inline float inBack(float p) { return p * p * p - p * sinf(p * PI); }

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*sinf((1-x)*pi))
inline float outBack(float p)
{
  float f = (1.f - p);
  return 1.f - (f * f * f - f * sinf(f * PI));
}


// Modeled after the piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*sinf(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*sinf((1-x)*pi))+1) ; [0.5, 1]
inline float inOutBack(float p)
{
  if (p < 0.5f)
  {
    float f = 2.f * p;
    return 0.5f * (f * f * f - f * sinf(f * PI));
  }
  else
  {
    float f = (1.f - (2.f * p - 1.f));
    return 0.5f * (1.f - (f * f * f - f * sinf(f * PI))) + 0.5f;
  }
}


// Exponentially-decaying bounce easing


inline float outBounce(float p)
{
  if (p < 4.f / 11.f)
  {
    return (121.f * p * p) / 16.f;
  }
  else if (p < 8.f / 11.f)
  {
    return (363.f / 40.f * p * p) - (99.f / 10.f * p) + 17.f / 5.f;
  }
  else if (p < 9.f / 10.f)
  {
    return (4356.f / 361.f * p * p) - (35442.f / 1805.f * p) + 16061.f / 1805.f;
  }
  else
  {
    return (54.f / 5.f * p * p) - (513.f / 25.f * p) + 268.f / 25.f;
  }
}

inline float inBounce(float p) { return 1.f - outBounce(1.f - p); }

inline float inOutBounce(float p)
{
  if (p < 0.5f)
  {
    return 0.5f * inBounce(p * 2.f);
  }
  else
  {
    return 0.5f * outBounce(p * 2.f - 1.f) + 0.5f;
  }
}


// our own internal and shiny

inline float inOutBezier(float p) { return p * p * (3.f - 2.f * p); }


inline float cosineFull(float p) { return 0.5f - cosf(p * TWOPI) * 0.5f; }

inline float inStep(float p) { return p > 0.5f ? 1.f : 0.f; }

inline float outStep(float p) { return p < 0.5f ? 1.f : 0.f; }

inline float blink(float p) { return (p < 0.1) ? 10.0 * p : 1.0 - (p - 0.1) / 0.9; }
inline float doubleBlink(float p) { return 0.5 - 0.5 * cosf(4 * PI * p); }

inline float blinkSin(float p)
{
  float val = (p < 0.1) ? 10.0 * p : 1.0 - (p - 0.1) / 0.9;
  if (p < 0.5)
    val *= 0.75 + 0.25 * sin((p * 80.0 - 0.5) * PI);
  return val;
}

inline float blinkCos(float p)
{
  float val = (p < 0.1) ? 10.0 * p : 1.0 - (p - 0.1) / 0.9;
  if (p < 0.5)
    val *= 0.75 + 0.25 * cosf((p * 40.0 - 0.5) * PI);
  return val;
}
