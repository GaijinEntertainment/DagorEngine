#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <3d/dag_drv3dConsts.h>


struct FfxFsr2ContextDescription;
struct FfxFsr2Context;

namespace drv3d_dx12
{
class Image;
struct Fsr2ParamsDx12
{
  Image *inColor;
  Image *inDepth;
  Image *inMotionVectors;
  Image *outColor;
  float sharpness;
  float frameTimeDelta;
  float jitterOffsetX;
  float jitterOffsetY;
  float motionVectorScaleX;
  float motionVectorScaleY;
  float cameraNear;
  float cameraFar;
  float cameraFovAngleVertical;
  uint32_t renderSizeX;
  uint32_t renderSizeY;
};

class Fsr2Wrapper
{
public:
  bool init(void *device);
  bool createContext(uint32_t target_width, uint32_t target_height, int quality);
  void evaluateFsr2(void *cmdList, const void *params);
  void getFsr2RenderingResolution(int &width, int &height) const;
  void shutdown();
  Fsr2State getFsr2State() const;

  Fsr2Wrapper();
  ~Fsr2Wrapper();

private:
  eastl::vector<unsigned char> scratchBuffer;
  eastl::unique_ptr<FfxFsr2ContextDescription> contextDescr;
  eastl::unique_ptr<FfxFsr2Context> context;
  Fsr2State state = Fsr2State::NOT_CHECKED;
  int renderWidth = 0, renderHeight = 0;
};
} // namespace drv3d_dx12
