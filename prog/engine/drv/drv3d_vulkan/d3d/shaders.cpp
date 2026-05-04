// Copyright (C) Gaijin Games KFT.  All rights reserved.

// shader mobules/programs implementation
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_platform.h>
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_memIo.h>
#include <memory/dag_framemem.h>
#include "globals.h"
#include "shader/program_database.h"
#include "device_context.h"
#include "backend/cmd/debug.h"

using namespace drv3d_vulkan;

static void decode_shader_binary(const uint32_t *metadata, uint32_t size, VkShaderStageFlags, Tab<spirv::ChunkHeader> &chunks,
  Tab<uint8_t> &chunk_data)
{
  G_UNUSED(size);
  G_ASSERT(metadata[1] + 2 <= size);
  if (spirv::SPIR_V_BLOB_IDENT == metadata[0])
  {
    InPlaceMemLoadCB crd(metadata + 2, metadata[1]);
    ZlibLoadCB zlibCrd(crd, metadata[1]);
    zlibCrd.readTab(chunks);
    zlibCrd.readTab(chunk_data);
    zlibCrd.ceaseReading();
  }
  else if (spirv::SPIR_V_BLOB_IDENT_UNCOMPRESSED == metadata[0])
  {
    InPlaceMemLoadCB crd(metadata + 2, metadata[1]);
    crd.readTab(chunks);
    crd.readTab(chunk_data);
    crd.ceaseReading();
  }
  else
    DAG_FATAL("vulkan: unknown shader binary ident %c%c%c%c", DUMP4C(metadata[0]));
}

static int create_shader_for_stage(const uint32_t *metadata, const ShaderSource &source, VkShaderStageFlagBits stage, uintptr_t size)
{
  if (spirv::SPIR_V_COMBINED_BLOB_IDENT == metadata[0])
  {
    uint32_t count = metadata[1];
    auto comboHeaderChunks = reinterpret_cast<const spirv::CombinedChunk *>(metadata + 2);
    auto comboData = reinterpret_cast<const uint8_t *>(comboHeaderChunks + count);

    dag::Vector<VkShaderStageFlagBits> comboStages;
    dag::Vector<Tab<spirv::ChunkHeader>> comboChunks;
    dag::Vector<Tab<uint8_t>> comboChunkData;
    dag::Vector<ShaderProgramData> comboBytecode;
    comboStages.reserve(count);
    comboChunks.reserve(count);
    comboChunkData.reserve(count);

    uint32_t bytecodeOffset = 0;
    for (auto &&chunk : make_span(comboHeaderChunks, count))
    {
      Tab<spirv::ChunkHeader> chunks;
      Tab<uint8_t> chunkData;
      decode_shader_binary(reinterpret_cast<const uint32_t *>(comboData), chunk.size, chunk.stage, chunks, chunkData);
      comboStages.push_back(chunk.stage);
      comboChunks.push_back(eastl::move(chunks));
      comboChunkData.push_back(eastl::move(chunkData));
      comboBytecode.emplace_back(bytecodeOffset, chunk.bytecode_size);
      comboData += chunk.size;
      bytecodeOffset += chunk.bytecode_size;
    }
    return Globals::shaderProgramDatabase
      .newShader(Globals::ctx, eastl::move(comboStages), eastl::move(comboChunks), eastl::move(comboChunkData),
        eastl::move(comboBytecode), source)
      .get();
  }
  else
  {
    Tab<spirv::ChunkHeader> chunks;
    Tab<uint8_t> chunkData;
    decode_shader_binary(metadata, size, stage, chunks, chunkData);
    return Globals::shaderProgramDatabase.newShader(Globals::ctx, stage, chunks, chunkData, source).get();
  }
}

VPROG d3d::create_vertex_shader(const ShaderSource &data)
{
  return create_shader_for_stage((const uint32_t *)data.metadata.data(), data, VK_SHADER_STAGE_VERTEX_BIT, data.metadata.size());
}

void d3d::delete_vertex_shader(VPROG vs) { Globals::shaderProgramDatabase.deleteShader(Globals::ctx, ShaderID(vs)); }


FSHADER d3d::create_pixel_shader(const ShaderSource &data)
{
  return create_shader_for_stage((const uint32_t *)data.metadata.data(), data, VK_SHADER_STAGE_FRAGMENT_BIT, data.metadata.size());
}

void d3d::delete_pixel_shader(FSHADER ps) { Globals::shaderProgramDatabase.deleteShader(Globals::ctx, ShaderID(ps)); }

PROGRAM d3d::get_debug_program() { return Globals::shaderProgramDatabase.getDebugProgram().get(); }

PROGRAM d3d::create_program(VPROG vs, FSHADER fs, VDECL vdecl, unsigned *, unsigned)
{
  return Globals::shaderProgramDatabase.newGraphicsProgram(Globals::ctx, InputLayoutID(vdecl), ShaderID(vs), ShaderID(fs)).get();
}

PROGRAM d3d::create_program_cs(const ShaderSource &data, CSPreloaded)
{
  Tab<spirv::ChunkHeader> chunks;
  Tab<uint8_t> chunkData;
  decode_shader_binary((const uint32_t *)data.metadata.data(), data.metadata.size(), VK_SHADER_STAGE_COMPUTE_BIT, chunks, chunkData);

  auto smh = spirv_extractor::getHeader(VK_SHADER_STAGE_COMPUTE_BIT, chunks, chunkData, 0);
  if (!smh)
    DAG_FATAL("Shader has no header");

  auto smb = spirv_extractor::getBlob(*smh, data, {0, 0}, chunks, chunkData, 0);
  if (smb.source.compressedData.empty())
    DAG_FATAL("Shader has no byte code blob");

  PROGRAM prog = Globals::shaderProgramDatabase.newComputeProgram(Globals::ctx, *smh, smb).get();

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  auto dbg = spirv_extractor::getDebugInfo(chunks, chunkData, 0);
  Globals::ctx.dispatchCmd<CmdAttachComputeProgramDebugInfo>({ProgramID(prog), eastl::make_unique<ShaderDebugInfo>(dbg).release()});
#endif

  return prog;
}

void d3d::delete_program(PROGRAM prog)
{
  ProgramID pid{prog};
  Globals::shaderProgramDatabase.removeProgram(Globals::ctx, pid);
}

#if _TARGET_PC
VPROG d3d::create_vertex_shader_dagor(const VPRTYPE * /*tokens*/, int /*len*/)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

VPROG d3d::create_vertex_shader_asm(const char * /*asm_text*/)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

FSHADER d3d::create_pixel_shader_dagor(const FSHTYPE * /*tokens*/, int /*len*/)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

FSHADER d3d::create_pixel_shader_asm(const char * /*asm_text*/)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

// NOTE: entry point should be removed from the interface
bool d3d::set_pixel_shader(FSHADER /*shader*/)
{
  G_ASSERT(false);
  return true;
}

// NOTE: entry point should be removed from the interface
bool d3d::set_vertex_shader(VPROG /*shader*/)
{
  G_ASSERT(false);
  return true;
}

VDECL d3d::get_program_vdecl(PROGRAM prog) { return Globals::shaderProgramDatabase.getGraphicsProgInputLayout(ProgramID(prog)).get(); }
#endif // _TARGET_PC
