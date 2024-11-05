// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vehiclePhys/carDynamics/tireForces.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>

// Table A3.1. (205/60R15 91V, 2.2 bar. ISO sign definition. Also, cf. App.3.2)
namespace pacejka
{
static const float Ro = 0.313, Fzo = 4000, mo = 9.3, Vo = 16.67, pCx1 = 1.685, pEx3 = -0.020, pHx1 = -0.002, rBx1 = 12.35, qsx1 = 0,
                   pCy1 = 1.193, pEy2 = -0.537, pKy3 = -0.028, pHy1 = 0.003, pVy1 = 0.045, rBy1 = 6.461, rHy1 = 0.009, rVy5 = 1.9,
                   qBz1 = 8.964, qBz9 = 18.47, qDz3 = 0.007, qDz9 = -0.009, qEz1 = -1.609, qEz5 = -0.896, ssz1 = 0.043, qIay = 0.109,
                   qIaxz = 0.071, qIby = 0.696, qIbxz = 0.357, qIc = 0.055, qV1 = 7.1e-5, qV2 = 2.489, qFz1 = 13.37, qFz2 = 14.35,
                   pDx1 = 1.210, pEx4 = 0, pHx2 = 0.002, rBx2 = -10.77, qsx2 = 0, pDy1 = -0.990, pEy3 = -0.083, pKy4 = 2,
                   pHy2 = -0.001, pVy2 = -0.024, rBy2 = 4.196, rVy1 = 0.053, rVy6 = -10.71, qBz2 = -1.106, qBz10 = 0, qDz4 = 13.05,
                   qDz10 = 0, qEz2 = -0.359, qHz1 = 0.007, ssz2 = 0.001, qma = 0.237, qmb = 0.763, qmc = 0.108, qFz3 = 0, qsy1 = 0.01,
                   qsy3 = 0, qsy4 = 0, pDx2 = -0.037, pKx1 = 21.51, pVx1 = 0, rBx3 = 0, qsx3 = 0, pDy2 = 0.145, pEy4 = -4.787,
                   pKy5 = 0, pHy3 = 0, pVy3 = -0.532, rBy3 = -0.015, rVy2 = -0.073, qBz3 = -0.842, qCz1 = 1.180, qDz6 = -0.008,
                   qDz11 = 0, qEz3 = 0, qHz2 = -0.002, ssz3 = 0.731, qcbx0_z = 121.4, qcby = 40.05, qccx = 391.9, qccy = 62.7,
                   qa1 = 0.135, qa2 = 0.035, qbvx_z = 3.957, qbvteta = 3.957, pEx1 = 0.344, pKx2 = -0.163, pVx2 = 0, rCx1 = 1.092,
                   pDy3 = -11.23, pKy1 = -14.95, pKy6 = -0.92, pVy4 = 0.039, rBy4 = 0, rVy3 = 0.517, qBz5 = -0.227, qDz1 = 0.100,
                   qDz7 = 0.000, qEz4 = 0.174, qHz3 = 0.147, ssz4 = -0.238, qkbx_z = 0.228, qkby = 0.284, qkcx = 0.910, qkcy = 0.910,
                   Breff = 9, Dreff = 0.23, Freff = 0.01, pEx2 = 0.095, pKx3 = 0.245, rHx1 = 0.007, pEy1 = -1.003, pKy2 = 2.130,
                   pKy7 = -0.24, rCy1 = 1.081, rVy4 = 35.44, qBz6 = 0, qDz2 = -0.001, qDz8 = -0.296, qHz4 = 0.004, qcbteta0 = 61.96,
                   qcbgamma_psi = 20.33, qccpsi = 55.82, qkbteta = 0.080, qkbgamma_psi = 0.038, qkcpsi = 0.834, qFcx1 = 0.1,
                   qFcy1 = 0.3, qFcx2 = 0, qFcy2 = 0,

                   //== next coefs are not listed in reference table, so assumed 0
  pEy5 = 0, rHy2 = 0;

static const float pDyphy1 = 0.4, pDyphy2 = 0, pDyphy3 = 0, pDyphy4 = 0, pDxphy1 = 0.4, pDxphy2 = 0, pDxphy3 = 0, pKyphy1 = 1,
                   pHyphy1 = 1, pHyphy2 = 0.15, pHyphy3 = 0, pHyphy4 = -4, pEpsgphy1 = 0, pEpsgphy2 = 0, qDtphy1 = 10, qCrphy1 = 0.2,
                   qCrphy2 = 0.1, qBrphy1 = 0.1, qDrphy1 = 1, qDrphy2 = -1.5;
} // namespace pacejka

static inline float mf_sin(float C, float B, float E) { return sinf(C * atanf(B - E * (B - atanf(B)))); }

static inline float mf_cos(float C, float B, float E) { return cosf(C * atanf(B - E * (B - atanf(B)))); }

static inline float mf_sin0(float C, float B) { return sinf(C * atanf(B)); }

static inline float mf_cos0(float C, float B) { return cosf(C * atanf(B)); }

static inline float sgn(float x) { return x > 0 ? 1 : (x < 0 ? -1 : 0); }

TireForceModel2::TireForceModel2()
{
  gamma = 0;
  a_s = 0;
  k = 0;
  Fz = 0;
  Vs = 0;
  absVcx = 1;
  sgnVcx = 1;
  cosAq = 1;
  mxs = mys = 1.0;
  phyT = phy = 0.0;

  laFz0 = laMux = laMuy = laKxk = laKya = laCx = laEx = laHx = laVx = laCy = laEy = laHy = laVy = laKyg = laKzg = lat = laMr = 1;
  laMuV = 0;

  laxa = layk = laVyk = las = 1;

  laCz = laMx = laMy = 1;

  rEx1 = 0.4;
  rEx2 = 0;
  rEy1 = 1.0;
  rEy2 = 0;

  // Output
  Fx = Fy = 0;
  Mz = 0;
  Gxa = Gyk = 0.f;
}

void TireForceModel2::load(const DataBlock *blk, bool left_wheel)
{
#define GET(v)     v = blk->getReal(#v, 1)
#define GET0(v)    v = blk->getReal(#v, 0)
#define GETX(v, x) v = blk->getReal(#v, x)

  GET(laFz0);
  GET(laMux);
  GET(laMuy);
  GET(laKxk);
  GET(laKya);
  GET(laCx);
  GET(laEx);
  GET(laHx);
  GET(laVx);
  GET(laCy);
  GET(laEy);
  GET(laHy);
  GET(laVy);
  GET(laKyg);
  GET(laKzg);
  GET(lat);
  GET(laMr);
  GET0(laMuV);

  GET(laxa);
  GET(layk);
  GET(laVyk);
  GET(las);

  GET(laCz);
  GET(laMx);
  GET(laMy);

  GETX(rEx1, 0.4);
  GETX(rEx2, 0);
  GETX(rEy1, 1.0);
  GETX(rEy2, 0);

  if (left_wheel)
  {
    laVyk = -laVyk;
    laMr = -laMr;
    las = -las;
  }

#undef GETX
#undef GET0
#undef GET
}


void TireForceModel2::computeForces(bool calc_mz)
{
  using namespace pacejka;
  static const float eps = 0.1, Amu = 10;
  float ksi0 = 1, ksi1 = 1, ksi2 = 1, ksi3 = 1, ksi4 = 1;
  bool simple_ksi = true;

  float g_s = sinf(gamma);
  float dfz = (Fz - Fzo * laFz0) / (Fzo * laFz0);
  float laMux_s = mxs * laMux / (1 + laMuV * Vs / Vo);
  float laMuy_s = mys * laMuy / (1 + laMuV * Vs / Vo);
  float laMux_q = Amu * laMux_s / (1 + (Amu - 1) * laMux_s);
  float laMuy_q = Amu * laMuy_s / (1 + (Amu - 1) * laMuy_s);
  float eps_g = pEpsgphy1 * (1 + pEpsgphy2 * dfz);

  if (fabs(phyT) > 0.05)
  {
    // calc ksi0..ksi3 for curvilinear motion
    float abs_phyT = fabsf(phyT);
    float tana = a_s * sgnVcx;
    float Byphy = pDyphy1 * (1 + pDyphy2 * dfz) * mf_cos0(1, pDyphy3 * tana);
    float Bxphy = pDxphy1 * (1 + pDxphy2 * dfz) * mf_cos0(1, pDxphy3 * k);
    ksi0 = 0;
    ksi1 = mf_cos0(1, Bxphy * Ro * phy);
    ksi2 = mf_cos0(1, Byphy * (Ro * abs_phyT + pDyphy4 * sqrtf(Ro * abs_phyT)));
    ksi3 = mf_cos0(1, pKyphy1 * Ro * Ro * phyT * phyT);
    simple_ksi = false;
  }

  // pure longitudinal slip
  float Cx = pCx1 * laCx;
  float mux = (pDx1 + pDx2 * dfz) * laMux_s;
  float Dx = mux * Fz * ksi1;
  float Ex = (pEx1 + pEx2 * dfz + pEx3 * dfz * dfz) * (1 - pEx4 * sgn(k)) * laEx;
  float Kxk = Fz * (pKx1 + pKx2 * dfz) * expf(pKx3 * dfz) * laKxk;
  float Bx = Kxk / (Cx * Dx + eps);
  float Shx = (pHx1 + pHx2 * dfz) * laHx;
  float Svx = Fz * (pVx1 + pVx2 * dfz) * (absVcx / (eps + absVcx)) * laVx * laMux_q * ksi1;
  float kx = k + Shx;
  float Fx0 = Dx * mf_sin(Cx, Bx * kx, Ex) + Svx;

  // pure side slip
  float Cy = pCy1 * laCy;
  float muy = ((pDy1 + pDy2 * dfz) / (1 + pDy3 * g_s * g_s)) * laMuy_s;
  float Dy = muy * Fz * ksi2;
  float Kya =
    pKy1 * Fzo * laFz0 * mf_sin0(pKy4, Fz / ((pKy2 + pKy5 * g_s * g_s) * Fzo * laFz0)) / (1 + pKy3 * g_s * g_s) * ksi3 * laKya;
  float Svyg = Fz * (pVy3 + pVy4 * dfz) * g_s * laKyg * laMuy_q * ksi2;
  float Kygo = Fz * (pKy6 + pKy7 * dfz) * laKyg;
  float By = Kya / (Cy * Dy + eps);

  if (!simple_ksi)
  {
    // calc ksi4 for curvilinear motion
    float Chyphy = pHyphy1;
    float Dhyphy = (pHyphy2 + pHyphy3 * dfz) * sgnVcx;
    float Ehyphy = pHyphy4;
    float KyRphy0 = Kygo / (1 - eps_g);
    float Bhyphy = KyRphy0 / (Chyphy * Dhyphy * (By * Cy * Dy) + eps);
    float Shyphy = Dhyphy * mf_sin(Chyphy, Bhyphy * Ro * phy, Ehyphy);
    ksi4 = 1 + Shyphy - Svyg / (Kya + eps);
    // debug("phyT=%.3f phy=%.3f ksi=%.5f  %.5f  %.5f  %.5f  %.5f",
    //   phyT, phy, ksi0, ksi1, ksi2, ksi3, ksi4);
  }

  float Shy = (pHy1 + pHy2 * dfz) * laHy + (Kygo * g_s - Svyg) * ksi0 / (Kya + eps) + ksi4 - 1;
  float ay = a_s + Shy;
  float Ey = (pEy1 + pEy2 * dfz) * (1 + pEy5 * g_s * g_s - (pEy3 + pEy4 * g_s) * sgn(ay)) * laEy;
  float Svy = Fz * (pVy1 + pVy2 * dfz) * laVy * laMuy_q * ksi2 + Svyg;
  float Fy0 = Dy * mf_sin(Cy, By * ay, Ey) + Svy;

  // longitudinal force (combined slip)
  float Shxa = rHx1;
  float Bxa = (rBx1 + rBx3 * g_s * g_s) * mf_cos0(1, rBx2 * k) * laxa;
  float Cxa = rCx1;
  float Exa = rEx1 + rEx2 * dfz;
  float as = a_s + Shxa;
  Gxa = mf_cos(Cxa, Bxa * as, Exa) / mf_cos(Cxa, Bxa * Shxa, Exa);

  // lateral force (combined slip)
  float Shyk = rHy1 + rHy2 * dfz;
  float ks = k + Shyk;
  float Byk = (rBy1 + rBy4 * g_s * g_s) * mf_cos0(1, rBy2 * (a_s - rBy3)) * layk;
  float Cyk = rCy1;
  float Eyk = rEy1 + rEy2 * dfz;
  float Dvyk = muy * Fz * (rVy1 + rVy2 * dfz + rVy3 * g_s) * mf_cos0(1, rVy4 * a_s) * ksi2;
  float Svyk = Dvyk * mf_sin0(rVy5, rVy6 * k) * laVyk;
  Gyk = mf_cos(Cyk, Byk * ks, Eyk) / mf_cos(Cyk, Byk * Shyk, Eyk);

  Fx = Fx0 * Gxa;
  Fy = Fy0 * Gyk + Svyk;

  if (calc_mz)
  {
    float ksi5 = 1, ksi6 = 1, ksi7 = 1, ksi8 = 1;
    float abs_g_s = fabsf(g_s);

    if (!simple_ksi)
    {
      // calc ksi5..ksi8 for curvilinear motion
      float laMphy = 1;

      float Mzphyinf = qCrphy1 * muy * Ro * Fz * sqrtf(Fz / (laFz0 * Fzo)) * laMphy;
      float Mzphy90 = Mzphyinf * 2 / PI * atan(qCrphy2 * Ro * fabsf(phyT)) * Gyk;
      float Cdrphy = qDrphy1;
      float Edrphy = qDrphy2;
      float Ddrphy = Mzphyinf / sin(0.5 * PI * Cdrphy);
      float Kzgr0 = Fz * Ro * (qDz8 + qDz9 * dfz) * laKzg;
      float Bdrphy = Kzgr0 / ((Cdrphy * Ddrphy + eps) * (1 - eps_g));
      float Drphy = Ddrphy * mf_sin(Cdrphy, Bdrphy * Ro * phy, Edrphy);

      ksi5 = mf_cos0(1, qDtphy1 * Ro * phyT);
      ksi6 = mf_cos0(1, qBrphy1 * Ro * phy);
      float ksi7_acos_arg = Mzphy90 / (fabs(Drphy) + eps);
      ksi7_acos_arg = fsel(ksi7_acos_arg - 1, 1.0f, fsel(ksi7_acos_arg + 1, ksi7_acos_arg, -1.0f));
      ksi7 = 2 / PI * acos(ksi7_acos_arg);
      ksi8 = 1 + Drphy;
      // debug("  ksi=%.5f  %.5f  %.5f  %.5f", ksi5, ksi6, ksi7, ksi8);
    }

    // pure side slip
    float Sht = qHz1 + qHz2 * dfz + (qHz3 + qHz4 * dfz) * g_s;
    float at = a_s + Sht;
    float Bt = (qBz1 + qBz2 * dfz + qBz3 * dfz * dfz) * (1 + qBz5 * abs_g_s + qBz6 * g_s * g_s) * laKya / laMuy_s;
    float Ct = qCz1;
    float Dt0 = Fz * (Ro / (Fzo * laFz0)) * (qDz1 + qDz2 * dfz) * lat * sgnVcx;
    float Dt = Dt0 * (1 + qDz3 * abs_g_s + qDz4 * g_s * g_s) * ksi5;
    float Et = (qEz1 + qEz2 * dfz + qEz3 * dfz * dfz) * (1 + (qEz4 + qEz5 * g_s) * 2 / PI * atan(Bt * Ct * at));
    float t0 = Dt * mf_cos(Ct, Bt * at, Et) * cosAq;
    float Mz0_q = -t0 * Fy0;

    float Kya_q = Kya + 0.01;
    float Shf = Shy + Svy / Kya_q;
    float ar = a_s + Shf;
    float Br = (qBz9 * laKya / laMuy_s + qBz10 * By * Cy) * ksi6;
    float Cr = ksi7;
    float Dr =
      Fz * Ro *
        ((qDz6 + qDz7 * dfz) * laMr * ksi2 + (qDz8 + qDz9 * dfz) * g_s * laKzg * ksi0 + (qDz10 + qDz11 * dfz) * g_s * abs_g_s * ksi0) *
        cosAq * laMuy_s * sgnVcx +
      ksi8 - 1;
    float Mzr0 = Dr * mf_cos0(Cr, Br * ar);

    float Mz0 = Mz0_q + Mzr0;

    // combined slip
    float Fy_q = Fy - Svyk;
    float _k = (Kxk / Kya_q) * k;
    float ateq = sqrtf(at * at + _k * _k) * sgn(at);
    float areq = sqrtf(ar * ar + _k * _k) * sgn(ar);
    float t = Dt * mf_cos(Ct, Bt * ateq, Et) * cosAq;
    float Mzr = Dr * mf_cos0(Cr, Br * areq);
    float s = Ro * (ssz1 + ssz2 * (Fy / (Fzo * laFz0)) + (ssz3 + ssz4 * dfz) * g_s) * las;
    float Mz_q = -t * Fy_q;

    Mz = Mz_q + Mzr + s * Fx;
  }
  else
    Mz = 0;
}
