
#define WAVE_INTRINSICS 1
#define WaveReadLaneAt __XB_ReadLane
#define WaveReadFirstLane(a) __XB_MakeUniform((a))
#define WaveIsHelperLane  WaveIsFirstLane
#define WaveGetLaneIndex __XB_GetLaneID
#define WaveAllBitOr __XB_WaveOR
#define WaveAllBitAnd __XB_WaveAND
#define WaveActiveAllTrue(val) WaveAllBitAnd(uint(val))
#define WaveAllSum_F32 __XB_WaveAdd_F32
#define WaveAllMax_F32 __XB_WaveMax_F32
#define WaveAllMax_I32 __XB_WaveMax_I32
#define WaveAllMax_U32 __XB_WaveMax_U32
#define WaveAllMin_F32 __XB_WaveMin_F32
#define WaveAllMin_I32 __XB_WaveMin_I32
#define WaveAllMin_U32 __XB_WaveMin_U32

#define NOP
#define _HARDWARE_XBOX 1
#define _HARDWARE_SCARLETT 1

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