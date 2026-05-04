// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include "render.h"
#include <AvailabilityMacros.h>
#include <osApiWrappers/dag_direct.h>
#include "drv_log_defs.h"

namespace drv3d_metal
{
  Shader::Shader()
  {
    src = nil;
    entry[0] = 0;
    num_reg = 0;

    for (int i = 0; i < BUFFER_POINT_COUNT; ++i)
      bufRemap[i].slot = -1;

    shd_type = 0;
    num_va = 0;

    tgrsz_x = 1;
    tgrsz_y = 1;
    tgrsz_z = 1;
  }

  bool Shader::loadFromSource(const char *source, bool async)
  {
    int sz = strlen(source);

    HashMD5 hash;
    hash.calc(source, sz);

    int data_size = strlen(source) + 1;

#if DAGOR_DBGLEVEL > 0
    src = [[NSString alloc] initWithUTF8String:source];
#endif

    num_tex = 2;

    for (int i = 0; i < num_tex; i++)
    {
      tex_binding[i] = i;
      tex_remap[i] = i;
      tex_type[i].value = 0;
    }
    if (strstr(source, "// debug_vs_metal"))
    {
      num_va = 2;
      num_reg = 4;
      va[0].reg = 0;
      va[0].vsdr = VSDR_POS;
      va[1].reg = 1;
      va[1].vsdr = VSDR_DIFF;
    }
    shader_hash = std::hash<std::string>{}(hash.get());

    return setup(source, data_size, async);
  }

  bool Shader::loadFromBinary(const uint8_t *meta, const char *source, uint64_t hash_override, bool async)
  {
    int magic = *((int*)meta);
    meta += 4;

    int data_size = *((int*)meta);
    meta += 4;

    const char* ptr = (const char *)meta;

    memcpy(&shader_hash, ptr, sizeof(shader_hash));
    ptr += sizeof(shader_hash);

    if (hash_override)
      shader_hash = hash_override;

    shd_type = *((int*)ptr);
    ptr += 4;

    strcpy(entry, ptr);
    ptr += 96;

    memcpy(bufRemap, ptr, sizeof(bufRemap));
    ptr += sizeof(bufRemap);

    uint32_t flags = 0;
    memcpy(&flags, ptr, sizeof(flags));
    ptr += sizeof(flags);

    if (flags & ShaderFlags::HasSamplerBiases)
      has_biases = true;
    if (flags & ShaderFlags::HasImmediateConstants)
      immediate_slot = IMMEDIATE_BIND_SLOT;

    if (shd_type == 1)
    {
      num_va = *((int*)ptr);
      ptr += 4;

      for (int i = 0; i < num_va; i++)
      {
        va[i].reg = *((int*)ptr);
        ptr += 4;

        va[i].vec = *((int*)ptr);
        ptr += 4;

        va[i].vsdr = *((int*)ptr);
        ptr += 4;
      }
    }

    if (shd_type == 2)
    {
      output_mask = *((uint32_t*)ptr);
      ptr += 4;
    }

    if (shd_type == 3)
    {
      tgrsz_x = *((int*)ptr);
      ptr += 4;

      tgrsz_y = *((int*)ptr);
      ptr += 4;

      tgrsz_z = *((int*)ptr);
      ptr += 4;

      accelerationStructureCount = *((int*)ptr);
      ptr += 4;

      for (int i = 0; i < accelerationStructureCount; i++)
      {
        const int inDriverSlot = *((int*)ptr);
        ptr += 4;
        acceleration_structure_remap[i] = inDriverSlot;

        const int inShaderSlot = *((int*)ptr);
        ptr += 4;
        acceleration_structure_binding[inDriverSlot] = inShaderSlot;
      }
    }

    num_reg = *((int*)ptr) * 16;
    ptr += 4;

    num_tex = *((int*)ptr);
    ptr += 4;

    G_ASSERT(num_tex <= g_max_textures_in_shader);
    for (int i = 0; i < num_tex; i++)
    {
      tex_binding[i] = *((int*)ptr);
      ptr += 4;

      tex_remap[i] = *((int*)ptr);
      ptr += 4;

      tex_type[i].value = *((int*)ptr);
      ptr += 4;
    }

    num_samplers = *((int*)ptr);
    ptr += 4;

    for (int i = 0; i < num_samplers; i++)
    {
      sampler_binding[i] = *((int*)ptr);
      ptr += 4;

      sampler_mask |= 1ull << sampler_binding[i];

      sampler_remap[i] = *((int*)ptr);
      ptr += 4;
    }

    bool binary = memcmp(source, "MTLB", 4) == 0;
#if DAGOR_DBGLEVEL > 0
    if (!binary)
      src = [[NSString alloc] initWithCString:source encoding : NSASCIIStringEncoding];
#endif
    if (!binary)
      name = getName(source);

    if (shd_type == 3)
    {
      MTLSize ts = render.device.maxThreadsPerThreadgroup;
      if (tgrsz_x > ts.width || tgrsz_y > ts.height || tgrsz_z > ts.depth)
      {
        D3D_ERROR("Shader %s has incorrect threadgroup size %dx%dx%d", name.c_str(), tgrsz_x, tgrsz_y, tgrsz_z);
        return false;
      }
    }

    return setup(source, data_size, async);
  }

  bool Shader::setup(const char *data, int data_size, bool async)
  {
    memset(tex_slot_remap, -1, sizeof(tex_slot_remap));
    memset(buf_slot_remap, -1, sizeof(buf_slot_remap));

    for (int i = CONST_BUFFER_POINT; i < BINDLESS_TEXTURE_ID_BUFFER_POINT; ++i)
    {
      if (bufRemap[i].slot >= 0)
      {
        buffers[num_buffers].slot = bufRemap[i].slot;
        buffers[num_buffers].remapped_slot = i;
        buffer_mask |= 1ull << bufRemap[i].slot;
        buf_slot_remap[i] = bufRemap[i].slot;
        num_buffers++;
      }
    }

    for (int i = 0; i < num_tex; i++)
    {
      texture_mask |= 1ull << tex_remap[i];
      tex_slot_remap[tex_binding[i]] = tex_remap[i];
    }

    for (int i = BINDLESS_TEXTURE_ID_BUFFER_POINT; i < BUFFER_POINT_COUNT; ++i)
      if (bufRemap[i].slot >= 0)
      {
        auto &remap = bindless_buffers[num_bindless_buffers++];
        remap = bufRemap[i];
        bindless_mask |= 1ull << remap.slot;
        buf_slot_remap[i] = bufRemap[i].slot;

        if (remap.remap_type == EncodedBufferRemap::RemapType::Sampler)
          bindless_type_mask |= BindlessTypeSampler;
        else if (remap.remap_type == EncodedBufferRemap::RemapType::Buffer)
          bindless_type_mask |= BindlessTypeBuffer;
        else
        {
          if (remap.texture_type == MetalImageType::Tex2D)
            bindless_type_mask |= BindlessTypeTexture2D;
          else if (remap.texture_type == MetalImageType::TexCube)
            bindless_type_mask |= BindlessTypeTextureCube;
          else if (remap.texture_type == MetalImageType::Tex2DArray)
            bindless_type_mask |= BindlessTypeTexture2DArray;
          else
            G_ASSERTF(0, "Unsupported bindless texture array type %d", int(remap.texture_type));
        }
      }

    auto func = render.shadersPreCache.getShader(this, data, data_size, async);
#if DAGOR_DBGLEVEL > 0
    if (func)
      func.label = [NSString stringWithUTF8String : name.c_str()];
#endif
    return async || func != nil;
  }

  bool Shader::compileShader(const uint8_t *meta, const char* source, uint64_t hash_override, bool async)
  {
    uint32_t magic = meta ? *(uint32_t *)meta : 0;
    if (magic == _MAKE4C('MTLM')) // packed mesh shader and friends
    {
      meta += 4;

      uint32_t ms_size = *(uint32_t *)meta;
      G_ASSERT(ms_size);
      meta += sizeof(ms_size);

      uint64_t ms_hash = 0;
      memcpy(&ms_hash, meta, sizeof(ms_hash));
      G_ASSERT(ms_hash);
      meta += sizeof(ms_hash);

      uint32_t as_size = *(uint32_t *)meta;
      meta += sizeof(as_size);

      uint64_t as_hash = 0;
      memcpy(&as_hash, meta, sizeof(as_hash));
      meta += sizeof(as_hash);

      this->mesh_shader = new Shader();
      if (!mesh_shader->compileShader(meta, source, ms_hash, async))
      {
        this->mesh_shader->release();
        this->mesh_shader = nullptr;
        return false;
      }
      source += ms_size;

      if (as_size)
      {
        this->amplification_shader = new Shader();
        if (!amplification_shader->compileShader(meta, source, as_hash, async))
        {
          this->mesh_shader->release();
          this->amplification_shader->release();
          this->mesh_shader = nullptr;
          this->amplification_shader = nullptr;
          return false;
        }
      }

      return true;
    }
    else if (magic == _MAKE4C('MTLZ')) // ordinary metal shader
      return loadFromBinary(meta, source, hash_override, async);
    else if (strncmp(source, "#inc", 4) == 0) // hardcoded shader source
      return loadFromSource(source, async);

    return false;
  }

  void Shader::release()
  {
    if (mesh_shader)
      this->mesh_shader->release();
    if (amplification_shader)
      this->amplification_shader->release();
    if (src)
      [src release];

    delete this;
  }

  eastl::string Shader::getName(const char *ptr)
  {
    bool binary = ptr && memcmp(ptr, "MTLB", 4) == 0;
    if (!binary)
    {
      const char *newline = strstr(ptr, "\n");
      if (newline)
        return eastl::string(ptr, newline - ptr);
    }
    return "(none)";
  }
}
