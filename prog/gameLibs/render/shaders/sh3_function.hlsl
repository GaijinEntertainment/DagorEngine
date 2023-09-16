#ifndef SH3_FUNCTION_HLSL_INCLUDED
#define SH3_FUNCTION_HLSL_INCLUDED 1

  half3 GetSHFunctionValue(half3 normal, float4 c0, float4 c1, float4 c2, float4 c3, float4 c4, float4 c5, float4 c6)
  {
    half4 normal1 = half4(normal, 1.h);
    half3 intermediate0, intermediate1, intermediate2;

    //sph.w - constant frequency band 0, sph.xyz - linear frequency band 1
    intermediate0.x = dot(half4(c0), normal1);
    intermediate0.y = dot(half4(c1), normal1);
    intermediate0.z = dot(half4(c2), normal1);

    //sph.xyzw and sph6 - quadratic polynomials frequency band 2
    half4 r1 = normal.xyxz * normal.zzyz;
    r1.w = 3.0h * r1.w - 1.h;
    intermediate1.x = dot(half4(c3), r1);
    intermediate1.y = dot(half4(c4), r1);
    intermediate1.z = dot(half4(c5), r1);

    half r2 = normal.x * normal.x - normal.y * normal.y;
    intermediate2 = half3(c6.xyz) * r2;

    //Attention! Intrinsic 'max' are specialized by the first argument (or with less component count)
    //on OpenGL therefore the constant 0 should be set as half3 (or as second argument)
    return max(intermediate0 + intermediate1 + intermediate2, half3(0.h, 0.h, 0.h));
  }
  half3 GetSHFunctionValueSimple(half3 normal, float4 c0, float4 c1, float4 c2)
  {
    half4 normal1 = half4(normal, 1.h);
    //sph.w - constant frequency band 0, sph.xyz - linear frequency band 1
    half3 intermediate0;
    intermediate0.x = dot(half4(c0), normal1);
    intermediate0.y = dot(half4(c1), normal1);
    intermediate0.z = dot(half4(c2), normal1);
    return max(intermediate0, half3(0.h, 0.h, 0.h));
  }

#endif
