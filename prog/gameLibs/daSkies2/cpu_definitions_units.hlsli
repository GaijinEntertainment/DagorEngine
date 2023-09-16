//-V::707 Giving short names to global variables

#define SamplerTexture2DFromName(a) float4(0,0,0,0)
#define SamplerTexture3DFromName(a) float4(0,0,0,0)
inline float4 sample_texture(const SamplerTexture2D &, const float2 &){return float4(0,0,0,0);}
inline float4 sample_texture(const SamplerTexture3D &, const float3 &){return float4(0,0,0,0);}
DimensionlessSpectrum DimensionlessSpectrumFromTexture(float4 a) {return float3(a.x,a.y, a.z);}
AbstractSpectrum AbstractSpectrumFromTexture(float4 a) {return float3(a.x,a.y, a.z);}
IrradianceSpectrum IrradianceSpectrumFromTexture(float4 a) {return float3(a.x,a.y, a.z);}

float mod(float a, float b) {return fmodf(a,b);}

static const Length meter = 1.0;
static const Wavelength nm = 1.0;
static const Angle rad = 1.0;
static const SolidAngle sr = 1.0;
static const Power watt = 1.0;
static const LuminousPower lm = 1.0;

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
