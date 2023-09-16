#ifndef SHADER_FAST_MATH_INC_FX
#define SHADER_FAST_MATH_INC_FX
//
// Trigonometric functions
//
#define fsl_PI 3.1415926535897932384626433f
#define fsl_HALF_PI (0.5f * 3.1415926535897932384626433f)

// 4th order polynomial approximation
// 4 VGRP, 16 ALU Full Rate
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
float acosFast4(float inX)
{
	float x1 = abs(inX);
	float x2 = x1 * x1;
	float x3 = x2 * x1;
	float s;

	s = -0.2121144f * x1 + 1.5707288f;
	s = 0.0742610f * x2 + s;
	s = -0.0187293f * x3 + s;
	s = sqrt(1.0f - x1) * s;

	// acos function mirroring
	// check per platform if compiles to a selector - no branch neeeded
	return inX >= 0.0f ? s : fsl_PI - s;
}

// 4th order polynomial approximation
// 4 VGRP, 16 ALU Full Rate
// 7 * 10^-5 radians precision 
float asinFast4(float inX)
{
	float x = inX;

	// asin is offset of acos
	return fsl_HALF_PI - acosFast4(x);
}

// 4th order hyperbolical approximation
// 4 VGRP, 12 ALU Full Rate
// 7 * 10^-5 radians precision 
// Reference : Efficient approximations for the arctangent function, Rajan, S. Sichun Wang Inkol, R. Joyal, A., May 2006
float atanFast4(float inX)
{
	float  x = inX;
	return x*(-0.1784f * abs(x) - 0.0663f * x * x + 1.0301f);
}

#undef fsl_PI
#undef fsl_HALF_PI

#endif