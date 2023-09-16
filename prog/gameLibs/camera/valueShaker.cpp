#include <camera/valueShaker.h>
#include <math/dag_mathUtils.h>
#include <math/random/dag_random.h>

ValueShaker::ValueShaker() : value(ZERO<Point3>()), vel(ZERO<Point3>()), fadeKoeff(0.0f), amp(0.0f) { reset(); }

void ValueShaker::reset()
{
  value = Point3(0, 0, 0);
  vel = Point3(0, 0, 0);
}

void ValueShaker::setup(float fadeKoeff_, float amp_)
{
  fadeKoeff = fadeKoeff_;
  amp = amp_;
}

Point3 ValueShaker::getNextValue(float dt)
{
  if (dt < 0.02f)
  {
    vel = approach(vel, Point3(0, 0, 0), dt, fadeKoeff) + Point3(gsrnd(), gsrnd(), gsrnd()) * dt * amp;
    value = approach(value, Point3(0, 0, 0), dt, fadeKoeff) + vel * amp;
    return value;
  }
  else
  {
    for (; dt > 0; dt -= 0.02f)
    {
      float t = min(dt, 0.02f);
      vel = approach(vel, Point3(0, 0, 0), t, fadeKoeff) + Point3(gsrnd(), gsrnd(), gsrnd()) * t * amp;
      value = approach(value, Point3(0, 0, 0), t, fadeKoeff) + vel * amp;
    }
    return value;
  }
}
