#include "asmShaderHLSL2MetalDXC.h"

#include <algorithm>
#include <sstream>

#include <spirv2Metal/spirv.hpp>
#include <spirv2Metal/spirv_msl.hpp>
#include <spirv2Metal/spirv_cpp.hpp>

#include "hlsl2spirvCompile.h"

#include <ioSys/dag_fileIo.h>
#include <util/dag_string.h>

#include "HLSL2MetalCommon.h"
#include "buffBindPoints.h"

#include "../hlsl2spirv/HLSL2SpirVCommon.h"
#include "dataAccumulator.h"
#include "spirv2MetalCompiler.h"
#include "metalShaderType.h"

#ifdef __APPLE__
#include <unistd.h>
#endif
#include <vector>
#include <fstream>
#include <optional>

#include <EASTL/span.h>

static void processTextureData(spirv_cross::CompilerMSL &msl, DataAccumulator &header, const spirv_cross::ShaderResources &resources,
  const RegisterData &registers)
{
  const int imageCount = (int)resources.separate_images.size() + (int)resources.storage_images.size();
  header.appendValue(imageCount);

  auto getNextTRegister = [&, i = 0]() mutable { return registers.tRegistersForImages[i++]; };
  auto getNextURegister = [&, i = 0]() mutable { return registers.uRegistersImage[i++]; };

  auto processImageAndAppendData = [&](const spirv_cross::Resource &image) -> int {
    const auto &type = msl.get_type(image.base_type_id);
    const auto &imageInfo = type.image;
    const int hlslRegister = [&]() -> int {
      const bool isRWTexture = imageInfo.sampled == 2;
      if (isRWTexture)
        return getNextURegister() + drv3d_metal::MAX_SHADER_TEXTURES;
      return getNextTRegister();
    }();

    header.appendValue(hlslRegister);

    const int inShaderSlot = msl.get_decoration(image.id, spv::DecorationBinding);
    G_ASSERT(inShaderSlot >= 0);
    header.appendValue(inShaderSlot);

    const MetalImageType img_tp = translateImageType(imageInfo);
    header.appendValue(img_tp);

    return hlslRegister;
  };

  for (const auto &image : resources.storage_images)
  {
    processImageAndAppendData(image);
  }

  auto hasSampler = [&](std::string_view imgName) -> bool {
    for (const auto &sampler : resources.separate_samplers)
      if (msl.get_name(sampler.id).find(imgName) != std::string::npos)
        return true;
    return false;
  };

  // MSLCompiler makes it so in shader sampler index == texture index
  // so we can just store those and use for samplers
  std::vector<int> texIndexes = {};
  for (const auto &image : resources.separate_images)
  {
    const int tRegister = processImageAndAppendData(image);

    if (hasSampler(msl.get_name(image.id)))
      texIndexes.push_back(tRegister);
  }

  header.appendValue((int)resources.separate_samplers.size());

  for (int i = 0; i < resources.separate_samplers.size(); i++)
  {
    const auto &sampler = resources.separate_samplers[i];
    const int inShaderSlot = msl.get_decoration(sampler.id, spv::DecorationBinding);
    G_ASSERT(inShaderSlot >= 0);

    header.appendValue(texIndexes[i]);
    header.appendValue(inShaderSlot);
  }
}

// We have global buffer hardcoded to have bind slot == 2 in our metal driver (some legacy stuff)
// so to escape it we have to offset all of buffer slots by 2 in shader and header.
// Solution -- store all bindings during processBufferData and later replace all strings in resulting code.
struct ReplacePair
{
  int candidate;
  int replace;
};

struct BufferData
{
  int globalBufferRegisterCount;
  std::vector<ReplacePair> bufferSlots;
};

static constexpr int OFFSET_SLOT = 2;

static BufferData processBufferData(spirv_cross::CompilerMSL &msl, std::string_view source,
  struct spirv_cross::ShaderResources &resources, eastl::span<int> buffer_remap, const RegisterData &registers)
{
  std::vector<ReplacePair> bufferSlots{};

  auto processGlobalBuffer = [&](const auto &buff) -> int {
    int numRegisters = 0;
    const auto &type = msl.get_type(buff.base_type_id);
    numRegisters = msl.get_member_decoration(type.self, int(type.member_types.size() - 1), spv::DecorationOffset) / 16;

    const auto &membertype = msl.get_type(type.member_types.back());

    int sz = membertype.columns;
    if (membertype.basetype == spirv_cross::SPIRType::Struct)
    {
      const auto &lm = msl.get_type(membertype.member_types.back());
      sz = msl.get_member_decoration(membertype.self, int(membertype.member_types.size() - 1), spv::DecorationOffset) / 16;
      sz += lm.array.size() == 0 ? lm.columns : lm.columns * lm.array[0];
    }

    if (membertype.array.size() == 0)
    {
      numRegisters += sz;
    }
    else
    {
      numRegisters += sz * membertype.array[0];
    }

    return numRegisters;
  };

  // For uRegistersBuffer the sequence is opposite to the one
  // in shader so iteration is reversed
  auto getNextURegister = [&, i = registers.uRegistersBuffer.size() - 1]() mutable { return registers.uRegistersBuffer[i--]; };

  auto getNextTRegister = [&, i = 0]() mutable { return registers.tRegistersBuffer[i++]; };

  auto getAndSaveAdjustedInBufferSlot = [&](const spirv_cross::ID &buff_id) {
    const int inShaderSlot = msl.get_decoration(buff_id, spv::DecorationBinding);
    const int adjustedSlot = inShaderSlot + OFFSET_SLOT;
    bufferSlots.push_back({inShaderSlot, adjustedSlot});
    return adjustedSlot;
  };

  auto setBufferMapping = [&](const spirv_cross::Resource &buff, const drv3d_metal::BufferType type) {
    const int hlslRegister = [&]() -> int {
      if (type == drv3d_metal::RW_BUFFER)
        return getNextURegister();
      if (type == drv3d_metal::CONST_BUFFER)
        return 0;
      return getNextTRegister();
    }();

    const int driverSlot = drv3d_metal::RemapBufferSlot(type, hlslRegister);
    const int adjustedSlot = getAndSaveAdjustedInBufferSlot(buff.id);

    buffer_remap[driverSlot] = adjustedSlot;
  };

  int constBufferCount = registers.bRegistersConstBuffer.size();
  int numRegisters = 0;
  for (const auto &buff : resources.uniform_buffers)
  {
    const bool isGlobal = msl.get_name(buff.id).find("Globals") != std::string::npos;
    if (isGlobal)
    {
      getAndSaveAdjustedInBufferSlot(buff.id);
      numRegisters = processGlobalBuffer(buff);
      continue; // For global buffer mapping is not set
    }

    const drv3d_metal::BufferType type = [&]() {
      if (constBufferCount-- > 0) // Const buffers always go first in shader
        return drv3d_metal::CONST_BUFFER;
      return drv3d_metal::STRUCT_BUFFER;
    }();

    setBufferMapping(buff, type);
  }

  for (const auto &buff : resources.storage_buffers)
  {
    const drv3d_metal::BufferType type = [&]() {
      // Sadly NonWritable decoration is not set at all
      // so resulting shader code must be parsed.
      // Every buffer in shader looks like
      // ", (const) device <type>& <buffer_name> [[buffer(<bind_slot>)]],"
      // where absence of const == RW buffer
      size_t id = source.find(msl.get_name(buff.id));
      while (source[id] != ',')
        id--;
      id += 2;

      if (source[id] == 'd')
        return drv3d_metal::RW_BUFFER;
      else
        return drv3d_metal::STRUCT_BUFFER;
    }();
    setBufferMapping(buff, type);
  }

  return {numRegisters, bufferSlots};
}

static void processStageInputs(spirv_cross::CompilerMSL &msl, DataAccumulator &header, spirv_cross::ShaderResources &resources)
{
  int in_size = (int)resources.stage_inputs.size();
  header.appendValue(in_size);

  for (int i = 0; i < resources.stage_inputs.size(); i++)
  {
    const auto &input = resources.stage_inputs[i];
    const auto &membertype = msl.get_type(input.base_type_id);
    const int vsdr = msl.get_decoration(input.id, spv::DecorationLocation);
    const int reg = i;
    header.appendValue(reg);

    header.appendValue(membertype.vecsize);

    header.appendValue(vsdr);
  }
}

static void processAccelerationStructureData(spirv_cross::CompilerMSL &msl, DataAccumulator &header,
  const spirv_cross::ShaderResources &resources, const RegisterData &registers)
{
  const int accStructCount = resources.acceleration_structures.size();
  header.appendValue(accStructCount);

  auto getNextTRegister = [&, i = 0]() mutable { return registers.tRegistersAccelerationStructure[i++]; };

  for (const auto &accStruct : resources.acceleration_structures)
  {
    const int inShaderSlot = msl.get_decoration(accStruct.id, spv::DecorationBinding);
    G_ASSERT(inShaderSlot >= 0);

    const int driverSlot = getNextTRegister();

    header.appendValue(driverSlot);
    header.appendValue(inShaderSlot);
  }
}

void shiftBufferIndexes(String &mtl_src, std::vector<ReplacePair> &allBufferSlots)
{
  std::sort(allBufferSlots.begin(), allBufferSlots.end(), [](const auto &lhs, const auto &rhs) { return lhs.replace > rhs.replace; });

  for (const auto &[candidate, replace] : allBufferSlots)
  {
    String candidateStr(-1, "buffer(%d)", candidate);
    String replaceStr(-1, "buffer(%d)", replace);
    mtl_src.replace(candidateStr.c_str(), replaceStr.c_str());
  }
}

CompileResult compileShaderMetalDXC(const char *source, const char *profile, const char *entry, bool need_disasm, bool skipValidation,
  bool optimize, int max_constants_no, int bones_const_used, const char *shader_name, bool use_ios_token, bool use_binary_msl)
{
  const int index = g_index++;

  CompileResult compileResult;

  std::array<int, drv3d_metal::BUFFER_POINT_COUNT> bufferRemap;

  std::fill(bufferRemap.begin(), bufferRemap.end(), -1);

  (void)need_disasm;

  const ShaderType shaderStage = profile_to_shader_type(profile);
  if (shaderStage == ShaderType::Invalid)
  {
    compileResult.errors.sprintf("unknown target profile %s", profile);
    return compileResult;
  }

#if DAGOR_DBGLEVEL > 1
  optimize = false;
#endif

  auto hlsl2spirvResult = hlsl2spirv(source, profile, entry, skipValidation, compileResult);

  if (hlsl2spirvResult.failed)
    return compileResult;

  CompilerMSLlocal msl(hlsl2spirvResult.byteCode);

  setupMSLCompiler(msl, use_ios_token);

  std::string msource;
  try
  {
    msource = msl.compile();
  }
  catch (std::exception &e)
  {
    compileResult.errors.sprintf("Spir-V to MSL compilation failed:\n %s", e.what());
    return compileResult;
  }

  if (shaderStage == ShaderType::Compute)
  {
    std::stringstream errors;
    if (!msl.validate(errors))
    {
      compileResult.errors.sprintf("Spir-V to MSL LDS size validation failed \n%s\n", errors.str().c_str());
      return compileResult;
    }
  }

  String mtl_src = prependMetaDataAndReplaceFuncs(msource, shader_name, entry, index);

  DataAccumulator header;

  header.appendValue(shaderStage);

  header.appendStr(entry, 96);

  auto resources = msl.get_shader_resources(msl.get_active_interface_variables());

  auto [numRegisters, bufferSlots] =
    processBufferData(msl, {mtl_src.c_str(), mtl_src.size()}, resources, bufferRemap, hlsl2spirvResult.registers);
  header.appendContainer(bufferRemap);

  shiftBufferIndexes(mtl_src, bufferSlots);

  if (shaderStage == ShaderType::Vertex)
  {
    processStageInputs(msl, header, resources);
  }

  if (shaderStage == ShaderType::Compute)
  {
    header.appendValue(hlsl2spirvResult.computeInfo.threadGroupSizeX);

    header.appendValue(hlsl2spirvResult.computeInfo.threadGroupSizeY);

    header.appendValue(hlsl2spirvResult.computeInfo.threadGroupSizeZ);

    processAccelerationStructureData(msl, header, resources, hlsl2spirvResult.registers);
  }

  header.appendValue(numRegisters);

  // Unprocessed resources
  // stage_outputs // subpass_inputs // atomic_counters

  processTextureData(msl, header, resources, hlsl2spirvResult.registers);

  msource.assign(mtl_src.data());

  spitfile(entry, "metal_dxc", index, (void *)mtl_src.data(), (int)data_size(mtl_src));
  save2Lib(msource);

  compressData(compileResult, header, {mtl_src.c_str(), mtl_src.size()});

  return compileResult;
}
