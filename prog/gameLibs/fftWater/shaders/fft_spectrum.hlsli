#ifndef M_PI
#define M_PI PI
#endif

#ifndef GRAV_ACCEL
#define GRAV_ACCEL 9.810f  // The acceleration of gravity, m/s^2
#endif

#ifndef SQR
#define SQR(a) ((a) * (a))
#endif

float omegaPhillips(float k)
{
  // The angular frequency is following the dispersion relation:
  //            omega^2 = g*k
  // So the equation of Gerstner wave is:
  //            x = x0 - K/k * A * sin(dot(K, x0) - sqrt(g * k) * t), x is a 2D vector.
  //            z = A * cos(dot(K, x0) - sqrt(g * k) * t)
  // Gerstner wave means: a point on a simple sinusoid wave is doing a uniform circular motion. The
  // center is (x0, y0, z0), the radius is A, and the circle plane is parallel to K.
  return sqrt(GRAV_ACCEL * k);
}

// Phillips Spectrum
// K: normalized wave vector, W: wind direction, v: wind velocity, a: constant affecting factor
static inline float calcPhillips(float2 K, float2 W, float v, float a, float dir_depend,
  float small_wave_fraction, float wind_align)
{
  // largest possible wave from constant wind of velocity v
  float l = v * v / GRAV_ACCEL;
  // damp out waves with very small length w << l
  float w = small_wave_fraction * l;
  float Ksqr = K.x * K.x + K.y * K.y;
  float KLenInv = 1.0f / max(sqrt(Ksqr), 0.000001f);
  float2 KNorm = float2(K.x * KLenInv, K.y * KLenInv);
  float Kcos = KNorm.x * W.x + KNorm.y * W.y;
  // Moskowitz (PM Gnnw U)
  // Optimized version, gives almost identical results but with different scale
  float phillips = Ksqr < 0.000001f ? 0 : a * exp(-1 / (l * l * Ksqr)) / (Ksqr * Ksqr) * pow(Kcos * Kcos, wind_align);
  // filter out waves moving opposite to wind
  if (Kcos < 0)
    phillips *= (1.0f-dir_depend);
  // SQR(2.718281828459045*0.25/(2*PI)) - to get normalized amplitudes in range of 0..1
  float normScale = 0.011697936092864;
  // damp out waves with very small length w << l
  return phillips * exp(-Ksqr * w * w) * normScale;
}

float omegaUnifiedDirectional(float k)
{
  const float km = 370.0; // Eq 24
  return sqrt(9.81 * k * (1.0 + SQR(k / km))); // Eq 24
}

// 1/kx and 1/ky in meters
float calcUnifiedDirectional(float2 K, float2 W, float v, float a, float dir_depend, float small_wave_fraction, float wind_align,
  bool omnispectrum = false)
{
  float WIND = v;
  float OMEGA = 0.84; // sea state (inverse wave age)
  const float cm = 0.23; // Eq 59
  const float km = 370.0; // Eq 24
  float A = a; // wave amplitude factor (should be one)

  float U10 = WIND;
  float Omega = OMEGA;

  // phase speed
  float k = sqrt(K.x * K.x + K.y * K.y);
  if (k <= 0.0001)
  {
    return 0;
  }
  K = K.x * W + K.y * float2(W.y, -W.x);
  float kx = K.x;
  float ky = K.y;
  float Kcos = kx;

  float c = omegaUnifiedDirectional(k) / k;

  // spectral peak
  float kp = 9.81 * SQR(Omega / U10); // after Eq 3
  float cp = omegaUnifiedDirectional(kp) / kp;

  // friction velocity
  float z0 = 3.7e-5 * SQR(U10) / 9.81 * pow(U10 / cp, 0.9f); // Eq 66
  float u_star = 0.41 * U10 / log(10.0 / z0); // Eq 60

  float Lpm = exp(- 5.0 / 4.0 * SQR(kp / k)); // after Eq 3
  float gamma = Omega < 1.0 ? 1.7 : 1.7 + 6.0 * log(Omega); // after Eq 3 // log10 or log?? - ln, i.e. log
  float sigma = 0.08 * (1.0 + 4.0 / pow(Omega, 3.0f)); // after Eq 3
  float Gamma = exp(-1.0 / (2.0 * SQR(sigma)) * SQR(sqrt(k / kp) - 1.0));
  float Jp = pow(gamma, Gamma); // Eq 3
  float Fp = Lpm * Jp * exp(- Omega / sqrt(10.0) * (sqrt(k / kp) - 1.0)); // Eq 32
  float alphap = 0.006 * sqrt(Omega); // Eq 34
  float Bl = 0.5 * alphap * cp / c * Fp; // Eq 31

  float alpham = 0.01 * (u_star < cm ? 1.0 + log(u_star / cm) : 1.0 + 3.0 * log(u_star / cm)); // Eq 44
  float Fm = exp(-0.25 * SQR(k / km - 1.0)); // Eq 41
  float Bh = 0.5 * alpham * cm / c * Fm * Lpm; // Eq 40 (fixed)

  if (omnispectrum)
  {
      return max(A * (Bl + Bh) / (k * SQR(k)), 0.0f); // Eq 30
  }

  float a0 = log(2.0) / 4.0; float ap = 4.0; float am = 0.13 * u_star / cm; // Eq 59
  float Delta = tanh(a0 + ap * pow(c / cp, 2.5f) + am * pow(cm / c, 2.5f)); // Eq 57

  float phi = atan2(ky, kx);

  Bl *= 2.0;
  Bh *= 2.0;

  float sw = A * (Bl + Bh) * (1.0 + Delta * cos(2.0 * phi)) / (2.0 * M_PI * SQR(SQR(k))); // Eq 67
  sw = max(sw, 0.0f);

  // filter out waves moving opposite to wind
  if (Kcos < 0)
    sw *= (1.0f-dir_depend);

  return sw;
}