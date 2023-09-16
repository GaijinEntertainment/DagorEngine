#ifndef PHASE_FUNCTIONS
#define PHASE_FUNCTIONS 1

float phaseFunctionHGBase(float mu, float mieG)
{
  float h = 1.0 + (mieG*mieG) - 2.0*mieG*mu;
  return (1.0 - mieG*mieG) * pow(h, -3.0/2.0) * (1.0 + mu * mu) / (2.0 + mieG*mieG);//todo: can be optimized
}

float phaseSchlickBase(float mu, float k)
{
  // Schlick phase function
  const float k2 = 1.0 - k * k;
  float h = 1 - (k * mu);
  return max(0, k2 / (h * h));
}
float phaseSchlick(float mu, float k)
{
  return phaseSchlickBase(mu,k)*(PI*1./(4.*PI));//sun_color is premultiplied by 1/pi
}

float phaseSchlickTwoLobe(float mu, float k0, float k1)
{
  return (phaseSchlickBase(mu,k0) + phaseSchlickBase(mu,k1)) *(PI*0.5/(4.*PI));//sun_color is premultiplied by 1/pi
}

float phaseFunctionM(float mu, float mieG)
{
  return (1.5 * 1.0 / (PI*4.0 * PI)) * phaseFunctionHGBase(mu, mieG);//sun_color is premultiplied by 1/pi
}
float phaseFunctionHGTwoLobe(float mu, float mieG0, float mieG1)
{
  return (1.5 * 1.0 / (4.0 * PI)) * 0.5 * (phaseFunctionHGBase(mu, mieG0) + phaseFunctionHGBase(mu, mieG1));
}
float phaseFunctionConst()
{
  return 1./2;//PI/ (4*PI);//float h = float_abs(mieG2_plus_1 + neg_two_mieG*mu);//ambient is premultiplied by 1/pi
}

#endif