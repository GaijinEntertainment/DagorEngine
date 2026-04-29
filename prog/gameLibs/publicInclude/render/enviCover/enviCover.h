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

enum class NodeBasedShaderQuality : uint32_t;

class EnviCover
{
public:
  static constexpr const int ENVI_COVER_MAX_RW_TARGETS = 5;
  enum class EnviCoverUseType
  {
    STANDALONE_FGNODE,
    COMBINED_FGNODE,
    NBS,
    NBS_PACKED_NORMS,
    NBS_COMBINED,
    NBS_COMBINED_PACKED_NORMS,
    UNDEFINED
  };
  void initShader(const String &root_graph);
  bool updateShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors);
  void render(int x, int y, const eastl::array<BaseTexture *, ENVI_COVER_MAX_RW_TARGETS> &&gbufBaseTextures);
  void initRender(EnviCoverUseType envi_cover_type);
  EnviCover();
  ~EnviCover();

  bool getValidNBS() { return validNBS; }
  void setValidNBS(bool valid_NBS) { validNBS = valid_NBS; }
  void setNbsQuality(NodeBasedShaderQuality nbs_quality);
  void afterReset();
  void beforeReset();
  void closeShaders();
  void enableOptionalShader(const String &shader_name, bool enable);

private:
  EnviCoverUseType usage = EnviCover::EnviCoverUseType::UNDEFINED;
  bool validNBS = false;
  eastl::unique_ptr<NodeBasedShader> enviCoverNBS;
  eastl::unique_ptr<NodeBasedShader> enviCoverCombinedNBS;
  eastl::unique_ptr<NodeBasedShader> enviCoverWithPackedNormsNBS;
  eastl::unique_ptr<NodeBasedShader> enviCoverCombinedWithPackedNormsNBS;
  eastl::unique_ptr<ComputeShaderElement> enviCoverStandaloneCS;
  eastl::unique_ptr<ComputeShaderElement> enviCoverCombinedCS;
};
