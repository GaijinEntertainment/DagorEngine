
#define RWBuffer         RWStructuredBuffer

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

#define WAVE_INTRINSICS 1
//#define WaveReadLaneAt    ReadLane
#define WaveReadFirstLane WaveReadLaneFirst
//#define WaveGetLaneIndex
#define WaveIsHelperLane  WaveIsFirstLane
#define WaveAllBitOr      WaveActiveBitOr
#define WaveAllBitAnd     WaveActiveBitAnd
#define WaveAllSum_F32    WaveActiveSum
#define WaveAllMax_F32    WaveActiveMax
#define WaveAllMax_I32    WaveActiveMax
#define WaveAllMax_U32    WaveActiveMax
#define WaveAllMin_F32    WaveActiveMin
#define WaveAllMin_I32    WaveActiveMin
#define WaveAllMin_U32    WaveActiveMin
