// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "asmShaderHLSL2MetalDXC.h"

#include <algorithm>
#include <sstream>

#include <spirv2Metal/spirv.hpp>
#include <spirv2Metal/spirv_msl.hpp>
#include <spirv2Metal/spirv_cpp.hpp>

#include "hlsl2spirvCompile.h"

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <util/dag_string.h>
#include <generic/dag_align.h>

#include "HLSL2MetalCommon.h"
#include "buffBindPoints.h"

#include "../debugSpitfile.h"
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

CompileResult compileShaderMetalDXC(const spirv::DXCContext *dxc_ctx, const char *source, const char *profile, const char *entry,
  bool need_disasm, bool hlsl2021, bool enable_fp16, bool skipValidation, bool optimize, int max_constants_no, const char *shader_name,
  bool use_ios_token, bool use_binary_msl, uint64_t shader_variant_hash, bool enableBindless)
{
  CompileResult compileResult;

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

  auto hlsl2spirvResult =
    hlsl2spirv(dxc_ctx, source, profile, entry, hlsl2021, enable_fp16, skipValidation, compileResult, enableBindless);

  if (hlsl2spirvResult.failed)
    return compileResult;

  CompilerMSLlocal msl(hlsl2spirvResult.byteCode, use_ios_token);

  compileResult.computeShaderInfo.threadGroupSizeX = hlsl2spirvResult.computeInfo.threadGroupSizeX;
  compileResult.computeShaderInfo.threadGroupSizeY = hlsl2spirvResult.computeInfo.threadGroupSizeY;
  compileResult.computeShaderInfo.threadGroupSizeZ = hlsl2spirvResult.computeInfo.threadGroupSizeZ;

  return msl.convertToMSL(compileResult, hlsl2spirvResult.reflection, source, shaderStage, shader_name, entry, shader_variant_hash,
    use_binary_msl);
}

template <class T>
static T read_pod(auto &reader)
{
  T t;
  reader.read(&t, sizeof(t));
  return t;
}
template <class T, size_t COUNT>
static void read_pod(auto &reader, T (&out)[COUNT])
{
  reader.read(out, sizeof(out));
}

eastl::string disassembleShaderMetalDXC(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata)
{
#define FORMAT_FAIL(fmt_, ...)                                            \
  do                                                                      \
  {                                                                       \
    logerr("Invalid Metal shader blob format: " fmt_ ".", ##__VA_ARGS__); \
    return {};                                                            \
  } while (0)

  InPlaceMemLoadCB reader{metadata.data(), int(metadata.size())};
  int sign = reader.readInt();

  if (sign != _MAKE4C('MTLZ'))
    FORMAT_FAIL("invalid signature '%c%c%c%c'", DUMP4C(sign));

  int bytecodeSize = reader.readInt();

  if (dag::align_up(bytecodeSize, sizeof(uint32_t)) != bytecode.size())
    FORMAT_FAIL("bytecode size mismatch in metadata and blob %d != %d", bytecodeSize, bytecode.size());

  auto hash = read_pod<uint64_t>(reader);

  eastl::string hashDisasm{eastl::string::CtorSprintf{}, "%x", hash};
  eastl::string_view sourceDisasm{(char const *)bytecode.data(), bytecode.size()};
  eastl::string headerDisasm{};

  {
    drv3d_metal::EncodedBufferRemap bufRemap[drv3d_metal::BUFFER_POINT_COUNT];
    char entry[96];

    auto shaderType = read_pod<ShaderType>(reader);
    char const *shaderTypeName;
    switch (shaderType)
    {
      case ShaderType::Vertex: shaderTypeName = "vertex"; break;
      case ShaderType::Pixel: shaderTypeName = "pixel"; break;
      case ShaderType::Compute: shaderTypeName = "compute"; break;
      case ShaderType::Mesh: shaderTypeName = "mesh"; break;
      case ShaderType::Amplification: shaderTypeName = "amplification"; break;

      default: FORMAT_FAIL("invalid shader type %d", int(shaderType));
    }

    headerDisasm.append_sprintf("  Type: %s\n", shaderTypeName);

    read_pod(reader, entry);
    headerDisasm.append_sprintf("  Profile: %s\n", entry);

    static constexpr char const *METAL_IMAGE_TYPE_NAMES[] = {
      "Tex2D",
      "Tex2DArray",
      "Tex2DDepth",
      "TexCube",
      "Tex3D",
      "TexBuffer",
      "TexCubeArray",
    };
    static constexpr char const *METAL_BUFFER_TYPE_NAMES[] = {
      "GEOM_BUFFER",
      "CONST_BUFFER",
      "STRUCT_BUFFER",
      "RW_BUFFER",
      "BINDLESS_TEXTURE_ID_BUFFER",
      "BINDLESS_SAMPLER_ID_BUFFER",
      "BINDLESS_BUFFER_ID_BUFFER",
    };

    read_pod(reader, bufRemap);
    headerDisasm.append("  Buffer remap: [");
    for (auto const &remap : bufRemap)
    {
      if (remap.remap_type == drv3d_metal::EncodedBufferRemap::RemapType::Invalid || remap.slot == -1)
      {
        headerDisasm.append(" {}");
        continue;
      }

      if (remap.remap_type == drv3d_metal::EncodedBufferRemap::RemapType::Texture)
        headerDisasm.append_sprintf(" {texture texType=%s slot=%d}", METAL_IMAGE_TYPE_NAMES[int(remap.texture_type)], int(remap.slot));
      else if (remap.remap_type == drv3d_metal::EncodedBufferRemap::RemapType::Buffer)
        headerDisasm.append_sprintf(" {buffer bufType=%s slot=%d}", METAL_BUFFER_TYPE_NAMES[int(remap.buffer_type)], int(remap.slot));
      else if (remap.remap_type == drv3d_metal::EncodedBufferRemap::RemapType::Sampler)
        headerDisasm.append_sprintf(" {sampler slot=%d}", int(remap.slot));
      else
        FORMAT_FAIL("invalid remapping entry %d", remap.value);
    }
    headerDisasm.append(" ]\n");

    auto flags = read_pod<uint32_t>(reader);
    headerDisasm.append("  Flags: ");
    eastl::string flagsDisasm{};

#define FLAG(name_) (eastl::make_pair(drv3d_metal::ShaderFlags::name_, #name_))
    for (auto [value, name] : {FLAG(HasSamplerBiases), FLAG(HasImmediateConstants)})
    {
      if (flags & value)
      {
        if (!flagsDisasm.empty())
          flagsDisasm.append(" | ");
        flagsDisasm.append(name);
      }
    }
    headerDisasm.append_sprintf("%s\n", flagsDisasm.c_str());
#undef FLAG

    if (shaderType == ShaderType::Vertex)
    {
      int numVA = read_pod<int>(reader);
      headerDisasm.append("  Vertex attributes: [");
      for (int i = 0; i < numVA; ++i)
      {
        struct VA
        {
          int reg;
          int vec;
          int vsdr;
        };
        VA va = read_pod<VA>(reader);
        headerDisasm.append_sprintf(" {reg=%d vec=%d vsdr=%d}", va.reg, va.vec, va.vsdr);
      }
      headerDisasm.append(" ]\n");
    }

    if (shaderType == ShaderType::Pixel)
      headerDisasm.append_sprintf("  Output mask: %x\n", read_pod<uint32_t>(reader));

    if (shaderType == ShaderType::Compute)
    {
      int tgs[3];
      read_pod(reader, tgs);
      headerDisasm.append_sprintf("  Thread group sizes: {%d %d %d}\n", tgs[0], tgs[1], tgs[2]);
      if (int numAS = read_pod<int>(reader))
      {
        headerDisasm.append("  Acceleration structure map: [");
        for (int i = 0; i < numAS; ++i)
        {
          struct AS
          {
            int driverSlot;
            int shaderSlot;
          };
          AS as = read_pod<AS>(reader);
          headerDisasm.append_sprintf(" {dirverSlot=%d shaderSlot=%d}", as.driverSlot, as.shaderSlot);
        }
        headerDisasm.append(" ]\n");
      }
    }

    int numReg, numTex, numSmp;

    headerDisasm.append_sprintf("  Num registers: %d\n", numReg = read_pod<int>(reader));
    headerDisasm.append_sprintf("  Num textures: %d\n", numTex = read_pod<int>(reader));

    if (numTex)
    {
      headerDisasm.append("  Texture map: [");
      for (int i = 0; i < numTex; ++i)
      {
        int bind = read_pod<int>(reader);
        int remap = read_pod<int>(reader);
        auto type = read_pod<drv3d_metal::EncodedMetalImageType>(reader);
        char const *elemType = type.is_uint ? "uint" : (type.is_int ? "int" : "float");
        headerDisasm.append_sprintf(" {binding=%d remap=%d type=%s elemType=%s}", bind, remap, METAL_IMAGE_TYPE_NAMES[int(type.type)],
          elemType);
      }
      headerDisasm.append(" ]\n");
    }

    headerDisasm.append_sprintf("  Num samplers: %d\n", numSmp = read_pod<int>(reader));

    if (numSmp)
    {
      headerDisasm.append("  Sampler map: [");
      for (int i = 0; i < numSmp; ++i)
      {
        int bind = read_pod<int>(reader);
        int remap = read_pod<int>(reader);
        headerDisasm.append_sprintf(" {binding=%d remap=%d}", bind, remap);
      }
      headerDisasm.append(" ]\n");
    }
  }

  auto optStr = [](auto const &str) -> char const * { return str.empty() ? "<not present>" : str.data(); };

  if (sourceDisasm.starts_with("MTLB"))
  {
    return eastl::string{eastl::string::CtorSprintf{},
      "Hash: %s\n"
      "Header:\n%s"
      "MSL: <binary>\n",
      optStr(hashDisasm), optStr(headerDisasm)};
  }
  else
  {
    return eastl::string{eastl::string::CtorSprintf{},
      "Hash: %s\n"
      "Header:\n%s"
      "MSL:\n%.*s\n",
      optStr(hashDisasm), optStr(headerDisasm), sourceDisasm.size(), sourceDisasm.data()};
  }
#undef FORMAT_FAIL
}
