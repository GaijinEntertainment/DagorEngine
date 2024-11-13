
#define NOP

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
