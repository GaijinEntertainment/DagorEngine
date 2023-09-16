#ifndef SSGI_INTEGRATE_AMBIENT_CUBE
#define SSGI_INTEGRATE_AMBIENT_CUBE 1

  void integrate_cube(float3 lightDir, float3 lightColor, inout float3 col0, inout float3 col1, inout float3 col2, inout float3 col3, inout float3 col4, inout float3 col5)
  {
    float3 posDir = saturate(lightDir);
    float3 negDir = saturate(-lightDir);

    col0 += lightColor * posDir.x;
    col1 += lightColor * negDir.x;
    col2 += lightColor * posDir.y;
    col3 += lightColor * negDir.y;
    col4 += lightColor * posDir.z;
    col5 += lightColor * negDir.z;
  }
  void integrate_cube_alpha(float3 lightDir, float4 lightColor, inout float4 col0, inout float4 col1, inout float4 col2, inout float4 col3, inout float4 col4, inout float4 col5)
  {
    float3 posDir = saturate(lightDir);
    float3 negDir = saturate(-lightDir);

    col0 += lightColor * posDir.x;
    col1 += lightColor * negDir.x;
    col2 += lightColor * posDir.y;
    col3 += lightColor * negDir.y;
    col4 += lightColor * posDir.z;
    col5 += lightColor * negDir.z;
  }

  void integrate_cube_scalar(float3 lightDir, float lightAmount, inout float col0, inout float col1, inout float col2, inout float col3, inout float col4, inout float col5)
  {
    float3 posDir = saturate(lightDir);
    float3 negDir = saturate(-lightDir);

    col0 += lightAmount * posDir.x;
    col1 += lightAmount * negDir.x;
    col2 += lightAmount * posDir.y;
    col3 += lightAmount * negDir.y;
    col4 += lightAmount * posDir.z;
    col5 += lightAmount * negDir.z;
  }

#endif