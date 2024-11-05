#ifndef EQUAL_AREA_OCTAHEDRAL_HLSL
#define EQUAL_AREA_OCTAHEDRAL_HLSL

// https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf
// http://jgt.akpeters.com/Clarberg08/

// Returns a unit vector. Argument u,v is an octahedral vector packed via equalAreaOctahedralEncode
//   on the [-1, +1] square
float3 equalAreaOctahedralDecode(float u, float v)
{
  float d = 1 - (abs(u) + abs(v));  // end of 3.1.1
  float r = 1 - abs(d); // end of 3.1.1
  float phi = abs(r) < 1e-12 ? 0 : (PI / 4.) * ((abs(v) - abs(u)) / r + 1.0); // equation (4), plus avoid division by zero
  float z = sign(d)*(1 - r * r); // end of 3.1.1
  // end of 3.1.1 gives us z radius of 1-r*r.
  // so, xy plane radius is sqrt(1 - z^2)
  float planeR = sqrt(saturate(1-z*z));
  return float3(
  	planeR * sign(u) * abs(cos(phi)), // equation (5)
  	planeR * sign(v) * abs(sin(phi)), // equation (5)
  	z
  );

}
float3 equalAreaOctahedralDecode(float2 o)
{
  return equalAreaOctahedralDecode(o.x, o.y);
}

// Takes a unit vector, returns projection on the [-1, +1] square
// https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf
// http://jgt.akpeters.com/Clarberg08/
// Mathematica given me different coeffiecients though
float2 equalAreaOctahedralEncode(float3 v)
{
  float r = sqrt(1 - abs(v.z));
  float2 absV2 = abs(v.xy);
  float x = min(absV2.x, absV2.y) / max3(absV2.x, absV2.y, 1e-19);

  // Coefficients for 6th degree minimax approximation of atan(x)*2/pi, x=[0,1].
  const float t1 = 3.93277e-9;//4.06531e-6;
  const float t2 = 0.636598;//0.636227;
  const float t3 = 0.00123579;//0.00615523;
  const float t4 = -0.224562;//-0.247326;
  const float t5 = 0.0417069;//0.0881627;
  const float t6 = 0.0849741;//0.0419157;
  const float t7 = -0.0399662;//-0.0251427;

  float phi = (((((t6 + t7 * x)*x + t5)*x + t4)*x + t3)*x + t2)*x + t1;

  phi = absV2.x < absV2.y ? 1 - phi : phi;
  float2 o = float2(r - phi * r, phi * r);
  o = (v.z < 0) ? 1 - o.yx : o;
  o = asfloat(asuint(o) ^ (asuint(v.xy) & 0x80000000u));
  return o;
}

#endif