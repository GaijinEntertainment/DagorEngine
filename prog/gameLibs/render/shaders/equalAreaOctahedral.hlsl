#ifndef EQUAL_AREA_OCTAHEDRAL_HLSL
#define EQUAL_AREA_OCTAHEDRAL_HLSL

// https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf
// http://jgt.akpeters.com/Clarberg08/


// Returns a unit vector. Argument u,v is an octahedral vector packed via equalAreaOctahedralEncode
//   on the [-1, +1] square
float3 equalAreaOctahedralDecode(float2 uv)
{
  float2 absUV = abs(uv);
  float d = 1.f - (absUV.x + absUV.y);  // end of 3.1.1
  float r = 1.f - abs(d); // end of 3.1.1
  float phi = abs(r) < 1e-12f ? 0 : (PI / 4.) * ((absUV.y - absUV.x) / r + 1.0f); // equation (4), plus avoid division by zero
  float z = sign(d)*(1.f - r * r); // end of 3.1.1
  // end of 3.1.1 gives us z radius of 1-r*r.
  // so, xy plane radius is sqrt(1 - z^2)
  float planeR = sqrt(saturate(1-z*z));
  return float3(
    planeR * sign(uv.x) * abs(cos(phi)), // equation (5)
    planeR * sign(uv.y) * abs(sin(phi)), // equation (5)
    z
  );

}
float3 equalAreaOctahedralDecode(float u, float v)
{
  return equalAreaOctahedralDecode(float2(u, v));
}

// Takes a unit vector, returns projection on the [-1, +1] square
// https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf
// http://jgt.akpeters.com/Clarberg08/
// Mathematica given me different coeffiecients though
float2 equalAreaOctahedralEncode(float3 v)
{
  float r = sqrt(saturate(1 - abs(v.z))); // Saturate to fix numerical inaccuracies casusing NaN in sqrt
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

  phi = absV2.x < absV2.y ? 1.0f - phi : phi;
  float2 o = float2(r - phi * r, phi * r);
  o = (v.z < 0) ? 1.0f - o.yx : o;
  o = asfloat(asuint(o) ^ (asuint(v.xy) & 0x80000000u));
  return o;
}

// Transforms a 3D vector to a position in the square [-1..1]
float2 equalAreaSemiOctahedralDecode(float3 v)
{
  float r = sqrt(saturate(1.0f - abs(v.z)));
  float2 absV2 = abs(v.xy);
  //float a = max(absDir.x, absDir.y);
  //float b = min(absDir.x, absDir.y);
  //b = a == 0.0f ? 0.0f : b / a;
  float x = min(absV2.x, absV2.y) / max3(absV2.x, absV2.y, 1e-19f);

  // Coefficients for 6th degree minimax approximation of atan(x)*2/pi, x=[0,1].
  const float t1 = 3.93277e-9;//4.06531e-6;
  const float t2 = 0.636598;//0.636227;
  const float t3 = 0.00123579;//0.00615523;
  const float t4 = -0.224562;//-0.247326;
  const float t5 = 0.0417069;//0.0881627;
  const float t6 = 0.0849741;//0.0419157;
  const float t7 = -0.0399662;//-0.0251427;

  //float phi = atan(x) * (2.0f / PI);
  float phi = (((((t6 + t7 * x)*x + t5)*x + t4)*x + t3)*x + t2)*x + t1;
  phi = absV2.x < absV2.y ? 1.0f - phi : phi;

  float2 o = float2(r - phi * r, phi * r);
  o *= sign(v).xy;

  // Since we only care about the hemisphere above the surface we rescale and center the output
  //   value range to the it occupies the whole unit square
  return float2(o.x + o.y, o.x - o.y);
}

// Transforms a mapped position in the [-1..1] back to a 3D direction vector.
float3 equalAreaHemiOctahedralDecode(float2 st)
{
  // Transform from unit square to diamond corresponding to +hemisphere
  st = float2(st.x + st.y, st.x - st.y) * 0.5f;

  float2 absMapped = abs(st);
  float distance = 1.0f - (absMapped.x + absMapped.y);
  float radius = 1.0f - abs(distance);

  float phi = (radius == 0.0f) ? 0.0f : PI/4.f * ((absMapped.y - absMapped.x) / radius + 1.0f);
  float radiusSqr = radius * radius;
  float sinTheta = radius * sqrt(2.0f - radiusSqr);
  float sinPhi, cosPhi;
  sincos(phi, sinPhi, cosPhi);

  return float3(sinTheta * sign(st.x) * cosPhi, sinTheta * sign(st.y) * sinPhi, sign(distance) * (1.0f - radiusSqr));
}
#endif