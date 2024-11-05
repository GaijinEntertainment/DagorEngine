// © 2021 NVIDIA Corporation

#ifndef STL_H
#define STL_H

//===========================================================================================================================
// Settings
//===========================================================================================================================

#define STL_D3D                                     0
#define STL_OGL                                     1
#define STL_WINDOW_ORIGIN                           STL_D3D

// Math
#define STL_SIGN_DEFAULT                            STL_SIGN_FAST
#define STL_SQRT_DEFAULT                            STL_SQRT_SAFE
#define STL_RSQRT_DEFAULT                           STL_POSITIVE_RSQRT_ACCURATE_SAFE
#define STL_POSITIVE_RCP_DEFAULT                    STL_POSITIVE_RCP_ACCURATE_SAFE
#define STL_SMALL_EPS                               1e-15
#define STL_EPS                                     1e-6

// BRDF
#define STL_SPECULAR_DOMINANT_DIRECTION_DEFAULT     STL_SPECULAR_DOMINANT_DIRECTION_APPROX
#define STL_RF0_DIELECTRICS                         0.04
#define STL_GTR_GAMMA                               1.5
#define STL_VNDF_VERSION                            3

// Text
#define STL_TEXT_DIGIT_FORMAT                       10000
#define STL_TEXT_WITH_NICE_ONE_PIXEL_BACKGROUND     0

// Other
#define STL_RNG_NEXT_MODE                           STL_RNG_HASH
#define STL_RNG_FLOAT01_MODE                        STL_RNG_MANTISSA_BITS
#define STL_BAYER_DEFAULT                           STL_BAYER_REVERSEBITS

#define compiletime

namespace STL
{
    //=======================================================================================================================
    // MATH
    //=======================================================================================================================

    namespace Math
    {
        // Pi
        #define _Pi( x ) radians( 180.0 * x )

        float Pi( float x )
        { return _Pi( x ); }

        float2 Pi( float2 x )
        { return _Pi( x ); }

        float3 Pi( float3 x )
        { return _Pi( x ); }

        float4 Pi( float4 x )
        { return _Pi( x ); }

        // Radians to degrees
        #define _RadToDeg( x ) ( x * 180.0 / Pi( 1.0 ) )

        float RadToDeg( float x )
        { return _RadToDeg( x ); }

        float2 RadToDeg( float2 x )
        { return _RadToDeg( x ); }

        float3 RadToDeg( float3 x )
        { return _RadToDeg( x ); }

        float4 RadToDeg( float4 x )
        { return _RadToDeg( x ); }

        // Degrees to radians
        #define _DegToRad( x ) ( x * Pi( 1.0 ) / 180.0 )

        float DegToRad( float x )
        { return _DegToRad( x ); }

        float2 DegToRad( float2 x )
        { return _DegToRad( x ); }

        float3 DegToRad( float3 x )
        { return _DegToRad( x ); }

        float4 DegToRad( float4 x )
        { return _DegToRad( x ); }

        // Swap two values
        #define _Swap( x, y ) x ^= y; y ^= x; x ^= y

        void Swap( inout uint x, inout uint y )
        { _Swap( x, y ); }

        void Swap( inout uint2 x, inout uint2 y )
        { _Swap( x, y ); }

        void Swap( inout uint3 x, inout uint3 y )
        { _Swap( x, y ); }

        void Swap( inout uint4 x, inout uint4 y )
        { _Swap( x, y ); }

        void Swap( inout float x, inout float y )
        { float t = x; x = y; y = t; }

        void Swap( inout float2 x, inout float2 y )
        { float2 t = x; x = y; y = t; }

        void Swap( inout float3 x, inout float3 y )
        { float3 t = x; x = y; y = t; }

        void Swap( inout float4 x, inout float4 y )
        { float4 t = x; x = y; y = t; }

        // LinearStep
        // REQUIREMENT: a < b
        #define _LinearStep( a, b, x ) saturate( ( x - a ) / ( b - a ) )

        float LinearStep( float a, float b, float x )
        { return _LinearStep( a, b, x ); }

        float2 LinearStep( float2 a, float2 b, float2 x )
        { return _LinearStep( a, b, x ); }

        float3 LinearStep( float3 a, float3 b, float3 x )
        { return _LinearStep( a, b, x ); }

        float4 LinearStep( float4 a, float4 b, float4 x )
        { return _LinearStep( a, b, x ); }

        // SmoothStep
        // REQUIREMENT: a < b
        #define _SmoothStep01( x ) ( x * x * ( 3.0 - 2.0 * x ) )

        float SmoothStep01( float x )
        { return _SmoothStep01( saturate( x ) ); }

        float2 SmoothStep01( float2 x )
        { return _SmoothStep01( saturate( x ) ); }

        float3 SmoothStep01( float3 x )
        { return _SmoothStep01( saturate( x ) ); }

        float4 SmoothStep01( float4 x )
        { return _SmoothStep01( saturate( x ) ); }

        float SmoothStep( float a, float b, float x )
        { x = _LinearStep( a, b, x ); return _SmoothStep01( x ); }

        float2 SmoothStep( float2 a, float2 b, float2 x )
        { x = _LinearStep( a, b, x ); return _SmoothStep01( x ); }

        float3 SmoothStep( float3 a, float3 b, float3 x )
        { x = _LinearStep( a, b, x ); return _SmoothStep01( x ); }

        float4 SmoothStep( float4 a, float4 b, float4 x )
        { x = _LinearStep( a, b, x ); return _SmoothStep01( x ); }

        // SmootherStep
        // https://en.wikipedia.org/wiki/Smoothstep
        // REQUIREMENT: a < b
        #define _SmootherStep01( x ) ( x * x * x * ( x * ( x * 6.0 - 15.0 ) + 10.0 ) )

        float SmootherStep( float a, float b, float x )
        { x = _LinearStep( a, b, x ); return _SmootherStep01( x ); }

        float2 SmootherStep( float2 a, float2 b, float2 x )
        { x = _LinearStep( a, b, x ); return _SmootherStep01( x ); }

        float3 SmootherStep( float3 a, float3 b, float3 x )
        { x = _LinearStep( a, b, x ); return _SmootherStep01( x ); }

        float4 SmootherStep( float4 a, float4 b, float4 x )
        { x = _LinearStep( a, b, x ); return _SmootherStep01( x ); }

        // Sign
        #define STL_SIGN_BUILTIN 0
        #define STL_SIGN_FAST 1
        #define _Sign( x ) ( step( 0.0, x ) * 2.0 - 1.0 )

        float Sign( float x, compiletime const uint mode = STL_SIGN_DEFAULT )
        { return mode == STL_SIGN_FAST ? _Sign( x ) : sign( x ); }

        float2 Sign( float2 x, compiletime const uint mode = STL_SIGN_DEFAULT )
        { return mode == STL_SIGN_FAST ? _Sign( x ) : sign( x ); }

        float3 Sign( float3 x, compiletime const uint mode = STL_SIGN_DEFAULT )
        { return mode == STL_SIGN_FAST ? _Sign( x ) : sign( x ); }

        float4 Sign( float4 x, compiletime const uint mode = STL_SIGN_DEFAULT )
        { return mode == STL_SIGN_FAST ? _Sign( x ) : sign( x ); }

        // Pow
        float Pow( float x, float y )
        { return pow( abs( x ), y ); }

        float2 Pow( float2 x, float y )
        { return pow( abs( x ), y ); }

        float2 Pow( float2 x, float2 y )
        { return pow( abs( x ), y ); }

        float3 Pow( float3 x, float y )
        { return pow( abs( x ), y ); }

        float3 Pow( float3 x, float3 y )
        { return pow( abs( x ), y ); }

        float4 Pow( float4 x, float y )
        { return pow( abs( x ), y ); }

        float4 Pow( float4 x, float4 y )
        { return pow( abs( x ), y ); }

        // Pow for values in range [0; 1]
        float Pow01( float x, float y )
        { return pow( saturate( x ), y ); }

        float2 Pow01( float2 x, float y )
        { return pow( saturate( x ), y ); }

        float2 Pow01( float2 x, float2 y )
        { return pow( saturate( x ), y ); }

        float3 Pow01( float3 x, float y )
        { return pow( saturate( x ), y ); }

        float3 Pow01( float3 x, float3 y )
        { return pow( saturate( x ), y ); }

        float4 Pow01( float4 x, float y )
        { return pow( saturate( x ), y ); }

        float4 Pow01( float4 x, float4 y )
        { return pow( saturate( x ), y ); }

        // Sqrt
        #define STL_SQRT_BUILTIN 0
        #define STL_SQRT_SAFE 1

        float Sqrt( float x, compiletime const uint mode = STL_SQRT_DEFAULT )
        { return sqrt( mode == STL_SQRT_SAFE ? max( x, 0 ) : x ); }

        float2 Sqrt( float2 x, compiletime const uint mode = STL_SQRT_DEFAULT )
        { return sqrt( mode == STL_SQRT_SAFE ? max( x, 0 ) : x ); }

        float3 Sqrt( float3 x, compiletime const uint mode = STL_SQRT_DEFAULT )
        { return sqrt( mode == STL_SQRT_SAFE ? max( x, 0 ) : x ); }

        float4 Sqrt( float4 x, compiletime const uint mode = STL_SQRT_DEFAULT )
        { return sqrt( mode == STL_SQRT_SAFE ? max( x, 0 ) : x ); }

        // Sqrt for values in range [0; 1]
        float Sqrt01( float x )
        { return sqrt( saturate( x ) ); }

        float2 Sqrt01( float2 x )
        { return sqrt( saturate( x ) ); }

        float3 Sqrt01( float3 x )
        { return sqrt( saturate( x ) ); }

        float4 Sqrt01( float4 x )
        { return sqrt( saturate( x ) ); }

        // 1 / Sqrt
        #define STL_POSITIVE_RSQRT_BUILTIN 0
        #define STL_POSITIVE_RSQRT_BUILTIN_SAFE 1
        #define STL_POSITIVE_RSQRT_ACCURATE 2
        #define STL_POSITIVE_RSQRT_ACCURATE_SAFE 3

        float Rsqrt( float x, compiletime const uint mode = STL_RSQRT_DEFAULT )
        {
            if( mode <= STL_POSITIVE_RSQRT_BUILTIN_SAFE )
                return rsqrt( mode == STL_POSITIVE_RSQRT_BUILTIN ? x : max( x, STL_SMALL_EPS ) );

            return 1.0 / sqrt( mode == STL_POSITIVE_RSQRT_ACCURATE ? x : max( x, STL_SMALL_EPS ) );
        }

        float2 Rsqrt( float2 x, compiletime const uint mode = STL_RSQRT_DEFAULT )
        {
            if( mode <= STL_POSITIVE_RSQRT_BUILTIN_SAFE )
                return rsqrt( mode == STL_POSITIVE_RSQRT_BUILTIN ? x : max( x, STL_SMALL_EPS ) );

            return 1.0 / sqrt( mode == STL_POSITIVE_RSQRT_ACCURATE ? x : max( x, STL_SMALL_EPS ) );
        }

        float3 Rsqrt( float3 x, compiletime const uint mode = STL_RSQRT_DEFAULT )
        {
            if( mode <= STL_POSITIVE_RSQRT_BUILTIN_SAFE )
                return rsqrt( mode == STL_POSITIVE_RSQRT_BUILTIN ? x : max( x, STL_SMALL_EPS ) );

            return 1.0 / sqrt( mode == STL_POSITIVE_RSQRT_ACCURATE ? x : max( x, STL_SMALL_EPS ) );
        }

        float4 Rsqrt( float4 x, compiletime const uint mode = STL_RSQRT_DEFAULT )
        {
            if( mode <= STL_POSITIVE_RSQRT_BUILTIN_SAFE )
                return rsqrt( mode == STL_POSITIVE_RSQRT_BUILTIN ? x : max( x, STL_SMALL_EPS ) );

            return 1.0 / sqrt( mode == STL_POSITIVE_RSQRT_ACCURATE ? x : max( x, STL_SMALL_EPS ) );
        }

        // Acos(x) (approximate)
        // https://www.desmos.com/calculator/x6ut8ros1u
        #define _AcosApprox( x ) ( sqrt( 2.0 ) * sqrt( saturate( 1.0 - x ) ) )

        float AcosApprox( float x )
        { return _AcosApprox( x ); }

        float2 AcosApprox( float2 x )
        { return _AcosApprox( x ); }

        float3 AcosApprox( float3 x )
        { return _AcosApprox( x ); }

        float4 AcosApprox( float4 x )
        { return _AcosApprox( x ); }

        // Atan(x) (approximate, for x in range [-1; 1])
        // https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=1628884
        // https://www.desmos.com/calculator/0h8hv7kfp6
        #define _AtanApprox( x ) ( Math::Pi( 0.25 ) * x - ( abs( x ) * x - x ) * ( 0.2447 + 0.0663 * abs( x ) ) )

        float AtanApprox( float x )
        { return _AtanApprox( x ); }

        float2 AtanApprox( float2 x )
        { return _AtanApprox( x ); }

        float3 AtanApprox( float3 x )
        { return _AtanApprox( x ); }

        float4 AtanApprox( float4 x )
        { return _AtanApprox( x ); }

        // 1 / positive
        #define STL_POSITIVE_RCP_BUILTIN 0
        #define STL_POSITIVE_RCP_BUILTIN_SAFE 1
        #define STL_POSITIVE_RCP_ACCURATE 2
        #define STL_POSITIVE_RCP_ACCURATE_SAFE 3

        float PositiveRcp( float x, compiletime const uint mode = STL_POSITIVE_RCP_DEFAULT )
        {
            if( mode <= STL_POSITIVE_RCP_BUILTIN_SAFE )
                return rcp( mode == STL_POSITIVE_RCP_BUILTIN ? x : max( x, STL_SMALL_EPS ) );

            return 1.0 / ( mode == STL_POSITIVE_RCP_ACCURATE ? x : max( x, STL_SMALL_EPS ) );
        }

        float2 PositiveRcp( float2 x, compiletime const uint mode = STL_POSITIVE_RCP_DEFAULT )
        {
            if( mode <= STL_POSITIVE_RCP_BUILTIN_SAFE )
                return rcp( mode == STL_POSITIVE_RCP_BUILTIN ? x : max( x, STL_SMALL_EPS ) );

            return 1.0 / ( mode == STL_POSITIVE_RCP_ACCURATE ? x : max( x, STL_SMALL_EPS ) );
        }

        float3 PositiveRcp( float3 x, compiletime const uint mode = STL_POSITIVE_RCP_DEFAULT )
        {
            if( mode <= STL_POSITIVE_RCP_BUILTIN_SAFE )
                return rcp( mode == STL_POSITIVE_RCP_BUILTIN ? x : max( x, STL_SMALL_EPS ) );

            return 1.0 / ( mode == STL_POSITIVE_RCP_ACCURATE ? x : max( x, STL_SMALL_EPS ) );
        }

        float4 PositiveRcp( float4 x, compiletime const uint mode = STL_POSITIVE_RCP_DEFAULT )
        {
            if( mode <= STL_POSITIVE_RCP_BUILTIN_SAFE )
                return rcp( mode == STL_POSITIVE_RCP_BUILTIN ? x : max( x, STL_SMALL_EPS ) );

            return 1.0 / ( mode == STL_POSITIVE_RCP_ACCURATE ? x : max( x, STL_SMALL_EPS ) );
        }

        // LengthSquared
        float LengthSquared( float2 v )
        { return dot( v, v ); }

        float LengthSquared( float3 v )
        { return dot( v, v ); }

        float LengthSquared( float4 v )
        { return dot( v, v ); }

        // Distance
        float Distance( float2 a, float2 b )
        { return length( a - b ); }

        float Distance( float3 a, float3 b )
        { return length( a - b ); }

        // Manhattan distance
        float ManhattanDistance( float2 a, float2 b )
        { return dot( abs( a - b ), 1.0 ); }

        float ManhattanDistance( float3 a, float3 b )
        { return dot( abs( a - b ), 1.0 ); }

        // Bit operations
        uint ReverseBits4( uint x )
        {
            x = ( ( x & 0x5 ) << 1 ) | ( ( x & 0xA ) >> 1 );
            x = ( ( x & 0x3 ) << 2 ) | ( ( x & 0xC ) >> 2 );

            return x;
        }

        uint ReverseBits8( uint x )
        {
            x = ( ( x & 0x55 ) << 1 ) | ( ( x & 0xAA ) >> 1 );
            x = ( ( x & 0x33 ) << 2 ) | ( ( x & 0xCC ) >> 2 );
            x = ( ( x & 0x0F ) << 4 ) | ( ( x & 0xF0 ) >> 4 );

            return x;
        }

        uint ReverseBits16( uint x )
        {
            x = ( ( x & 0x5555 ) << 1 ) | ( ( x & 0xAAAA ) >> 1 );
            x = ( ( x & 0x3333 ) << 2 ) | ( ( x & 0xCCCC ) >> 2 );
            x = ( ( x & 0x0F0F ) << 4 ) | ( ( x & 0xF0F0 ) >> 4 );
            x = ( ( x & 0x00FF ) << 8 ) | ( ( x & 0xFF00 ) >> 8 );

            return x;
        }

        uint ReverseBits32( uint x )
        {
            #if 1
                x = reversebits( x );
            #else
                x = ( x << 16 ) | ( x >> 16 );
                x = ( ( x & 0x55555555 ) << 1 ) | ( ( x & 0xAAAAAAAA ) >> 1 );
                x = ( ( x & 0x33333333 ) << 2 ) | ( ( x & 0xCCCCCCCC ) >> 2 );
                x = ( ( x & 0x0F0F0F0F ) << 4 ) | ( ( x & 0xF0F0F0F0 ) >> 4 );
                x = ( ( x & 0x00FF00FF ) << 8 ) | ( ( x & 0xFF00FF00 ) >> 8 );
            #endif

            return x;
        }

        uint CompactBits( uint x )
        {
            x &= 0x55555555;
            x = ( x ^ ( x >> 1 ) ) & 0x33333333;
            x = ( x ^ ( x >> 2 ) ) & 0x0F0F0F0F;
            x = ( x ^ ( x >> 4 ) ) & 0x00FF00FF;
            x = ( x ^ ( x >> 8 ) ) & 0x0000FFFF;

            return x;
        }
    }

    //=======================================================================================================================
    // GEOMETRY
    //=======================================================================================================================

    namespace Geometry
    {
        float4 GetRotator( float angle )
        {
            float ca = cos( angle );
            float sa = sin( angle );

            return float4( ca, sa, -sa, ca );
        }

        float3x3 GetRotator( float3 axis, float angle )
        {
            float sa = sin( angle );
            float ca = cos( angle );
            float one_ca = 1.0 - ca;

            float3 a = sa * axis;
            float3 b = one_ca * axis.xyx * axis.yzz;

            float3 t1 = one_ca * ( axis * axis ) + ca;
            float3 t2 = b.xyz - a.zxy;
            float3 t3 = b.zxy + a.yzx;

            return float3x3
            (
                t1.x, t2.x, t3.x,
                t3.y, t1.y, t2.y,
                t2.z, t3.z, t1.z
            );
        }

        float4 GetRotator( float sa, float ca )
        { return float4( ca, sa, -sa, ca ); }

        float4 CombineRotators( float4 r1, float4 r2 )
        { return r1.xyxy * r2.xxzz + r1.zwzw * r2.yyww; }

        float2 RotateVector( float4 rotator, float2 v )
        { return v.x * rotator.xz + v.y * rotator.yw; }

        float3 RotateVector( float4x4 m, float3 v )
        { return mul( ( float3x3 )m, v ); }

        float3 RotateVector( float3x3 m, float3 v )
        { return mul( m, v ); }

        float2 RotateVectorInverse( float4 rotator, float2 v )
        { return v.x * rotator.xy + v.y * rotator.zw; }

        float3 RotateVectorInverse( float4x4 m, float3 v )
        { return mul( ( float3x3 )transpose( m ), v ); }

        float3 RotateVectorInverse( float3x3 m, float3 v )
        { return mul( transpose( m ), v ); }

        float3 AffineTransform( float4x4 m, float3 p )
        { return mul( m, float4( p, 1.0 ) ).xyz; }

        float3 AffineTransform( float3x4 m, float3 p )
        { return mul( m, float4( p, 1.0 ) ); }

        float3 AffineTransform( float4x4 m, float4 p )
        { return mul( m, p ).xyz; }

        float4 ProjectiveTransform( float4x4 m, float3 p )
        { return mul( m, float4( p, 1.0 ) ); }

        float4 ProjectiveTransform( float4x4 m, float4 p )
        { return mul( m, p ); }

        float2 GetPerpendicular( float2 v )
        { return float2( -v.y, v.x ); }

        float3 GetPerpendicularVector( float3 N )
        {
            float3 T = float3( N.z, -N.x, N.y );
            T -= N * dot( T, N );

            return normalize( T );
        }

        // http://marc-b-reynolds.github.io/quaternions/2016/07/06/Orthonormal.html
        float3x3 GetBasis( float3 N )
        {
            float sz = Math::Sign( N.z );
            float a  = 1.0 / ( sz + N.z );
            float ya = N.y * a;
            float b  = N.x * ya;
            float c  = N.x * sz;

            float3 T = float3( c * N.x * a - 1.0, sz * b, c );
            float3 B = float3( b, N.y * ya - sz, N.y );

            // Note: due to the quaternion formulation, the generated frame is rotated by 180 degrees,
            // s.t. if N = (0, 0, 1), then T = (-1, 0, 0) and B = (0, -1, 0).
            return float3x3( T, B, N );
        }

        float2 GetBarycentricCoords( float3 p, float3 a, float3 b, float3 c )
        {
            float3 v0 = b - a;
            float3 v1 = c - a;
            float3 v2 = p - a;

            float d00 = dot( v0, v0 );
            float d01 = dot( v0, v1 );
            float d11 = dot( v1, v1 );
            float d20 = dot( v2, v0 );
            float d21 = dot( v2, v1 );

            float2 barys;
            barys.x = d11 * d20 - d01 * d21;
            barys.y = d00 * d21 - d01 * d20;

            float invDenom = 1.0 / ( d00 * d11 - d01 * d01 );

            return barys * invDenom;
        }

        float DistanceAttenuation( float dist, float Rmax )
        {
            // [Brian Karis 2013, "Real Shading in Unreal Engine 4 ( course notes )"]
            float falloff = dist / Rmax;
            falloff *= falloff;
            falloff = saturate( 1.0 - falloff * falloff );
            falloff *= falloff;

            float atten = falloff;
            atten *= Math::PositiveRcp( dist * dist + 1.0 );

            return atten;
        }

        float3 UnpackLocalNormal( float2 localNormal, bool isUnorm = true )
        {
            float3 n;
            n.xy = isUnorm ? min( localNormal * ( 255.0 / 127.0 ) - 1.0, 1.0 ) : localNormal;
            n.z = Math::Sqrt01( 1.0 - Math::LengthSquared( n.xy ) );

            return n;
        }

        float3 TransformLocalNormal( float2 localNormal, float4 T, float3 N )
        {
            float3 n = UnpackLocalNormal( localNormal );
            float3 B = cross( N, T.xyz ); // TODO: potentially "normalize" is needed here

            return normalize( T.xyz * n.x + B * n.y * T.w + N * n.z );
        }

        float SolidAngle( float cosHalfAngle )
        {
            return Math::Pi( 2.0 ) * ( 1.0 - cosHalfAngle );
        }

        // orthoMode = { 0 - perspective, -1 - right handed ortho, 1 - left handed ortho }
        float3 ReconstructViewPosition( float2 uv, float4 cameraFrustum, float viewZ = 1.0, float orthoMode = 0.0 )
        {
            float3 p;
            p.xy = uv * cameraFrustum.zw + cameraFrustum.xy;
            p.xy *= viewZ * ( 1.0 - abs( orthoMode ) ) + orthoMode;
            p.z = viewZ;

            return p;
        }

        float2 GetScreenUv( float4x4 worldToClip, float3 X, bool killBackprojection = false )
        {
            float4 clip = Geometry::ProjectiveTransform( worldToClip, X );

            #if( STL_WINDOW_ORIGIN == STL_OGL )
                float2 uv = ( clip.xy / clip.w ) * 0.5 + 0.5;
            #else
                float2 uv = ( clip.xy / clip.w ) * float2( 0.5, -0.5 ) + 0.5;
            #endif

            if( killBackprojection )
                uv = clip.w < 0.0 ? 99999.0 : uv;

            return uv;
        }

        #define STL_SCREEN_MOTION 0
        #define STL_WORLD_MOTION 1

        float2 GetPrevUvFromMotion( float2 uv, float3 X, float4x4 worldToClipPrev, float3 motionVector, compiletime const uint motionType = STL_WORLD_MOTION )
        {
            float3 Xprev = X + motionVector;
            float2 uvPrev = GetScreenUv( worldToClipPrev, Xprev );

            [flatten]
            if( motionType == STL_SCREEN_MOTION )
                uvPrev = uv + motionVector.xy;

            return uvPrev;
        }

        float3x3 GetMirrorMatrix( float3 n )
        {
            float3x3 m;
            m[ 0 ] = float3( 1.0 - 2.0 * n.x * n.x, 0.0 - 2.0 * n.y * n.x, 0.0 - 2.0 * n.z * n.x );
            m[ 1 ] = float3( 0.0 - 2.0 * n.x * n.y, 1.0 - 2.0 * n.y * n.y, 0.0 - 2.0 * n.z * n.y );
            m[ 2 ] = float3( 0.0 - 2.0 * n.x * n.z, 0.0 - 2.0 * n.y * n.z, 1.0 - 2.0 * n.z * n.z );

            return m;
        }
    }

    //=======================================================================================================================
    // COLOR
    //=======================================================================================================================

    // https://chrisbrejon.com/cg-cinematography/chapter-1-color-management/
    // https://viereck.ch/hue-xy-rgb/
    // https://handwiki.org/wiki/Color_spaces_with_RGB_primaries

    namespace Color
    {
        float Luminance( float3 x )
        {
            // https://en.wikipedia.org/wiki/Relative_luminance

            return dot( x, float3( 0.2126, 0.7152, 0.0722 ) );
        }

        float3 Saturation( float3 x, float amount )
        {
            float luma = Luminance( x );

            return lerp( x, luma, amount );
        }

        /*
        Gamma ramps and encoding transfer functions
        Taken from https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli

        Orthogonal to color space though usually tightly coupled. For instance, sRGB is both a
        color space (defined by three basis vectors and a white point) and a gamma ramp. Gamma
        ramps are designed to reduce perceptual error when quantizing floats to integers with a
        limited number of bits. More variation is needed in darker colors because our eyes are
        more sensitive in the dark. The way the curve helps is that it spreads out dark values
        across more code words allowing for more variation. Likewise, bright values are merged
        together into fewer code words allowing for less variation.

        The sRGB curve is not a true gamma ramp but rather a piecewise function comprising a linear
        section and a power function. When sRGB-encoded colors are passed to an LCD monitor, they
        look correct on screen because the monitor expects the colors to be encoded with sRGB, and it
        removes the sRGB curve to linearize the values. When textures are encoded with sRGB, as many
        are, the sRGB curve needs to be removed before involving the colors in linear mathematics such
        as physically based lighting.
        */

        float3 ToGamma( float3 x, float gamma = 2.2 )
        {
            return Math::Pow01( x, 1.0 / gamma );
        }

        float3 FromGamma( float3 x, float gamma = 2.2 )
        {
            return Math::Pow01( x, gamma );
        }

        // "Full RGB": approximately pow( x, 1.0 / 2.2 )

        float3 ToSrgb( float3 x )
        {
            const float4 consts = float4( 1.055, 0.41666, -0.055, 12.92 );
            return lerp( consts.x * Math::Pow( x, consts.yyy ) + consts.z, consts.w * x, x < 0.0031308 );
        }

        float3 FromSrgb( float3 x )
        {
            const float4 consts = float4( 1.0 / 12.92, 1.0 / 1.055, 0.055 / 1.055, 1.0 / 0.41666 );
            return lerp( x * consts.x, Math::Pow( x * consts.y + consts.z, consts.www ), x > 0.04045 );
        }

        // "Limited RGB"
        // The OETF (opto-electronic transfer function) recommended for content shown on HDTVs.
        // This "gamma ramp" may increase contrast as appropriate for viewing in a dark environment.
        // Always use this curve with "Limited RGB" as it is used in conjunction with HDTVs.

        float3 ToRec709( float3 x )
        {
            const float4 consts = float4( 1.0993, 0.45, -0.0993, 4.5 );
            return lerp( consts.x * Math::Pow( x, consts.yyy ) + consts.zzz, consts.w * x, x < 0.0181 );
        }

        float3 FromRec709( float3 x )
        {
            const float4 consts = float4( 1.0 / 4.5, 1.0 / 1.0993, 0.0993 / 1.0993, 1.0 / 0.45 );
            return lerp( x * consts.x, Math::Pow( x * consts.y + consts.z, consts.www ), x > 0.08145 );
        }

        // This is the new HDR transfer function, also called "PQ" for perceptual quantizer. Note that REC2084
        // does not also refer to a color space. REC2084 is typically used with the REC2020 color space.

        float3 ToRec2084( float3 x )
        {
            const float m1 = 2610.0 / 4096.0 / 4;
            const float m2 = 2523.0 / 4096.0 * 128;
            const float c1 = 3424.0 / 4096.0;
            const float c2 = 2413.0 / 4096.0 * 32;
            const float c3 = 2392.0 / 4096.0 * 32;

            float3 Lp = pow( x, m1 );

            return pow( ( c1 + c2 * Lp ) / ( 1.0 + c3 * Lp ), m2 );
        }

        float3 FromRec2084( float3 x )
        {
            const float m1 = 2610.0 / 4096.0 / 4;
            const float m2 = 2523.0 / 4096.0 * 128;
            const float c1 = 3424.0 / 4096.0;
            const float c2 = 2413.0 / 4096.0 * 32;
            const float c3 = 2392.0 / 4096.0 * 32;

            float3 Np = pow( x, 1.0 / m2 );

            return pow( max( Np - c1, 0.0 ) / ( c2 - c3 * Np ), 1.0 / m1 );
        }

        /*
        Color space conversions:
        Taken from https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli

        These assume linear (not gamma-encoded) values. A color space conversion is a change
        of basis (like in Linear Algebra). Since a color space is defined by three vectors,
        the basis vectors, changing space involves a matrix-vector multiplication. Note that
        changing the color space may result in colors that are "out of bounds" because some
        color spaces have larger gamuts than others. When converting some colors from a wide
        gamut to small gamut, negative values may result, which are inexpressible in that new
        color space.

        It would be ideal to build a color pipeline which never throws away inexpressible (but
        perceivable) colors. This means using a color space that is as wide as possible. The
        XYZ color space is the neutral, all-encompassing color space, but it has the unfortunate
        property of having negative values (specifically in X and Z). To correct this, a further
        transformation can be made to X and Z to make them always positive. They can have their
        precision needs reduced by dividing by Y, allowing X and Z to be packed into two UNORM8s.
        This color space is called YUV for lack of a better name.

        Note: Rec.709 and sRGB share the same color primaries and white point. Their only difference
        is the transfer curve used.
        */

        // YCoCg

        float3 RgbToYCoCg( float3 x )
        {
            float Y = dot( x, float3( 0.25, 0.5, 0.25 ) );
            float Co = dot( x, float3( 0.5, 0.0, -0.5 ) );
            float Cg = dot( x, float3( -0.25, 0.5, -0.25 ) );

            return float3( Y, Co, Cg );
        }

        float3 YCoCgToRgb( float3 x )
        {
            float t = x.x - x.z;

            float3 r;
            r.y = x.x + x.z;
            r.x = t + x.y;
            r.z = t - x.y;

            return r;
        }

        // REC2020

        float3 RgbToRec2020( float3 x )
        {
            static const float3x3 M =
            {
                0.627402, 0.329292, 0.043306,
                0.069095, 0.919544, 0.011360,
                0.016394, 0.088028, 0.895578
            };

            return mul( M, x );
        }

        float3 Rec2020ToRgb( float3 x )
        {
            static const float3x3 M =
            {
                1.660496, -0.587656, -0.072840,
                -0.124547, 1.132895, -0.008348,
                -0.018154, -0.100597, 1.118751
            };

            return mul( M, x );
        }

         // DCIP3

        float3 RgbToDcip3( float3 x )
        {
            static const float3x3 M =
            {
                0.822458, 0.177542, 0.000000,
                0.033193, 0.966807, 0.000000,
                0.017085, 0.072410, 0.910505
            };

            return mul( M, x );
        }

        float3 Dcip3ToRgb( float3 x )
        {
            static const float3x3 M =
            {
                1.224947, -0.224947, 0.000000,
                -0.042056, 1.042056, 0.000000,
                -0.019641, -0.078651, 1.098291
            };

            return mul( M, x );
        }

        // CIE XYZ

        float3 RgbToXyz( float3 x )
        {
            static const float3x3 M =
            {
                0.4123907992659595, 0.3575843393838780, 0.1804807884018343,
                0.2126390058715104, 0.7151686787677559, 0.0721923153607337,
                0.0193308187155918, 0.1191947797946259, 0.9505321522496608
            };

            return mul( M, x );
        }

        float3 XyzToRgb( float3 x )
        {
            static const float3x3 M =
            {
                3.240969941904522, -1.537383177570094, -0.4986107602930032,
                -0.9692436362808803, 1.875967501507721, 0.04155505740717569,
                0.05563007969699373, -0.2039769588889765, 1.056971514242878
            };

            return mul( M, x );
        }

        // Encode an RGB color into a 32-bit LogLuv HDR format. The supported luminance range is roughly 10^-6..10^6 in 0.17% steps.
        // The log-luminance is encoded with 14 bits and chroma with 9 bits each. This was empirically more accurate than using 8 bit chroma.
        // Black (all zeros) is handled exactly
        uint ToLogLuv( float3 x )
        {
            // Convert RGB to XYZ
            float3 XYZ = RgbToXyz( x );

            // Encode log2( Y ) over the range [ -20, 20 ) in 14 bits ( no sign bit ).
            // TODO: Fast path that uses the bits from the fp32 representation directly
            float logY = 409.6 * ( log2( XYZ.y ) + 20.0 ); // -inf if Y == 0
            uint Le = uint( clamp( logY, 0.0, 16383.0 ) );

            // Early out if zero luminance to avoid NaN in chroma computation.
            // Note Le == 0 if Y < 9.55e-7. We'll decode that as exactly zero
            if( Le == 0 )
                return 0;

            // Compute chroma (u,v) values by:
            //  x = X / ( X + Y + Z )
            //  y = Y / ( X + Y + Z )
            //  u = 4x / ( -2x + 12y + 3 )
            //  v = 9y / ( -2x + 12y + 3 )
            // These expressions can be refactored to avoid a division by:
            //  u = 4X / ( -2X + 12Y + 3(X + Y + Z) )
            //  v = 9Y / ( -2X + 12Y + 3(X + Y + Z) )
            float invDenom = 1.0 / ( -2.0 * XYZ.x + 12.0 * XYZ.y + 3.0 * ( XYZ.x + XYZ.y + XYZ.z ) );
            float2 uv = float2( 4.0, 9.0 ) * XYZ.xy * invDenom;

            // Encode chroma (u,v) in 9 bits each.
            // The gamut of perceivable uv values is roughly [0,0.62], so scale by 820 to get 9-bit values
            uint2 uve = uint2( clamp( 820.0 * uv, 0.0, 511.0 ) );

            return ( Le << 18 ) | ( uve.x << 9 ) | uve.y;
        }

        // Decode an RGB color stored in a 32-bit LogLuv HDR format
        float3 FromLogLuv( uint x )
        {
            // Decode luminance Y from encoded log-luminance
            uint Le = x >> 18;
            if( Le == 0 )
                return 0;

            float logY = ( float( Le ) + 0.5 ) / 409.6 - 20.0;
            float Y = exp2( logY );

            // Decode normalized chromaticity xy from encoded chroma (u,v)
            //  x = 9u / (6u - 16v + 12)
            //  y = 4v / (6u - 16v + 12)
            uint2 uve = uint2( x >> 9, x ) & 0x1ff;
            float2 uv = ( float2( uve ) + 0.5 ) / 820.0;

            float invDenom = 1.0 / ( 6.0 * uv.x - 16.0 * uv.y + 12.0 );
            float2 xy = float2( 9.0, 4.0 ) * uv * invDenom;

            // Convert chromaticity to XYZ and back to RGB.
            //  X = Y / y * x
            //  Z = Y / y * (1 - x - y)
            float s = Y / xy.y;
            float3 XYZ = float3( s * xy.x, Y, s * ( 1.0 - xy.x - xy.y ) );

            // Convert back to RGB and clamp to avoid out-of-gamut colors
            float3 color = max( XyzToRgb( XYZ ), 0.0 );

            return color;
        }

        // HDR compression ( tone mapping )
        float3 HdrToLinear( float3 colorMulExposure )
        {
            float3 x0 = colorMulExposure * 0.38317;
            float3 x1 = FromGamma( 1.0 - exp( -colorMulExposure ) );
            float3 color = lerp( x0, x1, step( 1.413, colorMulExposure ) );

            return color;
        }

        float3 HdrToLinear_Reinhard( float3 color, float exposure = 1.0 )
        {
            float luma = Luminance( color );

            return color * Math::PositiveRcp( 1.0 + luma * exposure );
        }

        float3 _UnchartedCurve( float3 x )
        {
            float A = 0.22; // Shoulder Strength
            float B = 0.3;  // Linear Strength
            float C = 0.1;  // Linear Angle
            float D = 0.2;  // Toe Strength
            float E = 0.01; // Toe Numerator
            float F = 0.3;  // Toe Denominator

            return ( ( x * ( A * x + C * B ) + D * E ) / ( x * ( A * x + B ) + D * F ) ) - ( E / F );
        }

        float3 HdrToLinear_Uncharted( float3 colorMulExposure, float whitePoint = 11.2 )
        {
            // John Hable's Uncharted 2 filmic tone mappping
            return _UnchartedCurve( colorMulExposure ) / _UnchartedCurve( whitePoint ).x;
        }

        float3 HdrToLinear_Aces( float3 colorMulExposure )
        {
            // Cancel out the pre-exposure mentioned in https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
            colorMulExposure *= 0.6;

            float A = 2.51;
            float B = 0.03;
            float C = 2.43;
            float D = 0.59;
            float E = 0.14;

            return ( ( colorMulExposure * ( A * colorMulExposure + B ) ) * Math::PositiveRcp( colorMulExposure * ( C * colorMulExposure + D ) + E ) );
        }

        // HDR decompression
        float3 LinearToHdr( float3 color )
        {
            float3 x0 = color / 0.38317;
            float3 x1 = -log( max( 1.0 - ToGamma( color ), STL_EPS ) );
            float3 colorMulExposure = lerp( x0, x1, step( 1.413, x0 ) );

            return colorMulExposure;
        }

        float3 LinearToHdr_Reinhard( float3 color, float exposure = 1.0 )
        {
            float luma = Luminance( color );

            return color * Math::PositiveRcp( 1.0 - luma * exposure );
        }

        // Blending functions
        float4 BlendSoft( float4 a, float4 b )
        {
            float4 t = 1.0 - 2.0 * b;
            float4 c = ( 2.0 * b + a * t ) * a;
            float4 d = 2.0 * a * ( 1.0 - b ) - Math::Sqrt( a ) * t;
            bool4 res = b > 0.5;

            return lerp( c, d, res );
        }

        float4 BlendDarken( float4 a, float4 b )
        {
            bool4 res = a > b;

            return lerp( a, b, res );
        }

        float4 BlendDifference( float4 a, float4 b )
        {
            return abs( a - b );
        }

        float4 BlendScreen( float4 a, float4 b )
        {
            return a + b * ( 1.0 - a );
        }

        float4 BlendOverlay( float4 a, float4 b )
        {
            bool4 res = a > 0.5;
            float4 c = 2.0 * a * b;
            float4 d = 2.0 * BlendScreen( a, b ) - 1.0;

            return lerp( c, d, res );
        }

        // Color clamping
        #define _Clamp( m1, sigma, input ) clamp( input, m1 - sigma, m1 + sigma )

        float4 Clamp( float4 m1, float4 sigma, float4 color )
        { return _Clamp( m1, sigma, color ); }

        float3 Clamp( float3 m1, float3 sigma, float3 color )
        { return _Clamp( m1, sigma, color ); }

        float2 Clamp( float2 m1, float2 sigma, float2 color )
        { return _Clamp( m1, sigma, color ); }

        float Clamp( float m1, float sigma, float color )
        { return _Clamp( m1, sigma, color ); }

        // More correct color clamping for RGB color space
        float3 ClampAabb( float3 m1, float3 sigma, float3 color )
        {
            float3 center = m1;
            float3 extents = sigma;

            float3 d = color - center;
            float3 ts = abs( extents / ( d + 0.0001 ) );
            float t = min( ts.x, min( ts.y, ts.z ) );

            return center + d * saturate( t );
        }

        // Misc
        float3 ColorizeLinear( float x )
        {
            x = saturate( x );

            float3 color;
            if( x < 0.25 )
                color = lerp( float3( 0, 0, 0 ), float3( 0, 0, 1 ), Math::SmoothStep( 0.00, 0.25, x ) );
            else if( x < 0.50 )
                color = lerp( float3( 0, 0, 1 ), float3( 0, 1, 0 ), Math::SmoothStep( 0.25, 0.50, x ) );
            else if( x < 0.75 )
                color = lerp( float3( 0, 1, 0 ), float3( 1, 1, 0 ), Math::SmoothStep( 0.50, 0.75, x ) );
            else
                color = lerp( float3( 1, 1, 0 ), float3( 1, 0, 0 ), Math::SmoothStep( 0.75, 1.00, x ) );

            return color;
        }

        // https://www.shadertoy.com/view/ls2Bz1
        float3 ColorizeZucconi( float x )
        {
            // Original solution converts visible wavelengths of light (400-700 nm) (represented as x = [0; 1]) to RGB colors
            x = saturate( x ) * 0.85;

            const float3 c1 = float3( 3.54585104, 2.93225262, 2.41593945 );
            const float3 x1 = float3( 0.69549072, 0.49228336, 0.27699880 );
            const float3 y1 = float3( 0.02312639, 0.15225084, 0.52607955 );

            float3 t = c1 * ( x - x1 );
            float3 a = saturate( 1.0 - t * t - y1 );

            const float3 c2 = float3( 3.90307140, 3.21182957, 3.96587128 );
            const float3 x2 = float3( 0.11748627, 0.86755042, 0.66077860 );
            const float3 y2 = float3( 0.84897130, 0.88445281, 0.73949448 );

            float3 k = c2 * ( x - x2 );
            float3 b = saturate( 1.0 - k * k - y2 );

            return saturate( a + b );
        }
    }

    //=======================================================================================================================
    // PACKING
    //=======================================================================================================================

    namespace Packing
    {
        // TODO: add signed packing / unpacking

        // Unsigned packing
        uint RgbaToUint( float4 c, compiletime const uint Rbits, compiletime const uint Gbits = 0, compiletime const uint Bbits = 0, compiletime const uint Abits = 0 )
        {
            const uint Rmask = ( 1u << Rbits ) - 1u;
            const uint Gmask = ( 1u << Gbits ) - 1u;
            const uint Bmask = ( 1u << Bbits ) - 1u;
            const uint Amask = ( 1u << Abits ) - 1u;
            const uint Gshift = Rbits;
            const uint Bshift = Gshift + Gbits;
            const uint Ashift = Bshift + Bbits;
            const float4 scale = float4( Rmask, Gmask, Bmask, Amask );

            uint4 p = uint4( saturate( c ) * scale + 0.5 );
            p.yzw <<= uint3( Gshift, Bshift, Ashift );
            p.xy |= p.zw;

            return p.x | p.y;
        }

        // Unsigned unpacking
        float4 UintToRgba( uint p, compiletime const uint Rbits, compiletime const uint Gbits = 0, compiletime const uint Bbits = 0, compiletime const uint Abits = 0 )
        {
            const uint Rmask = ( 1u << Rbits ) - 1u;
            const uint Gmask = ( 1u << Gbits ) - 1u;
            const uint Bmask = ( 1u << Bbits ) - 1u;
            const uint Amask = ( 1u << Abits ) - 1u;
            const uint Gshift = Rbits;
            const uint Bshift = Gshift + Gbits;
            const uint Ashift = Bshift + Bbits;
            const float4 scale = 1.0 / max( float4( Rmask, Gmask, Bmask, Amask ), 1.0 );

            uint4 c = p >> uint4( 0, Gshift, Bshift, Ashift );
            c &= uint4( Rmask, Gmask, Bmask, Amask );

            return float4( c ) * scale;
        }

        // Half float
        uint Rg16fToUint( float2 c )
        {
            return ( f32tof16( c.y ) << 16 ) | ( f32tof16( c.x ) & 0xFFFF ); // TODO: 0xFFFF is a WAR for NV VK compiler bug (<= 461.xx)
        }

        float2 UintToRg16f( uint p )
        {
            float2 c;
            c.x = f16tof32( p );
            c.y = f16tof32( p >> 16 );

            return c;
        }

        // RGBE - shared exponent, positive values only (Ward 1984)
        uint EncodeRgbe( float3 c )
        {
            float sharedExp = ceil( log2( max( max( c.x, c.y ), c.z ) ) );
            float4 p = float4( c * exp2( -sharedExp ), ( sharedExp + 128.0 ) / 255.0 );

            return RgbaToUint( p, 8, 8, 8, 8 );
        }

        float3 DecodeRgbe( uint p )
        {
            float4 c = UintToRgba( p, 8, 8, 8, 8 );

            return c.xyz * exp2( c.w * 255.0 - 128.0 );
        }

        // Octahedron packing for unit vectors
        // https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
        // [Cigolle 2014, "A Survey of Efficient Representations for Independent Unit Vectors"]
        // Error ( deg )
        //                    Mean      Max
        // oct     8:8        0.31485   0.63575
        // snorm   8:8:8      0.17015   0.38588
        // oct     10:10      0.07829   0.15722
        // snorm   10:10:10   0.04228   0.09598
        // oct     12:12      0.01953   0.03928
        // oct     16:16      0.00122   0.00246
        // snorm   16:16:16   0.00066   0.00149
        float2 EncodeUnitVector( float3 v, compiletime const bool bSigned = false )
        {
            v /= dot( abs( v ), 1.0 );

            float2 octWrap = ( 1.0 - abs( v.yx ) ) * Math::Sign( v.xy );
            v.xy = v.z >= 0.0 ? v.xy : octWrap;

            return bSigned ? v.xy : v.xy * 0.5 + 0.5;
        }

        float3 DecodeUnitVector( float2 p, compiletime const bool bSigned = false, compiletime const bool bNormalize = true )
        {
            p = bSigned ? p : ( p * 2.0 - 1.0 );

            // https://twitter.com/Stubbesaurus/status/937994790553227264
            float3 n = float3( p.xy, 1.0 - abs( p.x ) - abs( p.y ) );
            float t = saturate( -n.z );
            n.xy -= t * Math::Sign( n.xy );

            return bNormalize ? normalize( n ) : n;
        }
    }

    //=======================================================================================================================
    // FILTERING
    //=======================================================================================================================

    namespace Filtering
    {
        float GetModifiedRoughnessFromNormalVariance( float linearRoughness, float3 nonNormalizedAverageNormal )
        {
            // https://blog.selfshadow.com/publications/s2013-shading-course/rad/s2013_pbs_rad_notes.pdf (page 20)
            float l = length( nonNormalizedAverageNormal );
            float kappa = saturate( 1.0 - l * l ) * Math::PositiveRcp( l * ( 3.0 - l * l ) );

            return Math::Sqrt01( linearRoughness * linearRoughness + kappa );
        }

        // Mipmap level
        float GetMipmapLevel( float duvdxMulTexSizeSq, float duvdyMulTexSizeSq, compiletime const float maxAnisotropy = 1.0 )
        {
            // duvdtMulTexSizeSq = ||dUV/dt * TexSize|| ^ 2

            float Pmax = max( duvdxMulTexSizeSq, duvdyMulTexSizeSq );

            if( maxAnisotropy > 1.0 )
            {
                float Pmin = min( duvdxMulTexSizeSq, duvdyMulTexSizeSq );
                float N = min( Pmax * Math::PositiveRcp( Pmin ), maxAnisotropy );
                Pmax *= Math::PositiveRcp( N );
            }

            float mip = 0.5 * log2( Pmax );

            return mip; // Yes, no clamping to 0 here!
        }

        // Nearest filter (for nearest sampling)
        struct Nearest
        {
            float2 origin;
        };

        Nearest GetNearestFilter( float2 uv, float2 texSize )
        {
            float2 t = uv * texSize;

            Nearest result;
            result.origin = floor( t );

            return result;
        }

        // Bilinear filter (for nearest sampling)
        struct Bilinear
        {
            float2 origin;
            float2 weights;
        };

        Bilinear GetBilinearFilter( float2 uv, float2 texSize )
        {
            float2 t = uv * texSize - 0.5;

            Bilinear result;
            result.origin = floor( t );
            result.weights = saturate( t - result.origin );

            return result;
        }

        float ApplyBilinearFilter( float s00, float s10, float s01, float s11, Bilinear f )
        { return lerp( lerp( s00, s10, f.weights.x ), lerp( s01, s11, f.weights.x ), f.weights.y ); }

        float2 ApplyBilinearFilter( float2 s00, float2 s10, float2 s01, float2 s11, Bilinear f )
        { return lerp( lerp( s00, s10, f.weights.x ), lerp( s01, s11, f.weights.x ), f.weights.y ); }

        float3 ApplyBilinearFilter( float3 s00, float3 s10, float3 s01, float3 s11, Bilinear f )
        { return lerp( lerp( s00, s10, f.weights.x ), lerp( s01, s11, f.weights.x ), f.weights.y ); }

        float4 ApplyBilinearFilter( float4 s00, float4 s10, float4 s01, float4 s11, Bilinear f )
        { return lerp( lerp( s00, s10, f.weights.x ), lerp( s01, s11, f.weights.x ), f.weights.y ); }

        float4 GetBilinearCustomWeights( Bilinear f, float4 customWeights )
        {
            float2 oneMinusWeights = saturate( 1.0 - f.weights );

            float4 weights = customWeights;
            weights.x *= oneMinusWeights.x * oneMinusWeights.y;
            weights.y *= f.weights.x * oneMinusWeights.y;
            weights.z *= oneMinusWeights.x * f.weights.y;
            weights.w *= f.weights.x * f.weights.y;

            return weights;
        }

        // Normalization avoids dividing by small numbers to mitigate potential imprecision problems
        #define _ApplyBilinearCustomWeights( s00, s10, s01, s11, w, normalize ) \
            ( ( s00 * w.x + s10 * w.y + s01 * w.z + s11 * w.w ) * ( normalize ? ( dot( w, 1.0 ) < 0.0001 ? 0.0 : rcp( dot( w, 1.0 ) ) ) : 1.0 ) )

        float ApplyBilinearCustomWeights( float s00, float s10, float s01, float s11, float4 w, compiletime const bool normalize = true )
        { return _ApplyBilinearCustomWeights( s00, s10, s01, s11, w, normalize ); }

        float2 ApplyBilinearCustomWeights( float2 s00, float2 s10, float2 s01, float2 s11, float4 w, compiletime const bool normalize = true )
        { return _ApplyBilinearCustomWeights( s00, s10, s01, s11, w, normalize ); }

        float3 ApplyBilinearCustomWeights( float3 s00, float3 s10, float3 s01, float3 s11, float4 w, compiletime const bool normalize = true )
        { return _ApplyBilinearCustomWeights( s00, s10, s01, s11, w, normalize ); }

        float4 ApplyBilinearCustomWeights( float4 s00, float4 s10, float4 s01, float4 s11, float4 w, compiletime const bool normalize = true )
        { return _ApplyBilinearCustomWeights( s00, s10, s01, s11, w, normalize ); }

        // Catmull-Rom (for nearest sampling)
        struct CatmullRom
        {
            float2 origin;
            float2 weights[4];
        };

        CatmullRom GetCatmullRomFilter( float2 uv, float2 texSize, float sharpness = 0.5 )
        {
            float2 tci = uv * texSize;
            float2 tc = floor( tci - 0.5 ) + 0.5;
            float2 f = saturate( tci - tc );
            float2 f2 = f * f;
            float2 f3 = f2 * f;

            CatmullRom result;
            result.origin = tc - 1.5;
            result.weights[ 0 ] = -sharpness * f3 + 2.0 * sharpness * f2 - sharpness * f;
            result.weights[ 1 ] = ( 2.0 - sharpness ) * f3 - ( 3.0 - sharpness ) * f2 + 1.0;
            result.weights[ 2 ] = -( 2.0 - sharpness ) * f3 + ( 3.0 - 2.0 * sharpness ) * f2 + sharpness * f;
            result.weights[ 3 ] = sharpness * f3 - sharpness * f2;

            return result;
        }

        float4 ApplyCatmullRomFilterNoCorners( CatmullRom filter, float4 s10, float4 s20, float4 s01, float4 s11, float4 s21, float4 s31, float4 s02, float4 s12, float4 s22, float4 s32, float4 s13, float4 s23 )
        {
            /*
            s00, s30, s03, s33 - are not used

                  s10   s20
            s01   s11   s21   s31
            s02   s12   s22   s32
                  s13   s23
            */

            float w = filter.weights[ 1 ].x * filter.weights[ 0 ].y;
            float4 color = s10 * w;
            float sum = w;

            w = filter.weights[ 2 ].x * filter.weights[ 0 ].y;
            color += s20 * w;
            sum += w;


            w = filter.weights[ 0 ].x * filter.weights[ 1 ].y;
            color += s01 * w;
            sum += w;

            w = filter.weights[ 1 ].x * filter.weights[ 1 ].y;
            color += s11 * w;
            sum += w;

            w = filter.weights[ 2 ].x * filter.weights[ 1 ].y;
            color += s21 * w;
            sum += w;

            w = filter.weights[ 3 ].x * filter.weights[ 1 ].y;
            color += s31 * w;
            sum += w;


            w = filter.weights[ 0 ].x * filter.weights[ 2 ].y;
            color += s02 * w;
            sum += w;

            w = filter.weights[ 1 ].x * filter.weights[ 2 ].y;
            color += s12 * w;
            sum += w;

            w = filter.weights[ 2 ].x * filter.weights[ 2 ].y;
            color += s22 * w;
            sum += w;

            w = filter.weights[ 3 ].x * filter.weights[ 2 ].y;
            color += s32 * w;
            sum += w;


            w = filter.weights[ 1 ].x * filter.weights[ 3 ].y;
            color += s13 * w;
            sum += w;

            w = filter.weights[ 2 ].x * filter.weights[ 3 ].y;
            color += s23 * w;
            sum += w;

            return color * Math::PositiveRcp( sum );
        }

        float ApplyCatmullRomFilterNoCorners( CatmullRom filter, float s10, float s20, float s01, float s11, float s21, float s31, float s02, float s12, float s22, float s32, float s13, float s23 )
        {
            /*
            s00, s30, s03, s33 - are not used

                  s10   s20
            s01   s11   s21   s31
            s02   s12   s22   s32
                  s13   s23
            */

            float w = filter.weights[ 1 ].x * filter.weights[ 0 ].y;
            float color = s10 * w;
            float sum = w;

            w = filter.weights[ 2 ].x * filter.weights[ 0 ].y;
            color += s20 * w;
            sum += w;


            w = filter.weights[ 0 ].x * filter.weights[ 1 ].y;
            color += s01 * w;
            sum += w;

            w = filter.weights[ 1 ].x * filter.weights[ 1 ].y;
            color += s11 * w;
            sum += w;

            w = filter.weights[ 2 ].x * filter.weights[ 1 ].y;
            color += s21 * w;
            sum += w;

            w = filter.weights[ 3 ].x * filter.weights[ 1 ].y;
            color += s31 * w;
            sum += w;


            w = filter.weights[ 0 ].x * filter.weights[ 2 ].y;
            color += s02 * w;
            sum += w;

            w = filter.weights[ 1 ].x * filter.weights[ 2 ].y;
            color += s12 * w;
            sum += w;

            w = filter.weights[ 2 ].x * filter.weights[ 2 ].y;
            color += s22 * w;
            sum += w;

            w = filter.weights[ 3 ].x * filter.weights[ 2 ].y;
            color += s32 * w;
            sum += w;


            w = filter.weights[ 1 ].x * filter.weights[ 3 ].y;
            color += s13 * w;
            sum += w;

            w = filter.weights[ 2 ].x * filter.weights[ 3 ].y;
            color += s23 * w;
            sum += w;

            return color * Math::PositiveRcp( sum );
        }

        // Blur 1D
        // offsets = { -offsets.xy, offsets.zw, 0, offsets.xy, offsets.zw };
        float4 GetBlurOffsets1D( float2 directionDivTexSize )
        { return float2( 1.7229, 3.8697 ).xxyy * directionDivTexSize.xyxy; }

        // Blur 2D
        // offsets = { 0, offsets.xy, offsets.zw, offsets.wx, offsets.yz };
        float4 GetBlurOffsets2D( float2 invTexSize )
        { return float4( 0.4, 0.9, -0.4, -0.9 ) * invTexSize.xyxy; }
    }

    //=======================================================================================================================
    // SEQUENCE
    //=======================================================================================================================

    namespace Sequence
    {
        // https://en.wikipedia.org/wiki/Ordered_dithering
        #define STL_BAYER_LINEAR 0
        #define STL_BAYER_REVERSEBITS 1

        // RESULT: [0; 15]
        uint Bayer4x4ui( uint2 samplePos, uint frameIndex, compiletime const uint mode = STL_BAYER_DEFAULT )
        {
            uint2 samplePosWrap = samplePos & 3;
            uint a = 2068378560 * ( 1 - ( samplePosWrap.x >> 1 ) ) + 1500172770 * ( samplePosWrap.x >> 1 );
            uint b = ( samplePosWrap.y + ( ( samplePosWrap.x & 1 ) << 2 ) ) << 2;

            uint sampleOffset = mode == STL_BAYER_REVERSEBITS ? Math::ReverseBits4( frameIndex ) : frameIndex;

            return ( ( a >> b ) + sampleOffset ) & 0xF;
        }

        // RESULT: [0; 1)
        float Bayer4x4( uint2 samplePos, uint frameIndex, compiletime const uint mode = STL_BAYER_DEFAULT )
        {
            uint bayer = Bayer4x4ui( samplePos, frameIndex, mode );

            return float( bayer ) / 16.0;
        }

        // https://en.wikipedia.org/wiki/Low-discrepancy_sequence
        // RESULT: [0; 1)
        float2 Hammersley2D( uint index, float sampleCount )
        {
            float x = float( index ) / sampleCount;
            float y = float( Math::ReverseBits32( index ) ) * 2.3283064365386963e-10;

            return float2( x, y );
        }

        // https://en.wikipedia.org/wiki/Z-order_curve
        uint2 Morton2D( uint index )
        {
            return uint2( Math::CompactBits( index ), Math::CompactBits( index >> 1 ) );
        }

        // Checkerboard pattern
        uint CheckerBoard( uint2 samplePos, uint frameIndex )
        {
            uint a = samplePos.x ^ samplePos.y;

            return ( a ^ frameIndex ) & 0x1;
        }

        uint IntegerExplode( uint x )
        {
            x = ( x | ( x << 8 ) ) & 0x00FF00FF;
            x = ( x | ( x << 4 ) ) & 0x0F0F0F0F;
            x = ( x | ( x << 2 ) ) & 0x33333333;
            x = ( x | ( x << 1 ) ) & 0x55555555;
            return x;
        }

        uint Zorder( uint2 xy )
        {
            return IntegerExplode( xy.x ) | ( IntegerExplode( xy.y ) << 1 );
        }

        uint Lcg( uint x )
        {
            // http://en.wikipedia.org/wiki/Linear_congruential_generator
            return 1664525u * x + 1013904223u;
        }

        uint XorShift( uint x )
        {
            // Xorshift algorithm from George Marsaglia's paper
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;

            return x;
        }

        // NOTE: Hash( 0 ) = 0! If that's a problem do "Hash32( x + constant )". HashCombine does something similar already
        uint Hash( uint x )
        {
            // This little gem is from https://nullprogram.com/blog/2018/07/31/, "Prospecting for Hash Functions" by Chris Wellons
            #if 1 // faster, higher bias
                x ^= x >> 16;
                x *= 0x7FEB352D;
                x ^= x >> 15;
                x *= 0x846CA68B;
                x ^= x >> 16;

                return x;
            #else // slower, lower bias
                x ^= x >> 17;
                x *= 0xED5AD4BB;
                x ^= x >> 11;
                x *= 0xAC4C1B51;
                x ^= x >> 15;
                x *= 0x31848BAB;
                x ^= x >> 14;

                return x;
            #endif
        }

        uint HashCombine( uint seed, uint value )
        {
            return seed ^ ( Hash( value ) + 0x9E3779B9 + ( seed << 6 ) + ( seed >> 2 ) );
        }
    }

    //=======================================================================================================================
    // RNG
    //=======================================================================================================================

    namespace Rng
    {
        #define STL_RNG_LCG 0
        #define STL_RNG_XORSHIFT 1
        #define STL_RNG_HASH 2

        uint _Next( uint seed )
        {
            #if( STL_RNG_NEXT_MODE == STL_RNG_LCG )
                seed = Sequence::Lcg( seed );
            #elif( STL_RNG_NEXT_MODE == STL_RNG_XORSHIFT )
                seed = Sequence::XorShift( seed );
            #else
                seed = Sequence::Hash( seed );
            #endif

            return seed;
        }

        #define STL_RNG_UTOF 0
        #define STL_RNG_MANTISSA_BITS 1

        #if( STL_RNG_FLOAT01_MODE == STL_RNG_UTOF )
            #define _UintToFloat01( x ) ( ( x >> 8 ) * ( 1.0 / float( 1 << 24 ) ) )
        #else
            #define _UintToFloat01( x ) ( 2.0 - asfloat( ( x >> 9 ) | 0x3F800000 ) )
        #endif

        // Tiny Encryption Algorithm
        namespace Tea
        {
            static uint2 g_TeaSeed;

            void Initialize( uint linearIndex, uint frameIndex, uint spinNum = 16 )
            {
                g_TeaSeed.x = linearIndex;
                g_TeaSeed.y = frameIndex;

                uint s = 0;
                [unroll]
                for( uint n = 0; n < spinNum; n++ )
                {
                    s += 0x9E3779B9;
                    g_TeaSeed.x += ( ( g_TeaSeed.y << 4 ) + 0xA341316C ) ^ ( g_TeaSeed.y + s ) ^ ( ( g_TeaSeed.y >> 5 ) + 0xC8013EA4 );
                    g_TeaSeed.y += ( ( g_TeaSeed.x << 4 ) + 0xAD90777D ) ^ ( g_TeaSeed.x + s ) ^ ( ( g_TeaSeed.x >> 5 ) + 0x7E95761E );
                }
            }

            void Initialize( uint2 samplePos, uint frameIndex, uint spinNum = 16 )
            { Initialize( Sequence::Zorder( samplePos ), frameIndex, spinNum ); }

            uint GetUint( )
            {
                g_TeaSeed.x = _Next( g_TeaSeed.x );

                return g_TeaSeed.x;
            }

            uint2 GetUint2( )
            {
                g_TeaSeed.x = _Next( g_TeaSeed.x );
                g_TeaSeed.y = _Next( g_TeaSeed.y );

                return g_TeaSeed;
            }

            uint4 GetUint4( )
            { return uint4( GetUint2( ), GetUint2( ) ); }

            float GetFloat( )
            { uint x = GetUint( ); return _UintToFloat01( x ); }

            float2 GetFloat2( )
            { uint2 x = GetUint2( ); return _UintToFloat01( x ); }

            float4 GetFloat4( )
            { uint4 x = GetUint4( ); return _UintToFloat01( x ); }
        }

        // Hash-based
        namespace Hash
        {
            static uint g_HashSeed;

            void Initialize( uint linearIndex, uint frameIndex )
            { g_HashSeed = Sequence::HashCombine( Sequence::Hash( frameIndex + 0x035F9F29 ), linearIndex ); }

            void Initialize( uint2 samplePos, uint frameIndex )
            { Initialize( Sequence::Zorder( samplePos ), frameIndex ); }

            uint GetUint( )
            {
                g_HashSeed = _Next( g_HashSeed );

                return g_HashSeed;
            }

            uint2 GetUint2( )
            { return uint2( GetUint( ), GetUint( ) ); }

            uint4 GetUint4( )
            { return uint4( GetUint2( ), GetUint2( ) ); }

            float GetFloat( )
            { uint x = GetUint( ); return _UintToFloat01( x ); }

            float2 GetFloat2( )
            { uint2 x = GetUint2( ); return _UintToFloat01( x ); }

            float4 GetFloat4( )
            { uint4 x = GetUint4( ); return _UintToFloat01( x ); }
        }
    }

    //=======================================================================================================================
    // BRDF
    //=======================================================================================================================

    /*
    LINKS:
    https://www.pbr-book.org/3ed-2018/contents
    https://google.github.io/filament/Filament.html#materialsystem/specularbrdf
    https://knarkowicz.wordpress.com/2014/12/27/analytical-dfg-term-for-ibl/
    https://blog.selfshadow.com/publications/
    */

    // "roughness" ( aka "alpha", aka "m" ) = specular or real roughness
    // "linearRoughness" ( aka "perceptual roughness", aka "artistic roughness" ) = sqrt( roughness )
    // G1 = G1( V, m ) is % visible in one direction
    // G2 = G2( L, V, m ) is % visible in two directions ( in practice, derived from G1 )
    // G2( uncorellated ) = G1( L, m ) * G1( V, m )
    // Specular BRDF = F * D * G2 / ( 4.0 * NoV * NoL )

    namespace BRDF
    {
        //======================================================================================================================
        // Constants
        //======================================================================================================================

        namespace IOR
        {
            static const float Vacuum      = 1.0;
            static const float Air         = 1.00029; // at sea level
            static const float Ice         = 1.31;
            static const float Water       = 1.333;
            static const float Quartz      = 1.46;
            static const float Glass       = 1.55;
            static const float Sapphire    = 1.77;
            static const float Diamond     = 2.42;
        };

        //======================================================================================================================
        // Misc
        //======================================================================================================================

        float Pow5( float x )
        { return Math::Pow01( 1.0 - x, 5.0 ); }

        void ConvertBaseColorMetalnessToAlbedoRf0( float3 baseColor, float metalness, out float3 albedo, out float3 Rf0 )
        {
            // TODO: ideally, STL_RF0_DIELECTRICS needs to be replaced with reflectance "STL_RF0_DIELECTRICS = 0.16 * reflectance * reflectance"
            // see https://google.github.io/filament/Filament.html#toc4.8
            // see https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf (page 13)
            albedo = baseColor * saturate( 1.0 - metalness );
            Rf0 = lerp( STL_RF0_DIELECTRICS, baseColor, metalness );
        }

        float FresnelTerm_Shadowing( float3 Rf0 )
        {
            // UE4: anything less than 2% is physically impossible and is instead considered to be shadowing
            return saturate( Color::Luminance( Rf0 ) / 0.02 );
        }

        //======================================================================================================================
        // Diffuse terms
        //======================================================================================================================

        float DiffuseTerm_Lambert( float linearRoughness, float NoL, float NoV, float VoH )
        {
            float d = 1.0;

            return d / Math::Pi( 1.0 );
        }

        // [Burley 2012, "Physically-Based Shading at Disney"]
        float DiffuseTerm_Burley( float linearRoughness, float NoL, float NoV, float VoH )
        {
            linearRoughness = saturate( linearRoughness );

            float energyBias = 0.5;
            float energyFactor = 1.0;

            float f = 2.0 * VoH * VoH * linearRoughness + energyBias - 1.0; // yes, linear roughness
            float FdV = 1.0 + f * Pow5( NoV );
            float FdL = 1.0 + f * Pow5( NoL );
            float d = FdV * FdL * energyFactor;

            return d / Math::Pi( 1.0 );
        }

        // [Lagarde 2014, "Moving Frostbite to Physically Based Rendering 3.0"]
        // https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf (page 10)
        float DiffuseTerm_BurleyFrostbite( float linearRoughness, float NoL, float NoV, float VoH )
        {
            linearRoughness = saturate( linearRoughness );

            float energyBias = lerp( 0.0, 0.5, linearRoughness );
            float energyFactor = lerp( 1.0, 1.0 / 1.51, linearRoughness );

            float f = 2.0 * VoH * VoH * linearRoughness + energyBias - 1.0; // yes, linear roughness
            float FdV = 1.0 + f * Pow5( NoV );
            float FdL = 1.0 + f * Pow5( NoL );
            float d = FdV * FdL * energyFactor;

            return d / Math::Pi( 1.0 );
        }

        // [Gotanda 2012, "Beyond a Simple Physically Based Blinn-Phong Model in Real-Time"]
        float DiffuseTerm_OrenNayar( float linearRoughness, float NoL, float NoV, float VoH )
        {
            float m = saturate( linearRoughness * linearRoughness );
            float m2 = m * m;
            float VoL = 2.0 * VoH - 1.0;
            float c1 = 1.0 - 0.5 * m2 / ( m2 + 0.33 );
            float cosri = VoL - NoV * NoL;
            float a = cosri >= 0.0 ? saturate( NoL * Math::PositiveRcp( NoV ) ) : NoL;
            float c2 = 0.45 * m2 / ( m2 + 0.09 ) * cosri * a;
            float d = NoL * c1 + c2;

            return d / Math::Pi( 1.0 );
        }

        float DiffuseTerm( float linearRoughness, float NoL, float NoV, float VoH )
        {
            // DiffuseTerm_Lambert
            // DiffuseTerm_Burley
            // DiffuseTerm_OrenNayar

            return DiffuseTerm_BurleyFrostbite( linearRoughness, NoL, NoV, VoH );
        }

        //======================================================================================================================
        // Distribution terms - how the microfacet normal distributed around a given direction
        //======================================================================================================================

        // [Blinn 1977, "Models of light reflection for computer synthesized pictures"]
        float DistributionTerm_Blinn( float linearRoughness, float NoH )
        {
            float m = saturate( linearRoughness * linearRoughness );
            float m2 = m * m;
            float alpha = 2.0 * Math::PositiveRcp( m2 ) - 2.0;
            float norm = ( alpha + 2.0 ) / 2.0;
            float d = norm * Math::Pow01( NoH, alpha );

            return d / Math::Pi( 1.0 );
        }

        // [Beckmann 1963, "The scattering of electromagnetic waves from rough surfaces"]
        float DistributionTerm_Beckmann( float linearRoughness, float NoH )
        {
            float m = saturate( linearRoughness * linearRoughness );
            float m2 = m * m;
            float b = NoH * NoH;
            float a = m2 * b;
            float d = exp( ( b - 1.0 ) * Math::PositiveRcp( a ) ) * Math::PositiveRcp( a * b );

            return d / Math::Pi( 1.0 );
        }

        // GGX / Trowbridge-Reitz, [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
        float DistributionTerm_GGX( float linearRoughness, float NoH )
        {
            float m = saturate( linearRoughness * linearRoughness );
            float m2 = m * m;

            #if 1
                float t = 1.0 - NoH * NoH * ( 0.99999994 - m2 );
                float a = max( m, STL_EPS ) / t;
            #else
                float t = ( NoH * m2 - NoH ) * NoH + 1.0;
                float a = m * Math::PositiveRcp( t );
            #endif

            float d = a * a;

            return d / Math::Pi( 1.0 );
        }

        // Generalized Trowbridge-Reitz, [Burley 2012, "Physically-Based Shading at Disney"]
        float DistributionTerm_GTR( float linearRoughness, float NoH )
        {
            float m = saturate( linearRoughness * linearRoughness );
            float m2 = m * m;
            float t = ( NoH * m2 - NoH ) * NoH + 1.0;

            float t1 = Math::Pow01( t, -STL_GTR_GAMMA );
            float t2 = 1.0 - Math::Pow01( m2, -( STL_GTR_GAMMA - 1.0 ) );
            float d = ( STL_GTR_GAMMA - 1.0 ) * ( m2 * t1 - t1 ) * Math::PositiveRcp( t2 );

            return d / Math::Pi( 1.0 );
        }

        float DistributionTerm( float linearRoughness, float NoH )
        {
            // DistributionTerm_Blinn
            // DistributionTerm_Beckmann
            // DistributionTerm_GGX
            // DistributionTerm_GTR

            return DistributionTerm_GGX( linearRoughness, NoH );
        }

        //======================================================================================================================
        // Geometry terms - how much the microfacet is blocked by other microfacet
        //======================================================================================================================

        // Known as "G1"
        float GeometryTerm_Smith( float linearRoughness, float NoVL )
        {
            float m = saturate( linearRoughness * linearRoughness );
            float m2 = m * m;
            float a = NoVL + Math::Sqrt01( ( NoVL - m2 * NoVL ) * NoVL + m2 );

            return 2.0 * NoVL * Math::PositiveRcp( a );
        }

        //======================================================================================================================
        // Geometry terms - how much the microfacet is blocked by other microfacet
        // G(mod) = G / ( 4.0 * NoV * NoL ): BRDF = F * D * G / ( 4 * NoV * NoL ) => BRDF = F * D * Gmod
        //======================================================================================================================

        float GeometryTermMod_Implicit( float linearRoughness, float NoL, float NoV, float VoH, float NoH )
        {
            return 0.25;
        }

        // [Neumann 1999, "Compact metallic reflectance models"]
        float GeometryTermMod_Neumann( float linearRoughness, float NoL, float NoV, float VoH, float NoH )
        {
            return 0.25 * Math::PositiveRcp( max( NoL, NoV ) );
        }

        // [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
        float GeometryTermMod_Schlick( float linearRoughness, float NoL, float NoV, float VoH, float NoH )
        {
            float m = saturate( linearRoughness * linearRoughness );

            // original form
            //float k = m * sqrt( 2.0 / Math::Pi( 1.0 ) );

            // UE4: tuned to match GGX [Karis]
            float k = m * 0.5;

            float a = NoL * ( 1.0 - k ) + k;
            float b = NoV * ( 1.0 - k ) + k;

            return 0.25 / max( a * b, STL_EPS );
        }

        // [Smith 1967, "Geometrical shadowing of a random rough surface"]
        // https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf
        // Known as "G2 height correlated"
        float GeometryTermMod_SmithCorrelated( float linearRoughness, float NoL, float NoV, float VoH, float NoH )
        {
            float m = saturate( linearRoughness * linearRoughness );
            float m2 = m * m;
            float a = NoV * Math::Sqrt01( ( NoL - m2 * NoL ) * NoL + m2 );
            float b = NoL * Math::Sqrt01( ( NoV - m2 * NoV ) * NoV + m2 );

            return 0.5 * Math::PositiveRcp( a + b );
        }

        // Smith term for GGX modified by Disney to be less "hot" for small roughness values
        // [Burley 2012, "Physically-Based Shading at Disney"]
        // Known as "G2 = G1( NoL ) * G1( NoV )"
        float GeometryTermMod_SmithUncorrelated( float linearRoughness, float NoL, float NoV, float VoH, float NoH )
        {
            float m = saturate( linearRoughness * linearRoughness );
            float m2 = m * m;
            float a = NoL + Math::Sqrt01( ( NoL - m2 * NoL ) * NoL + m2 );
            float b = NoV + Math::Sqrt01( ( NoV - m2 * NoV ) * NoV + m2 );

            return Math::PositiveRcp( a * b );
        }

        // [Cook and Torrance 1982, "A Reflectance Model for Computer Graphics"]
        float GeometryTermMod_CookTorrance( float linearRoughness, float NoL, float NoV, float VoH, float NoH )
        {
            float k = 2.0 * NoH / VoH;
            float a = min( k * NoV, k * NoL );

            return saturate( a ) * 0.25 * Math::PositiveRcp( NoV * NoL );
        }

        float GeometryTermMod( float linearRoughness, float NoL, float NoV, float VoH, float NoH )
        {
            // GeometryTermMod_Implicit
            // GeometryTermMod_Neumann
            // GeometryTermMod_Schlick
            // GeometryTermMod_SmithCorrelated
            // GeometryTermMod_SmithUncorrelated
            // GeometryTermMod_CookTorrance

            return GeometryTermMod_SmithCorrelated( linearRoughness, NoL, NoV, VoH, NoH );
        }

        //======================================================================================================================
        // Fresnel terms - the amount of light that reflects from a mirror surface given its index of refraction
        //======================================================================================================================

        float3 FresnelTerm_None( float3 Rf0, float VoNH )
        {
            return Rf0;
        }

        // [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
        float3 FresnelTerm_Schlick( float3 Rf0, float VoNH )
        {
            return Rf0 + ( 1.0 - Rf0 ) * Pow5( VoNH );
        }

        float3 FresnelTerm_Fresnel( float3 Rf0, float VoNH )
        {
            float3 nu = Math::Sqrt01( Rf0 );
            nu = ( 1.0 + nu ) * Math::PositiveRcp( 1.0 - nu );

            float k = VoNH * VoNH - 1.0;
            float3 g = sqrt( nu * nu + k );
            float3 a = ( g - VoNH ) / ( g + VoNH );
            float3 c = ( g * VoNH + k ) / ( g * VoNH - k );

            return 0.5 * a * a * ( c * c + 1.0 );
        }

        // http://www.pbr-book.org/3ed-2018/Reflection_Models/Specular_Reflection_and_Transmission.html
        // eta - relative index of refraction "from" / "to"
        float FresnelTerm_Dielectric( float eta, float VoN )
        {
            float saSq = eta * eta * ( 1.0 - VoN * VoN );

            // Cosine of angle between negative normal and transmitted direction ( 0 for total internal reflection )
            float ca = Math::Sqrt01( 1.0 - saSq );

            float Rs = ( eta * VoN - ca ) * Math::PositiveRcp( eta * VoN + ca );
            float Rp = ( eta * ca - VoN ) * Math::PositiveRcp( eta * ca + VoN );

            return 0.5 * ( Rs * Rs + Rp * Rp );
        }

        float3 FresnelTerm( float3 Rf0, float VoNH )
        {
            // FresnelTerm_None
            // FresnelTerm_Schlick
            // FresnelTerm_Fresnel

            return FresnelTerm_Schlick( Rf0, VoNH ) * FresnelTerm_Shadowing( Rf0 );
        }

        //======================================================================================================================
        // Environment terms ( integrated over hemisphere )
        //======================================================================================================================

        // Approximate Models For Physically Based Rendering by Angelo Pesce and Michael Iwanicki
        // http://miciwan.com/SIGGRAPH2015/course_notes_wip.pdf
        float3 EnvironmentTerm_Pesce( float3 Rf0, float NoV, float linearRoughness )
        {
            float m = saturate( linearRoughness * linearRoughness );

            float a = 7.0 * NoV + 4.0 * m;
            float bias = exp2( -a );

            float b = min( linearRoughness, 0.739 + 0.323 * NoV ) - 0.434;
            float scale = 1.0 - bias - m * max( bias, b );

            return saturate( Rf0 * scale + bias );
        }

        // Shlick's approximation for Ross BRDF - makes Fresnel converge to less than 1.0 when NoV is low
        // https://hal.inria.fr/inria-00443630/file/article-1.pdf
        float3 EnvironmentTerm_Ross( float3 Rf0, float NoV, float linearRoughness )
        {
            float m = saturate( linearRoughness * linearRoughness );

            float f = Math::Pow01( 1.0 - NoV, 5.0 * exp( -2.69 * m ) ) / ( 1.0 + 22.7 * Math::Pow01( m, 1.5 ) );

            float scale = 1.0 - f;
            float bias = f;

            return saturate( Rf0 * scale + bias );
        }

        // "Ray Tracing Gems", Chapter 32, Equation 4 - the approximation assumes GGX VNDF and Schlick's approximation
        float3 EnvironmentTerm_Rtg( float3 Rf0, float NoV, float linearRoughness )
        {
            float m = saturate( linearRoughness * linearRoughness );

            float4 X;
            X.x = 1.0;
            X.y = NoV;
            X.z = NoV * NoV;
            X.w = NoV * X.z;

            float4 Y;
            Y.x = 1.0;
            Y.y = m;
            Y.z = m * m;
            Y.w = m * Y.z;

            float2x2 M1 = float2x2( 0.99044, -1.28514, 1.29678, -0.755907 );
            float3x3 M2 = float3x3( 1.0, 2.92338, 59.4188, 20.3225, -27.0302, 222.592, 121.563, 626.13, 316.627 );

            float2x2 M3 = float2x2( 0.0365463, 3.32707, 9.0632, -9.04756 );
            float3x3 M4 = float3x3( 1.0, 3.59685, -1.36772, 9.04401, -16.3174, 9.22949, 5.56589, 19.7886, -20.2123 );

            float bias = dot( mul( M1, X.xy ), Y.xy ) * Math::PositiveRcp( dot( mul( M2, X.xyw ), Y.xyw ) );
            float scale = dot( mul( M3, X.xy ), Y.xy ) * Math::PositiveRcp( dot( mul( M4, X.xzw ), Y.xyw ) );

            return saturate( Rf0 * scale + bias );
        }

        float3 EnvironmentTerm( float3 Rf0, float NoV, float linearRoughness )
        {
            // EnvironmentTerm_Pesce
            // EnvironmentTerm_Ross
            // EnvironmentTerm_Unknown

            return EnvironmentTerm_Ross( Rf0, NoV, linearRoughness ) * FresnelTerm_Shadowing( Rf0 );
        }

        //======================================================================================================================
        // Direct lighting
        //======================================================================================================================

        void DirectLighting( float3 N, float3 L, float3 V, float3 Rf0, float linearRoughness, out float3 Cdiff, out float3 Cspec )
        {
            float3 H = normalize( L + V );

            float NoL = saturate( dot( N, L ) );
            float NoH = saturate( dot( N, H ) );
            float VoH = saturate( dot( V, H ) );

            // Due to normal mapping can easily be < 0, it blows up GeometryTerm (Smith)
            float NoV = abs( dot( N, V ) );

            float D = DistributionTerm( linearRoughness, NoH );
            float G = GeometryTermMod( linearRoughness, NoL, NoV, VoH, NoH );
            float3 F = FresnelTerm( Rf0, VoH );
            float Kdiff = DiffuseTerm( linearRoughness, NoL, NoV, VoH );

            Cspec = F * D * G * NoL;
            Cdiff = ( 1.0 - F ) * Kdiff * NoL;

            Cspec = saturate( Cspec );
        }
    }

    //=======================================================================================================================
    // IMPORTANCE SAMPLING
    //=======================================================================================================================

    namespace ImportanceSampling
    {
        // Defines a cone angle, where micro-normals are distributed
        float GetSpecularLobeHalfAngle( float linearRoughness, float percentOfVolume = 0.75 )
        {
            float m = saturate( linearRoughness * linearRoughness );

            // Comparison of two methods:
            // https://www.desmos.com/calculator/4vvg1qrec7
            #if 1
                // [Lagarde 2014, "Moving Frostbite to Physically Based Rendering 3.0"]
                // https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf (page 72)
                // TODO: % of NDF volume - is it the trimming factor from VNDF sampling?
                return atan( m * percentOfVolume / ( 1.0 - percentOfVolume ) );
            #else
                return Math::DegToRad( 180.0 ) * m / ( 1.0 + m );
            #endif
        }

        float GetSpecularLobeTanHalfAngle( float linearRoughness, float percentOfVolume = 0.75 )
        {
            float m = saturate( linearRoughness * linearRoughness );

            #if 1
                return m * percentOfVolume / ( 1.0 - percentOfVolume );
            #else
                return tan( Math::DegToRad( 180.0 ) * m / ( 1.0 + m ) );
            #endif
        }

        float3 CorrectDirectionToInfiniteSource( float3 N, float3 L, float3 V, float tanOfAngularSize )
        {
            float3 R = reflect( -V, N );
            float3 centerToRay = L - dot( L, R ) * R;
            float3 closestPoint = centerToRay * saturate( tanOfAngularSize * Math::Rsqrt( Math::LengthSquared( centerToRay ) ) );

            return normalize( L - closestPoint );
        }

        // [Lagarde 2014, "Moving Frostbite to Physically Based Rendering 3.0"]
        // https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf (page 69)
        #define STL_SPECULAR_DOMINANT_DIRECTION_G1      0
        #define STL_SPECULAR_DOMINANT_DIRECTION_G2      1
        #define STL_SPECULAR_DOMINANT_DIRECTION_APPROX  2

        float GetSpecularDominantFactor( float NoV, float linearRoughness, compiletime const uint mode = STL_SPECULAR_DOMINANT_DIRECTION_DEFAULT )
        {
            float dominantFactor;
            if( mode == STL_SPECULAR_DOMINANT_DIRECTION_G2 )
            {
                float a = 0.298475 * log( 39.4115 - 39.0029 * linearRoughness );
                dominantFactor = Math::Pow01( 1.0 - NoV, 10.8649 ) * ( 1.0 - a ) + a;
            }
            else if( mode == STL_SPECULAR_DOMINANT_DIRECTION_G1 )
                dominantFactor = 0.298475 * NoV * log( 39.4115 - 39.0029 * linearRoughness ) + ( 0.385503 - 0.385503 * NoV ) * log( 13.1567 - 12.2848 * linearRoughness );
            else
            {
                float s = 1.0 - linearRoughness;
                dominantFactor = s * ( Math::Sqrt01( s ) + linearRoughness );
            }

            return saturate( dominantFactor );
        }

        float3 GetSpecularDominantDirectionWithFactor( float3 N, float3 V, float dominantFactor )
        {
            float3 R = reflect( -V, N );
            float3 D = lerp( N, R, dominantFactor );

            return normalize( D );
        }

        float4 GetSpecularDominantDirection( float3 N, float3 V, float linearRoughness, compiletime const uint mode = STL_SPECULAR_DOMINANT_DIRECTION_DEFAULT )
        {
            float NoV = abs( dot( N, V ) );
            float dominantFactor = GetSpecularDominantFactor( NoV, linearRoughness, mode );

            return float4( GetSpecularDominantDirectionWithFactor( N, V, dominantFactor ), dominantFactor );
        }

        //=================================================================================
        // Uniform
        //=================================================================================

        namespace Uniform
        {
            float GetPDF( )
            {
                return 1.0 / Math::Pi( 2.0 );
            }

            float3 GetRay( float2 rnd )
            {
                float phi = rnd.x * Math::Pi( 2.0 );

                float cosTheta = rnd.y;
                float sinTheta = Math::Sqrt01( 1.0 - cosTheta * cosTheta );

                float3 ray;
                ray.x = sinTheta * cos( phi );
                ray.y = sinTheta * sin( phi );
                ray.z = cosTheta;

                return ray;
            }
        }

        //=================================================================================
        // Cosine
        //=================================================================================

        namespace Cosine
        {
            float GetPDF( float NoL = 1.0 ) // default can be useful to handle NoL cancelation ( PDF's NoL cancels throughput's NoL )
            {
                float pdf = NoL / Math::Pi( 1.0 );

                return max( pdf, 1e-7 );
            }

            float3 GetRay( float2 rnd )
            {
                float phi = rnd.x * Math::Pi( 2.0 );

                float cosTheta = Math::Sqrt01( rnd.y );
                float sinTheta = Math::Sqrt01( 1.0 - cosTheta * cosTheta );

                float3 ray;
                ray.x = sinTheta * cos( phi );
                ray.y = sinTheta * sin( phi );
                ray.z = cosTheta;

                return ray;
            }
        }

        //=================================================================================
        // GGX-NDF
        //=================================================================================

        namespace NDF
        {
            float GetPDF( float linearRoughness, float NoH, float VoH )
            {
                float pdf = BRDF::DistributionTerm_GGX( linearRoughness, NoH );
                pdf *= NoH;
                pdf *= Math::PositiveRcp( 4.0 * VoH );

                return max( pdf, 1e-7 );
            }

            float3 GetRay( float2 rnd, float linearRoughness )
            {
                float m = saturate( linearRoughness * linearRoughness );
                float m2 = m * m;
                float t = ( m2 - 1.0 ) * rnd.y + 1.0;
                float cosThetaSq = ( 1.0 - rnd.y ) * Math::PositiveRcp( t );
                float sinTheta = Math::Sqrt01( 1.0 - cosThetaSq );
                float phi = rnd.x * Math::Pi( 2.0 );

                float3 ray;
                ray.x = sinTheta * cos( phi );
                ray.y = sinTheta * sin( phi );
                ray.z = Math::Sqrt01( cosThetaSq );

                return ray;
            }
        }

        //=================================================================================
        // GGX-VNDF
        // STL_VNDF_VERSION == 1: http://jcgt.org/published/0007/04/01/paper.pdf
        // STL_VNDF_VERSION == 2: https://diglib.eg.org/bitstream/handle/10.1111/cgf14867/v42i8_03_14867.pdf
        // STL_VNDF_VERSION == 3: https://gpuopen.com/download/publications/Bounded_VNDF_Sampling_for_Smith-GGX_Reflections.pdf
        //=================================================================================

        namespace VNDF
        {
            float GetPDF( float3 Vlocal, float NoH, float linearRoughness )
            {
                float2 m = saturate( linearRoughness * linearRoughness );

                float a = min( m.x, m.y );
                float a2 = a * a;

                float D = BRDF::DistributionTerm_GGX( linearRoughness, NoH );

                #if( STL_VNDF_VERSION == 1 )
                    /*
                    Eq 3  : Dv( H ) = G1 * D * VoH / NoV
                    Eq 17 : PDF = Dv( H ) / ( 4 * VoH )

                    Simplification:
                        G1 = BRDF::GeometryTerm_Smith( linearRoughness, NoV )
                        D = BRDF::DistributionTerm_GGX( linearRoughness, NoH )

                        m2 = m * m
                        G1mod = 1 / [ NoV + Math::Sqrt01( ( NoV - m2 * NoV ) * NoV + m2 ) ]
                        G1 = 2 * G1mod * NoV

                        => PDF = ( 2 * G1mod * NoV * D * VoH / NoV ) / ( 4 * VoH )
                        => PDF = ( 2 * G1mod * D * VoH ) / ( 4 * VoH )
                        => PDF = ( 2 * G1mod * D ) / 4
                        => PDF = ( G1mod * D ) / 2
                    */
                    float G1mod = Math::PositiveRcp( Vlocal.z + Math::Sqrt01( ( Vlocal.z - a2 * Vlocal.z ) * Vlocal.z + a2 ) ); // see GeometryTerm_Smith

                    return 0.5 * G1mod * D;
                #else
                    float2 wi = m * Vlocal.xy;
                    float lenSq = dot(wi, wi);
                    float t = sqrt(lenSq + Vlocal.z * Vlocal.z);

                    if( Vlocal.z < 0.0 )
                        return 0.5 * D * ( t - Vlocal.z ) / lenSq;

                    #if( STL_VNDF_VERSION == 3 )
                        float s = 1.0 + length( Vlocal.xy );
                        float s2 = s * s;
                        float k = ( 1.0 - a2 ) * s2 / ( s2 + a2 * Vlocal.z * Vlocal.z );
                    #else
                        float k = 1.0;
                    #endif

                    return 0.5 * D / ( k * Vlocal.z + t );
                #endif
            }

            float3 GetRay( float2 rnd, float2 linearRoughness, float3 Vlocal, float trimFactor = 1.0 )
            {
                /*
                Trimming:
                    0.0  - dominant direction
                    1.0  - full lobe
                    0.95 - allows to cut off samples with very low probabilities ( good for denoising )
                */
                rnd.y *= trimFactor;

                // TODO: instead of using 2 roughness values introduce "anisotropy" parameter
                // https://blog.selfshadow.com/publications/s2013-shading-course/rad/s2013_pbs_rad_notes.pdf (page 3)
                float2 m = saturate( linearRoughness * linearRoughness );

                // Warp to the hemisphere configuration
                float3 wi = normalize( float3( Vlocal.xy * m, Vlocal.z ) );

                float phi = Math::Pi( 2.0 ) * rnd.x;
                #if( STL_VNDF_VERSION == 1 )
                    float lenSq = dot( wi.xy, wi.xy );
                    float3 X = lenSq > STL_EPS ? float3( -wi.y, wi.x, 0.0 ) * rsqrt( lenSq ) : float3( 1.0, 0.0, 0.0 );
                    float3 Y = cross( wi, X );

                    float z = 0.5 * ( 1.0 + wi.z );
                    float r = Math::Sqrt01( rnd.y );
                    float x = r * cos( phi );
                    float y = lerp( Math::Sqrt01( 1.0 - x * x ), r * sin( phi ), z );

                    float3 h = x * X + y * Y + Math::Sqrt01( 1.0 - x * x - y * y ) * wi;
                #else
                    #if( STL_VNDF_VERSION == 2 )
                        float b = wi.z;
                    #elif( STL_VNDF_VERSION == 3 )
                        float a = min( m.x, m.y );
                        float s = 1.0 + length( Vlocal.xy );
                        float a2 = a * a;
                        float s2 = s * s;
                        float k = ( 1.0 - a2 ) * s2 / ( s2 + a2 * Vlocal.z * Vlocal.z );
                        float b = Vlocal.z > 0.0 ? k * wi.z : wi.z;
                    #endif

                    float z = 1.0 - rnd.y * ( 1.0 + b );
                    float r = Math::Sqrt01( 1.0 - z * z );
                    float x = r * cos( phi );
                    float y = r * sin( phi );

                    float3 h = float3( x, y, z ) + wi;
                #endif

                // Warp back to the ellipsoid configuration
                return normalize( float3( m * h.xy, max( h.z, STL_EPS ) ) );
            }
        }
    }

    //=======================================================================================================================
    // SPHERICAL HARMONICS
    //=======================================================================================================================

    // https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/gdc2018-precomputedgiobalilluminationinfrostbite.pdf

    struct SH1
    {
        float3 c0_chroma;
        float3 c1;
    };

    namespace SphericalHarmonics
    {
        SH1 ConvertToSecondOrder( float3 color, float3 direction )
        {
            float3 YCoCg = Color::RgbToYCoCg( color );

            SH1 sh;
            sh.c0_chroma = YCoCg;
            sh.c1 = YCoCg.x * direction;

            return sh;
        }

        float3 ExtractColor( SH1 sh )
        {
            return Color::YCoCgToRgb( sh.c0_chroma );
        }

        float3 ExtractDirection( SH1 sh )
        {
            return sh.c1 * Math::Rsqrt( Math::LengthSquared( sh.c1 ) );
        }

        void Add( inout SH1 result, SH1 x )
        {
            result.c0_chroma += x.c0_chroma;
            result.c1 += x.c1;
        }

        void Mul( inout SH1 result, float x )
        {
            result.c0_chroma *= x;
            result.c1 *= x;
        }

        float3 ResolveColor( SH1 sh, float3 N )
        {
            float Y = 0.5 * dot( N, sh.c1 ) + 0.25 * sh.c0_chroma.x;

            // 2 - hemisphere, 4 - sphere
            Y *= 2.0;

            // Corrected color reproduction
            Y = max( Y, 0.0 );

            float modifier = ( Y + STL_EPS ) / ( sh.c0_chroma.x + STL_EPS );
            float2 CoCg = sh.c0_chroma.yz * modifier;

            return Color::YCoCgToRgb( float3( Y, CoCg ) );
        }
    }

    //=======================================================================================================================
    // TEXT
    //=======================================================================================================================

    namespace Text
    {
        //===================================================================================================================
        // Constants
        //===================================================================================================================

        static const uint Char_0            = 0;
        static const uint Char_Minus        = 10;
        static const uint Char_Dot          = 11;
        static const uint Char_Space        = 12;
        static const uint Char_Ampersand    = 13;
        static const uint Char_A            = 14;

        //===================================================================================================================
        // Private
        //===================================================================================================================

        static const uint CHAR_W            = 5;
        static const uint CHAR_H            = 6;

        static const uint BACKGROUND        = 1;
        static const uint FOREGROUND        = 2;

        #define X                           1
        #define _                           0

    #if( STL_WINDOW_ORIGIN == STL_OGL )
        #define CHAR( a1, a2, a3, a4, a5, \
                      b1, b2, b3, b4, b5, \
                      c1, c2, c3, c4, c5, \
                      d1, d2, d3, d4, d5, \
                      e1, e2, e3, e4, e5, \
                      f1, f2, f3, f4, f5 )        ( ( f1 <<  0 ) | ( f2 <<  1 ) | ( f3 <<  2 ) | ( f4 <<  3 ) | ( f5 <<  4 ) | \
                                                    ( e1 <<  5 ) | ( e2 <<  6 ) | ( e3 <<  7 ) | ( e4 <<  8 ) | ( e5 <<  9 ) | \
                                                    ( d1 << 10 ) | ( d2 << 11 ) | ( d3 << 12 ) | ( d4 << 13 ) | ( d5 << 14 ) | \
                                                    ( c1 << 15 ) | ( c2 << 16 ) | ( c3 << 17 ) | ( c4 << 18 ) | ( c5 << 19 ) | \
                                                    ( b1 << 20 ) | ( b2 << 21 ) | ( b3 << 22 ) | ( b4 << 23 ) | ( b5 << 24 ) | \
                                                    ( a1 << 25 ) | ( a2 << 26 ) | ( a3 << 27 ) | ( a4 << 28 ) | ( a5 << 29 ) )
    #else
        #define CHAR( a1, a2, a3, a4, a5, \
                      b1, b2, b3, b4, b5, \
                      c1, c2, c3, c4, c5, \
                      d1, d2, d3, d4, d5, \
                      e1, e2, e3, e4, e5, \
                      f1, f2, f3, f4, f5 )        ( ( a1 <<  0 ) | ( a2 <<  1 ) | ( a3 <<  2 ) | ( a4 <<  3 ) | ( a5 <<  4 ) | \
                                                    ( b1 <<  5 ) | ( b2 <<  6 ) | ( b3 <<  7 ) | ( b4 <<  8 ) | ( b5 <<  9 ) | \
                                                    ( c1 << 10 ) | ( c2 << 11 ) | ( c3 << 12 ) | ( c4 << 13 ) | ( c5 << 14 ) | \
                                                    ( d1 << 15 ) | ( d2 << 16 ) | ( d3 << 17 ) | ( d4 << 18 ) | ( d5 << 19 ) | \
                                                    ( e1 << 20 ) | ( e2 << 21 ) | ( e3 << 22 ) | ( e4 << 23 ) | ( e5 << 24 ) | \
                                                    ( f1 << 25 ) | ( f2 << 26 ) | ( f3 << 27 ) | ( f4 << 28 ) | ( f5 << 29 ) )
    #endif

        static const uint font[ ] =
        {
            CHAR( _, X, X, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  _, X, X, X, _ ),

            CHAR( _, _, _, _, X,
                  _, _, _, X, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  _, _, _, _, X ),

            CHAR( X, X, X, X, _,
                  _, _, _, _, X,
                  X, X, X, X, X,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, X, X, X, X ),

            CHAR( X, X, X, X, _,
                  _, _, _, _, X,
                  _, X, X, X, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  X, X, X, X, _ ),

            CHAR( X, _, _, _, X,
                  X, _, _, _, X,
                  X, X, X, X, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  _, _, _, _, X ),

            CHAR( X, X, X, X, _,
                  X, _, _, _, _,
                  X, X, X, X, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  X, X, X, X, _ ),

            CHAR( _, X, X, X, X,
                  X, _, _, _, _,
                  X, X, X, X, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, X, X, X, _ ),

            CHAR( _, X, X, X, _,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  _, _, _, _, X ),

            CHAR( _, X, X, X, _,
                  X, _, _, _, X,
                  X, X, X, X, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  _, X, X, X, _ ),

            CHAR( _, X, X, X, _,
                  X, _, _, _, X,
                  X, X, X, X, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  X, X, X, X, _ ),

            CHAR( _, _, _, _, _,
                  _, _, _, _, _,
                  X, X, X, X, X,
                  _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, _, _, _ ),

            CHAR( _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, X, _, _ ),

            CHAR( _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, _, _, _,
                  _, _, _, _, _ ),

            CHAR( _, _, X, _, _,
                  _, X, _, X, _,
                  _, _, X, _, _,
                  _, X, _, X, _,
                  X, _, _, X, _,
                  _, X, X, _, X ),

            CHAR( _, _, X, X, X,
                  _, X, _, _, X,
                  X, _, _, _, X,
                  X, X, X, X, X,
                  X, _, _, _, X,
                  X, _, _, _, X ),

            CHAR( X, X, X, X, _,
                  X, _, _, _, X,
                  X, X, X, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, X, X, X, _ ),

            CHAR( _, X, X, X, X,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  _, X, X, X, X ),

            CHAR( X, X, X, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, X, X, X, _ ),

            CHAR( X, X, X, X, X,
                  X, _, _, _, _,
                  X, X, X, X, _,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, X, X, X, X ),

            CHAR( X, X, X, X, X,
                  X, _, _, _, _,
                  X, X, X, X, _,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, _, _, _, _ ),

            CHAR( _, X, X, X, X,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, _, _, X, X,
                  X, _, _, _, X,
                  _, X, X, X, X ),

            CHAR( X, _, _, _, X,
                  X, _, _, _, X,
                  X, X, X, X, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X ),

            CHAR( _, X, X, X, _,
                  _, _, X, _, _,
                  _, _, X, _, _,
                  _, _, X, _, _,
                  _, _, X, _, _,
                  _, X, X, X, _ ),

            CHAR( _, _, _, _, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  X, _, _, _, X,
                  _, X, X, X, _ ),

            CHAR( X, _, _, _, X,
                  X, _, _, X, _,
                  X, X, X, _, _,
                  X, _, _, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X ),

            CHAR( X, _, _, _, _,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, _, _, _, _,
                  X, X, X, X, X ),

            CHAR( X, _, _, _, X,
                  X, X, _, X, X,
                  X, _, X, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X ),

            CHAR( X, _, _, _, X,
                  X, X, _, _, X,
                  X, _, X, _, X,
                  X, _, _, X, X,
                  X, _, _, _, X,
                  X, _, _, _, X ),

            CHAR( _, X, X, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  _, X, X, X, _ ),

            CHAR( X, X, X, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, X, X, X, _,
                  X, _, _, _, _,
                  X, _, _, _, _ ),

            CHAR( _, X, X, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, X, _, X,
                  X, _, _, X, _,
                  _, X, X, _, X ),

            CHAR( X, X, X, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, X, X, X, _,
                  X, _, _, X, _,
                  X, _, _, _, X ),

            CHAR( _, X, X, X, X,
                  X, _, _, _, _,
                  _, X, X, X, _,
                  _, _, _, _, X,
                  _, _, _, _, X,
                  X, X, X, X, _ ),

            CHAR( X, X, X, X, X,
                  _, _, X, _, _,
                  _, _, X, _, _,
                  _, _, X, _, _,
                  _, _, X, _, _,
                  _, _, X, _, _ ),

            CHAR( X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  _, X, X, X, _ ),

            CHAR( X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, _, _, X,
                  _, X, _, X, _,
                  _, _, X, _, _ ),

            CHAR( X, _, _, _, X,
                  X, _, _, _, X,
                  X, _, X, _, X,
                  X, _, X, _, X,
                  X, X, X, X, X,
                  _, X, _, X, _ ),

            CHAR( X, _, _, _, X,
                  _, X, _, X, _,
                  _, _, X, _, _,
                  _, X, _, X, _,
                  X, _, _, _, X,
                  X, _, _, _, X ),

            CHAR( X, _, _, _, X,
                  _, X, _, X, _,
                  _, X, _, X, _,
                  _, _, X, _, _,
                  _, _, X, _, _,
                  _, _, X, _, _ ),

            CHAR( X, X, X, X, X,
                  _, _, _, X, _,
                  _, _, X, _, _,
                  _, X, _, _, _,
                  X, _, _, _, _,
                  X, X, X, X, X ),
        };

        #undef CHAR
        #undef _
        #undef X

        #if( STL_TEXT_WITH_NICE_ONE_PIXEL_BACKGROUND == 1 )

            #define _RenderChar( ch )  \
                if( all( int2( state.xy ) >= -int( state.z ) && int2( state.xy ) < int2( CHAR_W + 1, CHAR_H + 1 ) * int( state.z ) ) ) \
                { \
                    state.w = BACKGROUND; \
                    if( all( state.xy < uint2( CHAR_W, CHAR_H ) * state.z ) ) \
                    { \
                        uint2 p = state.xy / state.z; \
                        uint bits = font[ ch ]; \
                        uint bit = bits & ( 1u << ( p.y * CHAR_W + p.x ) ); \
                        state.w = bit ? FOREGROUND : BACKGROUND; \
                    } \
                }

        #else

            #define _RenderChar( ch )  \
                [flatten] \
                if( all( state.xy < uint2( CHAR_W, CHAR_H ) * state.z ) ) \
                { \
                    uint2 p = state.xy / state.z; \
                    uint bits = font[ ch ]; \
                    uint bit = bits & ( 1u << ( p.y * CHAR_W + p.x ) ); \
                    state.w = bit ? FOREGROUND : BACKGROUND; \
                }

        #endif

        //===================================================================================================================
        // Interface
        //===================================================================================================================

        uint4 Init( uint2 pixelPos, uint2 origin, uint scale )
        {
            uint4 state;
            state.xy = pixelPos - origin;
            state.z = scale;
            state.w = 0;

            return state;
        }

        bool IsBackground( uint4 state )
        { return state.w == BACKGROUND; }

        bool IsForeground( uint4 state )
        { return state.w == FOREGROUND; }

        void NextChar( inout uint4 state )
        { state.x -= ( CHAR_W + 1 ) * state.z; }

        void NextDigit( inout uint4 state )
        { state.x += ( CHAR_W + 1 ) * state.z; }

        void Print_ui( uint x, inout uint4 state )
        {
            while( x )
            {
                uint a = x / 10;
                uint digit = x - a * 10;

                NextDigit( state );
                _RenderChar( digit );

                x = a;
            }
        }

        void Print_i( int x, inout uint4 state )
        {
            // Absolute value
            Print_ui( abs( x ), state );

            // Minus
            if( x < 0 )
            {
                NextDigit( state );
                _RenderChar( Char_Minus );
            }
        }

        void Print_f( float x, inout uint4 state )
        {
            // Fractional part
            uint f = uint( frac( abs( x ) ) * STL_TEXT_DIGIT_FORMAT + 0.5 );
            Print_ui( f, state );

            // Zeros after dot
            [unroll]
            for( uint n = 10; n < STL_TEXT_DIGIT_FORMAT; n *= 10 )
            {
                if( f < n )
                {
                    NextDigit( state );
                    _RenderChar( Char_0 );
                }
            }

            // Dot
            NextDigit( state );
            _RenderChar( Char_Dot );

            // Integer part
            int i = int( floor( x ) + 0.5 );
            Print_i( i, state );
        }

        void Print_ch( uint ch, inout uint4 state )
        {
            if( ch == Char_Minus || ch == '-' )
                ch = Char_Minus;
            else if( ch == Char_Dot || ch == '.' )
                ch = Char_Dot;
            else if( ch == Char_Space || ch == ' ' )
                ch = Char_Space;
            else if( ch == Char_Ampersand || ch == '&' )
                ch = Char_Ampersand;
            else
                ch = Char_A + ch - 'A';

            _RenderChar( ch );
            NextChar( state );
        }
    }
}

#endif
