
hlsl(ps) {
#ifndef __Random__
#define __Random__

// @param xy should be a integer position (e.g. pixel position on the screen), repeats each 128x128 pixels
// similar to a texture lookup but is only ALU
float PseudoRandom(float2 xy)
{
	float2 pos = frac(xy / 128.0f) * 128.0f + float2(-64.340622f, -72.465622f);
	
	// found by experimentation
	return frac(dot(pos.xyx * pos.xyy, float3(20.390625f, 60.703125f, 2.4281209f)));
}


/**
 * Find good arbitrary axis vectors to represent U and V axes of a plane,
 * given just the normal. Ported from UnMath.h
 */
void FindBestAxisVectors(float3 In, out float3 Axis1, out float3 Axis2 )
{
	const float3 N = abs(In);

	// Find best basis vectors.
	if( N.z > N.x && N.z > N.y )
	{
		Axis1 = float3(1, 0, 0);
	}
	else
	{
		Axis1 = float3(0, 0, 1);
	}

	Axis1 = normalize(Axis1 - In * dot(Axis1, In));
	Axis2 = cross(Axis1, In);
}

// References for noise:
//
// Improved Perlin noise
//   http://mrl.nyu.edu/~perlin/noise/
//   http://http.developer.nvidia.com/GPUGems/gpugems_ch05.html
// Modified Noise for Evaluation on Graphics Hardware
//   http://www.csee.umbc.edu/~olano/papers/mNoise.pdf
// Perlin Noise
//   http://mrl.nyu.edu/~perlin/doc/oscar.html
// Fast Gradient Noise
//   http://prettyprocs.wordpress.com/2012/10/20/fast-perlin-noise


// -------- ALU based method ---------

/*
 * Pseudo random number generator, based on "TEA, a tiny Encrytion Algorithm"
 * http://citeseer.ist.psu.edu/viewdoc/download?doi=10.1.1.45.281&rep=rep1&type=pdf
 * @param v - old seed (full 32bit range)
 * @param IterationCount - >=1, bigger numbers cost more performance but improve quality
 * @return new seed
 */
uint2 ScrambleTEA(uint2 v, uint IterationCount = 3)
{
	// Start with some random data (numbers can be arbitrary but those have been used by others and seem to work well)
	uint k[4] ={ 0xA341316Cu , 0xC8013EA4u , 0xAD90777Du , 0x7E95761Eu };
	
	uint y = v[0];
	uint z = v[1];
	uint sum = 0;
	
	UNROLL for(uint i = 0; i < IterationCount; ++i)
	{
		sum += 0x9e3779b9;
		y += (z << 4u) + k[0] ^ z + sum ^ (z >> 5u) + k[1];
		z += (y << 4u) + k[2] ^ y + sum ^ (y >> 5u) + k[3];
	}

	return uint2(y, z);
}

// Computes a pseudo random number for a given integer 2D position
// @param v - old seed (full 32bit range)
// @return random number in the range -1 .. 1
float ComputeRandomFrom2DPosition(uint2 v)
{
	return (ScrambleTEA(v).x & 0xffff ) / (float)(0xffff) * 2 - 1;
}

// Computes a pseudo random number for a given integer 2D position
// @param v - old seed (full 32bit range)
// @return random number in the range -1 .. 1
float ComputeRandomFrom3DPosition(int3 v) 
{
	// numbers found by experimentation
	return ComputeRandomFrom2DPosition(v.xy ^ (uint2(0x123456, 0x23446) * v.zx) );
}

// Evaluate polynomial to get smooth transitions for Perlin noise
// only needed by Perlin functions in this file
// scalar(per component): 2 add, 5 mul
float4 PerlinRamp(float4 t)
{
	return t * t * t * (t * (t * 6 - 15) + 10); 
}

// bilinear filtered 2D noise, can be optimized
// @param v >0
// @return random number in the range -1 .. 1
float2 PerlinNoise2D_ALU(float2 fv)
{
	// floor is needed to avoid -1..1 getting the same value (int cast always rounds towards 0)
	int2 iv = int2(floor(fv));

	float2 a = ComputeRandomFrom2DPosition(iv + int2(0, 0));
	float2 b = ComputeRandomFrom2DPosition(iv + int2(1, 0));
	float2 c = ComputeRandomFrom2DPosition(iv + int2(0, 1));
	float2 d = ComputeRandomFrom2DPosition(iv + int2(1, 1));
	
	float2 Weights = PerlinRamp(float4(frac(fv), 0, 0)).xy;
	
	float2 e = lerp(a, b, Weights.x);
	float2 f = lerp(c, d, Weights.x);

	return lerp(e, f, Weights.y);
}

// bilinear filtered 2D noise, can be optimized
// @param v >0
// @return random number in the range -1 .. 1
float PerlinNoise3D_ALU(float3 fv)
{
	// floor is needed to avoid -1..1 getting the same value (int cast always rounds towards 0)
	int3 iv = int3(floor(fv));
	
	float2 a = ComputeRandomFrom3DPosition(iv + int3(0, 0, 0));
	float2 b = ComputeRandomFrom3DPosition(iv + int3(1, 0, 0));
	float2 c = ComputeRandomFrom3DPosition(iv + int3(0, 1, 0));
	float2 d = ComputeRandomFrom3DPosition(iv + int3(1, 1, 0));
	float2 e = ComputeRandomFrom3DPosition(iv + int3(0, 0, 1));
	float2 f = ComputeRandomFrom3DPosition(iv + int3(1, 0, 1));
	float2 g = ComputeRandomFrom3DPosition(iv + int3(0, 1, 1));
	float2 h = ComputeRandomFrom3DPosition(iv + int3(1, 1, 1));
	
	float3 Weights = PerlinRamp(frac(float4(fv, 0))).xyz;
	
	float2 i = lerp(lerp(a, b, Weights.x), lerp(c, d, Weights.x), Weights.y);
	float2 j = lerp(lerp(e, f, Weights.x), lerp(g, h, Weights.x), Weights.y);

	return lerp(i, j, Weights.z).x;
}

// -------- TEX based method ---------

// -------- Simplex method (faster in higher dimensions because less samples are used, uses gradient noise for quality) ---------
// <Dimensions>D:<Normal>/<Simplex> 1D:2, 2D:4/3, 3D:8/4, 4D:16/5 

// Computed weights and sample positions for simplex interpolation
// @return float3(a,b,c) Barycentric coordianate defined as Filtered = Tex(PosA) * a + Tex(PosB) * b + Tex(PosC) * c
float3 ComputeSimplexWeights2D(float2 OrthogonalPos, out float2 PosA, out float2 PosB, out float2 PosC)
{
	float2 OrthogonalPosFloor = floor(OrthogonalPos); 
	PosA = OrthogonalPosFloor;
	PosB = PosA + float2(1, 1); 

	float2 LocalPos = OrthogonalPos - OrthogonalPosFloor;

	PosC = PosA + ((LocalPos.x > LocalPos.y) ? float2(1,0) : float2(0,1));

	float b = min(LocalPos.x, LocalPos.y);
	float c = abs(LocalPos.y - LocalPos.x);
	float a = 1.0f - b - c;

	return float3(a, b, c);
}

// Computed weights and sample positions for simplex interpolation
// @return float4(a,b,c, d) Barycentric coordinate defined as Filtered = Tex(PosA) * a + Tex(PosB) * b + Tex(PosC) * c + Tex(PosD) * d
float4 ComputeSimplexWeights3D(float3 OrthogonalPos, out float3 PosA, out float3 PosB, out float3 PosC, out float3 PosD)
{
	float3 OrthogonalPosFloor = floor(OrthogonalPos);

	PosA = OrthogonalPosFloor;
	PosB = PosA + float3(1, 1, 1);

	OrthogonalPos -= OrthogonalPosFloor;

	float Largest = max(OrthogonalPos.x, max(OrthogonalPos.y, OrthogonalPos.z));
	float Smallest = min(OrthogonalPos.x, min(OrthogonalPos.y, OrthogonalPos.z));

	PosC = PosA + float3(Largest == OrthogonalPos.x, Largest == OrthogonalPos.y, Largest == OrthogonalPos.z);
	PosD = PosA + float3(Smallest != OrthogonalPos.x, Smallest != OrthogonalPos.y, Smallest != OrthogonalPos.z);

	float4 ret;

	float RG = OrthogonalPos.x - OrthogonalPos.y;
	float RB = OrthogonalPos.x - OrthogonalPos.z;
	float GB = OrthogonalPos.y - OrthogonalPos.z;

	ret.b = 
		  min(max(0, RG), max(0, RB))		// X
		+ min(max(0, -RG), max(0, GB))		// Y
		+ min(max(0, -RB), max(0, -GB));	// Z
	
	ret.a = 
		  min(max(0, -RG), max(0, -RB))		// X
		+ min(max(0, RG), max(0, -GB))		// Y
		+ min(max(0, RB), max(0, GB));		// Z

	ret.g = Smallest;
	ret.r = 1.0f - ret.g - ret.b - ret.a;

	return ret;
}

float2 SkewSimplex(float2 In)
{
	return In + dot(In, (sqrt(3.0f) - 1.0f) * 0.5f );
}
float2 UnSkewSimplex(float2 In)
{
	return In - dot(In, (3.0f - sqrt(3.0f)) / 6.0f );
}
float3 SkewSimplex(float3 In)
{
	return In + dot(In, 1.0 / 3.0f );
}
float3 UnSkewSimplex(float3 In)
{
	return In - dot(In, 1.0 / 6.0f );
}

#endif
}
