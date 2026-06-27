// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Editor-side runtime HLSL->SPIR-V compiler for daEditorX texgen on Linux/Vulkan. The engine and the
// shipped game never compile HLSL at runtime on Vulkan; the editor does, for graph texture generation.
// So this TU links gameLibs/spirv (DXC) and installs itself into texgen's texgen_compile_hlsl_shader
// hook: it compiles a node's HLSL to SPIR-V, packs a ShaderSource blob in the layout
// decode_shader_binary expects, and builds the shader through the Vulkan driver.

#include <textureGen/textureGenHlslCompiler.h>
#include <spirv/compiler.h>
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_consts.h>
#include <drvCommonConsts.h> // MAX_PS_CONSTS
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_genIo.h>
#include <memory/dag_memBase.h> // tmpmem
#include <generic/dag_tab.h>
#include <generic/dag_span.h>
#include <util/dag_string.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <string.h>

namespace
{
// DXC emits SPIR-V (not DXIL/DXBC like the Windows paths), and mishandles `half` arrays without 16-bit
// types enabled, so force half->float and define the vertex-id helper to compile the same node HLSL.
// compileHLSL_DXC bumps the shader model to >= 6 internally.
const char DXC_PROLOGUE[] = "#define SHADER_COMPILER_DXC 1\n"
                            "#define HW_VERTEX_ID uint vertexId : SV_VertexID;\n"
                            "#define half float\n"
                            "#define half1 float1\n"
                            "#define half2 float2\n"
                            "#define half3 float3\n"
                            "#define half4 float4\n";

// Texgen runs its compiles on a single worker thread, so the cached context needs no locking.
spirv::DXCContext *get_dxc()
{
  static spirv::DXCContext *ctx = nullptr;
  static bool tried = false;
  if (!tried)
  {
    tried = true;
    dag::Vector<String> params;
    ctx = spirv::setupDXC("", params);
  }
  return ctx;
}

// ShaderSource keeps non-owning spans that the Vulkan driver only reads later, at pipeline build, so
// the metadata blob and the SPIR-V bytes must outlive the shader. Texgen shaders live for the editor
// session and are sha1-cached, so retaining each distinct compile for the process lifetime is bounded.
// unique_ptr keeps each pointee fixed when the vector grows, so the spans taken from it below stay valid.
struct RetainedBlob
{
  Tab<uint8_t> meta;
  Tab<uint8_t> code;
};
eastl::vector<eastl::unique_ptr<RetainedBlob>> retained;

int compile_texgen_hlsl(const char *hlsl, int len, const char *entry, const char *profile, bool is_vertex, String &out_err)
{
  const int bad = is_vertex ? BAD_VPROG : BAD_FSHADER;

  spirv::DXCContext *ctx = get_dxc();
  if (!ctx)
  {
    out_err = "DXC unavailable (is libdxcompiler.so next to the editor?)";
    return bad;
  }

  eastl::string src;
  src.reserve(sizeof(DXC_PROLOGUE) + len);
  src.append(DXC_PROLOGUE);
  src.append(hlsl, static_cast<size_t>(len));

  // USE_SCALAR_LAYOUT: texgen's structured buffers are DX-packed (e.g. ParticleInstance), which strict
  // Vulkan 1.0 std430 rejects; the editor's Vulkan device enables the matching scalarBlockLayout feature.
  spirv::CompileToSpirVResult res = spirv::compileHLSL_DXC(ctx, make_span_const(src.data(), static_cast<intptr_t>(src.length())),
    entry, profile, spirv::CompileFlags::USE_SCALAR_LAYOUT, {});
  if (res.byteCode.empty())
  {
    for (const eastl::string &m : res.infoLog)
      out_err.aprintf(0, "%s\n", m.c_str());
    return bad;
  }

  // compileHLSL_DXC fills only the reflection metadata; version, constant count and smolv size are ours.
  res.header.verMagic = spirv::HEADER_MAGIC_VER;
  // texgen binds raw c-registers, so over-allocate the whole range rather than reflect exact usage.
  res.header.maxConstantCount = MAX_PS_CONSTS;
  res.header.smolvSize = 0; // raw SPIR-V is kept in compressedData, never smolv-encoded here

  eastl::unique_ptr<RetainedBlob> blob(new RetainedBlob);

  // metadata = [ident][content-byte-size][writeTab(chunks)][writeTab(chunkData)] with a single
  // SHADER_HEADER chunk; the bytecode lives in ShaderSource.compressedData, not in a chunk.
  Tab<spirv::ChunkHeader> chunks;
  Tab<uint8_t> chunkData;
  spirv::add_chunk(chunks, chunkData, 0u, res.header);

  DynamicMemGeneralSaveCB cwr(tmpmem, 0, 4 << 10);
  cwr.writeInt(static_cast<int>(spirv::SPIR_V_BLOB_IDENT_UNCOMPRESSED));
  cwr.writeInt(0); // content byte size, backfilled below
  const int contentStart = cwr.tell();
  cwr.writeTab(chunks);
  cwr.writeTab(chunkData);
  const int contentEnd = cwr.tell();
  cwr.seekto(4);
  cwr.writeInt(contentEnd - contentStart);
  cwr.seektoend();

  blob->meta.resize(static_cast<int>(cwr.size()));
  memcpy(blob->meta.data(), cwr.data(), static_cast<size_t>(cwr.size()));

  const int codeBytes = static_cast<int>(res.byteCode.size() * sizeof(unsigned));
  blob->code.resize(codeBytes);
  memcpy(blob->code.data(), res.byteCode.data(), static_cast<size_t>(codeBytes));

  ShaderSource s;
  s.metadata = make_span_const(blob->meta.data(), blob->meta.size());
  s.compressedData = make_span_const(blob->code.data(), blob->code.size());
  s.uncompressedSize = static_cast<uint32_t>(blob->code.size());
  s.dictionary = nullptr;

  const int handle = is_vertex ? d3d::create_vertex_shader(s) : d3d::create_pixel_shader(s);
  if (handle == bad)
  {
    out_err.aprintf(0, "%s shader create failed", is_vertex ? "vertex" : "pixel");
    return bad;
  }

  retained.push_back(eastl::move(blob)); // outlive the shader (read lazily at pipeline build)
  return handle;
}

// Installed at startup. This TU must stay a direct editor Source (object-linked), not pulled from a
// static lib, so the dynamic initializer is kept and runs.
struct InstallHook
{
  InstallHook() { texgen_compile_hlsl_shader = &compile_texgen_hlsl; }
} install_hook;
} // namespace
