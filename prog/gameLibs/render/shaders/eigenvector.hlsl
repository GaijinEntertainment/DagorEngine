#ifndef EIGENVECTOR_MATH_HLSL
#define EIGENVECTOR_MATH_HLSL 1
  // For major eigenvector for symmetric matrices, based on: https://hal.science/hal-01501221/
  // (a, d, f)
  // (d, b, e)
  // (f, e, c)
  // It's sensitive to small values and edge cases (degenerations).
  // Consider scaling values and adding bias.
  float3 majorEigenvector3x3sym(float a, float b, float c, float d, float e, float f)
  {
    float a2 = a*a;
    float b2 = b*b;
    float c2 = c*c;
    float d2 = d*d;
    float e2 = e*e;
    float f2 = f*f;

    float x1 = a2 + b2 + c2 - a*b - a*c - b*c + 3*(d2 + f2 + e2);
    float x2 = ( - (2*a - b - c)*(2*b - a - c)*(2*c - a - b) +
          + 9*((2*c - a - b)*d2 + (2*b - a - c)*f2 + (2*a - b - c)*e2) - 54*d*e*f );

    float phi = atan2(sqrt(max(4*(pow(x1, 3)) - pow(x2, 2), 0)), x2);
    float l = (a + b + c + 2*sqrt(x1)*cos((phi - PI) / 3)) / 3; // For phi = (0, pi), that's the greatest eigenvalue.

    float m_a = d*(c - l) - e*f;
    float m_p = f*(b - l) - d*e;
    float3 vec = float3(m_p*(l - c) - m_a*e, m_a*f, m_p*f);
    return vec;
  }
#endif