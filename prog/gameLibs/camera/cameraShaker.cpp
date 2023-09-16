#include <camera/cameraShaker.h>

#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix.h>
#include <math/random/dag_random.h>

float globalCameraShakeLimit = 0.4;

CameraShaker::CameraShaker() { reset(); }

void CameraShaker::reset()
{
  currentShake = 0;
  wishShake = 0;
  wishShakeHiFreq = 0;
  wishSmoothShakeHiFreq = 0.f;
  shaker.reset();
  hiFreqShaker.reset();
  shakerValue.zero();
  shakerSmoothHiFreqValue.zero();
  shakerHiFreqValue.zero();
  shakerPosInfluence.set(0.f, 0.f, 0.f);
  shakerDirInfluence.set(1.f, 1.f, 1.f);
  shakeZeroTau = 0.5f;
  shakeHiFreqZeroTau = 0.3f;
  shakeHiFreqSmoothZeroTau = 0.3f;
  shakeMultiplier = 1.0f;
  useRandomMult = true;
}

void CameraShaker::setShake(float wishShake_)
{
  if (wishShake_ > wishShake)
    wishShake = wishShake_;
}

void CameraShaker::setShakeHiFreq(float wishShake_)
{
  if (wishShake_ > wishShakeHiFreq)
    wishShakeHiFreq = wishShake_;
}

void CameraShaker::setShakeMultiplier(float mult) { shakeMultiplier = mult; }

void CameraShaker::setSmoothShakeHiFreq(float wishShake_) { wishSmoothShakeHiFreq = max(wishShake_, wishSmoothShakeHiFreq); }

void CameraShaker::setRandomMult(bool enabled) { useRandomMult = enabled; }

void CameraShaker::shakeMatrix(TMatrix &tm)
{
  TMatrix shakerTm;
  shakerTm.rotxTM((shakerValue.x * shakerDirInfluence.x + shakerSmoothHiFreqValue.x * shakerDirInfluence.y +
                    shakerHiFreqValue.x * shakerDirInfluence.z) *
                  shakeMultiplier);
  tm = tm * shakerTm;
  shakerTm.rotyTM((shakerValue.y * shakerDirInfluence.x + shakerSmoothHiFreqValue.y * shakerDirInfluence.y +
                    shakerHiFreqValue.y * shakerDirInfluence.z) *
                  shakeMultiplier);
  tm = tm * shakerTm;
  shakerTm.rotzTM((shakerValue.z * shakerDirInfluence.x + shakerSmoothHiFreqValue.z * shakerDirInfluence.y +
                    shakerHiFreqValue.z * shakerDirInfluence.z) *
                  shakeMultiplier);
  shakerTm.setcol(3,
    (shakerValue * shakerPosInfluence.x + shakerSmoothHiFreqValue * shakerPosInfluence.y + shakerHiFreqValue * shakerPosInfluence.z) *
      shakeMultiplier);
  tm = tm * shakerTm;
}

Point3 CameraShaker::calcShakeFreqValue(const float wish_shake_freq)
{
  if (useRandomMult)
    return Point3(gsrnd() * gsrnd() * wish_shake_freq, gsrnd() * gsrnd() * wish_shake_freq, gsrnd() * gsrnd() * wish_shake_freq);
  else
    return Point3(sign(gsrnd()) * 0.5f * wish_shake_freq, sign(gsrnd()) * 0.5f * wish_shake_freq,
      sign(gsrnd()) * 0.5f * wish_shake_freq);
}

void CameraShaker::update(float dt, float eps)
{
  currentShake = approach(currentShake, wishShake, dt, 0.1);
  wishShake = approach(wishShake, 0.0f, dt, shakeZeroTau);
  shakerValue.zero();
  shakerHiFreqValue.zero();
  shakerSmoothHiFreqValue.zero();

  if (wishShake < eps)
    wishShake = 0.0;
  if (currentShake < eps)
    wishShake = 0.0;

  if (wishShakeHiFreq > 0.5)
    wishShakeHiFreq = 0.5;

  if (wishShakeHiFreq < 0.001)
    wishShakeHiFreq = 0.0;
  else
  {
    shakerHiFreqValue = calcShakeFreqValue(wishShakeHiFreq);
    wishShakeHiFreq = approach(wishShakeHiFreq, 0.0f, dt, shakeHiFreqZeroTau);
  }

  if (wishSmoothShakeHiFreq < 0.001f)
    wishSmoothShakeHiFreq = 0.f;
  else
  {
    hiFreqShaker.setup(shakeSmoothFadeCoeff, wishSmoothShakeHiFreq);
    shakerSmoothHiFreqValue = hiFreqShaker.getNextValue(dt);
    wishSmoothShakeHiFreq = approach(wishSmoothShakeHiFreq, 0.f, dt, shakeHiFreqSmoothZeroTau);
  }

  if (currentShake > 0.15)
    currentShake = approach(currentShake, 0.15f, dt, 0.1);

  if (currentShake > 0.35)
    currentShake = approach(currentShake, 0.35f, dt, 0.1);

  if (currentShake > globalCameraShakeLimit)
    currentShake = globalCameraShakeLimit;

  shaker.setup(shakeFadeCoeff, currentShake);
  shakerValue = shaker.getNextValue(dt);
}
