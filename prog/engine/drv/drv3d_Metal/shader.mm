// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include "render.h"
#include <AvailabilityMacros.h>
#include <osApiWrappers/dag_direct.h>

extern void getCodeFromZ(const char *source, int sz, Tab<char> &code);

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

    src = [[NSString alloc] initWithUTF8String:source];

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

  bool Shader::loadFromBinary(const char *source, bool async)
  {
    int magic = *((int*)source);
    source += 4;

    int sz = *((int*)source);
    source += 4;

    Tab<char> code;
    getCodeFromZ(source, sz, code);

    char* ptr = &code[0];

    HashMD5 hash;
    hash.set(ptr);
    ptr += 32;

    shd_type = *((int*)ptr);
    ptr += 4;

    strcpy(entry, ptr);
    ptr += 96;

    memcpy(bufRemap, ptr, sizeof(bufRemap));
    ptr += sizeof(bufRemap);

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

      texture_mask |= 1ull << tex_binding[i];

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

    int data_size = code.size() - (ptr - code.data());

    if (memcmp(ptr, "MTLB", 4))
      src = [[NSString alloc] initWithCString:ptr encoding : NSASCIIStringEncoding];
    else
      data_size -= 1; // binary shouldn't be null terminated

    shader_hash = std::hash<std::string>{}(hash.get());

    return setup(ptr, data_size, async);
  }

  bool Shader::setup(const char *data, int data_size, bool async)
  {
    #define IMMEDIATE_CB_REGISTER_NO 8

    int remappedImmediate = RemapBufferSlot(CONST_BUFFER, IMMEDIATE_CB_REGISTER_NO);
    if (bufRemap[remappedImmediate].slot >= 0)
      immediate_slot = bufRemap[remappedImmediate].slot;
    bufRemap[remappedImmediate].slot = -1;

    for (int i = CONST_BUFFER_POINT; i < BINDLESS_TEXTURE_ID_BUFFER_POINT; ++i)
    {
      if (bufRemap[i].slot >= 0)
      {
        buffers[num_buffers].slot = bufRemap[i].slot;
        buffers[num_buffers].remapped_slot = i;
        buffer_mask |= 1ull << i;
        num_buffers++;
      }
    }

    for (int i = BINDLESS_TEXTURE_ID_BUFFER_POINT; i < BUFFER_POINT_COUNT; ++i)
      if (bufRemap[i].slot >= 0)
      {
        auto &remap = bindless_buffers[num_bindless_buffers++];
        remap = bufRemap[i];
        bindless_mask |= 1ull << i;

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

    auto func = render.shadersPreCache.getShader(shader_hash, entry, data, data_size, async);
    return async || func != nil;
  }

  bool Shader::compileShader(const char* source, bool async)
  {
    uint32_t magic = *(uint32_t *)source;
    if (magic == _MAKE4C('MTLM')) // packed mesh shader and friends
    {
      source += 4;

      uint32_t ms_size = *(uint32_t *)source;
      G_ASSERT(ms_size);
      source += 4;

      this->mesh_shader = new Shader();
      if (!mesh_shader->compileShader(source + 4, async))
      {
        this->mesh_shader->release();
        this->mesh_shader = nullptr;
        return false;
      }
      source += ms_size;

      uint32_t as_size = *(uint32_t *)source;
      source += 4;

      if (as_size)
      {
        this->amplification_shader = new Shader();
        if (!amplification_shader->compileShader(source + 4, async))
        {
          this->mesh_shader->release();
          this->amplification_shader->release();
          this->mesh_shader = nullptr;
          this->amplification_shader = nullptr;
          return false;
        }
      }
      source += as_size;

      return true;
    }
    else if (magic == _MAKE4C('MTLZ')) // ordinary metal shader
      return loadFromBinary(source, async);
    else if (magic == _MAKE4C('#inc')) // hardcoded shader source
      return loadFromSource(source, async);

    return false;
  }

  void Shader::release()
  {
    if (mesh_shader)
      this->mesh_shader->release();
    if (amplification_shader)
      this->amplification_shader->release();
    [src release];

    delete this;
  }
}
