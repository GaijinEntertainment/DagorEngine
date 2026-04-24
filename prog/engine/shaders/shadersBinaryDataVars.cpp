// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_PC_WIN
#undef _DEBUG_TAB_ // exec_stcode if called too frequently to allow any perfomance penalty there
#endif
#include "mapBinarySearch.h"
#include "shadersBinaryData.h"
#include "shBindumpsPrivate.h"
#include <shaders/dag_shaderVariableInfo.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderCommon.h>
#include <util/dag_globDef.h>
#include <string.h>
#include <debug/dag_debug.h>
#include "shStateBlk.h"
#include <util/dag_fastNameMapTS.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_dxmath.h>
#include <ctype.h>
#include <3d/dag_resPtr.h>

static FastNameMapTS<false> shvarNameMap;

void ScriptedShadersGlobalData::initVarIndexMaps(size_t max_shadervar_cnt)
{
  varIndexMap.resize(max_shadervar_cnt);
  globvarIndexMap.resize(max_shadervar_cnt);
  int nameCount = shvarNameMap.iterate([&](int nid, const char *name) {
    varIndexMap[nid] = mapbinarysearch::bfindStrId(backing->getDump()->varMap, name);
    globvarIndexMap[nid] = (varIndexMap[nid] == SHADERVAR_IDX_INVALID)
                             ? SHADERVAR_IDX_ABSENT
                             : bfind_packed_uint16_x2(backing->getDump()->gvMap, varIndexMap[nid]);
  });
  eastl::fill(varIndexMap.begin() + nameCount, varIndexMap.end(), SHADERVAR_IDX_ABSENT);
  eastl::fill(globvarIndexMap.begin() + nameCount, globvarIndexMap.end(), SHADERVAR_IDX_ABSENT);
}

void ScriptedShadersGlobalData::preRequestStaticSamplers(ScriptedShadersBinDumpOwner const &from_dump)
{
  auto const *v3 = from_dump.getDumpV3();

  if (!v3)
  {
    debug("[SH] shader dump V3 not present, static samplers will not be pre-loaded");
    return;
  }

  debug("[SH] pre-requesting %d immutable samplers...", v3->immutableSamplersMap.size());

  for (auto [smpId, gvarId] : v3->immutableSamplersMap)
  {
    // @TODO: once additional binudmps exist, skip already initialized samplers
    globVarsState.set(gvarId, d3d::request_sampler(v3->samplers[smpId]));
  }

  debug("[SH] successfully pre-loaded %d immutable samplers", v3->immutableSamplersMap.size());
}

#if DAGOR_DBGLEVEL > 0
bool ScriptedShadersGlobalData::validateShadervarWrite(int shadervar_id, int shadervar_name_id) const
{
  auto const *v4 = backing->getDumpV4();

  if (!v4)
    return true;
  if (DAGOR_UNLIKELY(v4->globVarsMetadata[shadervar_id].isLiteral))
  {
    logerr("Trying to modify a literal shadervar '%s' from application code.", shvarNameMap.getName(shadervar_name_id));
    return false;
  }
  return true;
}
bool validate_var_const_state(int var_name_id)
{
  auto const &globalData = shGlobalData();
  return globalData.validateShadervarWrite(globalData.globvarIndexMap[var_name_id], var_name_id);
}
#endif

#if DAGOR_DBGLEVEL > 0
namespace shaderbindump
{
void dump_shader_var_names();
}

static bool is_name_cident(const char *name, char delimiter = '\0')
{
  if (*name && !(*name == '_' || isalpha(*name)))
    return false;
  for (; *name && *name != delimiter; name++)
    if (!(isalnum(*name) || *name == '_'))
      return false;
  return true;
}

static bool name_is_arr_size_getter(const char *name)
{
  const int ARRAY_SIZE_GETTER_PREFIX_LEN = strchr(ARRAY_SIZE_GETTER_NAME, '(') - ARRAY_SIZE_GETTER_NAME + 1;

  const char *p = ARRAY_SIZE_GETTER_NAME;
  if (strncmp(name, p, ARRAY_SIZE_GETTER_PREFIX_LEN) != 0)
    return false;

  name += ARRAY_SIZE_GETTER_PREFIX_LEN;

  if (!is_name_cident(name, ')'))
    return false;

  name = strchr(name, ')');
  return name && *(name + 1) == '\0';
}

static bool shadervar_name_is_valid(const char *name) { return is_name_cident(name) || name_is_arr_size_getter(name); }
#endif

unsigned shaderbindump::blockStateWord = 0;

bool shaderbindump::autoBlockStateWordChange = false;

shaderbindump::ShaderBlock *shaderbindump::nullBlock[shaderbindump::MAX_BLOCK_LAYERS] = {0};

template <>
int shaderbindump::VarList::findVar(int var_id) const
{
  for (int i = 0, e = v.size(); i < e; i++)
    if (v[i].nameId == var_id)
      return i;
  return -1;
}

int mapbinarysearch::bfindStrId(dag::ConstSpan<StrHolder> map, const char *val)
{
  auto found = eastl::binary_search_i(map.begin(), map.end(), eastl::string_view(val),
    [](auto &a, auto &b) { return (eastl::string_view)a < (eastl::string_view)b; });
  return found == map.end() ? -1 : eastl::distance(map.begin(), found);
}

// VariableMap
static constexpr int NOT_GVAR = 0x40000000;

uint32_t VariableMap::generation() { return shaderbindump::get_generation(); }

const char *VariableMap::getVariableName(int var_id, bool sec_dump)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed()) // it is unsafe to add variable and access in differnt thread without sync.
                                                           // That means, that relaxed load is sufficient.
    return nullptr;
  return shvarNameMap.getName(var_id);
}
int VariableMap::getGlobalVariablesCount() { return shBinDump().gvMap.size(); }
const char *VariableMap::getGlobalVariableName(int globvar_idx)
{
  auto &dump = shBinDump();
  return unsigned(globvar_idx) < dump.gvMap.size() ? (const char *)dump.varMap[dump.gvMap[globvar_idx] & 0xFFFF] : nullptr;
}
int VariableMap::getVariableId(const char *var_name, bool sec_dump)
{
  int nid = shvarNameMap.addNameId(var_name);
  auto &globalData = shGlobalDataEx(!sec_dump);
  const auto &dump = *globalData.backing->getDump();
#if DAGOR_DBGLEVEL > 0
  if (DAGOR_UNLIKELY(nid >= globalData.maxShadervarCnt()))
    shaderbindump::dump_shader_var_names();
#endif
  G_ASSERTF_RETURN(nid < globalData.maxShadervarCnt(), -1, "var_name=%s shvarNameMap.nameCount=%d", var_name,
    shvarNameMap.nameCountRelaxed()); // it is unsafe to add variable and access in differnt thread without sync. That means, that
                                      // relaxed load is sufficient.
  if (globalData.varIndexMap[nid] == SHADERVAR_IDX_ABSENT)
  {
    uint16_t var_id = mapbinarysearch::bfindStrId(dump.varMap, var_name);
#if DAGOR_DBGLEVEL > 0
    if (var_id == SHADERVAR_IDX_INVALID && !shadervar_name_is_valid(var_name))
      logerr("bad var_name=%s for %s", var_name, __FUNCTION__);
#endif
    globalData.varIndexMap[nid] = var_id;
    globalData.globvarIndexMap[nid] = (globalData.varIndexMap[nid] == SHADERVAR_IDX_INVALID)
                                        ? SHADERVAR_IDX_ABSENT
                                        : bfind_packed_uint16_x2(dump.gvMap, globalData.varIndexMap[nid]);
  }
  return nid;
}

int VariableMap::getVariableArraySize(const char *name)
{
  eastl::string array_size_getter_name(eastl::string::CtorSprintf{}, ARRAY_SIZE_GETTER_NAME, name);
  int var_id = getVariableId(array_size_getter_name.c_str());
  return isVariablePresent(var_id) ? ShaderGlobal::get_int(var_id) : 0;
}

int VariableMap::getVariablesCount(bool /*sec_dump*/)
{
  return shvarNameMap.nameCountRelaxed();
} // it is unsafe to add variable and access in differnt thread without sync. That means, that relaxed load is sufficient.

bool VariableMap::isVariablePresent(int var_id)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed()) // it is unsafe to add variable and access in differnt thread without sync.
                                                           // That means, that relaxed load is sufficient.
    return false;
  return shGlobalData().varIndexMap[var_id] < SHADERVAR_IDX_ABSENT;
}

bool VariableMap::isGlobVariablePresent(int var_id)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed()) // it is unsafe to add variable and access in differnt thread without sync.
                                                           // That means, that relaxed load is sufficient.
    return false;
  return shGlobalData().globvarIndexMap[var_id] < SHADERVAR_IDX_ABSENT;
}

bool VariableMap::isVariablePresent(const ShaderVariableInfo &v) { return isVariablePresent(v.get_var_id()); }

bool VariableMap::isGlobVariablePresent(const ShaderVariableInfo &v) { return isGlobVariablePresent(v.get_var_id()); }

dag::Vector<String> VariableMap::getPresentedGlobalVariableNames()
{
  dag::Vector<String> varNames;
  auto &dump = shBinDump();
  varNames.reserve(dump.globVars.size());
  for (int i = 0; i < dump.globVars.size(); ++i)
  {
    const auto &name = dump.varMap[dump.globVars.getNameId(i)];
    varNames.push_back(String(name.c_str(), name.length()));
  }
  return varNames;
}

#define CHECK_VAR_ID(TYPE)                                                                                                          \
  if (var_id == -1)                                                                                                                 \
    return false;                                                                                                                   \
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed())                                                                          \
  {                                                                                                                                 \
    G_ASSERTF(0, "%s invalid var_id %d", __FUNCTION__, var_id);                                                                     \
    return false;                                                                                                                   \
  }                                                                                                                                 \
  auto &globalData = shGlobalData();                                                                                                \
  auto const &dump = *globalData.backing->getDump();                                                                                \
  if (!dump.globVars.size())                                                                                                        \
  {                                                                                                                                 \
    logerr("shaders not loaded while setting var_id=%d (%s)", var_id, shvarNameMap.getName(var_id));                                \
    return false;                                                                                                                   \
  }                                                                                                                                 \
  int id = globalData.globvarIndexMap[var_id];                                                                                      \
  if (id == SHADERVAR_IDX_ABSENT)                                                                                                   \
    return false;                                                                                                                   \
  G_ASSERTF_RETURN(id < dump.globVars.size(), false, "%s invalid var_id %d (to glob var %d)", __FUNCTION__, var_id, id);            \
  G_ASSERTF_RETURN(dump.globVars.getType(id) == TYPE, false, "var_id=%d (%s) glob_var_id=%d", var_id, shvarNameMap.getName(var_id), \
    id);                                                                                                                            \
  if (!globalData.validateShadervarWrite(id, var_id))                                                                               \
    return false;

static void set_global_interval_value_internal(int index, int v, const ScriptedShadersBinDump &dump)
{
#if DAGOR_DBGLEVEL > 0
  if (shGlobalData().violatedAssumedIntervals.count(index) > 0)
    return;
#endif

  const auto &interval = dump.intervals[index];
  if (DAGOR_UNLIKELY(interval.type == shaderbindump::Interval::TYPE_ASSUMED_INTERVAL))
  {
#if DAGOR_DBGLEVEL > 0
    if (interval.getAssumedVal() != v)
    {
      logwarn("`Interval Assume` violation for %s. This was assumed to %i, but set to %i", dump.varMap[interval.nameId].c_str(),
        interval.getAssumedVal(), v);

      shGlobalData().violatedAssumedIntervals.insert(index);
    }
#endif
    return;
  }
  shGlobalData().globIntervalNormValues[index] = interval.getNormalizedValue(v);
}

__forceinline void set_global_interval_value(int index, int v, const ScriptedShadersBinDump &dump)
{
  if (index >= 0)
    set_global_interval_value_internal(index, v, dump);
}

// ShaderGlobal

template <bool is_interval>
ShaderGlobal::detail::StringToHash<is_interval>::StringToHash(const char *name, size_t len) : hash(mem_hash_fnv1<32>(name, len))
{}

template struct ShaderGlobal::detail::StringToHash<false>;
template struct ShaderGlobal::detail::StringToHash<true>;

void ShaderGlobal::set_variant(Interval interv, Subinterval subinterv)
{
#if DAGOR_DBGLEVEL > 0
  if (!is_glob_interval_presented(interv))
    logerr("Interval with hash (%x) is not presented in the dump", interv.hash);
#endif

  auto *dump = shBinDumpOwner().getDumpV2();
  G_ASSERTF(dump, "The current shader dump is not supported for calling `%s`", __FUNCTION__);
  auto &info = dump->getIntervalInfoByHash(interv.hash);
  uint8_t index = info.getSubintervalIndexByHash(subinterv.hash);
  shGlobalData().globIntervalNormValues[info.intervalId] = index;

#if DAGOR_DBGLEVEL > 0
  if (info.subintervalHashes[index] != subinterv.hash)
  {
    auto &interval = dump->intervals[info.intervalId];
    logerr("Inavalid subinterval hash (%x) for '%s' interval", subinterv.hash, dump->varMap[interval.nameId].c_str());
  }
#endif
}

ShaderGlobal::Subinterval ShaderGlobal::get_variant(Interval interv)
{
  auto *dump = shBinDumpOwner().getDumpV2();
  G_ASSERTF(dump, "The current shader dump is not supported for calling `%s`", __FUNCTION__);
  auto &info = dump->getIntervalInfoByHash(interv.hash);
  uint8_t index = shGlobalData().globIntervalNormValues[info.intervalId];
  return Subinterval(info.subintervalHashes[index]);
}

bool ShaderGlobal::is_glob_interval_presented(Interval interv)
{
  auto *dump = shBinDumpOwner().getDumpV2();
  G_ASSERTF(dump, "The current shader dump is not supported for calling `%s`", __FUNCTION__);
  return dump->isIntervalPresented(interv.hash);
}

dag::Vector<String> ShaderGlobal::get_subinterval_names(Interval interv)
{
  auto *dump = shBinDumpOwner().getDumpV2();
  G_ASSERTF(dump, "The current shader dump is not supported for calling `%s`", __FUNCTION__);
  auto &info = dump->getIntervalInfoByHash(interv.hash);
  dag::Vector<String> ret(info.subintervals.size());
  for (size_t i = 0; i < ret.size(); i++)
    ret[i] = info.subintervals[i].c_str();
  return ret;
}

size_t ShaderGlobal::get_subinterval_count(Interval interv)
{
  auto *dump = shBinDumpOwner().getDumpV2();
  G_ASSERTF(dump, "The current shader dump is not supported for calling `%s`", __FUNCTION__);
  auto &info = dump->getIntervalInfoByHash(interv.hash);
  return info.subintervals.size();
}

char ShaderVariableInfo::zero[64]; // static variables are zero by default

const char *ShaderVariableInfo::getName() const
{
  if (var_id >= 0)
    return VariableMap::getVariableName(var_id);
  return data;
}

extern bool dgs_all_shader_vars_optionals;
void ShaderVariableInfo::resolve()
{
  G_STATIC_ASSERT(sizeof(Color4) == 16);  // verifies header inline get
  G_STATIC_ASSERT(sizeof(IPoint4) == 16); // verifies header inline get
  varType = -1;
  if (var_id == -1)
  {
    if (data)
    {
      if (!IShaderBindumpReloadListener::staticInitDone) // no sense in trying to resolve, shader dump is not loaded
        return;
      const int vid = VariableMap::getVariableId(data);
      if (vid >= 0)
        var_id = vid;
    }
  }
  if (var_id < 0)
    return;
  data = nullptr;
  iid = -1;
  if (!optional && !VariableMap::isVariablePresent(var_id) && !dgs_all_shader_vars_optionals)
  {
#if DAGOR_DBGLEVEL > 0
    uint32_t currentGen = shaderbindump::get_generation();
    if (lastNameResolveFailGeneration == currentGen)
      return;
    else
      lastNameResolveFailGeneration = currentGen;
#endif
    logerr("shader variable %s is mandatory (not optional), but is not present in shaders dump", getName());
    return;
  }
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed())
  {
    G_ASSERTF(0, "%s invalid var_id %d", __FUNCTION__, var_id);
    return;
  }
  auto &globalData = shGlobalData();
  auto &dump = *globalData.backing->getDump();
  if (!dump.globVars.size())
  {
    logerr("shaders not loaded while setting var_id=%d (%s)", var_id, shvarNameMap.getName(var_id));
    return;
  }
  int id = globalData.globvarIndexMap[var_id];
  if (id == SHADERVAR_IDX_ABSENT)
    return;
  if (id == SHADERVAR_IDX_INVALID)
  {
#if DAGOR_DBGLEVEL > 0
    if (!shadervar_name_is_valid(shvarNameMap.getName(var_id)))
      logerr("bad var_name=%s for %s", shvarNameMap.getName(var_id), __FUNCTION__);
#endif
    return;
  }
  data = static_cast<char *>(globalData.globVarsState.get_raw(id));
  varType = dump.globVars.getType(id);
  iid = globalData.globVarIntervalIdx[id];
}

void ShaderVariableInfo::nanError() const { logerr("setting <%s> variable to nan", VariableMap::getVariableName(var_id)); }

void ShaderVariableInfo::set_int_var_interval(int v) const { set_global_interval_value_internal(iid, v, shBinDump()); }

void ShaderVariableInfo::set_float_var_interval(float v) const
{
  auto &globalData = shGlobalData();
  auto const &dump = *globalData.backing->getDump();
  globalData.globIntervalNormValues[iid] = dump.intervals[iid].getNormalizedValue(v);
}

template <typename TextureType>
static bool set_texture_internal(shaders_internal::Tex &tex, int var_id, int iid, TextureType tex_arg)
{
  auto &globalData = shGlobalData();

  TEXTUREID oldId = tex.texId;
  BaseTexture *oldPtr = tex.tex;
  bool isOldTexManaged = tex.isTextureManaged();

  if constexpr (eastl::is_same_v<TEXTUREID, TextureType>)
  {
    tex.texId = tex_arg;
    tex.tex = acquire_managed_tex(tex_arg);
  }
  else if constexpr (eastl::is_same_v<BaseTexture *, TextureType>)
  {
    tex.texId = BAD_TEXTUREID;
    tex.tex = tex_arg;

#if DAGOR_DBGLEVEL > 0
    if (tex_arg)
    {
      auto &newCounter = globalData.globResourceVarPointerUsageMap[tex_arg];
      ++newCounter;
    }
#endif
  }

  if (isOldTexManaged)
  {
    release_managed_tex(oldId);
  }
  else
  {
#if DAGOR_DBGLEVEL > 0
    auto oldIt = globalData.globResourceVarPointerUsageMap.find(oldPtr);
    if (oldIt != globalData.globResourceVarPointerUsageMap.end())
    {
      if (--oldIt->second <= 0)
      {
        globalData.globResourceVarPointerUsageMap.erase(oldPtr);
      }
    }
#endif
  }


  if constexpr (eastl::is_same_v<TEXTUREID, TextureType>)
  {
    G_ASSERTF(tex_arg == BAD_TEXTUREID || get_managed_texture_refcount(tex_arg) > 0, "set_tex(%d) %d->%d rc=%d", var_id, oldId,
      tex_arg, get_managed_texture_refcount(tex_arg));
  }

  if (iid >= 0)
  {
    bool normValue = tex.tex != NULL;
    if constexpr (eastl::is_same_v<TEXTUREID, TextureType>)
    {
      normValue = normValue && (tex_arg != BAD_TEXTUREID);
    }
    globalData.globIntervalNormValues[iid] = static_cast<uint8_t>(normValue);
  }
  return true;
};


template <typename BufferType>
static bool set_buffer_internal(shaders_internal::Buf &buf, int var_id, int iid, BufferType buf_arg)
{
  auto &globalData = shGlobalData();

  D3DRESID oldId = buf.bufId;
  Sbuffer *oldPtr = buf.buf;
  bool isOldBufManaged = buf.isBufferManaged();

  if constexpr (eastl::is_same_v<D3DRESID, BufferType>)
  {
    buf.bufId = buf_arg;
    buf.buf = acquire_managed_buf(buf_arg);
  }
  else if constexpr (eastl::is_same_v<Sbuffer *, BufferType>)
  {
    buf.bufId = BAD_D3DRESID;
    buf.buf = buf_arg;

#if DAGOR_DBGLEVEL > 0
    if (buf_arg)
    {
      auto &newCounter = globalData.globResourceVarPointerUsageMap[buf_arg];
      ++newCounter;
    }
#endif
  }

  if (isOldBufManaged)
  {
    release_managed_res(oldId);
  }
  else
  {
#if DAGOR_DBGLEVEL > 0
    auto oldIt = globalData.globResourceVarPointerUsageMap.find(oldPtr);
    if (oldIt != globalData.globResourceVarPointerUsageMap.end())
    {
      if (--oldIt->second <= 0)
      {
        globalData.globResourceVarPointerUsageMap.erase(oldPtr);
      }
    }
#endif
  }

  if constexpr (eastl::is_same_v<D3DRESID, BufferType>)
  {
    G_ASSERTF(buf_arg == BAD_D3DRESID || get_managed_res_refcount(buf_arg) > 0, "set_buf(%d) %d->%d rc=%d", var_id, oldId, buf_arg,
      get_managed_res_refcount(buf_arg));
  }

  if (iid >= 0)
  {
    bool normValue = buf.buf != NULL;
    if constexpr (eastl::is_same_v<D3DRESID, BufferType>)
    {
      normValue = normValue && (buf_arg != BAD_D3DRESID);
    }
    globalData.globIntervalNormValues[iid] = static_cast<uint8_t>(normValue);
  }
  return true;
};

void ShaderVariableInfo::set_texture_ool(TEXTUREID tex_id) const
{
  G_STATIC_ASSERT(offsetof(shaders_internal::Tex, texId) == 0); // verifies header inline get
  auto &tex = *(shaders_internal::Tex *)data;
  set_texture_internal(tex, var_id, iid, tex_id);
}

void ShaderVariableInfo::set_texture_ool(BaseTexture *tex_ptr) const
{
  G_STATIC_ASSERT(offsetof(shaders_internal::Tex, texId) == 0); // verifies header inline get
  auto &tex = *(shaders_internal::Tex *)data;
  set_texture_internal(tex, var_id, iid, tex_ptr);
}

void ShaderVariableInfo::set_buffer_ool(D3DRESID buf_id) const
{
  G_STATIC_ASSERT(offsetof(shaders_internal::Buf, bufId) == 0); // verifies header inline get
  auto &buf = *(shaders_internal::Buf *)data;
  set_buffer_internal(buf, var_id, iid, buf_id);
}

void ShaderVariableInfo::set_buffer_ool(Sbuffer *buf_ptr) const
{
  G_STATIC_ASSERT(offsetof(shaders_internal::Buf, bufId) == 0); // verifies header inline get
  auto &buf = *(shaders_internal::Buf *)data;
  set_buffer_internal(buf, var_id, iid, buf_ptr);
}

bool ShaderGlobal::set_texture(const ShaderVariableInfo &v, const ManagedTex &texture) { return v.set_texture(texture.getTexId()); }

bool ShaderGlobal::set_buffer(const ShaderVariableInfo &v, const ManagedBuf &buffer) { return v.set_buffer(buffer.getBufId()); }

bool ShaderGlobal::set_int(int var_id, int v)
{
  CHECK_VAR_ID(SHVT_INT);
  globalData.globVarsState.set<int>(id, v);

  int iid = globalData.globVarIntervalIdx[id];
  set_global_interval_value(iid, v, shBinDump());
  return true;
}

int ShaderGlobal::get_var_type(int var_id)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed()) // it is unsafe to add variable and access in differnt thread without sync.
                                                           // That means, that relaxed load is sufficient.
    return -1;
  int id = shGlobalData().globvarIndexMap[var_id];
  if (id >= SHADERVAR_IDX_ABSENT)
    return -1;
  return shBinDump().globVars.getType(id);
}

static const shaderbindump::Interval *get_interval(int var_id)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed())
    return nullptr;

  const auto &globalData = shGlobalData();
  const int id = globalData.globvarIndexMap[var_id];
  if (id >= SHADERVAR_IDX_ABSENT)
    return nullptr;
  const int iid = globalData.globVarIntervalIdx[id];
  return iid >= 0 ? &shBinDump().intervals[iid] : nullptr;
}

bool ShaderGlobal::is_var_assumed(int var_id)
{
  auto interval = get_interval(var_id);
  if (!interval)
    return false;

  return interval->type == shaderbindump::Interval::TYPE_ASSUMED_INTERVAL;
}

int ShaderGlobal::get_interval_assumed_value(int var_id)
{
  G_ASSERT(is_var_assumed(var_id));
  auto interval = get_interval(var_id);
  if (!interval)
    return -1;
  return interval->getAssumedVal();
}

int ShaderGlobal::get_interval_current_value(int var_ir)
{
  if (ShaderGlobal::has_associated_interval(var_ir) && ShaderGlobal::is_var_assumed(var_ir))
    return ShaderGlobal::get_interval_assumed_value(var_ir);
  else
    return ShaderGlobal::get_int(var_ir);
}

bool ShaderGlobal::has_associated_interval(int var_id)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed())
    return false;
  const auto &globalData = shGlobalData();
  const int id = globalData.globvarIndexMap[var_id];
  return id != SHADERVAR_IDX_ABSENT ? globalData.globVarIntervalIdx[id] >= 0 : false;
}

dag::ConstSpan<float> ShaderGlobal::get_interval_ranges(int var_id)
{
  G_ASSERT(uint32_t(var_id) < shvarNameMap.nameCountRelaxed());
  const auto &globalData = shGlobalData();
  const auto &dump = *globalData.backing->getDump();
  const int id = globalData.globvarIndexMap[var_id];
  G_ASSERT_RETURN(id != SHADERVAR_IDX_ABSENT, {});
  const int iid = globalData.globVarIntervalIdx[id];
  G_ASSERT_RETURN(iid >= 0 && iid < dump.intervals.size(), {});
  return dump.intervals[iid].maxVal;
}

dag::ConstSpan<float> ShaderGlobal::get_interval_ranges(Interval interv)
{
  auto *dump = shBinDumpOwner().getDumpV2();
  G_ASSERTF(dump, "The current shader dump is not supported for calling `%s`", __FUNCTION__);
  auto &info = dump->getIntervalInfoByHash(interv.hash);
  return dump->intervals[info.intervalId].maxVal;
}

bool ShaderGlobal::set_float(int var_id, real v)
{
  CHECK_VAR_ID(SHVT_REAL);
  if (!shaders_internal::check_var_nan(v, var_id))
    return false;

  globalData.globVarsState.set<real>(id, v);

  int iid = globalData.globVarIntervalIdx[id];
  if (iid >= 0)
    globalData.globIntervalNormValues[iid] = dump.intervals[iid].getNormalizedValue(v);
  return true;
}

bool ShaderGlobal::set_float4(int var_id, const Point2 &rg, const Point2 &ba) { return set_float4(var_id, rg.x, rg.y, ba.x, ba.y); }
bool ShaderGlobal::set_float4(int var_id, const Point3 &rgb, float a) { return set_float4(var_id, rgb.x, rgb.y, rgb.z, a); }
bool ShaderGlobal::set_float4(int var_id, const Point4 &v) { return set_float4(var_id, v.x, v.y, v.z, v.w); }
bool ShaderGlobal::set_float4(int var_id, const Color3 &v, float a) { return set_float4(var_id, v.r, v.g, v.b, a); }
bool ShaderGlobal::set_float4(int var_id, const Color4 &v) { return set_float4(var_id, v.r, v.g, v.b, v.a); }
bool ShaderGlobal::set_float4(int var_id, E3DCOLOR c) { return set_float4(var_id, Color4(c)); }
bool ShaderGlobal::set_float4(int var_id, real r, real g, real b, real a)
{
  CHECK_VAR_ID(SHVT_COLOR4);

  if (!shaders_internal::check_var_nan(r + g + b + a, var_id))
    return false;

  globalData.globVarsState.set<Color4>(id, Color4(r, g, b, a));
  return true;
}
bool ShaderGlobal::set_float4_array(int var_id, const Color4 *data, int count)
{
  CHECK_VAR_ID(SHVT_COLOR4);
  eastl::copy_n(data, count, &globalData.globVarsState.get<Color4>(id));
  return true;
}
bool ShaderGlobal::set_float4_array(int var_id, const Point4 *data, int count)
{
  CHECK_VAR_ID(SHVT_COLOR4);
  eastl::copy_n(data, count, &globalData.globVarsState.get<Point4>(id));
  return true;
}
bool ShaderGlobal::set_float4x4(int var_id, const TMatrix4 &mat)
{
  CHECK_VAR_ID(SHVT_FLOAT4X4);
  globalData.globVarsState.set<TMatrix4>(id, mat);
  return true;
}
bool ShaderGlobal::set_int4(int var_id, const IPoint4 &v)
{
  CHECK_VAR_ID(SHVT_INT4);
  globalData.globVarsState.set<IPoint4>(id, v);
  return true;
}
bool ShaderGlobal::set_int4_array(int var_id, const IPoint4 *data, int count)
{
  CHECK_VAR_ID(SHVT_INT4);
  eastl::copy_n(data, count, &globalData.globVarsState.get<IPoint4>(id));
  return true;
}

#if DAGOR_DBGLEVEL > 0
bool ShaderGlobal::is_resource_used_as_umnamaged_pointer(D3dResource *resource_ptr, bool log_usage)
{
  auto &globalDump = shGlobalData();
  auto &state = globalDump.globVarsState;
  const auto &vars = globalDump.backing->getDump()->globVars;

  auto oldIt = globalDump.globResourceVarPointerUsageMap.find(resource_ptr);
  auto result = oldIt != globalDump.globResourceVarPointerUsageMap.end();

  if (!(result && log_usage))
    return result;

  dag::Vector<int> globVarIds;

  auto findVarIdByIndex = [&globalDump](int index) {
    auto varId = -1;
    for (int vid = 0; vid < globalDump.globvarIndexMap.size(); ++vid)
    {
      if (globalDump.globvarIndexMap[vid] == index)
      {
        varId = vid;
        break;
      }
    }
    G_ASSERT(varId != -1);
    return varId;
  };

  D3DRESID managedId = BAD_D3DRESID;
  const char *resourceName = "";

  for (int i = 0, e = state.size(); i < e; i++)
  {
    const auto type = vars.getType(i);
    D3DRESID managedId = BAD_D3DRESID;
    const char *resourceName = "";

    if (type == SHVT_TEXTURE)
    {
      const auto &tex = state.get<shaders_internal::Tex>(i);
      if (tex.tex == resource_ptr)
      {
        managedId = tex.texId;
        resourceName = tex.tex->getName();
        globVarIds.push_back(findVarIdByIndex(i));
      }
    }
    else if (type == SHVT_BUFFER)
    {
      const auto &buf = state.get<shaders_internal::Buf>(i);
      if (buf.buf == resource_ptr)
      {
        managedId = buf.bufId;
        resourceName = buf.buf->getName();
        globVarIds.push_back(findVarIdByIndex(i));
      }
    }
  }

  eastl::string varIdsStr = "[";
  for (size_t i = 0; i < globVarIds.size(); ++i)
  {
    if (i > 0)
    {
      varIdsStr += ", ";
    }
    varIdsStr += VariableMap::getVariableName(globVarIds[i]);
  }
  varIdsStr += "]";
  logerr("resource is used in shaderVars as unmanaged pointer:  [id: %d, managed name: %s, resource name: %s, "
         "globVars: %s]",
    managedId, get_managed_res_name(managedId), resourceName, varIdsStr);

  return result;
}
#endif

template <typename TextureType>
static bool set_texture_internal(int var_id, TextureType tex_arg)
{
  CHECK_VAR_ID(SHVT_TEXTURE);
  G_ASSERT_RETURN(id != SHADERVAR_IDX_ABSENT, false);

  auto &tex = globalData.globVarsState.get<shaders_internal::Tex>(id);
  return set_texture_internal(tex, var_id, globalData.globVarIntervalIdx[id], tex_arg);
};


template <typename BufferType>
static bool set_buffer_internal(int var_id, BufferType buf_arg)
{
#if DAGOR_DBGLEVEL > 0
  if (var_id == -1)
    logerr("do not try to set buffer to invalid variable!");
#endif

  CHECK_VAR_ID(SHVT_BUFFER);
  G_ASSERT_RETURN(id != SHADERVAR_IDX_ABSENT, false);

  auto &buf = globalData.globVarsState.get<shaders_internal::Buf>(id);
  return set_buffer_internal(buf, var_id, globalData.globVarIntervalIdx[id], buf_arg);
};

bool ShaderGlobal::set_texture(int var_id, TEXTUREID tex_id) { return set_texture_internal(var_id, tex_id); }

bool ShaderGlobal::set_texture(int var_id, BaseTexture *tex_ptr) { return set_texture_internal(var_id, tex_ptr); };

bool ShaderGlobal::set_texture(int variable_id, const ManagedTex &texture)
{
  return ShaderGlobal::set_texture(variable_id, texture.getTexId());
}

bool ShaderGlobal::set_sampler(int var_id, d3d::SamplerHandle handle)
{
  CHECK_VAR_ID(SHVT_SAMPLER);

  auto &smp = globalData.globVarsState.get<d3d::SamplerHandle>(id);
  smp = handle;
  return true;
}

bool ShaderGlobal::set_buffer(int var_id, D3DRESID buf_id) { return set_buffer_internal(var_id, buf_id); }

bool ShaderGlobal::set_buffer(int variable_id, Sbuffer *buffer_ptr) { return set_buffer_internal(variable_id, buffer_ptr); }

bool ShaderGlobal::set_buffer(int variable_id, const ManagedBuf &buffer)
{
  return ShaderGlobal::set_buffer(variable_id, buffer.getBufId());
}

bool ShaderGlobal::set_tlas(int var_id, RaytraceTopAccelerationStructure *tlas)
{
  CHECK_VAR_ID(SHVT_TLAS);

  auto &var = globalData.globVarsState.get<RaytraceTopAccelerationStructure *>(id);
  var = tlas;
  return true;
}

bool ShaderGlobal::set_float4(int var_id, const XMFLOAT2 &rg, const XMFLOAT2 &ba)
{
  return set_float4(var_id, rg.x, rg.y, ba.x, ba.y);
}

bool ShaderGlobal::set_float4(int var_id, const XMFLOAT3 &rgb, float a) { return set_float4(var_id, rgb.x, rgb.y, rgb.z, a); }

bool ShaderGlobal::set_float4(int var_id, const XMFLOAT4 &v) { return set_float4(var_id, v.x, v.y, v.z, v.w); }

bool ShaderGlobal::set_float4x4(int var_id, const XMFLOAT4X4 &mat)
{
  CHECK_VAR_ID(SHVT_FLOAT4X4);
  globalData.globVarsState.set<XMFLOAT4X4>(id, mat);
  return true;
}

bool ShaderGlobal::set_float4(int var_id, FXMVECTOR v)
{
  CHECK_VAR_ID(SHVT_COLOR4);

  if (!shaders_internal::check_var_nan(XMVectorGetX(XMVectorSum(v)), var_id))
    return false;

  XMFLOAT4A &f4 = globalData.globVarsState.get<XMFLOAT4A>(id);
  XMStoreFloat4A(&f4, v);

  return true;
}

bool ShaderGlobal::set_float4x4(int var_id, FXMMATRIX mat)
{
  CHECK_VAR_ID(SHVT_FLOAT4X4);

  XMFLOAT4X4A &f44 = globalData.globVarsState.get<XMFLOAT4X4A>(id);
  XMStoreFloat4x4A(&f44, mat);

  return true;
}

#undef CHECK_VAR_ID

#define CHECK_VAR_ID(TYPE, DEF_VAL)                                                                      \
  if (var_id < 0)                                                                                        \
    return DEF_VAL;                                                                                      \
  const auto &globalData = shGlobalData();                                                               \
  const auto &dump = *globalData.backing->getDump();                                                     \
  if (!dump.globVars.size())                                                                             \
  {                                                                                                      \
    logerr("shaders not loaded while getting var_id=%d (%s)", var_id, shvarNameMap.getName(var_id));     \
    return DEF_VAL;                                                                                      \
  }                                                                                                      \
  int glob_var_id = globalData.globvarIndexMap[var_id];                                                  \
  if (glob_var_id == SHADERVAR_IDX_ABSENT)                                                               \
    return DEF_VAL;                                                                                      \
  G_ASSERTF_RETURN(glob_var_id < dump.globVars.size(), DEF_VAL, "var_id=%d (%s) glob_var_id=%d", var_id, \
    shvarNameMap.getName(var_id), glob_var_id);                                                          \
  G_ASSERTF_RETURN(dump.globVars.getType(glob_var_id) == TYPE, DEF_VAL, "var_id=%d (%s)", var_id, shvarNameMap.getName(var_id));

int ShaderGlobal::get_int_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_INT, 0);
  return globalData.globVarsState.get<int>(glob_var_id);
}
real ShaderGlobal::get_float(int var_id)
{
  CHECK_VAR_ID(SHVT_REAL, 0.0f);
  return globalData.globVarsState.get<real>(glob_var_id);
}

Color4 ShaderGlobal::get_float4(int var_id)
{
  CHECK_VAR_ID(SHVT_COLOR4, ZERO<Color4>());
  return globalData.globVarsState.get<Color4>(glob_var_id);
}

TMatrix4 ShaderGlobal::get_float4x4(int var_id)
{
  CHECK_VAR_ID(SHVT_FLOAT4X4, ZERO<TMatrix4>());
  return globalData.globVarsState.get<TMatrix4>(glob_var_id);
}

IPoint4 ShaderGlobal::get_int4(int var_id)
{
  CHECK_VAR_ID(SHVT_INT4, IPoint4::ZERO);
  return globalData.globVarsState.get<IPoint4>(glob_var_id);
}

TEXTUREID ShaderGlobal::get_tex_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_TEXTURE, BAD_TEXTUREID);
  return globalData.globVarsState.get<shaders_internal::Tex>(glob_var_id).texId;
}

BaseTexture *ShaderGlobal::get_tex_ptr_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_TEXTURE, nullptr);
  return globalData.globVarsState.get<shaders_internal::Tex>(glob_var_id).tex;
}

D3DRESID ShaderGlobal::get_buf_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_BUFFER, BAD_D3DRESID);
  return globalData.globVarsState.get<shaders_internal::Buf>(glob_var_id).bufId;
}

Sbuffer *ShaderGlobal::get_buf_ptr_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_BUFFER, nullptr);
  return globalData.globVarsState.get<shaders_internal::Buf>(glob_var_id).buf;
}

d3d::SamplerHandle ShaderGlobal::get_sampler(int var_id)
{
  CHECK_VAR_ID(SHVT_SAMPLER, d3d::INVALID_SAMPLER_HANDLE);
  return globalData.globVarsState.get<d3d::SamplerHandle>(glob_var_id);
}

// shader binary dump
template <>
const shaderbindump::ShaderClass *ScriptedShadersBinDump::findShaderClass(const char *name) const
{
  int id = mapbinarysearch::bfindStrId(shaderNameMap, name);
  return id == -1 ? nullptr : const_cast<shaderbindump::ShaderClass *>(&classes[id]);
}
#undef CHECK_VAR_ID

template <>
int shaderbindump::VariantTable::qfindIntervalVariant(unsigned code) const
{
  int halfsz = mapData.size() / 2;
  if (halfsz == 0)
    return FIND_NOTFOUND;
  auto found = eastl::upper_bound(mapData.begin(), mapData.begin() + halfsz, code);
  if (found == mapData.begin())
    return FIND_NOTFOUND;
  --found;
  auto index = eastl::distance(mapData.begin(), found);
  return mapData[index + halfsz];
}

template <>
int shaderbindump::VariantTable::qfindDirectVariant(unsigned code) const
{
  int halfsz = mapData.size() / 2;
  auto found = eastl::binary_search_i(mapData.begin(), mapData.begin() + halfsz, code);
  return found == mapData.begin() + halfsz ? FIND_NOTFOUND : mapData[eastl::distance(mapData.begin(), found) + halfsz];
}

template <>
int shaderbindump::ShaderCode::findVar(int var_id) const
{
  return bfind_packed_uint16_x2_fast(stVarMap, var_id);
}

#if DAGOR_DBGLEVEL > 0
void shaderbindump::dump_shader_var_names()
{
  uint32_t cnt = shvarNameMap.nameCountAcquire();
  debug("shvarNameMap.nameCount()=%d:", cnt);
  int wd = cnt >= 1000 ? 4 : (cnt >= 100 ? 3 : (cnt >= 10 ? 2 : 1));
  const auto &globalData = ::shGlobalData();
  for (int i = 0; i < cnt; i++)
  {
    debug("  %*d [%c] %s", wd, i,
      globalData.globvarIndexMap[i] < SHADERVAR_IDX_ABSENT ? 'G' : (globalData.varIndexMap[i] < SHADERVAR_IDX_ABSENT ? 'v' : '?'),
      shvarNameMap.getName(i));
  }
}
#endif
