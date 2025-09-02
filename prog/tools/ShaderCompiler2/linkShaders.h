// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderSave.h"
#include "shTargetContext.h"
#include "intervals.h"
#include "globVar.h"
#include "samplers.h"
#include <util/dag_simpleString.h>
#include <util/dag_bindump_ext.h>
#include "shaderTab.h"
#include "shSemCode.h"
#include "cppStcodeAssembly.h"
#include <shaders/shader_layout.h>

class ShaderClass;
class IGenSave;

namespace shaders
{
struct RenderState;
}

struct ShadersBindump
{
  SerializableTab<ShaderGlobal::Var> variable_list;
  SerializableTab<Sampler> static_samplers;
  IntervalList intervals;
  bindump::Ptr<ShaderStateBlock> empty_block;
  SerializableTab<ShaderStateBlock *> state_blocks;
  SerializableTab<ShaderClass *> shaderClasses;
  SerializableTab<shaders::RenderState> renderStates;
  SerializableTab<TabFsh> shadersFsh;
  SerializableTab<TabVpr> shadersVpr;
  SerializableTab<TabStcode> shadersStcode;
  SerializableTab<CryptoHash> dynamicCppcodeHashes;
  SerializableTab<CryptoHash> staticCppcodeHashes;
  SerializableTab<bindump::string> dynamicCppcodeRoutineNames;
  SerializableTab<bindump::string> staticCppcodeRoutineNames;
  SerializableTab<SerializableTab<int32_t>> cppcodeRegisterTables;
  SerializableTab<int32_t> cppcodeRegisterTableOffsets;

  // Only used if shc::config().generateCppStcodeValidationData
  SerializableTab<shader_layout::StcodeConstValidationMask *> stcodeConstMasks{};
};

struct ShadersBindumpHeader
{
  int cache_sign = -1;
  int cache_version = -1;
  BlkHashBytes last_blk_hash = {};
  bindump::EnableHash<ShadersBindump> hash;
  bindump::vector<bindump::string> dependency_files;
};

struct CompressedShadersBindump : ShadersBindumpHeader
{
  uint64_t decompressed_size = 0;
  bindump::vector<uint8_t> compressed_shaders;
  int eof = 0; // 0 means invalid, need to initialize by hand
};

void close_shader_class();

void add_shader_class(ShaderClass *sc, shc::TargetContext &ctx);

int add_fshader(dag::ConstSpan<unsigned> code, shc::TargetContext &ctx);
int add_vprog(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs, dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs,
  shc::TargetContext &ctx);
int add_render_state(const SemanticShaderPass &state, shc::TargetContext &ctx);

int add_stcode(dag::ConstSpan<int> code, shc::TargetContext &ctx);

void add_stcode_validation_mask(int stcode_id, shader_layout::StcodeConstValidationMask *mask, shc::TargetContext &ctx);

void count_shader_stats(unsigned &uniqueFshBytesInFile, unsigned &uniqueFshCountInFile, unsigned &uniqueVprBytesInFile,
  unsigned &uniqueVprCountInFile, unsigned &stcodeBytes, const shc::TargetContext &ctx);

bool load_shaders_bindump(ShadersBindump &shaders, bindump::IReader &full_file_reader, shc::TargetContext &ctx);
bool link_scripted_shaders(const uint8_t *mapped_data, int data_size, const char *filename, const char *source_name,
  shc::TargetContext &ctx);
void save_scripted_shaders(const char *filename, dag::ConstSpan<SimpleString> files, shc::TargetContext &ctx,
  bool need_cppstcode_file_write = true);

/**
 * @brief Update the timestamps of the shader classes
 * @param dependencies The list of files that the shader classes depend on
 * @note If dagor git repo is present and files are not locally git modified, the function will use the timestamp of the last commit
 * for file dependencies. Otherwise, it will use the filesystem timestamp
 */
void update_shaders_timestamps(dag::ConstSpan<SimpleString> dependencies, shc::TargetContext &ctx);

#if _CROSS_TARGET_DX12
struct VertexProgramAndPixelShaderIdents
{
  int vprog;
  int fsh;
};
VertexProgramAndPixelShaderIdents add_phase_one_progs(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, dag::ConstSpan<unsigned> ps,
  SemanticShaderPass::EnableFp16State enableFp16, shc::TargetContext &ctx);
void recompile_shaders(shc::TargetContext &ctx);
#endif
