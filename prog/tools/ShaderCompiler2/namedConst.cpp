#include "namedConst.h"
#include "shsyn.h"
#include "shLog.h"
#include "shsem.h"
#include "linkShaders.h"
#include <math/dag_mathBase.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>
#include <shaders/shUtils.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>
#include "hash.h"
#include "fast_isalnum.h"
#include <algorithm>

#if _CROSS_TARGET_DX12
#include "dx12/asmShaderDXIL.h"
extern dx12::dxil::Platform targetPlatform;
#endif

static constexpr const char *EMPTY_BLOCK_NAME = "__empty_block__";

static String tmpDigest;
extern int hlsl_bones_base_reg;
extern bool enableBindless;

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
    case NamedConstSpace::uav: return "@uav";
    default: return "?unk?";
  }
}

static int remap_register(int r, NamedConstSpace ns)
{
  if (ns == NamedConstSpace::uav)
  {
    // reversed order of slots to avoid overlap with RTV
    r = 7 - r;
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

    default: fatal("unsupported namespace: %d", (int)name_space);
  }
}

int NamedConstBlock::arrangeRegIndices(int base_reg, NamedConstSpace name_space, int *base_cblock_reg)
{
  int vpr_arr_id = -1;
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
        if (hardcoded_regs[translate_namespace(name_space)].count(remap_register(r, name_space)))
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
      for (NamedConst &nc : vertexProps.sc)
      {
        if (nc.nameSpace == name_space && !nc.isDynamic && !(base_cblock_reg && nc.nameSpace == NamedConstSpace::vsf))
        {
          if (nc.size > 0)
          {
            if (needShift && base_reg == 1)
              base_reg++;

            base_reg = find_first_free_reg(vertexProps.hardcoded_regs, nc.size);

            if (nc.regIndex == -1)
            {
              nc.regIndex = remap_register(base_reg, nc.nameSpace);
              base_reg += nc.size;
            }
          }
          else
            sh_debug(SHLOG_ERROR, "ICE: vpr_array marked as static?");
        }
      }

      for (int i = 0; i < vertexProps.sc.size(); i++)
      {
        NamedConst &nc = vertexProps.sc[i];
        if (nc.nameSpace == name_space && nc.isDynamic)
        {
          if (nc.size < 0)
          {
            if (vpr_arr_id != -1)
              sh_debug(SHLOG_ERROR, "more than one vpr_array const");
            vpr_arr_id = i;
          }
          else
          {
            if (needShift && base_reg == 1)
              base_reg++;

            base_reg = find_first_free_reg(vertexProps.hardcoded_regs, nc.size);

            if (nc.regIndex == -1)
            {
              nc.regIndex = remap_register(base_reg, nc.nameSpace);
              base_reg += nc.size;
            }
          }
        }
      }

      if (vpr_arr_id != -1)
      {
        if (hlsl_bones_base_reg < 0)
        {
          vertexProps.sc[vpr_arr_id].regIndex = base_reg;
          base_reg = 256;
        }
        else
        {
          vertexProps.sc[vpr_arr_id].regIndex = hlsl_bones_base_reg;
        }
        if (vertexProps.hardcoded_regs[translate_namespace(name_space)].count(vertexProps.sc[vpr_arr_id].regIndex))
          sh_debug(SHLOG_ERROR, "The register '%i' is reserved and cannot be hardcoded", vertexProps.sc[vpr_arr_id].regIndex);
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
      auto handle_consts = [&](auto pred) {
        for (NamedConst &nc : pixelProps.sc)
        {
          if (nc.nameSpace == name_space && pred(nc))
          {
            if (needShift && base_reg == 1)
              base_reg++;

            base_reg = find_first_free_reg(pixelProps.hardcoded_regs, nc.size);

            if (nc.regIndex == -1)
            {
              nc.regIndex = remap_register(base_reg, nc.nameSpace);
              base_reg += nc.size;
            }

            if (nc.nameSpace == NamedConstSpace::smp && nc.regIndex > 15)
            {
              sh_debug(SHLOG_ERROR, "More than 16 textures with samplers in shader are not allowed");
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

    default: fatal("unsupported namespace: %d", (int)name_space);
  }
  return base_reg;
}

int NamedConstBlock::getHierConstValEx(const char *name_buf, const char *blk_name, NamedConstSpace ns, bool pixel_shader)
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
      return suppBlk[i]->shConst.getHierConstVal(name_buf, ns, pixel_shader);
    else
    {
      int reg = suppBlk[i]->shConst.getHierConstValEx(name_buf, blk_name, ns, pixel_shader);
      if (reg >= 0)
        return reg;
    }
  return -1;
}

int NamedConstBlock::getHierConstVal(const char *name_buf, NamedConstSpace ns, bool pixel_shader)
{
  int id = pixel_shader ? pixelProps.sn.getNameId(name_buf) : vertexProps.sn.getNameId(name_buf);
  if (id == -1)
  {
    if (!suppBlk.size())
      return -1;

    int reg = suppBlk[0]->shConst.getHierConstVal(name_buf, ns, pixel_shader);
    if (reg < 0)
      return -1;

    for (int i = 1; i < suppBlk.size(); i++)
    {
      int nreg = suppBlk[i]->shConst.getHierConstVal(name_buf, ns, pixel_shader);
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

CryptoHash NamedConstBlock::getDigest(bool ps_const, bool cs_const) const
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
    if (hasStaticCbuf)
      buildStaticConstBufHlslDecl(res);
    buildHlslDeclText(res, ps_const, cs_const, added_names, hasStaticCbuf);
    hasher.update(res.data(), res.length());
  }

  return hasher.hash();
}

static void calc_final_idx(ShaderStateBlock::UsageIdx &f, ShaderStateBlock::UsageIdx &m, dag::ConstSpan<NamedConstBlock::NamedConst> n,
  bool &bones_used, bool static_cbuf = false)
{
#define CASE_ARRANGE_REG(X)                                                                             \
  case NamedConstSpace::X:                                                                              \
  {                                                                                                     \
    if (static_cbuf && n[i].nameSpace == NamedConstSpace::psf && n[i].regIndex == 0xFF)                 \
      break;                                                                                            \
    if (n[i].size >= 0 || hlsl_bones_base_reg < 0)                                                      \
      f.X = std::max(f.X, remap_register(n[i].regIndex, n[i].nameSpace) + std::max(0, (int)n[i].size)); \
    if (n[i].size >= 0)                                                                                 \
      m.X = std::max(m.X, remap_register(n[i].regIndex, n[i].nameSpace) + n[i].size);                   \
    else                                                                                                \
      bones_used = true;                                                                                \
    break;                                                                                              \
  }
  for (int i = 0; i < n.size(); i++)
    switch (n[i].nameSpace)
    {
      CASE_ARRANGE_REG(vsf);
      CASE_ARRANGE_REG(vsmp);
      CASE_ARRANGE_REG(psf);
      CASE_ARRANGE_REG(smp);
      CASE_ARRANGE_REG(csf);
      CASE_ARRANGE_REG(uav);
    }
#undef CASE_ARRANGE_REG
}

struct SamplerState
{
  String name;
  int regIdx;
  SamplerState() : regIdx(-1) {}
  SamplerState(const char *tname, int rIdx) : name(tname), regIdx(rIdx) {}
};

extern int hlsl_maximum_vsf_allowed;
extern int hlsl_maximum_psf_allowed;

static bool shall_remove_ns(NamedConstSpace ns, bool pixel_shader, bool compute_shader)
{
  const bool vertex_shader = !compute_shader && !pixel_shader;
  bool remove = (pixel_shader && (ns == NamedConstSpace::vsf || ns == NamedConstSpace::vsmp)) ||
                (!pixel_shader && (ns == NamedConstSpace::psf || ns == NamedConstSpace::smp)) ||
                (!compute_shader && (ns == NamedConstSpace::csf)) ||
                (!pixel_shader && (ns == NamedConstSpace::ps_buf || ns == NamedConstSpace::ps_cbuf)) ||
                (!compute_shader && (ns == NamedConstSpace::cs_buf || ns == NamedConstSpace::cs_cbuf)) ||
                (!vertex_shader && (ns == NamedConstSpace::vs_buf || ns == NamedConstSpace::vs_cbuf)) ||
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

static const char *get_hlsl_decl_end(const char *hlsl)
{
  const char *p = strchr(hlsl, '\n');
  if (!p)
    return hlsl;
  if (const char *p1 = strchr(p + 1, ':'))
    return p1;
  if (const char *p1 = strchr(p + 1, '@'))
    return p1;
  return hlsl;
}

const char *MATERIAL_PROPS_NAME = "materialProps";

template <enum NamedConstSpace TOKEN_TO_PROCESS>
static void gather_static_consts(const NamedConstBlock::RegisterProperties &reg_props,
  const NamedConstBlock::RegisterProperties *prev_props, String &out_hlsl, String &access_functions, int &our_regs_count)
{
  for (int i = 0; i < reg_props.sc.size(); i++)
  {
    const NamedConstBlock::NamedConst &nc = reg_props.sc[i];
    const char *hlsl = nc.hlsl.c_str();
    if (nc.nameSpace != TOKEN_TO_PROCESS || nc.isDynamic)
      continue;
    const char *nm = reg_props.sn.getName(i);
    if (prev_props && prev_props->sn.getNameId(nm) >= 0) // skip duplicate VS/PS vars
      continue;
    const char *pe = get_hlsl_decl_end(hlsl);
    G_ASSERTF(nc.regIndex == our_regs_count, "ICE: nc.regIndex=%d regIndex=%d", nc.regIndex, our_regs_count);
    out_hlsl.aprintf(0, "  %.*s;", pe - hlsl, hlsl);
    our_regs_count += nc.size;
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
    const char *typeNameBegin = nullptr;
    for (const char *type : TYPE_NAMES)
    {
      typeNameBegin = strstr(hlsl, type);
      if (typeNameBegin)
        break;
    }
    access_functions.aprintf(0, "%.*s get_%s() { return %s[DRAW_CALL_ID].%s; }\n", strchr(typeNameBegin, ' ') - typeNameBegin,
      typeNameBegin, nm, MATERIAL_PROPS_NAME, nm);
    if (!strstr(hlsl, "float4 ") && !strstr(hlsl, "float4x4 ") && !strstr(hlsl, "int4 ")) // partial vector, needs padding
    {
      if (const char *p = strstr(hlsl, "float "))
        out_hlsl.aprintf(0, " float __pad1_%s, __pad2_%s, __pad3_%s;", nm, nm, nm);
      else if (const char *p = strstr(hlsl, "float2 "))
        out_hlsl.aprintf(0, " float2 __pad_%s;", nm);
      else if (const char *p = strstr(hlsl, "uint2 "))
        out_hlsl.aprintf(0, " float2 __pad_%s;", nm);
      else if (const char *p = strstr(hlsl, "float3 "))
        out_hlsl.aprintf(0, " float __pad_%s;", nm);
      else if (const char *p = strstr(hlsl, "int "))
        out_hlsl.aprintf(0, " int __pad1_%s, __pad2_%s, __pad3_%s;", nm, nm, nm);
      else if (const char *p = strstr(hlsl, "int2 "))
        out_hlsl.aprintf(0, " int2 __pad_%s;", nm);
      else if (const char *p = strstr(hlsl, "int3 "))
        out_hlsl.aprintf(0, " int __pad_%s;", nm);
      else
        G_ASSERTF(0, "ICE: unexpected decl: %s", hlsl);
    }
    out_hlsl += '\n';
  }
}

void NamedConstBlock::buildStaticConstBufHlslDecl(String &out_text) const
{
  const bool shaderSupportsMultidraw = staticCbufType == StaticCbuf::ARRAY;
  const char *bindlessProlog = "";
  const char *bindlessType = "";

  if (enableBindless)
  {
#if _CROSS_TARGET_C1 || _CROSS_TARGET_C2


#elif _CROSS_TARGET_SPIRV
    bindlessProlog = "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] Texture2D static_textures[];\n"
                     "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures_cube[];\n"
                     "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] Texture2DArray static_textures_array[];\n"
                     "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures_cube_array[];\n"
                     "[[vk::binding(0, BINDLESS_TEXTURE_SET_META_ID)]] TextureCube static_textures3d[];\n"
                     "[[vk::binding(0, BINDLESS_SAMPLER_SET_META_ID)]] SamplerState static_samplers[];\n";
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

  const char *drawCallIdDeclaration =
    shaderSupportsMultidraw ? "static uint DRAW_CALL_ID = 10000;\n" : "static const uint DRAW_CALL_ID = 0;\n";
  int regIndex = 0;
  String constantAccessFunctions;
  String materialPropsStruct;
  gather_static_consts<NamedConstSpace::vsf>(vertexProps, nullptr, materialPropsStruct, constantAccessFunctions, regIndex);
  gather_static_consts<NamedConstSpace::psf>(pixelProps, &vertexProps, materialPropsStruct, constantAccessFunctions, regIndex);

  const uint32_t MAX_CBUFFER_VECTORS = 4096;
  const uint32_t propSetsCount = shaderSupportsMultidraw ? MAX_CBUFFER_VECTORS / regIndex : 1;

  out_text.printf(0,
    "%s%s%s"
    "struct MaterialProperties\n{\n%s\n};\n\n"
    "cbuffer shader_static_cbuf:register(b1) { MaterialProperties %s[%d]; };\n\n%s\n\n",
    bindlessType, drawCallIdDeclaration, bindlessProlog, materialPropsStruct, MATERIAL_PROPS_NAME, propSetsCount,
    constantAccessFunctions);
}

void NamedConstBlock::buildGlobalConstBufHlslDecl(String &out_text, bool pixel_shader, bool compute_shader,
  SCFastNameMap &added_names) const
{
  String res;
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

extern String hlsl_defines;

void NamedConstBlock::patchHlsl(String &src, bool pixel_shader, bool compute_shader, int &max_const_no_used, int &bones_const_no_used)
{
  bones_const_no_used = -1;
  max_const_no_used = -1;
  int bones_base_reg = pixel_shader ? -1 : hlsl_bones_base_reg;
  bool bones_used = false;
  static String res, name_buf, blk_name_buf;
  int name_prefix_len = 0;
  res.clear();
  const bool doesShaderGlobalExist = ShaderStateBlock::countBlock(ShaderStateBlock::LEV_GLOBAL_CONST) > 0;

  if (hlsl_defines.length())
    src.insert(0, hlsl_defines.c_str());
  // add target-specific predefines
  const char *predefines_str =
#if _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_DX12
    dx12::dxil::Platform::XBOX_ONE == targetPlatform
      ?
#include "predefines_dx12x.hlsl.inl"
      : dx12::dxil::Platform::XBOX_SCARLETT == targetPlatform ?
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
  if (predefines_str)
    src.insert(0, predefines_str);

  bool hasStaticCbuf = staticCbufType != StaticCbuf::NONE;
  {
    SCFastNameMap added_names;
    if (hasStaticCbuf)
      buildStaticConstBufHlslDecl(res);
    buildHlslDeclText(res, pixel_shader, compute_shader, added_names, hasStaticCbuf);
  }
  if (const char *p = strstr(src, "#line "))
    src.insert(p - src.data(), res);
  else
    src.insert(0, res);
  clear_and_shrink(res);
  const char *text = src, *p, *start = text;

  // scan for hardcoded constants, : register(c#)
  ShaderStateBlock::UsageIdx final;
  bool final_inited = false;
  ShaderStateBlock::UsageIdx maximum;
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
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
    if (!strchr("tcsu", *p))
#else
    if (!strchr("csu", *p))
#endif
    {
      text = p;
      continue;
    }
    int regt_sym = *p;
    NamedConstSpace regt = NamedConstSpace::unknown;
    int idx = atoi(p + 1);
    int f_idx = 0;
    bool overlaps = false;
    switch (regt_sym)
    {
      case 'c': regt = compute_shader ? NamedConstSpace::csf : (pixel_shader ? NamedConstSpace::psf : NamedConstSpace::vsf); break;
      case 's': regt = pixel_shader ? NamedConstSpace::smp : NamedConstSpace::vsmp; break;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
      case 't': regt = pixel_shader ? NamedConstSpace::smp : NamedConstSpace::vsmp; break;
#endif
      case 'u': regt = NamedConstSpace::uav; break;
      default: G_ASSERTF(false, "Unknown namespace %c", regt_sym);
    }
    if (!final_inited)
    {
      bool psh_bones_used;
      memset(&final, 0, sizeof(final));
      memset(&maximum, 0, sizeof(maximum));
      calc_final_idx(final, maximum, pixelProps.sc, psh_bones_used, hasStaticCbuf);
      calc_final_idx(final, maximum, vertexProps.sc, bones_used, hasStaticCbuf);
      for (int i = 0; i < suppBlk.size(); i++)
      {
        calc_final_idx(final, maximum, suppBlk[i]->shConst.pixelProps.sc, psh_bones_used);
        calc_final_idx(final, maximum, suppBlk[i]->shConst.vertexProps.sc, bones_used);
      }
      final_inited = true;
    }
    idx = remap_register(idx, regt);
    switch (regt)
    {
#define CASE_TYPE(X)                    \
  case NamedConstSpace::X:              \
    overlaps = idx < (f_idx = final.X); \
    if (idx != bones_base_reg)          \
      maximum.X = max(maximum.X, idx);  \
    else                                \
      bones_used = true;                \
    break
      CASE_TYPE(vsf);
      CASE_TYPE(vsmp);
      CASE_TYPE(psf);
      CASE_TYPE(smp);
      CASE_TYPE(csf);
      CASE_TYPE(uav);
#undef CASE_TYPE
    }

    p++;
    while (isdigit(*p))
      p++;
    const char *fragment_end = p;
    while (*fragment_end && !strchr("\r\n", *fragment_end))
      fragment_end++;

    if (hlsl_maximum_vsf_allowed < maximum.vsf)
    {
      sh_debug(SHLOG_ERROR, "maximum register No used (%d) is bigger then allowed (%d)", maximum.vsf, hlsl_maximum_vsf_allowed);
    }
    if (hlsl_maximum_psf_allowed < maximum.psf)
    {
      sh_debug(SHLOG_ERROR, "maximum register No used (%d) is bigger then allowed (%d)", maximum.psf, hlsl_maximum_psf_allowed);
    }
    if (hlsl_maximum_psf_allowed < maximum.csf)
    {
      sh_debug(SHLOG_ERROR, "maximum register No used (%d) is bigger then allowed (%d)", maximum.csf, hlsl_maximum_psf_allowed);
    }

    if (overlaps)
    {
      sh_debug(SHLOG_ERROR, "hardcoded register constant overlaps with named ones");
      sh_debug(SHLOG_ERROR, "  %s: idx=%d, while named reserve upto %d", nameSpaceToStr(regt), remap_register(idx, regt), f_idx - 1);
      sh_debug(SHLOG_ERROR, "found %s %d overlaps with named, final index=%d, at HLSL code:\n\"%.*s\"\n", nameSpaceToStr(regt),
        remap_register(idx, regt), f_idx, fragment_end - fragment_start, fragment_start);
      debug("named consts map %s", nameSpaceToStr(regt));
      if (pixel_shader)
      {
        for (int j = 0; j < suppBlk.size(); j++)
          for (int i = 0; i < suppBlk[j]->shConst.pixelProps.sc.size(); i++)
            if (suppBlk[j]->shConst.pixelProps.sc[i].nameSpace == regt)
              debug("  %s: %d (sz=%d)", suppBlk[j]->shConst.pixelProps.sn.getName(i), suppBlk[j]->shConst.pixelProps.sc[i].regIndex,
                suppBlk[j]->shConst.pixelProps.sc[i].size);
        for (int i = 0; i < pixelProps.sc.size(); i++)
          if (pixelProps.sc[i].nameSpace == regt)
            debug("  %s: %d (sz=%d)", pixelProps.sn.getName(i), pixelProps.sc[i].regIndex, pixelProps.sc[i].size);
      }
      else
      {
        for (int j = 0; j < suppBlk.size(); j++)
          for (int i = 0; i < suppBlk[j]->shConst.vertexProps.sc.size(); i++)
            if (suppBlk[j]->shConst.vertexProps.sc[i].nameSpace == regt)
              debug("  %s: %d (sz=%d)", suppBlk[j]->shConst.vertexProps.sn.getName(i), suppBlk[j]->shConst.vertexProps.sc[i].regIndex,
                suppBlk[j]->shConst.vertexProps.sc[i].size);
        for (int i = 0; i < vertexProps.sc.size(); i++)
          if (vertexProps.sc[i].nameSpace == regt)
            debug("  %s: %d (sz=%d)", vertexProps.sn.getName(i), vertexProps.sc[i].regIndex, vertexProps.sc[i].size);
      }
      debug("");
    }
    text = p;
  }
  if (!final_inited)
  {
    bool psh_bones_used;
    memset(&final, 0, sizeof(final));
    memset(&maximum, 0, sizeof(maximum));
    calc_final_idx(final, maximum, pixelProps.sc, psh_bones_used, hasStaticCbuf);
    calc_final_idx(final, maximum, vertexProps.sc, bones_used, hasStaticCbuf);
    for (int i = 0; i < suppBlk.size(); i++)
    {
      calc_final_idx(final, maximum, suppBlk[i]->shConst.pixelProps.sc, psh_bones_used);
      calc_final_idx(final, maximum, suppBlk[i]->shConst.vertexProps.sc, bones_used);
    }
    final_inited = true;
  }
  if (compute_shader) // pixel & compute
    max_const_no_used = maximum.csf;
  else if (pixel_shader)
    max_const_no_used = maximum.psf;
  else
  {
    max_const_no_used = maximum.vsf;
    bones_const_no_used = bones_used ? bones_base_reg : -1;
  }

#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
  Tab<SamplerState> samplersX[2];
#endif
  text = src;
  while ((p = strchr(text, '@')) != NULL)
  {
    NamedConstSpace ns = NamedConstSpace::unknown;
    int mnemo_len = 0;
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
    bool is_shadow = false;
#endif
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
    else HANDLE_NS(ps_cbuf);
    else HANDLE_NS(vs_cbuf);
    else HANDLE_NS(cs_cbuf);
    else HANDLE_NS(uav);
    else HANDLE_NS(sampler);
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
    else if (strncmp(p + 1, "shd", 3) == 0)
    {
      ns = (pixel_shader | compute_shader) ? NamedConstSpace::smp : NamedConstSpace::vsmp;
      is_shadow = true;
      mnemo_len = 4;
    }
#endif
    G_ASSERT(mnemo_len > 0);
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
        regIndex = getHierConstVal(name_buf, ns, pixel_shader);
      else
        regIndex = getHierConstValEx(name_buf, blk_name_buf.str(), ns, pixel_shader);

      if (regIndex < 0)
        remove = true;
    }
    if (!remove)
    {
      char csym = 'c';
      if (ns == NamedConstSpace::smp)
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
        csym = is_shadow ? 's' : 't';
#else
        csym = 's';
#endif
      else if (ns == NamedConstSpace::vsmp)
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
        csym = is_shadow ? 's' : 't';
#else
        csym = 's';
#endif
      else if (ns == NamedConstSpace::sampler)
        csym = 's';
      else if (ns == NamedConstSpace::uav)
        csym = 'u';
      else if (ns == NamedConstSpace::ps_buf || ns == NamedConstSpace::vs_buf || ns == NamedConstSpace::cs_buf)
        csym = 't';
      else if (ns == NamedConstSpace::ps_cbuf || ns == NamedConstSpace::vs_cbuf || ns == NamedConstSpace::cs_cbuf)
      {
        csym = 'b';
        if (doesShaderGlobalExist && regIndex == 2)
          sh_debug(SHLOG_ERROR, "cbuf register b2 is reserved (%s)!", name_buf);
      }

#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
      if (!remove && (ns == NamedConstSpace::smp || ns == NamedConstSpace::vsmp))
      {
        Tab<SamplerState> &samplers = samplersX[(ns == NamedConstSpace::smp) ? 0 : 1];
        int i;
        for (i = 0; i < samplers.size(); ++i)
          if (!strcmp(samplers[i].name.str(), name_buf))
            break;
        if (i >= samplers.size())
          samplers.push_back(SamplerState(name_buf, regIndex));
      }
#endif

      if (replaceType == '#')
        res.aprintf(32, "%.*s%d", name - 1 - text, text, regIndex);
      else if (replaceType == '$')
        res.aprintf(32, "%.*s%c%d", name - 1 - text, text, csym, regIndex);
      else if (!name_prefix_len)
        res.aprintf(32, "%.*s: register(%c%d)", p - text, text, csym, regIndex);
      else
        res.aprintf(32, "%.*s%s: register(%c%d)", p - text - name_prefix_len, text, name_buf.str(), csym, regIndex);
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
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2 | _CROSS_TARGET_DX11 | _CROSS_TARGET_SPIRV | _CROSS_TARGET_METAL
  String ps4_samplerstates;
  Tab<SamplerState> &samplers = samplersX[pixel_shader ? 0 : 1];
  for (int i = 0; i < samplers.size(); ++i)
  {
#if _CROSS_TARGET_C1 | _CROSS_TARGET_C2



#endif
      ps4_samplerstates.aprintf(128, "SamplerState %s_samplerstate: register(s%d);\n", samplers[i].name, samplers[i].regIdx);
  }
  if (predefines_str)
    src.printf(1024, "%s\n%s\n%s", predefines_str, ps4_samplerstates.str(), res.str() + strlen(predefines_str));
  else
    src.printf(1024, "%s%s", ps4_samplerstates.str(), res.str());
#else
  src = res;
#endif
}

void NamedConstBlock::patchStcodeIndices(dag::Span<int> stcode, bool static_blk)
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
        id = shaderopcode::getOp3p1(stcode[i]);
        base = shaderopcode::getOp3p2(stcode[i]);
        G_ASSERT(id < ((op == SHCOD_VPR_CONST || op == SHCOD_TEXTURE_VS) ? vertexProps.sc.size() : pixelProps.sc.size()));

        dest =
          (op == SHCOD_VPR_CONST || op == SHCOD_TEXTURE_VS) ? vertexProps.sc[id].regIndex + base : pixelProps.sc[id].regIndex + base;
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
        stcode[i] = shaderopcode::makeOp2(op, dest, shaderopcode::getOp3p3(stcode[i]));
        break;
      case SHCOD_SAMPLER:
        id = shaderopcode::getOp2p1(stcode[i]);
        G_ASSERT(id < pixelProps.sc.size());
        dest = pixelProps.sc[id].regIndex;
        stcode[i] = shaderopcode::makeOp2(op, dest, shaderopcode::getOp2p2(stcode[i]));
        break;
      case SHCOD_BUFFER:
      case SHCOD_CONST_BUFFER:
      {
        const uint32_t stage = shaderopcode::getOpStageSlot_Stage(stcode[i]);
        uint32_t slot = shaderopcode::getOpStageSlot_Slot(stcode[i]);
        const uint32_t reg = shaderopcode::getOpStageSlot_Reg(stcode[i]);
        slot = (stage == STAGE_VS) ? vertexProps.sc[slot].regIndex : pixelProps.sc[slot].regIndex;
        stcode[i] = shaderopcode::makeOpStageSlot(op, stage, slot, reg);
      }
      break;

      case SHCOD_G_TM:
        id = shaderopcode::getOp2p1(stcode[i]) & 0x3FF;
        base = shaderopcode::getOp2p2(stcode[i]);
        G_ASSERT(id < vertexProps.sc.size());

        stcode[i] = shaderopcode::makeOp2_8_16(op, shaderopcode::getOp2p1(stcode[i]) >> 10, vertexProps.sc[id].regIndex + base);
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
      case SHCOD_GET_GINT_TOREAL:
      case SHCOD_GET_GVEC:
      case SHCOD_GET_GMAT44:
      case SHCOD_GET_GREAL:
      case SHCOD_GET_INT_TOREAL:
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

      default: fatal("stcode: %d '%s' not processed!", stcode[i], ShUtils::shcod_tokname(op));
    }
  }
}

static FastNameMap blockNames;
static Tab<ShaderStateBlock *> blocks(midmem);
static bindump::Ptr<ShaderStateBlock> emptyBlk;

ShaderStateBlock::ShaderStateBlock(const char *nm, int lev, const NamedConstBlock &ncb, dag::Span<int> stcode, int maxregsize,
  StaticCbuf supp_static_cbuf) :
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
  ARRANGE_REG(cs_buf);
  ARRANGE_REG(ps_buf);
  ARRANGE_REG(vs_buf);
  ARRANGE_REG(cs_cbuf);
  ARRANGE_REG(ps_cbuf);
  ARRANGE_REG(vs_cbuf);
#undef ARRANGE_REG

  shConst.patchStcodeIndices(stcode, false);
  if (stcode.size())
    stcodeId = add_stcode(stcode);
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
