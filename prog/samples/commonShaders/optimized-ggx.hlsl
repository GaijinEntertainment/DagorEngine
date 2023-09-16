/*
optimized-ggx.hlsl
This file describes several optimizations you can make when creating a GGX shader.

AUTHOR: John Hable

LICENSE:

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

float G1V(float dotNV, float k)
{
	return 1.0f/(dotNV*(1.0f-k)+k);
}

float LightingFuncGGX_REF(float3 N, float3 V, float3 L, float linear_roughness, float F0)
{
	float alpha = linear_roughness*linear_roughness;

	float3 H = normalize(V+L);

	float dotNL = saturate(dot(N,L));
	float dotNV = saturate(dot(N,V));
	float dotNH = saturate(dot(N,H));
	float dotLH = saturate(dot(L,H));

	float F, D, vis;

	// D
	float alphaSqr = alpha*alpha;
	float pi = 3.14159f;
	float denom = dotNH * dotNH *(alphaSqr-1.0) + 1.0f;
	D = alphaSqr/(pi * denom * denom);

	// F
	float dotLH5 = pow(1.0f-dotLH,5);
	F = F0 + (1.0-F0)*(dotLH5);

	// V
	float k = alpha/2.0f;
	vis = G1V(dotNL,k)*G1V(dotNV,k);

	float specular = dotNL * D * F * vis;
	return specular;
}

float LightingFuncGGX_OPT1(float3 N, float3 V, float3 L, float linear_roughness, float F0)
{
	float alpha = linear_roughness*linear_roughness;

	float3 H = normalize(V+L);

	float dotNL = saturate(dot(N,L));
	float dotLH = saturate(dot(L,H));
	float dotNH = saturate(dot(N,H));

	float F, D, vis;

	// D
	float alphaSqr = alpha*alpha;
	float pi = 3.14159f;
	float denom = dotNH * dotNH *(alphaSqr-1.0) + 1.0f;
	D = alphaSqr/(pi * denom * denom);

	// F
	float dotLH5 = pow(1.0f-dotLH,5);
	F = F0 + (1.0-F0)*(dotLH5);

	// V
	float k = alpha/2.0f;
	vis = G1V(dotLH,k)*G1V(dotLH,k);

	float specular = dotNL * D * F * vis;
	return specular;
}


float LightingFuncGGX_OPT2(float3 N, float3 V, float3 L, float linear_roughness, float F0)
{
	float alpha = linear_roughness*linear_roughness;

	float3 H = normalize(V+L);

	float dotNL = saturate(dot(N,L));

	float dotLH = saturate(dot(L,H));
	float dotNH = saturate(dot(N,H));

	float F, D, vis;

	// D
	float alphaSqr = alpha*alpha;
	float pi = 3.14159f;
	float denom = dotNH * dotNH *(alphaSqr-1.0) + 1.0f;
	D = alphaSqr/(pi * denom * denom);

	// F
	float dotLH5 = pow(1.0f-dotLH,5);
	F = F0 + (1.0-F0)*(dotLH5);

	// V
	float k = alpha/2.0f;
	float k2 = k*k;
	float invK2 = 1.0f-k2;
	vis = rcp(dotLH*dotLH*invK2 + k2);

	float specular = dotNL * D * F * vis;
	return specular;
}

float2 LightingFuncGGX_FV(float dotLH, float linear_roughness)
{
	float alpha = linear_roughness*linear_roughness;

	// F
	float F_a, F_b;
	float dotLH5 = pow(1.0f-dotLH,5);
	F_a = 1.0f;
	F_b = dotLH5;

	// V
	float vis;
	float k = alpha/2.0f;
	float k2 = k*k;
	float invK2 = 1.0f-k2;
	vis = rcp(dotLH*dotLH*invK2 + k2);

	return float2(F_a*vis,F_b*vis);
}

float LightingFuncGGX_D(float dotNH, float linear_roughness)
{
	float alpha = linear_roughness*linear_roughness;
	float alphaSqr = alpha*alpha;
	float pi = 3.14159f;
	float denom = dotNH * dotNH *(alphaSqr-1.0) + 1.0f;

	float D = alphaSqr/(pi * denom * denom);
	return D;
}

float LightingFuncGGX_OPT3(float3 N, float3 V, float3 L, float linear_roughness, float F0)
{
	float3 H = normalize(V+L);

	float dotNL = saturate(dot(N,L));
	float dotLH = saturate(dot(L,H));
	float dotNH = saturate(dot(N,H));

	float D = LightingFuncGGX_D(dotNH,linear_roughness);
	float2 FV_helper = LightingFuncGGX_FV(dotLH,linear_roughness);
	float FV = F0*FV_helper.x + (1.0f-F0)*FV_helper.y;
	float specular = dotNL * D * FV;

	return specular;
}

float Pow4(float x)
{
	return x*x*x*x;
}

float LightingFuncGGX_OPT4(float3 N, float3 V, float3 L, float linear_roughness, float F0)
{
	float3 H = normalize(V+L);

	float dotNL = saturate(dot(N,L));
	float dotLH = saturate(dot(L,H));
	float dotNH = saturate(dot(N,H));

	float D = g_txGgxDFV.Sample(g_samLinearClamp,float2(Pow4(dotNH),linear_roughness)).x;
	float2 FV_helper =
		g_txGgxDFV.Sample(g_samLinearClamp,float2(dotLH,linear_roughness)).yz;

	float FV = F0*FV_helper.x + (1.0f-F0)*FV_helper.y;
	float specular = dotNL * D * FV;

	return specular;
}

// This version includes Stephen Hill's optimization
float LightingFuncGGX_OPT5(float3 N, float3 V, float3 L, float linear_roughness, float F0)
{
	float3 H = normalize(V+L);

	float dotNL = saturate(dot(N,L));
	float dotLH = saturate(dot(L,H));
	float dotNH = saturate(dot(N,H));

	float D = g_txGgxDFV.Sample(g_samLinearClamp,float2(Pow4(dotNH),linear_roughness)).x;
	float2 FV_helper =
		g_txGgxDFV.Sample(g_samLinearClamp,float2(dotLH,linear_roughness)).yz;

	float FV = F0*FV_helper.x + FV_helper.y;
	float specular = dotNL * D * FV;

	return specular;
}

