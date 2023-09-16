#ifndef ATMOSPHERE_DEFINITIONS_UNITS
#define ATMOSPHERE_DEFINITIONS_UNITS 1
struct SamplerTexture2D
{
  Texture2D<float4> tex;
  SamplerState tex_smp;
};
struct SamplerTexture3D
{
  Texture3D<float4> tex;
  SamplerState tex_smp;
};
SamplerTexture2D from_texture2d(Texture2D<float4> tex, SamplerState tex_smp){ SamplerTexture2D r;r.tex=tex;r.tex_smp = tex_smp;return r;}
SamplerTexture3D from_texture3d(Texture3D<float4> tex, SamplerState tex_smp){ SamplerTexture3D r;r.tex=tex;r.tex_smp = tex_smp;return r;}

#define SamplerTexture2DFromName(a) from_texture2d(a, a##_samplerstate)
#define SamplerTexture3DFromName(a) from_texture3d(a, a##_samplerstate)
#define TransmittanceTexture SamplerTexture2D
#define AbstractScatteringTexture SamplerTexture3D
#define PreparedSkiesScatteringTexture SamplerTexture2D
#define PreparedScatteringTexture SamplerTexture3D
#define ReducedScatteringTexture SamplerTexture3D
#define ScatteringTexture SamplerTexture3D
#define ScatteringDensityTexture SamplerTexture3D
#define IrradianceTexture SamplerTexture2D
#define MultipleScatteringTexture SamplerTexture2D

#define IN(t) t
#define OUT(t) out t
#define INOUT(t) inout t
#define INLINE
#define G_UNUSED(a)
#define assert(a)
#define vec2 float2
#define vec4 float4
#define vec3 float3
#define TEMPLATE(a)
#define TEMPLATE_ARGUMENT(a)
DimensionlessSpectrum DimensionlessSpectrumFromTexture(float4 a) {return a.xyz;}
AbstractSpectrum AbstractSpectrumFromTexture(float4 a) {return a.xyz;}
IrradianceSpectrum IrradianceSpectrumFromTexture(float4 a) {return a.xyz;}

float mod(float a, float b) {return fmod(a,b);}

float4 sample_texture(SamplerTexture2D ts, float2 uv)
{
  return ts.tex.SampleLevel(ts.tex_smp, uv, 0);
}

float4 sample_texture(SamplerTexture3D ts, float3 uv)
{
  return ts.tex.SampleLevel(ts.tex_smp, uv, 0);
}

/*
<h3>Physical units</h3>

<p>We can then define the units for our six base physical quantities:
meter (m), nanometer (nm), radian (rad), steradian (sr), watt (watt) and lumen
(lm):
*/

static const Length meter = 1.0;
static const Wavelength nm = 1.0;
static const Angle rad = 1.0;
static const SolidAngle sr = 1.0;
static const Power watt = 1.0;
static const LuminousPower lm = 1.0;

/*
<p>From which we can derive the units for some derived physical quantities,
as well as some derived units (kilometer km, kilocandela kcd, degree deg):
*/

static const Length km = 1000.0 * meter;
static const Area m2 = meter * meter;
static const Volume m3 = meter * meter * meter;
static const Angle pi = PI * rad;
static const Angle deg = pi / 180.0;
static const Irradiance watt_per_square_meter = watt / m2;
static const Radiance watt_per_square_meter_per_sr = watt / (m2 * sr);
static const SpectralIrradiance watt_per_square_meter_per_nm = watt / (m2 * nm);
static const SpectralRadiance watt_per_square_meter_per_sr_per_nm =
    watt / (m2 * sr * nm);
static const SpectralRadianceDensity watt_per_cubic_meter_per_sr_per_nm =
    watt / (m3 * sr * nm);
static const LuminousIntensity cd = lm / sr;
static const LuminousIntensity kcd = 1000.0 * cd;
static const Luminance cd_per_square_meter = cd / m2;
static const Luminance kcd_per_square_meter = kcd / m2;

#endif