// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <generic/dag_carray.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>

static class TextureGenBlurIter : public TextureGenShader
{
public:
  virtual ~TextureGenBlurIter() {}
  // virtual void destroy(){delete this;}
  virtual void destroy() {} // it is static

  virtual const char *getName() const { return "blur_iterational"; }
  virtual int getInputParametersCount() const { return 6; }
  virtual bool isInputParameterOptional(int) const { return true; }

  virtual int getRegCount(TShaderRegType tp) const { return tp == TSHADER_REG_TYPE_INPUT ? 2 : 2; }
  virtual bool isRegOptional(TShaderRegType tp, int reg) const { return tp == TSHADER_REG_TYPE_INPUT ? reg == 1 : false; }

  virtual bool process(TextureGenerator &gen, TextureRegManager &regs, const TextureGenNodeData &data, int)
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
#define REQUIRE(a)                                                                                            \
  TextureGenShader *a = texgen_get_shader(&gen, #a);                                                          \
  if (!a)                                                                                                     \
  {                                                                                                           \
    texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "no shader <%s> required for %s", #a, getName())); \
    return false;                                                                                             \
  }
    REQUIRE(blur_hq_step);

    bool wrap = parameters.getBool("wrap", true);
    float distance = clamp(parameters.getReal("distance", 0.05), 0.f, 1.f);
    float random = clamp(parameters.getReal("random", 1.0), 0.f, 1.f);
    float startTurn = clamp(parameters.getReal("start_turn", 0.0), 0.f, 1.f);
    float stepTurn = clamp(parameters.getReal("step_turn", 0.2), 0.f, 1.f);
    float stepScale = clamp(parameters.getReal("step_scale", 0.65), 0.01f, 1.f);
    int steps = clamp(parameters.getInt("steps", 8), 1, 14);
    int rays = clamp(parameters.getInt("rays", 7), 1, 16);

    carray<int, 2> tempReg;
    BaseTexture *temp[2] = {NULL, NULL};
    if (steps > 1)
    {
      uint32_t tempRegFmt = extend_tex_fmt_to_32f(inputs[0].tex);
      tempReg[0] = regs.createTexture(String(128, "blur_iter_%dx%d_0", w, h), tempRegFmt, w, h, 1);
      tempReg[1] = regs.createTexture(String(128, "blur_iter_%dx%d_1", w, h), tempRegFmt, w, h, 1);
      temp[0] = regs.getTexture(tempReg[0]);
      temp[1] = regs.getTexture(tempReg[1]);
    }

    for (int i = 0; i < steps; i++)
    {
      DataBlock stepParams;
      stepParams.setBool("wrap", wrap);
      stepParams.setReal("distance", distance);
      stepParams.setReal("random_angle", random * 2 * PI / rays);
      stepParams.setReal("start_turn", startTurn + i * 2 * PI * stepTurn);
      stepParams.setInt("rays", rays);
      stepParams.setReal("step", float(i));

      TextureInput in[2] = {i == 0 ? inputs[0] : TextureInput(temp[i % 2], false), inputs[1]};
      D3dResource *out = (i == steps - 1) ? outputs[0] : (D3dResource *)temp[(i + 1) % 2];

      blur_hq_step->processAll(gen, regs,
        TextureGenNodeData(w, h, NO_BLENDING, false, stepParams, make_span_const(in, countof(in)), make_span_const(&out, 1), NULL));

      distance = max(distance * stepScale, 1.f / 8192);
    }

    if (steps > 1)
    {
      regs.textureUsed(tempReg[0]);
      regs.textureUsed(tempReg[1]);
    }

    return true;
  }
} texgen_blur_iter;

void init_texgen_blur_iter_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_blur_iter.getName(), &texgen_blur_iter);
}
