// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_color.h>
#include <util/dag_string.h>
#include <3d/dag_texMgr.h>

#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

class BaseTexture;
class DataBlock;
struct GraphData;

// Plugin-supplied graph-to-BLK compiler. Installed into the texgen service via
// `setGraphCompiler`. The service calls `compile()` on its worker thread when the
// graph has been marked dirty via `markGraphDirty()`. The implementation must:
//   1. Take its own lock around source-of-truth graph state.
//   2. Commit the produced BLKs into `graphData->mainGraphBlk` /
//      `graphData->shaderListBlk` under THAT SAME lock, before returning.
// The service does NOT hold any plugin lock while invoking compile() and does
// not write to graphData's BLKs itself -- otherwise the worker's stateLock and
// the plugin's mutation lock would both guard the same memory under different
// names, which is not a guard at all.
//
// Returns false on compile failure (e.g. iteration-limit hit); in that case the
// implementation must leave `graphData->mainGraphBlk` / `shaderListBlk`
// untouched and the service treats the dirty generation as having advanced (no
// retry on the same input) to avoid spinning on a deterministically-failing
// compile.
struct IGraphCompiler
{
  virtual ~IGraphCompiler() = default;
  virtual bool compile() = 0;
};

struct SelectedTextureState
{
  BaseTexture *texture = nullptr;
  eastl::string name;
  int width = 0;
  int height = 0;
  int channels = 0;

  eastl::vector<Color4> histogram01;
  eastl::vector<Color4> histogramFull;
  float minHistValue = 0.0f;
  float maxHistValue = 0.0f;
};

struct IGraphTexGenService
{
  static constexpr unsigned HUID = 0x7A3E91D2u; // ITexGenService

  virtual void initTexGen(const char *default_shader_path, const char *default_texgen_path) = 0;
  virtual void shutdownTexGen() = 0;
  virtual void tickTexGeneration() = 0;

  // Plug in the graph data the plugin has loaded. Pass nullptr to put the service in idle state.
  // The service holds onto the pointer (no copy); the caller (plugin) owns the lifetime. The
  // service also mutates `gd->mainGraphBlk` for setPreviewFinal, so it must be the same instance
  // for the lifetime of a loaded graph. Calling with a new pointer (or after the BLK content
  // changes) triggers regeneration.
  virtual void setGraphData(GraphData *gd) = 0;

  // Install / remove the plugin's graph compiler. The service does not take
  // ownership; the plugin must call setGraphCompiler(nullptr) before destroying
  // the implementation. Safe to set before or after setGraphData; the worker
  // only invokes compile() when both a compiler is installed and the dirty flag
  // is set.
  virtual void setGraphCompiler(IGraphCompiler *compiler) = 0;

  // Mark the graph as needing recompile + regenerate. Returns instantly -- the
  // texgen worker thread runs the compile asynchronously. Coalesces naturally:
  // rapid-fire marks before the worker picks up the dirty bit collapse into a
  // single compile pass.
  virtual void markGraphDirty() = 0;

  virtual void requestForceRebuild() = 0;
  // Soft regeneration: schedule one regen pass while bypassing the autoupdate gate, but
  // without forceRebuild's pipeline reset (no regcache::clear / texGenReg.close /
  // closeTexGen). Mirrors the old standalone tool's `graph.reload_textures` console command.
  virtual void requestRegenerate() = 0;
  virtual bool getAutoupdate() const = 0;
  virtual void toggleAutoupdate() = 0;
  virtual String getCompilerLog() const = 0;

  // Heightmap metadata extracted from main graph (for landscape preview)
  virtual void setHeightmapParams(float scale, float min_height, float cell_size) = 0;
  virtual float getHeightmapScale() const = 0;
  virtual float getHeightmapMin() const = 0;
  virtual float getHeightmapCellSize() const = 0;
  virtual TEXTUREID getHeightmapTextureId() const = 0;

  // Texture preview.
  // Returned by value: the service may swap selectedTexState's BaseTexture* underneath the caller
  // when the worker thread regenerates the preview, so a reference would be a use-after-free
  // hazard. Callers receive a snapshot copied under stateLock.
  virtual SelectedTextureState getSelectedTextureState() const = 0;
  virtual const eastl::hash_set<eastl::string> &getFinalStrings() const = 0;
  virtual bool readSelectedTexturePixel(int x, int y, Color4 &out) const = 0;
  // Promote an intermediate texture register (e.g. an output pin's customTextureName) to a final
  // for the next generation cycle, so its texture survives long enough to be previewed. Pass empty
  // or nullptr to clear. Setting a different name triggers regeneration. selectedTexState updates
  // automatically when the next pipeline cycle completes -- callers don't need to poll.
  virtual void setPreviewFinal(const char *tex_name) = 0;

  // Compilation feedback
  virtual bool hasCompilationErrors() const = 0;
  virtual String getFirstCompilationError() const = 0;
  virtual bool isGenerationPending() const = 0;

  // Save
  virtual void saveTextures(const char *mask = nullptr) = 0;
  virtual bool isSaving() const = 0;
};

void init_graph_texgen_service();
