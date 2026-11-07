// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <3d/dag_resMgr.h>
#include <3d/dag_texPackMgr2.h>
#include <drv/3d/dag_rwResource.h>
#include "levelProfilerInterface.h"

namespace levelprofiler
{

// In the pipeline order: the source with its HQ part, the limit of the quality settings, the part loaded at the collect.
enum class TextureState
{
  FULL,
  MAX,
  RES,
  COUNT
};
inline constexpr int operator*(TextureState s) { return static_cast<int>(s); }

// mips == 0: the state has no texture data (a state that is not loaded).
struct TextureStateSize
{
  int w = 0;
  int h = 0;
  int mips = 0;
  uint64_t memBytes = 0;
};

// The quality id (0 high, 1 medium, 2 low, 3 ultra low) that the graphics settings and the lists of the level select for a
// streamed texture, and why. A texture loaded before a list change keeps its quality until it is reloaded.
struct TextureQuality
{
  int selectedQuality = 0;
  int settings = 0; // The quality of the graphics settings (dgs_tex_quality) at the collect
  // The explicit list of the level that holds the texture: 2 forceTexLQ, 1 forceTexMQ, 0 none.
  int explicitList = ddsx::EXPLICIT_TEX_LIST_NONE;
  TexQL maxTql = TQL_stub;

  bool isLoweredByList() const { return explicitList != ddsx::EXPLICIT_TEX_LIST_NONE && selectedQuality > settings; }
};

struct TextureData
{
  TextureInfo info; // w, h and mipLevels are the Full state
  TextureStateSize states[*TextureState::COUNT];
  // The texture manager keeps the levels of the texture (a texture of a pack or a loose .ddsx, an array texture, a texture
  // built on demand). For any other texture, as a render target, all states are the resource as it is at the collect.
  bool isStreamed = false;
  TextureQuality quality;
  ProfilerString name;

  const TextureStateSize &getState(TextureState state) const { return states[*state]; }
};

class TextureModule : public IDataCollector, public IProfilerModule
{
public:
  TextureModule();
  virtual ~TextureModule();
  void collect() override;
  void clear() override;
  void init() override;
  void shutdown() override;
  void drawUI() override;

  const eastl::hash_map<ProfilerString, TextureData> &getTextures() const;
  // Changes each time the collected textures change.
  unsigned getDataGeneration() const { return dataGeneration; }
  // The Full state memory in MB.
  float getTextureMemorySize(const TextureData &texture_data) const;
  void resetMipLevel() { selectedMipLevel = 0; }
  void drawTextureView(const ProfilerString &texture_name);


  static const char *getFormatName(uint32_t format);
  // Width or height as the size filters key it: the size when it is a power of 2 or a multiple of 256, else -1 ("Other").
  static int getSizeFilterKey(int size);

protected:
  void shutdownImpl();
  void clearImpl();

private:
  eastl::hash_map<ProfilerString, TextureData> textures; // All textures data

  int selectedMipLevel = 0;
  bool channelMask[4] = {true, true, true, true};                      // RGBA channel toggles
  d3d::SamplerHandle pointSamplerHandle = d3d::INVALID_SAMPLER_HANDLE; // For texture preview
  unsigned dataGeneration = 0;

  void setImagePreviewMode(const ImDrawList *draw_list, const ImDrawCmd *command);
  void resetImagePreviewMode(const ImDrawList *draw_list, const ImDrawCmd *command);

  int imguiChannelMaskVariableId = -1;
  int imguiMipLevelVariableId = -1;
  int imguiUseCustomSamplerVariableId = -1;
};

} // namespace levelprofiler