// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_shader.h>
#include <util/dag_string.h>
#include <util/dag_hash.h>
#include <dag/dag_vector.h>
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
struct ShaderDebugInfo
{
  String name;
  String spirvDisAsm;
  String dxbcDisAsm;
  String sourceGLSL;
  String reconstructedHLSL;
  String reconstructedHLSLAndSourceHLSLXDif;
  String sourceHLSL;
  // name from engine with additional usefull info
  String debugName;
};
#endif

struct ShaderModuleBlob
{
  ShaderSource source;
  uint32_t offset = 0;
  // size 0 means whole bytecode data is used
  uint32_t size = 0;
  // 0 means the bytecode is not smolv encoded
  uint32_t sizeSmolv = 0;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  String name;
#endif

  ShaderModuleBlob() = default;
  ShaderModuleBlob(const uint8_t *start, const uint8_t *end)
  {
    source.compressedData = make_span_const(start, end - start);
    source.uncompressedSize = source.compressedData.size();
    size = source.compressedData.size();
  }

  uint32_t getHash32() const { return 0u; }
};

struct ShaderModuleHeader
{
  spirv::ShaderHeader header;
  spirv::HashValue hash;
  VkShaderStageFlags stage;

  uint32_t getHash32() const { return mem_hash_fnv1<32>((const char *)&hash, sizeof(hash)); }
};

struct ShaderModuleUse
{
  ShaderModuleHeader header;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  ShaderDebugInfo dbg;
#endif
  uint32_t blobId;
};
