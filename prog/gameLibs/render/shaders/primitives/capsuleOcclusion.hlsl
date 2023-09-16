#ifndef CAPSULE_OCCLUSION_HLSL_INCLUDED
#define CAPSULE_OCCLUSION_HLSL_INCLUDED 1
//based on
  //https://www.shadertoy.com/view/llGyzG
  float capOcclusionSquared( in float3 p, in float3 n, in float3 a, in float3 b, in float r )
  {
    // closest sphere
    float3  ba = b - a, pa = p - a;
    float h = saturate(dot(pa,ba)/dot(ba,ba));
    float3  d = pa - h*ba;
    float l = length(d);
    float o = 1.0 - max(0.0,dot(-d,n))*r*r/(l*l*l);
    return max(o*o*o, 0);
  }

  float capOcclusionOpacity( in float3 p, in float3 n, in float3 a, in float3 b, in float r, in float opacity = 0.5 )
  {
    // closest sphere
    float3  ba = b - a, pa = p - a;
    float h = saturate(dot(pa,ba)/dot(ba,ba));
    float3  d = pa - h*ba;
    float l = length(d);
    float o = 1.0 - max(0.0,dot(-d,n))*r*r/(l*l*l);
    float sd = l-r;
    //return sd<0 ? sd/-r :0;
    float invOpacity = (1-opacity);
    FLATTEN
    if (o<=0)
      return invOpacity*(saturate(1 + sd/r));

    return invOpacity + sqrt(o*o*o)*opacity;
  }

  //signed distance field with capsule
  float sdCapsule( float3 p, float3 a, float3 b, float r )
  {
    float3 pa = p-a, ba = b-a;
    float h = saturate(dot(pa,ba)/dot(ba,ba));
    return length( pa - ba*h ) - r;
  }

  //capsule normal
  float3 capNormal( in float3 pos, in float3 a, in float3 b, in float r )
  {
    float3  ba = b - a;
    float3  pa = pos - a;
    float h = saturate(dot(pa,ba)/dot(ba,ba));
    return (pa - h*ba)/r;
  }

  //intersect capsule with ray
  float capIntersect( in float3 ro, in float3 rd, in float3 pa, in float3 pb, in float r )
  {
    float3  ba = pb - pa;
    float3  oa = ro - pa;

    float baba = dot(ba,ba);
    float bard = dot(ba,rd);
    float baoa = dot(ba,oa);
    float rdoa = dot(rd,oa);
    float oaoa = dot(oa,oa);

    float a = baba      - bard*bard;
    float b = baba*rdoa - baoa*bard;
    float c = baba*oaoa - baoa*baoa - r*r*baba;
    float h = b*b - a*c;
    FLATTEN
    if( h>=0.0 )
    {
      float t = (-b-sqrt(h))/a;

      float y = baoa + t*bard;

      // body
      if( y>0.0 && y<baba ) return t;

      // caps
      float3 oc = (y<=0.0) ? oa : ro - pb;
      b = dot(rd,oc);
      c = dot(oc,oc) - r*r;
      h = b*b - c;
      if( h>0.0 )
      {
        return -b - sqrt(h);
      }
    }
    return -1.0;
  }

#endif