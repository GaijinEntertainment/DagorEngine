
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

#if WAVE_INTRINSICS
  //#define WaveReadLaneAt    ReadLane
  #define WaveReadFirstLane WaveReadLaneFirst
  //#define WaveGetLaneIndex
  #define WaveAllBitOr      WaveActiveBitOr
  #define WaveAllBitAnd     WaveActiveBitAnd
  #define WaveAllSum_F32    WaveActiveSum
  #define WaveAllMax_F32    WaveActiveMax
  #define WaveAllMax_I32    WaveActiveMax
  #define WaveAllMax_U32    WaveActiveMax
  #define WaveAllMin_F32    WaveActiveMin
  #define WaveAllMin_I32    WaveActiveMin
  #define WaveAllMin_U32    WaveActiveMin
#endif

#define ACCESS_FORMAT(fmt) [[vk::image_format(#fmt)]]
