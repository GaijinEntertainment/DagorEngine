// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "namedConst.h"
#include "globalConfig.h"
#include "shsyn.h"
#include "shLog.h"
#include "shsem.h"
#include "linkShaders.h"
#include "cppStcode.h"
#include "const3d.h"
#include <math/dag_mathBase.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shUtils.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>
#include "hash.h"
#include "fast_isalnum.h"
#include <EASTL/bitvector.h>
#include <EASTL/string_view.h>
#include <algorithm>

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
#endif

static constexpr const char *EMPTY_BLOCK_NAME = "__empty_block__";

static String tmpDigest;

const char *NamedConstBlock::nameSpaceToStr(NamedConstSpace name_space)
{
  switch (name_space)
  {
    case NamedConstSpace::vsf: return "@vsf";
    case NamedConstSpace::vsmp: return "@vsmp";
    case NamedConstSpace::psf: return "@psf";
    case NamedConstSpace::smp: return "@smp";
    case NamedConstSpace::sampler: return "@sampler";
    case NamedConstSpace::csf: return "@csf";
    case NamedConstSpace::uav:
    case NamedConstSpace::vs_uav: return "@uav";
    case NamedConstSpace::vs_buf: return "@vs_buf";
    case NamedConstSpace::ps_buf: return "@ps_buf";
    case NamedConstSpace::cs_buf: return "@cs_buf";
    case NamedConstSpace::vs_cbuf: return "@vs_cbuf";
    case NamedConstSpace::ps_cbuf: return "@ps_cbuf";
    case NamedConstSpace::cs_cbuf: return "@cs_cbuf";
    default: return "?unk?";
  }
}

static int remap_register(int r, int cnt, NamedConstSpace ns)
{
  if (ns == NamedConstSpace::uav)
  {
    // reversed order of slots to avoid overlap with RTV
    r = 7 - r - cnt + 1;
  }
  else if (ns == NamedConstSpace::ps_buf || ns == NamedConstSpace::vs_buf || ns == NamedConstSpace::cs_buf)
    r += 16;
  else if (ns == NamedConstSpace::ps_cbuf || ns == NamedConstSpace::vs_cbuf || ns == NamedConstSpace::cs_cbuf)
    r += 1;
  return r;
}

static int translate_namespace(NamedConstSpace nc)
{
  switch (nc)
  {
    case NamedConstSpace::vsmp:
    case NamedConstSpace::vs_buf: return (int)NamedConstSpace::vsmp;
    case NamedConstSpace::smp:
    case NamedConstSpace::ps_buf:
    case NamedConstSpace::cs_buf: return (int)NamedConstSpace::smp;
    default: return (int)nc;
  }
}

int NamedConstBlock::addConst(int stage, const char *name, NamedConstSpace name_space, int sz, int hardcoded_reg, String &&hlsl_decl)
{
  G_ASSERT(sz > 0);

  auto &props = stage == STAGE_VS ? vertexProps : pixelProps;
  auto &xsn = props.sn;
  auto &xsc = props.sc;
  int id = xsn.getNameId(name);
  if (id == -1)
  {
    if (hardcoded_reg >= 0)
    {
      for (int r = hardcoded_reg; r < hardcoded_reg + sz; r++)
      {
        bool inserted = props.hardcoded_regs[translate_namespace(name_space)].emplace(r).second;
        if (!inserted)
          sh_debug(SHLOG_ERROR, "Constant '%s' is hardcoded with register '%i', which is already occupied", name, r);
      }
    }
    id = xsn.addNameId(name);
    xsc.resize(id + 1);
    xsc[id].nameSpace = name_space;
    xsc[id].size = sz;
    xsc[id].regIndex = hardcoded_reg;
    xsc[id].isDynamic = true;
    xsc[id].hlsl = eastl::move(hlsl_decl);
    return id;
  }
  return -1;
}

void NamedConstBlock::addHlslPostCode(const char *postfix_hlsl)
{
  if (!postfix_hlsl)
    return;
  vertexProps.postfixHlsl.push_back(postfix_hlsl);
  pixelProps.postfixHlsl.push_back(postfix_hlsl);
}

void NamedConstBlock::markStatic(int id, NamedConstSpace name_space)
{
  switch (name_space)
  {
    case NamedConstSpace::vsf:
    case NamedConstSpace::vsmp:
    case NamedConstSpace::vs_uav:
      if (id >= 0 && id < vertexProps.sc.size())
        vertexProps.sc[id].isDynamic = false;
      break;

    case NamedConstSpace::psf:
    case NamedConstSpace::smp:
    case NamedConstSpace::csf:
    case NamedConstSpace::uav:
      if (id >= 0 && id < pixelProps.sc.size())
        pixelProps.sc[id].isDynamic = false;
      break;

    default: DAG_FATAL("unsupported namespace: %d", (int)name_space);
  }
}

int NamedConstBlock::arrangeRegIndices(int base_reg, NamedConstSpace name_space, int *base_cblock_reg)
{
  if (base_cblock_reg)
  {
    switch (name_space)
    {
      case NamedConstSpace::vsf:
        for (NamedConst &nc : vertexProps.sc)
        {
          if (nc.nameSpace == name_space && !nc.isDynamic)
          {
            nc.regIndex = *base_cblock_reg;
            *base_cblock_reg += nc.size;
          }
        }
        break;
      case NamedConstSpace::psf:
        for (int i = 0; i < pixelProps.sc.size(); i++)
        {
          NamedConst &nc = pixelProps.sc[i];
          if (nc.nameSpace == name_space && !nc.isDynamic)
          {
            if (vertexProps.sn.getNameId(pixelProps.sn.getName(i)) >= 0) // skip duplicate VS/PS vars
              nc.regIndex = 0xFF;
            else
            {
              nc.regIndex = *base_cblock_reg;
              *base_cblock_reg += nc.size;
            }
          }
        }
        break;
    }
  }
  const bool needShift = (name_space == NamedConstSpace::cs_cbuf || // is cbuf
                           name_space == NamedConstSpace::ps_cbuf || name_space == NamedConstSpace::vs_cbuf) &&
                         ShaderStateBlock::countBlock(ShaderStateBlock::LEV_GLOBAL_CONST) > 0; // does global const block exist

  auto find_first_free_reg = [&base_reg, name_space](auto &hardcoded_regs, int size) {
    for (int start_r = base_reg; true; start_r++)
    {
      int r = start_r;
      for (; r < start_r + size; r++)
      {
        if (hardcoded_regs[translate_namespace(name_space)].count(remap_register(r, 1, name_space)))
          break;
      }
      if (r == start_r + size)
        return start_r;
    }
    G_ASSERTF(false, "Unreachable");
    return -1;
  };

  switch (name_space)
  {
    case NamedConstSpace::vsf:
    case NamedConstSpace::vsmp:
    case NamedConstSpace::vs_buf:
    case NamedConstSpace::vs_cbuf:
    case NamedConstSpace::vs_uav:
      for (NamedConst &nc : vertexProps.sc)
      {
        if (nc.nameSpace == name_space && !nc.isDynamic && !(base_cblock_reg && nc.nameSpace == NamedConstSpace::vsf))
        {
          if (needShift && base_reg == 1)
            base_reg++;

          base_reg = find_first_free_reg(vertexProps.hardcoded_regs, nc.size);

          if (nc.regIndex == -1)
          {
            nc.regIndex = remap_register(base_reg, nc.size, nc.nameSpace);
            base_reg += nc.size;
          }
        }
      }

      for (int i = 0; i < vertexProps.sc.size(); i++)
      {
        NamedConst &nc = vertexProps.sc[i];
        if (nc.nameSpace == name_space && nc.isDynamic)
        {
          if (needShift && base_reg == 1)
            base_reg++;

          base_reg = find_first_free_reg(vertexProps.hardcoded_regs, nc.size);

          if (nc.regIndex == -1)
          {
            nc.regIndex = remap_register(base_reg, nc.size, nc.nameSpace);
            base_reg += nc.size;
          }
        }
      }
      break;

    case NamedConstSpace::ps_buf:
    case NamedConstSpace::ps_cbuf:
    case NamedConstSpace::cs_buf:
    case NamedConstSpace::cs_cbuf:
    case NamedConstSpace::psf:
    case NamedConstSpace::smp:
    case NamedConstSpace::sampler:
    case NamedConstSpace::csf:
    case NamedConstSpace::uav:
    {
      auto handle_consts = [&, this](auto pred) {
        for (NamedConst &nc : pixelProps.sc)
        {
          if (nc.nameSpace == name_space && pred(nc))
          {
            if (needShift && base_reg == 1)
              base_reg++;

            base_reg = find_first_free_reg(pixelProps.hardcoded_regs, nc.size);

            if (nc.regIndex == -1)
            {
              nc.regIndex = remap_register(base_reg, nc.size, nc.nameSpace);
              base_reg += nc.size;
            }

            if (nc.nameSpace == NamedConstSpace::smp && nc.regIndex > 15)
            {
              String listOfSamplers;

              const auto dumpSamplers = [&listOfSamplers](RegisterProperties &props) {
                for (int i = 0; i < props.sc.size(); i++)
                {
                  if (props.sc[i].nameSpace != NamedConstSpace::smp && props.sc[i].nameSpace != NamedConstSpace::sampler)
                    continue;

                  listOfSamplers.aprintf(0, "%02d: %s\n", props.sc[i].regIndex + 1, props.sn.getName(i));
                }
              };

              for (auto sb : suppBlk)
              {
                listOfSamplers.aprintf(0, "Shader block [%s]:\n", sb->name);
                dumpSamplers(sb->shConst.pixelProps);
                listOfSamplers.append("\n");
              }
              listOfSamplers.aprintf(0, "Shader:\n");
              dumpSamplers(pixelProps);

              sh_debug(SHLOG_ERROR, "More than 16 textures with samplers in shader are not allowed! Dump of used samplers:\n%s",
                listOfSamplers.c_str());
            }
            if (nc.nameSpace == NamedConstSpace::uav)
            {
              if (nc.regIndex < 0)
                sh_debug(SHLOG_ERROR, "More than 8 uav registers are not allowed");
            }
          }
        }
      };

      handle_consts([&](auto &nc) { return !nc.isDynamic && !(base_cblock_reg && nc.nameSpace == NamedConstSpace::psf); });
      handle_consts([&](auto &nc) { return nc.isDynamic; });
      break;
    }

    default: DAG_FATAL("unsupported namespace: %d", (int)name_space);
  }
  return base_reg;
}

int NamedConstBlock::getRegForNamedConstEx(const char *name_buf, const char *blk_name, NamedConstSpace ns, bool pixel_shader)
{
  if (!blk_name[0])
  {
    int id = pixel_shader ? pixelProps.sn.getNameId(name_buf) : vertexProps.sn.getNameId(name_buf);
    if (id == -1)
      return -1;
    else if ((pixel_shader ? pixelProps.sc[id].nameSpace : vertexProps.sc[id].nameSpace) != ns)
    {
      sh_debug(SHLOG_ERROR, "%s: wrong usage namespace=%d, decl namespace=%d", name_buf,
        pixel_shader ? (int)pixelProps.sc[id].nameSpace : (int)vertexProps.sc[id].nameSpace, (int)ns);
      return -1;
    }
    return pixel_shader ? pixelProps.sc[id].regIndex : vertexProps.sc[id].regIndex;
  }

  for (int i = 0; i < suppBlk.size(); i++)
    if (suppBlk[i]->name == blk_name)
      return suppBlk[i]->shConst.getRegForNamedConst(name_buf, ns, pixel_shader);
    else
    {
      int reg = suppBlk[i]->shConst.getRegForNamedConstEx(name_buf, blk_name, ns, pixel_shader);
      if (reg >= 0)
        return reg;
    }
  return -1;
}

int NamedConstBlock::getRegForNamedConst(const char *name_buf, NamedConstSpace ns, bool pixel_shader)
{
  int id = pixel_shader ? pixelProps.sn.getNameId(name_buf) : vertexProps.sn.getNameId(name_buf);
  if (id == -1)
  {
    if (!suppBlk.size())
      return -1;

    int reg = suppBlk[0]->shConst.getRegForNamedConst(name_buf, ns, pixel_shader);
    if (reg < 0)
      return -1;

    for (int i = 1; i < suppBlk.size(); i++)
    {
      int nreg = suppBlk[i]->shConst.getRegForNamedConst(name_buf, ns, pixel_shader);
      if (nreg != reg)
        return -1;
    }
    return reg;
  }
  else if ((pixel_shader ? pixelProps.sc[id].nameSpace : vertexProps.sc[id].nameSpace) != ns)
  {
    sh_debug(SHLOG_ERROR, "%s: wrong usage namespace=%d, decl namespace=%d", name_buf,
      pixel_shader ? (int)pixelProps.sc[id].nameSpace : (int)vertexProps.sc[id].nameSpace, (int)ns);
    return -1;
  }
  return pixel_shader ? pixelProps.sc[id].regIndex : vertexProps.sc[id].regIndex;
}

CryptoHash NamedConstBlock::getDigest(bool ps_const, bool cs_const, const MergedVariablesData &merged_vars) const
{
  CryptoHasher hasher;
  hasher.update(suppBlk.size());

  for (int i = 0; i < suppBlk.size(); i++)
    hasher.update(suppBlk[i]);
  auto constUpdates = [&](auto &n, auto &c) {
    hasher.update(n.nameCount());
    for (int id = 0, e = n.nameCount(); id < e; id++)
    {
      hasher.update(n.getName(id));
      hasher.update(c[id].nameSpace);
      hasher.update(c[id].size);
      hasher.update(c[id].regIndex);
    }
  };
  if (ps_const || cs_const)
    constUpdates(pixelProps.sn, pixelProps.sc);
  else
    constUpdates(vertexProps.sn, vertexProps.sc);

  bool hasStaticCbuf = staticCbufType != StaticCbuf::NONE;
  {
    String res;
    SCFastNameMap added_names;
    buildDrawcallIdHlslDecl(res);
    if (hasStaticCbuf)
      buildStaticConstBufHlslDecl(res, merged_vars);
    buildHlslDeclText(res, ps_const, cs_const, added_names, hasStaticCbuf);
    hasher.update(res.data(), res.length());
  }

  return hasher.hash();
}

enum class HlslNameSpace
{
  t,
  s,
  c,
  u,
  b
};

#define HLSL_NS_SWITCH(switch_val, default_val) \
  switch (switch_val)                           \
  {                                             \
    CASE(t)                                     \
    CASE(s)                                     \
    CASE(c)                                     \
    CASE(u)                                     \
    CASE(b)                                     \
    default: G_ASSERT_RETURN(0, (default_val)); \
  }

static HlslNameSpace sym_to_hlsl_namespace(char sym)
{
#define CASE(symbol) \
  case (*(#symbol)): return HlslNameSpace::symbol;

  HLSL_NS_SWITCH(sym, HlslNameSpace::c)
#undef CASE
}

static char hlsl_namespace_to_sym(HlslNameSpace hlslNs)
{
#define CASE(symbol) \
  case HlslNameSpace::symbol: return (*(#symbol));

  HLSL_NS_SWITCH(hlslNs, '\0')
#undef CASE
}

static HlslNameSpace named_const_space_to_hlsl_namespace(NamedConstSpace name_space, bool is_sampler_in_pair = false)
{
  switch (name_space)
  {
    case NamedConstSpace::vsf:
    case NamedConstSpace::psf:
    case NamedConstSpace::csf: return HlslNameSpace::c;

    case NamedConstSpace::smp:
    case NamedConstSpace::vsmp: return is_sampler_in_pair ? HlslNameSpace::s : HlslNameSpace::t;

    case NamedConstSpace::sampler: return HlslNameSpace::s;

    case NamedConstSpace::uav:
    case NamedConstSpace::vs_uav: return HlslNameSpace::u;

    case NamedConstSpace::vs_buf:
    case NamedConstSpace::ps_buf:
    case NamedConstSpace::cs_buf: return HlslNameSpace::t;

    case NamedConstSpace::vs_cbuf:
    case NamedConstSpace::ps_cbuf:
    case NamedConstSpace::cs_cbuf: return HlslNameSpace::b;

    default: G_ASSERT_RETURN(0, HlslNameSpace::c);
  }
}

class RegistersUsage
{
  struct RegSet : eastl::bitvector<>
  {
    RegSet(int cap) { reserve(cap); }
  };

  static constexpr int BASE_RESOURCE_REG_CAP = 32;
  static constexpr int BASE_CONST_REG_CAP = 2048;

public:
  RegistersUsage() :
    t(BASE_RESOURCE_REG_CAP), s(BASE_RESOURCE_REG_CAP), c(BASE_CONST_REG_CAP), u(BASE_RESOURCE_REG_CAP), b(BASE_RESOURCE_REG_CAP)
  {}

  void addReg(int base_reg, int num_regs, NamedConstSpace name_space)
  {
    addRegToSet(base_reg, num_regs, setForNamespace(name_space));
    if (name_space == NamedConstSpace::smp || name_space == NamedConstSpace::vsmp)
      addRegToSet(base_reg, num_regs, setForNamespace(name_space, true));
  }
  void addReg(int base_reg, int num_regs, HlslNameSpace hlsl_namespace)
  {
    addRegToSet(base_reg, num_regs, setForHlslNamespace(hlsl_namespace));
  }

  int regCap(NamedConstSpace name_space) const { return setForNamespace(name_space).size(); }
  int regCap(HlslNameSpace hlsl_namepace) const { return setForHlslNamespace(hlsl_namepace).size(); }

  bool containsReg(int reg, NamedConstSpace name_space) const
  {
    return reg >= regCap(name_space) ? false : setForNamespace(name_space)[reg];
  }
  bool containsReg(int reg, HlslNameSpace hlsl_namepace) const
  {
    return reg >= regCap(hlsl_namepace) ? false : setForHlslNamespace(hlsl_namepace)[reg];
  }

private:
  RegSet t, s, c, u, b;

  RegSet &setForHlslNamespace(HlslNameSpace hlsl_namespace)
  {
#define CASE(symbol) \
  case (HlslNameSpace::symbol): return symbol;

    HLSL_NS_SWITCH(hlsl_namespace, c)
#undef CASE
  }
  const RegSet &setForHlslNamespace(HlslNameSpace hlsl_namespace) const
  {
#define CASE(symbol) \
  case (HlslNameSpace::symbol): return symbol;

    HLSL_NS_SWITCH(hlsl_namespace, c)
#undef CASE
  }

  RegSet &setForNamespace(NamedConstSpace name_space, bool is_sampler_in_pair = false)
  {
    return setForHlslNamespace(named_const_space_to_hlsl_namespace(name_space, is_sampler_in_pair));
  }
  const RegSet &setForNamespace(NamedConstSpace name_space, bool is_sampler_in_pair = false) const
  {
    return setForHlslNamespace(named_const_space_to_hlsl_namespace(name_space, is_sampler_in_pair));
  }

  inline static void addRegToSet(int base_reg, int num_regs, RegSet &set)
  {
    G_ASSERT(num_regs > 0); // sanity check
    int cap = base_reg + num_regs;
    if (cap > set.size())
      set.resize(cap, false);
    for (int r = base_reg; r < base_reg + num_regs; ++r)
      set[r] = true;
  }
};

static void process_used_regs(RegistersUsage &used_regs, dag::ConstSpan<NamedConstBlock::NamedConst> n, bool static_cbuf = false)
{
#define CASE_ADD_USED_REG(X)                                                            \
  case NamedConstSpace::X:                                                              \
  {                                                                                     \
    if (static_cbuf && n[i].nameSpace == NamedConstSpace::psf && n[i].regIndex == 0xFF) \
      break;                                                                            \
    used_regs.addReg(n[i].regIndex, n[i].size, n[i].nameSpace);                         \
    break;                                                                              \
  }
  for (int i = 0; i < n.size(); i++)
    switch (n[i].nameSpace)
    {
      CASE_ADD_USED_REG(vsf)
      CASE_ADD_USED_REG(psf)
      CASE_ADD_USED_REG(csf)
      CASE_ADD_USED_REG(smp)
      CASE_ADD_USED_REG(vsmp)
      CASE_ADD_USED_REG(sampler)
      CASE_ADD_USED_REG(uav)
      CASE_ADD_USED_REG(vs_uav)
      CASE_ADD_USED_REG(ps_buf)
      CASE_ADD_USED_REG(cs_buf)
      CASE_ADD_USED_REG(vs_buf)
      CASE_ADD_USED_REG(ps_cbuf)
      CASE_ADD_USED_REG(cs_cbuf)
      CASE_ADD_USED_REG(vs_cbuf)
    }
#undef CASE_ADD_USED_REG
}

struct SamplerState
{
  String name;
  int regIdx;
  SamplerState() : regIdx(-1) {}
  SamplerState(const char *tname, int rIdx) : name(tname), regIdx(rIdx) {}
};

static bool shall_remove_ns(NamedConstSpace ns, bool pixel_shader, bool compute_shader)
{
  const bool vertex_shader = !compute_shader && !pixel_shader;
  bool remove =
    (pixel_shader && (ns == NamedConstSpace::vsf || ns == NamedConstSpace::vsmp || ns == NamedConstSpace::vs_uav)) ||
    (!pixel_shader && (ns == NamedConstSpace::psf || ns == NamedConstSpace::smp)) ||
    (!compute_shader && (ns == NamedConstSpace::csf)) ||
    (!pixel_shader && (ns == NamedConstSpace::ps_buf || ns == NamedConstSpace::ps_cbuf)) ||
    (!compute_shader && (ns == NamedConstSpace::cs_buf || ns == NamedConstSpace::cs_cbuf)) ||
    (!vertex_shader && (ns == NamedConstSpace::vs_buf || ns == NamedConstSpace::vs_cbuf || ns == NamedConstSpace::vs_uav)) ||
    (vertex_shader && ns == NamedConstSpace::uav);
  return remove;
}
static void add_lines_commented(String &out_text, const char *add_text)
{
  while (const char *p = strchr(add_text, '\n'))
  {
    out_text += String(0, "%s%.*s", strnicmp(add_text, "#line ", 6) == 0 ? "" : "//", p - add_text + 1, add_text);
    add_text = p + 1;
  }

  if (*add_text)
    out_text += String(0, "%s%s\n", strnicmp(add_text, "#line ", 6) == 0 ? "" : "//", add_text);
}

static eastl::string_view get_hlsl_decl(const char *hlsl)
{
  const char *p = strchr(hlsl, '\n');
  if (!p)
    return eastl::string_view(hlsl);

  if (const char *p1 = strchr(p + 1, ':'))
    return eastl::string_view(hlsl, p1 - hlsl);
  if (const char *p1 = strchr(p + 1, '@'))
    return eastl::string_view(hlsl, p1 - hlsl);

  return eastl::string_view(hlsl);
}

const char *MATERIAL_PROPS_NAME = "materialProps";

template <enum NamedConstSpace TOKEN_TO_PROCESS>
static void gather_static_consts(const NamedConstBlock::RegisterProperties &reg_props,
  const NamedConstBlock::RegisterProperties *prev_props, const ShaderParser::VariablesMerger &merged_vars, String &out_hlsl,
  String &access_functions, int &our_regs_count)
{
  using MergedVarInfo = ShaderParser::VariablesMerger::MergedVarInfo;
  using MergedVars = ShaderParser::VariablesMerger::MergedVars;

  for (int i = 0; i < reg_props.sc.size(); i++)
  {
    const NamedConstBlock::NamedConst &statConst = reg_props.sc[i];
    if (statConst.nameSpace != TOKEN_TO_PROCESS || statConst.isDynamic)
      continue;
    const char *constName = reg_props.sn.getName(i);
    if (prev_props && prev_props->sn.getNameId(constName) >= 0) // skip duplicate VS/PS vars
      continue;

    const char *hlsl = statConst.hlsl.c_str();
    eastl::string_view hlsl_decl = get_hlsl_decl(hlsl);
    G_ASSERTF(statConst.regIndex == our_regs_count, "ICE: nc.regIndex=%d regIndex=%d", statConst.regIndex, our_regs_count);
    our_regs_count += statConst.size;
    eastl::array<const char *, 13> TYPE_NAMES = {
      "float4x4 ",
      "float4 ",
      "float3 ",
      "float2 ",
      "float ",
      "uint4 ",
      "uint3 ",
      "uint2 ",
      "uint ",
      "int4 ",
      "int3 ",
      "int2 ",
      "int ",
    };

    eastl::string_view type_name;
    for (const char *type : TYPE_NAMES)
    {
      size_t found_pos = hlsl_decl.find(type);
      if (found_pos != eastl::string_view::npos)
      {
        type_name = eastl::string_view(type, strlen(type) - 1); // without the space
        break;
      }
    }

    // If this constant is a key in the merger's map, it means that it is not a normal constant, but a few constants
    // packed together into one register. In this case, we have to define getters for each individual packed const
    if (const MergedVars *originalVars = merged_vars.findOriginalVarsInfo(constName, false))
    {
      for (const MergedVarInfo &varInfo : *originalVars)
      {
        out_hlsl.aprintf(0, " %s %s;", varInfo.getType(), varInfo.name);
        access_functions.aprintf(0, "%s get_%s() { return %s[DRAW_CALL_ID].%s; }\n", varInfo.getType(), varInfo.name,
          MATERIAL_PROPS_NAME, varInfo.name);
      }
    }
    // Otherwise, this is a regular constant which is used as-is, and we just define the getter for the const itself.
    else
    {
      out_hlsl.aprintf(0, "  %.*s;", hlsl_decl.length(), hlsl_decl);
      access_functions.aprintf(0, "%.*s get_%s() { return %s[DRAW_CALL_ID].%s; }\n", type_name.length(), type_name, constName,
        MATERIAL_PROPS_NAME, constName);
    }

    if (type_name.find('4') == eastl::string_view::npos) // not float4x4/float4/int4/uint4 partial vector, needs padding
    {
      if (type_name == "float")
        out_hlsl.aprintf(0, " float __pad1_%s, __pad2_%s, __pad3_%s;", constName, constName, constName);
      else if (type_name == "int")
        out_hlsl.aprintf(0, " int __pad1_%s, __pad2_%s, __pad3_%s;", constName, constName, constName);
      else if (type_name == "uint")
        out_hlsl.aprintf(0, " uint __pad1_%s, __pad2_%s, __pad3_%s;", constName, constName, constName);
      else if (type_name == "float2")
        out_hlsl.aprintf(0, " float2 __pad_%s;", constName);
      else if (type_name == "int2")
        out_hlsl.aprintf(0, " int2 __pad_%s;", constName);
      else if (type_name == "uint2")
        out_hlsl.aprintf(0, " uint2 __pad_%s;", constName);
      else if (type_name == "float3")
        out_hlsl.aprintf(0, " float __pad_%s;", constName);
      else if (type_name == "int3")
        out_hlsl.aprintf(0, " int __pad_%s;", constName);
      else if (type_name == "uint3")
        out_hlsl.aprintf(0, " uint __pad_%s;", constName);
      else
        G_ASSERTF(0, "ICE: unexpected decl: %s", hlsl);
    }
    out_hlsl += '\n';
  }
}

void NamedConstBlock::buildDrawcallIdHlslDecl(String &out_text) const
{
  const bool shaderSupportsMultidraw = staticCbufType != StaticCbuf::SINGLE;
  const char *drawCallIdDeclaration =
    shaderSupportsMultidraw ? "static uint DRAW_CALL_ID = 10000;\n" : "static const uint DRAW_CALL_ID = 0;\n";
  out_text = drawCallIdDeclaration;
}

void NamedConstBlock::buildStaticConstBufHlslDecl(String &out_text, const MergedVariablesData &merged_vars) const
{
  const bool shaderSupportsMultidraw = staticCbufType == StaticCbuf::ARRAY;
  const char *bindlessProlog = "";
  const char *bindlessType = "";

  if (shc::config().enableBindless)
  {
#if _CROSS_TARGET_C1 || _CROSS_TARGET_C2

#elif _CROSS_TARGET_SPIRV
    bindlessProlog = "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] Texture2D static_textures[];\n"
                     "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures_cube[];\n"
                     "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] Texture2DArray static_textures_array[];\n"
                     "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures_cube_array[];\n"
                     "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures3d[];\n"
                     "[[vk::binding(0, BINDLESS_SAMPLER_SET_META_ID)]] SamplerState static_samplers[];\n";
#elif _CROSS_TARGET_METAL
    bindlessProlog = "Texture2D static_textures[];\n"
                     "TextureCube static_textures_cube[];\n"
                     "Texture2DArray static_textures_array[];\n"
                     "TextureCube static_textures_cube_array[];\n"
                     "TextureCube static_textures3d[];\n"
                     "SamplerState static_samplers[];\n";
#else
    bindlessProlog = "Texture2D static_textures[] : BINDLESS_REGISTER(t, 1);\n"
                     "SamplerState static_samplers[] : BINDLESS_REGISTER(s, 1);\n"
                     "TextureCube static_textures_cube[] : BINDLESS_REGISTER(t, 2);\n"
                     "Texture2DArray static_textures_array[] : BINDLESS_REGISTER(t, 3);\n"
                     "TextureCube static_textures_cube_array[] : BINDLESS_REGISTER(t, 4);\n"
                     "TextureCube static_textures3d[] : BINDLESS_REGISTER(t, 5);\n";
#endif
    bindlessType = shaderSupportsMultidraw ? "#define BINDLESS_CBUFFER_ARRAY\n" : "#define BINDLESS_CBUFFER_SINGLE\n";
  }

  int regIndex = 0;
  String constantAccessFunctions;
  String materialPropsStruct;
  gather_static_consts<NamedConstSpace::vsf>(vertexProps, nullptr, merged_vars, materialPropsStruct, constantAccessFunctions,
    regIndex);
  gather_static_consts<NamedConstSpace::psf>(pixelProps, &vertexProps, merged_vars, materialPropsStruct, constantAccessFunctions,
    regIndex);

  const uint32_t MAX_CBUFFER_VECTORS = 4096;
  const uint32_t propSetsCount = shaderSupportsMultidraw ? MAX_CBUFFER_VECTORS / regIndex : 1;

  out_text.aprintf(0,
    "%s%s"
    "struct MaterialProperties\n{\n%s\n};\n\n"
    "#define MATERIAL_PROPS_SIZE %d\n"
    "cbuffer shader_static_cbuf:register(b1) { MaterialProperties %s[MATERIAL_PROPS_SIZE]; };\n\n%s\n\n",
    bindlessType, bindlessProlog, materialPropsStruct, propSetsCount, MATERIAL_PROPS_NAME, constantAccessFunctions);
}

void NamedConstBlock::buildGlobalConstBufHlslDecl(String &out_text, bool pixel_shader, bool compute_shader,
  SCFastNameMap &added_names) const
{
  String res;
  String suppCode;
  bool hasAnyConst = false;
  int psfCount = 0;

  auto processXsc = [&](const SCFastNameMap &xsn, const Tab<NamedConst> &xsc, NamedConstSpace nameSpace) {
    int insertedCount = 0;
    for (int i = 0; i < xsc.size(); i++)
    {
      const NamedConst &nc = xsc[i];
      const char *hlsl = nc.hlsl.c_str();
      if (*hlsl && !shall_remove_ns(nc.nameSpace, pixel_shader, compute_shader))
      {
        const char *nm = xsn.getName(i);
        if (nc.nameSpace == NamedConstSpace::vsf || nc.nameSpace == NamedConstSpace::psf || nc.nameSpace == NamedConstSpace::csf)
        {
          if (nc.nameSpace == nameSpace)
          {
            if (added_names.getNameId(nm) == -1)
            {
              const char *atPos = nullptr;
              if ((atPos = strchr(hlsl, '@')))
              {
                const int csfOffset = nc.nameSpace == NamedConstSpace::csf ? psfCount : 0;
                res.aprintf(32, "\n%.*s : packoffset(c%d);", atPos - hlsl, hlsl, nc.regIndex + csfOffset);
                insertedCount++;
                hasAnyConst = true;

                const char *supportCodeBegin = strchr(atPos, ';');
                if (supportCodeBegin)
                  suppCode.append(supportCodeBegin + 1);
              }
              added_names.addNameId(nm);
            }
            else
              add_lines_commented(res, hlsl);
          }
        }
        else
          G_ASSERTF(0, "Global const block could not contain other than consts (f1-4, i1-4)!");
      }
    }
    return insertedCount;
  };
  processXsc(vertexProps.sn, vertexProps.sc, NamedConstSpace::vsf);
  psfCount = processXsc(pixelProps.sn, pixelProps.sc, NamedConstSpace::psf);
  processXsc(pixelProps.sn, pixelProps.sc, NamedConstSpace::csf);

  if (hasAnyConst)
  {
    out_text += "cbuffer global_const_block : register(b2) {";
    out_text += res;
    out_text += "\n};\n\n";
    out_text += suppCode;
    out_text += "\n\n";
  }
}

void NamedConstBlock::buildHlslDeclText(String &out_text, bool pixel_shader, bool compute_shader, SCFastNameMap &added_names,
  bool has_static_cbuf) const
{
  if (globConstBlk)
    globConstBlk->shConst.buildGlobalConstBufHlslDecl(out_text, pixel_shader, compute_shader, added_names);
  for (const ShaderStateBlock *sb : suppBlk)
    sb->shConst.buildHlslDeclText(out_text, pixel_shader, compute_shader, added_names, false);

  for (int i = 0; i < vertexProps.sc.size(); i++)
  {
    const NamedConst &nc = vertexProps.sc[i];
    const char *hlsl = nc.hlsl.c_str();
    if (has_static_cbuf && !nc.isDynamic && nc.nameSpace == NamedConstSpace::vsf)
      continue;
    if (*hlsl && !shall_remove_ns(nc.nameSpace, pixel_shader, compute_shader))
    {
      const char *nm = vertexProps.sn.getName(i);
      if (added_names.getNameId(nm) == -1)
      {
        out_text += hlsl;
        added_names.addNameId(nm);
      }
      else
        add_lines_commented(out_text, hlsl);
    }
  }
  for (int i = 0; i < pixelProps.sc.size(); i++)
  {
    const NamedConst &nc = pixelProps.sc[i];
    const char *hlsl = nc.hlsl.c_str();
    if (has_static_cbuf && !nc.isDynamic && nc.nameSpace == NamedConstSpace::psf)
      continue;
    if (*hlsl && !shall_remove_ns(nc.nameSpace, pixel_shader, compute_shader))
    {
      const char *nm = pixelProps.sn.getName(i);
      if (added_names.getNameId(nm) == -1)
      {
        out_text += hlsl;
        added_names.addNameId(nm);
      }
      else
        add_lines_commented(out_text, hlsl);
    }
  }

  const Tab<eastl::string> &hlsl = pixel_shader ? pixelProps.postfixHlsl : vertexProps.postfixHlsl;
  for (const eastl::string &snippet : hlsl)
    out_text += snippet.c_str();
}

template <typename TF> // TF: void(char regt_sym, int regt_id, eastl::string_view code_fragment)
static void process_hardcoded_register_declarations(const char *hlsl_src, TF &&processor)
{
  const char *text = hlsl_src, *p, *start = text;

  while ((p = strstr(text, "register")) != NULL)
  {
    if (p <= start || fast_isalnum_or_(p[-1]) || fast_isalnum_or_(p[8]))
    {
      text = p + 8;
      continue;
    }
    const char *fragment_start = p;
    while (fragment_start > start && !strchr("\r\n", fragment_start[-1]))
      fragment_start--;

    p += 8;
    while (strchr(" \t\v\n\r", *p))
      p++;
    if (*p != '(')
    {
      text = p;
      continue;
    }
    p++;
    while (strchr(" \t\v\n\r", *p))
      p++;
    if (!strchr("tcsub", *p))
    {
      text = p;
      continue;
    }
    int regt_sym = *p;
    int idx = atoi(p + 1);

    p++;
    while (isdigit(*p))
      p++;
    const char *fragment_end = p;
    while (*fragment_end && !strchr("\r\n", *fragment_end))
      fragment_end++;

    processor(regt_sym, idx, eastl::string_view(fragment_start, fragment_end - fragment_start));

    text = p;
  }
}

#include "transcodeCommon.h"

static void build_predefines_str(String &predefines, String &src, const char *hw_defines)
{
  // add target-specific predefines
  const char *predefines_str =
#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_DX12
    dx12::dxil::Platform::XBOX_ONE == shc::config().targetPlatform
      ?
#include "predefines_dx12x.hlsl.inl"
      : dx12::dxil::Platform::XBOX_SCARLETT == shc::config().targetPlatform ?
#include "predefines_dx12xs.hlsl.inl"
                                                                            :
#include "predefines_dx12.hlsl.inl"
#elif _CROSS_TARGET_DX11
#include "predefines_dx11.hlsl.inl"
#elif _CROSS_TARGET_SPIRV
#include "predefines_spirv.hlsl.inl"
#elif _CROSS_TARGET_METAL
#include "predefines_metal.hlsl.inl"
#else
    nullptr
#endif
    ;

  const char *predefines_add_str = nullptr;
#if _CROSS_TARGET_C2









#endif


  predefines = hw_defines ? hw_defines : "";
  if (shc::config().hlslDefines.length())
    predefines += shc::config().hlslDefines;
  if (predefines_str)
    predefines += predefines_str;
  if (predefines_add_str)
    predefines += predefines_add_str;
}

// TODO: rename pixel_shader to pixel_or_compute_shader everywhere applicable in this file. Or just pass shader stage instead.
void NamedConstBlock::patchHlsl(String &src, bool pixel_shader, bool compute_shader, const MergedVariablesData &merged_vars,
  int &max_const_no_used, const char *hw_defines)
{
  max_const_no_used = -1;
  static String res, name_buf, blk_name_buf;
  int name_prefix_len = 0;
  res.clear();
  const bool doesShaderGlobalExist = ShaderStateBlock::countBlock(ShaderStateBlock::LEV_GLOBAL_CONST) > 0;

  static String predefines;
  build_predefines_str(predefines, src, hw_defines);

  bool hasStaticCbuf = staticCbufType != StaticCbuf::NONE;
  {
    SCFastNameMap added_names;
    buildDrawcallIdHlslDecl(res);
    if (hasStaticCbuf)
      buildStaticConstBufHlslDecl(res, merged_vars);
    buildHlslDeclText(res, pixel_shader, compute_shader, added_names, hasStaticCbuf);
    src.insert(0, res);
    res.clear();
  }

  const char *text = src, *p, *start = text;

  const RegisterProperties NamedConstBlock::*regPropsMember =
    pixel_shader ? &NamedConstBlock::pixelProps : &NamedConstBlock::vertexProps;
  const RegisterProperties &regProps = this->*regPropsMember;

  const NamedConstSpace constNamespace =
    compute_shader ? NamedConstSpace::csf : (pixel_shader ? NamedConstSpace::psf : NamedConstSpace::vsf);
  const NamedConstSpace cbufNamespace =
    compute_shader ? NamedConstSpace::cs_cbuf : (pixel_shader ? NamedConstSpace::ps_cbuf : NamedConstSpace::vs_cbuf);

  // scan for hardcoded constants, : register(@#)
  RegistersUsage allocatedRegs, allHardcodedRegs;

  // First, collect hardcoded registers from predefines into a separate collection. This is made for correct validation:
  //  In user-defined or auto-allocated registers we can't allocate past hard limits (MAX_T_REGISTERS, for example)
  //  However, predefines are allowed to bypass this restricition for internal purposes (bindless tex/smp for ps4/5),
  //  therefore we have to exclude them from this validation and trust the developers not to mess up with predefines.
  RegistersUsage internalHardcodedRegs, userHardcodedRegs;

  process_hardcoded_register_declarations(predefines,
    [&allHardcodedRegs, &internalHardcodedRegs](char regt_sym, int regt_id, eastl::string_view) {
      // @NOTE: this doesn't validate constants that take up >1 regs correctly.
      //        However, this would require us to parse hlsl more seriously.
      auto ns = sym_to_hlsl_namespace(regt_sym);
      allHardcodedRegs.addReg(regt_id, 1, ns);
      internalHardcodedRegs.addReg(regt_id, 1, sym_to_hlsl_namespace(regt_sym));
    });

  // Next, collect all data about automatically allocated registers
  {
    process_used_regs(allocatedRegs, regProps.sc, hasStaticCbuf);
    for (int i = 0; i < suppBlk.size(); i++)
      process_used_regs(allocatedRegs, (suppBlk[i]->shConst.*regPropsMember).sc);
  }

  bool hasCbuf0Constants = eastl::any_of(regProps.sc.begin(), regProps.sc.end(),
    [constNamespace](const NamedConst &nc) { return nc.isDynamic && nc.nameSpace == constNamespace; });
  bool hardcodedCbuf0 = false;
  eastl::string_view hardcodedCbuf0Decl;

  // Finally, process all of the code for register collisions, and collect all data about hardcoded registers, including ones from
  // predefines, because we still need to validate that they don't collide with anything.
  process_hardcoded_register_declarations(src, [&](char regt_sym, int regt_id, eastl::string_view code_fragment) {
    HlslNameSpace hlslNs = sym_to_hlsl_namespace(regt_sym);

    const bool collidesWithNamed = allocatedRegs.containsReg(regt_id, hlslNs);
    const bool collidesWithHardcoded = allHardcodedRegs.containsReg(regt_id, hlslNs);

    // @NOTE: this doesn't validate constants that take up >1 regs correctly.
    //        However, this would require us to parse hlsl more seriously.
    allHardcodedRegs.addReg(regt_id, 1, hlslNs);
    if (!internalHardcodedRegs.containsReg(regt_id, hlslNs))
      userHardcodedRegs.addReg(regt_id, 1, hlslNs);

    if (hlslNs == HlslNameSpace::b && regt_id == 0)
    {
      hardcodedCbuf0 = true;
      hardcodedCbuf0Decl = code_fragment;
    }

    if (collidesWithNamed)
    {
      sh_debug(SHLOG_ERROR, "hardcoded register constant collides with named ones");
      sh_debug(SHLOG_ERROR, "found %c# at idx %d collides with named at HLSL code:\n\"%.*s\"\n", regt_sym, regt_id,
        code_fragment.size(), code_fragment);

      debug("named consts map for hlsl nampespace %c#", regt_sym);

      auto debugOutputNamedConsts = [regPropsMember, hlslNs](const NamedConstBlock &consts) {
        const RegisterProperties &regProps = consts.*regPropsMember;

        for (int i = 0; i < regProps.sc.size(); i++)
        {
          const NamedConst &nc = regProps.sc[i];
          if (named_const_space_to_hlsl_namespace(nc.nameSpace) == hlslNs ||
              named_const_space_to_hlsl_namespace(nc.nameSpace, true) == hlslNs)
          {
            debug("  %s%s: %d (sz=%d)", regProps.sn.getName(i), nameSpaceToStr(nc.nameSpace), nc.regIndex, nc.size);
          }
        }
      };

      for (const auto &blk : suppBlk)
        debugOutputNamedConsts(blk->shConst);
      debugOutputNamedConsts(*this);
      debug("");
    }
    else if (collidesWithHardcoded)
    {
      if (hasStaticCbuf && hlslNs == HlslNameSpace::b && regt_id == 1)
        sh_debug(SHLOG_ERROR, "hardcoded register cbuf collides with static cbuf");
      else if (doesShaderGlobalExist && hlslNs == HlslNameSpace::b && regt_id == 2)
        sh_debug(SHLOG_ERROR, "hardcoded register cbuf collides with global block cbuf");
      else
      {
        // @HACK Tricks with redefining hardcoded regs can be used in shader code,
        // so only static cbuf and global block will be validated.
        //
        // For example, one can write (this is simplified dshl code)
        // hlsl (cs) {
        //   Texture2D tex1 : register(t0);
        //   void compute_shader1() {
        //     *do smth with tex1*
        //   }
        //
        //   Texture2D tex2 : register(t0);
        //   void compute_shader2() {
        //     *do smth with tex2*
        //   }
        // }
        //
        // And then based on some interval values or conditions do
        // compile("target_cs", "compute_shader1")
        // or
        // compile("target_cs", "compute_shader2")
        // And it would work fine, because tex1 and tex2 act as aliases for the same bound resource.

        // sh_debug(SHLOG_ERROR, "hardcoded register constant collides with another");
        text = p;
        return;
      }

      sh_debug(SHLOG_ERROR, "found %c# at idx %d collides with previously declared at HLSL code:\n\"%.*s\"\n", regt_sym, regt_id,
        code_fragment.size(), code_fragment);
    }
  });

  if (hasCbuf0Constants && hardcodedCbuf0)
  {
    sh_debug(SHLOG_ERROR,
      "Shader constants are used, but hardcoded register b0 collides with implicit constbuffer at HLSL code:\n\"%.*s\"\n",
      hardcodedCbuf0Decl.length(), hardcodedCbuf0Decl.data());
  }

  auto regRangeCapAndLast = [&allocatedRegs](HlslNameSpace hlsl_ns, const RegistersUsage &hardcoded_regs) {
    int cap = max<int>(allocatedRegs.regCap(hlsl_ns), hardcoded_regs.regCap(hlsl_ns));
    return eastl::make_pair(cap, max<int>(cap - 1, 0));
  };

  const int maxAllowedConsts = pixel_shader ? shc::config().hlslMaximumPsfAllowed : shc::config().hlslMaximumVsfAllowed;

  // Since we use this for cbuf size in driver, we have to check against all registers
  const auto [cRegRangeCap, cLastReg] = regRangeCapAndLast(HlslNameSpace::c, allHardcodedRegs);
  // For t registers we just validate the user registers
  const auto [tRegRangeCap, tLastReg] = regRangeCapAndLast(HlslNameSpace::t, userHardcodedRegs);

  if (cRegRangeCap > maxAllowedConsts)
    sh_debug(SHLOG_ERROR, "maximum register No used (%d) is bigger then allowed (%d)", cLastReg, maxAllowedConsts - 1);
  if (tRegRangeCap > MAX_T_REGISTERS)
    sh_debug(SHLOG_ERROR, "maximum T register No used (%d) is bigger than allowed (%d)", tLastReg, MAX_T_REGISTERS - 1);

  // @TODO: validate other namespaces: s u b

  max_const_no_used = cLastReg;

  Tab<SamplerState> samplersX[2];
  text = src;
  while ((p = strchr(text, '@')) != NULL)
  {
    NamedConstSpace ns = NamedConstSpace::unknown;
    int mnemo_len = 0;
    bool is_shadow = false;

#define HANDLE_NS_BASE(str, space)                 \
  if (strncmp(p + 1, #str, sizeof(#str) - 1) == 0) \
  ns = NamedConstSpace::space, mnemo_len = sizeof(#str)
#define HANDLE_NS(str) HANDLE_NS_BASE(str, str)

    HANDLE_NS(vsf);
    else HANDLE_NS(vsmp);
    else HANDLE_NS(psf);
    else HANDLE_NS(smp);
    else HANDLE_NS(csf);
    else HANDLE_NS(ps_buf);
    else HANDLE_NS(vs_buf);
    else HANDLE_NS(cs_buf);
    else HANDLE_NS_BASE(ps_tex, ps_buf);
    else HANDLE_NS_BASE(vs_tex, vs_buf);
    else HANDLE_NS_BASE(cs_tex, cs_buf);
    else HANDLE_NS_BASE(ps_tlas, ps_buf);
    else HANDLE_NS_BASE(vs_tlas, vs_buf);
    else HANDLE_NS_BASE(cs_tlas, cs_buf);
    else HANDLE_NS(ps_cbuf);
    else HANDLE_NS(vs_cbuf);
    else HANDLE_NS(cs_cbuf);
    else HANDLE_NS(uav);
    else HANDLE_NS(vs_uav);
    else HANDLE_NS(sampler);
    else if (strncmp(p + 1, "shd", 3) == 0)
    {
      ns = pixel_shader ? NamedConstSpace::smp : NamedConstSpace::vsmp;
      is_shadow = true;
      mnemo_len = 4;
    }
    if (mnemo_len <= 0)
    {
      sh_debug(SHLOG_ERROR, "Symbol @ is forbidden in hlsl and hlsli files.");
      return;
    }
    if (ns != NamedConstSpace::unknown && fast_isalnum_or_(p[mnemo_len]))
      ns = NamedConstSpace::unknown;

    if (ns == NamedConstSpace::unknown)
    {
      int clen = 0;
      while (fast_isalnum(p[clen + 1]))
        clen++;
      sh_debug(SHLOG_ERROR, "invalid namespace suffix @%.*s, see shaderlog", clen, p + 1);
      while (p[clen + 1] != '\n')
        clen++;
      debug("invalid namespace suffix during patching HLSL code:\n%.*s <--- error here", p + 1 + clen - text, text);
      src.clear();
      return;
    }
    while (p > start && fast_isspace(p[-1]))
    {
      p--;
      mnemo_len++;
    }
    const char *name = p;
    int regIndex = 0;
    while (name > start && (fast_isalnum_or_(name[-1]) || name[-1] == ':'))
      name--;
    int replaceType = 0;
    if (name > text && (name[-1] == '#' || name[-1] == '$'))
      replaceType = name[-1];

    bool remove = shall_remove_ns(ns, pixel_shader, compute_shader);
    if (!remove)
    {
      const char *pp = (char *)memchr(name, ':', p - name);
      if (pp)
      {
        blk_name_buf.printf(32, "%.*s", pp - name, name);
        while (*pp == ':')
          pp++;
        name_buf.printf(32, "%.*s", p - pp, pp);
        name_prefix_len = p - name;
      }
      else
      {
        name_buf.printf(32, "%.*s", p - name, name);
        name_prefix_len = 0;
      }

      if (!name_prefix_len)
        regIndex = getRegForNamedConst(name_buf, ns, pixel_shader);
      else
        regIndex = getRegForNamedConstEx(name_buf, blk_name_buf.str(), ns, pixel_shader);

      if (regIndex < 0)
        remove = true;
    }
    if (!remove)
    {
      HlslNameSpace hlslNs = named_const_space_to_hlsl_namespace(ns, is_shadow);
      char nsSym = hlsl_namespace_to_sym(hlslNs);

      if (hlslNs == HlslNameSpace::b)
      {
        if (regIndex == 0)
          sh_debug(SHLOG_FATAL, "Internal error! cbuf register b0 is allocated for %s, but it is reserved for implicit cbuf!",
            name_buf);
        if (hasStaticCbuf && regIndex == 1)
          sh_debug(SHLOG_FATAL, "Internal error! cbuf register b1 is allocated for %s, but static cbuf exists!", name_buf);
        if (doesShaderGlobalExist && regIndex == 2)
          sh_debug(SHLOG_FATAL, "Internal error! cbuf register b2 is allocated for %s, but global const block exists!", name_buf);
      }

      if (!remove && !is_shadow && (ns == NamedConstSpace::smp || ns == NamedConstSpace::vsmp))
      {
        Tab<SamplerState> &samplers = samplersX[(ns == NamedConstSpace::smp) ? 0 : 1];
        int i;
        for (i = 0; i < samplers.size(); ++i)
          if (!strcmp(samplers[i].name.str(), name_buf))
            break;
        if (i >= samplers.size())
          samplers.push_back(SamplerState(name_buf, regIndex));
      }

      if (replaceType == '#')
        res.aprintf(32, "%.*s%d", name - 1 - text, text, regIndex);
      else if (replaceType == '$')
        res.aprintf(32, "%.*s%c%d", name - 1 - text, text, nsSym, regIndex);
      else if (!name_prefix_len)
        res.aprintf(32, "%.*s: register(%c%d)", p - text, text, nsSym, regIndex);
      else
        res.aprintf(32, "%.*s%s: register(%c%d)", p - text - name_prefix_len, text, name_buf.str(), nsSym, regIndex);
      text = p + mnemo_len;
    }
    else if (!replaceType)
    {
      const char *lns = name, *lne = p + mnemo_len;

      // search for colon to the left, break on non-whitespace
      while (lns > text && strchr(" \n\r\t,", lns[-1]))
      {
        lns--;
        if (lns[0] == ',')
          break;
      }
      // search for colon to the right, break on non-whitespace
      while (lne[0] && strchr(" \n\r\t,", lne[0]))
      {
        lne++;
        if (lne[-1] == ',')
          break;
      }

      // Process cbuffer declaration
      if ((ns == NamedConstSpace::ps_cbuf || ns == NamedConstSpace::vs_cbuf || ns == NamedConstSpace::cs_cbuf) && lne[0] == '{')
      {
        int bracketsBalance = 1;
        while (bracketsBalance != 0)
        {
          lne++;
          if (lne[0] == '{')
            bracketsBalance++;
          else if (lne[0] == '}')
            bracketsBalance--;
        }
        lne++;
      }

      if (lns[0] == ',' && lne[-1] == ',')
        lne--; // if colons found on both sides, comment out left colon
      else if (lns[0] != ',' && lne[-1] != ',')
      {
        // if nn colons found, comment out whole declaration (expanding to newline on both sides)
        while (lns > text && (lns[-1] != '\r' && lns[-1] != '\n'))
          lns--;
        while (lne[0] && (lne[0] != '\r' && lne[0] != '\n'))
          lne++;
      }

      // compact /* */ braces so that whitespace is left outside comment
      while (lns < name && strchr(" \n\r\t", lns[0]))
        lns++;
      while (lne > p + mnemo_len && strchr(" \n\r\t", lne[-1]))
        lne--;

      // finally, comment out code
      if (strncmp(lns, "//", 2) == 0)
        res.aprintf(32, "%.*s", lne - text, text);
      else
        res.aprintf(32, "%.*s /*%.*s*/", lns - text, text, lne - lns, lns);
      text = lne;
    }
    else
    {
      if (replaceType == '$')
      {
        const char *lns = name;
        while (lns > text && lns[-1] != '\n' && lns[-1] != '\r')
          lns--;
        res.aprintf(32, "%.*s//%.*s", lns - text, text, name - lns, lns);
      }
      else if (replaceType == '#')
        res.aprintf(32, "%.*s-1", name - 1 - text, text);
      else
        res.aprintf(32, "%.*s", name - 1 - text, text);
      text = p + mnemo_len;
    }
  }
  res += text;
  String ps4_samplerstates;
  Tab<SamplerState> &samplers = samplersX[pixel_shader ? 0 : 1];
  for (int i = 0; i < samplers.size(); ++i)
  {
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2



#endif
      ps4_samplerstates.aprintf(128, "SamplerState %s_samplerstate: register(s%d);\n", samplers[i].name, samplers[i].regIdx);
  }
  src.setStrCat4(predefines, "\n", ps4_samplerstates, res);
}

void NamedConstBlock::patchStcodeIndices(dag::Span<int> stcode, StcodeRoutine &cpp_stcode, bool static_blk)
{
  int id, base, dest;
  bool static_cbuf = static_blk && staticCbufType != StaticCbuf::NONE;
  for (int i = 0; i < stcode.size(); i++)
  {
    int op = shaderopcode::getOp(stcode[i]);

    switch (op)
    {
      case SHCOD_FSH_CONST:
      case SHCOD_VPR_CONST:
      case SHCOD_CS_CONST:
      case SHCOD_TEXTURE:
      case SHCOD_TEXTURE_VS:
      case SHCOD_RWTEX:
      case SHCOD_RWBUF:
      case SHCOD_REG_BINDLESS:
      {
        id = shaderopcode::getOp3p1(stcode[i]);
        base = shaderopcode::getOp3p2(stcode[i]);
        G_ASSERT(id < ((op == SHCOD_VPR_CONST || op == SHCOD_TEXTURE_VS) ? vertexProps.sc.size() : pixelProps.sc.size()));

        bool isVs = (op == SHCOD_VPR_CONST || op == SHCOD_TEXTURE_VS);
        dest = isVs ? vertexProps.sc[id].regIndex + base : pixelProps.sc[id].regIndex + base;

        int destWithoutVprOffset = dest;

        if (static_cbuf && op == SHCOD_FSH_CONST && dest == 0xFF)
        {
          stcode[i] = shaderopcode::makeOp2_8_16(SHCOD_IMM_REAL1, shaderopcode::getOp3p3(stcode[i]), 0); // effectively NOP
          continue;
        }
        if (static_cbuf && (op == SHCOD_VPR_CONST || op == SHCOD_FSH_CONST))
        {
          op = SHCOD_VPR_CONST;
          dest += 0x800;
        }
        if (static_cbuf && op == SHCOD_REG_BINDLESS)
          dest += 0x800;

        // @NOTE: for matrices and arrays we want to patch only the base call
        if (base == 0 || !(op == SHCOD_VPR_CONST || op == SHCOD_FSH_CONST || op == SHCOD_CS_CONST))
          cpp_stcode.patchConstLocation(isVs ? STAGE_VS : STAGE_PS, id, destWithoutVprOffset);
        stcode[i] = shaderopcode::makeOp2(op, dest, shaderopcode::getOp3p3(stcode[i]));
      }
      break;
      case SHCOD_GLOB_SAMPLER:
      case SHCOD_SAMPLER:
      {
        id = shaderopcode::getOpStageSlot_Slot(stcode[i]);
        uint32_t stage = shaderopcode::getOpStageSlot_Stage(stcode[i]);
        if (stage == STAGE_VS)
        {
          G_ASSERT(id < vertexProps.sc.size());
          dest = vertexProps.sc[id].regIndex;
        }
        else
        {
          G_ASSERT(id < pixelProps.sc.size());
          dest = pixelProps.sc[id].regIndex;
        }
        cpp_stcode.patchConstLocation((ShaderStage)stage, id, dest);
        stcode[i] = shaderopcode::makeOpStageSlot(op, stage, dest, shaderopcode::getOpStageSlot_Reg(stcode[i]));
        break;
      }
      case SHCOD_BUFFER:
      case SHCOD_CONST_BUFFER:
      case SHCOD_TLAS:
      {
        const uint32_t stage = shaderopcode::getOpStageSlot_Stage(stcode[i]);
        uint32_t slot = shaderopcode::getOpStageSlot_Slot(stcode[i]);
        const uint32_t reg = shaderopcode::getOpStageSlot_Reg(stcode[i]);
        uint32_t patchedSlot = (stage == STAGE_VS) ? vertexProps.sc[slot].regIndex : pixelProps.sc[slot].regIndex;
        cpp_stcode.patchConstLocation((ShaderStage)stage, slot, patchedSlot);
        stcode[i] = shaderopcode::makeOpStageSlot(op, stage, patchedSlot, reg);
      }
      break;

      case SHCOD_G_TM:
      {
        id = shaderopcode::getOp2p1(stcode[i]) & 0x3FF;
        base = shaderopcode::getOp2p2(stcode[i]);
        G_ASSERT(id < vertexProps.sc.size());
        int saveId = id + base;
        int destId = vertexProps.sc[id].regIndex + base;
        cpp_stcode.patchConstLocation(STAGE_VS, saveId, destId);
        stcode[i] = shaderopcode::makeOp2_8_16(op, shaderopcode::getOp2p1(stcode[i]) >> 10, destId);
      }
      break;

      case SHCOD_INVERSE:
      case SHCOD_LVIEW:
      case SHCOD_TMWORLD:
      case SHCOD_ADD_REAL:
      case SHCOD_SUB_REAL:
      case SHCOD_MUL_REAL:
      case SHCOD_DIV_REAL:
      case SHCOD_ADD_VEC:
      case SHCOD_SUB_VEC:
      case SHCOD_MUL_VEC:
      case SHCOD_DIV_VEC:
      case SHCOD_GET_INT:
      case SHCOD_GET_TEX:
      case SHCOD_GET_GINT:
      case SHCOD_GET_GTEX:
      case SHCOD_GET_GBUF:
      case SHCOD_GET_GTLAS:
      case SHCOD_GET_GINT_TOREAL:
      case SHCOD_GET_GIVEC_TOREAL:
      case SHCOD_GET_GVEC:
      case SHCOD_GET_GMAT44:
      case SHCOD_GET_GREAL:
      case SHCOD_GET_INT_TOREAL:
      case SHCOD_GET_IVEC_TOREAL:
      case SHCOD_GET_REAL:
      case SHCOD_GET_VEC:
      case SHCOD_BLK_ICODE_LEN:
      case SHCOD_IMM_REAL1:
      case SHCOD_IMM_SVEC1:
      case SHCOD_COPY_REAL:
      case SHCOD_COPY_VEC: break;

      case SHCOD_IMM_REAL: i++; break;

      case SHCOD_IMM_VEC: i += 4; break;

      case SHCOD_MAKE_VEC: i++; break;

      case SHCOD_CALL_FUNCTION: i += shaderopcode::getOp3p3(stcode[i]); break;

      default: DAG_FATAL("stcode: %d '%s' not processed!", stcode[i], ShUtils::shcod_tokname(op));
    }
  }
}

static FastNameMap blockNames;
static Tab<ShaderStateBlock *> blocks(midmem);
static bindump::Ptr<ShaderStateBlock> emptyBlk;

ShaderStateBlock::ShaderStateBlock(const char *nm, int lev, const NamedConstBlock &ncb, dag::Span<int> stcode,
  StcodeRoutine *cpp_stcode, int maxregsize, StaticCbuf supp_static_cbuf) :
  shConst(ncb), name(nm), nameId(-1), stcodeId(-1), layerLevel(lev), regSize(maxregsize)
{
  memset(&start, 0, sizeof(start));
  memset(&final, 0, sizeof(final));

  supportsStaticCbuf = supp_static_cbuf;
  for (int i = 0; i < shConst.suppBlk.size(); i++)
  {
    const ShaderStateBlock &b = *shConst.suppBlk[i];
#define GET_MAX(x)         \
  if (start.x < b.final.x) \
  start.x = b.final.x
    GET_MAX(vsf);
    GET_MAX(vsmp);
    GET_MAX(psf);
    GET_MAX(smp);
    GET_MAX(sampler);
    GET_MAX(csf);
    GET_MAX(uav);
    GET_MAX(vs_uav);
    GET_MAX(cs_buf);
    GET_MAX(ps_buf);
    GET_MAX(vs_buf);
    GET_MAX(cs_cbuf);
    GET_MAX(ps_cbuf);
    GET_MAX(vs_cbuf);
#undef GET_MAX
    if (supportsStaticCbuf != StaticCbuf::NONE)
    {
      if ((b.supportsStaticCbuf == StaticCbuf::NONE && (b.final.vs_cbuf || b.final.ps_cbuf)))
      {
        if (lev != ShaderStateBlock::LEV_SHADER)
          sh_debug(SHLOG_ERROR, "block <%s> supports __static_cbuf but base block <%s> doesn't", name, b.name);
        supportsStaticCbuf = StaticCbuf::NONE;
      }
      else if (supportsStaticCbuf == StaticCbuf::SINGLE && b.supportsStaticCbuf == StaticCbuf::ARRAY)
        supportsStaticCbuf = StaticCbuf::ARRAY;
    }
  }
  if (lev != ShaderStateBlock::LEV_SHADER && supportsStaticCbuf != StaticCbuf::NONE)
  {
    if (start.vs_cbuf == 0)
      start.vs_cbuf = 1;
    if (start.ps_cbuf == 0)
      start.ps_cbuf = 1;
  }

  int start_cbuf_reg = 0;
  shConst.staticCbufType = (lev == ShaderStateBlock::LEV_SHADER && stcode.size() == 0) ? supportsStaticCbuf : StaticCbuf::NONE;
  if (shConst.staticCbufType != StaticCbuf::NONE)
  {
    bool has_static = false;
    bool missing_hlsl = false;
    for (int i = 0; i < shConst.vertexProps.sc.size(); i++)
    {
      const NamedConstBlock::NamedConst &nc = shConst.vertexProps.sc[i];
      const char *hlsl = nc.hlsl.c_str();
      if (nc.nameSpace == NamedConstSpace::vsf && !nc.isDynamic)
      {
        has_static = true;
        if (!*hlsl)
        {
          debug("missing HLSL for static %s%s", shConst.vertexProps.sn.getName(i),
            NamedConstBlock::nameSpaceToStr(shConst.vertexProps.sc[i].nameSpace));
          missing_hlsl = true;
        }
      }
    }
    for (int i = 0; i < shConst.pixelProps.sc.size(); i++)
    {
      const NamedConstBlock::NamedConst &nc = shConst.pixelProps.sc[i];
      const char *hlsl = nc.hlsl.c_str();
      if (nc.nameSpace == NamedConstSpace::psf && !nc.isDynamic)
      {
        has_static = true;
        if (!*hlsl)
        {
          debug("missing HLSL for static %s%s", shConst.pixelProps.sn.getName(i),
            NamedConstBlock::nameSpaceToStr(shConst.pixelProps.sc[i].nameSpace));
          missing_hlsl = true;
        }
      }
    }
    if (!has_static || missing_hlsl)
      shConst.staticCbufType = StaticCbuf::NONE;
    else if (supportsStaticCbuf != StaticCbuf::NONE || (start.vs_cbuf == 0 && start.ps_cbuf == 0))
    {
      if (start.vs_cbuf == 0)
        start.vs_cbuf = 1;
      if (start.ps_cbuf == 0)
        start.ps_cbuf = 1;
    }
    else
    {
      sh_debug(SHLOG_ERROR, "STATIC_CBUF cannot be used while supported blocks contain @cbuf"
                            " and don't support __static_cbuf explicitly");
      for (int i = 0; i < shConst.suppBlk.size(); i++)
        if (shConst.suppBlk[i]->supportsStaticCbuf == StaticCbuf::NONE)
          sh_debug(SHLOG_ERROR, "  block <%s> final.vs_cbuf=%d final.ps_cbuf=%d", shConst.suppBlk[i]->name,
            shConst.suppBlk[i]->final.vs_cbuf, shConst.suppBlk[i]->final.ps_cbuf);
    }
    if (missing_hlsl)
      sh_debug(SHLOG_WARNING, "STATIC_CBUF disabled due to missing HLSL decl for static constants");
  }

#define ARRANGE_REG(x) \
  final.x =            \
    shConst.arrangeRegIndices(start.x, NamedConstSpace::x, shConst.staticCbufType != StaticCbuf::NONE ? &start_cbuf_reg : nullptr);

  ARRANGE_REG(vsf);
  ARRANGE_REG(vsmp);
  ARRANGE_REG(psf);
  ARRANGE_REG(smp);
  final.sampler = shConst.arrangeRegIndices(final.smp, NamedConstSpace::sampler,
    shConst.staticCbufType != StaticCbuf::NONE ? &start_cbuf_reg : nullptr);
  ARRANGE_REG(csf);
  ARRANGE_REG(uav);
  ARRANGE_REG(vs_uav);
  ARRANGE_REG(cs_buf);
  ARRANGE_REG(ps_buf);
  ARRANGE_REG(vs_buf);
  ARRANGE_REG(cs_cbuf);
  ARRANGE_REG(ps_cbuf);
  ARRANGE_REG(vs_cbuf);
#undef ARRANGE_REG

  final.smp = final.sampler;

  G_ASSERTF(stcode.empty() || cpp_stcode, "If you pass non-empty stcode for patching, also pass the cpp stcode!");

  if (cpp_stcode)
  {
    shConst.patchStcodeIndices(stcode, *cpp_stcode, false);
    if (stcode.size())
    {
      auto [id, isNew] = add_stcode(stcode);
      stcodeId = id;
      if (isNew)
        g_cppstcode.addCode(eastl::move(*cpp_stcode), stcodeId);
    }
  }
}
int ShaderStateBlock::getVsNameId(const char *name)
{
  int id = shConst.vertexProps.sn.getNameId(name);
  if (id != -1)
    return id;
  for (int i = 0; i < shConst.suppBlk.size(); i++)
  {
    id = shConst.suppBlk[i]->getVsNameId(name);
    if (id != -1)
      return id;
  }
  return -1;
}
int ShaderStateBlock::getPsNameId(const char *name)
{
  int id = shConst.pixelProps.sn.getNameId(name);
  if (id != -1)
    return id;
  for (int i = 0; i < shConst.suppBlk.size(); i++)
  {
    id = shConst.suppBlk[i]->getPsNameId(name);
    if (id != -1)
      return id;
  }
  return -1;
}

bool ShaderStateBlock::registerBlock(ShaderStateBlock *blk, bool allow_identical_redecl)
{
  int id = blockNames.addNameId(blk->name.c_str());
  if (id < blocks.size())
  {
    if (!allow_identical_redecl)
      return false;

    ShaderStateBlock &sb = *blocks[id];
    if (sb.layerLevel != blk->layerLevel)
    {
      sh_debug(SHLOG_ERROR, "non-ident block <%s> redecl: layerLevel=%d != %d(prev)", blk->name.c_str(), blk->layerLevel,
        sb.layerLevel);
      return false;
    }
    if (sb.stcodeId != blk->stcodeId)
    {
      sh_debug(SHLOG_ERROR, "non-ident block <%s> redecl: stcodeId=%d != %d(prev)", blk->name.c_str(), blk->stcodeId, sb.stcodeId);
      return false;
    }
    if (sb.shConst.suppBlk != blk->shConst.suppBlk)
    {
      sh_debug(SHLOG_ERROR, "non-ident block <%s> redecl: supported block list changed", blk->name.c_str());
      return false;
    }
    return true;
  }

  G_ASSERT(blocks.size() == id);
  blocks.push_back(blk);
  blk->nameId = id;
  return true;
}
ShaderStateBlock *ShaderStateBlock::emptyBlock()
{
  static ShaderStateBlock empty;
  if (!emptyBlk)
    emptyBlk.create() = empty;
  return emptyBlk.get();
}
void ShaderStateBlock::deleteAllBlocks()
{
  clear_all_ptr_items(blocks);
  blockNames.reset();
  clear_and_shrink(tmpDigest);
}
ShaderStateBlock *ShaderStateBlock::findBlock(const char *name)
{
  int id = blockNames.getNameId(name);
  return (id < 0) ? NULL : blocks[id];
}
Tab<ShaderStateBlock *> &ShaderStateBlock::getBlocks() { return blocks; }
bindump::Ptr<ShaderStateBlock> &ShaderStateBlock::getEmptyBlock() { return emptyBlk; }

int ShaderStateBlock::countBlock(int level)
{
  if (level == -1)
    return blocks.size();
  return eastl::count_if(blocks.begin(), blocks.end(),
    [&level](const ShaderStateBlock *const blk) { return blk->layerLevel == level; });
}

void ShaderStateBlock::link(Tab<ShaderStateBlock *> &loaded_blocks, dag::ConstSpan<int> stcode_remap)
{
  for (ShaderStateBlock *&block : loaded_blocks)
  {
    ShaderStateBlock &b = *block;
    for (auto &blk : b.shConst.suppBlk)
    {
      const char *bname = blk->name.c_str();
      ShaderStateBlock *sb = !blk->name.empty() ? ShaderStateBlock::findBlock(bname) : ShaderStateBlock::emptyBlock();
      if (sb)
        blk = sb;
      else
        sh_debug(SHLOG_FATAL, "undefined block <%s>", bname);
    }
    if (stcode_remap.size() && b.stcodeId != -1)
    {
      if (b.stcodeId >= 0 && b.stcodeId < stcode_remap.size())
        b.stcodeId = stcode_remap[b.stcodeId];
      else
        sh_debug(SHLOG_FATAL, "block <%s>: stcodeId=%d is out of range [0..%d]", b.name.c_str(), b.stcodeId, stcode_remap.size() - 1);
    }

    if (!registerBlock(&b, true))
    {
      sh_debug(SHLOG_FATAL, "can't register block <%s> - definition differs from previous", b.name.c_str());
      delete &b;
    }
  }
}
