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

static class TextureGenHeightmapOcclusion : public TextureGenShader
{
public:
  virtual ~TextureGenHeightmapOcclusion() {}
  virtual void destroy() {}

  virtual const char *getName() const { return "heightmap_occlusion"; }

  virtual int getInputParametersCount() const { return 2; }
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
    REQUIRE(heightmap_occlusion_ps);
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
    psParams.setReal("strength", data.params.getReal("strength", 2.f));
    psParams.setReal("octaves", (float)clamp(data.params.getInt("octaves", 2), 1, 8));

    heightmap_occlusion_ps->processAll(gen, regs,
      TextureGenNodeData(data.nodeW, data.nodeH, NO_BLENDING, false, psParams, data.inputs, data.outputs, nullptr));

    return true;
  }
} texgen_heightmap_occlusion;

void init_texgen_heightmap_occlusion_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_heightmap_occlusion.getName(), &texgen_heightmap_occlusion);
}