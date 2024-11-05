#ifndef PHASE_FUNCTIONS
#define PHASE_FUNCTIONS 1

half phaseFunctionHGBase(half mu, half mieG)
{
  half h = 1.0h + (mieG*mieG) - 2.0*mieG*mu;
  return (1.0h - mieG*mieG) * pow(h, -3.0h/2.0h) * (1.0h + mu * mu) / (2.0h + mieG*mieG);//todo: can be optimized
}

half phaseSchlickBase(half mu, half k)
{
  // Schlick phase function
  const half k2 = 1.0h - k * k;
  half h = 1 - (k * mu);
  return max(0, k2 / (h * h));
}
half phaseSchlick(half mu, half k)
{
  return phaseSchlickBase(mu,k)*(half(PI)*1.h/(4.h*half(PI)));//sun_color is premultiplied by 1/pi
}

half phaseSchlickTwoLobe(half mu, half k0, half k1)
{
  return (phaseSchlickBase(mu,k0) + phaseSchlickBase(mu,k1)) *(half(PI)*0.5h/(4.h*half(PI)));//sun_color is premultiplied by 1/pi
}

half phaseFunctionM(half mu, half mieG)
{
  return (1.5h * 1.0h / (half(PI)*4.0h * half(PI))) * phaseFunctionHGBase(mu, mieG);//sun_color is premultiplied by 1/pi
}
half phaseFunctionHGTwoLobe(half mu, half mieG0, half mieG1)
{
  return (1.5h * 1.0h / (4.0h * half(PI))) * 0.5h * (phaseFunctionHGBase(mu, mieG0) + phaseFunctionHGBase(mu, mieG1));
}
half phaseFunctionConst()
{
  return 1.h/2;//PI/ (4*PI);//half h = half_abs(mieG2_plus_1 + neg_two_mieG*mu);//ambient is premultiplied by 1/pi
}

#endif