#ifndef G3D_octahedral_hlsl
#define G3D_octahedral_hlsl

/** Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square. */
float2 octEncode(in float3 n) {
  float l1norm = abs(n.x) + abs(n.y) + abs(n.z);
  float2 result = n.xy * (1.0 / l1norm);
  if ( n.z <= 0 )
    result = (1.0 - abs(result.yx)) * ( n.xy >= 0 ? float2(1,1) : float2(-1,-1) );
  return result;
}


/** Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
    on the [-1, +1] square*/
float3 octDecode(float2 oct) {
  float3 n = float3( oct, 1 - dot( 1, abs(oct) ) );
  float t = max( -n.z, 0 );
  n.xy += n.xy >= 0 ? float2(-t, -t) : float2(t, t);
  return normalize(n);
}

#endif
