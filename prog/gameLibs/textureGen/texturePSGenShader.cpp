// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_info.h>

#include <util/dag_string.h>
#include <generic/dag_carray.h>
#include <generic/dag_smallTab.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenLogger.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/entitiesSaver.h>
#include <3d/dag_textureIDHolder.h>
#include <render/blkToConstBuffer.h>

#include <hash/sha1.h>
#include <EASTL/string.h>
#include <EASTL/array.h>
#include <EASTL/hash_map.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_overrideStates.h>
#include <drv/3d/dag_renderStates.h>
#include <shaders/dag_renderStateId.h>
// #include <EASTL/shared_ptr.h>
#include <ioSys/dag_memIo.h>
// #include "hashSaver.h"


struct ParticleStruct
{
  Point2 pos;
  Point2 scale;
  float angle;
  float brightness;
  float depth;
  // ENTITY_PROPERTIES
  Point3 entityWorldPos;
  Point3 entityForward;
  Point3 entityUp;
  int entityNameIndex;
  int entityPlaceType;
  int entitySeed;
  Point3 entityScale;
  float reserved0;
  float reserved1;
};


static int create_constants_param_block(DataBlock &params, const DataBlock &params_, TextureGenLogger &logger);

typedef char sha1_string_type[25];

struct VprogCached
{
  VPROG vprog = BAD_VPROG;
  sha1_string_type sha1 = {0};

  ~VprogCached() { close(); }

  void close()
  {
    if (vprog != BAD_VPROG)
      d3d::delete_vertex_shader(vprog);
    vprog = BAD_VPROG;
    sha1[0] = 0;
  }
};

static VprogCached common_vprog, common_particles_vprog;
static VDECL common_vdecl = BAD_VDECL;
static Sbuffer *particlesInstancesIndirect = 0;
static Sbuffer *textureParamsCB = 0;
static String premain, postmain;
static TextureIDHolder biggest_depth;
static int bigW = 0, bigH = 0;

static void create_biggest_depth(int w, int h)
{
  if (bigW >= w && bigH >= h)
    return;
  bigW = max(bigW, w);
  bigH = max(bigH, h);
  biggest_depth.close();
  biggest_depth.set(d3d::create_tex(NULL, bigW, bigH, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "texgen_depth"), "texgen_depth");
}

static constexpr int MAX_PARAMS_SIZE = 128;

class CachedShader // todo: replace with shared_ptr
{
public:
  CachedShader() {}

  ~CachedShader() { close(); }

  void close()
  {
    G_ASSERT(refCount == 0);
    d3d::delete_pixel_shader(shader);
  }

  int getRefCount() const { return refCount; }

  void release()
  {
    G_ASSERT(refCount > 0);
    if (refCount > 0)
      refCount--; // fixme: replace with atomic?
  }

  void addRef()
  {
    refCount++; // fixme: replace with atomic?
  }

  bool init(const char *shader_text, int len, TextureGenLogger &logger)
  {
    G_ASSERT(refCount == 0);
    String outErr;
    shader = d3d::create_pixel_shader_hlsl(shader_text, len, "main_ps", "ps_5_0", &outErr);
    if (!outErr.empty())
      logger.log(shader != BAD_FSHADER ? LOGLEVEL_WARN : LOGLEVEL_ERR, outErr);
    refCount = 1;
    return shader != BAD_FSHADER;
  }

  FSHADER getShader() const { return shader; }

protected:
  int refCount = 0;
  FSHADER shader = BAD_FSHADER;
  // eastl::string hash;
};

static eastl::hash_map<eastl::string, eastl::unique_ptr<CachedShader>> cached_shaders;
static void make_sha1_string(const void *buf, int len, sha1_string_type result);

static eastl::hash_map<eastl::string, sha1_string_type> cached_blk_shaders;

static void prune_cache()
{
  cached_blk_shaders.clear();
  cached_shaders.clear();
}

static void remove_unused_shaders_from_cache()
{
  for (auto it = cached_shaders.begin(); it != cached_shaders.end();)
  {
    G_ASSERT(it->second);
    if (it->second && !it->second->getRefCount())
    {
      it->second.reset(0);
      it = cached_shaders.erase(it);
    }
    else
      it++;
  }
}

static void make_sha1_string(const void *buf, int len, sha1_string_type sha1_string)
{
  sha1_context ctx;
  unsigned char sha1_output[20];
  sha1_starts(&ctx);
  sha1_update(&ctx, (const unsigned char *)buf, len);
  sha1_finish(&ctx, sha1_output);
  sha1_string[24] = sha1_string[23] = sha1_string[22] = sha1_string[21] = sha1_string[20] = 0;
  // convert to c-string.
  //  minimum C-string will have length of 20 + 20 bits encoded, i.e. 24 bytes length + trailing zero.
  uint32_t zeros = 0;
  for (int i = 0; i < 20; ++i)
  {
    if (sha1_output[i] == 0)
    {
      sha1_string[i] = i + 1;
      zeros |= (1 << i);
    }
    else
      sha1_string[i] = sha1_output[i];
  }

  if (zeros)
    for (int cz = 0, i = 0; i < 4; ++i)
    {
      uint8_t byte = zeros & (i ? 0xFF : 0xF);
      if (byte)
      {
        sha1_string[20 + cz] = byte;
        cz++;
      }
      sha1_string[20] |= byte ? (1 << i) : 0;
      zeros >>= (i ? 8 : 4);
    }
}

static CachedShader *add_shader_to_cache(const char *pixel_shader, int len, TextureGenLogger &logger, bool &was_in_cache)
{
  was_in_cache = false;
  sha1_string_type sha1_string;
  make_sha1_string(pixel_shader, len, sha1_string);

  auto it = cached_shaders.find_as(sha1_string);
  if (it != cached_shaders.end())
  {
    it->second->addRef();
    was_in_cache = true;
    return it->second.get();
  }

  CachedShader *shader = new CachedShader;
  if (!shader->init(pixel_shader, len, logger))
  {
    shader->release();
    del_it(shader);
    return NULL;
  }

  cached_shaders[((char *)sha1_string)].reset(shader);
  return shader;
}


class PSGenShader : public TextureGenShader
{
  shaders::RenderStateId renderState;
  eastl::array<eastl::array<shaders::UniqueOverrideStateId, BLENDS_COUNT>, 4> blendTypes;
  d3d::SamplerHandle wrapSampler;
  d3d::SamplerHandle clampSampler;

public:
  struct ShaderCode
  {
    PROGRAM prog;
    PROGRAM particlesProg;
    CachedShader *shader;

    void reset()
    {
      shader = 0;
      prog = BAD_PROGRAM;
    }

    ShaderCode() : shader(0), prog(BAD_PROGRAM), particlesProg(BAD_PROGRAM) {}
    ShaderCode &operator=(ShaderCode &&) = default;

    ~ShaderCode() { close(); }

    void close()
    {
      if (shader)
        shader->release();
      shader = NULL;
      // d3d::delete_program(prog); - autodeleted with fshader
      prog = particlesProg = BAD_PROGRAM;
    }
  } defShader;

  virtual ~PSGenShader() {}

  PSGenShader(const char *name_) : name(name_), totalParamsSize(0)
  {
    for (int i = 0; i < blendTypes[0].size(); ++i)
    {
      for (int j = 0; j < 2; ++j)
      {
        shaders::OverrideState state;
        if (j)
          state.set(shaders::OverrideState::SCISSOR_ENABLED);
        set_blending((BlendingType)i, state);
        blendTypes[0 + 2 * j][i] = shaders::overrides::create(state);
        state.set(shaders::OverrideState::Z_TEST_DISABLE);
        state.set(shaders::OverrideState::Z_WRITE_DISABLE);
        blendTypes[1 + 2 * j][i] = shaders::overrides::create(state);
      }
    }

    shaders::RenderState state;
    state.independentBlendEnabled = 0;
    for (auto &blendParam : state.blendParams)
    {
      blendParam.ablend = 0;
      blendParam.sepablend = 0;
    }
    state.ztest = 0;
    state.zwrite = 0;
    state.cull = CULL_NONE;
    state.colorWr = WRITEMASK_ALL;
    renderState = shaders::render_states::create(state);
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    clampSampler = d3d::request_sampler(smpInfo);
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
    wrapSampler = d3d::request_sampler(smpInfo);
  }

  void initConstants(const DataBlock &params_, TextureGenLogger &logger)
  {
    totalParamsSize = create_constants_param_block(params, params_, logger);
    logger.log(LOGLEVEL_REMARK, String(128, "shader <%s> has %d", name, totalParamsSize));
  }

  String enumerateLines(const char *s)
  {
    String res;
    int num = 1;
    int len = int(strlen(s));
    res.reserve(int(len * 1.1f));
    res.setStr("   1 ");
    for (int i = 0; i < len; i++)
    {
      if (s[i] == '\n')
      {
        num++;
        res.aprintf(16, "\n%4d ", num);
      }
      else
        res += s[i];
    }

    return res;
  }

  bool linkShader(const char *shaderPostCode, ShaderCode &code, const DataBlock *subst_blk, TextureGenLogger &logger)
  {
    String pixel_shader(256, "%s\n%s\n%s%s\n%s", shaderPreMain.str(), premain.str(), shaderCode.str(), shaderPostCode, postmain.str());
    if (subst_blk)
      for (int i = 0; i < subst_blk->paramCount(); i++)
        if (subst_blk->getParamType(i) == DataBlock::TYPE_STRING)
        {
          String from(64, "[[%s]]", subst_blk->getParamName(i));
          pixel_shader.replaceAll(from, subst_blk->getStr(i));
        }

    bool was_in_cache = false;
    code.shader = add_shader_to_cache(pixel_shader.str(), pixel_shader.length(), logger, was_in_cache);
    if (code.shader)
    {
      code.prog = d3d::create_program(common_vprog.vprog, code.shader->getShader(), common_vdecl);
      code.particlesProg = d3d::create_program(common_particles_vprog.vprog, code.shader->getShader(), common_vdecl);
    }
    else
    {
      logger.log(LOGLEVEL_ERR,
        String(128, "can not compile shader name<%s> shader:=====\n%s\n======\n>", name, enumerateLines(pixel_shader).str()));
    }

    return code.prog != BAD_PROGRAM;
  }

  bool initShader(const char *shader_pre_main, const char *premain_, const char *shader_code, const char *shaderPostCode,
    const char *postmain_, const DataBlock *subst_blk, int inputs, int outputs, int defSubSize, TextureGenLogger &logger)
  {
    defMaxSubStepSize = defSubSize;
    shaderPreMain = shader_pre_main;
    premain = premain_;
    shaderCode = shader_code;
    postmain = postmain_;
    linkShader(shaderPostCode, defShader, subst_blk, logger);
    regs[TSHADER_REG_TYPE_INPUT] = inputs;
    regs[TSHADER_REG_TYPE_OUTPUT] = outputs;
    return defShader.prog != BAD_PROGRAM;
  }

  virtual const char *getName() const { return name.str(); }
  virtual int getInputParametersCount() const { return params.paramCount(); }
  virtual bool isInputParameterOptional(int) const { return true; }

  virtual int getRegCount(TShaderRegType tp) const { return regs[tp]; }
  virtual bool isRegOptional(TShaderRegType, int) const { return true; }

  static void set_blending(BlendingType tp, shaders::OverrideState &state)
  {
    switch (tp)
    {
      case MAX_BLENDING:
      case MIN_BLENDING:
      case ADD_BLENDING:
      case MUL_BLENDING:
      case ALPHA_BLENDING:
      {
        state.set(shaders::OverrideState::BLEND_OP);
        state.set(shaders::OverrideState::BLEND_SRC_DEST);
      }
      break;
      default: break;
    }
    switch (tp)
    {
      case MAX_BLENDING:
        state.blendOp = BLENDOP_MAX;
        state.sblend = BLEND_ONE;
        state.dblend = BLEND_ONE;
        break;
      case MIN_BLENDING:
        state.blendOp = BLENDOP_MIN;
        state.sblend = BLEND_ONE;
        state.dblend = BLEND_ONE;
        break;
      case ADD_BLENDING:
        state.blendOp = BLENDOP_ADD;
        state.sblend = BLEND_ONE;
        state.dblend = BLEND_ONE;
        break;
      case MUL_BLENDING:
        state.blendOp = BLENDOP_ADD;
        state.sblend = BLEND_DESTCOLOR;
        state.dblend = BLEND_ZERO;
        break;
      case ALPHA_BLENDING:
        state.blendOp = BLENDOP_ADD;
        state.sblend = BLEND_SRCALPHA;
        state.dblend = BLEND_INVSRCALPHA;
        break;
      default: break;
    }
  }

  unsigned getMaxSubStepSize(const TextureGenNodeData &data) const { return data.params.getInt("maxSubStepSize", defMaxSubStepSize); }

  static unsigned steps_w(int w, int maxSubStepSize) { return ((w + maxSubStepSize - 1) / maxSubStepSize); }

  unsigned subSteps(const TextureGenNodeData &data) const override
  {
    for (int i = 0; i < data.outputs.size(); ++i)
      if (data.outputs[i] && data.outputs[i]->getType() == D3DResourceType::SBUF)
        return 1;

    int maxSubStepSize = getMaxSubStepSize(data);
    return steps_w(data.nodeW, maxSubStepSize) * steps_w(data.nodeH, maxSubStepSize);
  }

  void getSubStepXY(int &stepX, int &stepY, int &stepW, int &stepH, int subStep, const TextureGenNodeData &data) const
  {
    int maxSubStepSize = getMaxSubStepSize(data);
    int stepsW = steps_w(data.nodeW, maxSubStepSize);
    stepX = (subStep % stepsW) * maxSubStepSize;
    stepY = (subStep / stepsW) * maxSubStepSize;
    stepW = min<int>(maxSubStepSize, data.nodeW - stepX);
    stepH = min<int>(maxSubStepSize, data.nodeH - stepY);
  }

  virtual bool processAll(TextureGenerator &texGen, TextureRegManager &r, const TextureGenNodeData &data) override
  {
    return process(texGen, r, data, -1);
  }

  virtual bool process(TextureGenerator &texGen, TextureRegManager &regMananger, const TextureGenNodeData &data, int subStep) override
  {
    int w = data.nodeW, h = data.nodeH;
    BlendingType blend = data.blending;
    bool depth = data.use_depth;
    const DataBlock &params_override = data.params;
    dag::ConstSpan<TextureInput> inputs = data.inputs;
    dag::ConstSpan<D3dResource *> outputs = data.outputs;
    Sbuffer *particles = data.particles;

    if (particles && regMananger.getEntitiesSaver())
    {
      G_ASSERT(sizeof(ParticleStruct) == particles->getElementSize());
      ParticleStruct *buf = nullptr;
      if (!particles->lockEx(0, particles->getElementSize() * particles->getNumElements(), (void **)&buf, VBLOCK_READONLY))
        return false;

      Tab<TextureGenEntity> entities;
      for (int i = 0; i < particles->getNumElements(); i++)
      {
        TextureGenEntity ent;
        memset(&ent, 0, sizeof(ent));

        ent.worldPos = buf[i].entityWorldPos;
        ent.forward = buf[i].entityForward;
        ent.up = buf[i].entityUp;
        ent.nameIndex = buf[i].entityNameIndex;
        ent.placeType = buf[i].entityPlaceType;
        ent.seed = buf[i].entitySeed;
        ent.scale = buf[i].entityScale;
        if (ent.up.lengthSq() > 0 && ent.forward.lengthSq() > 0)
          entities.push_back(ent);
      }

      particles->unlock();

      const char *names = data.params.getStr("entitiesNames", "");
      Tab<String> entityNames;
      for (;;)
      {
        const char *p = strchr(names, ',');
        if (!p)
        {
          entityNames.push_back(String(names));
          break;
        }
        else
        {
          entityNames.push_back(String(names, p - names));
          names = p + 1;
        }
      }

      if (names && *names)
      {
        const char *blkName = data.params.getStr("saveEntitiesToFile", "");
        if (!regMananger.getEntitiesSaver()->onEntitiesGenerated(entities, blkName, entityNames))
        {
          texgen_get_logger(&texGen)->log(LOGLEVEL_ERR, String(128, "Failed to generate entites '%s'", blkName));
          return false;
        }
      }
    }

    if (!data.outputs.size())
      return true;

    const bool scissorEnabled = subStep >= 0 && subSteps(data) != 1;
    depth = particles && depth;
    shaders::overrides::set(blendTypes[(depth ? 0 : 1) + (scissorEnabled ? 2 : 0)][blend]);
    shaders::render_states::set(renderState);

    int inputsCnt = min<int>(inputs.size(), getRegCount(TSHADER_REG_TYPE_INPUT));
    for (int i = 0; i < inputsCnt; ++i)
    {
      d3d::settex(i, inputs[i].tex);
      d3d::set_sampler(STAGE_PS, i, inputs[i].wrap ? wrapSampler : clampSampler);
    }


    if (particles)
      d3d::set_buffer(STAGE_VS, 0, particles);

    if (inputsCnt < getRegCount(TSHADER_REG_TYPE_INPUT))
    {
      for (int i = inputsCnt; i < getRegCount(TSHADER_REG_TYPE_INPUT); ++i)
      {
        if (!isRegOptional(TSHADER_REG_TYPE_INPUT, i))
        {
          texgen_get_logger(&texGen)->log(LOGLEVEL_ERR, String(128, "shader <%s> has missing #%d tex input reg, and it is mandatory"));
          return false;
        }
        else
          d3d::settex(i, 0);
      }
    }

    d3d::set_render_target();
    int outputsCnt = min<int>(outputs.size(), getRegCount(TSHADER_REG_TYPE_OUTPUT));
    bool textureSet = false;
    int particlesOut = 0, rtOut = 0;
    for (int i = 0; i < outputsCnt; ++i)
    {
      if (outputs[i] && outputs[i]->getType() == D3DResourceType::SBUF)
      {
        uint32_t *dataIndirect;
        d3d_err(particlesInstancesIndirect->lock(0, 0, (void **)&dataIndirect, VBLOCK_WRITEONLY));
        dataIndirect[0] = 6;
        dataIndirect[1] = 0;
        dataIndirect[2] = 0;
        dataIndirect[3] = 0;
        particlesInstancesIndirect->unlock();

        d3d::set_rwbuffer(STAGE_PS, 7 - particlesOut, (Sbuffer *)outputs[i]);
        d3d::set_rwbuffer(STAGE_PS, 6 - particlesOut, particlesInstancesIndirect);
        particlesOut += 2;
        continue;
      }
      rtOut++;
      if (!outputs[i])
      {
        d3d::set_render_target(rtOut - 1, 0, 0);
        continue;
      }

      Texture *tex = outputs[i]->getType() == D3DResourceType::TEX ? ((Texture *)outputs[i]) : 0;
      if (!tex)
      {
        d3d::set_render_target(rtOut - 1, 0, 0);
        continue;
      }
      TextureInfo tinfo;
      tex->getinfo(tinfo, 0);
      if (tinfo.cflg & TEXCF_RTARGET)
      {
        if (tinfo.cflg & TEXCF_UNORDERED)
          d3d::set_rwtex(STAGE_PS, 8 - rtOut, tex, 0, 0);
        else
          d3d::set_render_target(rtOut - 1, tex, 0);
        textureSet = true;
      }
    }

    if (!textureSet)
      d3d::set_render_target((Texture *)NULL, 0);

    if (depth || !textureSet)
    {
      create_biggest_depth(w, h);
      d3d::set_depth(biggest_depth.getTex2D(), DepthAccess::RW);
    }

    d3d::setview(0, 0, w, h, 0, 1);

    const char *shader_postcode = params_override.getStr("shader_postcode", NULL);
    if (shader_postcode)
    {
      auto shaderFound = shaders.find_as(shader_postcode);
      if (shaderFound != shaders.end())
      {
        d3d::set_program(shaderFound->second.prog == BAD_PROGRAM
                           ? (particles ? defShader.particlesProg : defShader.prog)
                           : (particles ? shaderFound->second.particlesProg : shaderFound->second.prog));
      }
      else
      {
        ShaderCode code;
        if (linkShader(shader_postcode, code, params_override.getBlockByName("substitutions"), *texgen_get_logger(&texGen)))
        {
          shaders[shader_postcode] = eastl::move(code);
          d3d::set_program(particles ? code.particlesProg : code.prog);
          code.reset();
        }
        else
        {
          shaders[shader_postcode] = eastl::move(ShaderCode());
          texgen_get_logger(&texGen)->log(LOGLEVEL_ERR, String(128, "can not link shader name<%s> post<%s>", name, shader_postcode));
          return false;
        }
      }
    }
    else
      d3d::set_program(particles ? defShader.particlesProg : defShader.prog);

    if (particles && depth && subStep <= 0)
    {
      d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
    }
    d3d::set_vs_const1(0, 1.0, 0, 0, 0); // fixme: remove
    uint32_t *dwraps = NULL;
    d3d_err(textureParamsCB->lock(0, 0, (void **)&dwraps, VBLOCK_WRITEONLY | VBLOCK_DISCARD));
    for (int i = 0; i < inputs.size(); ++i)
      dwraps[i] = inputs[i].wrap ? 1 : 0;
    textureParamsCB->unlock();

    d3d::set_const_buffer(STAGE_PS, 1, textureParamsCB);

    Tab<Point4> constBuffer;
    create_constant_buffer(params, params_override, constBuffer);
    if (constBuffer.size())
      d3d::set_ps_const(0, &constBuffer[0].x, constBuffer.size());

    if (scissorEnabled)
    {
      int stepX, stepY, stepW, stepH;
      getSubStepXY(stepX, stepY, stepW, stepH, subStep, data);
      G_ASSERT(stepX < w && stepY < h);
      d3d::setscissor(stepX, stepY, stepW, stepH);
    }

    if (particles && particlesInstancesIndirect)
    {
      d3d::draw_indirect(PRIM_TRILIST, particlesInstancesIndirect, 0);
    }
    else
      d3d::draw(PRIM_TRILIST, 0, 1);

    for (int i = 0; i < 8; ++i)
      d3d::set_rwbuffer(STAGE_PS, i, 0);

    for (int i = 0; i < inputsCnt; ++i)
      d3d::settex(i, 0);

    if (particles)
      d3d::set_buffer(STAGE_VS, 0, 0);

    d3d::set_render_target();
    d3d::set_const_buffer(STAGE_PS, 1, 0);

    d3d::driver_command(Drv3dCommand::D3D_FLUSH);

    shaders::overrides::reset();

    return true;
  }

protected:
  String name;
  String shaderPreMain, premain, shaderCode, postmain;
  eastl::array<int, TSHADER_REG_TYPE_CNT> regs;
  DataBlock params;
  int totalParamsSize;
  int defMaxSubStepSize = 4096;
  eastl::hash_map<eastl::string, ShaderCode> shaders;
};

bool init_pixel_shader_texgen()
{
  prune_cache();
  del_d3dres(textureParamsCB);
  textureParamsCB = d3d::buffers::create_persistent_cb(16, "textureParamsCB");

  del_d3dres(particlesInstancesIndirect);
  particlesInstancesIndirect = d3d::create_sbuffer(4, 4, SBCF_UA_INDIRECT, 0, "particlesInstancesIndirect");
  if (particlesInstancesIndirect)
  {
    uint32_t *data;
    d3d_err(particlesInstancesIndirect->lock(0, 0, (void **)&data, VBLOCK_WRITEONLY));
    data[0] = 6;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    particlesInstancesIndirect->unlock();
  }

  d3d::delete_vdecl(common_vdecl);
  static VSDTYPE vsd[] = {VSD_STREAM(0), VSD_END};
  common_vdecl = d3d::create_vdecl(vsd);

  return particlesInstancesIndirect != NULL;
}

static void init_cached_vprog(VprogCached &vprog, const char *shader)
{
  int len = strlen(shader);
  sha1_string_type sha1;
  make_sha1_string(shader, len, sha1);
  if (memcmp(vprog.sha1, sha1, sizeof(sha1)) == 0)
    return;
  vprog.vprog = d3d::create_vertex_shader_hlsl(shader, len, "main_vs", "vs_5_0");
  memcpy(vprog.sha1, sha1, sizeof(sha1));
}

bool load_pixel_shader_texgen(const DataBlock &shaders)
{
  if (common_vdecl == BAD_VDECL)
    if (!init_pixel_shader_texgen())
      return false;

  if (!shaders.getStr("vprog"))
  {
    logerr("no vprog for texture gen!");
    return false;
  }
  init_cached_vprog(common_vprog, shaders.getStr("vprog"));
  // d3d::delete_vertex_shader(common_particles_vprog);
  init_cached_vprog(common_particles_vprog, shaders.getStr("particles_vprog"));

  if (common_vprog.vprog == BAD_VPROG)
  {
    logerr("can't create vprog <%s>for texture gen!", shaders.getStr("vprog"));
    return false;
  }

  premain = shaders.getStr("premain", "");
  postmain = shaders.getStr("postmain", "");

  return true;
}

bool add_pixel_shader_texgen(const DataBlock &shaders, TextureGenerator *texGen)
{
  if (common_vprog.vprog == BAD_VPROG)
    return false;

  DynamicMemGeneralSaveCB mem(tmpmem, 2048);
  String resultShader;
  for (int i = 0; i < shaders.blockCount(); ++i)
  {
    const DataBlock *shaderBlock = shaders.getBlock(i);
    const char *shaderName = shaderBlock->getBlockName();

    // check if shader exist in cache
    mem.resize(0);
    shaderBlock->saveToStream(mem);
    sha1_string_type sha1;
    make_sha1_string(mem.data(), mem.size(), sha1);

    auto cachedShaderIt = cached_blk_shaders.find_as(shaderName);
    if (cachedShaderIt != cached_blk_shaders.end() && memcmp(cachedShaderIt->second, sha1, sizeof(sha1)) == 0 &&
        texgen_get_shader(texGen, shaderName))
    {
      // debug("shader <%s> already added", shaderName);
      continue; // already in cache and already exist in texgen
    }


    const char *shaderCode = shaderBlock->getStr("shaderCode", NULL);
    const char *fullShaderCode = shaderBlock->getStr("fullShaderCode", NULL);
    const char *shaderPostCode = shaderBlock->getBlockByNameEx("params")->getStr("shader_postcode", "");
    const char *shaderPreMain = shaderBlock->getStr("shaderPreMain", "");
    const DataBlock *substBlk = shaderBlock->getBlockByName("substitutions");
    if (!shaderCode && !fullShaderCode)
    {
      texgen_get_logger(texGen)->log(LOGLEVEL_ERR, String(128, "shader <%s> has no shader code", shaderName));
      continue;
    }

    PSGenShader *shader = new PSGenShader(shaderName);

    if (fullShaderCode)
    {
      shaderPreMain = fullShaderCode;
      premain = shaderCode = shaderPostCode = postmain = "";
    }

    if (!shader->initShader(shaderPreMain, premain, shaderCode, shaderPostCode, postmain, substBlk, shaderBlock->getInt("inputs", 0),
          shaderBlock->getInt("outputs", 1), shaderBlock->getInt("defMaxSubSize", 4096), *texgen_get_logger(texGen)))
    {
      texgen_get_logger(texGen)->log(LOGLEVEL_ERR, String(128, "can not init shader <%s>", shaderName));
      del_it(shader);
      continue;
    }

    if (shaderBlock->getBlockByName("params"))
      shader->initConstants(*shaderBlock->getBlockByName("params"), *texgen_get_logger(texGen));
    texgen_add_shader(texGen, shaderName, shader);
    memcpy(cached_blk_shaders[shaderName], sha1, sizeof(sha1));
  }

  remove_unused_shaders_from_cache();

  return true;
}

void close_pixel_shader_texgen()
{
  d3d::delete_vdecl(common_vdecl);
  common_vdecl = BAD_VDECL;
  common_vprog.close();
  common_particles_vprog.close();

  del_d3dres(particlesInstancesIndirect);
  particlesInstancesIndirect = NULL;

  del_d3dres(textureParamsCB);
  textureParamsCB = NULL;
  clear_and_shrink(premain);
  clear_and_shrink(postmain);
  biggest_depth.close();
  prune_cache();
}


static int create_constants_param_block(DataBlock &params, const DataBlock &params_, TextureGenLogger &logger)
{
  params.reset();
  int paramsSize = 0;
  for (int i = 0; i < params_.paramCount(); ++i)
  {
    if (params_.getParamType(i) == params_.TYPE_STRING)
      continue;

    const char *name = params_.getParamName(i);
    int typeSize = getDataBlockTypeSize(params_.getParamType(i));
    if ((paramsSize + typeSize - 1) / 4 != paramsSize / 4)
      paramsSize = (paramsSize & (~3)) + 4;

    logger.log(LOGLEVEL_REMARK, String(128, "param <%s> starts at %d size %d", name, paramsSize, typeSize));
    paramsSize += typeSize;
    switch (params_.getParamType(i))
    {
      case params_.TYPE_INT: params.addInt(name, params_.getInt(i)); break;
      case params_.TYPE_REAL: params.addReal(name, params_.getReal(i)); break;
      case params_.TYPE_IPOINT2: params.addIPoint2(name, params_.getIPoint2(i)); break;
      case params_.TYPE_POINT2: params.addPoint2(name, params_.getPoint2(i)); break;
      case params_.TYPE_IPOINT3: params.addIPoint3(name, params_.getIPoint3(i)); break;
      case params_.TYPE_IPOINT4: params.addIPoint4(name, params_.getIPoint4(i)); break;
      case params_.TYPE_POINT3: params.addPoint3(name, params_.getPoint3(i)); break;
      case params_.TYPE_POINT4: params.addPoint4(name, params_.getPoint4(i)); break;
      case params_.TYPE_BOOL: params.addBool(name, params_.getBool(i)); break;
      case params_.TYPE_E3DCOLOR: params.addE3dcolor(name, params_.getE3dcolor(i)); break;
    }
  }

  paramsSize = (paramsSize + 3) & (~3);
  G_ASSERT(paramsSize <= MAX_PARAMS_SIZE);

  return min(paramsSize, (int)MAX_PARAMS_SIZE);
}
