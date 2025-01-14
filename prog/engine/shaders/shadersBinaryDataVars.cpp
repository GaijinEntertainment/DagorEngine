// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_PC_WIN
#undef _DEBUG_TAB_ // exec_stcode if called too frequently to allow any perfomance penalty there
#endif
#include "mapBinarySearch.h"
#include "shadersBinaryData.h"
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

void ScriptedShadersBinDumpOwner::initVarIndexMaps(size_t max_shadervar_cnt)
{
  varIndexMap.resize(max_shadervar_cnt);
  globvarIndexMap.resize(max_shadervar_cnt);
  int nameCount = shvarNameMap.iterate([&](int nid, const char *name) {
    varIndexMap[nid] = mapbinarysearch::bfindStrId(mShaderDump->varMap, name);
    globvarIndexMap[nid] = (varIndexMap[nid] == SHADERVAR_IDX_INVALID) ? SHADERVAR_IDX_ABSENT
                                                                       : bfind_packed_uint16_x2(mShaderDump->gvMap, varIndexMap[nid]);
  });
  eastl::fill(varIndexMap.begin() + nameCount, varIndexMap.end(), SHADERVAR_IDX_ABSENT);
  eastl::fill(globvarIndexMap.begin() + nameCount, globvarIndexMap.end(), SHADERVAR_IDX_ABSENT);
}

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
  auto &dumpOwner = shBinDumpExOwner(!sec_dump);
  const auto &dump = *dumpOwner.getDump();
#if DAGOR_DBGLEVEL
  if (DAGOR_UNLIKELY(nid >= dumpOwner.maxShadervarCnt()))
    shaderbindump::dump_shader_var_names();
#endif
  G_ASSERTF_RETURN(nid < dumpOwner.maxShadervarCnt(), -1, "var_name=%s shvarNameMap.nameCount=%d", var_name,
    shvarNameMap.nameCountRelaxed()); // it is unsafe to add variable and access in differnt thread without sync. That means, that
                                      // relaxed load is sufficient.
  if (dumpOwner.varIndexMap[nid] == SHADERVAR_IDX_ABSENT)
  {
    uint16_t var_id = mapbinarysearch::bfindStrId(dump.varMap, var_name);
#if DAGOR_DBGLEVEL
    if (var_id == SHADERVAR_IDX_INVALID && !shadervar_name_is_valid(var_name))
      logerr("bad var_name=%s for %s", var_name, __FUNCTION__);
#endif
    dumpOwner.varIndexMap[nid] = var_id;
    dumpOwner.globvarIndexMap[nid] = (dumpOwner.varIndexMap[nid] == SHADERVAR_IDX_INVALID)
                                       ? SHADERVAR_IDX_ABSENT
                                       : bfind_packed_uint16_x2(dump.gvMap, dumpOwner.varIndexMap[nid]);
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
  return shBinDumpOwner().varIndexMap[var_id] < SHADERVAR_IDX_ABSENT;
}

bool VariableMap::isGlobVariablePresent(int var_id)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed()) // it is unsafe to add variable and access in differnt thread without sync.
                                                           // That means, that relaxed load is sufficient.
    return false;
  return shBinDumpOwner().globvarIndexMap[var_id] < SHADERVAR_IDX_ABSENT;
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
  auto &dumpOwner = shBinDumpOwner();                                                                                               \
  auto &dump = shBinDumpRW();                                                                                                       \
  if (!dump.globVars.size())                                                                                                        \
  {                                                                                                                                 \
    logerr("shaders not loaded while setting var_id=%d (%s)", var_id, shvarNameMap.getName(var_id));                                \
    return false;                                                                                                                   \
  }                                                                                                                                 \
  int id = dumpOwner.globvarIndexMap[var_id];                                                                                       \
  if (id == SHADERVAR_IDX_ABSENT)                                                                                                   \
    return false;                                                                                                                   \
  G_ASSERTF_RETURN(id < dump.globVars.size(), false, "%s invalid var_id %d (to glob var %d)", __FUNCTION__, var_id, id);            \
  G_ASSERTF_RETURN(dump.globVars.getType(id) == TYPE, false, "var_id=%d (%s) glob_var_id=%d", var_id, shvarNameMap.getName(var_id), \
    id);

inline void set_global_interval_value_internal(int index, int v, ScriptedShadersBinDump &dump)
{
  shaderbindump::Interval &interval = dump.intervals[index];
#if DAGOR_DBGLEVEL > 0
  if (interval.type == shaderbindump::Interval::TYPE_VIOLATED_ASSUMED_INTERVAL)
    return;
#endif
  if (DAGOR_UNLIKELY(interval.type == shaderbindump::Interval::TYPE_ASSUMED_INTERVAL))
  {
#if DAGOR_DBGLEVEL > 0
    if (interval.getAssumedVal() != v)
    {
      logwarn("`Interval Assume` violation for %s. This was assumed to %i, but set to %i", dump.varMap[interval.nameId].c_str(),
        interval.getAssumedVal(), v);
      interval.type = shaderbindump::Interval::TYPE_VIOLATED_ASSUMED_INTERVAL;
    }
#endif
    return;
  }
  shBinDumpOwner().globIntervalNormValues[index] = interval.getNormalizedValue(v);
}

__forceinline void set_global_interval_value(int index, int v, ScriptedShadersBinDump &dump)
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
  shBinDumpOwner().globIntervalNormValues[info.intervalId] = index;

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
  uint8_t index = shBinDumpOwner().globIntervalNormValues[info.intervalId];
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
      if (!VariableMap::generation()) // no sense in trying to resolve, shader dump is not loaded
        return;
      const int vid = VariableMap::getVariableId(data);
      if (vid >= 0)
        var_id = vid;
    }
  }
  if (var_id < 0)
    return;
  if (!optional && !VariableMap::isVariablePresent(var_id) && !dgs_all_shader_vars_optionals)
    logerr("shader variable %s is mandatory (not optional), but is not present in shaders dump", getName());
  data = nullptr;
  iid = -1;
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed())
  {
    G_ASSERTF(0, "%s invalid var_id %d", __FUNCTION__, var_id);
    return;
  }
  auto &dumpOwner = shBinDumpOwner();
  auto &dump = shBinDumpRW();
  if (!dump.globVars.size())
  {
    logerr("shaders not loaded while setting var_id=%d (%s)", var_id, shvarNameMap.getName(var_id));
    return;
  }
  int id = dumpOwner.globvarIndexMap[var_id];
  if (id == SHADERVAR_IDX_ABSENT)
    return;
  if (id == SHADERVAR_IDX_INVALID)
  {
#if DAGOR_DBGLEVEL
    if (!shadervar_name_is_valid(shvarNameMap.getName(var_id)))
      logerr("bad var_name=%s for %s", shvarNameMap.getName(var_id), __FUNCTION__);
#endif
    return;
  }
  data = &dump.globVars.get<char>(id);
  varType = dump.globVars.getType(id);
  iid = dumpOwner.globVarIntervalIdx[id];
}

void ShaderVariableInfo::nanError() const { logerr("setting <%s> variable to nan", VariableMap::getVariableName(var_id)); }

void ShaderVariableInfo::set_int_var_interval(int v) const { set_global_interval_value_internal(iid, v, shBinDumpRW()); }

void ShaderVariableInfo::set_float_var_interval(float v) const
{
  shBinDumpOwner().globIntervalNormValues[iid] = shBinDump().intervals[iid].getNormalizedValue(v);
}

void ShaderVariableInfo::set_texture_ool(TEXTUREID tex_id) const
{
  G_STATIC_ASSERT(offsetof(shaders_internal::Tex, texId) == 0); // verifies header inline get
  auto &tex = *(shaders_internal::Tex *)data;
  TEXTUREID old_id = tex.texId;
  tex.texId = tex_id;
  tex.tex = acquire_managed_tex(tex_id);
  release_managed_tex(old_id);
  G_ASSERTF(tex_id == BAD_TEXTUREID || get_managed_texture_refcount(tex_id) > 0, "set_tex(%d) %d->%d rc=%d", var_id, old_id, tex_id,
    get_managed_texture_refcount(tex_id));

  if (iid >= 0)
    shBinDumpOwner().globIntervalNormValues[iid] = (tex_id != BAD_TEXTUREID && tex.tex);
}

void ShaderVariableInfo::set_buffer_ool(D3DRESID buf_id) const
{
  G_STATIC_ASSERT(offsetof(shaders_internal::Buf, bufId) == 0); // verifies header inline get
  auto &buf = *(shaders_internal::Buf *)data;
  D3DRESID old_id = buf.bufId;
  buf.bufId = buf_id;
  buf.buf = acquire_managed_buf(buf_id);
  release_managed_res(old_id);
  G_ASSERTF(buf_id == BAD_D3DRESID || get_managed_res_refcount(buf_id) > 0, "set_tex(%d) %d->%d rc=%d", var_id, old_id, buf_id,
    get_managed_res_refcount(buf_id));

  if (iid >= 0)
    shBinDumpOwner().globIntervalNormValues[iid] = (buf_id != BAD_D3DRESID && buf.buf);
}

bool ShaderGlobal::set_texture(const ShaderVariableInfo &v, const ManagedTex &texture) { return v.set_texture(texture.getTexId()); }

bool ShaderGlobal::set_buffer(const ShaderVariableInfo &v, const ManagedBuf &buffer) { return v.set_buffer(buffer.getBufId()); }

bool ShaderGlobal::set_int(int var_id, int v)
{
  CHECK_VAR_ID(SHVT_INT);
  dump.globVars.set<int>(id, v);

  int iid = shBinDumpOwner().globVarIntervalIdx[id];
  set_global_interval_value(iid, v, dump);
  return true;
}

bool ShaderGlobal::get_color4_by_name(const char *name, Color4 &val)
{
  int id = VariableMap::getVariableId(name);
#if DAGOR_DBGLEVEL > 0
  if (!VariableMap::isVariablePresent(id)) // we check only var validity here (not checking that var is global) to allow asserts on
                                           // wrong calls
#else
  if (!VariableMap::isGlobVariablePresent(id))
#endif
    return false;
  val = get_color4_fast(id);
  return true;
}

int ShaderGlobal::get_var_type(int var_id)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed()) // it is unsafe to add variable and access in differnt thread without sync.
                                                           // That means, that relaxed load is sufficient.
    return -1;
  int id = shBinDumpOwner().globvarIndexMap[var_id];
  if (id >= SHADERVAR_IDX_ABSENT)
    return -1;
  return shBinDump().globVars.getType(id);
}

static const shaderbindump::Interval *get_interval(int var_id)
{
  if (uint32_t(var_id) >= shvarNameMap.nameCountRelaxed())
    return nullptr;

  const auto &dumpOwner = shBinDumpOwner();
  int id = dumpOwner.globvarIndexMap[var_id];
  return id != SHADERVAR_IDX_ABSENT ? &shBinDump().intervals[dumpOwner.globVarIntervalIdx[id]] : nullptr;
}

bool ShaderGlobal::is_var_assumed(int var_id)
{
  auto interval = get_interval(var_id);
  if (!interval)
    return false;

  return (interval->type == shaderbindump::Interval::TYPE_VIOLATED_ASSUMED_INTERVAL) ||
         (interval->type == shaderbindump::Interval::TYPE_ASSUMED_INTERVAL);
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
  const auto &dumpOwner = shBinDumpOwner();
  int id = dumpOwner.globvarIndexMap[var_id];
  return id != SHADERVAR_IDX_ABSENT ? dumpOwner.globVarIntervalIdx[id] >= 0 : false;
}

dag::ConstSpan<float> ShaderGlobal::get_interval_ranges(int var_id)
{
  G_ASSERT(uint32_t(var_id) < shvarNameMap.nameCountRelaxed());
  const auto &dumpOwner = shBinDumpOwner();
  const auto &dump = shBinDump();
  int id = dumpOwner.globvarIndexMap[var_id];
  G_ASSERT_RETURN(id != SHADERVAR_IDX_ABSENT, {});
  int iid = dumpOwner.globVarIntervalIdx[id];
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

bool ShaderGlobal::set_real(int var_id, real v)
{
  CHECK_VAR_ID(SHVT_REAL);
  if (!shaders_internal::check_var_nan(v, var_id))
    return false;

  dump.globVars.set<real>(id, v);

  int iid = shBinDumpOwner().globVarIntervalIdx[id];
  if (iid >= 0)
    shBinDumpOwner().globIntervalNormValues[iid] = dump.intervals[iid].getNormalizedValue(v);
  return true;
}

bool ShaderGlobal::set_color4(int var_id, const Point2 &rg, const Point2 &ba) { return set_color4(var_id, rg.x, rg.y, ba.x, ba.y); }
bool ShaderGlobal::set_color4(int var_id, const Point3 &rgb, float a) { return set_color4(var_id, rgb.x, rgb.y, rgb.z, a); }
bool ShaderGlobal::set_color4(int var_id, const Point4 &v) { return set_color4(var_id, v.x, v.y, v.z, v.w); }
bool ShaderGlobal::set_color4(int var_id, const Color3 &v, float a) { return set_color4(var_id, v.r, v.g, v.b, a); }
bool ShaderGlobal::set_color4(int var_id, const Color4 &v) { return set_color4(var_id, v.r, v.g, v.b, v.a); }
bool ShaderGlobal::set_color4(int var_id, E3DCOLOR c) { return set_color4(var_id, Color4(c)); }
bool ShaderGlobal::set_color4(int var_id, real r, real g, real b, real a)
{
  CHECK_VAR_ID(SHVT_COLOR4);

  if (!shaders_internal::check_var_nan(r + g + b + a, var_id))
    return false;

  dump.globVars.set<Color4>(id, Color4(r, g, b, a));
  return true;
}
bool ShaderGlobal::set_color4_array(int var_id, const Color4 *data, int count)
{
  CHECK_VAR_ID(SHVT_COLOR4);
  eastl::copy_n(data, count, &dump.globVars.get<Color4>(id));
  return true;
}
bool ShaderGlobal::set_color4_array(int var_id, const Point4 *data, int count)
{
  CHECK_VAR_ID(SHVT_COLOR4);
  eastl::copy_n(data, count, &dump.globVars.get<Point4>(id));
  return true;
}
bool ShaderGlobal::set_float4x4(int var_id, const TMatrix4 &mat)
{
  CHECK_VAR_ID(SHVT_FLOAT4X4);
  dump.globVars.set<TMatrix4>(id, mat);
  return true;
}
bool ShaderGlobal::set_int4(int var_id, const IPoint4 &v)
{
  CHECK_VAR_ID(SHVT_INT4);
  dump.globVars.set<IPoint4>(id, v);
  return true;
}
bool ShaderGlobal::set_int4_array(int var_id, const IPoint4 *data, int count)
{
  CHECK_VAR_ID(SHVT_INT4);
  eastl::copy_n(data, count, &dump.globVars.get<IPoint4>(id));
  return true;
}

bool ShaderGlobal::set_texture(int var_id, TEXTUREID tex_id)
{
  CHECK_VAR_ID(SHVT_TEXTURE);

  auto &tex = dump.globVars.get<shaders_internal::Tex>(id);
  TEXTUREID old_id = tex.texId;
  tex.texId = tex_id;
  tex.tex = acquire_managed_tex(tex_id);
  release_managed_tex(old_id);
  G_ASSERTF(tex_id == BAD_TEXTUREID || get_managed_texture_refcount(tex_id) > 0, "set_tex(%d) %d->%d rc=%d", var_id, old_id, tex_id,
    get_managed_texture_refcount(tex_id));

  G_ASSERT_RETURN(id != SHADERVAR_IDX_ABSENT, false);
  int iid = shBinDumpOwner().globVarIntervalIdx[id];
  if (iid >= 0)
    shBinDumpOwner().globIntervalNormValues[iid] = (tex_id != BAD_TEXTUREID && tex.tex);
  return true;
}

bool ShaderGlobal::set_texture(int variable_id, const ManagedTex &texture)
{
  return ShaderGlobal::set_texture(variable_id, texture.getTexId());
}

bool ShaderGlobal::set_sampler(int var_id, d3d::SamplerHandle handle)
{
  CHECK_VAR_ID(SHVT_SAMPLER);

  auto &smp = dump.globVars.get<d3d::SamplerHandle>(id);
  smp = handle;
  return true;
}

bool ShaderGlobal::set_buffer(int var_id, D3DRESID buf_id)
{
#if DAGOR_DBGLEVEL > 0
  if (var_id == -1)
    logerr("do not try to set buffer to invalid variable!");
#endif
  CHECK_VAR_ID(SHVT_BUFFER);

  auto &buf = dump.globVars.get<shaders_internal::Buf>(id);
  D3DRESID old_id = buf.bufId;
  buf.bufId = buf_id;
  buf.buf = acquire_managed_buf(buf_id);
  release_managed_res(old_id);
  G_ASSERTF(buf_id == BAD_D3DRESID || get_managed_res_refcount(buf_id) > 0, "set_tex(%d) %d->%d rc=%d", var_id, old_id, buf_id,
    get_managed_res_refcount(buf_id));

  G_ASSERT_RETURN(id != SHADERVAR_IDX_ABSENT, false);
  int iid = shBinDumpOwner().globVarIntervalIdx[id];
  if (iid >= 0)
    shBinDumpOwner().globIntervalNormValues[iid] = (buf_id != BAD_D3DRESID && buf.buf);
  return true;
}

bool ShaderGlobal::set_buffer(int variable_id, const ManagedBuf &buffer)
{
  return ShaderGlobal::set_buffer(variable_id, buffer.getBufId());
}

bool ShaderGlobal::set_tlas(int var_id, RaytraceTopAccelerationStructure *tlas)
{
  CHECK_VAR_ID(SHVT_TLAS);

  auto &var = dump.globVars.get<RaytraceTopAccelerationStructure *>(id);
  var = tlas;
  return true;
}

bool ShaderGlobal::set_color4(int var_id, const XMFLOAT2 &rg, const XMFLOAT2 &ba)
{
  return set_color4(var_id, rg.x, rg.y, ba.x, ba.y);
}

bool ShaderGlobal::set_color4(int var_id, const XMFLOAT3 &rgb, float a) { return set_color4(var_id, rgb.x, rgb.y, rgb.z, a); }

bool ShaderGlobal::set_color4(int var_id, const XMFLOAT4 &v) { return set_color4(var_id, v.x, v.y, v.z, v.w); }

bool ShaderGlobal::set_float4x4(int var_id, const XMFLOAT4X4 &mat)
{
  CHECK_VAR_ID(SHVT_FLOAT4X4);
  dump.globVars.set<XMFLOAT4X4>(id, mat);
  return true;
}

bool ShaderGlobal::set_color4(int var_id, FXMVECTOR v)
{
  CHECK_VAR_ID(SHVT_COLOR4);

  if (!shaders_internal::check_var_nan(XMVectorGetX(XMVectorSum(v)), var_id))
    return false;

  XMFLOAT4A &f4 = dump.globVars.get<XMFLOAT4A>(id);
  XMStoreFloat4A(&f4, v);

  return true;
}

bool ShaderGlobal::set_float4x4(int var_id, FXMMATRIX mat)
{
  CHECK_VAR_ID(SHVT_FLOAT4X4);

  XMFLOAT4X4A &f44 = dump.globVars.get<XMFLOAT4X4A>(id);
  XMStoreFloat4x4A(&f44, mat);

  return true;
}

#undef CHECK_VAR_ID

#define CHECK_VAR_ID(TYPE, DEF_VAL)                                                                      \
  if (var_id < 0)                                                                                        \
    return DEF_VAL;                                                                                      \
  const auto &dump = shBinDump();                                                                        \
  if (!dump.globVars.size())                                                                             \
  {                                                                                                      \
    logerr("shaders not loaded while getting var_id=%d (%s)", var_id, shvarNameMap.getName(var_id));     \
    return DEF_VAL;                                                                                      \
  }                                                                                                      \
  int glob_var_id = shBinDumpOwner().globvarIndexMap[var_id];                                            \
  if (glob_var_id == SHADERVAR_IDX_ABSENT)                                                               \
    return DEF_VAL;                                                                                      \
  G_ASSERTF_RETURN(glob_var_id < dump.globVars.size(), DEF_VAL, "var_id=%d (%s) glob_var_id=%d", var_id, \
    shvarNameMap.getName(var_id), glob_var_id);                                                          \
  G_ASSERTF_RETURN(dump.globVars.getType(glob_var_id) == TYPE, DEF_VAL, "var_id=%d (%s)", var_id, shvarNameMap.getName(var_id));

int ShaderGlobal::get_int_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_INT, 0);
  return dump.globVars.get<int>(glob_var_id);
}
real ShaderGlobal::get_real_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_REAL, 0.0f);
  return dump.globVars.get<real>(glob_var_id);
}

Color4 ShaderGlobal::get_color4_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_COLOR4, ZERO<Color4>());
  return dump.globVars.get<Color4>(glob_var_id);
}
Color4 ShaderGlobal::get_color4(int glob_var_id) { return get_color4_fast(glob_var_id); }

TMatrix4 ShaderGlobal::get_float4x4(int var_id)
{
  CHECK_VAR_ID(SHVT_FLOAT4X4, ZERO<TMatrix4>());
  return dump.globVars.get<TMatrix4>(glob_var_id);
}

IPoint4 ShaderGlobal::get_int4(int var_id)
{
  CHECK_VAR_ID(SHVT_INT4, IPoint4::ZERO);
  return dump.globVars.get<IPoint4>(glob_var_id);
}

TEXTUREID ShaderGlobal::get_tex_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_TEXTURE, BAD_TEXTUREID);
  return dump.globVars.getTex(glob_var_id).texId;
}

D3DRESID ShaderGlobal::get_buf_fast(int var_id)
{
  CHECK_VAR_ID(SHVT_BUFFER, BAD_D3DRESID);
  return dump.globVars.getBuf(glob_var_id).bufId;
}

d3d::SamplerHandle ShaderGlobal::get_sampler(int var_id)
{
  CHECK_VAR_ID(SHVT_SAMPLER, d3d::INVALID_SAMPLER_HANDLE);
  return dump.globVars.get<d3d::SamplerHandle>(glob_var_id);
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
  auto found = eastl::upper_bound(mapData.begin(), mapData.begin() + halfsz, code);
  --found;
  auto index = eastl::distance(mapData.begin(), found);
  return found == mapData.end() ? FIND_NOTFOUND : mapData[index + halfsz];
}

template <>
int shaderbindump::VariantTable::qfindDirectVariant(unsigned code) const
{
  int halfsz = mapData.size() / 2;
  auto found = eastl::binary_search_i(mapData.begin(), mapData.begin() + halfsz, code);
  return found == mapData.end() ? FIND_NOTFOUND : mapData[eastl::distance(mapData.begin(), found) + halfsz];
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
  const auto &dumpOwner = ::shBinDumpOwner();
  for (int i = 0; i < cnt; i++)
  {
    debug("  %*d [%c] %s", wd, i,
      dumpOwner.globvarIndexMap[i] < SHADERVAR_IDX_ABSENT ? 'G' : (dumpOwner.varIndexMap[i] < SHADERVAR_IDX_ABSENT ? 'v' : '?'),
      shvarNameMap.getName(i));
  }
}
#endif
