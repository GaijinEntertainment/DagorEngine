
#define NOP
#define _HARDWARE_DX11 1

struct TextureSampler
{
  Texture2D tex;
  SamplerState smp;
};

struct TextureSamplerCube
{
  TextureCube tex;
  SamplerState smp;
};

struct TextureArraySampler
{
  Texture2DArray tex;
  SamplerState smp;
};

struct Texture3DSampler
{
  Texture3D tex;
  SamplerState smp;
};

struct TextureCubeArraySampler
{
  TextureCubeArray tex;
  SamplerState smp;
};