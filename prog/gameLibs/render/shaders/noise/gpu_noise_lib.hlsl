//
//	Code repository for GPU noise development blog
//	http://briansharpe.wordpress.com
//	https://github.com/BrianSharpe
//
//	I'm not one for copyrights.  Use the code however you wish.
//	All I ask is that credit be given back to the blog or myself when appropriate.
//	And also to let me know if you come up with any changes, improvements, thoughts or interesting uses for this stuff. :)
//	Thanks!
//
//	Brian Sharpe
//	brisharpe CIRCLE_A yahoo DOT com
//	http://briansharpe.wordpress.com
//	https://github.com/BrianSharpe
//


//
//	NoiseLib TODO
//
//	1) Ensure portability across different cards
//	2) 16bit and 24bit implementations of hashes and noises
//	3) Lift various noise implementations out to individual self-contained files
//	4) Implement texture-based versions
//


//
//	Permutation polynomial idea is from Stefan Gustavson's and Ian McEwan's work at...
//	http://github.com/ashima/webgl-noise
//	http://www.itn.liu.se/~stegu/GLSL-cellular
//
//	http://briansharpe.wordpress.com/2011/10/01/gpu-texture-free-noise/
//
float4 SGPP_coord_prepare(float4 x) { return x - floor(x * ( 1.0 / 289.0 )) * 289.0; }
float3 SGPP_coord_prepare(float3 x) { return x - floor(x * ( 1.0 / 289.0 )) * 289.0; }
float4 SGPP_permute(float4 x) { return frac( x * ( ( 34.0 / 289.0 ) * x + ( 1.0 / 289.0 ) ) ) * 289.0; }
float4 SGPP_resolve(float4 x) { return frac( x * ( 7.0 / 288.0 ) ); }
float4 SGPP_hash_2D( float2 gridcell )		//	generates a random number for each of the 4 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    float4 hash_coord = SGPP_coord_prepare( float4( gridcell.xy, gridcell.xy + 1.0 ) );
    return SGPP_resolve( SGPP_permute( SGPP_permute( hash_coord.xzxz ) + hash_coord.yyww ) );
}
void SGPP_hash_2D( float2 gridcell, out float4 hash_0, out float4 hash_1 )	//	generates 2 random numbers for each of the 4 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    float4 hash_coord = SGPP_coord_prepare( float4( gridcell.xy, gridcell.xy + 1.0 ) );
    hash_0 = SGPP_permute( SGPP_permute( hash_coord.xzxz ) + hash_coord.yyww );
    hash_1 = SGPP_resolve( SGPP_permute( hash_0 ) );
    hash_0 = SGPP_resolve( hash_0 );
}
void SGPP_hash_3D( float3 gridcell, out float4 lowz_hash, out float4 highz_hash )		//	generates a random number for each of the 8 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    gridcell = SGPP_coord_prepare( gridcell );
    float3 gridcell_inc1 = step( gridcell, float( 287.5 ).xxx ) * ( gridcell + 1.0 );
    highz_hash = SGPP_permute( SGPP_permute( float2( gridcell.x, gridcell_inc1.x ).xyxy ) + float2( gridcell.y, gridcell_inc1.y ).xxyy );
    lowz_hash = SGPP_resolve( SGPP_permute( highz_hash + gridcell.zzzz ) );
    highz_hash = SGPP_resolve( SGPP_permute( highz_hash + gridcell_inc1.zzzz ) );
}
void SGPP_hash_3D( 	float3 gridcell,
                    float3 v1_mask,		//	user definable v1 and v2.  ( 0's and 1's )
                    float3 v2_mask,
                    out float4 hash_0,
                    out float4 hash_1,
                    out float4 hash_2	)		//	generates 3 random numbers for each of the 4 3D cell corners.  cell corners:  v0=0,0,0  v3=1,1,1  the other two are user definable
{
    float3 coords0 = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / 289.0 )) * 289.0;
    float3 coords3 = step( coords0, float( 287.5 ).xxx ) * ( coords0 + 1.0 );
    float3 coords1 = lerp( coords0, coords3, v1_mask );
    float3 coords2 = lerp( coords0, coords3, v2_mask );
    hash_2 = SGPP_permute( SGPP_permute( SGPP_permute( float4( coords0.x, coords1.x, coords2.x, coords3.x ) ) + float4( coords0.y, coords1.y, coords2.y, coords3.y ) ) + float4( coords0.z, coords1.z, coords2.z, coords3.z ) );
    hash_0 = SGPP_resolve( hash_2  );
    hash_1 = SGPP_resolve( hash_2 = SGPP_permute( hash_2 ) );
    hash_2 = SGPP_resolve( SGPP_permute( hash_2 ) );
}
void SGPP_hash_3D( 	float3 gridcell,
                    out float4 lowz_hash_0,
                    out float4 lowz_hash_1,
                    out float4 lowz_hash_2,
                    out float4 highz_hash_0,
                    out float4 highz_hash_1,
                    out float4 highz_hash_2	)	//	generates 3 random numbers for each of the 8 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    gridcell = SGPP_coord_prepare( gridcell );
    float3 gridcell_inc1 = step( gridcell, float( 287.5 ).xxx ) * ( gridcell + 1.0 );
    highz_hash_2 = SGPP_permute( SGPP_permute( float2( gridcell.x, gridcell_inc1.x ).xyxy ) + float2( gridcell.y, gridcell_inc1.y ).xxyy );
    lowz_hash_0 = SGPP_resolve( lowz_hash_2 = SGPP_permute( highz_hash_2 + gridcell.zzzz ) );
    highz_hash_0 = SGPP_resolve( highz_hash_2 = SGPP_permute( highz_hash_2 + gridcell_inc1.zzzz ) );
    lowz_hash_1 = SGPP_resolve( lowz_hash_2 = SGPP_permute( lowz_hash_2 ) );
    highz_hash_1 = SGPP_resolve( highz_hash_2 = SGPP_permute( highz_hash_2 ) );
    lowz_hash_2 = SGPP_resolve( SGPP_permute( lowz_hash_2 ) );
    highz_hash_2 = SGPP_resolve( SGPP_permute( highz_hash_2 ) );
}


//
//	implementation of the blumblumshub hash
//	as described in MNoise paper http://www.cs.umbc.edu/~olano/papers/mNoise.pdf
//
//	http://briansharpe.wordpress.com/2011/10/01/gpu-texture-free-noise/
//
float4 BBS_coord_prepare(float4 x) { return x - floor(x * ( 1.0 / 61.0 )) * 61.0; }
float3 BBS_coord_prepare(float3 x) { return x - floor(x * ( 1.0 / 61.0 )) * 61.0; }
float4 BBS_permute(float4 x) { return frac( x * x * ( 1.0 / 61.0 )) * 61.0; }
float4 BBS_permute_and_resolve(float4 x) { return frac( x * x * ( 1.0 / 61.0 ) ); }
float4 BBS_hash_2D( float2 gridcell )	//	generates a random number for each of the 4 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    float4 hash_coord = BBS_coord_prepare( float4( gridcell.xy, gridcell.xy + 1.0 ) );
    float4 p = BBS_permute( hash_coord.xzxz /* * 7.0 */ ); // * 7.0 will increase variance close to origin
    return BBS_permute_and_resolve( p + hash_coord.yyww );
}
float4 BBS_hash_hq_2D( float2 gridcell )	//	generates a hq random number for each of the 4 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    float4 hash_coord = BBS_coord_prepare( float4( gridcell.xy, gridcell.xy + 1.0 ) );
    float4 p = BBS_permute( hash_coord.xzxz /* * 7.0 */ );  // * 7.0 will increase variance close to origin
    p = BBS_permute( p + hash_coord.yyww );
    return BBS_permute_and_resolve( p + hash_coord.xzxz );
}
void BBS_hash_3D( float3 gridcell, out float4 lowz_hash, out float4 highz_hash )		//	generates a random number for each of the 8 cell corners
{
    //	gridcell is assumed to be an integer coordinate

    //	was having precision issues here with 61.0  60.0 fixes it.  need to test on other cards.
    const float DOMAIN = 60.0;
    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / DOMAIN )) * DOMAIN;
    float3 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxx ) * ( gridcell + 1.0 );

    float4 p = BBS_permute( float2( gridcell.x, gridcell_inc1.x ).xyxy /* * 7.0 */ );  // * 7.0 will increase variance close to origin
    p = BBS_permute( p + float2( gridcell.y, gridcell_inc1.y ).xxyy );
    lowz_hash = BBS_permute_and_resolve( p + gridcell.zzzz );
    highz_hash = BBS_permute_and_resolve( p + gridcell_inc1.zzzz );
}


//
//	FAST32_hash
//	A very fast hashing function.  Requires 32bit support.
//	http://briansharpe.wordpress.com/2011/11/15/a-fast-and-simple-32bit-floating-point-hash-function/
//
//	The 2D hash formula takes the form....
//	hash = mod( coord.x * coord.x * coord.y * coord.y, SOMELARGEFLOAT ) / SOMELARGEFLOAT
//	We truncate and offset the domain to the most interesting part of the noise.
//	SOMELARGEFLOAT should be in the range of 400.0->1000.0 and needs to be hand picked.  Only some give good results.
//	A 3D hash is achieved by offsetting the SOMELARGEFLOAT value by the Z coordinate
//
float4 FAST32_hash_2D( float2 gridcell )	//	generates a random number for each of the 4 cell corners
{
    //	gridcell is assumed to be an integer coordinate
    const float2 OFFSET = float2( 26.0, 161.0 );
    const float DOMAIN = 71.0;
    const float SOMELARGEFLOAT = 951.135664;
    float4 P = float4( gridcell.xy, gridcell.xy + 1.0 );
    P = P - floor(P * ( 1.0 / DOMAIN )) * DOMAIN;	//	truncate the domain
    P += OFFSET.xyxy;								//	offset to interesting part of the noise
    P *= P;											//	calculate and return the hash
    return frac( P.xzxz * P.yyww * ( 1.0 / SOMELARGEFLOAT ) );
}
void FAST32_hash_2D( float2 gridcell, out float4 hash_0, out float4 hash_1 )	//	generates 2 random numbers for each of the 4 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    const float2 OFFSET = float2( 26.0, 161.0 );
    const float DOMAIN = 71.0;
    const float2 SOMELARGEFLOATS = float2( 951.135664, 642.949883 );
    float4 P = float4( gridcell.xy, gridcell.xy + 1.0 );
    P = P - floor(P * ( 1.0 / DOMAIN )) * DOMAIN;
    P += OFFSET.xyxy;
    P *= P;
    P = P.xzxz * P.yyww;
    hash_0 = frac( P * ( 1.0 / SOMELARGEFLOATS.x ) );
    hash_1 = frac( P * ( 1.0 / SOMELARGEFLOATS.y ) );
}
void FAST32_hash_2D( 	float2 gridcell,
                        out float4 hash_0,
                        out float4 hash_1,
                        out float4 hash_2	)	//	generates 3 random numbers for each of the 4 cell corners
{
    //    gridcell is assumed to be an integer coordinate
    const float2 OFFSET = float2( 26.0, 161.0 );
    const float DOMAIN = 71.0;
    const float3 SOMELARGEFLOATS = float3( 951.135664, 642.949883, 803.202459 );
    float4 P = float4( gridcell.xy, gridcell.xy + 1.0 );
    P = P - floor(P * ( 1.0 / DOMAIN )) * DOMAIN;
    P += OFFSET.xyxy;
    P *= P;
    P = P.xzxz * P.yyww;
    hash_0 = frac( P * ( 1.0 / SOMELARGEFLOATS.x ) );
    hash_1 = frac( P * ( 1.0 / SOMELARGEFLOATS.y ) );
    hash_2 = frac( P * ( 1.0 / SOMELARGEFLOATS.z ) );
}
float4 FAST32_hash_2D_Cell( float2 gridcell )	//	generates 4 different random numbers for the single given cell point
{
    //	gridcell is assumed to be an integer coordinate
    const float2 OFFSET = float2( 26.0, 161.0 );
    const float DOMAIN = 71.0;
    const float4 SOMELARGEFLOATS = float4( 951.135664, 642.949883, 803.202459, 986.973274 );
    float2 P = gridcell - floor(gridcell * ( 1.0 / DOMAIN )) * DOMAIN;
    P += OFFSET.xy;
    P *= P;
    return frac( (P.x * P.y) * ( 1.0 / SOMELARGEFLOATS.xyzw ) );
}
float4 FAST32_hash_3D_Cell( float3 gridcell )	//	generates 4 different random numbers for the single given cell point
{
    //    gridcell is assumed to be an integer coordinate

    //	TODO: 	these constants need tweaked to find the best possible noise.
    //			probably requires some kind of brute force computational searching or something....
    const float2 OFFSET = float2( 50.0, 161.0 );
    const float DOMAIN = 69.0;
    const float4 SOMELARGEFLOATS = float4( 635.298681, 682.357502, 668.926525, 588.255119 );
    const float4 ZINC = float4( 48.500388, 65.294118, 63.934599, 63.279683 );

    //	truncate the domain
    gridcell.xyz = gridcell - floor(gridcell * ( 1.0 / DOMAIN )) * DOMAIN;
    gridcell.xy += OFFSET.xy;
    gridcell.xy *= gridcell.xy;
    return frac( ( gridcell.x * gridcell.y ) * ( 1.0 / ( SOMELARGEFLOATS + gridcell.zzzz * ZINC ) ) );
}
void FAST32_hash_3D( float3 gridcell, out float4 lowz_hash, out float4 highz_hash )	//	generates a random number for each of the 8 cell corners
{
    //    gridcell is assumed to be an integer coordinate

    //	TODO: 	these constants need tweaked to find the best possible noise.
    //			probably requires some kind of brute force computational searching or something....
    const float2 OFFSET = float2( 50.0, 161.0 );
    const float DOMAIN = 69.0;
    const float SOMELARGEFLOAT = 635.298681;
    const float ZINC = 48.500388;

    //	truncate the domain
    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / DOMAIN )) * DOMAIN;
    float3 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxx ) * ( gridcell + 1.0 );

    //	calculate the noise
    float4 P = float4( gridcell.xy, gridcell_inc1.xy ) + OFFSET.xyxy;
    P *= P;
    P = P.xzxz * P.yyww;
    highz_hash.xy = float2( 1.0 / ( SOMELARGEFLOAT + float2( gridcell.z, gridcell_inc1.z ) * ZINC ) );
    lowz_hash = frac( P * highz_hash.xxxx );
    highz_hash = frac( P * highz_hash.yyyy );
}
void FAST32_hash_3D( 	float3 gridcell,
                        float3 v1_mask,		//	user definable v1 and v2.  ( 0's and 1's )
                        float3 v2_mask,
                        out float4 hash_0,
                        out float4 hash_1,
                        out float4 hash_2	)		//	generates 3 random numbers for each of the 4 3D cell corners.  cell corners:  v0=0,0,0  v3=1,1,1  the other two are user definable
{
    //    gridcell is assumed to be an integer coordinate

    //	TODO: 	these constants need tweaked to find the best possible noise.
    //			probably requires some kind of brute force computational searching or something....
    const float2 OFFSET = float2( 50.0, 161.0 );
    const float DOMAIN = 69.0;
    const float3 SOMELARGEFLOATS = float3( 635.298681, 682.357502, 668.926525 );
    const float3 ZINC = float3( 48.500388, 65.294118, 63.934599 );

    //	truncate the domain
    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / DOMAIN )) * DOMAIN;
    float3 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxx ) * ( gridcell + 1.0 );

    //	compute x*x*y*y for the 4 corners
    float4 P = float4( gridcell.xy, gridcell_inc1.xy ) + OFFSET.xyxy;
    P *= P;
    float4 V1xy_V2xy = lerp( P.xyxy, P.zwzw, float4( v1_mask.xy, v2_mask.xy ) );		//	apply mask for v1 and v2
    P = float4( P.x, V1xy_V2xy.xz, P.z ) * float4( P.y, V1xy_V2xy.yw, P.w );

    //	get the lowz and highz mods
    float3 lowz_mods = float3( 1.0 / ( SOMELARGEFLOATS.xyz + gridcell.zzz * ZINC.xyz ) );
    float3 highz_mods = float3( 1.0 / ( SOMELARGEFLOATS.xyz + gridcell_inc1.zzz * ZINC.xyz ) );

    //	apply mask for v1 and v2 mod values
    v1_mask = ( v1_mask.z < 0.5 ) ? lowz_mods : highz_mods;
    v2_mask = ( v2_mask.z < 0.5 ) ? lowz_mods : highz_mods;

    //	compute the final hash
    hash_0 = frac( P * float4( lowz_mods.x, v1_mask.x, v2_mask.x, highz_mods.x ) );
    hash_1 = frac( P * float4( lowz_mods.y, v1_mask.y, v2_mask.y, highz_mods.y ) );
    hash_2 = frac( P * float4( lowz_mods.z, v1_mask.z, v2_mask.z, highz_mods.z ) );
}
float4 FAST32_hash_3D( 	float3 gridcell,
                        float3 v1_mask,		//	user definable v1 and v2.  ( 0's and 1's )
                        float3 v2_mask )		//	generates 1 random number for each of the 4 3D cell corners.  cell corners:  v0=0,0,0  v3=1,1,1  the other two are user definable
{
    //    gridcell is assumed to be an integer coordinate

    //	TODO: 	these constants need tweaked to find the best possible noise.
    //			probably requires some kind of brute force computational searching or something....
    const float2 OFFSET = float2( 50.0, 161.0 );
    const float DOMAIN = 69.0;
    const float SOMELARGEFLOAT = 635.298681;
    const float ZINC = 48.500388;

    //	truncate the domain
    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / DOMAIN )) * DOMAIN;
    float3 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxx ) * ( gridcell + 1.0 );

    //	compute x*x*y*y for the 4 corners
    float4 P = float4( gridcell.xy, gridcell_inc1.xy ) + OFFSET.xyxy;
    P *= P;
    float4 V1xy_V2xy = lerp( P.xyxy, P.zwzw, float4( v1_mask.xy, v2_mask.xy ) );		//	apply mask for v1 and v2
    P = float4( P.x, V1xy_V2xy.xz, P.z ) * float4( P.y, V1xy_V2xy.yw, P.w );

    //	get the z mod vals
    float2 V1z_V2z = float2( v1_mask.z < 0.5 ? gridcell.z : gridcell_inc1.z, v2_mask.z < 0.5 ? gridcell.z : gridcell_inc1.z );
    float4 mod_vals = float4( 1.0 / ( SOMELARGEFLOAT + float4( gridcell.z, V1z_V2z, gridcell_inc1.z ) * ZINC ) );

    //	compute the final hash
    return frac( P * mod_vals );
}
void FAST32_hash_3D( 	float3 gridcell,
                        out float4 lowz_hash_0,
                        out float4 lowz_hash_1,
                        out float4 lowz_hash_2,
                        out float4 highz_hash_0,
                        out float4 highz_hash_1,
                        out float4 highz_hash_2	)		//	generates 3 random numbers for each of the 8 cell corners
{
    //    gridcell is assumed to be an integer coordinate

    //	TODO: 	these constants need tweaked to find the best possible noise.
    //			probably requires some kind of brute force computational searching or something....
    const float2 OFFSET = float2( 50.0, 161.0 );
    const float DOMAIN = 69.0;
    const float3 SOMELARGEFLOATS = float3( 635.298681, 682.357502, 668.926525 );
    const float3 ZINC = float3( 48.500388, 65.294118, 63.934599 );

    //	truncate the domain
    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / DOMAIN )) * DOMAIN;
    float3 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxx ) * ( gridcell + 1.0 );

    //	calculate the noise
    float4 P = float4( gridcell.xy, gridcell_inc1.xy ) + OFFSET.xyxy;
    P *= P;
    P = P.xzxz * P.yyww;
    float3 lowz_mod = float3( 1.0 / ( SOMELARGEFLOATS.xyz + gridcell.zzz * ZINC.xyz ) );
    float3 highz_mod = float3( 1.0 / ( SOMELARGEFLOATS.xyz + gridcell_inc1.zzz * ZINC.xyz ) );
    lowz_hash_0 = frac( P * lowz_mod.xxxx );
    highz_hash_0 = frac( P * highz_mod.xxxx );
    lowz_hash_1 = frac( P * lowz_mod.yyyy );
    highz_hash_1 = frac( P * highz_mod.yyyy );
    lowz_hash_2 = frac( P * lowz_mod.zzzz );
    highz_hash_2 = frac( P * highz_mod.zzzz );
}
void FAST32_hash_3D( 	float3 gridcell,
                        out float4 lowz_hash_0,
                        out float4 lowz_hash_1,
                        out float4 lowz_hash_2,
                        out float4 lowz_hash_3,
                        out float4 highz_hash_0,
                        out float4 highz_hash_1,
                        out float4 highz_hash_2,
                        out float4 highz_hash_3	)		//	generates 4 random numbers for each of the 8 cell corners
{
    //    gridcell is assumed to be an integer coordinate

    //	TODO: 	these constants need tweaked to find the best possible noise.
    //			probably requires some kind of brute force computational searching or something....
    const float2 OFFSET = float2( 50.0, 161.0 );
    const float DOMAIN = 69.0;
    const float4 SOMELARGEFLOATS = float4( 635.298681, 682.357502, 668.926525, 588.255119 );
    const float4 ZINC = float4( 48.500388, 65.294118, 63.934599, 63.279683 );

    //	truncate the domain
    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / DOMAIN )) * DOMAIN;
    float3 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxx ) * ( gridcell + 1.0 );

    //	calculate the noise
    float4 P = float4( gridcell.xy, gridcell_inc1.xy ) + OFFSET.xyxy;
    P *= P;
    P = P.xzxz * P.yyww;
    lowz_hash_3.xyzw = float4( 1.0 / ( SOMELARGEFLOATS.xyzw + gridcell.zzzz * ZINC.xyzw ) );
    highz_hash_3.xyzw = float4( 1.0 / ( SOMELARGEFLOATS.xyzw + gridcell_inc1.zzzz * ZINC.xyzw ) );
    lowz_hash_0 = frac( P * lowz_hash_3.xxxx );
    highz_hash_0 = frac( P * highz_hash_3.xxxx );
    lowz_hash_1 = frac( P * lowz_hash_3.yyyy );
    highz_hash_1 = frac( P * highz_hash_3.yyyy );
    lowz_hash_2 = frac( P * lowz_hash_3.zzzz );
    highz_hash_2 = frac( P * highz_hash_3.zzzz );
    lowz_hash_3 = frac( P * lowz_hash_3.wwww );
    highz_hash_3 = frac( P * highz_hash_3.wwww );
}


//
//	FastHash32_2
//
//	An alternative to FastHash32
//	- slightly slower
//	- can have a larger domain
//	- allows for a 4D implementation
//
//	(eg)4D is computed like so....
//	coord = mod( coord, DOMAIN );
//	coord = ( coord * NOISE_SCALE ) + OFFSET;
//	coord *= coord;
//	hash = mod( coord.x * coord.y * coord.z * coord.w, SOMELARGEFLOAT ) / SOMELARGEFLOAT;
//
float4 FAST32_2_hash_2D( float2 gridcell )	//	generates a random number for each of the 4 cell corners
{
    //	gridcell is assumed to be an integer coordinate
    const float2 OFFSET = float2( 403.839172, 377.242706 );
    const float DOMAIN = 69.0;		//	NOTE:  this can most likely be extended with some tweaking of the other parameters
    const float SOMELARGEFLOAT = 32745.708984;
    const float2 NOISE_SCALE = float2( 2.009842, 1.372549 );

    float4 P = float4( gridcell.xy, gridcell.xy + 1.0 );
    P = P - floor(P * ( 1.0 / DOMAIN )) * DOMAIN;
    P = ( P * NOISE_SCALE.xyxy ) + OFFSET.xyxy;
    P *= P;
    return frac( P.xzxz * P.yyww * ( 1.0 / SOMELARGEFLOAT ) );
}
void FAST32_2_hash_3D( 	float3 gridcell,
                        out float4 z0_hash,			//  float4 == ( x0y0, x1y0, x0y1, x1y1 )
                        out float4 z1_hash	)		//	generates a random number for each of the 8 cell corners
{
    //	gridcell is assumed to be an integer coordinate
    const float3 OFFSET = float3( 55.882355, 63.167774, 52.941177 );
    const float DOMAIN = 69.0;		//	NOTE:  this can most likely be extended with some tweaking of the other parameters
    const float SOMELARGEFLOAT = 69412.070313;
    const float3 NOISE_SCALE = float3( 0.235142, 0.205890, 0.216449 );

    //	truncate the domain
    gridcell = gridcell - floor(gridcell * ( 1.0 / DOMAIN )) * DOMAIN;
    float3 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxx ) * ( gridcell + 1.0 );

    //	calculate the noise
    gridcell = ( gridcell * NOISE_SCALE ) + OFFSET;
    gridcell_inc1 = ( gridcell_inc1 * NOISE_SCALE ) + OFFSET;
    gridcell *= gridcell;
    gridcell_inc1 *= gridcell_inc1;

    float4 x0y0_x1y0_x0y1_x1y1 = float4( gridcell.x, gridcell_inc1.x, gridcell.x, gridcell_inc1.x ) * float4( gridcell.yy, gridcell_inc1.yy );

    z0_hash = frac( x0y0_x1y0_x0y1_x1y1 * gridcell.zzzz * ( 1.0 / SOMELARGEFLOAT ) );
    z1_hash = frac( x0y0_x1y0_x0y1_x1y1 * gridcell_inc1.zzzz * ( 1.0 / SOMELARGEFLOAT ) );
}
void FAST32_2_hash_4D( 	float4 gridcell,
                        out float4 z0w0_hash,		//  float4 == ( x0y0, x1y0, x0y1, x1y1 )
                        out float4 z1w0_hash,
                        out float4 z0w1_hash,
                        out float4 z1w1_hash	)
{
    //    gridcell is assumed to be an integer coordinate

    //	TODO: 	these constants need tweaked to find the best possible noise.
    //			probably requires some kind of brute force computational searching or something....
    const float4 OFFSET = float4( 16.841230, 18.774548, 16.873274, 13.664607 );
    const float DOMAIN = 69.0;
    const float SOMELARGEFLOAT = 47165.636719;
    const float4 NOISE_SCALE = float4( 0.102007, 0.114473, 0.139651, 0.084550 );

    //	truncate the domain
    gridcell = gridcell - floor(gridcell * ( 1.0 / DOMAIN )) * DOMAIN;
    float4 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxxx ) * ( gridcell + 1.0 );

    //	calculate the noise
    gridcell = ( gridcell * NOISE_SCALE ) + OFFSET;
    gridcell_inc1 = ( gridcell_inc1 * NOISE_SCALE ) + OFFSET;
    gridcell *= gridcell;
    gridcell_inc1 *= gridcell_inc1;

    float4 x0y0_x1y0_x0y1_x1y1 = float4( gridcell.x, gridcell_inc1.x, gridcell.x, gridcell_inc1.x ) * float4( gridcell.yy, gridcell_inc1.yy );
    float4 z0w0_z1w0_z0w1_z1w1 = float4( gridcell.z, gridcell_inc1.z, gridcell.z, gridcell_inc1.z ) * float4( gridcell.ww, gridcell_inc1.ww ) * ( 1.0 / SOMELARGEFLOAT );

    z0w0_hash = frac( x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.xxxx );
    z1w0_hash = frac( x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.yyyy );
    z0w1_hash = frac( x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.zzzz );
    z1w1_hash = frac( x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.wwww );
}
void FAST32_2_hash_4D( 	float4 gridcell,
                        out float4 z0w0_hash_0,		//  float4 == ( x0y0, x1y0, x0y1, x1y1 )
                        out float4 z0w0_hash_1,
                        out float4 z0w0_hash_2,
                        out float4 z0w0_hash_3,
                        out float4 z1w0_hash_0,
                        out float4 z1w0_hash_1,
                        out float4 z1w0_hash_2,
                        out float4 z1w0_hash_3,
                        out float4 z0w1_hash_0,
                        out float4 z0w1_hash_1,
                        out float4 z0w1_hash_2,
                        out float4 z0w1_hash_3,
                        out float4 z1w1_hash_0,
                        out float4 z1w1_hash_1,
                        out float4 z1w1_hash_2,
                        out float4 z1w1_hash_3	)
{
    //    gridcell is assumed to be an integer coordinate

    //	TODO: 	these constants need tweaked to find the best possible noise.
    //			probably requires some kind of brute force computational searching or something....
    const float4 OFFSET = float4( 16.841230, 18.774548, 16.873274, 13.664607 );
    const float DOMAIN = 69.0;
    const float4 SOMELARGEFLOATS = float4( 56974.746094, 47165.636719, 55049.667969, 49901.273438 );
    const float4 NOISE_SCALE = float4( 0.102007, 0.114473, 0.139651, 0.084550 );

    //	truncate the domain
    gridcell = gridcell - floor(gridcell * ( 1.0 / DOMAIN )) * DOMAIN;
    float4 gridcell_inc1 = step( gridcell, float( DOMAIN - 1.5 ).xxxx ) * ( gridcell + 1.0 );

    //	calculate the noise
    gridcell = ( gridcell * NOISE_SCALE ) + OFFSET;
    gridcell_inc1 = ( gridcell_inc1 * NOISE_SCALE ) + OFFSET;
    gridcell *= gridcell;
    gridcell_inc1 *= gridcell_inc1;

    float4 x0y0_x1y0_x0y1_x1y1 = float4( gridcell.x, gridcell_inc1.x, gridcell.x, gridcell_inc1.x ) * float4( gridcell.yy, gridcell_inc1.yy );
    float4 z0w0_z1w0_z0w1_z1w1 = float4( gridcell.z, gridcell_inc1.z, gridcell.z, gridcell_inc1.z ) * float4( gridcell.ww, gridcell_inc1.ww );

    float4 hashval = x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.xxxx;
    z0w0_hash_0 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.x ) );
    z0w0_hash_1 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.y ) );
    z0w0_hash_2 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.z ) );
    z0w0_hash_3 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.w ) );
    hashval = x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.yyyy;
    z1w0_hash_0 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.x ) );
    z1w0_hash_1 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.y ) );
    z1w0_hash_2 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.z ) );
    z1w0_hash_3 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.w ) );
    hashval = x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.zzzz;
    z0w1_hash_0 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.x ) );
    z0w1_hash_1 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.y ) );
    z0w1_hash_2 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.z ) );
    z0w1_hash_3 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.w ) );
    hashval = x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.wwww;
    z1w1_hash_0 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.x ) );
    z1w1_hash_1 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.y ) );
    z1w1_hash_2 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.z ) );
    z1w1_hash_3 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.w ) );
}


//
//	Interpolation functions
//	( smoothly increase from 0.0 to 1.0 as x increases linearly from 0.0 to 1.0 )
//	http://briansharpe.wordpress.com/2011/11/14/two-useful-interpolation-functions-for-noise-development/
//
float Interpolation_C1( float x ) { return x * x * (3.0 - 2.0 * x); }   //  3x^2-2x^3  ( Hermine Curve.  Same as SmoothStep().  As used by Perlin in Original Noise. )
float2 Interpolation_C1( float2 x ) { return x * x * (3.0 - 2.0 * x); }
float3 Interpolation_C1( float3 x ) { return x * x * (3.0 - 2.0 * x); }
float4 Interpolation_C1( float4 x ) { return x * x * (3.0 - 2.0 * x); }

float Interpolation_C2( float x ) { return x * x * x * (x * (x * 6.0 - 15.0) + 10.0); }   //  6x^5-15x^4+10x^3	( Quintic Curve.  As used by Perlin in Improved Noise.  http://mrl.nyu.edu/~perlin/paper445.pdf )
float2 Interpolation_C2( float2 x ) { return x * x * x * (x * (x * 6.0 - 15.0) + 10.0); }
float3 Interpolation_C2( float3 x ) { return x * x * x * (x * (x * 6.0 - 15.0) + 10.0); }
float4 Interpolation_C2( float4 x ) { return x * x * x * (x * (x * 6.0 - 15.0) + 10.0); }
float4 Interpolation_C2_InterpAndDeriv( float2 x ) { return x.xyxy * x.xyxy * ( x.xyxy * ( x.xyxy * ( x.xyxy * float2( 6.0, 0.0 ).xxyy + float2( -15.0, 30.0 ).xxyy ) + float2( 10.0, -60.0 ).xxyy ) + float2( 0.0, 30.0 ).xxyy ); }
float3 Interpolation_C2_Deriv( float3 x ) { return x * x * (x * (x * 30.0 - 60.0) + 30.0); }

float Interpolation_C2_Fast( float x ) { float x3 = x*x*x; return ( 7.0 + ( x3 - 7.0 ) * x ) * x3; }   //  7x^3-7x^4+x^7   ( Faster than Perlin Quintic.  Not quite as good shape. )
float2 Interpolation_C2_Fast( float2 x ) { float2 x3 = x*x*x; return ( 7.0 + ( x3 - 7.0 ) * x ) * x3; }
float3 Interpolation_C2_Fast( float3 x ) { float3 x3 = x*x*x; return ( 7.0 + ( x3 - 7.0 ) * x ) * x3; }
float4 Interpolation_C2_Fast( float4 x ) { float4 x3 = x*x*x; return ( 7.0 + ( x3 - 7.0 ) * x ) * x3; }

float Interpolation_C3( float x ) { float xsq = x*x; float xsqsq = xsq*xsq; return xsqsq * ( 25.0 - 48.0 * x + xsq * ( 25.0 - xsqsq ) ); }   //  25x^4-48x^5+25x^6-x^10		( C3 Interpolation function.  If anyone ever needs it... :) )
float2 Interpolation_C3( float2 x ) { float2 xsq = x*x; float2 xsqsq = xsq*xsq; return xsqsq * ( 25.0 - 48.0 * x + xsq * ( 25.0 - xsqsq ) ); }
float3 Interpolation_C3( float3 x ) { float3 xsq = x*x; float3 xsqsq = xsq*xsq; return xsqsq * ( 25.0 - 48.0 * x + xsq * ( 25.0 - xsqsq ) ); }
float4 Interpolation_C3( float4 x ) { float4 xsq = x*x; float4 xsqsq = xsq*xsq; return xsqsq * ( 25.0 - 48.0 * x + xsq * ( 25.0 - xsqsq ) ); }


//
//	Falloff defined in XSquared
//	( smoothly decrease from 1.0 to 0.0 as xsq increases from 0.0 to 1.0 )
//	http://briansharpe.wordpress.com/2011/11/14/two-useful-interpolation-functions-for-noise-development/
//
float Falloff_Xsq_C1( float xsq ) { xsq = 1.0 - xsq; return xsq*xsq; }	// ( 1.0 - x*x )^2   ( Used by Humus for lighting falloff in Just Cause 2.  GPUPro 1 )
float Falloff_Xsq_C2( float xsq ) { xsq = 1.0 - xsq; return xsq*xsq*xsq; }	// ( 1.0 - x*x )^3.   NOTE: 2nd derivative is 0.0 at x=1.0, but non-zero at x=0.0
float4 Falloff_Xsq_C2( float4 xsq ) { xsq = 1.0 - xsq; return xsq*xsq*xsq; }

//
//	Value Noise 2D
//	Return value range of 0.0->1.0
//	http://briansharpe.files.wordpress.com/2011/11/valuesample1.jpg
//
float Value2D( float2 P )
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash = FAST32_hash_2D( Pi );
    //float4 hash = FAST32_2_hash_2D( Pi );
    //float4 hash = BBS_hash_2D( Pi );
    //float4 hash = SGPP_hash_2D( Pi );
    //float4 hash = BBS_hash_hq_2D( Pi );

    //	blend the results and return
    float2 blend = Interpolation_C2( Pf );
    float4 blend2 = float4( blend, float2( 1.0 - blend ) );
    return dot( hash, blend2.zxzx * blend2.wwyy );
}

//
//	Value Noise 3D
//	Return value range of 0.0->1.0
//	http://briansharpe.files.wordpress.com/2011/11/valuesample1.jpg
//
float Value3D( float3 P )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_lowz, hash_highz;
    FAST32_hash_3D( Pi, hash_lowz, hash_highz );
    //FAST32_2_hash_3D( Pi, hash_lowz, hash_highz );
    //BBS_hash_3D( Pi, hash_lowz, hash_highz );
    //SGPP_hash_3D( Pi, hash_lowz, hash_highz );

    //	blend the results and return
    float3 blend = Interpolation_C2( Pf );
    float4 res0 = lerp( hash_lowz, hash_highz, blend.z );
    float4 blend2 = float4( blend.xy, float2( 1.0 - blend.xy ) );
    return dot( res0, blend2.zxzx * blend2.wwyy );
}

//
//	Value Noise 4D
//	Return value range of 0.0->1.0
//
float Value4D( float4 P )
{
    //	establish our grid cell and unit position
    float4 Pi = floor(P);
    float4 Pf = P - Pi;

    //	calculate the hash.
    float4 z0w0_hash;		//  float4 == ( x0y0, x1y0, x0y1, x1y1 )
    float4 z1w0_hash;
    float4 z0w1_hash;
    float4 z1w1_hash;
    FAST32_2_hash_4D( Pi, z0w0_hash, z1w0_hash, z0w1_hash, z1w1_hash );

    //	blend the results and return
    float4 blend = Interpolation_C2( Pf );
    float4 res0 = z0w0_hash + ( z0w1_hash - z0w0_hash ) * blend.wwww;
    float4 res1 = z1w0_hash + ( z1w1_hash - z1w0_hash ) * blend.wwww;
    res0 = res0 + ( res1 - res0 ) * blend.zzzz;
    blend.zw = float2( 1.0 - blend.xy );
    return dot( res0, blend.zxzx * blend.wwyy );
}

//
//	Perlin Noise 2D  ( gradient noise )
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2011/11/perlinsample.jpg
//
float Perlin2D( float2 P )
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float4 Pf_Pfmin1 = P.xyxy - float4( Pi, Pi + 1.0 );

#if 1
    //
    //	classic noise looks much better than improved noise in 2D, and with an efficent hash function runs at about the same speed.
    //	requires 2 random numbers per point.
    //

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	calculate the gradient results
    float4 grad_x = hash_x - 0.49999;
    float4 grad_y = hash_y - 0.49999;
    float4 grad_results = rsqrt( grad_x * grad_x + grad_y * grad_y ) * ( grad_x * Pf_Pfmin1.xzxz + grad_y * Pf_Pfmin1.yyww );

#if 1
    //	Classic Perlin Interpolation
    grad_results *= 1.4142135623730950488016887242097;		//	(optionally) scale things to a strict -1.0->1.0 range    *= 1.0/sqrt(0.5)
    float2 blend = Interpolation_C2( Pf_Pfmin1.xy );
    float4 blend2 = float4( blend, float2( 1.0 - blend ) );
    return dot( grad_results, blend2.zxzx * blend2.wwyy );
#else
    //	Classic Perlin Surflet
    //	http://briansharpe.wordpress.com/2012/03/09/modifications-to-classic-perlin-noise/
    grad_results *= 2.3703703703703703703703703703704;		//	(optionally) scale things to a strict -1.0->1.0 range    *= 1.0/cube(0.75)
    float4 vecs_len_sq = Pf_Pfmin1 * Pf_Pfmin1;
    vecs_len_sq = vecs_len_sq.xzxz + vecs_len_sq.yyww;
    return dot( Falloff_Xsq_C2( min( float4( 1.0 ), vecs_len_sq ) ), grad_results );
#endif

#else
    //
    //	2D improved perlin noise.
    //	requires 1 random value per point.
    //	does not look as good as classic in 2D due to only a small number of possible cell types.  But can run a lot faster than classic perlin noise if the hash function is slow
    //

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash = FAST32_hash_2D( Pi );
    //float4 hash = BBS_hash_2D( Pi );
    //float4 hash = SGPP_hash_2D( Pi );
    //float4 hash = BBS_hash_hq_2D( Pi );

    //
    //	evaulate the gradients
    //	choose between the 4 diagonal gradients.  ( slightly slower than choosing the axis gradients, but shows less grid artifacts )
    //	NOTE:  diagonals give us a nice strict -1.0->1.0 range without additional scaling
    //	[1.0,1.0] [-1.0,1.0] [1.0,-1.0] [-1.0,-1.0]
    //
    hash -= 0.5;
    float4 grad_results = Pf_Pfmin1.xzxz * sign( hash ) + Pf_Pfmin1.yyww * sign( abs( hash ) - 0.25 );

    //	blend the results and return
    float2 blend = Interpolation_C2( Pf_Pfmin1.xy );
    float4 blend2 = float4( blend, float2( 1.0 - blend ) );
    return dot( grad_results, blend2.zxzx * blend2.wwyy );

#endif

}

//
//	Perlin Noise 3D  ( gradient noise )
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2011/11/perlinsample.jpg
//
float Perlin3D( float3 P )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;
    float3 Pf_min1 = Pf - 1.0;

#if 1
    //
    //	classic noise.
    //	requires 3 random values per point.  with an efficent hash function will run faster than improved noise
    //

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hashx0, hashy0, hashz0, hashx1, hashy1, hashz1;
    FAST32_hash_3D( Pi, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1 );
    //SGPP_hash_3D( Pi, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1 );

    //	calculate the gradients
    float4 grad_x0 = hashx0 - 0.49999;
    float4 grad_y0 = hashy0 - 0.49999;
    float4 grad_z0 = hashz0 - 0.49999;
    float4 grad_x1 = hashx1 - 0.49999;
    float4 grad_y1 = hashy1 - 0.49999;
    float4 grad_z1 = hashz1 - 0.49999;
    float4 grad_results_0 = rsqrt( grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0 ) * ( float2( Pf.x, Pf_min1.x ).xyxy * grad_x0 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y0 + Pf.zzzz * grad_z0 );
    float4 grad_results_1 = rsqrt( grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1 ) * ( float2( Pf.x, Pf_min1.x ).xyxy * grad_x1 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1 );

#if 1
    //	Classic Perlin Interpolation
    float3 blend = Interpolation_C2( Pf );
    float4 res0 = lerp( grad_results_0, grad_results_1, blend.z );
    float4 blend2 = float4( blend.xy, float2( 1.0 - blend.xy ) );
    float final = dot( res0, blend2.zxzx * blend2.wwyy );
    final *= 1.1547005383792515290182975610039;		//	(optionally) scale things to a strict -1.0->1.0 range    *= 1.0/sqrt(0.75)
    return final;
#else
    //	Classic Perlin Surflet
    //	http://briansharpe.wordpress.com/2012/03/09/modifications-to-classic-perlin-noise/
    Pf *= Pf;
    Pf_min1 *= Pf_min1;
    float4 vecs_len_sq = float4( Pf.x, Pf_min1.x, Pf.x, Pf_min1.x ) + float4( Pf.yy, Pf_min1.yy );
    float final = dot( Falloff_Xsq_C2( min( float4( 1.0 ), vecs_len_sq + Pf.zzzz ) ), grad_results_0 ) + dot( Falloff_Xsq_C2( min( float4( 1.0 ), vecs_len_sq + Pf_min1.zzzz ) ), grad_results_1 );
    final *= 2.3703703703703703703703703703704;		//	(optionally) scale things to a strict -1.0->1.0 range    *= 1.0/cube(0.75)
    return final;
#endif

#else
    //
    //	improved noise.
    //	requires 1 random value per point.  Will run faster than classic noise if a slow hashing function is used
    //

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_lowz, hash_highz;
    FAST32_hash_3D( Pi, hash_lowz, hash_highz );
    //BBS_hash_3D( Pi, hash_lowz, hash_highz );
    //SGPP_hash_3D( Pi, hash_lowz, hash_highz );

    //
    //	"improved" noise using 8 corner gradients.  Faster than the 12 mid-edge point method.
    //	Ken mentions using diagonals like this can cause "clumping", but we'll live with that.
    //	[1,1,1]  [-1,1,1]  [1,-1,1]  [-1,-1,1]
    //	[1,1,-1] [-1,1,-1] [1,-1,-1] [-1,-1,-1]
    //
    hash_lowz -= 0.5;
    float4 grad_results_0_0 = float2( Pf.x, Pf_min1.x ).xyxy * sign( hash_lowz );
    hash_lowz = abs( hash_lowz ) - 0.25;
    float4 grad_results_0_1 = float2( Pf.y, Pf_min1.y ).xxyy * sign( hash_lowz );
    float4 grad_results_0_2 = Pf.zzzz * sign( abs( hash_lowz ) - 0.125 );
    float4 grad_results_0 = grad_results_0_0 + grad_results_0_1 + grad_results_0_2;

    hash_highz -= 0.5;
    float4 grad_results_1_0 = float2( Pf.x, Pf_min1.x ).xyxy * sign( hash_highz );
    hash_highz = abs( hash_highz ) - 0.25;
    float4 grad_results_1_1 = float2( Pf.y, Pf_min1.y ).xxyy * sign( hash_highz );
    float4 grad_results_1_2 = Pf_min1.zzzz * sign( abs( hash_highz ) - 0.125 );
    float4 grad_results_1 = grad_results_1_0 + grad_results_1_1 + grad_results_1_2;

    //	blend the gradients and return
    float3 blend = Interpolation_C2( Pf );
    float4 res0 = lerp( grad_results_0, grad_results_1, blend.z );
    float4 blend2 = float4( blend.xy, float2( 1.0 - blend.xy ) );
    return dot( res0, blend2.zxzx * blend2.wwyy ) * (2.0 / 3.0);	//	(optionally) mult by (2.0/3.0) to scale to a strict -1.0->1.0 range
#endif

}


//
// Perlin Noise 4D ( gradient noise )
// Return value range of -1.0->1.0
//
float Perlin4D( float4 P )
{
    // establish our grid cell and unit position
    float4 Pi = floor(P);
    float4 Pf = P - Pi;
    float4 Pf_min1 = Pf - 1.0;

    //    calculate the hash.
    float4 lowz_loww_hash_0, lowz_loww_hash_1, lowz_loww_hash_2, lowz_loww_hash_3;
    float4 highz_loww_hash_0, highz_loww_hash_1, highz_loww_hash_2, highz_loww_hash_3;
    float4 lowz_highw_hash_0, lowz_highw_hash_1, lowz_highw_hash_2, lowz_highw_hash_3;
    float4 highz_highw_hash_0, highz_highw_hash_1, highz_highw_hash_2, highz_highw_hash_3;
    FAST32_2_hash_4D(
        Pi,
        lowz_loww_hash_0, lowz_loww_hash_1, lowz_loww_hash_2, lowz_loww_hash_3,
        highz_loww_hash_0, highz_loww_hash_1, highz_loww_hash_2, highz_loww_hash_3,
        lowz_highw_hash_0, lowz_highw_hash_1, lowz_highw_hash_2, lowz_highw_hash_3,
        highz_highw_hash_0, highz_highw_hash_1, highz_highw_hash_2, highz_highw_hash_3 );

    //	calculate the gradients
    lowz_loww_hash_0 -= 0.49999;
    lowz_loww_hash_1 -= 0.49999;
    lowz_loww_hash_2 -= 0.49999;
    lowz_loww_hash_3 -= 0.49999;
    highz_loww_hash_0 -= 0.49999;
    highz_loww_hash_1 -= 0.49999;
    highz_loww_hash_2 -= 0.49999;
    highz_loww_hash_3 -= 0.49999;
    lowz_highw_hash_0 -= 0.49999;
    lowz_highw_hash_1 -= 0.49999;
    lowz_highw_hash_2 -= 0.49999;
    lowz_highw_hash_3 -= 0.49999;
    highz_highw_hash_0 -= 0.49999;
    highz_highw_hash_1 -= 0.49999;
    highz_highw_hash_2 -= 0.49999;
    highz_highw_hash_3 -= 0.49999;

    float4 grad_results_lowz_loww = rsqrt( lowz_loww_hash_0 * lowz_loww_hash_0 + lowz_loww_hash_1 * lowz_loww_hash_1 + lowz_loww_hash_2 * lowz_loww_hash_2 + lowz_loww_hash_3 * lowz_loww_hash_3 );
    grad_results_lowz_loww *= ( float2( Pf.x, Pf_min1.x ).xyxy * lowz_loww_hash_0 + float2( Pf.y, Pf_min1.y ).xxyy * lowz_loww_hash_1 + Pf.zzzz * lowz_loww_hash_2 + Pf.wwww * lowz_loww_hash_3 );

    float4 grad_results_highz_loww = rsqrt( highz_loww_hash_0 * highz_loww_hash_0 + highz_loww_hash_1 * highz_loww_hash_1 + highz_loww_hash_2 * highz_loww_hash_2 + highz_loww_hash_3 * highz_loww_hash_3 );
    grad_results_highz_loww *= ( float2( Pf.x, Pf_min1.x ).xyxy * highz_loww_hash_0 + float2( Pf.y, Pf_min1.y ).xxyy * highz_loww_hash_1 + Pf_min1.zzzz * highz_loww_hash_2 + Pf.wwww * highz_loww_hash_3 );

    float4 grad_results_lowz_highw = rsqrt( lowz_highw_hash_0 * lowz_highw_hash_0 + lowz_highw_hash_1 * lowz_highw_hash_1 + lowz_highw_hash_2 * lowz_highw_hash_2 + lowz_highw_hash_3 * lowz_highw_hash_3 );
    grad_results_lowz_highw *= ( float2( Pf.x, Pf_min1.x ).xyxy * lowz_highw_hash_0 + float2( Pf.y, Pf_min1.y ).xxyy * lowz_highw_hash_1 + Pf.zzzz * lowz_highw_hash_2 + Pf_min1.wwww * lowz_highw_hash_3 );

    float4 grad_results_highz_highw = rsqrt( highz_highw_hash_0 * highz_highw_hash_0 + highz_highw_hash_1 * highz_highw_hash_1 + highz_highw_hash_2 * highz_highw_hash_2 + highz_highw_hash_3 * highz_highw_hash_3 );
    grad_results_highz_highw *= ( float2( Pf.x, Pf_min1.x ).xyxy * highz_highw_hash_0 + float2( Pf.y, Pf_min1.y ).xxyy * highz_highw_hash_1 + Pf_min1.zzzz * highz_highw_hash_2 + Pf_min1.wwww * highz_highw_hash_3 );

    // Classic Perlin Interpolation
    float4 blend = Interpolation_C2( Pf );
    float4 res0 = grad_results_lowz_loww + ( grad_results_lowz_highw - grad_results_lowz_loww ) * blend.wwww;
    float4 res1 = grad_results_highz_loww + ( grad_results_highz_highw - grad_results_highz_loww ) * blend.wwww;
    res0 = res0 + ( res1 - res0 ) * blend.zzzz;
    blend.zw = float( 1.0 ).xx - blend.xy;
    return dot( res0, blend.zxzx * blend.wwyy );
}


//
//	ValuePerlin Noise 2D	( value gradient noise )
//	A uniform blend between value and perlin noise
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2011/11/valueperlinsample.jpg
//
float ValuePerlin2D( float2 P, float blend_val )
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float4 Pf_Pfmin1 = P.xyxy - float4( Pi, Pi + 1.0 );

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_value, hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_value, hash_x, hash_y );

    //	calculate the gradient results
    float4 grad_x = hash_x - 0.49999;
    float4 grad_y = hash_y - 0.49999;
    float4 grad_results = rsqrt( grad_x * grad_x + grad_y * grad_y ) * ( grad_x * Pf_Pfmin1.xzxz + grad_y * Pf_Pfmin1.yyww );
    grad_results *= 1.4142135623730950488016887242097;		//	scale the perlin component to a -1.0->1.0 range    *= 1.0/sqrt(0.5)
    grad_results = lerp( (hash_value * 2.0 - 1.0), grad_results, blend_val );

    //	blend the results and return
    float2 blend = Interpolation_C2( Pf_Pfmin1.xy );
    float4 blend2 = float4( blend, float2( 1.0 - blend ) );
    return dot( grad_results, blend2.zxzx * blend2.wwyy );
}


//
//	ValuePerlin Noise 3D	( value gradient noise )
//	A uniform blend between value and perlin noise
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2011/11/valueperlinsample.jpg
//
float ValuePerlin3D( float3 P, float blend_val )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;
    float3 Pf_min1 = Pf - 1.0;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_value0, hashx0, hashy0, hashz0, hash_value1, hashx1, hashy1, hashz1;
    FAST32_hash_3D( Pi, hash_value0, hashx0, hashy0, hashz0, hash_value1, hashx1, hashy1, hashz1 );

    //	calculate the gradients
    float4 grad_x0 = hashx0 - 0.49999;
    float4 grad_y0 = hashy0 - 0.49999;
    float4 grad_z0 = hashz0 - 0.49999;
    float4 grad_x1 = hashx1 - 0.49999;
    float4 grad_y1 = hashy1 - 0.49999;
    float4 grad_z1 = hashz1 - 0.49999;
    float4 grad_results_0 = rsqrt( grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0 ) * ( float2( Pf.x, Pf_min1.x ).xyxy * grad_x0 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y0 + Pf.zzzz * grad_z0 );
    float4 grad_results_1 = rsqrt( grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1 ) * ( float2( Pf.x, Pf_min1.x ).xyxy * grad_x1 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1 );
    grad_results_0 *= 1.1547005383792515290182975610039;		//	scale the perlin component to a -1.0->1.0 range    *= 1.0/sqrt(0.75)
    grad_results_1 *= 1.1547005383792515290182975610039;
    grad_results_0 = lerp( (hash_value0 * 2.0 - 1.0), grad_results_0, blend_val );
    grad_results_1 = lerp( (hash_value1 * 2.0 - 1.0), grad_results_1, blend_val );

    //	blend the gradients and return
    float3 blend = Interpolation_C2( Pf );
    float4 res0 = lerp( grad_results_0, grad_results_1, blend.z );
    float4 blend2 = float4( blend.xy, float2( 1.0 - blend.xy ) );
    return dot( res0, blend2.zxzx * blend2.wwyy );
}


//
//	Cubist Noise 2D
//	http://briansharpe.files.wordpress.com/2011/12/cubistsample.jpg
//
//	Generates a noise which resembles a cubist-style painting pattern.  Final Range 0.0->1.0
//	NOTE:  contains discontinuities.  best used only for texturing.
//	NOTE:  Any serious game implementation should hard-code these parameter values for efficiency.
//
float Cubist2D( float2 P, float2 range_clamp )	// range_clamp.x = low, range_clamp.y = 1.0/(high-low).  suggest value low=-2.0  high=1.0
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float4 Pf_Pfmin1 = P.xyxy - float4( Pi, Pi + 1.0 );

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y, hash_value;
    FAST32_hash_2D( Pi, hash_x, hash_y, hash_value );

    //	calculate the gradient results
    float4 grad_x = hash_x - 0.49999;
    float4 grad_y = hash_y - 0.49999;
    float4 grad_results = rsqrt( grad_x * grad_x + grad_y * grad_y ) * ( grad_x * Pf_Pfmin1.xzxz + grad_y * Pf_Pfmin1.yyww );

    //	invert the gradient to convert from perlin to cubist
    grad_results = ( hash_value - 0.5 ) * ( 1.0 / grad_results );

    //	blend the results and return
    float2 blend = Interpolation_C2( Pf_Pfmin1.xy );
    float4 blend2 = float4( blend, float2( 1.0 - blend ) );
    float final = dot( grad_results, blend2.zxzx * blend2.wwyy );

    //	the 1.0/grad calculation pushes the result to a possible to +-infinity.  Need to clamp to keep things sane
    return clamp( ( final - range_clamp.x ) * range_clamp.y, 0.0, 1.0 );
    //return smoothstep( 0.0, 1.0, ( final - range_clamp.x ) * range_clamp.y );		//	experiments.  smoothstep doesn't look as good, but does remove some discontinuities....
}


//
//	Cubist Noise 3D
//	http://briansharpe.files.wordpress.com/2011/12/cubistsample.jpg
//
//	Generates a noise which resembles a cubist-style painting pattern.  Final Range 0.0->1.0
//	NOTE:  contains discontinuities.  best used only for texturing.
//	NOTE:  Any serious game implementation should hard-code these parameter values for efficiency.
//
float Cubist3D( float3 P, float2 range_clamp )	// range_clamp.x = low, range_clamp.y = 1.0/(high-low).  suggest value low=-2.0  high=1.0
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;
    float3 Pf_min1 = Pf - 1.0;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hashx0, hashy0, hashz0, hash_value0, hashx1, hashy1, hashz1, hash_value1;
    FAST32_hash_3D( Pi, hashx0, hashy0, hashz0, hash_value0, hashx1, hashy1, hashz1, hash_value1 );

    //	calculate the gradients
    float4 grad_x0 = hashx0 - 0.49999;
    float4 grad_y0 = hashy0 - 0.49999;
    float4 grad_z0 = hashz0 - 0.49999;
    float4 grad_x1 = hashx1 - 0.49999;
    float4 grad_y1 = hashy1 - 0.49999;
    float4 grad_z1 = hashz1 - 0.49999;
    float4 grad_results_0 = rsqrt( grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0 ) * ( float2( Pf.x, Pf_min1.x ).xyxy * grad_x0 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y0 + Pf.zzzz * grad_z0 );
    float4 grad_results_1 = rsqrt( grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1 ) * ( float2( Pf.x, Pf_min1.x ).xyxy * grad_x1 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1 );

    //	invert the gradient to convert from perlin to cubist
    grad_results_0 = ( hash_value0 - 0.5 ) * ( 1.0 / grad_results_0 );
    grad_results_1 = ( hash_value1 - 0.5 ) * ( 1.0 / grad_results_1 );

    //	blend the gradients and return
    float3 blend = Interpolation_C2( Pf );
    float4 res0 = lerp( grad_results_0, grad_results_1, blend.z );
    float4 blend2 = float4( blend.xy, float2( 1.0 - blend.xy ) );
    float final = dot( res0, blend2.zxzx * blend2.wwyy );

    //	the 1.0/grad calculation pushes the result to a possible to +-infinity.  Need to clamp to keep things sane
    return clamp( ( final - range_clamp.x ) * range_clamp.y, 0.0, 1.0 );
    //return smoothstep( 0.0, 1.0, ( final - range_clamp.x ) * range_clamp.y );		//	experiments.  smoothstep doesn't look as good, but does remove some discontinuities....
}


//	convert a 0.0->1.0 sample to a -1.0->1.0 sample weighted towards the extremes
float4 Cellular_weight_samples( float4 samples )
{
    samples = samples * 2.0 - 1.0;
    //return (1.0 - samples * samples) * sign(samples);	// square
    return (samples * samples * samples) - sign(samples);	// cubic (even more variance)
}

//
//	Cellular Noise 2D
//	Based off Stefan Gustavson's work at http://www.itn.liu.se/~stegu/GLSL-cellular
//	http://briansharpe.files.wordpress.com/2011/12/cellularsample.jpg
//
//	Speed up by using 2x2 search window instead of 3x3
//	produces a range of 0.0->1.0
//
float Cellular2D(float2 P)
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	generate the 4 random points
#if 1
    //	restrict the random point offset to eliminate artifacts
    //	we'll improve the variance of the noise by pushing the points to the extremes of the jitter window
    const float JITTER_WINDOW = 0.25;	// 0.25 will guarentee no artifacts.  0.25 is the intersection on x of graphs f(x)=( (0.5+(0.5-x))^2 + (0.5-x)^2 ) and f(x)=( (0.5+x)^2 + x^2 )
    hash_x = Cellular_weight_samples( hash_x ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y = Cellular_weight_samples( hash_y ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);
#else
    //	non-weighted jitter window.  jitter window of 0.4 will give results similar to Stefans original implementation
    //	nicer looking, faster, but has minor artifacts.  ( discontinuities in signal )
    const float JITTER_WINDOW = 0.4;
    hash_x = hash_x * JITTER_WINDOW * 2.0 + float4(-JITTER_WINDOW, 1.0-JITTER_WINDOW, -JITTER_WINDOW, 1.0-JITTER_WINDOW);
    hash_y = hash_y * JITTER_WINDOW * 2.0 + float4(-JITTER_WINDOW, -JITTER_WINDOW, 1.0-JITTER_WINDOW, 1.0-JITTER_WINDOW);
#endif

    //	return the closest squared distance
    float4 dx = Pf.xxxx - hash_x;
    float4 dy = Pf.yyyy - hash_y;
    float4 d = dx * dx + dy * dy;
    d.xy = min(d.xy, d.zw);
    return min(d.x, d.y) * ( 1.0 / 1.125 );	//	scale return value from 0.0->1.125 to 0.0->1.0  ( 0.75^2 * 2.0  == 1.125 )
}



//
//	Cellular Noise 3D
//	Based off Stefan Gustavson's work at http://www.itn.liu.se/~stegu/GLSL-cellular
//	http://briansharpe.files.wordpress.com/2011/12/cellularsample.jpg
//
//	Speed up by using 2x2x2 search window instead of 3x3x3
//	produces range of 0.0->1.0
//
float Cellular3D(float3 P)
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1;
    FAST32_hash_3D( Pi, hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1 );
    //SGPP_hash_3D( Pi, hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1 );

    //	generate the 8 random points
#if 1
    //	restrict the random point offset to eliminate artifacts
    //	we'll improve the variance of the noise by pushing the points to the extremes of the jitter window
    const float JITTER_WINDOW = 0.166666666;	// 0.166666666 will guarentee no artifacts. It is the intersection on x of graphs f(x)=( (0.5 + (0.5-x))^2 + 2*((0.5-x)^2) ) and f(x)=( 2 * (( 0.5 + x )^2) + x * x )
    hash_x0 = Cellular_weight_samples( hash_x0 ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y0 = Cellular_weight_samples( hash_y0 ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);
    hash_x1 = Cellular_weight_samples( hash_x1 ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y1 = Cellular_weight_samples( hash_y1 ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);
    hash_z0 = Cellular_weight_samples( hash_z0 ) * JITTER_WINDOW + float4(0.0, 0.0, 0.0, 0.0);
    hash_z1 = Cellular_weight_samples( hash_z1 ) * JITTER_WINDOW + float4(1.0, 1.0, 1.0, 1.0);
#else
    //	non-weighted jitter window.  jitter window of 0.4 will give results similar to Stefans original implementation
    //	nicer looking, faster, but has minor artifacts.  ( discontinuities in signal )
    const float JITTER_WINDOW = 0.4;
    hash_x0 = hash_x0 * JITTER_WINDOW * 2.0 + float4(-JITTER_WINDOW, 1.0-JITTER_WINDOW, -JITTER_WINDOW, 1.0-JITTER_WINDOW);
    hash_y0 = hash_y0 * JITTER_WINDOW * 2.0 + float4(-JITTER_WINDOW, -JITTER_WINDOW, 1.0-JITTER_WINDOW, 1.0-JITTER_WINDOW);
    hash_x1 = hash_x1 * JITTER_WINDOW * 2.0 + float4(-JITTER_WINDOW, 1.0-JITTER_WINDOW, -JITTER_WINDOW, 1.0-JITTER_WINDOW);
    hash_y1 = hash_y1 * JITTER_WINDOW * 2.0 + float4(-JITTER_WINDOW, -JITTER_WINDOW, 1.0-JITTER_WINDOW, 1.0-JITTER_WINDOW);
    hash_z0 = hash_z0 * JITTER_WINDOW * 2.0 + float4(-JITTER_WINDOW, -JITTER_WINDOW, -JITTER_WINDOW, -JITTER_WINDOW);
    hash_z1 = hash_z1 * JITTER_WINDOW * 2.0 + float4(1.0-JITTER_WINDOW, 1.0-JITTER_WINDOW, 1.0-JITTER_WINDOW, 1.0-JITTER_WINDOW);
#endif

    //	return the closest squared distance
    float4 dx1 = Pf.xxxx - hash_x0;
    float4 dy1 = Pf.yyyy - hash_y0;
    float4 dz1 = Pf.zzzz - hash_z0;
    float4 dx2 = Pf.xxxx - hash_x1;
    float4 dy2 = Pf.yyyy - hash_y1;
    float4 dz2 = Pf.zzzz - hash_z1;
    float4 d1 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;
    float4 d2 = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;
    d1 = min(d1, d2);
    d1.xy = min(d1.xy, d1.wz);
    return min(d1.x, d1.y) * ( 9.0 / 12.0 );	//	scale return value from 0.0->1.333333 to 0.0->1.0  	(2/3)^2 * 3  == (12/9) == 1.333333
}


//
//	NOTE:
//	This noise is not yet finished.
//	At the moment it is only giving one "dot" per gridcell.  It really needs to give at least two.
//
/*
//
//	SparseConvolution2D
//
//	Very crude approximation of sparse convolution noise.  ( derived from the Cellular2D implementation )
//	return value scaling to 0.0->1.0 range TODO
//
float SparseConvolution2D(float2 P)
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	generate the 4 random points
    //	restrict the random point offset to eliminate artifacts
    //	we'll improve the variance of the noise by pushing the points to the extremes of the jitter window
    const float JITTER_WINDOW = 0.25;	// 0.25 will guarentee no artifacts.  0.25 is the intersection on x of graphs f(x)=( (0.5+(0.5-x))^2 + (0.5-x)^2 ) and f(x)=( (0.5+x)^2 + x^2 )
    hash_x = Cellular_weight_samples( hash_x ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y = Cellular_weight_samples( hash_y ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);

    //	find the squared distance to each point
    float4 dx = Pf.xxxx - hash_x;
    float4 dy = Pf.yyyy - hash_y;
    float4 d = dx * dx + dy * dy;

    //	sum kernels and return
    const float RADIUS = 1.0 - JITTER_WINDOW;
    d *= ( ( 1.0 / RADIUS ) * ( 1.0 / RADIUS ) );
    return dot( Falloff_Xsq_C2( min( d, float4( 1.0 ) ) ), float4( 1.0 ) );
}


//
//	SparseConvolution3D
//
//	Very crude approximation of sparse convolution noise.  ( derived from the Cellular3D implementation )
//	return value scaling to 0.0->1.0 range TODO
//
float SparseConvolution3D(float3 P)
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1;
    FAST32_hash_3D( Pi, hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1 );
    //SGPP_hash_3D( Pi, hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1 );

    //	generate the 8 random points
    //	restrict the random point offset to eliminate artifacts
    //	we'll improve the variance of the noise by pushing the points to the extremes of the jitter window
    const float JITTER_WINDOW = 0.166666666;	// 0.166666666 will guarentee no artifacts. It is the intersection on x of graphs f(x)=( (0.5 + (0.5-x))^2 + 2*((0.5-x)^2) ) and f(x)=( 2 * (( 0.5 + x )^2) + x * x )
    hash_x0 = Cellular_weight_samples( hash_x0 ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y0 = Cellular_weight_samples( hash_y0 ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);
    hash_x1 = Cellular_weight_samples( hash_x1 ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y1 = Cellular_weight_samples( hash_y1 ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);
    hash_z0 = Cellular_weight_samples( hash_z0 ) * JITTER_WINDOW + float4(0.0, 0.0, 0.0, 0.0);
    hash_z1 = Cellular_weight_samples( hash_z1 ) * JITTER_WINDOW + float4(1.0, 1.0, 1.0, 1.0);

    //	find the squared distance to each point
    float4 dx1 = Pf.xxxx - hash_x0;
    float4 dy1 = Pf.yyyy - hash_y0;
    float4 dz1 = Pf.zzzz - hash_z0;
    float4 dx2 = Pf.xxxx - hash_x1;
    float4 dy2 = Pf.yyyy - hash_y1;
    float4 dz2 = Pf.zzzz - hash_z1;
    float4 d1 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;
    float4 d2 = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;

    //	sum kernels and return
    const float RADIUS = ( 1.0 - JITTER_WINDOW );
    d1 *= ( ( 1.0 / RADIUS ) * ( 1.0 / RADIUS ) );
    d2 *= ( ( 1.0 / RADIUS ) * ( 1.0 / RADIUS ) );
    return dot( Falloff_Xsq_C2( min( d1, float4( 1.0 ) ) ) + Falloff_Xsq_C2( min( d2, float4( 1.0 ) ) ), float4( 1.0 ) );
}
*/


//
//	PolkaDot Noise 2D
//	http://briansharpe.files.wordpress.com/2011/12/polkadotsample.jpg
//	http://briansharpe.files.wordpress.com/2012/01/polkaboxsample.jpg
//	TODO, these images have random intensity and random radius.  This noise now has intensity as proportion to radius.  Images need updated.  TODO
//
//	Generates a noise of smooth falloff polka dots.
//	Allow for control on radius.  Intensity is proportional to radius
//	Return value range of 0.0->1.0
//
float PolkaDot2D( 	float2 P,
                    float radius_low,		//	radius range is 0.0->1.0
                    float radius_high	)
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash = FAST32_hash_2D_Cell( Pi );

    //	user variables
    float RADIUS = max( 0.0, radius_low + hash.z * ( radius_high - radius_low ) );
    float VALUE = RADIUS / max( radius_high, radius_low );	//	new keep value in proportion to radius.  Behaves better when used for bumpmapping, distortion and displacement

    //	calc the noise and return
    RADIUS = 2.0/RADIUS;
    Pf *= RADIUS;
    Pf -= ( RADIUS - 1.0 );
    Pf += hash.xy * ( RADIUS - 2.0 );
    //Pf *= Pf;		//	this gives us a cool box looking effect
    return Falloff_Xsq_C2( min( dot( Pf, Pf ), 1.0 ) ) * VALUE;
}
//	PolkaDot2D_FixedRadius, PolkaDot2D_FixedValue, PolkaDot2D_FixedRadius_FixedValue TODO


//
//	PolkaDot Noise 3D
//	http://briansharpe.files.wordpress.com/2011/12/polkadotsample.jpg
//	http://briansharpe.files.wordpress.com/2012/01/polkaboxsample.jpg
//	TODO, these images have random intensity and random radius.  This noise now has intensity as proportion to radius.  Images need updated.  TODO
//
//	Generates a noise of smooth falloff polka dots.
//	Allow for control on radius.  Intensity is proportional to radius
//	Return value range of 0.0->1.0
//
float PolkaDot3D( 	float3 P,
                    float radius_low,		//	radius range is 0.0->1.0
                    float radius_high	)
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    float4 hash = FAST32_hash_3D_Cell( Pi );

    //	user variables
    float RADIUS = max( 0.0, radius_low + hash.w * ( radius_high - radius_low ) );
    float VALUE = RADIUS / max( radius_high, radius_low );	//	new keep value in proportion to radius.  Behaves better when used for bumpmapping, distortion and displacement

    //	calc the noise and return
    RADIUS = 2.0/RADIUS;
    Pf *= RADIUS;
    Pf -= ( RADIUS - 1.0 );
    Pf += hash.xyz * ( RADIUS - 2.0 );
    //Pf *= Pf;		//	this gives us a cool box looking effect
    return Falloff_Xsq_C2( min( dot( Pf, Pf ), 1.0 ) ) * VALUE;
}
//	PolkaDot3D_FixedRadius, PolkaDot3D_FixedValue, PolkaDot3D_FixedRadius_FixedValue TODO


//
//	Stars2D
//	http://briansharpe.files.wordpress.com/2011/12/starssample.jpg
//
//	procedural texture for creating a starry background.  ( looks good when combined with a nebula/space-like colour texture )
//	NOTE:  Any serious game implementation should hard-code these parameter values for efficiency.
//
//	Return value range of 0.0->1.0
//
float Stars2D(	float2 P,
                float probability_threshold,		//	probability a star will be drawn  ( 0.0->1.0 )
                float max_dimness,					//	the maximal dimness of a star ( 0.0->1.0   0.0 = all stars bright,  1.0 = maximum variation )
                float two_over_radius )				//	fixed radius for the stars.  radius range is 0.0->1.0  shader requires 2.0/radius as input.
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash = FAST32_hash_2D_Cell( Pi );
    //float4 hash = FAST32_hash_2D( Pi * 2.0 );		//	Need to multiply by 2.0 here because we want to use all 4 corners once per cell.  No sharing with other cells.  It helps if the hash function has an odd domain.
    //float4 hash = BBS_hash_2D( Pi * 2.0 );
    //float4 hash = SGPP_hash_2D( Pi * 2.0 );
    //float4 hash = BBS_hash_hq_2D( Pi * 2.0 );

    //	user variables
    float VALUE = 1.0 - max_dimness * hash.z;

    //	calc the noise and return
    Pf *= two_over_radius;
    Pf -= ( two_over_radius - 1.0 );
    Pf += hash.xy * ( two_over_radius - 2.0 );
    return ( hash.w < probability_threshold ) ? ( Falloff_Xsq_C1( min( dot( Pf, Pf ), 1.0 ) ) * VALUE ) : 0.0;
}


//
//	SimplexPerlin2D  ( simplex gradient noise )
//	Perlin noise over a simplex (triangular) grid
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2012/01/simplexperlinsample.jpg
//
//	Implementation originally based off Stefan Gustavson's and Ian McEwan's work at...
//	http://github.com/ashima/webgl-noise
//
float SimplexPerlin2D( float2 P )
{
    //	simplex math constants
    const float SKEWFACTOR = 0.36602540378443864676372317075294;			// 0.5*(sqrt(3.0)-1.0)
    const float UNSKEWFACTOR = 0.21132486540518711774542560974902;			// (3.0-sqrt(3.0))/6.0
    const float SIMPLEX_TRI_HEIGHT = 0.70710678118654752440084436210485;	// sqrt( 0.5 )	height of simplex triangle
    const float3 SIMPLEX_POINTS = float3( 1.0-UNSKEWFACTOR, -UNSKEWFACTOR, 1.0-2.0*UNSKEWFACTOR );		//	vertex info for simplex triangle

    //	establish our grid cell.
    P *= SIMPLEX_TRI_HEIGHT;		// scale space so we can have an approx feature size of 1.0  ( optional )
    float2 Pi = floor( P + dot( P, float( SKEWFACTOR ).xx ) );

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	establish vectors to the 3 corners of our simplex triangle
    float2 v0 = Pi - dot( Pi, float( UNSKEWFACTOR ).xx ) - P;
    float4 v1pos_v1hash = (v0.x < v0.y) ? float4(SIMPLEX_POINTS.xy, hash_x.y, hash_y.y) : float4(SIMPLEX_POINTS.yx, hash_x.z, hash_y.z);
    float4 v12 = float4( v1pos_v1hash.xy, SIMPLEX_POINTS.zz ) + v0.xyxy;

    //	calculate the dotproduct of our 3 corner vectors with 3 random normalized vectors
    float3 grad_x = float3( hash_x.x, v1pos_v1hash.z, hash_x.w ) - 0.49999;
    float3 grad_y = float3( hash_y.x, v1pos_v1hash.w, hash_y.w ) - 0.49999;
    float3 grad_results = rsqrt( grad_x * grad_x + grad_y * grad_y ) * ( grad_x * float3( v0.x, v12.xz ) + grad_y * float3( v0.y, v12.yw ) );

    //	Normalization factor to scale the final result to a strict 1.0->-1.0 range
    //	x = ( sqrt( 0.5 )/sqrt( 0.75 ) ) * 0.5
    //	NF = 1.0 / ( x * ( ( 0.5 – x*x ) ^ 4 ) * 2.0 )
    //	http://briansharpe.wordpress.com/2012/01/13/simplex-noise/##comment-36
    const float FINAL_NORMALIZATION = 99.204334582718712976990005025589;

    //	evaluate the surflet, sum and return
    float3 m = float3( v0.x, v12.xz ) * float3( v0.x, v12.xz ) + float3( v0.y, v12.yw ) * float3( v0.y, v12.yw );
    m = max(0.5 - m, 0.0);		//	The 0.5 here is SIMPLEX_TRI_HEIGHT^2
    m = m*m;
    return dot(m*m, grad_results) * FINAL_NORMALIZATION;
}

//
//	SimplexPolkaDot2D
//	polkadots over a simplex (triangular) grid
//	Return value range of 0.0->1.0
//	http://briansharpe.files.wordpress.com/2012/01/simplexpolkadotsample.jpg
//
float SimplexPolkaDot2D( 	float2 P,
                            float radius, 		//	radius range is 0.0->1.0
                            float max_dimness )	//	the maximal dimness of a dot ( 0.0->1.0   0.0 = all dots bright,  1.0 = maximum variation )
{
    //	simplex math based off Stefan Gustavson's and Ian McEwan's work at...
    //	http://github.com/ashima/webgl-noise

    //	simplex math constants
    const float SKEWFACTOR = 0.36602540378443864676372317075294;			// 0.5*(sqrt(3.0)-1.0)
    const float UNSKEWFACTOR = 0.21132486540518711774542560974902;			// (3.0-sqrt(3.0))/6.0
    const float SIMPLEX_TRI_HEIGHT = 0.70710678118654752440084436210485;	// sqrt( 0.5 )	height of simplex triangle
    const float INV_SIMPLEX_TRI_HALF_EDGELEN = 2.4494897427831780981972840747059;	// sqrt( 0.75 )/(2.0*sqrt( 0.5 ))
    const float3 SIMPLEX_POINTS = float3( 1.0-UNSKEWFACTOR, -UNSKEWFACTOR, 1.0-2.0*UNSKEWFACTOR );		//	vertex info for simplex triangle

    //	establish our grid cell.
    P *= SIMPLEX_TRI_HEIGHT;		// scale space so we can have an approx feature size of 1.0  ( optional )
    float2 Pi = floor( P + dot( P, float( SKEWFACTOR ).xx ) );

    //	establish vectors to the 4 corners of our simplex triangle
    float2 v0 = ( Pi - dot( Pi, float( UNSKEWFACTOR ).xx ) - P );
    float4 v0123_x = float4( 0.0, SIMPLEX_POINTS.xyz ) + v0.x;
    float4 v0123_y = float4( 0.0, SIMPLEX_POINTS.yxz ) + v0.y;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash = FAST32_hash_2D( Pi );
    //float4 hash = BBS_hash_2D( Pi );
    //float4 hash = SGPP_hash_2D( Pi );
    //float4 hash = BBS_hash_hq_2D( Pi );

    //	apply user controls
    radius = INV_SIMPLEX_TRI_HALF_EDGELEN/radius;		//	INV_SIMPLEX_TRI_HALF_EDGELEN here is to scale to a nice 0.0->1.0 range
    v0123_x *= radius;
    v0123_y *= radius;

    //	return a smooth falloff from the closest point.  ( we use a f(x)=(1.0-x*x)^3 falloff )
    float4 point_distance = max( 0, 1.0 - ( v0123_x*v0123_x + v0123_y*v0123_y ) );
    point_distance = point_distance*point_distance*point_distance;
    return dot( 1.0 - hash * max_dimness, point_distance );
}


//
//	SimplexCellular2D
//	cellular noise over a simplex (triangular) grid
//	Return value range of 0.0->~1.0
//	http://briansharpe.files.wordpress.com/2012/01/simplexcellularsample.jpg
//
//	TODO:  scaling of return value to strict 0.0->1.0 range
//
float SimplexCellular2D( float2 P )
{
    //	simplex math based off Stefan Gustavson's and Ian McEwan's work at...
    //	http://github.com/ashima/webgl-noise

    //	simplex math constants
    const float SKEWFACTOR = 0.36602540378443864676372317075294;			// 0.5*(sqrt(3.0)-1.0)
    const float UNSKEWFACTOR = 0.21132486540518711774542560974902;			// (3.0-sqrt(3.0))/6.0
    const float SIMPLEX_TRI_HEIGHT = 0.70710678118654752440084436210485;	// sqrt( 0.5 )	height of simplex triangle.
    const float INV_SIMPLEX_TRI_HEIGHT = 1.4142135623730950488016887242097;	//	1.0 / sqrt( 0.5 )
    const float3 SIMPLEX_POINTS = float3( 1.0-UNSKEWFACTOR, -UNSKEWFACTOR, 1.0-2.0*UNSKEWFACTOR ) * INV_SIMPLEX_TRI_HEIGHT;		//	vertex info for simplex triangle

    //	establish our grid cell.
    P *= SIMPLEX_TRI_HEIGHT;		// scale space so we can have an approx feature size of 1.0  ( optional )
    float2 Pi = floor( P + dot( P, float( SKEWFACTOR ).xx ) );

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	push hash values to extremes of jitter window
    const float JITTER_WINDOW = ( 0.10566243270259355887271280487451 * INV_SIMPLEX_TRI_HEIGHT );		// this will guarentee no artifacts.
    hash_x = Cellular_weight_samples( hash_x ) * JITTER_WINDOW;
    hash_y = Cellular_weight_samples( hash_y ) * JITTER_WINDOW;

    //	calculate sq distance to closest point
    float2 p0 = ( ( Pi - dot( Pi, float( UNSKEWFACTOR ).xx ) ) - P ) * INV_SIMPLEX_TRI_HEIGHT;
    hash_x += p0.xxxx;
    hash_y += p0.yyyy;
    hash_x.yzw += SIMPLEX_POINTS.xyz;
    hash_y.yzw += SIMPLEX_POINTS.yxz;
    float4 distsq = hash_x*hash_x + hash_y*hash_y;
    float2 tmp = min( distsq.xy, distsq.zw );
    return min( tmp.x, tmp.y );
}

//
//	Given an arbitrary 3D point this calculates the 4 vectors from the corners of the simplex pyramid to the point
//	It also returns the integer grid index information for the corners
//
void Simplex3D_GetCornerVectors( 	float3 P,					//	input point
                                    out float3 Pi,			//	integer grid index for the origin
                                    out float3 Pi_1,			//	offsets for the 2nd and 3rd corners.  ( the 4th = Pi + 1.0 )
                                    out float3 Pi_2,
                                    out float4 v1234_x,		//	vectors from the 4 corners to the intput point
                                    out float4 v1234_y,
                                    out float4 v1234_z )
{
    //
    //	Simplex math from Stefan Gustavson's and Ian McEwan's work at...
    //	http://github.com/ashima/webgl-noise
    //

    //	simplex math constants
    const float SKEWFACTOR = 1.0/3.0;
    const float UNSKEWFACTOR = 1.0/6.0;
    const float SIMPLEX_CORNER_POS = 0.5;
    const float SIMPLEX_PYRAMID_HEIGHT = 0.70710678118654752440084436210485;	// sqrt( 0.5 )	height of simplex pyramid.

    P *= SIMPLEX_PYRAMID_HEIGHT;		// scale space so we can have an approx feature size of 1.0  ( optional )

    //	Find the vectors to the corners of our simplex pyramid
    Pi = floor( P + dot( P, float( SKEWFACTOR).xxx ) );
    float3 x0 = P - Pi + dot(Pi, float( UNSKEWFACTOR ).xxx );
    float3 g = step(x0.yzx, x0.xyz);
    float3 l = 1.0 - g;
    Pi_1 = min( g.xyz, l.zxy );
    Pi_2 = max( g.xyz, l.zxy );
    float3 x1 = x0 - Pi_1 + UNSKEWFACTOR;
    float3 x2 = x0 - Pi_2 + SKEWFACTOR;
    float3 x3 = x0 - SIMPLEX_CORNER_POS;

    //	pack them into a parallel-friendly arrangement
    v1234_x = float4( x0.x, x1.x, x2.x, x3.x );
    v1234_y = float4( x0.y, x1.y, x2.y, x3.y );
    v1234_z = float4( x0.z, x1.z, x2.z, x3.z );
}

//
//	Calculate the weights for the 3D simplex surflet
//
float4 Simplex3D_GetSurfletWeights( 	float4 v1234_x,
                                    float4 v1234_y,
                                    float4 v1234_z )
{
    //	perlins original implementation uses the surlet falloff formula of (0.6-x*x)^4.
    //	This is buggy as it can cause discontinuities along simplex faces.  (0.5-x*x)^3 solves this and gives an almost identical curve

    //	evaluate surflet. f(x)=(0.5-x*x)^3
    float4 surflet_weights = v1234_x * v1234_x + v1234_y * v1234_y + v1234_z * v1234_z;
    surflet_weights = max(0.5 - surflet_weights, 0.0);		//	0.5 here represents the closest distance (squared) of any simplex pyramid corner to any of its planes.  ie, SIMPLEX_PYRAMID_HEIGHT^2
    return surflet_weights*surflet_weights*surflet_weights;
}



//
//	SimplexPerlin3D  ( simplex gradient noise )
//	Perlin noise over a simplex (tetrahedron) grid
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2012/01/simplexperlinsample.jpg
//
//	Implementation originally based off Stefan Gustavson's and Ian McEwan's work at...
//	http://github.com/ashima/webgl-noise
//
float SimplexPerlin3D(float3 P)
{
    //	calculate the simplex vector and index math
    float3 Pi;
    float3 Pi_1;
    float3 Pi_2;
    float4 v1234_x;
    float4 v1234_y;
    float4 v1234_z;
    Simplex3D_GetCornerVectors( P, Pi, Pi_1, Pi_2, v1234_x, v1234_y, v1234_z );

    //	generate the random vectors
    //	( various hashing methods listed in order of speed )
    float4 hash_0;
    float4 hash_1;
    float4 hash_2;
    FAST32_hash_3D( Pi, Pi_1, Pi_2, hash_0, hash_1, hash_2 );
    //SGPP_hash_3D( Pi, Pi_1, Pi_2, hash_0, hash_1, hash_2 );
    hash_0 -= 0.49999;
    hash_1 -= 0.49999;
    hash_2 -= 0.49999;

    //	evaluate gradients
    float4 grad_results = rsqrt( hash_0 * hash_0 + hash_1 * hash_1 + hash_2 * hash_2 ) * ( hash_0 * v1234_x + hash_1 * v1234_y + hash_2 * v1234_z );

    //	Normalization factor to scale the final result to a strict 1.0->-1.0 range
    //	x = sqrt( 0.75 ) * 0.5
    //	NF = 1.0 / ( x * ( ( 0.5 – x*x ) ^ 3 ) * 2.0 )
    //	http://briansharpe.wordpress.com/2012/01/13/simplex-noise/##comment-36
    const float FINAL_NORMALIZATION = 37.837227241611314102871574478976;

    //	sum with the surflet and return
    return dot( Simplex3D_GetSurfletWeights( v1234_x, v1234_y, v1234_z ), grad_results ) * FINAL_NORMALIZATION;
}

//
//	SimplexCellular3D
//	cellular noise over a simplex (tetrahedron) grid
//	Return value range of 0.0->~1.0
//	http://briansharpe.files.wordpress.com/2012/01/simplexcellularsample.jpg
//
//	TODO:  scaling of return value to strict 0.0->1.0 range
//
float SimplexCellular3D( float3 P )
{
    //	calculate the simplex vector and index math
    float3 Pi;
    float3 Pi_1;
    float3 Pi_2;
    float4 v1234_x;
    float4 v1234_y;
    float4 v1234_z;
    Simplex3D_GetCornerVectors( P, Pi, Pi_1, Pi_2, v1234_x, v1234_y, v1234_z );

    //	generate the random vectors
    //	( various hashing methods listed in order of speed )
    float4 hash_x;
    float4 hash_y;
    float4 hash_z;
    FAST32_hash_3D( Pi, Pi_1, Pi_2, hash_x, hash_y, hash_z );
    //SGPP_hash_3D( Pi, Pi_1, Pi_2, hash_x, hash_y, hash_z );

    //	push hash values to extremes of jitter window
    const float INV_SIMPLEX_PYRAMID_HEIGHT = 1.4142135623730950488016887242097;	//	1.0 / sqrt( 0.5 )   This scales things so to a nice 0.0->1.0 range
    const float JITTER_WINDOW = ( 0.0597865779345250670558198111 * INV_SIMPLEX_PYRAMID_HEIGHT) ;		// this will guarentee no artifacts.
    hash_x = Cellular_weight_samples( hash_x ) * JITTER_WINDOW;
    hash_y = Cellular_weight_samples( hash_y ) * JITTER_WINDOW;
    hash_z = Cellular_weight_samples( hash_z ) * JITTER_WINDOW;

    //	offset the vectors.
    v1234_x *= INV_SIMPLEX_PYRAMID_HEIGHT;
    v1234_y *= INV_SIMPLEX_PYRAMID_HEIGHT;
    v1234_z *= INV_SIMPLEX_PYRAMID_HEIGHT;
    v1234_x += hash_x;
    v1234_y += hash_y;
    v1234_z += hash_z;

    //	calc the distance^2 to the closest point
    float4 distsq = v1234_x*v1234_x + v1234_y*v1234_y + v1234_z*v1234_z;
    return min( min( distsq.x, distsq.y ), min( distsq.z, distsq.w ) );
}


//
//	SimplexPolkaDot3D
//	polkadots over a simplex (tetrahedron) grid
//	Return value range of 0.0->1.0
//	http://briansharpe.files.wordpress.com/2012/01/simplexpolkadotsample.jpg
//
float SimplexPolkaDot3D( 	float3 P,
                            float radius, 		//	radius range is 0.0->1.0
                            float max_dimness )	//	the maximal dimness of a dot ( 0.0->1.0   0.0 = all dots bright,  1.0 = maximum variation )
{
    //	calculate the simplex vector and index math
    float3 Pi;
    float3 Pi_1;
    float3 Pi_2;
    float4 v1234_x;
    float4 v1234_y;
    float4 v1234_z;
    Simplex3D_GetCornerVectors( P, Pi, Pi_1, Pi_2, v1234_x, v1234_y, v1234_z );

    //	calculate the hash
    float4 hash = FAST32_hash_3D( Pi, Pi_1, Pi_2 );

    //	apply user controls
    const float INV_SIMPLEX_TRI_HALF_EDGELEN = 2.3094010767585030580365951220078;	// scale to a 0.0->1.0 range.  2.0 / sqrt( 0.75 )
    radius = INV_SIMPLEX_TRI_HALF_EDGELEN/radius;
    v1234_x *= radius;
    v1234_y *= radius;
    v1234_z *= radius;

    //	return a smooth falloff from the closest point.  ( we use a f(x)=(1.0-x*x)^3 falloff )
    float4 point_distance = max( 0, 1.0 - ( v1234_x*v1234_x + v1234_y*v1234_y + v1234_z*v1234_z ) );
    point_distance = point_distance*point_distance*point_distance;
    return dot( 1.0 - hash * max_dimness, point_distance );
}


//
//	Quintic Hermite Interpolation
//	http://www.rose-hulman.edu/~finn/CCLI/Notes/day09.pdf
//
//  NOTE: maximum value of a hermitequintic interpolation with zero acceleration at the endpoints would be...
//        f(x=0.5) = MAXPOS + MAXVELOCITY * ( ( x - 6x^3 + 8x^4 - 3x^5 ) - ( -4x^3 + 7x^4 -3x^5 ) ) = MAXPOS + MAXVELOCITY * 0.3125
//
//	variable naming conventions:
//	val = value ( position )
//	grad = gradient ( velocity )
//	x = 0.0->1.0 ( time )
//	i = interpolation = a value to be interpolated
//	e = evaluation = a value to be used to calculate the interpolation
//	0 = start
//	1 = end
//
float QuinticHermite( float x, float ival0, float ival1, float egrad0, float egrad1 )		// quintic hermite with start/end acceleration of 0.0
{
    const float3 C0 = float3( -15.0, 8.0, 7.0 );
    const float3 C1 = float3( 6.0, -3.0, -3.0 );
    const float3 C2 = float3( 10.0, -6.0, -4.0 );
    float3 h123 = ( ( ( C0 + C1 * x ) * x ) + C2 ) * ( x*x*x );
    return ival0 + dot( float3( (ival1 - ival0), egrad0, egrad1 ), h123.xyz + float3( 0.0, x, 0.0 ) );
}
float4 QuinticHermite( float x, float4 ival0, float4 ival1, float4 egrad0, float4 egrad1 )		// quintic hermite with start/end acceleration of 0.0
{
    const float3 C0 = float3( -15.0, 8.0, 7.0 );
    const float3 C1 = float3( 6.0, -3.0, -3.0 );
    const float3 C2 = float3( 10.0, -6.0, -4.0 );
    float3 h123 = ( ( ( C0 + C1 * x ) * x ) + C2 ) * ( x*x*x );
    return ival0 + (ival1 - ival0) * h123.xxxx + egrad0 * ( h123.y + x ) + egrad1 * h123.zzzz;
}
float4 QuinticHermite( float x, float2 igrad0, float2 igrad1, float2 egrad0, float2 egrad1 )		// quintic hermite with start/end position and acceleration of 0.0
{
    const float3 C0 = float3( -15.0, 8.0, 7.0 );
    const float3 C1 = float3( 6.0, -3.0, -3.0 );
    const float3 C2 = float3( 10.0, -6.0, -4.0 );
    float3 h123 = ( ( ( C0 + C1 * x ) * x ) + C2 ) * ( x*x*x );
    return float4( egrad1, igrad0 ) * float4( h123.zz, 1.0, 1.0 ) + float4( egrad0, h123.xx ) * float4( ( h123.y + x ).xx, (igrad1 - igrad0) );	//	returns float4( out_ival.xy, out_igrad.xy )
}
void QuinticHermite( 	float x,
                        float4 ival0, float4 ival1,			//	values are interpolated using the gradient arguments
                        float4 igrad_x0, float4 igrad_x1, 	//	gradients are interpolated using eval gradients of 0.0
                        float4 igrad_y0, float4 igrad_y1,
                        float4 egrad0, float4 egrad1, 		//	our evaluation gradients
                        out float4 out_ival, out float4 out_igrad_x, out float4 out_igrad_y )	// quintic hermite with start/end acceleration of 0.0
{
    const float3 C0 = float3( -15.0, 8.0, 7.0 );
    const float3 C1 = float3( 6.0, -3.0, -3.0 );
    const float3 C2 = float3( 10.0, -6.0, -4.0 );
    float3 h123 = ( ( ( C0 + C1 * x ) * x ) + C2 ) * ( x*x*x );
    out_ival = ival0 + (ival1 - ival0) * h123.xxxx + egrad0 * ( h123.y + x ) + egrad1 * h123.zzzz;
    out_igrad_x = igrad_x0 + (igrad_x1 - igrad_x0) * h123.xxxx;	//	NOTE: gradients of 0.0
    out_igrad_y = igrad_y0 + (igrad_y1 - igrad_y0) * h123.xxxx;	//	NOTE: gradients of 0.0
}
void QuinticHermite( 	float x,
                        float4 igrad_x0, float4 igrad_x1, 	//	gradients are interpolated using eval gradients of 0.0
                        float4 igrad_y0, float4 igrad_y1,
                        float4 egrad0, float4 egrad1, 		//	our evaluation gradients
                        out float4 out_ival, out float4 out_igrad_x, out float4 out_igrad_y )	// quintic hermite with start/end position and acceleration of 0.0
{
    const float3 C0 = float3( -15.0, 8.0, 7.0 );
    const float3 C1 = float3( 6.0, -3.0, -3.0 );
    const float3 C2 = float3( 10.0, -6.0, -4.0 );
    float3 h123 = ( ( ( C0 + C1 * x ) * x ) + C2 ) * ( x*x*x );
    out_ival = egrad0 * ( h123.y + x ) + egrad1 * h123.zzzz;
    out_igrad_x = igrad_x0 + (igrad_x1 - igrad_x0) * h123.xxxx;	//	NOTE: gradients of 0.0
    out_igrad_y = igrad_y0 + (igrad_y1 - igrad_y0) * h123.xxxx;	//	NOTE: gradients of 0.0
}
float QuinticHermiteDeriv( float x, float ival0, float ival1, float egrad0, float egrad1 )	// gives the derivative of quintic hermite with start/end acceleration of 0.0
{
    const float3 C0 = float3( 30.0, -15.0, -15.0 );
    const float3 C1 = float3( -60.0, 32.0, 28.0 );
    const float3 C2 = float3( 30.0, -18.0, -12.0 );
    float3 h123 = ( ( ( C1 + C0 * x ) * x ) + C2 ) * ( x*x );
    return dot( float3( (ival1 - ival0), egrad0, egrad1 ), h123.xyz + float3( 0.0, 1.0, 0.0 ) );
}

//
//	Hermite2D
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2012/01/hermitesample.jpg
//
float Hermite2D( float2 P )
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_gradx, hash_grady;
    FAST32_hash_2D( Pi, hash_gradx, hash_grady );

    //	scale the hash values
    hash_gradx = ( hash_gradx - 0.49999);
    hash_grady = ( hash_grady - 0.49999);

#if 1
    //	normalize gradients
    float4 norm = rsqrt( hash_gradx * hash_gradx + hash_grady * hash_grady );
    hash_gradx *= norm;
    hash_grady *= norm;
    const float FINAL_NORM_VAL = 2.2627416997969520780827019587355;
#else
    //	unnormalized gradients
    const float FINAL_NORM_VAL = 3.2;  // 3.2 = 1.0 / ( 0.5 * 0.3125 * 2.0 )
#endif

    //	evaluate the hermite
    float4 qh_results = QuinticHermite( Pf.y, hash_gradx.xy, hash_gradx.zw, hash_grady.xy, hash_grady.zw );
    return QuinticHermite( Pf.x, qh_results.x, qh_results.y, qh_results.z, qh_results.w ) * FINAL_NORM_VAL;
}

//
//	Hermite3D
//	Return value range of -1.0->1.0
//	http://briansharpe.files.wordpress.com/2012/01/hermitesample.jpg
//
float Hermite3D( float3 P )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_gradx0, hash_grady0, hash_gradz0, hash_gradx1, hash_grady1, hash_gradz1;
    FAST32_hash_3D( Pi, hash_gradx0, hash_grady0, hash_gradz0, hash_gradx1, hash_grady1, hash_gradz1 );

    //	scale the hash values
    hash_gradx0 = ( hash_gradx0 - 0.49999);
    hash_grady0 = ( hash_grady0 - 0.49999);
    hash_gradz0 = ( hash_gradz0 - 0.49999);
    hash_gradx1 = ( hash_gradx1 - 0.49999);
    hash_grady1 = ( hash_grady1 - 0.49999);
    hash_gradz1 = ( hash_gradz1 - 0.49999);

#if 1
    //	normalize gradients
    float4 norm0 = rsqrt( hash_gradx0 * hash_gradx0 + hash_grady0 * hash_grady0 + hash_gradz0 * hash_gradz0 );
    hash_gradx0 *= norm0;
    hash_grady0 *= norm0;
    hash_gradz0 *= norm0;
    float4 norm1 = rsqrt( hash_gradx1 * hash_gradx1 + hash_grady1 * hash_grady1 + hash_gradz1 * hash_gradz1 );
    hash_gradx1 *= norm1;
    hash_grady1 *= norm1;
    hash_gradz1 *= norm1;
    const float FINAL_NORM_VAL = 1.8475208614068024464292760976063;
#else
    //	unnormalized gradients
    const float FINAL_NORM_VAL = (1.0/0.46875);  // = 1.0 / ( 0.5 * 0.3125 * 3.0 )
#endif

    //	evaluate the hermite
    float4 ival_results, igrad_results_x, igrad_results_y;
    QuinticHermite( Pf.z, hash_gradx0, hash_gradx1, hash_grady0, hash_grady1, hash_gradz0, hash_gradz1, ival_results, igrad_results_x, igrad_results_y );
    float4 qh_results = QuinticHermite( Pf.y, float4(ival_results.xy, igrad_results_x.xy), float4(ival_results.zw, igrad_results_x.zw), float4( igrad_results_y.xy, 0.0, 0.0 ), float4( igrad_results_y.zw, 0.0, 0.0 ) );
    return QuinticHermite( Pf.x, qh_results.x, qh_results.y, qh_results.z, qh_results.w ) * FINAL_NORM_VAL;
}

//
//	ValueHermite2D
//	Return value range of -1.0->1.0
//	( allows for a blend between value and hermite noise )
//	http://briansharpe.files.wordpress.com/2012/01/valuehermitesample.jpg
//
float ValueHermite2D( 	float2 P,
                        float value_scale,			//	value_scale = 2.0*MAXVALUE
                        float gradient_scale, 		//	gradient_scale = 2.0*MAXGRADIENT
                        float normalization_val )	//	normalization_val = ( 1.0 / ( MAXVALUE + MAXGRADIENT * 0.3125 * 2.0 ) )
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_value, hash_gradx, hash_grady;
    FAST32_hash_2D( Pi, hash_value, hash_gradx, hash_grady );

    //	scale the hash values
    hash_gradx = ( hash_gradx - 0.49999) * gradient_scale;
    hash_grady = ( hash_grady - 0.49999) * gradient_scale;		//	hmmm...   should we normalize gradients?
    hash_value = ( hash_value - 0.5) * value_scale;

    //	evaluate the hermite
    float4 qh_results = QuinticHermite( Pf.y, float4(hash_value.xy, hash_gradx.xy), float4(hash_value.zw, hash_gradx.zw), float4( hash_grady.xy, 0.0, 0.0 ), float4( hash_grady.zw, 0.0, 0.0 ) );
    return QuinticHermite( Pf.x, qh_results.x, qh_results.y, qh_results.z, qh_results.w ) * normalization_val;
}


//
//	ValueHermite3D
//	Return value range of -1.0->1.0
//	( allows for a blend between value and hermite noise )
//	http://briansharpe.files.wordpress.com/2012/01/valuehermitesample.jpg
//
float ValueHermite3D( 	float3 P,
                    float value_scale,			//	value_scale = 2.0*MAXVALUE
                    float gradient_scale, 		//	gradient_scale = 2.0*MAXGRADIENT
                    float normalization_val )	//	normalization_val = ( 1.0 / ( MAXVALUE + MAXGRADIENT * 0.3125 * 3.0 ) )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_value0, hash_gradx0, hash_grady0, hash_gradz0, hash_value1, hash_gradx1, hash_grady1, hash_gradz1;
    FAST32_hash_3D( Pi, hash_value0, hash_gradx0, hash_grady0, hash_gradz0, hash_value1, hash_gradx1, hash_grady1, hash_gradz1 );

    //	scale the hash values
    hash_gradx0 = ( hash_gradx0 - 0.49999) * gradient_scale;
    hash_grady0 = ( hash_grady0 - 0.49999) * gradient_scale;
    hash_gradz0 = ( hash_gradz0 - 0.49999) * gradient_scale;
    hash_gradx1 = ( hash_gradx1 - 0.49999) * gradient_scale;
    hash_grady1 = ( hash_grady1 - 0.49999) * gradient_scale;
    hash_gradz1 = ( hash_gradz1 - 0.49999) * gradient_scale;		//	hmmm...   should we normalize gradients?
    hash_value0 = ( hash_value0 - 0.5) * value_scale;
    hash_value1 = ( hash_value1 - 0.5) * value_scale;

    //	evaluate the hermite
    float4 ival_results, igrad_results_x, igrad_results_y;
    QuinticHermite( Pf.z, hash_value0, hash_value1, hash_gradx0, hash_gradx1, hash_grady0, hash_grady1, hash_gradz0, hash_gradz1, ival_results, igrad_results_x, igrad_results_y );
    float4 qh_results = QuinticHermite( Pf.y, float4(ival_results.xy, igrad_results_x.xy), float4(ival_results.zw, igrad_results_x.zw), float4( igrad_results_y.xy, 0.0, 0.0 ), float4( igrad_results_y.zw, 0.0, 0.0 ) );
    return QuinticHermite( Pf.x, qh_results.x, qh_results.y, qh_results.z, qh_results.w ) * normalization_val;
}



//
//	Derivative Noises
//

//
//	Value2D_Deriv
//	Value2D noise with derivatives
//	returns float3( value, xderiv, yderiv )
//
float3 Value2D_Deriv( float2 P )
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    float4 hash = FAST32_hash_2D( Pi );

    //	blend result and return
    float4 blend = Interpolation_C2_InterpAndDeriv( Pf );
    float4 res0 = lerp( hash.xyxz, hash.zwyw, blend.yyxx );
    return float3( res0.x, 0.0, 0.0 ) + ( res0.yyw - res0.xxz ) * blend.xzw;
}

//
//	Value3D_Deriv
//	Value3D noise with derivatives
//	returns float4( value, xderiv, yderiv, zderiv )
//
float4 Value3D_Deriv( float3 P )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_lowz, hash_highz;
    FAST32_hash_3D( Pi, hash_lowz, hash_highz );

    //	blend the results and return
    float3 blend = Interpolation_C2( Pf );
    float4 res0 = lerp( hash_lowz, hash_highz, blend.z );
    float4 res1 = lerp( res0.xyxz, res0.zwyw, blend.yyxx );
    float4 res3 = lerp( float4( hash_lowz.xy, hash_highz.xy ), float4( hash_lowz.zw, hash_highz.zw ), blend.y );
    float2 res4 = lerp( res3.xz, res3.yw, blend.x );
    return float4( res1.x, 0.0, 0.0, 0.0 ) + ( float4( res1.yyw, res4.y ) - float4( res1.xxz, res4.x ) ) * float4( blend.x, Interpolation_C2_Deriv( Pf ) );
}


//
//	Perlin2D_Deriv
//	Classic Perlin 2D noise with derivatives
//	returns float3( value, xderiv, yderiv )
//
float3 Perlin2D_Deriv( float2 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Perlin2D_Deriv.glsl

    // establish our grid cell and unit position
    float2 Pi = floor(P);
    float4 Pf_Pfmin1 = P.xyxy - float4( Pi, Pi + 1.0 );

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	calculate the gradient results
    float4 grad_x = hash_x - 0.49999;
    float4 grad_y = hash_y - 0.49999;
    float4 norm = rsqrt( grad_x * grad_x + grad_y * grad_y );
    grad_x *= norm;
    grad_y *= norm;
    float4 dotval = ( grad_x * Pf_Pfmin1.xzxz + grad_y * Pf_Pfmin1.yyww );

    //	Convert our data to a more parallel format
    float3 dotval0_grad0 = float3( dotval.x, grad_x.x, grad_y.x );
    float3 dotval1_grad1 = float3( dotval.y, grad_x.y, grad_y.y );
    float3 dotval2_grad2 = float3( dotval.z, grad_x.z, grad_y.z );
    float3 dotval3_grad3 = float3( dotval.w, grad_x.w, grad_y.w );

    //	evaluate common constants
    float3 k0_gk0 = dotval1_grad1 - dotval0_grad0;
    float3 k1_gk1 = dotval2_grad2 - dotval0_grad0;
    float3 k2_gk2 = dotval3_grad3 - dotval2_grad2 - k0_gk0;

    //	C2 Interpolation
    float4 blend = Interpolation_C2_InterpAndDeriv( Pf_Pfmin1.xy );

    //	calculate final noise + deriv
    float3 results = dotval0_grad0
                    + blend.x * k0_gk0
                    + blend.y * ( k1_gk1 + blend.x * k2_gk2 );

    results.yz += blend.zw * ( float2( k0_gk0.x, k1_gk1.x ) + blend.yx * k2_gk2.xx );

    return results * 1.4142135623730950488016887242097;  // scale things to a strict -1.0->1.0 range  *= 1.0/sqrt(0.5)
}

//
//	Perlin3D_Deriv
//	Classic Perlin 3D noise with derivatives
//	returns float4( value, xderiv, yderiv, zderiv )
//
float4 Perlin3D_Deriv( float3 P )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;
    float3 Pf_min1 = Pf - 1.0;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hashx0, hashy0, hashz0, hashx1, hashy1, hashz1;
    FAST32_hash_3D( Pi, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1 );
    //SGPP_hash_3D( Pi, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1 );

    //	calculate the gradients
    float4 grad_x0 = hashx0 - 0.49999;
    float4 grad_y0 = hashy0 - 0.49999;
    float4 grad_z0 = hashz0 - 0.49999;
    float4 grad_x1 = hashx1 - 0.49999;
    float4 grad_y1 = hashy1 - 0.49999;
    float4 grad_z1 = hashz1 - 0.49999;
    float4 norm_0 = rsqrt( grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0 );
    float4 norm_1 = rsqrt( grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1 );
    grad_x0 *= norm_0;
    grad_y0 *= norm_0;
    grad_z0 *= norm_0;
    grad_x1 *= norm_1;
    grad_y1 *= norm_1;
    grad_z1 *= norm_1;

    //	calculate the dot products
    float4 dotval_0 = float2( Pf.x, Pf_min1.x ).xyxy * grad_x0 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y0 + Pf.zzzz * grad_z0;
    float4 dotval_1 = float2( Pf.x, Pf_min1.x ).xyxy * grad_x1 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1;

    //
    //	NOTE:  the following is based off Milo Yips derivation, but modified for parallel execution
    //	http://stackoverflow.com/a/14141774
    //

    //	Convert our data to a more parallel format
    float4 dotval0_grad0 = float4( dotval_0.x, grad_x0.x, grad_y0.x, grad_z0.x );
    float4 dotval1_grad1 = float4( dotval_0.y, grad_x0.y, grad_y0.y, grad_z0.y );
    float4 dotval2_grad2 = float4( dotval_0.z, grad_x0.z, grad_y0.z, grad_z0.z );
    float4 dotval3_grad3 = float4( dotval_0.w, grad_x0.w, grad_y0.w, grad_z0.w );
    float4 dotval4_grad4 = float4( dotval_1.x, grad_x1.x, grad_y1.x, grad_z1.x );
    float4 dotval5_grad5 = float4( dotval_1.y, grad_x1.y, grad_y1.y, grad_z1.y );
    float4 dotval6_grad6 = float4( dotval_1.z, grad_x1.z, grad_y1.z, grad_z1.z );
    float4 dotval7_grad7 = float4( dotval_1.w, grad_x1.w, grad_y1.w, grad_z1.w );

    //	evaluate common constants
    float4 k0_gk0 = dotval1_grad1 - dotval0_grad0;
    float4 k1_gk1 = dotval2_grad2 - dotval0_grad0;
    float4 k2_gk2 = dotval4_grad4 - dotval0_grad0;
    float4 k3_gk3 = dotval3_grad3 - dotval2_grad2 - k0_gk0;
    float4 k4_gk4 = dotval5_grad5 - dotval4_grad4 - k0_gk0;
    float4 k5_gk5 = dotval6_grad6 - dotval4_grad4 - k1_gk1;
    float4 k6_gk6 = (dotval7_grad7 - dotval6_grad6) - (dotval5_grad5 - dotval4_grad4) - k3_gk3;

    //	C2 Interpolation
    float3 blend = Interpolation_C2( Pf );
    float3 blendDeriv = Interpolation_C2_Deriv( Pf );

    //	calculate final noise + deriv
    float u = blend.x;
    float v = blend.y;
    float w = blend.z;

    float4 result = dotval0_grad0
        + u * ( k0_gk0 + v * k3_gk3 )
        + v * ( k1_gk1 + w * k5_gk5 )
        + w * ( k2_gk2 + u * ( k4_gk4 + v * k6_gk6 ) );

    result.y += dot( float4( k0_gk0.x, k3_gk3.x * v, float2( k4_gk4.x, k6_gk6.x * v ) * w ), ( blendDeriv.xxxx ) );
    result.z += dot( float4( k1_gk1.x, k3_gk3.x * u, float2( k5_gk5.x, k6_gk6.x * u ) * w ), ( blendDeriv.yyyy ) );
    result.w += dot( float4( k2_gk2.x, k4_gk4.x * u, float2( k5_gk5.x, k6_gk6.x * u ) * v ), ( blendDeriv.zzzz ) );

    //	normalize and return
    result *= 1.1547005383792515290182975610039;		//	(optionally) scale things to a strict -1.0->1.0 range    *= 1.0/sqrt(0.75)
    return result;
}

//
//	PerlinSurflet2D_Deriv
//	Perlin Surflet 2D noise with derivatives
//	returns float3( value, xderiv, yderiv )
//
float3 PerlinSurflet2D_Deriv( float2 P )
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float4 Pf_Pfmin1 = P.xyxy - float4( Pi, Pi + 1.0 );

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	calculate the gradient results
    float4 grad_x = hash_x - 0.49999;
    float4 grad_y = hash_y - 0.49999;
    float4 norm = rsqrt( grad_x * grad_x + grad_y * grad_y );
    grad_x *= norm;
    grad_y *= norm;
    float4 grad_results = grad_x * Pf_Pfmin1.xzxz + grad_y * Pf_Pfmin1.yyww;

    //	eval the surflet
    float4 m = Pf_Pfmin1 * Pf_Pfmin1;
    m = m.xzxz + m.yyww;
    m = max(1.0 - m, 0.0);
    float4 m2 = m*m;
    float4 m3 = m*m2;

    //	calc the deriv
    float4 temp = -6.0 * m2 * grad_results;
    float xderiv = dot( temp, Pf_Pfmin1.xzxz ) + dot( m3, grad_x );
    float yderiv = dot( temp, Pf_Pfmin1.yyww ) + dot( m3, grad_y );

    //	sum the surflets and return all results combined in a float3
    const float FINAL_NORMALIZATION = 2.3703703703703703703703703703704;		//	scales the final result to a strict 1.0->-1.0 range
    return float3( dot( m3, grad_results ), xderiv, yderiv ) * FINAL_NORMALIZATION;
}


//
//	PerlinSurflet3D_Deriv
//	Perlin Surflet 3D noise with derivatives
//	returns float4( value, xderiv, yderiv, zderiv )
//
float4 PerlinSurflet3D_Deriv( float3 P )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;
    float3 Pf_min1 = Pf - 1.0;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hashx0, hashy0, hashz0, hashx1, hashy1, hashz1;
    FAST32_hash_3D( Pi, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1 );
    //SGPP_hash_3D( Pi, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1 );

    //	calculate the gradients
    float4 grad_x0 = hashx0 - 0.49999;
    float4 grad_y0 = hashy0 - 0.49999;
    float4 grad_z0 = hashz0 - 0.49999;
    float4 norm_0 = rsqrt( grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0 );
    grad_x0 *= norm_0;
    grad_y0 *= norm_0;
    grad_z0 *= norm_0;
    float4 grad_x1 = hashx1 - 0.49999;
    float4 grad_y1 = hashy1 - 0.49999;
    float4 grad_z1 = hashz1 - 0.49999;
    float4 norm_1 = rsqrt( grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1 );
    grad_x1 *= norm_1;
    grad_y1 *= norm_1;
    grad_z1 *= norm_1;
    float4 grad_results_0 = float2( Pf.x, Pf_min1.x ).xyxy * grad_x0 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y0 + Pf.zzzz * grad_z0;
    float4 grad_results_1 = float2( Pf.x, Pf_min1.x ).xyxy * grad_x1 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1;

    //	get lengths in the x+y plane
    float3 Pf_sq = Pf*Pf;
    float3 Pf_min1_sq = Pf_min1*Pf_min1;
    float4 vecs_len_sq = float2( Pf_sq.x, Pf_min1_sq.x ).xyxy + float2( Pf_sq.y, Pf_min1_sq.y ).xxyy;

    //	evaluate the surflet
    float4 m_0 = vecs_len_sq + Pf_sq.zzzz;
    m_0 = max(1.0 - m_0, 0.0);
    float4 m2_0 = m_0*m_0;
    float4 m3_0 = m_0*m2_0;

    float4 m_1 = vecs_len_sq + Pf_min1_sq.zzzz;
    m_1 = max(1.0 - m_1, 0.0);
    float4 m2_1 = m_1*m_1;
    float4 m3_1 = m_1*m2_1;

    //	calc the deriv
    float4 temp_0 = -6.0 * m2_0 * grad_results_0;
    float xderiv_0 = dot( temp_0, float2( Pf.x, Pf_min1.x ).xyxy ) + dot( m3_0, grad_x0 );
    float yderiv_0 = dot( temp_0, float2( Pf.y, Pf_min1.y ).xxyy ) + dot( m3_0, grad_y0 );
    float zderiv_0 = dot( temp_0, Pf.zzzz ) + dot( m3_0, grad_z0 );

    float4 temp_1 = -6.0 * m2_1 * grad_results_1;
    float xderiv_1 = dot( temp_1, float2( Pf.x, Pf_min1.x ).xyxy ) + dot( m3_1, grad_x1 );
    float yderiv_1 = dot( temp_1, float2( Pf.y, Pf_min1.y ).xxyy ) + dot( m3_1, grad_y1 );
    float zderiv_1 = dot( temp_1, Pf_min1.zzzz ) + dot( m3_1, grad_z1 );

    const float FINAL_NORMALIZATION = 2.3703703703703703703703703703704;	//	scales the final result to a strict 1.0->-1.0 range
    return float4( dot( m3_0, grad_results_0 ) + dot( m3_1, grad_results_1 ) , float3(xderiv_0,yderiv_0,zderiv_0) + float3(xderiv_1,yderiv_1,zderiv_1) ) * FINAL_NORMALIZATION;
}

//
//	Cellular2D_Deriv
//	Cellular 2D noise with derivatives
//	returns float3( value, xderiv, yderiv )
//
float3 Cellular2D_Deriv(float2 P)
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	generate the 4 random points
    //	restrict the random point offset to eliminate artifacts
    //	we'll improve the variance of the noise by pushing the points to the extremes of the jitter window
    const float JITTER_WINDOW = 0.25;	// 0.25 will guarentee no artifacts.  0.25 is the intersection on x of graphs f(x)=( (0.5+(0.5-x))^2 + (0.5-x)^2 ) and f(x)=( (0.5+x)^2 + x^2 )
    hash_x = Cellular_weight_samples( hash_x ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y = Cellular_weight_samples( hash_y ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);

    //	return the closest squared distance ( + derivs )
    //  thanks to Jonathan Dupuy for the initial implementation
    float4 dx = Pf.xxxx - hash_x;
    float4 dy = Pf.yyyy - hash_y;
    float4 d = dx * dx + dy * dy;
    float3 t1 = d.x < d.y ? float3( d.x, dx.x, dy.x ) : float3( d.y, dx.y, dy.y );
    float3 t2 = d.z < d.w ? float3( d.z, dx.z, dy.z ) : float3( d.w, dx.w, dy.w );
    return ( t1.x < t2.x ? t1 : t2 ) * float3( 1.0, 2.0, 2.0 ) * ( 1.0 / 1.125 ); //	scale return value from 0.0->1.125 to 0.0->1.0  ( 0.75^2 * 2.0  == 1.125 )
}

//
//	Cellular3D Deriv
//	Cellular3D noise with derivatives
//	returns float3( value, xderiv, yderiv )
//
float4 Cellular3D_Deriv(float3 P)
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1;
    FAST32_hash_3D( Pi, hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1 );
    //SGPP_hash_3D( Pi, hash_x0, hash_y0, hash_z0, hash_x1, hash_y1, hash_z1 );

    //	generate the 8 random points
    //	restrict the random point offset to eliminate artifacts
    //	we'll improve the variance of the noise by pushing the points to the extremes of the jitter window
    const float JITTER_WINDOW = 0.166666666;	// 0.166666666 will guarentee no artifacts. It is the intersection on x of graphs f(x)=( (0.5 + (0.5-x))^2 + 2*((0.5-x)^2) ) and f(x)=( 2 * (( 0.5 + x )^2) + x * x )
    hash_x0 = Cellular_weight_samples( hash_x0 ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y0 = Cellular_weight_samples( hash_y0 ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);
    hash_x1 = Cellular_weight_samples( hash_x1 ) * JITTER_WINDOW + float4(0.0, 1.0, 0.0, 1.0);
    hash_y1 = Cellular_weight_samples( hash_y1 ) * JITTER_WINDOW + float4(0.0, 0.0, 1.0, 1.0);
    hash_z0 = Cellular_weight_samples( hash_z0 ) * JITTER_WINDOW + float4(0.0, 0.0, 0.0, 0.0);
    hash_z1 = Cellular_weight_samples( hash_z1 ) * JITTER_WINDOW + float4(1.0, 1.0, 1.0, 1.0);

    //	return the closest squared distance ( + derivs )
    //  thanks to Jonathan Dupuy for the initial implementation
    float4 dx1 = Pf.xxxx - hash_x0;
    float4 dy1 = Pf.yyyy - hash_y0;
    float4 dz1 = Pf.zzzz - hash_z0;
    float4 dx2 = Pf.xxxx - hash_x1;
    float4 dy2 = Pf.yyyy - hash_y1;
    float4 dz2 = Pf.zzzz - hash_z1;
    float4 d1 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;
    float4 d2 = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;
    float4 r1 = d1.x < d1.y ? float4( d1.x, dx1.x, dy1.x, dz1.x ) : float4( d1.y, dx1.y, dy1.y, dz1.y );
    float4 r2 = d1.z < d1.w ? float4( d1.z, dx1.z, dy1.z, dz1.z ) : float4( d1.w, dx1.w, dy1.w, dz1.w );
    float4 r3 = d2.x < d2.y ? float4( d2.x, dx2.x, dy2.x, dz2.x ) : float4( d2.y, dx2.y, dy2.y, dz2.y );
    float4 r4 = d2.z < d2.w ? float4( d2.z, dx2.z, dy2.z, dz2.z ) : float4( d2.w, dx2.w, dy2.w, dz2.w );
    float4 t1 = r1.x < r2.x ? r1 : r2;
    float4 t2 = r3.x < r4.x ? r3 : r4;
    return ( t1.x < t2.x ? t1 : t2 ) * float4( 1.0, float( 2.0 ).xxx ) * ( 9.0 / 12.0 );	//	scale return value from 0.0->1.333333 to 0.0->1.0  	(2/3)^2 * 3  == (12/9) == 1.333333
}

//
//	SimplexPerlin2D_Deriv
//	SimplexPerlin2D noise with derivatives
//	returns float3( value, xderiv, yderiv )
//
float3 SimplexPerlin2D_Deriv( float2 P )
{
    //	simplex math constants
    const float SKEWFACTOR = 0.36602540378443864676372317075294;			// 0.5*(sqrt(3.0)-1.0)
    const float UNSKEWFACTOR = 0.21132486540518711774542560974902;			// (3.0-sqrt(3.0))/6.0
    const float SIMPLEX_TRI_HEIGHT = 0.70710678118654752440084436210485;	// sqrt( 0.5 )	height of simplex triangle
    const float3 SIMPLEX_POINTS = float3( 1.0-UNSKEWFACTOR, -UNSKEWFACTOR, 1.0-2.0*UNSKEWFACTOR );		//	vertex info for simplex triangle

    //	establish our grid cell.
    P *= SIMPLEX_TRI_HEIGHT;		// scale space so we can have an approx feature size of 1.0  ( optional )
    float2 Pi = floor( P + dot( P, float( SKEWFACTOR ).xx ) );

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_x, hash_y;
    FAST32_hash_2D( Pi, hash_x, hash_y );
    //SGPP_hash_2D( Pi, hash_x, hash_y );

    //	establish vectors to the 3 corners of our simplex triangle
    float2 v0 = Pi - dot( Pi, float( UNSKEWFACTOR ).xx ) - P;
    float4 v1pos_v1hash = (v0.x < v0.y) ? float4(SIMPLEX_POINTS.xy, hash_x.y, hash_y.y) : float4(SIMPLEX_POINTS.yx, hash_x.z, hash_y.z);
    float4 v12 = float4( v1pos_v1hash.xy, SIMPLEX_POINTS.zz ) + v0.xyxy;

    //	calculate the dotproduct of our 3 corner vectors with 3 random normalized vectors
    float3 grad_x = float3( hash_x.x, v1pos_v1hash.z, hash_x.w ) - 0.49999;
    float3 grad_y = float3( hash_y.x, v1pos_v1hash.w, hash_y.w ) - 0.49999;
    float3 norm = rsqrt( grad_x * grad_x + grad_y * grad_y );
    grad_x *= norm;
    grad_y *= norm;
    float3 grad_results = grad_x * float3( v0.x, v12.xz ) + grad_y * float3( v0.y, v12.yw );

    //	evaluate the surflet
    float3 m = float3( v0.x, v12.xz ) * float3( v0.x, v12.xz ) + float3( v0.y, v12.yw ) * float3( v0.y, v12.yw );
    m = max(0.5 - m, 0.0);		//	The 0.5 here is SIMPLEX_TRI_HEIGHT^2
    float3 m2 = m*m;
    float3 m4 = m2*m2;

    //	calc the deriv
    float3 temp = 8.0 * m2 * m * grad_results;
    float xderiv = dot( temp, float3( v0.x, v12.xz ) ) - dot( m4, grad_x );
    float yderiv = dot( temp, float3( v0.y, v12.yw ) ) - dot( m4, grad_y );

    const float FINAL_NORMALIZATION = 99.204334582718712976990005025589;	//	scales the final result to a strict 1.0->-1.0 range

    //	sum the surflets and return all results combined in a float3
    return float3( dot( m4, grad_results ), xderiv, yderiv ) * FINAL_NORMALIZATION;
}

//
//	SimplexPerlin3D_Deriv
//	SimplexPerlin3D noise with derivatives
//	returns float3( value, xderiv, yderiv, zderiv )
//
float4 SimplexPerlin3D_Deriv(float3 P)
{
    //	calculate the simplex vector and index math
    float3 Pi;
    float3 Pi_1;
    float3 Pi_2;
    float4 v1234_x;
    float4 v1234_y;
    float4 v1234_z;
    Simplex3D_GetCornerVectors( P, Pi, Pi_1, Pi_2, v1234_x, v1234_y, v1234_z );

    //	generate the random vectors
    //	( various hashing methods listed in order of speed )
    float4 hash_0;
    float4 hash_1;
    float4 hash_2;
    FAST32_hash_3D( Pi, Pi_1, Pi_2, hash_0, hash_1, hash_2 );
    //SGPP_hash_3D( Pi, Pi_1, Pi_2, hash_0, hash_1, hash_2 );
    hash_0 -= 0.49999;
    hash_1 -= 0.49999;
    hash_2 -= 0.49999;

    //	normalize random gradient vectors
    float4 norm = rsqrt( hash_0 * hash_0 + hash_1 * hash_1 + hash_2 * hash_2 );
    hash_0 *= norm;
    hash_1 *= norm;
    hash_2 *= norm;

    //	evaluate gradients
    float4 grad_results = hash_0 * v1234_x + hash_1 * v1234_y + hash_2 * v1234_z;

    //	evaluate the surflet f(x)=(0.5-x*x)^3
    float4 m = v1234_x * v1234_x + v1234_y * v1234_y + v1234_z * v1234_z;
    m = max(0.5 - m, 0.0);		//	The 0.5 here is SIMPLEX_PYRAMID_HEIGHT^2
    float4 m2 = m*m;
    float4 m3 = m*m2;

    //	calc the deriv
    float4 temp = -6.0 * m2 * grad_results;
    float xderiv = dot( temp, v1234_x ) + dot( m3, hash_0 );
    float yderiv = dot( temp, v1234_y ) + dot( m3, hash_1 );
    float zderiv = dot( temp, v1234_z ) + dot( m3, hash_2 );

    const float FINAL_NORMALIZATION = 37.837227241611314102871574478976;	//	scales the final result to a strict 1.0->-1.0 range

    //	sum with the surflet and return
    return float4( dot( m3, grad_results ), xderiv, yderiv, zderiv ) * FINAL_NORMALIZATION;
}


//
//	Hermite2D_Deriv
//	Hermite2D noise with derivatives
//	returns float3( value, xderiv, yderiv )
//
float3 Hermite2D_Deriv( float2 P )
{
    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_gradx, hash_grady;
    FAST32_hash_2D( Pi, hash_gradx, hash_grady );

    //	scale the hash values
    hash_gradx = ( hash_gradx - 0.49999);
    hash_grady = ( hash_grady - 0.49999);

#if 1
    //	normalize gradients
    float4 norm = rsqrt( hash_gradx * hash_gradx + hash_grady * hash_grady );
    hash_gradx *= norm;
    hash_grady *= norm;
    const float FINAL_NORM_VAL = 2.2627416997969520780827019587355;
#else
    //	unnormalized gradients
    const float FINAL_NORM_VAL = 3.2;  // 3.2 = 1.0 / ( 0.5 * 0.3125 * 2.0 )
#endif

    //
    //	NOTE:  This stuff can be optimized further.
    //	But it also appears the compiler is doing a lot of that automatically for us anyway
    //

    float4 qh_results_x = QuinticHermite( Pf.y, hash_gradx.xy, hash_gradx.zw, hash_grady.xy, hash_grady.zw );
    float4 qh_results_y = QuinticHermite( Pf.x, hash_grady.xz, hash_grady.yw, hash_gradx.xz, hash_gradx.yw );
    float finalpos = QuinticHermite( Pf.x, qh_results_x.x, qh_results_x.y, qh_results_x.z, qh_results_x.w );
    float deriv_x = QuinticHermiteDeriv( Pf.x, qh_results_x.x, qh_results_x.y, qh_results_x.z, qh_results_x.w );
    float deriv_y = QuinticHermiteDeriv( Pf.y, qh_results_y.x, qh_results_y.y, qh_results_y.z, qh_results_y.w );
    return float3( finalpos, deriv_x, deriv_y ) * FINAL_NORM_VAL;
}

//
//	Hermite3D_Deriv
//	Hermite3D noise with derivatives
//	returns float3( value, xderiv, yderiv, zderiv )
//
float4 Hermite3D_Deriv( float3 P )
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    //	calculate the hash.
    //	( various hashing methods listed in order of speed )
    float4 hash_gradx0, hash_grady0, hash_gradz0, hash_gradx1, hash_grady1, hash_gradz1;
    FAST32_hash_3D( Pi, hash_gradx0, hash_grady0, hash_gradz0, hash_gradx1, hash_grady1, hash_gradz1 );

    //	scale the hash values
    hash_gradx0 = ( hash_gradx0 - 0.49999);
    hash_grady0 = ( hash_grady0 - 0.49999);
    hash_gradz0 = ( hash_gradz0 - 0.49999);
    hash_gradx1 = ( hash_gradx1 - 0.49999);
    hash_grady1 = ( hash_grady1 - 0.49999);
    hash_gradz1 = ( hash_gradz1 - 0.49999);

#if 1
    //	normalize gradients
    float4 norm0 = rsqrt( hash_gradx0 * hash_gradx0 + hash_grady0 * hash_grady0 + hash_gradz0 * hash_gradz0 );
    hash_gradx0 *= norm0;
    hash_grady0 *= norm0;
    hash_gradz0 *= norm0;
    float4 norm1 = rsqrt( hash_gradx1 * hash_gradx1 + hash_grady1 * hash_grady1 + hash_gradz1 * hash_gradz1 );
    hash_gradx1 *= norm1;
    hash_grady1 *= norm1;
    hash_gradz1 *= norm1;
    const float FINAL_NORM_VAL = 1.8475208614068024464292760976063;
#else
    //	unnormalized gradients
    const float FINAL_NORM_VAL = (1.0/0.46875);  // = 1.0 / ( 0.5 * 0.3125 * 3.0 )
#endif

    //
    //	NOTE:  This stuff can be optimized further.
    //	But it also appears the compiler is doing a lot of that automatically for us anyway
    //

    //	drop things from three dimensions to two
    float4 ival_results_z, igrad_results_x_z, igrad_results_y_z;
    QuinticHermite( Pf.z, hash_gradx0, hash_gradx1, hash_grady0, hash_grady1, hash_gradz0, hash_gradz1, ival_results_z, igrad_results_x_z, igrad_results_y_z );

    float4 ival_results_y, igrad_results_x_y, igrad_results_z_y;
    QuinticHermite( Pf.y, 	float4( hash_gradx0.xy, hash_gradx1.xy ), float4( hash_gradx0.zw, hash_gradx1.zw ),
                            float4( hash_gradz0.xy, hash_gradz1.xy ), float4( hash_gradz0.zw, hash_gradz1.zw ),
                            float4( hash_grady0.xy, hash_grady1.xy ), float4( hash_grady0.zw, hash_grady1.zw ),
                            ival_results_y, igrad_results_x_y, igrad_results_z_y );

    //	drop things from two dimensions to one
    float4 qh_results_x = QuinticHermite( Pf.y, float4(ival_results_z.xy, igrad_results_x_z.xy), float4(ival_results_z.zw, igrad_results_x_z.zw), float4( igrad_results_y_z.xy, 0.0, 0.0 ), float4( igrad_results_y_z.zw, 0.0, 0.0 ) );
    float4 qh_results_y = QuinticHermite( Pf.x, float4(ival_results_z.xz, igrad_results_y_z.xz), float4(ival_results_z.yw, igrad_results_y_z.yw), float4( igrad_results_x_z.xz, 0.0, 0.0 ), float4( igrad_results_x_z.yw, 0.0, 0.0 ) );
    float4 qh_results_z = QuinticHermite( Pf.x, float4(ival_results_y.xz, igrad_results_z_y.xz), float4(ival_results_y.yw, igrad_results_z_y.yw), float4( igrad_results_x_y.xz, 0.0, 0.0 ), float4( igrad_results_x_y.yw, 0.0, 0.0 ) );

    //	for each hermite curve calculate the derivative
    float deriv_x = QuinticHermiteDeriv( Pf.x, qh_results_x.x, qh_results_x.y, qh_results_x.z, qh_results_x.w );
    float deriv_y = QuinticHermiteDeriv( Pf.y, qh_results_y.x, qh_results_y.y, qh_results_y.z, qh_results_y.w );
    float deriv_z = QuinticHermiteDeriv( Pf.z, qh_results_z.x, qh_results_z.y, qh_results_z.z, qh_results_z.w );

    //	and also the final noise value off any one of them
    float finalpos = QuinticHermite( Pf.x, qh_results_x.x, qh_results_x.y, qh_results_x.z, qh_results_x.w );

    //	normalize and return results! :)
    return float4( finalpos, deriv_x, deriv_y, deriv_z ) * FINAL_NORM_VAL;
}
