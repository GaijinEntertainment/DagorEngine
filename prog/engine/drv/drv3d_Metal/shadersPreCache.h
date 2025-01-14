// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>

#include <hash/md5.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <memory/dag_fixedBlockAllocator.h>
#include <util/dag_simpleString.h>

#include "EASTL/vector.h"

#include "program.h"

namespace drv3d_metal
{
struct HashMD5
{
  md5_state_t st;
  char hash[33];

  HashMD5() { md5_init(&st); }

  void calc(const void *ptr, int size)
  {
    md5_append(&st, (const md5_byte_t *)ptr, size);

    unsigned char tmp[16];
    md5_finish(&st, (md5_byte_t *)tmp);

    for (int i = 0; i < 16; i++)
    {
      snprintf(&hash[i * 2], sizeof(hash), "%02x", tmp[i]);
    }
  }

  const char *get() { return hash; }

  void set(const char *ptr)
  {
    memcpy(hash, ptr, 32);
    hash[32] = 0;
  }
};

class Shader;

class ShadersPreCache
{
  struct CachedShader
  {
    id<MTLFunction> func = nil;
    id<MTLLibrary> lib = nil;
  };

  // this is binary serialized. if something is changed need to bump pipeline cache version
  struct CachedVertexDescriptor
  {
    static const int max_attributes = 16;
    static const int max_streams = 4;

    struct Attr
    {
      MTLVertexFormat format = MTLVertexFormatInvalid;
      uint8_t stream = 0;
      uint8_t offset = 0;
      uint8_t slot = 0;
    };

    struct Stream
    {
      uint8_t stride = 0;
      uint8_t step = 0;
      uint8_t stream = 0;
    };

    uint32_t attr_count = 0;
    uint32_t stream_count = 0;

    Attr attributes[max_attributes];
    Stream streams[max_streams];

    MTLVertexDescriptor *descriptor = nil;
  };

  struct CachedPipelineState
  {
    uint64_t vs_hash = 0;
    uint64_t ps_hash = 0;
    uint64_t descriptor_hash = 0;
    Program::RenderState rstate;
    id<MTLRenderPipelineState> pso = nil;
    uint32_t discard = 0;
    uint32_t output_mask = 0;
  };

  struct CachedComputePipelineState
  {
    id<MTLComputePipelineState> cso = nil;
  };

  ska::flat_hash_map<uint64_t, CachedShader *> shader_cache;
  ska::flat_hash_map<uint64_t, CachedVertexDescriptor *> descriptor_cache;
  ska::flat_hash_map<uint64_t, CachedPipelineState *> pso_cache;
  ska::flat_hash_map<uint64_t, CachedComputePipelineState *> cso_cache;

  FixedBlockAllocator shader_cache_objects;
  FixedBlockAllocator descriptor_cache_objects;
  FixedBlockAllocator pso_cache_objects;
  FixedBlockAllocator cso_cache_objects;

  uint32_t cache_version = 0;

  char shdCachePath[1024];

  struct QueuedShader
  {
    uint64_t hash = 0;
    char entry[96] = {0};
    eastl::vector<uint8_t> data;
    CachedShader *result = nullptr;
  };

  size_t g_shaders_saved = 0;
  size_t g_descriptors_saved = 0;
  size_t g_psos_saved = 0;
  size_t g_csos_saved = 0;
  bool g_cache_dirty = false;
  eastl::vector<QueuedShader> g_queued_shaders;

  bool pso_cache_loaded = false;

  std::atomic<bool> g_is_exiting;
  std::atomic<bool> g_saver_exited;
  std::atomic<bool> g_compiler_exited;

  std::mutex g_saver_mutex;
  std::condition_variable g_saver_condition;

  std::mutex g_compiler_mutex;
  std::condition_variable g_compiler_condition;
  ska::flat_hash_map<uint64_t, CachedPipelineState *> pso_compiler_cache;
  ska::flat_hash_map<uint64_t, CachedPipelineState *> pso_compiler_done;

  ska::flat_hash_map<uint64_t, QueuedShader> shader_compiler_cache;
  ska::flat_hash_map<uint64_t, CachedShader *> shader_compiler_done;

public:
  ShadersPreCache();
  ~ShadersPreCache();

  void init(uint32_t version);

  void loadPreCache();
  void savePreCache();

  MTLVertexDescriptor *buildDescriptor(Shader *vshader, VDecl *vdecl, const Program::RenderState &rstate, uint64_t &hash);
  id<MTLFunction> getShader(uint64_t shader_hash, const char *entry, const char *data, int size, bool async);
  id<MTLRenderPipelineState> getState(Program *program, VDecl *vdecl, const Program::RenderState &rstate, bool async);

  id<MTLFunction> compileShader(const QueuedShader &shader);
  MTLVertexDescriptor *compileDescriptor(uint64_t hash, CachedVertexDescriptor &desc);
  id<MTLRenderPipelineState> compilePipeline(uint64_t hash, CachedPipelineState *pso, bool free);
  id<MTLComputePipelineState> compilePipeline(uint64_t hash);

  void saveShaders(const ska::flat_hash_map<uint64_t, CachedShader *> &shaderCache);
  void saveDescriptors(const ska::flat_hash_map<uint64_t, CachedVertexDescriptor *> &descriptorCache);
  void savePSOs(const ska::flat_hash_map<uint64_t, CachedPipelineState *> &psoCache);
  void saveCSOs(const ska::flat_hash_map<uint64_t, CachedComputePipelineState *> &psoCache);

  void tickCache();

  void saverThread();
  void compilerThread();

  void release();
};
} // namespace drv3d_metal
