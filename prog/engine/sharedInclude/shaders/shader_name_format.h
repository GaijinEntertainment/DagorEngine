// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shader_name_format_types.h"
#include "shader_layout.h"
#include <fast_float/fast_float.h>
#include <generic/dag_expected.h>
#include <ioSys/dag_dataBlock.h>

// Common format for shader variant naming for tooling and logging
//
// minimal-name (key for tools):
//  [dump-identifier@]shader-class-name#static-var-idx.dynamic-var-idx
//
// human-readable-name (for display in logs/capture tools, 'static' part):
//  minimal-name < [STATIC: (variable-name=value )*][DYNAMIC: (variable-name=value )*>
//  mode intervals print as 2SIDED=ON|OFF and REAL2SIDED=ON|OFF in place of variable-name=value
//
// list-of-aliases (same, but conservatively list all aliasing names, used when listAllAliases=true):
//  human-readable-name( \| human-readable-name)*
//
// drivers are allowed to
// - Add render state information after a human readable name
// - Compose human readable names of different modules to form pipeline names

namespace shader_name_format
{

// Consumes only the string prefix -- whatever comes after is discarded, it can be free-form additional info
inline dag::Expected<VariantIdentifier, eastl::string> parse_variant_name(eastl::string_view name,
  const VariantNameParsingSettings &settings = {})
{
  using Error = dag::Unexpected<eastl::string>;

  VariantIdentifier ident{};

  if (size_t dumpSep = name.find('@'); dumpSep != eastl::string::npos)
  {
    ident.shDumpName = eastl::string{name.begin(), dumpSep};
    name = name.substr(dumpSep + 1);
  }
  size_t classSep = name.find('#');
  if (classSep == eastl::string::npos)
    return Error{"missing mandatory shader class separator '#'"};
  else if (classSep == 0)
    return Error{"shader class name is empty"};
  ident.shClassName = eastl::string{name.begin(), classSep};
  name = name.substr(classSep + 1);

  if (name.empty())
    return Error{"missing variant index string"};

  auto [ptrs, ecs] = fast_float::from_chars(name.data(), name.end(), ident.stVarCode);
  if (ecs != std::errc{})
  {
    return Error{eastl::in_place, eastl::string::CtorSprintf{}, "invalid static variant index string: %s",
      std::make_error_condition(ecs).message().c_str()};
  }
  else if (ident.stVarCode < 0)
  {
    return Error{"invalid negative static variant index"};
  }
  else if (ptrs != name.end() && *ptrs != '.')
  {
    return Error{"missing separator '.' between static and dynamic variant indices"};
  }
  else if (ptrs == name.end())
  {
    if (settings.allowNoDynamicVariant)
      return ident;
    else
      return Error{"missing dynamic variant index"};
  }
  name = name.substr(ptrs - name.data() + 1);

  auto [ptrd, ecd] = fast_float::from_chars(name.data(), name.end(), ident.dynVarCode);
  if (ecd != std::errc{})
  {
    return Error{eastl::in_place, eastl::string::CtorSprintf{}, "invalid dynamic variant index string: %s",
      std::make_error_condition(ecd).message().c_str()};
  }
  else if (ident.dynVarCode < 0)
  {
    return Error{"invalid negative dynamic variant index"};
  }

  return ident;
}

inline void truncate_variant_name(auto &out, const VariantNameCompilationSettings &settings = {})
{
  G_ASSERT(settings.maxNameLen > 0);
  if (out.length() > settings.maxNameLen)
  {
    out.resize(settings.maxNameLen);
    char *p = &out[out.length() - 1];
    for (size_t i = 0; i < min<size_t>(3, out.length()); ++i)
      *p-- = '.';
  }
}

inline void compile_minimal_variant_name(auto &out, const VariantIdentifierRef &ident,
  const VariantNameCompilationSettings &settings = {})
{
  if (!ident.shDumpName.empty() && !settings.omitDumpName)
    out.append_sprintf("%s@", ident.shDumpName.data());
  out.append(ident.shClassName.data());
  out.append_sprintf("#%u.%u", ident.stVarCode, ident.dynVarCode);
  truncate_variant_name(out, settings);
}

inline void append_interval_value(auto &out, const bindump::Mapper<shader_layout::ScriptedShadersBinDumpV2> &dump, const auto &ival,
  int subcode, const VariantNameCompilationSettings &settings)
{
  if (settings.intervalValuesAsNames && ival.type == ival.TYPE_GLOBAL_INTERVAL)
  {
    const auto &ivalName = dump.varMap[ival.nameId];
    uint32_t ivalNameHash = mem_hash_fnv1<32>(ivalName.c_str(), ivalName.length());
    const auto &ivalInfo = dump.getIntervalInfoByHash(ivalNameHash);
    G_ASSERT(subcode >= 0);
    G_ASSERT(subcode < ivalInfo.subintervals.size());
    out.append(ivalInfo.subintervals[subcode].c_str());
  }
  else
  {
    // TODO: add static variant names as well
    out.append_sprintf("#%d", subcode);
  }
}

inline void decode_variant_str(auto &out, const bindump::Mapper<shader_layout::ScriptedShadersBinDumpV2> &dump,
  dag::ConstSpan<bindump::Mapper<shader_layout::VariantTable>::IntervalBind> p, unsigned code,
  const VariantNameCompilationSettings &settings = {})
{
  for (int i = 0; i < p.size(); i++)
  {
    if (out.length() >= settings.maxNameLen)
      break;
    using Interval = bindump::Mapper<shader_layout::Interval>;
    const Interval &ival = dump.intervals[p[i].intervalId];
    int mul = p[i].totalMul;
    int subcode = (code / mul) % ival.getValCount();
    switch (ival.type)
    {
      case Interval::TYPE_MODE:
        switch (ival.nameId)
        {
          case Interval::MODE_2SIDED: out.append_sprintf("2SIDED=%s", subcode ? "ON" : "OFF"); break;
          case Interval::MODE_REAL2SIDED: out.append_sprintf("REAL2SIDED=%s", subcode ? "ON" : "OFF"); break;
        }
        break;

      case Interval::TYPE_INTERVAL:
      case Interval::TYPE_GLOBAL_INTERVAL:
        out.append_sprintf("%s=", dump.varMap[ival.nameId].c_str());
        append_interval_value(out, dump, ival, subcode, settings);
        break;
    }

    out.append(" ");
  }
}

inline void decode_variant_diff_str(auto &out, const bindump::Mapper<shader_layout::ScriptedShadersBinDumpV2> &dump,
  dag::ConstSpan<bindump::Mapper<shader_layout::VariantTable>::IntervalBind> p, unsigned from_code, unsigned to_code,
  const VariantNameCompilationSettings &settings = {})
{
  for (int i = 0; i < p.size(); i++)
  {
    if (out.length() >= settings.maxNameLen)
      break;
    const auto &ival = dump.intervals[p[i].intervalId];
    int mul = p[i].totalMul;
    int fromSubcode = (from_code / mul) % ival.getValCount();
    int toSubcode = (to_code / mul) % ival.getValCount();
    if (fromSubcode == toSubcode)
      continue;
    out.append_sprintf("%s=", dump.varMap[ival.nameId].c_str());
    append_interval_value(out, dump, ival, fromSubcode, settings);
    out.append("->");
    append_interval_value(out, dump, ival, toSubcode, settings);
    out.append(" ");
  }
}

inline void compile_human_readable_variant_name_single(auto &out, const VariantIdentifierRef &ident,
  const bindump::Mapper<shader_layout::ScriptedShadersBinDump> &dump,
  const bindump::Mapper<shader_layout::ScriptedShadersBinDumpV2> &interval_storage_dump,
  const VariantNameCompilationSettings &settings = {})
{
  compile_minimal_variant_name(out, ident, settings);
  if (settings.minimalNamesOnly)
    return;
  const auto *sclass = dump.findShaderClass(ident.shClassName.data());
  G_ASSERT_RETURN(sclass, );
  const int stVarId = sclass->stVariants.findVariant(ident.stVarCode);
  G_ASSERT_RETURN(stVarId != sclass->stVariants.FIND_NOTFOUND, );
  G_ASSERT_RETURN(stVarId != sclass->stVariants.FIND_NULL, );
  G_ASSERT_RETURN(stVarId < sclass->code.size(), );
  const auto *code = &sclass->code[stVarId];
  const bool hasStatic = sclass->stVariants.codePieces.size() > 0;
  const bool hasDynamic = code->dynVariants.codePieces.size() > 0;
  if (!hasStatic && !hasDynamic)
    return;
  out.append(" < ");
  if (hasStatic)
  {
    out.append("STATIC: ");
    decode_variant_str(out, interval_storage_dump, sclass->stVariants.codePieces, ident.stVarCode, settings);
  }
  if (hasDynamic)
  {
    out.append("DYNAMIC: ");
    decode_variant_str(out, interval_storage_dump, code->dynVariants.codePieces, ident.dynVarCode, settings);
  }
  out.append(">");
  truncate_variant_name(out, settings);
}

inline const shader_layout::detail::ShRef *find_pass_for_enumeration(const VariantIdentifierRef &ident,
  const bindump::Mapper<shader_layout::ScriptedShadersBinDump> &dump)
{
  const auto *sclass = dump.findShaderClass(ident.shClassName.data());
  G_ASSERT_RETURN(sclass, nullptr);
  const int stVarId = sclass->stVariants.findVariant(ident.stVarCode);
  G_ASSERT_RETURN(stVarId != sclass->stVariants.FIND_NOTFOUND, nullptr);
  G_ASSERT_RETURN(stVarId != sclass->stVariants.FIND_NULL, nullptr);
  G_ASSERT_RETURN(stVarId < sclass->code.size(), nullptr);
  const auto *code = &sclass->code[stVarId];
  const int dynVarId = code->dynVariants.findVariant(ident.dynVarCode);
  G_ASSERT_RETURN(dynVarId != code->dynVariants.FIND_NOTFOUND, nullptr);
  G_ASSERT_RETURN(dynVarId != code->dynVariants.FIND_NULL, nullptr);
  G_ASSERT_RETURN(dynVarId < code->passes.size(), nullptr);
  return &(*code->passes[dynVarId].rpass);
}

inline void compile_human_readable_variant_name(auto &out, const VariantIdentifierRef &ident,
  const bindump::Mapper<shader_layout::ScriptedShadersBinDump> &dump,
  const bindump::Mapper<shader_layout::ScriptedShadersBinDumpV2> &interval_storage_dump,
  const VariantNameCompilationSettings &settings = {})
{
  compile_human_readable_variant_name_single(out, ident, dump, interval_storage_dump, settings);
  if (!settings.listAllAliases)
    return;
  if (out.length() >= settings.maxNameLen)
    return;
  G_ASSERT(ident.stage < STAGE_MAX);
  const auto *aliasedPass = find_pass_for_enumeration(ident, dump);
  auto passIsAliasing = [&](const auto &p) {
    return ident.stage == STAGE_VS ? (p.vprId == aliasedPass->vprId) : (p.fshId == aliasedPass->fshId);
  };
  for (auto &sclass : dump.classes)
  {
    auto ref = eastl::find_if(eastl::begin(sclass.shrefStorage), eastl::end(sclass.shrefStorage), passIsAliasing);
    if (ref == eastl::end(sclass.shrefStorage))
      continue;
    for (uint32_t stVarId = 0; stVarId < sclass.code.size(); ++stVarId)
    {
      const auto &code = sclass.code[stVarId];
      for (uint32_t dynVarId = 0; dynVarId < code.passes.size(); ++dynVarId)
      {
        const auto &pass = code.passes[dynVarId];
        if (!passIsAliasing(*pass.rpass))
          continue;

        sclass.stVariants.enumerateCodesForVariant(stVarId, [&](uint32_t st_var_code) {
          code.dynVariants.enumerateCodesForVariant(dynVarId, [&](uint32_t dyn_var_code) {
            if (st_var_code == ident.stVarCode && dyn_var_code == ident.dynVarCode)
              return;
            if (!out.empty())
              out.append(" | ");
            compile_human_readable_variant_name_single(out,
              VariantIdentifierRef{.shClassName = sclass.name.data(),
                .shDumpName = ident.shDumpName,
                .stVarCode = int(st_var_code),
                .dynVarCode = int(dyn_var_code)},
              dump, interval_storage_dump, settings);
          });
        });
      }
    }
  }
  truncate_variant_name(out, settings);
}

inline void read_parsing_settings_from_blk(VariantNameParsingSettings &out, const DataBlock &blk)
{
  out = {};
  out.allowNoDynamicVariant = blk.getBool("allowNoDynamicVariant", out.allowNoDynamicVariant);
}

inline void read_compilation_settings_from_blk(VariantNameCompilationSettings &out, const DataBlock &blk)
{
  out = {};
  out.intervalValuesAsNames = blk.getBool("intervalValuesAsNames", out.intervalValuesAsNames);
  out.omitDumpName = blk.getBool("omitDumpName", out.omitDumpName);
  out.listAllAliases = blk.getBool("listAllAliases", out.listAllAliases);
  out.minimalNamesOnly = blk.getBool("minimalNamesOnly", out.minimalNamesOnly);

  int maxNameLenFromBlk = blk.getInt("maxNameLen", 0);
  if (maxNameLenFromBlk > 0)
    out.maxNameLen = size_t(maxNameLenFromBlk);
}

} // namespace shader_name_format
