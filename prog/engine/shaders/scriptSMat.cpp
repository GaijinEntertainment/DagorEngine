// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_PC_WIN
#undef _DEBUG_TAB_ // exec_stcode if called too frequently to allow any perfomance penalty there
#endif
#include "scriptSMat.h"
#include "scriptSElem.h"
#include "shStateBlk.h"
#include <shaders/dag_shaderDbg.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/dag_shaderMesh.h> //for ShaderMesh::STG_trans
#include <obsolete/dag_cfg.h>
#include <ioSys/dag_genIo.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/dag_texIdSet.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_info.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <shaders/dag_shMaterialUtils.h>

float max_emission = 1e10f;
static bool assert_on_vars = false;
#if DAGOR_DBGLEVEL > 0
static int max_reload_padding = 0;
#endif

#if _TARGET_PC
static bool diffuse_mipmaps_debug_enabled = false;
static TEXTUREID get_diffuse_mipmap_tex(TEXTUREID tex_id);
#else
static inline TEXTUREID get_diffuse_mipmap_tex(TEXTUREID tex_id) { return tex_id; }
#endif

const char *get_shader_class_name_by_material_name(const char *mat_name)
{
  const shaderbindump::ShaderClass *sc = shBinDumpEx(true).findShaderClass(mat_name);
  if (!sc)
    return NULL;
  return (const char *)sc->name;
}


bool shader_exists(const char *shader_name) { return shBinDump().findShaderClass(shader_name) != nullptr; }

ShaderMaterial *new_shader_material(const MaterialData &m, bool sec_dump_for_exp, bool do_log)
{
  if (m.className.empty())
  {
    if (do_log)
      logerr("NULL material in new_shader_material()");
    return NULL;
  }

  const shaderbindump::ShaderClass *sc = shBinDumpEx(!sec_dump_for_exp).findShaderClass(m.className);
  if (!sc)
  {
    if (do_log)
      logerr("Shader '%s' not found in bin dump", m.className.str());
    return NULL;
  }
  if (dd_stricmp((const char *)sc->name, m.className))
    DAG_FATAL("Shader '%s' not found", m.className.str());

  ShaderMaterialProperties *smp = ShaderMaterialProperties::create(sc, m, sec_dump_for_exp);
  ShaderMaterial *mat = ScriptedShaderMaterial::create(*smp);
  delete smp;

  return mat;
}

ShaderMaterial *new_shader_material_by_name_optional(const char *shader_name, const char *mat_script, bool sec_dump_for_exp)
{
  const shaderbindump::ShaderClass *sc = shBinDumpEx(!sec_dump_for_exp).findShaderClass(shader_name);
  if (!sc)
    return NULL;

  MaterialData m;
  m.matScript = mat_script;
  ShaderMaterialProperties *smp = ShaderMaterialProperties::create(sc, m, sec_dump_for_exp);
  ShaderMaterial *mat = ScriptedShaderMaterial::create(*smp);
  delete smp;

  return mat;
}
ShaderMaterial *new_shader_material_by_name(const char *shader_name, const char *mat_script, bool sec_dump)
{
  ShaderMaterial *sm = new_shader_material_by_name_optional(shader_name, mat_script, sec_dump);
  if (!sm)
  {
    logerr("Shader '%s' not found in bin dump", shader_name);
    return NULL;
  }
  if (dd_stricmp(sm->getShaderClassName(), shader_name) != 0)
    DAG_FATAL("Shader '%s' not found", shader_name);
  return sm;
}

bool shader_mats_are_equal(ShaderMaterial *m1, ShaderMaterial *m2) { return m1->native().props.isEqual(m2->native().props); }

/*********************************
 *
 * class ScriptedShaderMaterial
 *
 *********************************/
TEXTUREID ScriptedShaderMaterial::get_texture(int i) const
{
  if ((i < 0) || (i >= MAXMATTEXNUM))
    return BAD_TEXTUREID;
  return props.textureId[i];
}

void ScriptedShaderMaterial::set_texture(int i, TEXTUREID tex)
{
  if (i < 0 || i >= MAXMATTEXNUM)
    return;
  props.textureId[i] = tex;
}

void ScriptedShaderMaterial::gatherUsedTex(TextureIdSet &tex_id_list) const
{
  if (varElem)
    varElem->gatherUsedTex(tex_id_list);

  if (props.sclass->localVars.v.size() != props.stvar.size())
    logerr("%s: props.sclass->localVars.v.size()=%d != %d=props.stvar.size()"
           " Try to rebuild shaders or re-export assets.",
      (const char *)props.sclass->name, props.sclass->localVars.v.size(), props.stvar.size());
  else
    for (int i = 0; i < props.stvar.size(); i++)
      if (props.sclass->localVars.v[i].type == SHVT_TEXTURE && props.stvar[i].texId != BAD_TEXTUREID)
        tex_id_list.add(props.stvar[i].texId);

  for (int i = 0; i < MAXMATTEXNUM; i++)
    if (props.textureId[i] != BAD_TEXTUREID)
      tex_id_list.add(props.textureId[i]);
}

bool ScriptedShaderMaterial::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  bool replaced = false;
  int i;

  if (varElem && varElem->replaceTexture(tex_id_old, tex_id_new))
  {
    replaced = true;
  }

  if (tex_id_old == tex_id_new)
    ; // skip next cycle and proceed to MAXMATTEXNUM
  else if (props.sclass->localVars.v.size() != props.stvar.size())
    logerr("%s: props.sclass->localVars.v.size()=%d != %d=props.stvar.size()"
           " Try to rebuild shaders or re-export assets.",
      (const char *)props.sclass->name, props.sclass->localVars.v.size(), props.stvar.size());
  else
    for (i = 0; i < props.stvar.size(); i++)
    {
      if ((props.sclass->localVars.v[i].type == SHVT_TEXTURE) && (props.stvar[i].texId == tex_id_old))
      {
#if DAGOR_DBGLEVEL > 0
        debug("ScriptedShaderMaterial: replaced stvar %d, id#%d->%d", i, tex_id_old, tex_id_new);
#endif
        props.stvar[i].texId = tex_id_new;
      }
    }

  for (i = 0; i < MAXMATTEXNUM; i++)
  {
    if (props.textureId[i] == tex_id_old)
    {
      props.textureId[i] = tex_id_new;
      replaced = true;
    }
  }

  return replaced;
}

ScriptedShaderMaterial *ScriptedShaderMaterial::create(const ShaderMaterialProperties &p, bool clone_mat)
{
  ScriptedShaderMaterial *mat = NULL;
  size_t sz = sizeof(ScriptedShaderMaterial) + elem_size(p.stvar) * p.sclass->localVars.v.size();
  if (shaders_internal::shader_reload_allowed)
    sz += shaders_internal::shader_pad_for_reload;
  void *mem = memalloc(sz, midmem);
  mat = new (mem, _NEW_INPLACE) ScriptedShaderMaterial(p);
  if (!clone_mat)
    mat->props.execInitCode();
  return mat;
}

ScriptedShaderMaterial::ScriptedShaderMaterial(const ShaderMaterialProperties &p)
{
  nonSharedRefCount = 0;
  char *base = ((char *)this) + sizeof(ScriptedShaderMaterial);

  G_ASSERT(&props != &p);
  memcpy(&props, &p, sizeof(p));
  props.legacy2 = 0;
  props.stvar.init(base, props.sclass->localVars.v.size());
  if (data_size(props.stvar) <= data_size(p.stvar))
    mem_copy_from(props.stvar, p.stvar.data());
  else
  {
    mem_set_0(props.stvar);
    mem_copy_to(p.stvar, props.stvar.data());
  }
  base += data_size(props.stvar);

  shaders_internal::register_material(this);

  if (p.stvar.size() != p.sclass->localVars.v.size())
  {
    if (assert_on_vars)
    {
      G_ASSERTF(0, "Variables mismatch in '%s'", (const char *)props.sclass->name);
      assert_on_vars = false;
    }

    // startup initcode to fix mismatched material dump / shader
    for (int i = 0; i < props.stvar.size(); i++)
      if (props.sclass->localVars.v[i].type == SHVT_TEXTURE)
        props.stvar[i].texId = BAD_TEXTUREID;
      else if (props.sclass->localVars.v[i].type == SHVT_SAMPLER)
        props.stvar[i].samplerHnd = d3d::INVALID_SAMPLER_HANDLE;
      else if (props.sclass->localVars.v[i].type == SHVT_REAL && check_nan(props.stvar[i].r))
        props.stvar[i].r = 0;
  }
}
ScriptedShaderMaterial::~ScriptedShaderMaterial()
{
  varElem = NULL;
  shaders_internal::unregister_material(this);
}

void ScriptedShaderMaterial::recreateMat(bool delete_programs)
{
  int variant = varElem.get() ? &varElem->code - props.sclass->code.begin() : -1;
  int rc = ref_count;
  int nsrc = nonSharedRefCount;
  ref_count = 0;
  Ptr<ScriptedShaderElement> e = varElem;
  varElem = NULL;
  ShaderMaterialProperties copyProperties;
  memcpy(&copyProperties, &this->props, sizeof(copyProperties)); //-V780
  this->~ScriptedShaderMaterial();
  copyProperties.recreateMat();
  if (shaders_internal::shader_reload_allowed)
  {
    int memSize = midmem->getSize(this);
    size_t sz = sizeof(ScriptedShaderMaterial) + elem_size(copyProperties.stvar) * copyProperties.sclass->localVars.v.size();
    G_ASSERTF_RETURN(sz <= memSize, ,
      "Can't recreate scripted material. Requesting %d bytes, allocated %d (including %d byte padding)"
      " please increase graphics/shader_pad_for_reload:i to at least %d in config to fit reloaded material",
      sz, memSize, shaders_internal::shader_pad_for_reload, shaders_internal::shader_pad_for_reload + sz - memSize);
    G_UNUSED(sz);
    G_UNUSED(memSize);

#if DAGOR_DBGLEVEL > 0
    if (memSize - sz > max_reload_padding)
    {
      max_reload_padding = memSize - sz;
      debug("ScriptedMaterial max padding for reload is %d", max_reload_padding);
    }
#endif
  }
  new (this, _NEW_INPLACE) ScriptedShaderMaterial(copyProperties);

  ref_count = rc;
  nonSharedRefCount = nsrc;
  varElem = e;

  if (varElem.get())
  {
    const shaderbindump::ShaderCode *code = find_variant();
    if (code && code->varSize > varElem->code.varSize)
      code = nullptr, variant = -1;
    if (code)
      varElem->recreateElem(*code, *this);
    else if (variant >= 0 && variant < props.sclass->code.size() && props.sclass->code[variant].varSize <= varElem->code.varSize)
    {
      logwarn("recreating shElem=%p with code variant %d, while mat.find_variant() returns NULL", varElem.get(), variant);
      varElem->recreateElem(props.sclass->code[variant], *this);
    }
    else
    {
      varElem->native().resetShaderPrograms(delete_programs);
      varElem->native().resetStateBlocks();
      varElem = NULL;
    }
  }
}

ShaderMaterial *ScriptedShaderMaterial::clone() const
{
  ScriptedShaderMaterial *mat = ScriptedShaderMaterial::create(props, true);
  mat->varElem = NULL;
  return mat;
}

void ScriptedShaderMaterial::updateStVar(int prop_stvar_id)
{
  if (varElem)
    varElem->update_stvar(*this, prop_stvar_id);
}

#define SET_VARIABLE(var_type, param)                                              \
  if (uint32_t(variable_id) >= uint32_t(props.shBinDumpOwner().maxShadervarCnt())) \
    return false;                                                                  \
  int vid = props.shBinDumpOwner().varIndexMap[variable_id];                       \
  if (vid >= SHADERVAR_IDX_ABSENT)                                                 \
    return false;                                                                  \
  int id = props.sclass->localVars.findVar(vid);                                   \
  if (id < 0)                                                                      \
    return false;                                                                  \
  if (props.sclass->localVars.v[id].type != var_type)                              \
    return false;                                                                  \
  props.stvar[id].param = v;                                                       \
  updateStVar(id);

bool ScriptedShaderMaterial::set_int_param(const int variable_id, const int v)
{
  SET_VARIABLE(SHVT_INT, i);
  return true;
}

bool ScriptedShaderMaterial::set_real_param(const int variable_id, const real v)
{
  if (!shaders_internal::check_var_nan(v, variable_id))
    return false;
  SET_VARIABLE(SHVT_REAL, r);
  return true;
}

bool ScriptedShaderMaterial::set_color4_param(const int variable_id, const Color4 &v)
{
  if (!shaders_internal::check_var_nan(v.r + v.g + v.b + v.a, variable_id))
    return false;
  SET_VARIABLE(SHVT_COLOR4, c4());
  return true;
}

bool ScriptedShaderMaterial::set_texture_param(const int variable_id, const TEXTUREID v)
{
  SET_VARIABLE(SHVT_TEXTURE, texId);
  return true;
}

bool ScriptedShaderMaterial::set_sampler_param(const int variable_id, d3d::SamplerHandle v)
{
  SET_VARIABLE(SHVT_SAMPLER, samplerHnd);
  return true;
}

#undef SET_VARIABLE

#define GET_VARIABLE(GET_FUNC)                                                     \
  if (uint32_t(variable_id) >= uint32_t(props.shBinDumpOwner().maxShadervarCnt())) \
    return false;                                                                  \
  int vid = props.shBinDumpOwner().varIndexMap[variable_id];                       \
  if (vid >= SHADERVAR_IDX_ABSENT)                                                 \
    return false;                                                                  \
  int id = props.sclass->localVars.findVar(vid);                                   \
  if (id < 0)                                                                      \
    return false;                                                                  \
  value = GET_FUNC(id);                                                            \
  return true;

bool ScriptedShaderMaterial::hasVariable(const int variable_id) const
{
  if (variable_id < 0 || variable_id >= props.shBinDumpOwner().maxShadervarCnt())
    return false;
  int vid = props.shBinDumpOwner().varIndexMap[variable_id];
  return (vid < SHADERVAR_IDX_ABSENT) ? props.sclass->localVars.findVar(vid) >= 0 : false;
}

bool ScriptedShaderMaterial::getColor4Variable(const int variable_id, Color4 &value) const { GET_VARIABLE(get_color4_stvar); }

bool ScriptedShaderMaterial::getRealVariable(const int variable_id, real &value) const { GET_VARIABLE(get_real_stvar); }

bool ScriptedShaderMaterial::getIntVariable(const int variable_id, int &value) const { GET_VARIABLE(get_int_stvar); }

bool ScriptedShaderMaterial::getTextureVariable(const int variable_id, TEXTUREID &value) const { GET_VARIABLE(get_tex_stvar); }

bool ScriptedShaderMaterial::getSamplerVariable(const int variable_id, d3d::SamplerHandle &value) const
{
  GET_VARIABLE(get_sampler_stvar);
}

#undef GET_VARIABLE

void ScriptedShaderMaterial::set_flags(int value, int mask)
{
  mask &= ~(SHFLG_LIGHTMAP | SHFLG_VLTMAP);
  props.matflags = (props.matflags & ~mask) | (value & mask);
}

const shaderbindump::ShaderCode *ScriptedShaderMaterial::find_variant() const
{
  if (!props.sclass->code.size())
    return NULL;

  dag::ConstSpan<shaderbindump::VariantTable::IntervalBind> pcs = props.sclass->stVariants.codePieces;
  unsigned variant_code = 0;

  for (int i = 0; i < pcs.size(); i++)
  {
    const shaderbindump::Interval &interval = shBinDump().intervals[pcs[i].intervalId];
    unsigned normVal = 0;

    if (interval.type == shaderbindump::Interval::TYPE_MODE)
    {
      switch (interval.nameId)
      {
        case shaderbindump::Interval::MODE_2SIDED: normVal = (get_flags() & SHFLG_2SIDED) ? 1 : 0; break;
        case shaderbindump::Interval::MODE_REAL2SIDED: normVal = (get_flags() & SHFLG_REAL2SIDED) ? 1 : 0; break;
        case shaderbindump::Interval::MODE_LIGHTING: break;

        default: G_ASSERT(0);
      }
      G_ASSERT(normVal < interval.getValCount());
    }
    else if (interval.type == shaderbindump::Interval::TYPE_INTERVAL)
    {
      int varIndex = props.sclass->localVars.findVar(interval.nameId);
      if (varIndex < 0)
      {
        logerr("variable '%s' not found in shader material <%s>", (const char *)shBinDump().varMap[interval.nameId],
          (const char *)props.sclass->name);
        continue;
      }

      real varValue = 0;
      switch (props.sclass->localVars.v[varIndex].type)
      {
        case SHVT_INT: varValue = props.stvar[varIndex].i; break;
        case SHVT_REAL: varValue = props.stvar[varIndex].r; break;
        case SHVT_COLOR4:
          varValue = (props.stvar[varIndex].c[0] + props.stvar[varIndex].c[1] + props.stvar[varIndex].c[2]) / 3.;
          break;
        case SHVT_TEXTURE: varValue = (props.stvar[varIndex].texId == BAD_TEXTUREID) ? 0 : 1; break;
        default:
          logerr("illegal variable type <%d>, for interval '%s', shader<%s>!", props.sclass->localVars.v[varIndex].type,
            (const char *)shBinDump().varMap[interval.nameId], (const char *)props.sclass->name);
          G_ASSERT(0);
          continue;
      }
      normVal = interval.getNormalizedValue(varValue);
    }
    else
      continue;

    variant_code += normVal * pcs[i].totalMul;
  }

  int id = props.sclass->stVariants.findVariant(variant_code);
  if (id == props.sclass->stVariants.FIND_NOTFOUND)
  {
#if DAGOR_DBGLEVEL > 0
    bool has_dump = shaderbindump::hasShaderInvalidVariants(props.sclass->nameId);
    if (!shaderbindump::markInvalidVariant(props.sclass->nameId, variant_code, 0xFFFF))
      return NULL;
    if (!has_dump)
      shaderbindump::dumpShaderInfo(*props.sclass);
#endif
    DAG_FATAL("static variant %u not found (shader %s)", variant_code, props.sclass->name.data());
    return NULL;
  }
  if (id == props.sclass->stVariants.FIND_NULL)
    return NULL;

  if (id >= props.sclass->code.size())
  {
    DAG_FATAL("static variant %s:%u not found (ret=%08x)", (const char *)props.sclass->name, variant_code, id);
    return NULL;
  }

#if DAGOR_DBGLEVEL > 0
  auto &codeFlags = const_cast<shaderbindump::ShaderClass *>(props.sclass)->code[id].codeFlags;
  G_FAST_ASSERT(uintptr_t(&codeFlags) % sizeof(codeFlags) == 0); // Otherwise atomic might crash on ARM
  interlocked_or(codeFlags, shaderbindump::ShaderCode::CF_USED);
#endif
  return &props.sclass->code[id];
}

bool ScriptedShaderMaterial::enum_channels(ShaderChannelsEnumCB &cb, int &ret_code_flags) const
{
  //== RMODE_ALL enumeration
  const shaderbindump::ShaderCode *code = find_variant();
  if (!code)
    return false;

  ret_code_flags = code->codeFlags & ~shaderbindump::ShaderCode::CF_USED;
  if (!(ret_code_flags & SC_NEW_STAGE_FMT))
  {
    if (ret_code_flags & 1)
      ret_code_flags = ShaderMesh::STG_trans | (ret_code_flags & ~SC_STAGE_IDX_MASK);
    ret_code_flags |= SC_NEW_STAGE_FMT;
  }

  for (int i = 0; i < code->channel.size(); ++i)
  {
    const ShaderChannelId &c = code->channel[i];
    G_ASSERT(c.u != SCUSAGE_LIGHTMAP);
    cb.enum_shader_channel(c.u, c.ui, c.t, c.vbu, c.vbui, c.mod);
  }
  return true;
}

bool ScriptedShaderMaterial::isPositionPacked() const
{
  // All variants of the shader should have the same packing
  G_FAST_ASSERT(props.sclass && !props.sclass->code.empty());
  auto &code = props.sclass->code[0];

  for (auto &channel : code.channel)
    if (channel.mod == ChannelModifier::CMOD_BOUNDING_PACK)
      return true;

  return false;
}

ShaderElement *ScriptedShaderMaterial::make_elem(bool acq_tex_ref, const char *info)
{
  const shaderbindump::ShaderCode *code = find_variant();
  if (!code)
    return NULL;

  if (varElem && &varElem->code == code)
    return varElem;
  if (varElem)
  {
    debug("%p.make_elem changes %p -> %p", this, &varElem->code, code);
    varElem = NULL;
  }

  varElem = ScriptedShaderElement::create(*code, *this, info);
  if (acq_tex_ref)
    varElem->native().acquireTexRefs();
  return varElem;
}

// return true, if channels are valid for this material & specified render mode
bool ScriptedShaderMaterial::checkChannels(CompiledShaderChannelId *ch, int ch_count) const
{
  const shaderbindump::ShaderCode *code = find_variant();
  if (!code)
    return false;

  if (code->channel.size() != ch_count)
    return false;

  for (int i = 0; i < code->channel.size(); ++i)
  {
    const ShaderChannelId &c = code->channel[i];
    const CompiledShaderChannelId &dest = ch[i];
    if ((dest.t != c.t) || (dest.vbu != c.vbu) || (dest.vbui != c.vbui))
      return false;
  }

  return true;
}

static const char *attrib_type_name(int type)
{
  switch (type)
  {
    case SHVT_INT: return "SHVT_INT";
    case SHVT_REAL: return "SHVT_REAL";
    case SHVT_COLOR4: return "SHVT_COLOR4";
    case SHVT_TEXTURE: return "SHVT_TEXTURE";
    case SHVT_BUFFER: return "SHVT_BUFFER";
    case SHVT_INT4: return "SHVT_INT4";
    case SHVT_FLOAT4X4: return "SHVT_FLOAT4X4";
    case SHVT_SAMPLER: return "SHVT_SAMPLER";
    default: return "UNKNOWN";
  }
}

static const char *attrib_name(int name_id) { return (const char *)shBinDump().varMap[name_id]; }

#define CHECK_TYPE(t)                     \
  auto &v = props.sclass->localVars.v[i]; \
  G_ASSERTF(v.type == t, "In %s, %s is not %s, but %s", getShaderClassName(), attrib_name(v.nameId), #t, attrib_type_name(v.type));

Color4 ScriptedShaderMaterial::get_color4_stvar(int i) const
{
  G_ASSERT(i >= 0 && i < props.sclass->localVars.v.size());
  CHECK_TYPE(SHVT_COLOR4);
  return props.stvar[i].c4();
}
real ScriptedShaderMaterial::get_real_stvar(int i) const
{
  G_ASSERT(i >= 0 && i < props.sclass->localVars.v.size());
  CHECK_TYPE(SHVT_REAL);
  return props.stvar[i].r;
}
int ScriptedShaderMaterial::get_int_stvar(int i) const
{
  G_ASSERT(i >= 0 && i < props.sclass->localVars.v.size());
  CHECK_TYPE(SHVT_INT);
  return props.stvar[i].i;
}
TEXTUREID ScriptedShaderMaterial::get_tex_stvar(int i) const
{
  G_ASSERT(i >= 0 && i < props.sclass->localVars.v.size());
  CHECK_TYPE(SHVT_TEXTURE);
  return props.stvar[i].texId;
}

d3d::SamplerHandle ScriptedShaderMaterial::get_sampler_stvar(int i) const
{
  G_ASSERT(i >= 0 && i < props.sclass->localVars.v.size());
  CHECK_TYPE(SHVT_SAMPLER);
  return props.stvar[i].samplerHnd;
}

void ScriptedShaderMaterial::getMatData(ShaderMatData &dest) const
{
  memset(&dest, 0, sizeof(dest));

  dest.d3dm = props.d3dm;
  dest.sclassName = (const char *)props.sclass->name;

  memcpy(dest.textureId, props.textureId, sizeof(dest.textureId));

  dest.stvar = make_span(const_cast<ShaderMaterialProperties &>(props).stvar);
  // reset texture IDs (required for shader mesh build to be deterministic)
  // side effect: since dest.stvar is dag::Span, changing it contents changes also props.stvar
  for (int i = 0; i < props.sclass->localVars.v.size(); i++)
    if (props.sclass->localVars.v[i].type == SHVT_TEXTURE)
      dest.stvar[i].texId = BAD_TEXTUREID;

  dest.matflags = props.matflags;
}

void ScriptedShaderMaterial::buildMaterialData(MaterialData &out_data, const char *orig_mat_script)
{
  out_data.mat = props.d3dm;
  out_data.flags = (props.matflags & SHFLG_2SIDED) ? MATF_2SIDED : 0;
  out_data.className = (const char *)props.sclass->name;
  G_STATIC_ASSERT(sizeof(out_data.mtex) == sizeof(props.textureId));
  G_STATIC_ASSERT(sizeof(out_data.mtex[0]) == sizeof(props.textureId[0]));
  memcpy(out_data.mtex, props.textureId, sizeof(out_data.mtex));
  String par;
  CfgReader c;
  if (orig_mat_script)
    c.getdiv_text(String(0, "[q]\r\n%s\r\n", orig_mat_script), "q");
  for (int i = 0; i < props.sclass->localVars.v.size(); i++)
  {
    const char *vn = (const char *)shBinDump().varMap[props.sclass->localVars.v[i].nameId];
    if (orig_mat_script && !c.getstr(vn, nullptr))
      continue;
    switch (props.sclass->localVars.v[i].type)
    {
      case SHVT_INT: par.aprintf(0, "%s=%d\n", vn, props.stvar[i].i); break;
      case SHVT_REAL: par.aprintf(0, "%s=%g\n", vn, props.stvar[i].r); break;
      case SHVT_COLOR4:
        par.aprintf(0, "%s=%g,%g,%g,%g\n", vn, props.stvar[i].c4().r, props.stvar[i].c4().g, props.stvar[i].c4().b,
          props.stvar[i].c4().a);
        break;
    }
  }
  if (props.matflags & SHFLG_REAL2SIDED)
    par.aprintf(0, "real_two_sided=1\n", true);
  if (par.size())
    par.pop(); // remove last \n
  out_data.matScript = par;
}

/*********************************
 *
 * class ShaderMaterialProperties
 *
 *********************************/
ShaderMaterialProperties *ShaderMaterialProperties::create(const shaderbindump::ShaderClass *sc, const MaterialData &m,
  bool sec_dump_for_exp)
{
  ShaderMaterialProperties *mp = NULL;
  void *mem = memalloc(sizeof(ShaderMaterialProperties) + elem_size(mp->stvar) * sc->localVars.v.size(), midmem);
  mp = new (mem, _NEW_INPLACE) ShaderMaterialProperties(sc, m, sec_dump_for_exp);
  return mp;
}

ShaderMaterialProperties::ShaderMaterialProperties(const shaderbindump::ShaderClass *sc, const MaterialData &m, bool sec_dump_for_exp)
{
  sclass = sc;
  secondDump = sec_dump_for_exp;

  // gather used tex mask (to avoid referencing textures in material that will not be used in shader)
  unsigned used_tex_mask = get_shclass_used_tex_mask(sclass);

  for (int i = 0; i < MAXMATTEXNUM; ++i)
    if (used_tex_mask & (1 << i))
      textureId[i] = m.mtex[i];
    else
    {
      if (m.mtex[i] != BAD_TEXTUREID)
        logwarn("%s: skip unused mtex[%d]=%d (%s), texMask=%04X", (const char *)sc->name, i, m.mtex[i],
          get_managed_texture_name(m.mtex[i]), used_tex_mask);
      textureId[i] = BAD_TEXTUREID;
    }

  d3dm = m.mat;

  CfgReader c;
  c.getdiv_text(String(128, "[q]\r\n%s\r\n", m.matScript.str()), "q");

  // Intentionally removed. Emission was multiplied in StaticGeometryContainer::DagLoadData::getMaterial.
  // d3dm.emis*=c.getreal("emission",1);

  // fill all static variables with new values (initally zeroed)
  stvar.init(((char *)this) + sizeof(*this), sclass->localVars.v.size());
  mem_set_0(stvar);

  for (int i = 0, e = stvar.size(); i < e; ++i)
  {
    if (sclass->localVars.isPublic(i))
      continue;

    const char *varName = (const char *)shBinDump().varMap[sclass->localVars.getNameId(i)];

    switch (sclass->localVars.getType(i))
    {
      case SHVT_COLOR4: stvar[i].c4() = c.getcolor4(varName, sclass->localVars.get<Color4>(i)); break;
      case SHVT_INT: stvar[i].i = c.getint(varName, sclass->localVars.get<int>(i)); break;
      case SHVT_REAL: stvar[i].r = c.getreal(varName, sclass->localVars.get<real>(i)); break;
      case SHVT_TEXTURE: stvar[i].texId = sclass->localVars.getTex(i).texId; break;
      case SHVT_SAMPLER: stvar[i].samplerHnd = sclass->localVars.get<d3d::SamplerHandle>(i); break;
      default: G_ASSERT(0);
    }
  }

  execInitCode();

  matflags = 0;
  legacy2 = 0;

  if (m.flags & MATF_2SIDED)
    matflags |= SHFLG_2SIDED;
  if (c.getbool("real_two_sided", false))
    matflags |= SHFLG_REAL2SIDED;
}

void ShaderMaterialProperties::patchNonSharedData(void *base, dag::Span<TEXTUREID> texMap)
{
  stvar.patch(base);
  const char *shname = (char *)base + sclassNameOfs;
  sclass = ::shBinDump().findShaderClass(shname);

  if (!sclass)
    DAG_FATAL("shader <%s> not found!", shname);

  if (stvar.size() != sclass->localVars.v.size())
    logerr("%s: props.sclass->localVars.v.size()=%d != %d=props.stvar.size()"
           " Try to rebuild shaders or re-export assets.",
      (const char *)sclass->name, sclass->localVars.v.size(), stvar.size());

  for (int i = 0; i < MAXMATTEXNUM; i++)
    textureId[i] = unsigned(textureId[i]) < texMap.size() ? texMap[unsigned(textureId[i])] : BAD_TEXTUREID;

  // cannot change content of stvar here since it is shared between several materials!
}

void ShaderMaterialProperties::recreateMat()
{
  const char *shname = (const char *)sclass->name;
  sclass = shBinDump().findShaderClass(shname);

  if (!sclass)
  {
    logwarn("shader <%s> not found!", shname);
    sclass = &shaderbindump::null_shader_class(false);
  }

  if (stvar.size() != sclass->localVars.v.size())
    logerr("%s: props.sclass->localVars.v.size()=%d != %d=props.stvar.size()"
           " Try to rebuild shaders or re-export assets.",
      (const char *)sclass->name, sclass->localVars.v.size(), stvar.size());
}

int ShaderMaterialProperties::execInitCode()
{
  int diff_stvar_id = -1;
  for (int i = 0; i < sclass->initCode.size(); i += 2)
  {
    int stVarId = sclass->initCode[i], ind;
    switch (shaderopcode::getOp(sclass->initCode[i + 1]))
    {
      case SHCOD_DIFFUSE: *(Color4 *)stvar[stVarId].c = d3dm.diff; break;
      case SHCOD_EMISSIVE: *(Color4 *)stvar[stVarId].c = d3dm.emis; break;
      case SHCOD_SPECULAR: *(Color4 *)stvar[stVarId].c = d3dm.spec; break;
      case SHCOD_AMBIENT: *(Color4 *)stvar[stVarId].c = d3dm.amb; break;
      case SHCOD_TEXTURE:
        ind = shaderopcode::getOp2p1(sclass->initCode[i + 1]);
        if (ind == 0)
        {
          diff_stvar_id = stVarId;
          stvar[stVarId].texId = get_diffuse_mipmap_tex(textureId[ind]);
        }
        else
          stvar[stVarId].texId = textureId[ind];
        break;

      default: G_ASSERT(0 && "unsupported");
    }
  }
  return diff_stvar_id;
}

const String ShaderMaterialProperties::getInfo() const
{
  String info;

  info.aprintf(128, "'%s' ", sclass ? (const char *)sclass->name : "<sclass==null>");

  for (TEXTUREID tid : textureId)
    if (tid)
      info.aprintf(128, "0x%x='%s' ", unsigned(tid), get_managed_texture_name(tid));

  info.aprintf(128, "f=0x%x ", (int)matflags);
  for (unsigned j = 0; j < stvar.size(); j++)
  {
    info.aprintf(128, "%s=", (const char *)shBinDump().varMap[sclass->localVars.v[j].nameId]);

    switch (sclass->localVars.v[j].type)
    {
      case SHVT_INT: info.aprintf(128, "(%d) ", stvar[j].i); break;
      case SHVT_REAL: info.aprintf(128, "(%.4f) ", stvar[j].r); break;
      case SHVT_TEXTURE: info.aprintf(128, "(0x%x) ", stvar[j].texId); break;
      case SHVT_SAMPLER: info.aprintf(128, "(0x%llx) ", (uint64_t)stvar[j].samplerHnd); break;
      case SHVT_COLOR4: info.aprintf(128, "(%~c4) ", stvar[j].c4()); break;
    }
  }

  info.aprintf(4096, "[(%~c4) (%~c4) (%~c4) (%~c4) pow=%g]", d3dm.diff, d3dm.amb, d3dm.spec, d3dm.emis, d3dm.power);

  return info;
}

bool ShaderMaterialProperties::isEqual(const ShaderMaterialProperties &p) const
{
  if (memcmp(textureId, p.textureId, sizeof(textureId)) != 0)
    return false;
  return isEqualWithoutTex(p);
}

bool ShaderMaterialProperties::isEqualWithoutTex(const ShaderMaterialProperties &p) const
{
  if (sclass != p.sclass)
    return false;
  if (memcmp(&d3dm, &p.d3dm, sizeof(d3dm)) != 0)
    return false;
  if (matflags != p.matflags)
    return false;

  if (data_size(stvar) != data_size(p.stvar))
    return false;

  if (!mem_eq(stvar, p.stvar.data()))
  {
    for (int j = 0; j < stvar.size(); j++)
    {
      if (sclass->localVars.v[j].type == SHVT_TEXTURE)
        continue;
      if (memcmp(&stvar[j], &p.stvar[j], sizeof(stvar[j])) != 0)
        return false;
    }
  }

  // at last, we now they are identical
  return true;
}

//
// mipmap debugging
//
#if _TARGET_PC
struct DebugMipmapTexRec
{
  Texture *tex;
  TEXTUREID texId;
  int w, h;
};
static Tab<DebugMipmapTexRec> dbg_mipmap_tex_list(tmpmem);

static TEXTUREID get_diffuse_mipmap_tex(TEXTUREID tex_id)
{
  if (!diffuse_mipmaps_debug_enabled || tex_id == BAD_TEXTUREID)
    return tex_id;
  BaseTexture *t = acquire_managed_tex(tex_id);
  if (!t)
    return tex_id;

  if (t->restype() != RES3D_TEX)
  {
    release_managed_tex(tex_id);
    return tex_id;
  }

  TextureInfo ti;
  d3d_err(t->getinfo(ti));
  release_managed_tex(tex_id);

  for (int i = 0; i < dbg_mipmap_tex_list.size(); i++)
    if (dbg_mipmap_tex_list[i].w == ti.w && dbg_mipmap_tex_list[i].h == ti.h)
      return dbg_mipmap_tex_list[i].texId;

  DebugMipmapTexRec &r = dbg_mipmap_tex_list.push_back();
  String name(128, "*mipdbg@%dx%d", ti.w, ti.h);
  static const int lev_num = 6;
  static unsigned colors[lev_num] = {
    E3DCOLOR_MAKE(255, 0, 0, 255),   // RED
    E3DCOLOR_MAKE(0, 255, 0, 255),   // GREEN
    E3DCOLOR_MAKE(0, 0, 255, 255),   // BLUE
    E3DCOLOR_MAKE(0, 255, 255, 255), // CYAN
    E3DCOLOR_MAKE(255, 0, 255, 255), // MAGENTA
    E3DCOLOR_MAKE(255, 255, 0, 255), // YELLOW
  };

  r.w = ti.w;
  r.h = ti.h;
  r.tex = d3d::create_tex(NULL, r.w, r.h, TEXCF_RGB, lev_num, name);
  if (!r.tex)
  {
    dbg_mipmap_tex_list.pop_back();
    return tex_id;
  }
  r.texId = register_managed_tex(name, r.tex);

  for (int i = 0; i < lev_num; i++)
  {
    unsigned *p;
    int stride;
    if (!r.tex->lockimgEx(&p, stride, i))
      break;
    G_ASSERT(stride >= ti.w * 4);
    for (int y = 0; y < ti.h; y++, p += stride / 4)
      for (unsigned *pl = p, *pe = p + ti.w; pl < pe; pl++)
        *pl = colors[i];

    ti.w >>= 1;
    ti.h >>= 1;
    if (ti.w < 1)
      ti.w = 1;
    if (ti.h < 1)
      ti.h = 1;
  }
  return r.texId;
}

void enable_diffuse_mipmaps_debug(bool en)
{
  using shaders_internal::shader_mats;

  if (diffuse_mipmaps_debug_enabled == en)
    return;
  diffuse_mipmaps_debug_enabled = en;
  debug("diffuse_mipmaps_debug_enabled=%d", diffuse_mipmaps_debug_enabled);
  for (auto sm : shader_mats)
    if (sm)
    {
      int prop_stvarid = sm->props.execInitCode();
      if (prop_stvarid != -1)
        sm->updateStVar(prop_stvarid);
    }

  if (!diffuse_mipmaps_debug_enabled)
  {
    debug("releasing %d used debug mipmap textures", dbg_mipmap_tex_list.size());
    for (int i = 0; i < dbg_mipmap_tex_list.size(); i++)
      ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(dbg_mipmap_tex_list[i].texId, dbg_mipmap_tex_list[i].tex);
    clear_and_shrink(dbg_mipmap_tex_list);
  }
}
#else
void enable_diffuse_mipmaps_debug(bool /*en*/) { DEBUG_CTX("n/a"); }
#endif

void enable_assert_on_shader_mat_vars(bool en) { assert_on_vars = en; }
