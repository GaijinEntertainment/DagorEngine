#ifndef OCTAHEDRAL_HLSL
#define OCTAHEDRAL_HLSL

// Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square.
float2 octEncode(in float3 n) {
  float l1norm = abs(n.x) + abs(n.y) + abs(n.z);
  float2 result = n.xy * (1.0 / l1norm);
  if ( n.z <= 0 )
    result = (1.0 - abs(result.yx)) * select( n.xy >= 0, float2(1,1), float2(-1,-1) );
  return result;
}


// Returns a unit vector. Argument o is an octahedral vector packed via octEncode
//   on the [-1, +1] square
float3 octDecode(float2 oct) {
  float3 n = float3( oct, 1 - dot( 1, abs(oct) ) );
  float t = max( -n.z, 0 );
  n.xy += select(n.xy >= 0, float2(-t, -t), float2(t, t));
  return normalize(n);
}

/// Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square.
half2 octEncodeh(in half3 n) {
  half l1norm = abs(n.x) + abs(n.y) + abs(n.z);
  half2 result = n.xy * (1.0h / l1norm);
  if ( n.z <= 0 )
    result = (1.0h - abs(result.yx)) * select( n.xy >= 0, half2(1,1), half2(-1,-1) );
  return result;
}


// Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
//    on the [-1, +1] square
half3 octDecodeh(half2 oct) {
  half3 n = half3( oct, 1 - dot( 1, abs(oct) ) );
  half t = max( -n.z, 0 );
  n.xy += select(n.xy >= 0, half2(-t, -t), half2(t, t));
  return normalize(n);
}

#endif
