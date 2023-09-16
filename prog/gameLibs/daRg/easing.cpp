#include "easing.h"

#include <math/dag_mathBase.h>
#include <math/dag_easingFunctions.h>


namespace darg
{

namespace easing
{

static float discrete8(float p) { return floorf(p * 8) / 8; }
static float shake4(float p) { return sinf(4.0f * p * TWOPI) * sinf(p * PI); }
static float shake6(float p) { return sinf(6.0f * p * TWOPI) * sinf(p * PI); }


Func functions[] = {linear, inQuad, outQuad, inOutQuad, inCubic, outCubic, inOutCubic, inQuintic, outQuintic, inOutQuintic, inQuart,
  outQuart, inOutQuart, inSine, outSine, inOutSine, inCirc, outCirc, inOutCirc, inExp, outExp, inOutExp, inElastic, outElastic,
  inOutElastic, inBack, outBack, inOutBack, inBounce, outBounce, inOutBounce, inOutBezier, cosineFull, inStep, outStep, blink,
  doubleBlink, blinkSin, blinkCos,

  discrete8, shake4, shake6};


} // namespace easing

} // namespace darg
