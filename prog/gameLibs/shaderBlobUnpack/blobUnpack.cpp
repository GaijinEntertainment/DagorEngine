// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaderBlobUnpack/shaderBlobUnpack.h>

using shader_blob::Stage;

static bool set_error(eastl::string *out_error, const char *msg)
{
  if (out_error)
    *out_error = msg;
  return false;
}

static Stage stage_from_dxil(dxil::ShaderStage stage)
{
  switch (stage)
  {
    case dxil::ShaderStage::VERTEX: return Stage::VS;
    case dxil::ShaderStage::PIXEL: return Stage::PS;
    case dxil::ShaderStage::COMPUTE: return Stage::CS;
    case dxil::ShaderStage::GEOMETRY: return Stage::GS;
    case dxil::ShaderStage::HULL: return Stage::HS;
    case dxil::ShaderStage::DOMAIN: return Stage::DS;
    case dxil::ShaderStage::MESH: return Stage::MS;
    case dxil::ShaderStage::AMPLIFICATION: return Stage::AS;
    default: return Stage::INVALID;
  }
}

static Stage stage_from_vk(VkShaderStageFlagBits stage)
{
  switch (stage)
  {
    case VK_SHADER_STAGE_VERTEX_BIT: return Stage::VS;
    case VK_SHADER_STAGE_FRAGMENT_BIT: return Stage::PS;
    case VK_SHADER_STAGE_COMPUTE_BIT: return Stage::CS;
    case VK_SHADER_STAGE_GEOMETRY_BIT: return Stage::GS;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return Stage::HS;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return Stage::DS;
    default: return Stage::INVALID;
  }
}

static bool range_fits(dag::ConstSpan<uint8_t> bytecode, uint32_t offset, uint32_t size)
{
  return offset <= bytecode.size() && size <= bytecode.size() - offset;
}

const char *shader_blob::api_name(Api stage)
{
  switch (stage)
  {
    case Api::DX11: return "dx11";
    case Api::DX12: return "dx12";
    case Api::SPIRV: return "vulkan";
    default: return "invalid";
  }
}

const char *shader_blob::stage_name(Stage stage)
{
  switch (stage)
  {
    case Stage::VS: return "vs";
    case Stage::PS: return "ps";
    case Stage::CS: return "cs";
    case Stage::GS: return "gs";
    case Stage::HS: return "hs";
    case Stage::DS: return "ds";
    case Stage::MS: return "ms";
    case Stage::AS: return "as";
    default: return "invalid";
  }
}

const char *shader_blob::bytecode_ext(Api api)
{
  switch (api)
  {
    case Api::DX11: return "dxbc";
    case Api::DX12: return "dxil";
    case Api::SPIRV: return "spv";
  }
  return "bin";
}

dag::Expected<shader_blob::Api, eastl::string> shader_blob::detect_api(dag::ConstSpan<uint8_t> metadata)
{
  const uint32_t *dwords = (const uint32_t *)metadata.data();
  if (metadata.size() >= sizeof(uint32_t))
  {
    if (dwords[0] == spirv::SPIR_V_BLOB_IDENT || dwords[0] == spirv::SPIR_V_BLOB_IDENT_UNCOMPRESSED ||
        dwords[0] == spirv::SPIR_V_COMBINED_BLOB_IDENT)
      return Api::SPIRV;
  }
  if (metadata.size() == dx11::SIMPLE_METADATA_SIZE ||
      (metadata.size() == dx11::COMBINED_METADATA_SIZE && dwords[dx11::COMBINED_HEADER_OFFSET / 4] == dx11::COMBINED_SHADERS_IDENT))
    return Api::DX11;
  if (auto *container = bindump::map<dxil::ShaderContainer>(metadata.data()))
    return Api::DX12;
  return dag::Unexpected<eastl::string>{"unsupported metadata format"};
}

static bool unpack_dx11(dag::ConstSpan<uint8_t> metadata, dag::ConstSpan<uint8_t> bytecode, Stage main_stage,
  shader_blob::UnpackedShader &out, eastl::string *out_error);
static bool unpack_dxil(dag::ConstSpan<uint8_t> metadata, dag::ConstSpan<uint8_t> bytecode, shader_blob::UnpackedShader &out,
  eastl::string *out_error);
static bool unpack_spirv(dag::ConstSpan<uint8_t> metadata, dag::ConstSpan<uint8_t> bytecode, Stage main_stage,
  shader_blob::UnpackedShader &out, eastl::string *out_error);

bool shader_blob::unpack(Api api, dag::ConstSpan<uint8_t> metadata, dag::ConstSpan<uint8_t> bytecode, Stage main_stage,
  UnpackedShader &out, eastl::string *out_error)
{
  out.api = api;
  out.stages.clear();
  out.spirvCodeStorage.clear();
  out.spirvChunkSets.clear();
  out.spirvChunkRefs.clear();
  out.spirvHeaders.clear();
  out.dxilHeaders.clear();

  switch (api)
  {
    case Api::DX11: return unpack_dx11(metadata, bytecode, main_stage, out, out_error);
    case Api::DX12: return unpack_dxil(metadata, bytecode, out, out_error);
    case Api::SPIRV: return unpack_spirv(metadata, bytecode, main_stage, out, out_error);
  }
  return false;
}

bool shader_blob::unpack(Api api, const ShaderSource &src, Stage main_stage, UnpackedShader &out, eastl::string *out_error)
{
  if (src.compressedData.empty())
    return set_error(out_error, "shader source has no bytecode");
  src.uncompress(out.bytecodeStorage);
  return unpack(api, src.metadata, make_span_const(out.bytecodeStorage.data(), src.uncompressedSize), main_stage, out, out_error);
}

static bool unpack_dx11(dag::ConstSpan<uint8_t> metadata, dag::ConstSpan<uint8_t> bytecode, Stage main_stage,
  shader_blob::UnpackedShader &out, eastl::string *out_error)
{
  dx11::DecodedShader decoded;
  if (!dx11::decode_metadata(metadata, decoded))
    return set_error(out_error, "malformed dx11 shader metadata");

  out.dx11Header = {
    decoded.totalBytecodeSize, decoded.maxConstantRegUsed, int8_t(decoded.maxRtvUsed), decoded.uavReadMask, decoded.uavWriteMask, 0};

  const eastl::pair<Stage, dx11::StageBytecodeRange> ranges[] = {
    {decoded.combined ? Stage::VS : main_stage, decoded.vs},
    {Stage::HS, decoded.hs},
    {Stage::DS, decoded.ds},
    {Stage::GS, decoded.gs},
  };
  for (auto &[stage, range] : ranges)
  {
    if (!range.size)
      continue;
    if (!range_fits(bytecode, range.offset, range.size))
      return set_error(out_error, "dx11 stage bytecode range exceeds bytecode size");
    shader_blob::StageBlob &blob = out.stages.push_back();
    blob.stage = stage;
    blob.bytecode = make_span_const(bytecode.data() + range.offset, range.size);
    blob.dx11Header = &out.dx11Header;
  }
  return !out.stages.empty() || set_error(out_error, "dx11 shader blob has no bytecode");
}

static bool unpack_dxil(dag::ConstSpan<uint8_t> metadata, dag::ConstSpan<uint8_t> bytecode, shader_blob::UnpackedShader &out,
  eastl::string *out_error)
{
  dxil::DecodedShaderRef decoded;
  if (!dxil::decode_metadata(metadata, true, decoded, out_error))
    return false;

  const dxil::StageModuleRef *modules[] = {&decoded.main, &decoded.hs, &decoded.ds, &decoded.gsOrAs};
  for (const dxil::StageModuleRef *module : modules)
  {
    if (!module->bytecodeSize)
      continue;
    if (!range_fits(bytecode, module->bytecodeOffset, module->bytecodeSize))
      return set_error(out_error, "dxil stage bytecode range exceeds bytecode size");
    out.dxilHeaders.push_back(module->header);
    shader_blob::StageBlob &blob = out.stages.push_back();
    blob.stage = stage_from_dxil(static_cast<dxil::ShaderStage>(module->header.shaderType));
    blob.bytecode = make_span_const(bytecode.data() + module->bytecodeOffset, module->bytecodeSize);
    blob.dxilHeader = &out.dxilHeaders.back();
  }
  return true;
}

static bool unpack_spirv(dag::ConstSpan<uint8_t> metadata, dag::ConstSpan<uint8_t> bytecode, Stage main_stage,
  shader_blob::UnpackedShader &out, eastl::string *out_error)
{
  spirv::CombinedStageList stages;
  if (!spirv::decode_container(metadata, VK_SHADER_STAGE_VERTEX_BIT, stages, out_error))
    return false;
  const bool combined = spirv::is_combined(metadata);

  for (const spirv::CombinedStageRef &stageRef : stages)
  {
    auto &chunkSet = out.spirvChunkSets.push_back();
    if (!spirv::decode_chunked_metadata(stageRef.metadata, chunkSet.chunks, chunkSet.chunkData, out_error))
      return false;
    out.spirvChunkRefs.push_back({make_span_const(chunkSet.chunks), make_span_const(chunkSet.chunkData)});
    const spirv::ChunkSetRef &chunks = out.spirvChunkRefs.back();

    eastl::string headerError;
    auto header = spirv::extract_header(chunks, 0, &headerError);
    if (!header)
    {
      if (out_error)
        *out_error = headerError.empty() ? eastl::string("spirv blob has no shader header chunk") : headerError;
      return false;
    }

    dag::ConstSpan<uint8_t> stageCode = bytecode;
    if (combined)
    {
      if (!range_fits(bytecode, stageRef.bytecodeOffset, stageRef.bytecodeSize))
        return set_error(out_error, "spirv stage bytecode range exceeds bytecode size");
      stageCode = make_span_const(bytecode.data() + stageRef.bytecodeOffset, stageRef.bytecodeSize);
    }

    out.spirvHeaders.push_back(header->header);
    shader_blob::StageBlob &blob = out.stages.push_back();
    blob.stage = combined ? stage_from_vk(stageRef.stage) : main_stage;
    blob.spirvHeader = &out.spirvHeaders.back();
    if (header->header.smolvSize)
    {
      Tab<uint8_t> &spirvCode = out.spirvCodeStorage.push_back();
      if (!spirv::decode_smolv(stageCode, header->header.smolvSize, spirvCode))
        return set_error(out_error, "SMOL-V decode failed");
      blob.bytecode = make_span_const(spirvCode.data(), spirvCode.size());
    }
    else
      blob.bytecode = stageCode;
  }
  return true;
}
