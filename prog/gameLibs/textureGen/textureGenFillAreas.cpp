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

static class TextureGenFillAreas : public TextureGenShader
{
public:
  virtual ~TextureGenFillAreas() {}
  // virtual void destroy(){delete this;}
  virtual void destroy() {} // it is static

  virtual const char *getName() const { return "fill_areas"; }
  virtual int getInputParametersCount() const { return 2; }
  virtual bool isInputParameterOptional(int) const { return true; }

  virtual int getRegCount(TShaderRegType tp) const { return tp == TSHADER_REG_TYPE_INPUT ? 2 : 2; }
  virtual bool isRegOptional(TShaderRegType tp, int reg) const { return tp == TSHADER_REG_TYPE_INPUT ? reg == 1 : false; }

  virtual bool process(TextureGenerator &gen, TextureRegManager &regs, const TextureGenNodeData &data, int)
  {
    int w = data.nodeW, h = data.nodeH;
    const DataBlock &parameters = data.params;
    dag::ConstSpan<TextureInput> inputs = data.inputs;
    dag::ConstSpan<D3dResource *> outputs = data.outputs;

    if (inputs.size() < 1)
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
    REQUIRE(fill_areas_init);
    REQUIRE(fill_areas_step);
    REQUIRE(fill_areas_postprocess);

    float seed = parameters.getReal("seed", 0.0);
    int steps = clamp(parameters.getInt("steps", 8), 2, 32) * 2 + 2; // +2 - for init and postprocess
    int quantization = clamp(parameters.getInt("quantization", 500000), 0, INT_MAX);
    if (quantization == 0)
      quantization = (1 << 19) - 1;

    carray<int, 2> tempReg;
    BaseTexture *temp[2] = {NULL, NULL};
    if (steps > 1)
    {
      uint32_t tempRegFmt = TEXFMT_R32F;
      tempReg[0] = regs.createTexture(String(128, "fill_iter_%dx%d_0", w, h), tempRegFmt, w, h, 1);
      tempReg[1] = regs.createTexture(String(128, "fill_iter_%dx%d_1", w, h), tempRegFmt, w, h, 1);
      temp[0] = regs.getTexture(tempReg[0]);
      temp[1] = regs.getTexture(tempReg[1]);
    }

    for (int i = 0; i < steps; i++)
    {
      DataBlock stepParams;
      stepParams.setReal("seed", seed);
      stepParams.setInt("step", i);
      stepParams.setInt("quantization", quantization);

      TextureInput in[1] = {i == 0 ? inputs[0] : TextureInput(temp[i % 2], false)};
      D3dResource *out = (i == steps - 1) ? outputs[0] : (D3dResource *)temp[(i + 1) % 2];

      TextureGenShader *curShader = nullptr;
      if (i == 0)
        curShader = fill_areas_init;
      else if (i == steps - 1)
        curShader = fill_areas_postprocess;
      else
        curShader = fill_areas_step;

      curShader->processAll(gen, regs,
        TextureGenNodeData(w, h, NO_BLENDING, false, stepParams, make_span_const(in, countof(in)), make_span_const(&out, 1), NULL));
    }

    regs.textureUsed(tempReg[0]);
    regs.textureUsed(tempReg[1]);

    return true;
  }
} texgen_fill_areas;

void init_texgen_fill_areas_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_fill_areas.getName(), &texgen_fill_areas);
}
