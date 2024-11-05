#ifndef SHADER_FAST_TRIG_INC_FX
#define SHADER_FAST_TRIG_INC_FX
//
// Trigonometric functions
//
#define fsl_PI 3.1415926535897932384626433f
#define fsl_HALF_PI (0.5f * 3.1415926535897932384626433f)

// Eberly's polynomial degree 1 - respect bounds
// 4 VGPR, 12 ALU Full Rate
// max absolute error 9.0x10^-3
// input [-1, 1] and output [0, PI]
float acosFastUnsafe(float inX)
{
    float x = abs(inX);
    float res = -0.156583f * x + fsl_HALF_PI;
    res *= sqrt(1.0f - x);
    return (inX >= 0) ? res : fsl_PI - res;
}

//should be same amount of instructions on all reasonable HW
float acosFast(float inX)
{
    float x = abs(inX);
    float res = -0.156583f * x + fsl_HALF_PI;
    res *= sqrt(saturate(1.0f - x));
    return (inX >= 0) ? res : fsl_PI - res;
}
// 4th order polynomial approximation
// 4 VGRP, 16 ALU Full Rate
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
float acosFast4Unsafe(float inX)
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

//should be same amount of instructions on all reasonable HW
float acosFast4(float inX)
{
	float x1 = abs(inX);
	float x2 = x1 * x1;
	float x3 = x2 * x1;
	float s;

	s = -0.2121144f * x1 + 1.5707288f;
	s = 0.0742610f * x2 + s;
	s = -0.0187293f * x3 + s;
	s = sqrt(saturate(1.0f - x1)) * s;

	// acos function mirroring
	// check per platform if compiles to a selector - no branch neeeded
	return inX >= 0.0f ? s : fsl_PI - s;
}

#if SHADER_COMPILER_FP16_ENABLED
    half acosFast4(half inX)
    {
        half x1 = abs(inX);
        half x2 = x1 * x1;
        half x3 = x2 * x1;
        half s;

        s = -0.2121144h * x1 + 1.5707288h;
        s = 0.0742610h * x2 + s;
        s = -0.0187293h * x3 + s;
        s = sqrt(saturate(1.0h - x1)) * s;

        // acos function mirroring
        // check per platform if compiles to a selector - no branch neeeded
        return inX >= 0.0h ? s : half(fsl_PI) - s;
    }
#endif

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
// 4 VGRP, 7 ALU Full Rate
// 7 * 10^-5 radians precision
// Reference : Efficient approximations for the arctangent function, Rajan, S. Sichun Wang Inkol, R. Joyal, A., May 2006
float atanFast4(float inX)
{
	float  x = inX;
	return x*(-0.1784f * abs(x) - 0.0663f * x * x + 1.0301f);
}
float atan2Fast4(float y, float x)
{
    float poly = atanFast4(min( abs(x), abs(y) ) / max( abs(x), abs(y) ));
    poly = abs(y) < abs(x) ? poly : fsl_HALF_PI - poly;
    poly = x < 0 ? fsl_PI - poly : poly;
    poly = y < 0 ? -poly : poly;
    return poly;
}
// https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
// max absolute error 1.3x10^-3
// Eberly's odd polynomial degree 5 - respect bounds
// 4 VGPR, 14 FR (10 FR, 1 QR), 2 scalar
// input [0, infinity] and output [0, PI/2]
float atanFastPos(float x)
{
    float t0 = (x < 1.0f) ? x : 1.0f / x;
    float t1 = t0 * t0;
    float poly = 0.0872929f;
    poly = -0.301895f + poly * t1;
    poly = 1.0f + poly * t1;
    poly = poly * t0;
    return (x < 1.0f) ? poly : fsl_HALF_PI - poly;
}

// 4 VGPR, 16 FR (12 FR, 1 QR), 2 scalar
// input [-infinity, infinity] and output [-PI/2, PI/2]
float atanFast(float x)
{
    float t0 = atanFastPos(abs(x));
    return (x < 0.0f) ? -t0: t0;
}

float atan2Fast(float y, float x)
{
    float t0 = min( abs(x), abs(y) ) / max( abs(x), abs(y) );
    float t1 = t0 * t0;
    // exactly same poly
    float poly = 0.0872929f;
    poly = -0.301895 + poly * t1;
    poly = 1.0 + poly * t1;
    poly = poly * t0;

    poly = abs(y) < abs(x) ? poly : fsl_HALF_PI - poly;
    //now reflect to other 3 qudrants
    poly = x < 0 ? fsl_PI - poly : poly;
    poly = y < 0 ? -poly : poly;

    return poly;
}

#undef fsl_PI
#undef fsl_HALF_PI

#endif // SHADER_FAST_TRIG_INC_FX