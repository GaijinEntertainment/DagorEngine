//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;
class BaseTexture;
typedef BaseTexture Texture;
class Sbuffer;
class BaseTexture;
class D3dResource;
class TextureRegManager;
class TextureGenerator;
#include <generic/dag_tabFwd.h>
#include <generic/dag_tab.h>

enum TShaderRegType
{
  TSHADER_REG_TYPE_INPUT = 0,
  TSHADER_REG_TYPE_OUTPUT = 1,
  TSHADER_REG_TYPE_CNT,
  TSHADER_REG_TYPE_NO = 0xFF
};
enum BlendingType
{
  NO_BLENDING,
  MAX_BLENDING,
  MIN_BLENDING,
  ADD_BLENDING,
  MUL_BLENDING,
  ALPHA_BLENDING,
  DEPTH_TEST,
  BLENDS_COUNT
};

static inline const char *get_tgen_type_name(TShaderRegType tp)
{
  switch (tp)
  {
    case TSHADER_REG_TYPE_INPUT: return "input";
    case TSHADER_REG_TYPE_OUTPUT: return "output";
    case TSHADER_REG_TYPE_NO: return "no";
    default: return "invalid";
  }
}

struct TextureInput
{
  BaseTexture *tex = 0;
  bool wrap = false;
  TextureInput() = default;
  TextureInput(BaseTexture *t, bool w) : tex(t), wrap(w) {}
};

struct TextureGenNodeData
{
  Tab<TextureInput> inputs;
  Tab<D3dResource *> outputs;
  Tab<int> usedRegs;
  int nodeW, nodeH;
  bool use_depth = false;
  BlendingType blending;
  Sbuffer *particles = 0;
  const char *nodeName = "";
  DataBlock params;
  TextureGenNodeData() = default;
  TextureGenNodeData(int w, int h, BlendingType blend, bool depth, const DataBlock &parameters, dag::ConstSpan<TextureInput> in,
    dag::ConstSpan<D3dResource *> out, Sbuffer *p) :
    nodeW(w), nodeH(h), blending(blend), use_depth(depth), params(parameters), inputs(in, tmpmem), outputs(out, tmpmem), particles(p)
  {}
};

// Tiled passes split a w x h node into tile x tile squares (tile > 0), row by row; the last row and column can be smaller.
inline int texgen_tiles_per_side(int size, int tile) { return (size + tile - 1) / tile; }
inline void texgen_get_tile(int w, int h, int tile, int index, int &x, int &y, int &tile_w, int &tile_h)
{
  const int tilesW = texgen_tiles_per_side(w, tile);
  x = index % tilesW * tile;
  y = index / tilesW * tile;
  tile_w = w - x < tile ? w - x : tile;
  tile_h = h - y < tile ? h - y : tile;
}


class TextureGenShader
{
public:
  virtual ~TextureGenShader() {}
  virtual void destroy() { delete this; }

  virtual const char *getName() const = 0;
  virtual int getInputParametersCount() const = 0;
  virtual bool isInputParameterOptional(int n) const = 0;

  virtual int getRegCount(TShaderRegType tp) const = 0;
  virtual bool isRegOptional(TShaderRegType tp, int reg) const = 0;
  virtual unsigned subSteps(const TextureGenNodeData &) const { return 1; }
  // True when sub-step sub_step can block until all GPU work queued before it is done, so a host ends its batch before it.
  virtual bool mapsResourcesOnCpu(int /*sub_step*/) const { return false; }
  // Pixels one process() call of this sub-step covers; by default the whole node, so a shader that splits a node overrides it.
  virtual int subStepPixels(const TextureGenNodeData &data, int) const { return data.nodeW * data.nodeH; }
  // Shader that does the work of this sub-step; a wrapper shader names the one it runs.
  virtual const char *subStepShaderName(const TextureGenNodeData &, int) const { return getName(); }
  // The pass stopped this node, or its next_sub_step failed, after sub-step 0 succeeded and while its textures are still live:
  // release what its remaining sub-steps would take or free, such as a started readback or work registers.
  virtual void cancelSubSteps(TextureRegManager &, const TextureGenNodeData &, int /*next_sub_step*/) {}

  // to be removed
  virtual bool process(TextureGenerator &gen, TextureRegManager &regs, const TextureGenNodeData &data, int subStep) = 0;
  virtual bool processAll(TextureGenerator &gen, TextureRegManager &regs, const TextureGenNodeData &data) // all substeps
  {
    unsigned cnt = subSteps(data);
    for (int i = 0; i < cnt; ++i)
      if (!process(gen, regs, data, i))
        return false;
    return true;
  }
};
