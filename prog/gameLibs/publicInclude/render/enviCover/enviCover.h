//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>

class NodeBasedShader;
class ComputeShaderElement;
class BaseTexture;
class DataBlock;
class String;

class EnviCover
{
public:
  static constexpr const int ENVI_COVER_RW_TARGETS = 4;
  enum class EnviCoverUseType
  {
    STANDALONE_FGNODE,
    COMBINED_FGNODE,
    NBS,
    NBS_COMBINED,
    UNDEFINED
  };
  void initShader(const String &root_graph);
  bool updateShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors);
  void render(int x, int y, const eastl::array<BaseTexture *, ENVI_COVER_RW_TARGETS> &gbufBaseTextures);
  void initRender(EnviCoverUseType usage_type);
  EnviCover();
  ~EnviCover();

  bool getValidNBS() { return validNBS; }
  void setValidNBS(bool valid_NBS) { validNBS = valid_NBS; }
  void afterReset();
  void beforeReset();
  void closeShaders();
  void enableOptionalShader(const String &shader_name, bool enable);

private:
  EnviCoverUseType usage = EnviCover::EnviCoverUseType::UNDEFINED;
  bool validNBS = false;
  eastl::unique_ptr<NodeBasedShader> enviCoverNBS;
  eastl::unique_ptr<NodeBasedShader> enviCoverCombinedNBS;
  eastl::unique_ptr<ComputeShaderElement> enviCoverStandaloneCS;
  eastl::unique_ptr<ComputeShaderElement> enviCoverCombinedCS;
};