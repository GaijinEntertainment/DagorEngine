macro USE_FAST_NOISE()

hlsl
{
  #ifndef use_fast_noise
  #define use_fast_noise
  float2 fast_noise2(float2 co)
  {
    float2 cinco = sin(co*12.9898);
    return (frac(cinco * 0.1437585453)+frac(cinco * 4.37585453)*0.3+frac(cinco * 42.7585453)*0.11);
  }

  float fast_noise_val( float2 co, float src, float val )
  {
    BRANCH
    if ( val < 0.99f )
      return ( clamp( dot( fast_noise2( co ), 0.2f ), -0.49, 0.49 ) + val ) * src;
    else
      return src;
  }

  float fast_noise_rad( float2 co )
  {
    float2 offset = co - float2( 0.5, 0.5 );
    float r = dot( offset, offset );
    float sr = sin( r * 6.4949 );
    float rnd = frac( sr * 0.1437585453 ) +
                frac( sr * 42.7585453 ) * 0.11;

    return clamp( rnd, -0.49, 0.49 );
  }

  float fast_noise_rad_val( float2 co, float src, float val )
  {
    BRANCH
    if ( val < 0.99f )
      return saturate( fast_noise_rad( co ) + val ) * src;
    else
      return src;
  }

  float fast_noise_rad3d_val( float3 pos, float inv_rad, float src, float val )
  {
    float r = 1.f - saturate( dot( pos, pos ) * 0.5 * inv_rad );
    float sr = sin( r * 3.2524 );
    float rnd = frac( sr * 0.1437585453 ) + frac( sr * 42.7585453 ) * 0.11;
    float n = clamp( rnd, -0.49, 0.49 );
    return saturate( n + val + abs( dot(frac(1.5 * pos), 1) ) * 0.1 ) * src;
  }

  #endif
}

endmacro

macro USE_SNOISE()
hlsl {
  float4 mod289(float4 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
  }

  float3 mod289(float3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
  }

  float2 mod289(float2 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
  }

  float3 permute(float3 x) {
    return mod289(((x*34.0)+1.0)*x);
  }

  float4 permute(float4 x) {
       return mod289(((x*34.0)+1.0)*x);
  }

  float4 taylorInvSqrt(float4 r)
  {
    return 1.79284291400159 - 0.85373472095314 * r;
  }


  float snoise2(float2 v)
  {
    const float4 C = float4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                        0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                       -0.577350269189626,  // -1.0 + 2.0 * C.x
                        0.024390243902439); // 1.0 / 41.0
  // First corner
    float2 i  = floor(v + dot(v, C.yy) );
    float2 x0 = v -   i + dot(i, C.xx);

  // Other corners
    float2 i1;
    //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
    //i1.y = 1.0 - i1.x;
    i1 = (x0.x > x0.y) ? float2(1.0, 0.0) : float2(0.0, 1.0);
    // x0 = x0 - 0.0 + 0.0 * C.xx ;
    // x1 = x0 - i1 + 1.0 * C.xx ;
    // x2 = x0 - 1.0 + 2.0 * C.xx ;
    float4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;

  // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    float3 p = permute( permute( i.y + float3(0.0, i1.y, 1.0 ))
  		+ i.x + float3(0.0, i1.x, 1.0 ));

    float3 m = max(0.5 - float3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;

  // Gradients: 41 points uniformly over a line, mapped onto a diamond.
  // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

    float3 x = 2.0 * frac(p * C.www) - 1.0;
    float3 h = abs(x) - 0.5;
    float3 ox = floor(x + 0.5);
    float3 a0 = x - ox;

  // Normalise gradients implicitly by scaling m
  // Approximation of: m *= inversesqrt( a0*a0 + h*h );
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

  // Compute final noise value at P
    float3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
  }

  float snoise2_grad(float2 v, out float2 noise_d)
  {
    const float4 C = float4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                        0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                       -0.577350269189626,  // -1.0 + 2.0 * C.x
                        0.024390243902439); // 1.0 / 41.0
  // First corner
    float2 i  = floor(v + dot(v, C.yy) );//const
    float2 x0 = v -   i + dot(i, C.xx);

  // Other corners
    float2 i1;
    //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
    //i1.y = 1.0 - i1.x;
    i1 = (x0.x > x0.y) ? float2(1.0, 0.0) : float2(0.0, 1.0);
    // x0 = x0 - 0.0 + 0.0 * C.xx ;
    // x1 = x0 - i1 + 1.0 * C.xx ;
    // x2 = x0 - 1.0 + 2.0 * C.xx ;
    float4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;

  // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    float3 p = permute( permute( i.y + float3(0.0, i1.y, 1.0 ))
  		+ i.x + float3(0.0, i1.x, 1.0 ));

  // Gradients: 41 points uniformly over a line, mapped onto a diamond.
  // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

    float3 x = 2.0 * frac(p * C.www) - 1.0;
    float3 h = abs(x) - 0.5;
    float3 ox = floor(x + 0.5);
    float3 a0 = x - ox;

    float3 originalM = max(0.5 - float3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    float3 mSq = originalM*originalM ;
    float3 m3 = mSq*originalM;
    float3 m = mSq*mSq ;
  // Normalise gradients implicitly by scaling m
  // Approximation of: m *= inversesqrt( a0*a0 + h*h );
    //m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

  // Compute final noise value at P
    float3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    //g *= rsqrt(a0*a0 + h*h);
    g *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
    //m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

    float3 xxx = float3(x0.x,x12.xz);
    float3 yyy = float3(x0.y,x12.yw);
    float3 g_m3 = -8*g*m3;
    noise_d.x = (dot(g_m3, xxx) + dot(a0, m));
    noise_d.y = (dot(g_m3, yyy) + dot(h, m));
    noise_d *= 130;
    return 130.0 * dot(m, g);
  }

  float snoise3(float3 v)
  { 
    const float2  C = float2(1.0/6.0, 1.0/3.0) ;
    const float4  D = float4(0.0, 0.5, 1.0, 2.0);

  // First corner
    float3 i  = floor(v + dot(v, C.yyy) );
    float3 x0 =   v - i + dot(i, C.xxx) ;

  // Other corners
    float3 g = step(x0.yzx, x0.xyz);
    float3 l = 1.0 - g;
    float3 i1 = min( g.xyz, l.zxy );
    float3 i2 = max( g.xyz, l.zxy );

    //   x0 = x0 - 0.0 + 0.0 * C.xxx;
    //   x1 = x0 - i1  + 1.0 * C.xxx;
    //   x2 = x0 - i2  + 2.0 * C.xxx;
    //   x3 = x0 - 1.0 + 3.0 * C.xxx;
    float3 x1 = x0 - i1 + C.xxx;
    float3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
    float3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

  // Permutations
    i = mod289(i); 
    float4 p = permute( permute( permute( 
               i.z + float4(0.0, i1.z, i2.z, 1.0 ))
             + i.y + float4(0.0, i1.y, i2.y, 1.0 )) 
             + i.x + float4(0.0, i1.x, i2.x, 1.0 ));

  // Gradients: 7x7 points over a square, mapped onto an octahedron.
  // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_ = 0.142857142857; // 1.0/7.0
    float3  ns = n_ * D.wyz - D.xzx;

    float4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

    float4 x_ = floor(j * ns.z);
    float4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

    float4 x = x_ *ns.x + ns.yyyy;
    float4 y = y_ *ns.x + ns.yyyy;
    float4 h = 1.0 - abs(x) - abs(y);

    float4 b0 = float4( x.xy, y.xy );
    float4 b1 = float4( x.zw, y.zw );

    //float4 s0 = float4(lessThan(b0,0.0))*2.0 - 1.0;
    //float4 s1 = float4(lessThan(b1,0.0))*2.0 - 1.0;
    float4 s0 = floor(b0)*2.0 + 1.0;
    float4 s1 = floor(b1)*2.0 + 1.0;
    float4 sh = -step(h, 0.0);

    float4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
    float4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

    float3 p0 = float3(a0.xy,h.x);
    float3 p1 = float3(a0.zw,h.y);
    float3 p2 = float3(a1.xy,h.z);
    float3 p3 = float3(a1.zw,h.w);

  //Normalise gradients
    float4 norm = taylorInvSqrt(float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

  // Mix final noise value
    float4 m = max(0.6 - float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot( m*m, float4( dot(p0,x0), dot(p1,x1), 
                                  dot(p2,x2), dot(p3,x3) ) );
  }
  
  float snoise3_grad(float3 v, out float3 noise_d)
  { 
    const float2  C = float2(1.0/6.0, 1.0/3.0) ;
    const float4  D = float4(0.0, 0.5, 1.0, 2.0);

  // First corner
    float3 i  = floor(v + dot(v, C.yyy) );
    float3 x0 =   v - i + dot(i, C.xxx) ;

  // Other corners
    float3 g = step(x0.yzx, x0.xyz);
    float3 l = 1.0 - g;
    float3 i1 = min( g.xyz, l.zxy );
    float3 i2 = max( g.xyz, l.zxy );

    //   x0 = x0 - 0.0 + 0.0 * C.xxx;
    //   x1 = x0 - i1  + 1.0 * C.xxx;
    //   x2 = x0 - i2  + 2.0 * C.xxx;
    //   x3 = x0 - 1.0 + 3.0 * C.xxx;
    float3 x1 = x0 - i1 + C.xxx;
    float3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
    float3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

  // Permutations
    i = mod289(i); 
    float4 p = permute( permute( permute( 
               i.z + float4(0.0, i1.z, i2.z, 1.0 ))
             + i.y + float4(0.0, i1.y, i2.y, 1.0 )) 
             + i.x + float4(0.0, i1.x, i2.x, 1.0 ));

  // Gradients: 7x7 points over a square, mapped onto an octahedron.
  // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_ = 0.142857142857; // 1.0/7.0
    float3  ns = n_ * D.wyz - D.xzx;

    float4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

    float4 x_ = floor(j * ns.z);
    float4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

    float4 x = x_ *ns.x + ns.yyyy;
    float4 y = y_ *ns.x + ns.yyyy;
    float4 h = 1.0 - abs(x) - abs(y);

    float4 b0 = float4( x.xy, y.xy );
    float4 b1 = float4( x.zw, y.zw );

    //float4 s0 = float4(lessThan(b0,0.0))*2.0 - 1.0;
    //float4 s1 = float4(lessThan(b1,0.0))*2.0 - 1.0;
    float4 s0 = floor(b0)*2.0 + 1.0;
    float4 s1 = floor(b1)*2.0 + 1.0;
    float4 sh = -step(h, 0.0);

    float4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
    float4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

    float3 p0 = float3(a0.xy,h.x);
    float3 p1 = float3(a0.zw,h.y);
    float3 p2 = float3(a1.xy,h.z);
    float3 p3 = float3(a1.zw,h.w);

  //Normalise gradients
    float4 norm = taylorInvSqrt(float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

  // Mix final noise value
    float4 originalM = max(0.6 - float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    float4 mSq = originalM*originalM ;
    float4 m3 = mSq*originalM;
    float4 m = mSq*mSq ;
    float4 gradients = float4( dot(p0,x0), dot(p1,x1), 
                               dot(p2,x2), dot(p3,x3));

    float4 g_m3 = -8*m3*gradients;
    noise_d = g_m3.x*x0 + g_m3.y*x1 + g_m3.z*x2 + g_m3.w*x3 + p0*m.x  + p1*m.y  + p2*m.z  + p3*m.w;

    //noise_d.x = (dot(g_m3, float4(x0.x,x1.x, x2.x, x3.x)) + dot(float4(p0.x, p1.x, p2.x, p3.x), m));
    //noise_d.y = (dot(g_m3, float4(x0.y,x1.y, x2.y, x3.y)) + dot(float4(p0.y, p1.y, p2.y, p3.y), m));
    //noise_d.z = (dot(g_m3, float4(x0.z,x1.z, x2.z, x3.z)) + dot(float4(p0.z, p1.z, p2.z, p3.z), m));
    return 42.0 * dot( m,  gradients);
  }

}
endmacro