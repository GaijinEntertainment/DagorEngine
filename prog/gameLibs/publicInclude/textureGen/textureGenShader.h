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
  DataBlock params;
  TextureGenNodeData() = default;
  TextureGenNodeData(int w, int h, BlendingType blend, bool depth, const DataBlock &parameters, dag::ConstSpan<TextureInput> in,
    dag::ConstSpan<D3dResource *> out, Sbuffer *p) :
    nodeW(w), nodeH(h), blending(blend), use_depth(depth), params(parameters), inputs(in, tmpmem), outputs(out, tmpmem), particles(p)
  {}
};


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
