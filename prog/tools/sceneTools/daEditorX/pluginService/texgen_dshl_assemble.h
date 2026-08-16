// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/hash_map.h>

class DataBlock;
class String;

// Assembles a per-graph .dshl (one graphics shader{} per DISTINCT node-shader variant) from the
// compiled main-graph BLK plus the shader-source BLKs the texgen service already holds. This is the
// dshl-pipeline replacement for texturePSGenShader's runtime HLSL concatenation: the per-invocation
// [[substitutions]] + shader_postcode are resolved here, at graph-compile time, into named variants.
//
// A variant body mirrors texturePSGenShader::linkShader exactly:
//   shaderPreMain + "\n" + premain + "\n" + shaderCode + shaderPostCode + "\n" + postmain
// then replaceAll [[name]] from the source block's `substitutions{}`, then blank the particle-field
// markers (non-particle path). The body is wrapped in a self-contained, include-free dshl graphics
// shader (hand-written VS from default_texgen.blk `vprog`, the body as the PS) so dsc2 builds it into
// an additional bindump. Self-contained matters: pulling engine dshl includes drags in mutable globals
// the non-primary dump rejects. The node param `cbuffer global{}` is pinned to register(b3): dsc2's
// implicit const buffer takes b0 and a set_program-bound element ignores set_ps_const(0), so params
// are delivered via set_const_buffer at b3 (see assemble_body and PSGenShader::process).
//
// Variant identity = the assembled body text (== the sha1 key texturePSGenShader caches on today), so
// param-only edits (which never change the body) never add a variant and never trigger recompile.

struct TexgenDshlSkeleton
{
  // From default_texgen.blk (the texgen service already loads it).
  eastl::string vprog;          // fullscreen-quad VS, defines main_vs + VsOutput
  eastl::string particlesVprog; // instanced particles VS, defines main_vs + VsOutput (reads StructuredBuffer t0)
  eastl::string premain;  // shared HLSL prologue; opens `MRTOutput main_ps(...) {`; carries [[structures_decl]]/[[shader_function]]
                          // markers
  eastl::string postmain; // closes main_ps: `; return result; }`
};

struct TexgenDshlResult
{
  eastl::string dshlText;                                        // the full .dshl to feed dsc2
  eastl::hash_map<eastl::string, eastl::string> nodeToVariant;   // mainGraph node-block name -> variant shader name
  eastl::hash_map<eastl::string, eastl::string> variantToSource; // variant shader name -> original source shader name (for
                                                                 // params/regs)
  eastl::hash_map<eastl::string, eastl::string> sourceToVariant; // every source shader name -> variant (so C++ orchestrators'
                                                                 // texgen_get_shader(name) resolve)
  uint32_t bodyHash = 0;                                         // crc32 of all variant bodies; gate dsc2 recompile on this
  int variantCount = 0;
};

// `main_graph_blk` is the compiled mainGraphBlk (its _node_* blocks carry shader:t + params{}).
// `shader_sources` is a name-keyed lookup: block-name == shader name, each holding shaderPreMain /
//   shaderCode / params{shader_postcode} / substitutions{}. The service builds it by merging
//   default_shaders.blk and the expanded shaderListBlk (same inputs it feeds add_pixel_shader_texgen).
// Returns false (and sets out_err) only on an internal inconsistency. A node whose shader has no PS
// source is left untouched (shader:t unchanged) so the C++ orchestrator resolves it; particle-output
// nodes are reported via out_err (particle-field substitution is not yet wired).
bool assemble_texgen_dshl(const DataBlock &main_graph_blk, const DataBlock &shader_sources, const TexgenDshlSkeleton &skeleton,
  TexgenDshlResult &out_result, String &out_err);
