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

static class TextureGenPseudoDLA : public TextureGenShader
{
public:
  virtual ~TextureGenPseudoDLA() {}
  // virtual void destroy(){delete this;}
  virtual void destroy() {} // it is static

  virtual const char *getName() const { return "pseudo_dla"; }
  virtual int getInputParametersCount() const { return 7; }
  virtual bool isInputParameterOptional(int) const { return true; }

  // input1: heightmap
  // input2: noise
  virtual int getRegCount(TShaderRegType tp) const { return tp == TSHADER_REG_TYPE_INPUT ? 2 : 1; }
  virtual bool isRegOptional(TShaderRegType tp, int /* reg */) const { return tp == TSHADER_REG_TYPE_INPUT; }

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
    REQUIRE(gaussian_max);
    REQUIRE(warp_native);
    REQUIRE(madd_texture);
    REQUIRE(tex_autolevels);
    REQUIRE(levels_native);

    float shapeHeight = parameters.getReal("shape_height", 0.025f);
    float slopeFactor = parameters.getReal("slope_factor", 0.55f);
    float slopeDistance = parameters.getReal("slope_distance", 0.025f);
    float slopeDetailFilter = parameters.getReal("slope_detail_filter", 0.02f);
    float slopeDetailIntensity = parameters.getReal("slope_detail_intensity", 0.005f);
    float heightOffset = parameters.getReal("height_offset", 0.01f);
    int iterations = clamp(parameters.getInt("iterations", 4), 1, 10);

    G_UNUSED(iterations);

    int blackReg = regs.createTexture(String(128, "dla_black_1x1", w, h), TEXFMT_R32F, 1, 1, 1);
    BaseTexture *black = regs.getTexture(blackReg);
    float *ptr = nullptr;
    int stride = 0;
    if (black->lockimg((void **)&ptr, stride, 0, TEXLOCK_WRITE))
    {
      ptr[0] = 0.0f;
      black->unlockimg();
    }

    int whiteReg = regs.createTexture(String(128, "dla_white_1x1", w, h), TEXFMT_R32F, 1, 1, 1);
    BaseTexture *white = regs.getTexture(whiteReg);
    if (white->lockimg((void **)&ptr, stride, 0, TEXLOCK_WRITE))
    {
      ptr[0] = 1.0f;
      white->unlockimg();
    }

    int gaussianMaxResultReg = regs.createTexture(String(128, "dla_gm_result_%dx%d", w, h), TEXFMT_R32F, w, h, 1);
    BaseTexture *gaussianMaxResult = regs.getTexture(gaussianMaxResultReg);

    int warpResultReg = regs.createTexture(String(128, "dla_warp_result_%dx%d", w, h), TEXFMT_R32F, w, h, 1);
    BaseTexture *warpResult = regs.getTexture(warpResultReg);

    int maddResultReg[2] = {
      regs.createTexture(String(128, "dla_madd_result0_%dx%d", w, h), TEXFMT_R32F, w, h, 1),
      regs.createTexture(String(128, "dla_madd_result1_%dx%d", w, h), TEXFMT_R32F, w, h, 1),
    };
    BaseTexture *maddResult[2] = {
      regs.getTexture(maddResultReg[0]),
      regs.getTexture(maddResultReg[1]),
    };

    int tmpReg = regs.createTexture(String(128, "dla_tmp_%dx%d", w, h), TEXFMT_R32F, w, h, 1);
    BaseTexture *tmp = regs.getTexture(tmpReg);

    int maddInput = 0;
    int maddOutput = 1;

    for (int iter = 0; iter < iterations; iter++)
    {
      maddInput = iter % 2;
      maddOutput = (iter + 1) % 2;

      {
        DataBlock gaussianMaxParams;
        gaussianMaxParams.setReal("distance", slopeDistance);
        gaussianMaxParams.setReal("sigma", 0.227761);
        gaussianMaxParams.setBool("vertical", true);
        gaussianMaxParams.setBool("wrap", false);

        TextureInput shaderInputs[2] = {
          (iter == 0 ? inputs[0] : TextureInput(warpResult, false)), // base
          TextureInput(white, false),                                // dist
        };

        D3dResource *out = (D3dResource *)tmp;

        gaussian_max->processAll(gen, regs,
          TextureGenNodeData(w, h, NO_BLENDING, false, gaussianMaxParams, make_span_const(shaderInputs, countof(shaderInputs)),
            make_span_const(&out, 1), NULL));

        gaussianMaxParams.setBool("vertical", false);

        shaderInputs[0] = TextureInput(tmp, false);
        out = (D3dResource *)gaussianMaxResult;

        gaussian_max->processAll(gen, regs,
          TextureGenNodeData(w, h, NO_BLENDING, false, gaussianMaxParams, make_span_const(shaderInputs, countof(shaderInputs)),
            make_span_const(&out, 1), NULL));
      }

      {
        DataBlock warpParams;
        warpParams.setReal("filter_size", slopeDetailFilter);
        warpParams.setReal("gradient_mul", slopeDetailIntensity);

        TextureInput shaderInputs[2] = {
          TextureInput(gaussianMaxResult, false), // texture
          inputs[1],                              // noise
        };

        D3dResource *out = (D3dResource *)warpResult;

        warp_native->processAll(gen, regs,
          TextureGenNodeData(w, h, NO_BLENDING, false, warpParams, make_span_const(shaderInputs, countof(shaderInputs)),
            make_span_const(&out, 1), NULL));
      }

      {
        DataBlock maddParams;
        maddParams.setReal("scale", heightOffset);

        TextureInput shaderInputs[2] = {
          TextureInput(warpResult, false),                                // base
          TextureInput(iter == 0 ? black : maddResult[maddInput], false), // to_add * scale
        };

        D3dResource *out = (D3dResource *)maddResult[maddOutput];
        madd_texture->processAll(gen, regs,
          TextureGenNodeData(w, h, NO_BLENDING, false, maddParams, make_span_const(shaderInputs, countof(shaderInputs)),
            make_span_const(&out, 1), NULL));
      }
    } // iterations

    {
      // tex_autolevels
      DataBlock autolevelsParams;

      TextureInput autolevelsInputs[1] = {
        TextureInput(maddResult[maddOutput], false), // texture
      };

      D3dResource *out = (D3dResource *)gaussianMaxResult;
      tex_autolevels->processAll(gen, regs,
        TextureGenNodeData(w, h, NO_BLENDING, false, autolevelsParams, make_span_const(autolevelsInputs, countof(autolevelsInputs)),
          make_span_const(&out, 1), NULL));
    }

    {
      // levels_native
      DataBlock levelsParams;
      levelsParams.setReal("in_low", 0.0f);
      levelsParams.setReal("in_mid", slopeFactor);
      levelsParams.setReal("in_high", 1.0f);
      levelsParams.setReal("out_low", 0.0f);
      levelsParams.setReal("out_high", shapeHeight);

      TextureInput levelsInputs[1] = {
        TextureInput(gaussianMaxResult, false), // texture
      };

      D3dResource *out = outputs[0];
      levels_native->processAll(gen, regs,
        TextureGenNodeData(w, h, NO_BLENDING, false, levelsParams, make_span_const(levelsInputs, countof(levelsInputs)),
          make_span_const(&out, 1), NULL));
    }

    regs.textureUsed(blackReg);
    regs.textureUsed(whiteReg);
    regs.textureUsed(gaussianMaxResultReg);
    regs.textureUsed(warpResultReg);
    regs.textureUsed(maddResultReg[0]);
    regs.textureUsed(maddResultReg[1]);
    regs.textureUsed(tmpReg);
    return true;
  }
} texgen_pseudo_dla;

void init_texgen_pseudo_dla_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_pseudo_dla.getName(), &texgen_pseudo_dla);
}
