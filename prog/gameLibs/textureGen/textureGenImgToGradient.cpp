// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_mathBase.h>
#include <util/dag_string.h>
#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>

static int sort_mode_from_string(const char *s)
{
  if (!s)
    return 0;
  if (stricmp(s, "temperature") == 0)
    return 1;
  if (stricmp(s, "hue") == 0)
    return 2;
  if (stricmp(s, "saturation") == 0)
    return 3;
  return 0; // default: luminance
}

static class TextureGenImgToGradient : public TextureGenShader
{
public:
  virtual ~TextureGenImgToGradient() {}
  virtual void destroy() {}
  virtual const char *getName() const { return "img_to_gradient"; }
  virtual int getInputParametersCount() const { return 8; }
  virtual bool isInputParameterOptional(int) const { return true; }
  virtual int getRegCount(TShaderRegType /*tp*/) const { return 1; }
  virtual bool isRegOptional(TShaderRegType tp, int /*reg*/) const { return tp != TSHADER_REG_TYPE_INPUT; }

  virtual bool process(TextureGenerator &gen, TextureRegManager &regs, const TextureGenNodeData &data, int)
  {
#define REQUIRE(a)                                                                                            \
  TextureGenShader *a = texgen_get_shader(&gen, #a);                                                          \
  if (!a)                                                                                                     \
  {                                                                                                           \
    texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "no shader <%s> required for %s", #a, getName())); \
    return false;                                                                                             \
  }
    REQUIRE(img_to_gradient_reduce_ps);
    REQUIRE(img_to_gradient_ps);
#undef REQUIRE

    if (data.inputs.size() < 1 || !data.inputs[0].tex)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "not enough inputs for %s", getName()));
      return false;
    }
    if (data.outputs.size() < 1 || !data.outputs[0])
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_REMARK, String(128, "not enough outputs for %s", getName()));
      return true;
    }

    DataBlock psParams;
    psParams.setReal("bandwidth", data.params.getReal("bandwidth", 0.05f));
    psParams.setBool("invert", data.params.getBool("invert", false));
    psParams.setReal("clip_left", data.params.getReal("clip_left", 0.0f));
    psParams.setReal("clip_right", data.params.getReal("clip_right", 1.0f));
    psParams.setReal("color_influence", data.params.getReal("color_influence", 3.0f));
    psParams.setReal("bias", data.params.getReal("bias", 0.5f));
    psParams.setInt("sort_mode", sort_mode_from_string(data.params.getStr("sort_mode", "luminance")));

    const int cellsReg = regs.createTexture("img_to_gradient_cells", TEXFMT_A32B32G32R32F, CELLS, CELLS, 1);
    if (cellsReg < 0)
      return false;
    BaseTexture *cells = regs.getTexture(cellsReg);
    D3dResource *cellsOut = cells;
    const TextureInput cellsIn(cells, false);
    const bool ok =
      img_to_gradient_reduce_ps->processAll(gen, regs,
        TextureGenNodeData(CELLS, CELLS, NO_BLENDING, false, DataBlock(), make_span_const(data.inputs.data(), 1),
          make_span_const(&cellsOut, 1), nullptr)) &&
      img_to_gradient_ps->processAll(gen, regs,
        TextureGenNodeData(data.nodeW, data.nodeH, NO_BLENDING, false, psParams, make_span_const(&cellsIn, 1), data.outputs, nullptr));
    regs.textureUsed(cellsReg);
    return ok;
  }

private:
  static constexpr int CELLS = 32;
} texgen_img_to_gradient;

void init_texgen_img_to_gradient_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_img_to_gradient.getName(), &texgen_img_to_gradient);
}
