// Copyright (C) Gaijin Games KFT.  All rights reserved.

// shader mobules/programs implementation
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_platform.h>
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_memIo.h>
#include "globals.h"
#include "shader/program_database.h"
#include "device_context.h"

using namespace drv3d_vulkan;

static void decode_shader_binary(const uint32_t *native_code, uint32_t, VkShaderStageFlags, Tab<spirv::ChunkHeader> &chunks,
  Tab<uint8_t> &chunk_data)
{
  if (spirv::SPIR_V_BLOB_IDENT == native_code[0])
  {
    InPlaceMemLoadCB crd(native_code + 2, native_code[1]);
    ZlibLoadCB zlib_crd(crd, native_code[1]);
    zlib_crd.readTab(chunks);
    zlib_crd.readTab(chunk_data);
    zlib_crd.ceaseReading();
  }
  else if (spirv::SPIR_V_BLOB_IDENT_UNCOMPRESSED == native_code[0])
  {
    InPlaceMemLoadCB crd(native_code + 2, native_code[1]);
    crd.readTab(chunks);
    crd.readTab(chunk_data);
    crd.ceaseReading();
  }
  else
    DAG_FATAL("vulkan: unknown shader binary ident %llX", native_code[0]);
}

static int create_shader_for_stage(const uint32_t *native_code, VkShaderStageFlagBits stage, uintptr_t size)
{
  if (spirv::SPIR_V_COMBINED_BLOB_IDENT == native_code[0])
  {
    uint32_t count = native_code[1];
    auto comboHeaderChunks = reinterpret_cast<const spirv::CombinedChunk *>(native_code + 2);
    auto comboData = reinterpret_cast<const uint8_t *>(comboHeaderChunks + count);

    dag::Vector<VkShaderStageFlagBits> comboStages;
    dag::Vector<Tab<spirv::ChunkHeader>> comboChunks;
    dag::Vector<Tab<uint8_t>> comboChunkData;
    comboStages.reserve(count);
    comboChunks.reserve(count);
    comboChunkData.reserve(count);

    for (auto &&chunk : make_span(comboHeaderChunks, count))
    {
      Tab<spirv::ChunkHeader> chunks;
      Tab<uint8_t> chunkData;
      decode_shader_binary(reinterpret_cast<const uint32_t *>(comboData + chunk.offset), chunk.size, chunk.stage, chunks, chunkData);
      comboStages.push_back(chunk.stage);
      comboChunks.push_back(eastl::move(chunks));
      comboChunkData.push_back(eastl::move(chunkData));
    }
    return Globals::shaderProgramDatabase
      .newShader(Globals::ctx, eastl::move(comboStages), eastl::move(comboChunks), eastl::move(comboChunkData))
      .get();
  }
  else
  {
    Tab<spirv::ChunkHeader> chunks;
    Tab<uint8_t> chunkData;
    decode_shader_binary(native_code, size, stage, chunks, chunkData);
    return Globals::shaderProgramDatabase.newShader(Globals::ctx, stage, chunks, chunkData).get();
  }
}

VPROG d3d::create_vertex_shader(const uint32_t *native_code)
{
  return create_shader_for_stage(native_code, VK_SHADER_STAGE_VERTEX_BIT, 0);
}

void d3d::delete_vertex_shader(VPROG vs) { Globals::shaderProgramDatabase.deleteShader(Globals::ctx, ShaderID(vs)); }


FSHADER d3d::create_pixel_shader(const uint32_t *native_code)
{
  return create_shader_for_stage(native_code, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
}

void d3d::delete_pixel_shader(FSHADER ps) { Globals::shaderProgramDatabase.deleteShader(Globals::ctx, ShaderID(ps)); }

PROGRAM d3d::get_debug_program() { return Globals::shaderProgramDatabase.getDebugProgram().get(); }

PROGRAM d3d::create_program(VPROG vs, FSHADER fs, VDECL vdecl, unsigned *, unsigned)
{
  return Globals::shaderProgramDatabase.newGraphicsProgram(Globals::ctx, InputLayoutID(vdecl), ShaderID(vs), ShaderID(fs)).get();
}

PROGRAM d3d::create_program(const uint32_t *vs, const uint32_t *ps, VDECL vdecl, unsigned *strides, unsigned streams)
{
  VPROG vprog = create_vertex_shader(vs);
  FSHADER fshad = create_pixel_shader(ps);
  return create_program(vprog, fshad, vdecl, strides, streams);
}

PROGRAM d3d::create_program_cs(const uint32_t *cs_native, CSPreloaded)
{
  Tab<spirv::ChunkHeader> chunks;
  Tab<uint8_t> chunkData;
  decode_shader_binary(cs_native, 0, VK_SHADER_STAGE_COMPUTE_BIT, chunks, chunkData);

  auto smh = spirv_extractor::getHeader(VK_SHADER_STAGE_COMPUTE_BIT, chunks, chunkData, 0);
  if (!smh)
    DAG_FATAL("Shader has no header");

  auto smb = spirv_extractor::getBlob(chunks, chunkData, 0);
  if (smb.blob.empty())
    DAG_FATAL("Shader has no byte code blob");

  PROGRAM prog = Globals::shaderProgramDatabase.newComputeProgram(Globals::ctx, *smh, smb).get();

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  auto dbg = spirv_extractor::getDebugInfo(chunks, chunkData, 0);
  Globals::ctx.attachComputeProgramDebugInfo(ProgramID(prog), eastl::make_unique<ShaderDebugInfo>(dbg));
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
