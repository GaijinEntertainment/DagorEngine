// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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
  dag::Vector<uint32_t> blob;
  spirv::HashValue hash;
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  String name;
#endif
  size_t getBlobSize() const { return blob.size() * sizeof(uint32_t); }

  uint32_t getHash32() const { return mem_hash_fnv1<32>((const char *)&hash, sizeof(hash)); }
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
