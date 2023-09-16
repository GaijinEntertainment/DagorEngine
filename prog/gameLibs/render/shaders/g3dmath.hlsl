#ifndef g3dmath_glsl // -*- c++ -*-
#define g3dmath_glsl

/**
  \file g3dmath.glsl
  \author Morgan McGuire (http://graphics.cs.williams.edu), Michael Mara (http://www.illuminationcodified.com)
  Defines basic mathematical constants and functions used in several shaders
*/

// Some constants
static const float pi_          = 3.1415927;
static const float invPi       = 1.0 / 3.1415927;
static const float inv8Pi      = 1.0 / (8.0 * 3.1415927);

static const float meters      = 1.0;
static const float centimeters = 0.01;
static const float millimeters = 0.001;

#define dFdx ddx
#define dFdy ddy
#define fract frac
#define mix lerp

// Avoid the 1/0 underflow warning (requires GLSL 400 or later):
static const float inf         =
#if !SHADER_COMPILER_DXC
#if __VERSION__ >= 420 && !_HARDWARE_METAL
    intBitsToFloat(0x7f800000);
#else
    1.0 / 0.0;
#endif
#else
    1.0 / 0.0;
#endif

#define vec1 float1
#define vec2 float2
#define vec3 float3
#define vec4 float4
#define ivec1 int1
#define ivec2 int2
#define ivec3 int3
#define ivec4 int4

#define uvec1 uint1
#define uvec2 uint2
#define uvec3 uint3
#define uvec4 uint4

#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4
#define mat4x4 float4x4

#define Vector2 vec2
#define Point2  vec2
#define Vector3 vec3
#define Point3  vec3
#define Vector4 vec4

#define Color3  vec3
#define Radiance3 vec3
#define Biradiance3 vec3
#define Irradiance3 vec3
#define Radiosity3 vec3
#define Power3 vec3

#define Color4  vec4
#define Radiance4 vec4
#define Biradiance4 vec4
#define Irradiance4 vec4
#define Radiosity4 vec4
#define Power4 vec4

#define Vector2int32 int2
#define Vector3int32 int3
#define Matrix4      mat4
#define Matrix3      mat3
#define Matrix2      mat2

/** Given a z-axis, construct an orthonormal tangent space. Assumes z is a unit vector */
Matrix3 referenceFrameFromZAxis(Vector3 z) {
    Vector3 y = (abs(z.y) > 0.85) ? Vector3(-1, 0, 0) : Vector3(0, 1, 0);
    Vector3 x = normalize(cross(y, z));
    y = normalize(cross(z, x));
    return Matrix3(x, y, z);
}




float square(in float x) { return x * x; }
float2 square(in float2 x) { return x * x; }
float3 square(in float3 x) { return x * x; }
float4 square(in float4 x) { return x * x; }


float lengthSquared(float2 v) {return dot(v, v);}
float lengthSquared(float3 v) {return dot(v, v);}
float lengthSquared(float4 v) {return dot(v, v);}

/** Computes x<sup>5</sup> */


float maxComponent(vec2 a) {
    return max(a.x, a.y);
}

float maxComponent(vec3 a) {
    return max3(a.x, a.y, a.z);
}

float maxComponent(vec4 a) {
    return max4(a.x, a.y, a.z, a.w);
}


float minComponent(vec2 a) {
    return min(a.x, a.y);
}

float minComponent(vec3 a) {
    return min3(a.x, a.y, a.z);
}

float minComponent(vec4 a) {
    return min4(a.x, a.y, a.z, a.w);
}

float meanComponent(vec4 a) {
    return (a.r + a.g + a.b + a.a) * (1.0 / 4.0);
}

float meanComponent(vec3 a) {
    return (a.r + a.g + a.b) * (1.0 / 3.0);
}

float meanComponent(vec2 a) {
    return (a.r + a.g) * (1.0 / 2.0);
}

float meanComponent(float a) {
    return a;
}



/** http://blog.selfshadow.com/2011/07/22/specular-showdown/ */ 
float computeToksvigGlossyExponent(float rawGlossyExponent, float rawNormalLength) {
    rawNormalLength = min(1.0, rawNormalLength);
    float ft = rawNormalLength / lerp(max(0.1, rawGlossyExponent), 1.0, rawNormalLength);
    float scale = (1.0 + ft * rawGlossyExponent) / (1.0 + rawGlossyExponent);
    return scale * rawGlossyExponent;
}


/** \cite http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl */
vec3 RGBtoHSV(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = (c.g < c.b) ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
    vec4 q = (c.r < p.x) ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float eps = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + eps)), d / (q.x + eps), q.x);
}

/** \cite http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl */
vec3 HSVtoRGB(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), clamp(c.y, 0.0, 1.0));
}


/** 
 Generate the ith 2D Hammersley point out of N on the unit square [0, 1]^2
 \cite http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html */
vec2 hammersleySquare(uint i, const uint N) {
    vec2 P;
    P.x = float(i) * (1.0 / float(N));

    i = (i << 16u) | (i >> 16u);
    i = ((i & 0x55555555u) << 1u) | ((i & 0xAAAAAAAAu) >> 1u);
    i = ((i & 0x33333333u) << 2u) | ((i & 0xCCCCCCCCu) >> 2u);
    i = ((i & 0x0F0F0F0Fu) << 4u) | ((i & 0xF0F0F0F0u) >> 4u);
    i = ((i & 0x00FF00FFu) << 8u) | ((i & 0xFF00FF00u) >> 8u);
    P.y = float(i) * 2.3283064365386963e-10; // / 0x100000000

    return P;
}


/** 
 Generate the ith 2D Hammersley point out of N on the cosine-weighted unit hemisphere
 with Z = up.
 \cite http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html */
vec3 hammersleyHemi(uint i, const uint N) {
    vec2 P = hammersleySquare(i, N);
    float phi = P.y * 2.0 * pi_;
    float cosTheta = 1.0 - P.x;
    float sinTheta = sqrt(1.0 - square(cosTheta));
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

/** 
 Generate the ith 2D Hammersley point out of N on the cosine-weighted unit hemisphere
 with Z = up.
 \cite http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html */
vec3 hammersleyCosHemi(uint i, const uint N) {
    vec3 P = hammersleyHemi(i, N);
    P.z = sqrt(P.z);
    return P;
}

mat4x4 identity4x4() {
    return mat4x4(1,0,0,0,
                  0,1,0,0,
                  0,0,1,0,
                  0,0,0,1);
}

/** Constructs a 4x4 translation matrix, assuming T * v multiplication. */
mat4x4 translate4x4(Vector3 t) {
    return mat4x4(1,0,0,0,
                  0,1,0,0,
                  0,0,1,0,
                  t,1);
}

/** Constructs a 4x4 Y->Z rotation matrix, assuming R * v multiplication. a is in radians.*/
mat4x4 pitch4x4(float a) {
    return mat4x4(1, 0, 0, 0,
                  0,cos(a),-sin(a), 0,
                  0,sin(a), cos(a), 0,
                  0, 0, 0, 1);
}

/** Constructs a 4x4 X->Y rotation matrix, assuming R * v multiplication. a is in radians.*/
mat4x4 roll4x4(float a) {
    return mat4x4(cos(a),-sin(a),0,0,
                  sin(a), cos(a),0,0,
                  0,0,1,0,
                  0,0,0,1);
}

/** Constructs a 4x4 Z->X rotation matrix, assuming R * v multiplication. a is in radians.*/
mat4x4 yaw4x4(float a) {
    return mat4x4(cos(a),0,sin(a),0,
                  0,1,0,0,
                  -sin(a),0,cos(a),0,
                  0,0,0,1);
}

#if G3D_SHADER_STAGE == G3D_FRAGMENT_SHADER
/** Computes the MIP-map level for a texture coordinate.
    \cite The OpenGL Graphics System: A Specification 4.2
    chapter 3.9.11, equation 3.21 */
float mipMapLevel(in vec2 texture_coordinate, in vec2 textureSize) {
    vec2  dx        = dFdx(texture_coordinate);
    vec2  dy        = dFdy(texture_coordinate);
    float delta_max_sqr = max(dot(dx, dx), dot(dy, dy));
  
    return 0.5 * log2(delta_max_sqr * square(maxComponent(textureSize))); // == log2(sqrt(delta_max_sqr));
}
#endif


/**
 A bicubic magnification filter, primarily useful for magnifying LOD 0
 without bilinear magnification artifacts. This just adjusts the rate
 at which we move between taps, not the number of taps.
 \cite https://www.shadertoy.com/view/4df3Dn
*/
/*vec4 textureLodSmooth(Texture tex, SamplerState sampler, vec2 uv, int lod) {
  vec2 res;
  tex.GetDimensions(lod, res.x, res.y);
	uv = uv*res + 0.5;
	vec2 iuv = floor( uv );
	vec2 fuv = fract( uv );
	uv = iuv + fuv*fuv*(3.0-2.0*fuv); // fuv*fuv*fuv*(fuv*(fuv*6.0-15.0)+10.0);;
	uv = (uv - 0.5) / res;
	return tex.SampleLevel(sampler, uv, lod);
}*/


/**
 Implementation of Sigg and Hadwiger's Fast Third-Order Texture Filtering 

 http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter20.html
 \cite http://vec3.ca/bicubic-filtering-in-fewer-taps/
 \cite http://pastebin.com/raw.php?i=YLLSBRFq
 */
/*vec4 textureLodBicubic(sampler2D tex, vec2 uv, int lod) {
    //--------------------------------------------------------------------------------------
    // Calculate the center of the texel to avoid any filtering

    float2 textureDimensions    = textureSize(tex, lod);
    float2 invTextureDimensions = 1.0 / textureDimensions;

    uv *= textureDimensions;

    float2 texelCenter   = floor(uv - 0.5) + 0.5;
    float2 fracOffset    = uv - texelCenter;
    float2 fracOffset_x2 = fracOffset * fracOffset;
    float2 fracOffset_x3 = fracOffset * fracOffset_x2;

    //--------------------------------------------------------------------------------------
    // Calculate the filter weights (B-Spline Weighting Function)

    float2 weight0 = fracOffset_x2 - 0.5 * (fracOffset_x3 + fracOffset);
    float2 weight1 = 1.5 * fracOffset_x3 - 2.5 * fracOffset_x2 + 1.0;
    float2 weight3 = 0.5 * ( fracOffset_x3 - fracOffset_x2 );
    float2 weight2 = 1.0 - weight0 - weight1 - weight3;

    //--------------------------------------------------------------------------------------
    // Calculate the texture coordinates

    float2 scalingFactor0 = weight0 + weight1;
    float2 scalingFactor1 = weight2 + weight3;

    float2 f0 = weight1 / (weight0 + weight1);
    float2 f1 = weight3 / (weight2 + weight3);

    float2 texCoord0 = texelCenter - 1.0 + f0;
    float2 texCoord1 = texelCenter + 1.0 + f1;

    texCoord0 *= invTextureDimensions;
    texCoord1 *= invTextureDimensions;

    //--------------------------------------------------------------------------------------
    // Sample the texture

    return textureLod(tex, float2(texCoord0.x, texCoord0.y), lod) * scalingFactor0.x * scalingFactor0.y +
           textureLod(tex, float2(texCoord1.x, texCoord0.y), lod) * scalingFactor1.x * scalingFactor0.y +
           textureLod(tex, float2(texCoord0.x, texCoord1.y), lod) * scalingFactor0.x * scalingFactor1.y +
           textureLod(tex, float2(texCoord1.x, texCoord1.y), lod) * scalingFactor1.x * scalingFactor1.y;
}*/

/* L1 norm */
float norm1(vec3 v) {
    return abs(v.x) + abs(v.y) + abs(v.z);
}


float norm2(vec3 v) {
    return length(v);
}


/** L^4 norm */
float norm4(vec2 v) {
    v *= v;
    v *= v;
    return pow(v.x + v.y, 0.25);
}

/** L^8 norm */
float norm8(vec2 v) {
    v *= v;
    v *= v;
    v *= v;
    return pow(v.x + v.y, 0.125);
}

float normInf(vec3 v) {
    return maxComponent(abs(v));
}

float infIfNegative(float x) { return (x >= 0.0) ? x : inf; }


struct Ray {
    Point3      origin;
    /** Unit direction of propagation */
    Vector3     direction;
};


/** */
uint extractEvenBits(uint x) {
	x = x & 0x55555555u;
	x = (x | (x >> 1)) & 0x33333333u;
	x = (x | (x >> 2)) & 0x0F0F0F0Fu;
	x = (x | (x >> 4)) & 0x00FF00FFu;
	x = (x | (x >> 8)) & 0x0000FFFFu;

	return x;
}


/** Expands a 10-bit integer into 30 bits
by inserting 2 zeros after each bit. */
uint expandBits(uint v) {
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;

	return v;
}


/** Calculates a 30-bit Morton code for the
given 3D point located within the unit cube [0,1]. */
uint interleaveBits(uvec3 inputval) {
	uint xx = expandBits(inputval.x);
	uint yy = expandBits(inputval.y);
	uint zz = expandBits(inputval.z);

	return xx + yy * uint(2) + zz * uint(4);
}


float signNotZero(in float k) {
    return (k >= 0.0) ? 1.0 : -1.0;
}


vec2 signNotZero(in vec2 v) {
    return vec2(signNotZero(v.x), signNotZero(v.y));
}


bool isFinite(float x) {
    return ! isnan(x) && (abs(x) != inf);
}



/**  Generate a spherical fibonacci point
    http://lgdv.cs.fau.de/publications/publication/Pub.2015.tech.IMMD.IMMD9.spheri/
    To generate a nearly uniform point distribution on the unit sphere of size N, do
    for (float i = 0.0; i < N; i += 1.0) {
        float3 point = sphericalFibonacci(i,N);
    }

    The points go from z = +1 down to z = -1 in a spiral. To generate samples on the +z hemisphere,
    just stop before i > N/2.

*/
Vector3 sphericalFibonacci(float i, float n) {
    float PHI = sqrt(5.0) * 0.5 + 0.5;
#   define madfrac(A, B) ((A)*(B)-floor((A)*(B)))
    float phi = 2.0 * pi_ * madfrac(i, PHI - 1);
    float cosTheta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
    float sinTheta = sqrt(saturate(1.0 - cosTheta*cosTheta));

    return Vector3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta);

#   undef madfrac
}

/**
    \param r Two uniform random numbers on [0,1]

    \return uniformly distributed random sample on unit sphere

    http://mathworld.wolfram.com/SpherePointPicking.html

    See also g3d_sphereRandom texture and sphericalFibonacci()
*/
Vector3 sphereRandom(vec2 r) {
    float cosPhi = r.x * 2.0 - 1.0;
    float sinPhi = sqrt(1 - square(cosPhi));
    float theta = r.y * 2.0 * pi_;
    return Vector3(sinPhi * cos(theta), sinPhi * cos(theta), cosPhi);
}

/**
    \param r Two uniform random numbers on [0,1]

  \return uniformly distributed random sample on unit hemisphere about +z
    See also hemisphereRandom texture.
*/
Vector3 hemisphereRandom(vec2 r) {
    Vector3 s = sphereRandom(r);
    return Vector3(s.x, s.y, abs(s.z));
}


/** Returns cos^k distributed random values distributed on the
    hemisphere about +z

    \param r Two uniform random numbers on [0,1]
    See also cosHemiRandom texture.
*/
Vector3 cosPowHemiRandom(vec2 r, const float k) {
    float cos_theta = pow(r.x, 1.0 / (k + 1.0));
    float sin_theta = sqrt(1.0f - square(cos_theta));
    float phi = 2 * pi_ * r.y;

    return Vector3( cos(phi) * sin_theta,
                    sin(phi) * sin_theta,
                    cos_theta);
}

/** Returns the index of the largest element */
int indexOfMaxComponent(vec3 v) { return (v.x > v.y) ? ((v.x > v.z) ? 0 : 2) : (v.z > v.y) ? 2 : 1; }

/** Returns the index of the largest element */
int indexOfMaxComponent(vec2 v) { return (v.x > v.y) ? 0 : 1; }

/** Returns zero */
int indexOfMaxComponent(float v) { return 0; }

#endif
