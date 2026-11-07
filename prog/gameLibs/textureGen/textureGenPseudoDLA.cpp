// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderTarget.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <generic/dag_carray.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>

namespace
{
enum DlaIterationPass
{
  DLA_GAUSSIAN_MAX_V,
  DLA_GAUSSIAN_MAX_H,
  DLA_WARP,
  DLA_MADD,
  DLA_ITERATION_PASSES
};

enum DlaTarget
{
  DLA_BLACK,
  DLA_WHITE,
  DLA_GM_RESULT,
  DLA_WARP_RESULT,
  DLA_MADD_RESULT0,
  DLA_MADD_RESULT1,
  DLA_TMP,
  DLA_TARGET_COUNT
};

const char *const DLA_TARGET_NAMES[DLA_TARGET_COUNT] = {"dla_black_1x1", "dla_white_1x1", "dla_gm_result_%dx%d",
  "dla_warp_result_%dx%d", "dla_madd_result0_%dx%d", "dla_madd_result1_%dx%d", "dla_tmp_%dx%d"};

struct DlaStep
{
  int pass = 0;
  int tile = 0;
};

int dla_iterations(const DataBlock &params) { return clamp(params.getInt("iterations", 4), 1, 10); }

// tex_autolevels, the pass after the iteration passes, needs the min and max of the whole input.
bool dla_pass_tiled(int pass, int iteration_passes) { return texgen_get_max_tile_size() > 0 && pass != iteration_passes; }

int dla_tiles(int w, int h)
{
  const int tile = texgen_get_max_tile_size();
  return tile > 0 ? texgen_tiles_per_side(w, tile) * texgen_tiles_per_side(h, tile) : 1;
}

DlaStep dla_step(const TextureGenNodeData &data, int iteration_passes, int sub_step)
{
  const int tiles = dla_tiles(data.nodeW, data.nodeH);
  if (sub_step < iteration_passes * tiles)
  {
    return {sub_step / tiles, sub_step % tiles};
  }
  if (sub_step == iteration_passes * tiles)
  {
    return {iteration_passes, 0};
  }
  return {iteration_passes + 1, sub_step - iteration_passes * tiles - 1};
}
} // namespace

static class TextureGenPseudoDLA : public TextureGenShader
{
public:
  virtual ~TextureGenPseudoDLA() {}
  virtual void destroy() {} // it is static

  virtual const char *getName() const { return "pseudo_dla"; }
  virtual int getInputParametersCount() const { return 7; }
  virtual bool isInputParameterOptional(int) const { return true; }

  // input1: heightmap
  // input2: noise
  virtual int getRegCount(TShaderRegType tp) const { return tp == TSHADER_REG_TYPE_INPUT ? 2 : 1; }
  virtual bool isRegOptional(TShaderRegType tp, int /* reg */) const { return tp == TSHADER_REG_TYPE_INPUT; }

  virtual unsigned subSteps(const TextureGenNodeData &data) const
  {
    if (data.inputs.size() < 2 || data.outputs.size() < 1)
    {
      return 1;
    }
    const int tiles = dla_tiles(data.nodeW, data.nodeH);
    return dla_iterations(data.params) * DLA_ITERATION_PASSES * tiles + 1 + tiles;
  }

  virtual int subStepPixels(const TextureGenNodeData &data, int sub_step) const
  {
    const int iterationPasses = dla_iterations(data.params) * DLA_ITERATION_PASSES;
    const DlaStep step = dla_step(data, iterationPasses, sub_step);
    if (!dla_pass_tiled(step.pass, iterationPasses))
    {
      return data.nodeW * data.nodeH;
    }
    int x = 0, y = 0, tileW = 0, tileH = 0;
    texgen_get_tile(data.nodeW, data.nodeH, texgen_get_max_tile_size(), step.tile, x, y, tileW, tileH);
    return tileW * tileH;
  }

  virtual const char *subStepShaderName(const TextureGenNodeData &data, int sub_step) const
  {
    const int iterationPasses = dla_iterations(data.params) * DLA_ITERATION_PASSES;
    const int pass = dla_step(data, iterationPasses, sub_step).pass;
    if (pass >= iterationPasses)
    {
      return pass == iterationPasses ? "tex_autolevels" : "levels_native";
    }
    const int kind = pass % DLA_ITERATION_PASSES;
    return kind == DLA_WARP ? "warp_native" : (kind == DLA_MADD ? "madd_texture" : "gaussian_max");
  }

  virtual void cancelSubSteps(TextureRegManager &regs, const TextureGenNodeData &data, int)
  {
    for (const char *name : DLA_TARGET_NAMES)
      if (const int reg = regs.getRegNo(String(128, name, data.nodeW, data.nodeH)); regs.getRegUsageLeft(reg) > 0)
        regs.discard(reg);
  }

  virtual bool process(TextureGenerator &gen, TextureRegManager &regs, const TextureGenNodeData &data, int sub_step)
  {
    int w = data.nodeW, h = data.nodeH;
    const DataBlock &parameters = data.params;
    dag::ConstSpan<TextureInput> inputs = data.inputs;
    dag::ConstSpan<D3dResource *> outputs = data.outputs;

    if (inputs.size() < 2)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "not enough inputs for %s", getName()));
      return false;
    }
    if (outputs.size() < 1)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_REMARK, String(128, "not enough outputs for %s", getName()));
      return true;
    }
    const char *shaderName = subStepShaderName(data, sub_step);
    TextureGenShader *shader = texgen_get_shader(&gen, shaderName);
    if (!shader)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "no shader <%s> required for %s", shaderName, getName()));
      return false;
    }

    const int iterations = dla_iterations(parameters);
    const int iterationPasses = iterations * DLA_ITERATION_PASSES;

    int targetReg[DLA_TARGET_COUNT];
    BaseTexture *target[DLA_TARGET_COUNT];
    for (int i = 0; i < DLA_TARGET_COUNT; ++i)
    {
      const bool single = i < DLA_GM_RESULT;
      const String name(128, DLA_TARGET_NAMES[i], w, h);
      const int stale = sub_step == 0 ? regs.getRegNo(name) : -1;
      if (regs.getRegUsageLeft(stale) > 0) // left in use by a run cut short
        regs.discard(stale);
      targetReg[i] = sub_step == 0 ? regs.createTexture(name, TEXFMT_R32F, single ? 1 : w, single ? 1 : h, 1) : regs.getRegNo(name);
      target[i] = targetReg[i] >= 0 ? regs.getTexture(targetReg[i]) : nullptr;
      if (!target[i])
      {
        texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "no texture <%s> for %s", name.str(), getName()));
        return false;
      }
    }
    if (sub_step == 0)
    {
      d3d::clear_rt({target[DLA_BLACK], 0, 0});
      d3d::clear_rt({target[DLA_WHITE], 0, 0}, make_clear_value(1.f, 1.f, 1.f, 1.f));
    }

    const DlaStep step = dla_step(data, iterationPasses, sub_step);
    DataBlock passParams;
    TextureInput passInputs[2];
    int passInputCount = 1;
    D3dResource *out = outputs[0];
    if (step.pass < iterationPasses)
    {
      const int iter = step.pass / DLA_ITERATION_PASSES;
      const int kind = step.pass % DLA_ITERATION_PASSES;
      passInputCount = 2;
      if (kind == DLA_GAUSSIAN_MAX_V || kind == DLA_GAUSSIAN_MAX_H)
      {
        passParams.setReal("distance", parameters.getReal("slope_distance", 0.025f));
        passParams.setReal("sigma", 0.227761);
        passParams.setBool("vertical", kind == DLA_GAUSSIAN_MAX_V);
        passParams.setBool("wrap", false);
        if (kind == DLA_GAUSSIAN_MAX_V)
        {
          passInputs[0] = iter == 0 ? inputs[0] : TextureInput(target[DLA_WARP_RESULT], false); // base
          out = target[DLA_TMP];
        }
        else
        {
          passInputs[0] = TextureInput(target[DLA_TMP], false);
          out = target[DLA_GM_RESULT];
        }
        passInputs[1] = TextureInput(target[DLA_WHITE], false); // dist
      }
      else if (kind == DLA_WARP)
      {
        passParams.setReal("filter_size", parameters.getReal("slope_detail_filter", 0.02f));
        passParams.setReal("gradient_mul", parameters.getReal("slope_detail_intensity", 0.005f));
        passInputs[0] = TextureInput(target[DLA_GM_RESULT], false); // texture
        passInputs[1] = inputs[1];                                  // noise
        out = target[DLA_WARP_RESULT];
      }
      else
      {
        passParams.setReal("scale", parameters.getReal("height_offset", 0.01f));
        passInputs[0] = TextureInput(target[DLA_WARP_RESULT], false);                                             // base
        passInputs[1] = TextureInput(iter == 0 ? target[DLA_BLACK] : target[DLA_MADD_RESULT0 + iter % 2], false); // to_add * scale
        out = target[DLA_MADD_RESULT0 + (iter + 1) % 2];
      }
    }
    else if (step.pass == iterationPasses)
    {
      passInputs[0] = TextureInput(target[DLA_MADD_RESULT0 + iterations % 2], false);
      out = target[DLA_GM_RESULT];
    }
    else
    {
      passParams.setReal("in_low", 0.0f);
      passParams.setReal("in_mid", parameters.getReal("slope_factor", 0.55f));
      passParams.setReal("in_high", 1.0f);
      passParams.setReal("out_low", 0.0f);
      passParams.setReal("out_high", parameters.getReal("shape_height", 0.025f));
      passInputs[0] = TextureInput(target[DLA_GM_RESULT], false);
    }

    const int tileSize = texgen_get_max_tile_size();
    const bool tiled = dla_pass_tiled(step.pass, iterationPasses);
    if (tiled)
    {
      passParams.setInt("maxSubStepSize", tileSize);
    }
    const TextureGenNodeData passData(w, h, NO_BLENDING, false, passParams, make_span_const(passInputs, passInputCount),
      make_span_const(&out, 1), NULL);
    if (!(tiled ? shader->process(gen, regs, passData, step.tile) : shader->processAll(gen, regs, passData)))
    {
      return false;
    }

    if (sub_step + 1 == static_cast<int>(subSteps(data)))
    {
      for (int reg : targetReg)
      {
        regs.textureUsed(reg);
      }
    }
    return true;
  }
} texgen_pseudo_dla;

void init_texgen_pseudo_dla_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_pseudo_dla.getName(), &texgen_pseudo_dla);
}
