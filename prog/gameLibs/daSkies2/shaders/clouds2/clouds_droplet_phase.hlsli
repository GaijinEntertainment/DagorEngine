#ifndef CLOUDS_DROPLET_PHASE_HLSLI
#define CLOUDS_DROPLET_PHASE_HLSLI 1
//Jendersie & d'Eon, "An Approximate Mie Scattering Function for Fog and Cloud
//Rendering" (SIGGRAPH 2023): HG+Draine mixture fitted to Mie solutions for water
//droplets of diameter d in [5..50] um. Recovers fogbow, glory and silver-lining
//sharpening that a dual-lobe HG cannot represent.
//Phase values are the standard (1/4pi-normalized) phase multiplied by PI: cloud
//lighting pre-divides light by PI and uses 1/4 in its phase constants.

//the LUT stores only the smooth w*Draine remainder (narrowest scale ~(1-gD)^2 = 0.14
//in cos space), so 128 texels keep the linear-interp error ~0.2%; the sub-texel
//(1-w)*HG forward spike is evaluated analytically at the lookup
#define CLOUDS_PHASE_LUT_SIZE 128

//C++ consumers must provide exp/sqrt (math.h) before including this file
#ifdef __cplusplus
#define DROPLET_FUNC static inline float
#else
#define DROPLET_FUNC float
#endif

DROPLET_FUNC clouds_droplet_hg_g(float d) { return float(exp(-0.0990567f/(d - 1.67154f))); }
DROPLET_FUNC clouds_droplet_hg_weight(float d) { return float(exp(-0.599085f/(d - 0.641583f) - 0.665888f)); }
DROPLET_FUNC clouds_droplet_draine_g(float d) { return float(exp(-2.20679f/(d + 3.91029f) - 0.428934f)); }
DROPLET_FUNC clouds_droplet_draine_alpha(float d) { return float(exp(3.62489f - 8.29288f/(d + 5.52825f))); }
DROPLET_FUNC draine_phase_x_pi(float a, float g, float c)
{
  float g2 = g*g;
  float h = 1.f + g2 - 2.f*g*c;
  return ((1.f - g2)*(1.f + a*c*c)) / (4.f*(1.f + a*(1.f + 2.f*g2)/3.f) * h*float(sqrt(h)));
}
DROPLET_FUNC hg_phase_x_pi(float g, float c)
{
  float g2 = g*g;
  float h = 1.f + g2 - 2.f*g*c;
  return (1.f - g2) / (4.f * h*float(sqrt(h)));
}
#undef DROPLET_FUNC
#endif
