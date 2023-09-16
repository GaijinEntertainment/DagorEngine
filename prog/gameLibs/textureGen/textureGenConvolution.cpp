#include <ioSys/dag_dataBlock.h>
#include <3d/dag_drv3d.h>
#include <math/dag_adjpow2.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_string.h>
#include <generic/dag_carray.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>


#define CONVOLUTION_STEPS 5 // up to 32768 x 32768


static class TextureGenConvolution : public TextureGenShader
{
public:
  virtual ~TextureGenConvolution() {}
  // virtual void destroy(){delete this;}
  virtual void destroy() {} // it is static

  virtual const char *getName() const { return "tex_convolution"; }
  virtual int getInputParametersCount() const { return 0; }
  virtual bool isInputParameterOptional(int) const { return true; }

  virtual int getRegCount(TShaderRegType tp) const { return tp == TSHADER_REG_TYPE_INPUT ? 1 : 1; }
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
    REQUIRE(min_value_x8_step);
    REQUIRE(max_value_x8_step);
    REQUIRE(average_value_x8_step);
    REQUIRE(unary);

    if (!(Texture *)inputs[0].tex)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR,
        String(128, "internal error in TextureGenConvolution (inputs[0].tex == NULL) %s", getName()));
      return false;
    }

    TextureGenShader *convShader = average_value_x8_step;
    const char *convFunction = parameters.getStr("function", "average");

    if (!strcmp(convFunction, "average"))
      convShader = average_value_x8_step;
    else if (!strcmp(convFunction, "min"))
      convShader = min_value_x8_step;
    else if (!strcmp(convFunction, "max"))
      convShader = max_value_x8_step;
    else
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "invalid convolution function %s", convFunction));


    carray<int, CONVOLUTION_STEPS> tempReg;
    BaseTexture *temp[CONVOLUTION_STEPS] = {NULL, NULL, NULL, NULL, NULL};
    carray<IPoint2, CONVOLUTION_STEPS> sizes;
    int steps = 0;

    TextureInfo tinfo;
    inputs[0].tex->getinfo(tinfo, 0);

    int stepWidth = tinfo.w;
    int stepHeight = tinfo.h;
    uint32_t tempRegFmt = tinfo.cflg & TEXFMT_MASK;

    while (stepWidth > 1 || stepHeight > 1)
    {
      stepWidth = max(stepWidth / 8, 1);
      stepHeight = max(stepHeight / 8, 1);
      sizes[steps] = IPoint2(stepWidth, stepHeight);
      tempReg[steps] = regs.createTexture(String(128, "conv_iter_%dx%d", stepWidth, stepHeight), tempRegFmt, stepWidth, stepHeight, 1);
      temp[steps] = regs.getTexture(tempReg[steps]);
      steps++;

      if (steps == CONVOLUTION_STEPS)
        break;
    }

    for (int i = 0; i <= steps; i++)
    {
      DataBlock stepParams;

      TextureInput in = i == 0 ? inputs[0] : TextureInput(temp[i - 1], false);
      D3dResource *out = (i == steps) ? outputs[0] : (D3dResource *)temp[i];

      if (i == steps)
      {
        unary->processAll(gen, regs,
          TextureGenNodeData(w, h, NO_BLENDING, false, stepParams, make_span_const(&in, 1), make_span_const(&out, 1), NULL));
      }
      else
      {
        stepParams.addInt("resultWidth", sizes[i].x);
        stepParams.addInt("resultHeight", sizes[i].y);

        convShader->processAll(gen, regs,
          TextureGenNodeData(sizes[i].x, sizes[i].y, NO_BLENDING, false, stepParams, make_span_const(&in, 1), make_span_const(&out, 1),
            NULL));
      }
    }

    for (int i = 0; i < steps; i++)
      regs.textureUsed(tempReg[i]);

    return true;
  }
} texgen_convolution;

void init_texgen_convolution_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_convolution.getName(), &texgen_convolution);
}
