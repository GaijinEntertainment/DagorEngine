#include <ioSys/dag_dataBlock.h>
#include <3d/dag_drv3d.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <generic/dag_carray.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>

static class TextureGenDF : public TextureGenShader
{
public:
  virtual ~TextureGenDF() {}
  // virtual void destroy(){delete this;}
  virtual void destroy() {} // it is static

  virtual const char *getName() const { return "distance_field"; }
  virtual int getInputParametersCount() const { return 3; }
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
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "not enough inputs for distance field"));
      return false;
    }
    if (outputs.size() < 1)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_REMARK, String(128, "not enough outputs for distance field"));
      return true;
    }

    for (int i = 0; i < inputs.size(); i++)
      if (inputs[i].tex == NULL)
      {
        texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "Distance Field: input #%d is NULL", i));
        return false;
      }

#define REQUIRE(a)                                                                                             \
  TextureGenShader *a = texgen_get_shader(&gen, #a);                                                           \
  if (!a)                                                                                                      \
  {                                                                                                            \
    texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "no shader <%s> required for distance field", #a)); \
    return false;                                                                                              \
  }
    REQUIRE(selectPoints);
    REQUIRE(jumpFlooding);

    REQUIRE(writeDistance);
    REQUIRE(writeVoronoiColor);

    bool wrap = parameters.getBool("wrap", true);
    bool signedDist = parameters.getBool("signed", true);
    float dfLength = clamp(parameters.getReal("distance", 1), 0.f, 1.f);

    int pixelsLen = clamp((int)(dfLength * w), (int)1, w);
    int log2Len = get_bigger_log2(pixelsLen);

    carray<int, 2> tempReg;
    tempReg[0] = regs.createTexture(String(128, "df_%dx%d_0", w, h), TEXFMT_G16R16, w, h, 1);
    tempReg[1] = regs.createTexture(String(128, "df_%dx%d_1", w, h), TEXFMT_G16R16, w, h, 1);
    TextureInput temp[2] = {{regs.getTexture(tempReg[0]), false}, {regs.getTexture(tempReg[1]), false}};
    int cTemp = 0;
    {
      DataBlock selectPointsParams;
      // selectPointsParams.setInt("distanceI", 1);
      selectPointsParams.setReal("threshold", parameters.getReal("threshold", 0.5));
      selectPointsParams.setBool("wrap", wrap);
      selectPoints->processAll(gen, regs,
        TextureGenNodeData(w, h, NO_BLENDING, false, selectPointsParams, make_span_const(&inputs[0], 1),
          make_span_const((D3dResource **)&temp[1 - cTemp].tex, 1), NULL));
      cTemp = 1 - cTemp;
    }

    DataBlock jumpFloodingParams;
    for (int i = log2Len; i >= -1; --i)
    {
      jumpFloodingParams.setInt("distanceI", 1 << max(i, (int)0));
      jumpFloodingParams.setBool("wrap", wrap);
      jumpFlooding->processAll(gen, regs,
        TextureGenNodeData(w, h, NO_BLENDING, false, jumpFloodingParams, make_span_const(&temp[cTemp], 1),
          make_span_const((D3dResource **)&temp[1 - cTemp].tex, 1), NULL));
      cTemp = 1 - cTemp;
    }
    regs.textureUsed(tempReg[1 - cTemp]);

    DataBlock write;
    write.setBool("wrap", wrap);
    write.setReal("distance", pixelsLen);
    write.setBool("signed", signedDist);


    TextureInfo tinfo;
    inputs[1].tex->getinfo(tinfo, 0);
    bool isColorConnected = tinfo.w > 1;

    if (isColorConnected == false)
    {
      writeDistance->processAll(gen, regs,
        TextureGenNodeData(w, h, data.blending, data.use_depth, write, make_span_const(&temp[cTemp], 1), outputs, NULL));
    }
    else
    {
      TextureInput defColors[2] = {temp[cTemp], inputs[1]};
      writeVoronoiColor->processAll(gen, regs,
        TextureGenNodeData(w, h, data.blending, data.use_depth, write, make_span_const(defColors, 2), outputs, NULL));
    }
    regs.textureUsed(tempReg[cTemp]);
    return true;
  }
} texgen_df;

void init_texgen_df_shader(TextureGenerator *tex_gen) { texgen_add_shader(tex_gen, texgen_df.getName(), &texgen_df); }
