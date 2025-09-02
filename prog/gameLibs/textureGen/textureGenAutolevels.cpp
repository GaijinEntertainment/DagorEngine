// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_adjpow2.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_string.h>
#include <generic/dag_carray.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>


static class TextureGenAutolevels : public TextureGenShader
{
public:
  virtual ~TextureGenAutolevels() {}
  // virtual void destroy(){delete this;}
  virtual void destroy() {} // it is static

  virtual const char *getName() const { return "tex_autolevels"; }
  virtual int getInputParametersCount() const { return 0; }
  virtual bool isInputParameterOptional(int) const { return true; }

  virtual int getRegCount(TShaderRegType tp) const { return tp == TSHADER_REG_TYPE_INPUT ? 1 : 1; }
  virtual bool isRegOptional(TShaderRegType tp, int reg) const { return tp == TSHADER_REG_TYPE_INPUT ? reg == 1 : false; }

  virtual bool process(TextureGenerator &gen, TextureRegManager &regs, const TextureGenNodeData &data, int)
  {
    int w = data.nodeW, h = data.nodeH;
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
    REQUIRE(tex_convolution);
    REQUIRE(autolevels_final_shader);

    int regMin = regs.createTexture(String(128, "autolevels_min"), TEXFMT_A32B32G32R32F, 2, 2, 1);
    int regMax = regs.createTexture(String(128, "autolevels_max"), TEXFMT_A32B32G32R32F, 2, 2, 1);
    G_ASSERT(regMin >= 0);
    G_ASSERT(regMax >= 0);

    BaseTexture *texMin = regs.getTexture(regMin);
    BaseTexture *texMax = regs.getTexture(regMax);

    {
      DataBlock paramsMin;
      paramsMin.setStr("function", "min");
      D3dResource *outMin = (D3dResource *)texMin;
      tex_convolution->processAll(gen, regs,
        TextureGenNodeData(2, 2, NO_BLENDING, false, paramsMin, inputs, make_span_const(&outMin, 1), NULL));

      DataBlock paramsMax;
      paramsMax.setStr("function", "max");
      D3dResource *outMax = (D3dResource *)texMax;
      tex_convolution->processAll(gen, regs,
        TextureGenNodeData(2, 2, NO_BLENDING, false, paramsMax, inputs, make_span_const(&outMax, 1), NULL));
    }

    {
      DataBlock params;
      carray<TextureInput, 3> in = {inputs[0], {texMin, false}, {texMax, false}};

      autolevels_final_shader->processAll(gen, regs, TextureGenNodeData(w, h, NO_BLENDING, false, params, in, outputs, NULL));
    }

    regs.textureUsed(regMin);
    regs.textureUsed(regMax);

    return true;
  }
} texgen_autolevels;

void init_texgen_autolevels_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_autolevels.getName(), &texgen_autolevels);
}
