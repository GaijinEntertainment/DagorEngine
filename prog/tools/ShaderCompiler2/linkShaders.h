// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderSave.h"
#include "intervals.h"
#include "globVar.h"
#include "samplers.h"
#include <util/dag_simpleString.h>
#include <util/dag_bindump_ext.h>
#include "shaderTab.h"
#include "shSemCode.h"
#include "cppStcodeAssembly.h"

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
  SerializableTab<ShaderClass *> shader_classes;
  SerializableTab<shaders::RenderState> render_states;
  SerializableTab<TabFsh> shaders_fsh;
  SerializableTab<TabVpr> shaders_vpr;
  SerializableTab<TabStcode> shaders_stcode;
};

struct ShadersBindumpHeader
{
  int cache_sign = -1;
  int cache_version = -1;
  bindump::EnableHash<ShadersBindump> hash;
  bindump::vector<bindump::string> dependency_files;
};

struct CompressedShadersBindump : ShadersBindumpHeader
{
  uint64_t decompressed_size = 0;
  bindump::vector<uint8_t> compressed_shaders;
  int eof = 0; // 0 means invalid, need to initialize by hand
};

void init_shader_class();
void close_shader_class();

void add_shader_class(ShaderClass *sc);

int add_fshader(dag::ConstSpan<unsigned> code);
int add_vprog(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs, dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs);
int add_render_state(const ShaderSemCode::Pass &state);

struct StcodeAddResult
{
  int id;
  bool isNew;
};

StcodeAddResult add_stcode(dag::ConstSpan<int> code);

void count_shader_stats(unsigned &uniqueFshBytesInFile, unsigned &uniqueFshCountInFile, unsigned &uniqueVprBytesInFile,
  unsigned &uniqueVprCountInFile, unsigned &stcodeBytes);

bool load_shaders_bindump(ShadersBindump &shaders, bindump::IReader &full_file_reader);
bool link_scripted_shaders(const uint8_t *mapped_data, int data_size, const char *filename, const char *source_name,
  StcodeInterface &stcode_interface);
void save_scripted_shaders(const char *filename, dag::ConstSpan<SimpleString> files);

/*
 * @brief Update the timestamps of the shader classes
 * @param dependencies The list of files that the shader classes depend on
 * @note If dagor git repo is present and files are not locally git modified, the function will use the timestamp of the last commit
 * for file dependencies. Otherwise, it will use the filesystem timestamp
 */
void update_shaders_timestamps(dag::ConstSpan<SimpleString> dependencies);

struct BindumpPackingFlagsBits
{
  static constexpr uint32_t NONE = 0;
  static constexpr uint32_t SHADER_GROUPS = 1;
  static constexpr uint32_t WHOLE_BINARY = 1 << 1;
};
using BindumpPackingFlags = uint32_t;

bool make_scripted_shaders_dump(const char *dump_name, const char *cache_filename, bool strip_shaders_and_stcode,
  BindumpPackingFlags packing_flags, StcodeInterface *stcode_interface);

#if _CROSS_TARGET_DX12
struct VertexProgramAndPixelShaderIdents
{
  int vprog;
  int fsh;
};
VertexProgramAndPixelShaderIdents add_phase_one_progs(dag::ConstSpan<unsigned> vs, dag::ConstSpan<unsigned> hs,
  dag::ConstSpan<unsigned> ds, dag::ConstSpan<unsigned> gs, dag::ConstSpan<unsigned> ps, bool enable_fp16);
void recompile_shaders();
#endif