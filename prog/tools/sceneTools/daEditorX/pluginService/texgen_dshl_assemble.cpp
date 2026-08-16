// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texgen_dshl_assemble.h"

#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <hash/crc32.h>
#include <EASTL/string.h>
#include <EASTL/hash_set.h>
#include <EASTL/utility.h>
#include <string.h>

namespace
{
void replace_all(eastl::string &s, const char *from, const char *to)
{
  if (!from || !*from)
  {
    return;
  }
  const size_t fromLen = strlen(from);
  const size_t toLen = to ? strlen(to) : 0;
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != eastl::string::npos)
  {
    s.replace(pos, fromLen, to ? to : "", toLen);
    pos += toLen;
  }
}

// Mirrors texturePSGenShader::linkShader for the common (non-fullShaderCode) path:
//   shaderPreMain + "\n" + premain + "\n" + shaderCode + shaderPostCode + "\n" + postmain
// then [[name]] replaceAll from substitutions{}, then blank particle-field markers.
eastl::string assemble_body(const DataBlock &source, const DataBlock &node_params, const TexgenDshlSkeleton &skeleton)
{
  const char *fullShaderCode = source.getStr("fullShaderCode", nullptr);
  const char *shaderPreMain = source.getStr("shaderPreMain", "");
  const char *shaderCode = source.getStr("shaderCode", "");

  // shader_postcode: per-node-instance override (params_override in process()) wins over the source default.
  const char *srcPostCode = source.getBlockByNameEx("params")->getStr("shader_postcode", "");
  const char *shaderPostCode = node_params.getStr("shader_postcode", srcPostCode);

  eastl::string body;
  if (fullShaderCode)
  {
    body = fullShaderCode; // self-contained whole-PS override: premain/postmain are not applied (matches add_pixel_shader_texgen)
  }
  else
  {
    body.reserve(
      strlen(shaderPreMain) + skeleton.premain.size() + strlen(shaderCode) + strlen(shaderPostCode) + skeleton.postmain.size() + 8);
    body.append(shaderPreMain);
    body.append("\n");
    body.append(skeleton.premain.c_str());
    body.append("\n");
    body.append(shaderCode);
    body.append(shaderPostCode);
    body.append("\n");
    body.append(skeleton.postmain.c_str());
  }

  // [[name]] substitution: source substitutions{} first, then the node-instance override block (node wins).
  for (const DataBlock *substBlk : {source.getBlockByName("substitutions"), node_params.getBlockByName("substitutions")})
  {
    if (!substBlk)
    {
      continue;
    }
    for (int i = 0; i < substBlk->paramCount(); ++i)
    {
      if (substBlk->getParamType(i) != DataBlock::TYPE_STRING)
      {
        continue;
      }
      eastl::string marker;
      marker.sprintf("[[%s]]", substBlk->getParamName(i));
      replace_all(body, marker.c_str(), substBlk->getStr(i));
    }
  }

  // Fallback for non-consumer shaders: blank any particle-field markers left after the substitution
  // loop. Particle consumers (source_consumes_particles) fill these above from their own
  // substitutions{}; everything else leaves the markers empty so the PS uses the base VsOutput.
  replace_all(body, "[[particle_fields_with_semantics]]", "");
  replace_all(body, "[[particle_fields]]", "");
  replace_all(body, "[[particle_fields_ps_copy]]", "");

  // Node params live in `cbuffer global`. Pin it to b3 and feed it via set_const_buffer in
  // PSGenShader::process. b0/b1/b2 are reserved (dsc2 implicit const buf / material params / global const),
  // and the legacy set_ps_const(0)->b0 path is NOT honored for a dsc2 element bound with set_program only,
  // so an unregistered (b0) cbuffer would receive garbage -- e.g. blur_hq_step's `rays` loop count, which
  // then runs unbounded and hangs the GPU. First strip any hardcoded :register(b0) (a few builtins pin it,
  // and dsc2 rejects the clash), then pin the params cbuffer to the free b3 slot.
  replace_all(body, ":register(b0)", "");
  replace_all(body, ": register(b0)", "");
  replace_all(body, "cbuffer global", "cbuffer global:register(b3)");
  return body;
}

// A shader is a particle CONSUMER when its source declares particle interpolant fields
// (particle_fields_with_semantics in substitutions) -- it draws instanced particle quads and reads
// per-particle attributes, so it must be compiled with the instanced particles VS (which reads the
// StructuredBuffer at t0 and emits those interpolants). Scatter nodes (output fmt=particles) and
// everything else use the fullscreen VS: the scatter PS covers the screen and writes the RW instance
// buffer. This matches the runtime, where VS binding follows the input buffer (data.particles): a
// consumer has a particles{} input -> draw_indirect + particles VS; a scatter has none -> fullscreen.
bool source_consumes_particles(const DataBlock &source)
{
  const DataBlock *subst = source.getBlockByName("substitutions");
  return subst && subst->paramExists("particle_fields_with_semantics");
}
} // namespace

bool assemble_texgen_dshl(const DataBlock &main_graph_blk, const DataBlock &shader_sources, const TexgenDshlSkeleton &skeleton,
  TexgenDshlResult &out_result, String &out_err)
{
  out_result = TexgenDshlResult{};
  G_UNUSED(out_err); // reserved for assembly-level errors; kept in the signature as the caller's contract

  eastl::hash_map<eastl::string, eastl::string> bodyToVariant; // body text -> variant; dedups identical bodies
  eastl::hash_set<eastl::string> usedNames;
  eastl::string dshl;
  dshl.append("// AUTO-GENERATED by assemble_texgen_dshl -- one graphics shader{} per distinct texgen node-shader variant.\n");
  uint32_t combinedHash = 0;

  // Emit (once) a self-contained graphics shader{} for `body`, returning its variant name.
  // is_particles selects the instanced particles VS (reads StructuredBuffer at t0) vs the fullscreen VS.
  auto getOrCreateVariant = [&](const eastl::string &body, bool is_particles) -> eastl::string {
    // Dedup key includes the VS choice: two bodies could coincide while needing different VS.
    const eastl::string key = body + (is_particles ? "\x01P" : "\x01F");
    if (auto it = bodyToVariant.find(key); it != bodyToVariant.end())
    {
      return it->second;
    }
    const uint32_t h = calc_crc32(reinterpret_cast<const unsigned char *>(body.data()), body.size(), 0);
    combinedHash = calc_crc32(reinterpret_cast<const unsigned char *>(body.data()), body.size(), combinedHash);
    eastl::string variantName;
    variantName.sprintf("texgen_v_%08x", h);
    for (int dis = 1; usedNames.find(variantName) != usedNames.end(); ++dis) // crc32 collision guard
    {
      variantName.sprintf("texgen_v_%08x_%d", h, dis);
    }
    usedNames.insert(variantName);

    const eastl::string &vs = is_particles ? skeleton.particlesVprog : skeleton.vprog;
    dshl.append_sprintf("\nshader %s\n{\n", variantName.c_str());
    dshl.append("  no_ablend;\n  cull_mode = none;\n  z_write = false;\n  z_test = false;\n");
    // texgen renders these as standalone postfx-style passes and binds all constants/textures RAW;
    // they must depend on NO shader block, else ShaderElement::setStates tries to set the env's default
    // object block (gui_aces_object) in texgen's frame/scene-only context -> per-draw block LOGERR spam.
    dshl.append("  supports none;\n");
    dshl.append("  hlsl(vs) {\n");
    dshl.append(vs.c_str());
    dshl.append("\n  }\n  hlsl(ps) {\n");
    dshl.append(body.c_str());
    dshl.append("\n  }\n");
    dshl.append("  compile(\"target_vs\", \"main_vs\");\n");
    dshl.append("  compile(\"target_ps\", \"main_ps\");\n}\n");

    bodyToVariant.emplace(key, variantName);
    ++out_result.variantCount;
    return variantName;
  };

  // 1) Per main-graph node: a variant baked with that node's params/substitutions. shader:t is later
  //    rewritten to this variant name (applyDshlShaderRewrite).
  for (int i = 0; i < main_graph_blk.blockCount(); ++i)
  {
    const DataBlock *node = main_graph_blk.getBlock(i);
    if (!node)
    {
      continue;
    }
    const char *shaderName = node->getStr("shader", nullptr);
    if (!shaderName || !*shaderName)
    {
      continue; // not a shader node (structural / metadata block)
    }

    const DataBlock *source = shader_sources.getBlockByName(shaderName);
    if (!source)
    {
      // No PS source -> this node's shader is a C++-registered orchestrator (distance_field, blur_*,
      // erosion, cache, tex_autolevels, ...) or genuinely missing. Leave shader:t unchanged so the
      // scheduler resolves it to the C++ TextureGenShader; do NOT emit a dshl variant for it (a #error
      // stub would crash dsc2, and rewriting shader:t would hide the real orchestrator).
      continue;
    }

    const bool isParticles = source_consumes_particles(*source);

    const eastl::string body = assemble_body(*source, *node->getBlockByNameEx("params"), skeleton);
    const eastl::string v = getOrCreateVariant(body, isParticles);
    out_result.nodeToVariant.emplace(eastl::string(node->getBlockName()), v);
    out_result.variantToSource.emplace(v, eastl::string(shaderName));
  }

  // 2) Every shader source (default_shaders.blk builtins + the graph's shaderListBlk), registered under
  //    its ORIGINAL name, so the C++ orchestrators (distance_field/blur/...) resolve their REQUIRE'd
  //    sub-shaders (selectPoints, jumpFlooding, ...) via texgen_get_shader(name). Source defaults only;
  //    param values still arrive via the runtime constant buffer.
  static const DataBlock emptyParams;
  for (int i = 0; i < shader_sources.blockCount(); ++i)
  {
    const DataBlock *src = shader_sources.getBlock(i);
    if (!src || (!src->getStr("shaderCode", nullptr) && !src->getStr("fullShaderCode", nullptr)))
    {
      continue;
    }
    const char *name = src->getBlockName();
    if (out_result.sourceToVariant.find_as(name) != out_result.sourceToVariant.end())
    {
      continue;
    }
    const eastl::string v = getOrCreateVariant(assemble_body(*src, emptyParams, skeleton), source_consumes_particles(*src));
    out_result.sourceToVariant.emplace(eastl::string(name), v);
    out_result.variantToSource.emplace(v, eastl::string(name)); // no-op if v already mapped
  }

  out_result.dshlText = eastl::move(dshl);
  out_result.bodyHash = combinedHash;
  return true;
}
