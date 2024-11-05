#ifndef SP_DEPTH_PRECISION_INCLUDED
#define SP_DEPTH_PRECISION_INCLUDED 1

//due to depth buffer precision issues on big zfar, we have to tolerate more errors where hyperZ is imprecise.
//empirically, this happens on zfar more than >10000, and with hyper z precision dependence (actually, it should depend on znear as well, ofc)
//so,with zfar ~8000 this formula results in changing tolerance only within 1m
//with zfar 50000, only first 6.25 meters are affected, (with pow4 dependence)

//float depth_exp_precision_scale(float normalizedW) {return saturate(pow4(normalizedW*8000 + 0.125));}
//float depth_exp_precision_scale(float w, float zfar) {return rcp(1+0.5*w);}
float depth_exp_precision_scale(float w, float zfar) {return 1;}

float depth_exp_precision(float depth_exp, float w, float zfar)
{
  return depth_exp * depth_exp_precision_scale(w, zfar);
}
#endif