#define Length float
#define Wavelength float
#define Angle float
#define SolidAngle float
#define Power float
#define LuminousPower float

#define Number float
#define InverseLength float
#define Area float
#define Volume float
#define NumberDensity float
#define Irradiance float
#define Radiance float
#define SpectralPower float
#define SpectralIrradiance float
#define SpectralRadiance float
#define SpectralRadianceDensity float
#define ScatteringCoefficient float
#define InverseSolidAngle float
#define LuminousIntensity float
#define Luminance float
#define Illuminance float

#define AbstractSpectrum float3
#define DimensionlessSpectrum float3
#define PowerSpectrum float3
#define IrradianceSpectrum float3
#define RadianceSpectrum float3
#define RadianceDensitySpectrum float3
#define ScatteringSpectrum float3

#define Position float3
#define Direction float3
#define Luminance3 float3
#define Illuminance3 float3


struct SamplerTexture2D{};
struct SamplerTexture3D{};
struct IrradianceTexture;
//#define TransmittanceTexture SamplerTexture2D
#define AbstractScatteringTexture SamplerTexture3D
#define PreparedSkiesScatteringTexture SamplerTexture2D
#define PreparedScatteringTexture SamplerTexture3D
#define ReducedScatteringTexture SamplerTexture3D
#define ScatteringTexture SamplerTexture3D
#define ScatteringDensityTexture SamplerTexture3D
#define MultipleScatteringTexture SamplerTexture2D

#define IN(t) const t &
#define OUT(t) t&
#define INOUT(t) t&
#define assert(a) G_ASSERT(a)
#define INLINE __forceinline
#define vec2 float2
#define vec4 float4
#define vec3 float3
#define TEMPLATE(a)
#define TEMPLATE_ARGUMENT(a)

#define SKIES_SEPARATE_SINGLE_SCATTERING 1
#define COMBINED_SCATTERING_TEXTURES 1
