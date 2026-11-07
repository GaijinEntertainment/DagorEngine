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

#include <util/dag_finally.h>
#include <util/dag_string.h>
#include <util/dag_strUtil.h>
#include <generic/dag_carray.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_DObject.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenLogger.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/entitiesSaver.h>
#include <3d/dag_resPtr.h>
#include <render/blkToConstBuffer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shBindumps.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <textureGen/textureGenDshl.h>
#include <debug/dag_log.h>

#include <hash/sha1.h>
#include <EASTL/string.h>
#include <EASTL/array.h>
#include <EASTL/hash_map.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_overrideStates.h>
#include <generic/dag_span.h>
#include <drv/3d/dag_renderStates.h>
#include <shaders/dag_renderStateId.h>
// #include <EASTL/shared_ptr.h>
#include <ioSys/dag_memIo.h>
#include <ctype.h>

namespace
{
TextureGenCommandSync texgen_command_sync = TextureGenCommandSync::FlushAndWait;
int texgen_max_tile_size = 0;
} // namespace

void texgen_set_command_sync(TextureGenCommandSync mode) { texgen_command_sync = mode; }
// Tiny tiles make millions of commands per node.
void texgen_set_max_tile_size(int size) { texgen_max_tile_size = size > 0 ? max(size, 64) : 0; }
int texgen_get_max_tile_size() { return texgen_max_tile_size; }


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

typedef char sha1_string_type[20 * 2 + 1];

struct VprogCached
{
  VPROG vprog = BAD_VPROG;

  ~VprogCached() { close(); }

  void close()
  {
    if (vprog != BAD_VPROG)
      d3d::delete_vertex_shader(vprog);
    vprog = BAD_VPROG;
  }
};

static VprogCached common_vprog, common_particles_vprog;
static VDECL common_vdecl = BAD_VDECL;
static Sbuffer *textureParamsCB = 0;
// Node params for dsc2-compiled (dshl) variants are bound at b3 via set_const_buffer; the legacy
// set_ps_const(0)->b0 path is not honored for a set_program-bound dsc2 element. Sized well above any
// node's packed-param count (the old set_ps_const path capped near MAX_PS_CONSTS ~272).
static constexpr int TEXGEN_NODE_PARAMS_CB_REGS = 512;
static Sbuffer *textureNodeParamsCB = 0;
static String premain, postmain;
static UniqueTex biggest_depth;
static int bigW = 0, bigH = 0;

static void create_biggest_depth(int w, int h)
{
  if (bigW >= w && bigH >= h)
    return;
  bigW = max(bigW, w);
  bigH = max(bigH, h);
  biggest_depth = dag::create_tex(NULL, bigW, bigH, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "texgen_depth", RESTAG_TEXGEN);
}

static constexpr int MAX_PARAMS_SIZE = 128;

static bool is_param_read_by_cpp(const char *name)
{
  for (const char *cppParam : {"maxSubStepSize", "entitiesNames", "saveEntitiesToFile", "shader_postcode"})
    if (strcmp(cppParam, name) == 0)
      return true;
  return false;
}

static bool is_ident_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

static const char *skip_blanks(const char *p)
{
  while (*p == ' ' || *p == '\t')
    p++;
  return p;
}

static void blank_comments(String &text)
{
  for (char *p = text.data(); *p; p++)
  {
    if (p[0] != '/' || (p[1] != '/' && p[1] != '*'))
      continue;
    const char *end = p[1] == '/' ? strchr(p, '\n') : strstr(p + 2, "*/");
    char *stop = end ? (char *)end + (p[1] == '*' ? 2 : 0) : p + strlen(p);
    memset(p, ' ', stop - p);
    p = stop - 1;
  }
}

static bool param_is_only_copied(const char *text, const char *name)
{
  const int len = (int)strlen(name);
  bool copied = false;
  for (const char *p = strstr(text, name); p; p = strstr(p + len, name))
  {
    if ((p > text && is_ident_char(p[-1])) || is_ident_char(p[len]))
      continue;
    const char *next = skip_blanks(p + len);
    const char *prev = p;
    while (prev > text && (prev[-1] == ' ' || prev[-1] == '\t'))
      prev--;
    const char *word = prev;
    while (word > text && is_ident_char(word[-1]))
      word--;
    if (*next == ';' && word < prev && !(prev - word == 6 && strncmp(word, "return", 6) == 0))
      continue;
    if (prev > text && prev[-1] == '.' && *next == '=' && next[1] != '=')
    {
      const char *src = skip_blanks(next + 1);
      if (strncmp(src, name, len) == 0 && *skip_blanks(src + len) == ';')
      {
        copied = true;
        p = src;
      }
      continue;
    }
    return false;
  }
  return copied;
}

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
#if _TARGET_PC_WIN
    shader = d3d::create_pixel_shader_hlsl(shader_text, len, "main_ps", "ps_5_0", &outErr);
#else
    shader = BAD_FSHADER, outErr = "hlsl n/a";
    G_UNUSED(shader_text), G_UNUSED(len);
#endif
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

static eastl::hash_map<eastl::string, sha1_string_type> cached_blk_shaders;

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
  data_to_str_hex_buf(sha1_string, sizeof(sha1_string_type), sha1_output, sizeof(sha1_output));
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
      prog = particlesProg = BAD_PROGRAM;
    }

    ShaderCode() : shader(0), prog(BAD_PROGRAM), particlesProg(BAD_PROGRAM) {}
    ShaderCode &operator=(ShaderCode &&) = default;

    ~ShaderCode() { close(); }

    void close()
    {
      if (prog != BAD_PROGRAM)
        d3d::delete_program(prog);
      if (particlesProg != BAD_PROGRAM)
        d3d::delete_program(particlesProg);
      if (shader)
        shader->release();
      reset();
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

  bool initConstants(const DataBlock &params_, TextureGenLogger &logger)
  {
    totalParamsSize = create_constants_param_block(params, params_, logger);
    logger.log(LOGLEVEL_REMARK, String(128, "shader <%s> has %d", name, totalParamsSize));
    if (totalParamsSize <= MAX_PARAMS_SIZE)
      return true;
    logger.log(LOGLEVEL_ERR,
      String(128, "shader <%s>: the params take %d floats, the limit is %d", name, totalParamsSize, MAX_PARAMS_SIZE));
    return false;
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
      if (common_particles_vprog.vprog != BAD_VPROG)
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
    findUnreadParams(shaderPostCode, subst_blk);
    linkShader(shaderPostCode, defShader, subst_blk, logger);
    regs[TSHADER_REG_TYPE_INPUT] = inputs;
    regs[TSHADER_REG_TYPE_OUTPUT] = outputs;
    return defShader.prog != BAD_PROGRAM;
  }

  void findUnreadParams(const char *post_code, const DataBlock *subst_blk)
  {
    String text(0, "%s\n%s\n%s", shaderPreMain.str(), shaderCode.str(), post_code);
    for (int i = 0; subst_blk && i < subst_blk->paramCount(); i++)
      if (subst_blk->getParamType(i) == DataBlock::TYPE_STRING)
        text.aprintf(0, "\n%s", subst_blk->getStr(i));
    blank_comments(text);
    unreadParams = params;
    for (int i = 0; i < params.paramCount(); i++)
      if (!param_is_only_copied(text, params.getParamName(i)))
        unreadParams.removeParam(params.getParamName(i));
  }

  void logUnappliedParams(TextureGenLogger &logger, const TextureGenNodeData &data) const
  {
    for (int i = 0; i < data.params.paramCount(); i++)
    {
      const char *param = data.params.getParamName(i);
      if (params.findParam(param) < 0 && !is_param_read_by_cpp(param))
        logger.log(LOGLEVEL_WARN,
          String(0, "%s node <%s>: the shader has no param '%s', so its value is not applied", name.str(), data.nodeName, param));
    }
    if (!unreadParams.paramCount() || data.params.getStr("shader_postcode", nullptr))
      return;
    Tab<NameOffset> offsets;
    create_constant_buffer_offsets(offsets, unreadParams, 0);
    Tab<Point4> applied, declared;
    create_constant_buffer(unreadParams, data.params, applied);
    create_constant_buffer(unreadParams, declared);
    for (int i = 0; i < offsets.size(); i++)
      if (memcmp(&applied[0].x + offsets[i].ofs, &declared[0].x + offsets[i].ofs,
            getDataBlockTypeSize(unreadParams.getParamType(i)) * sizeof(float)) != 0)
        logger.log(LOGLEVEL_WARN, String(0, "%s node <%s>: the shader does not read its param '%s', so its value is not applied",
                                    name.str(), data.nodeName, offsets[i].name));
  }

  // Configure this shader to render a precompiled dshl bindump variant (Stage-3 dshl pipeline).
  // Element creation is deferred to the first process() (needs the GPU lock + the loaded bindump).
  bool initDshl(ShaderBindumpHandle bindump, const char *variant_name, const DataBlock *params_blk, int inputs, int outputs,
    int def_sub_size, TextureGenLogger &logger)
  {
    dshlBindump = bindump;
    dshlVariant = variant_name;
    defMaxSubStepSize = def_sub_size;
    regs[TSHADER_REG_TYPE_INPUT] = inputs;
    regs[TSHADER_REG_TYPE_OUTPUT] = outputs;
    return !params_blk || initConstants(*params_blk, logger);
  }

  void ensureDshlElem(TextureGenLogger &logger)
  {
    if (dshlElem || dshlBindump == INVALID_BINDUMP_HANDLE)
      return;
    // _optional variant: a stale/mismatched bindump degrades to null (logged below), never DAG_FATAL.
    dshlMat = new_shader_material_by_name_optional(dshlBindump, dshlVariant.c_str());
    dshlElem = dshlMat ? dshlMat->make_elem() : nullptr;
    if (!dshlElem)
      logger.log(LOGLEVEL_ERR, String(128, "texgen dshl: variant <%s> not found in bindump", dshlVariant.c_str()));
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

  unsigned getMaxSubStepSize(const TextureGenNodeData &data) const
  {
    const int size = data.params.getInt("maxSubStepSize", defMaxSubStepSize);
    return texgen_max_tile_size > 0 && !data.particles ? min(size, texgen_max_tile_size) : size;
  }

  unsigned subSteps(const TextureGenNodeData &data) const override
  {
    for (int i = 0; i < data.outputs.size(); ++i)
      if (data.outputs[i] && data.outputs[i]->getType() == D3DResourceType::SBUF)
        return 1;

    int maxSubStepSize = getMaxSubStepSize(data);
    return texgen_tiles_per_side(data.nodeW, maxSubStepSize) * texgen_tiles_per_side(data.nodeH, maxSubStepSize);
  }

  void getSubStepXY(int &stepX, int &stepY, int &stepW, int &stepH, int subStep, const TextureGenNodeData &data) const
  {
    texgen_get_tile(data.nodeW, data.nodeH, getMaxSubStepSize(data), subStep, stepX, stepY, stepW, stepH);
  }

  int subStepPixels(const TextureGenNodeData &data, int sub_step) const override
  {
    if (subSteps(data) == 1)
    {
      return data.nodeW * data.nodeH;
    }
    int stepX = 0, stepY = 0, stepW = 0, stepH = 0;
    getSubStepXY(stepX, stepY, stepW, stepH, sub_step, data);
    return stepW * stepH;
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
    Sbuffer *particlesArgs = particles ? regMananger.getParticlesArgs(particles) : nullptr;
    G_ASSERT_RETURN(!particles || particlesArgs, false);
    if (subStep <= 0 && *data.nodeName)
      logUnappliedParams(*texgen_get_logger(&texGen), data);

    const char *names = particles && regMananger.getEntitiesSaver() ? data.params.getStr("entitiesNames", "") : "";
    if (*names)
    {
      G_ASSERT(sizeof(ParticleStruct) == particles->getElementSize());
      uint32_t *args = nullptr;
      if (!particlesArgs->lock(0, 0, (void **)&args, VBLOCK_READONLY))
        return false;
      const int count = min<uint32_t>(args[1], particles->getNumElements());
      particlesArgs->unlock();

      ParticleStruct *buf = nullptr;
      // a lock of size 0 locks the whole buffer
      if (count > 0 && !particles->lockEx(0, particles->getElementSize() * count, (void **)&buf, VBLOCK_READONLY))
        return false;

      Tab<TextureGenEntity> entities;
      for (int i = 0; i < count; i++)
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

      if (buf)
        particles->unlock();

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
    FINALLY([&] {
      for (int i = 0; i < 8; ++i)
        d3d::set_rwbuffer(STAGE_PS, i, 0);
      for (int i = 0; i < inputsCnt; ++i)
        d3d::settex(i, 0);
      if (particles)
        d3d::set_buffer(STAGE_VS, 0, 0);
      d3d::set_render_target();
      d3d::set_const_buffer(STAGE_PS, 1, 0);
      d3d::set_const_buffer(STAGE_PS, 3, 0);
      shaders::overrides::reset();
    });
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
          texgen_get_logger(&texGen)->log(LOGLEVEL_ERR,
            String(128, "shader <%s> has missing #%d tex input reg, and it is mandatory", name.str(), i));
          return false;
        }
        else
          d3d::settex(i, 0);
      }
    }

    int outputsCnt = min<int>(outputs.size(), getRegCount(TSHADER_REG_TYPE_OUTPUT));
    eastl::array<RenderTarget, Driver3dRenderTarget::MAX_SIMRT> outRts = {};
    bool textureSet = false;
    int particlesOut = 0, rtOut = 0;
    for (int i = 0; i < outputsCnt; ++i)
    {
      if (outputs[i] && outputs[i]->getType() == D3DResourceType::SBUF)
      {
        Sbuffer *args = regMananger.getParticlesArgs(outputs[i]);
        G_ASSERT_RETURN(args, false);
        uint32_t *dataIndirect = nullptr;
        d3d_err(args->lock(0, 0, (void **)&dataIndirect, VBLOCK_WRITEONLY));
        if (!dataIndirect)
          return false;
        dataIndirect[0] = 6;
        dataIndirect[1] = 0;
        dataIndirect[2] = 0;
        dataIndirect[3] = 0;
        args->unlock();

        d3d::set_rwbuffer(STAGE_PS, 7 - particlesOut, (Sbuffer *)outputs[i]);
        d3d::set_rwbuffer(STAGE_PS, 6 - particlesOut, args);
        particlesOut += 2;
        continue;
      }
      rtOut++;
      if (!outputs[i])
        continue;

      Texture *tex = outputs[i]->getType() == D3DResourceType::TEX ? ((Texture *)outputs[i]) : 0;
      if (!tex)
        continue;
      TextureInfo tinfo;
      tex->getinfo(tinfo, 0);
      if (tinfo.cflg & TEXCF_RTARGET)
      {
        if (tinfo.cflg & TEXCF_UNORDERED)
          d3d::set_rwtex(STAGE_PS, 8 - rtOut, tex, 0, 0);
        else
          outRts[rtOut - 1] = {tex, 0, 0};
        textureSet = true;
      }
    }

    BaseTexture *depthTex = nullptr;
    if (depth || !textureSet)
    {
      create_biggest_depth(w, h);
      depthTex = biggest_depth.getTex2D();
    }
    d3d::set_render_target({depthTex, 0, 0}, DepthAccess::RW, make_span_const(outRts.data(), rtOut));

    d3d::setview(0, 0, w, h, 0, 1);

    // dshl pipeline: the per-node variant is a precompiled bindump shader (VS+PS). Bind ONLY its program
    // (exactly like the old raw d3d::create_program path) via get_dynamic_variant_states + set_program.
    // We deliberately do NOT call ShaderElement::setStates(): that runs the shader-block conformity check
    // which, in texgen's frame/scene block context, auto-sets the environment's default object block
    // (gui_aces_object) and floods the log. texgen owns all render state (overrides above) + the raw
    // texture/const/UAV binding below; the variant declares no shadervars, so nothing else is needed.
    if (dshlBindump != INVALID_BINDUMP_HANDLE)
    {
      ensureDshlElem(*texgen_get_logger(&texGen));
      bool setProgram = false;
      if (dshlElem)
      {
        shaders::CombinedDynVariantState dynVarState = get_dynamic_variant_states(dshlElem->native());
        if (is_valid(dynVarState) && dynVarState.program != BAD_PROGRAM)
        {
          d3d::set_program(dynVarState.program);
          setProgram = true;
        }
      }
      if (!setProgram)
      {
        texgen_get_logger(&texGen)->log(LOGLEVEL_ERR, String(128, "texgen dshl: no program for variant <%s>", dshlVariant.c_str()));
        return false;
      }
    }
    else
    {
      PROGRAM program = particles ? defShader.particlesProg : defShader.prog;
      const char *shader_postcode = params_override.getStr("shader_postcode", NULL);
      if (shader_postcode)
      {
        auto shaderFound = shaders.find_as(shader_postcode);
        if (shaderFound != shaders.end())
        {
          if (shaderFound->second.prog != BAD_PROGRAM)
            program = particles ? shaderFound->second.particlesProg : shaderFound->second.prog;
        }
        else
        {
          ShaderCode code;
          if (linkShader(shader_postcode, code, params_override.getBlockByName("substitutions"), *texgen_get_logger(&texGen)))
          {
            program = particles ? code.particlesProg : code.prog;
            shaders[shader_postcode] = eastl::move(code);
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
      if (program == BAD_PROGRAM)
      {
        texgen_get_logger(&texGen)->log(LOGLEVEL_ERR,
          String(128, "shader <%s> has no %sprogram", name.str(), particles ? "particles " : ""));
        return false;
      }
      d3d::set_program(program);
    }

    if (particles && depth && subStep <= 0)
    {
      d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
    }
    d3d::set_vs_const1(0, 1.0, 0, 0, 0); // fixme: remove
    uint32_t *dwraps = NULL;
    d3d_err(textureParamsCB->lock(0, 0, (void **)&dwraps, VBLOCK_WRITEONLY | VBLOCK_DISCARD));
    if (!dwraps)
    {
      // lock returns null when the device was lost (e.g. a prior node's draw hung the GPU). Bail with a
      // clean failure instead of dereferencing null, so a device-removed turns into a logged error.
      texgen_get_logger(&texGen)->log(LOGLEVEL_ERR, String(128, "texgen: textureParamsCB lock failed for <%s> (device lost?)", name));
      return false;
    }
    for (int i = 0; i < inputs.size(); ++i)
      dwraps[i] = inputs[i].wrap ? 1 : 0;
    textureParamsCB->unlock();

    d3d::set_const_buffer(STAGE_PS, 1, textureParamsCB);

    Tab<Point4> constBuffer;
    create_constant_buffer(params, params_override, constBuffer);
    if (dshlBindump != INVALID_BINDUMP_HANDLE)
    {
      // dsc2 elements are bound here via set_program only (no setStates), so legacy set_ps_const(0)
      // (which targets b0) never reaches the shader -- params would be garbage. Deliver them through a
      // dedicated cbuffer at b3 (the assembler pins `cbuffer global` to b3), the same explicit-register +
      // set_const_buffer path textureParamsCB uses at b1.
      if (constBuffer.size() && textureNodeParamsCB)
      {
        void *dst = nullptr;
        d3d_err(textureNodeParamsCB->lock(0, 0, &dst, VBLOCK_WRITEONLY | VBLOCK_DISCARD));
        if (!dst)
        {
          texgen_get_logger(&texGen)->log(LOGLEVEL_ERR,
            String(128, "texgen: node params CB lock failed for <%s> (device lost?)", name));
          return false;
        }
        const int srcBytes = (int)constBuffer.size() * (int)sizeof(Point4);
        memcpy(dst, constBuffer.data(), min(srcBytes, TEXGEN_NODE_PARAMS_CB_REGS * (int)sizeof(Point4)));
        textureNodeParamsCB->unlock();
        d3d::set_const_buffer(STAGE_PS, 3, textureNodeParamsCB);
      }
    }
    else if (constBuffer.size())
      d3d::set_ps_const(0, &constBuffer[0].x, constBuffer.size());

    if (scissorEnabled)
    {
      int stepX, stepY, stepW, stepH;
      getSubStepXY(stepX, stepY, stepW, stepH, subStep, data);
      G_ASSERT(stepX < w && stepY < h);
      d3d::setscissor(stepX, stepY, stepW, stepH);
    }

    if (particles)
    {
      d3d::draw_indirect(PRIM_TRILIST, particlesArgs, 0);
    }
    else
      d3d::draw(PRIM_TRILIST, 0, 1);

    if (texgen_command_sync == TextureGenCommandSync::FlushAndWait)
    {
      d3d::driver_command(Drv3dCommand::D3D_FLUSH);
    }

    return true;
  }

protected:
  String name;
  String shaderPreMain, premain, shaderCode, postmain;
  eastl::array<int, TSHADER_REG_TYPE_CNT> regs;
  DataBlock params, unreadParams;
  int totalParamsSize;
  int defMaxSubStepSize = 4096;
  eastl::hash_map<eastl::string, ShaderCode> shaders;

  // dshl pipeline (Stage 3): when dshlBindump is set, process() renders this precompiled bindump
  // variant via a ShaderElement instead of runtime-compiling HLSL. Created lazily on the first
  // process() (needs the GPU lock + loaded bindump); Ptr<> releases the material/element when this
  // shader is destroyed (add_dshl_shader_texgen re-registers a fresh PSGenShader per generate).
  ShaderBindumpHandle dshlBindump = INVALID_BINDUMP_HANDLE;
  eastl::string dshlVariant;
  Ptr<ShaderMaterial> dshlMat;
  Ptr<ShaderElement> dshlElem;
};

bool init_pixel_shader_texgen()
{
  remove_unused_shaders_from_cache();
  del_d3dres(textureParamsCB);
  textureParamsCB = d3d::buffers::create_persistent_cb(16, "textureParamsCB", RESTAG_TEXGEN);

  del_d3dres(textureNodeParamsCB);
  textureNodeParamsCB = d3d::buffers::create_persistent_cb(TEXGEN_NODE_PARAMS_CB_REGS, "textureNodeParamsCB", RESTAG_TEXGEN);

  // Linked HLSL programs keep this vdecl.
  if (common_vdecl == BAD_VDECL)
  {
    static VSDTYPE vsd[] = {VSD_STREAM(0), VSD_END};
    common_vdecl = d3d::create_vdecl(vsd);
  }

  return textureParamsCB != nullptr;
}

static void init_cached_vprog(VprogCached &vprog, const char *shader)
{
  if (vprog.vprog != BAD_VPROG || !shader)
    return;
#if _TARGET_PC_WIN
  vprog.vprog = d3d::create_vertex_shader_hlsl(shader, strlen(shader), "main_vs", "vs_5_0");
#else
  vprog.vprog = BAD_VPROG;
#endif
}

bool load_pixel_shader_texgen(const DataBlock &shaders)
{
  if (common_vdecl == BAD_VDECL)
    if (!init_pixel_shader_texgen())
      return false;

  const char *vprog = shaders.getStr("vprog", nullptr);
  if (!vprog)
  {
    logerr("no vprog for texture gen!");
    return false;
  }
  const char *particlesVprog = shaders.getStr("particles_vprog", nullptr);
  if (!particlesVprog && common_particles_vprog.vprog == BAD_VPROG)
    logerr("no particles_vprog for texture gen: nodes cannot draw particles");
  const VPROG oldParticlesVprog = common_particles_vprog.vprog;
  init_cached_vprog(common_vprog, vprog);
  init_cached_vprog(common_particles_vprog, particlesVprog);
  const char *newPremain = shaders.getStr("premain", ""), *newPostmain = shaders.getStr("postmain", "");
  if (strcmp(newPremain, premain) != 0 || strcmp(newPostmain, postmain) != 0 || common_particles_vprog.vprog != oldParticlesVprog)
  {
    premain = newPremain;
    postmain = newPostmain;
    cached_blk_shaders.clear();
  }

  if (common_vprog.vprog == BAD_VPROG)
  {
    logerr("can't create vprog <%s>for texture gen!", vprog);
    return false;
  }
  return true;
}

bool add_pixel_shader_texgen(const DataBlock &shaders, TextureGenerator *texGen)
{
  if (common_vprog.vprog == BAD_VPROG)
    return false;

  DynamicMemGeneralSaveCB mem(tmpmem, 2048);
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

    const char *nodePremain = premain, *nodePostmain = postmain;
    if (fullShaderCode)
    {
      shaderPreMain = fullShaderCode;
      nodePremain = shaderCode = shaderPostCode = nodePostmain = "";
    }

    const DataBlock *paramsBlk = shaderBlock->getBlockByName("params");
    if ((paramsBlk && !shader->initConstants(*paramsBlk, *texgen_get_logger(texGen))) ||
        !shader->initShader(shaderPreMain, nodePremain, shaderCode, shaderPostCode, nodePostmain, substBlk,
          shaderBlock->getInt("inputs", 0), shaderBlock->getInt("outputs", 1), shaderBlock->getInt("defMaxSubSize", 4096),
          *texgen_get_logger(texGen)))
    {
      texgen_get_logger(texGen)->log(LOGLEVEL_ERR, String(128, "can not init shader <%s>", shaderName));
      del_it(shader);
      continue;
    }
    texgen_add_shader(texGen, shaderName, shader);
    memcpy(cached_blk_shaders[shaderName], sha1, sizeof(sha1));
  }

  remove_unused_shaders_from_cache();

  return true;
}

bool add_dshl_shader_texgen(TextureGenerator *tex_gen, const char *shader_name, const char *variant_name, ShaderBindumpHandle bindump,
  const DataBlock *params_blk, int inputs, int outputs, int def_sub_size)
{
  PSGenShader *shader = new PSGenShader(shader_name);
  if (!shader->initDshl(bindump, variant_name, params_blk, inputs, outputs, def_sub_size, *texgen_get_logger(tex_gen)))
  {
    del_it(shader);
    return false;
  }
  texgen_add_shader(tex_gen, shader_name, shader);
  return true;
}

void close_pixel_shader_texgen()
{
  d3d::delete_vdecl(common_vdecl);
  common_vdecl = BAD_VDECL;
  common_vprog.close();
  common_particles_vprog.close();

  del_d3dres(textureParamsCB);
  textureParamsCB = NULL;
  del_d3dres(textureNodeParamsCB);
  textureNodeParamsCB = NULL;
  clear_and_shrink(premain);
  clear_and_shrink(postmain);
  biggest_depth.close();
  bigW = bigH = 0;
  cached_blk_shaders.clear();
  cached_shaders.clear();
}


static int create_constants_param_block(DataBlock &params, const DataBlock &params_, TextureGenLogger &logger)
{
  params.reset();
  for (int i = 0; i < params_.paramCount(); ++i)
  {
    const char *name = params_.getParamName(i);
    switch (params_.getParamType(i))
    {
      case DataBlock::TYPE_INT: params.addInt(name, params_.getInt(i)); break;
      case DataBlock::TYPE_REAL: params.addReal(name, params_.getReal(i)); break;
      case DataBlock::TYPE_IPOINT2: params.addIPoint2(name, params_.getIPoint2(i)); break;
      case DataBlock::TYPE_POINT2: params.addPoint2(name, params_.getPoint2(i)); break;
      case DataBlock::TYPE_IPOINT3: params.addIPoint3(name, params_.getIPoint3(i)); break;
      case DataBlock::TYPE_IPOINT4: params.addIPoint4(name, params_.getIPoint4(i)); break;
      case DataBlock::TYPE_POINT3: params.addPoint3(name, params_.getPoint3(i)); break;
      case DataBlock::TYPE_POINT4: params.addPoint4(name, params_.getPoint4(i)); break;
      case DataBlock::TYPE_BOOL: params.addBool(name, params_.getBool(i)); break;
      case DataBlock::TYPE_E3DCOLOR: params.addE3dcolor(name, params_.getE3dcolor(i)); break;
    }
  }

  Tab<NameOffset> offsets;
  const int paramsSize = create_constant_buffer_offsets(offsets, params, 0);
  for (int i = 0; i < offsets.size(); ++i)
    logger.log(LOGLEVEL_REMARK,
      String(128, "param <%s> starts at %d size %d", offsets[i].name, offsets[i].ofs, getDataBlockTypeSize(params.getParamType(i))));
  return paramsSize;
}
