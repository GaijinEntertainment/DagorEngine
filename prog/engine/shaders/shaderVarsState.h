// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>
#include <drv/3d/dag_samplerHandle.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include <vecmath/dag_vecMathDecl.h>
#include <memory/dag_framemem.h>
#include <memory/dag_memBase.h>
#include <shaders/dag_shaderVarType.h>
#include <shaders/shInternalTypes.h>
#include <shaders/shader_layout.h>


struct RaytraceTopAccelerationStructure;

// NOTE: this CAN be done through templates, but I hate it
// Also note that we use simd types for their alignment
#define SHVAR_TYPES                      \
  X(SHVT_INT, int)                       \
  X(SHVT_REAL, float)                    \
  X(SHVT_COLOR4, vec4f)                  \
  X(SHVT_TEXTURE, shaders_internal::Tex) \
  X(SHVT_BUFFER, shaders_internal::Buf)  \
  X(SHVT_INT4, vec4i)                    \
  X(SHVT_FLOAT4X4, TMatrix4_vec4)        \
  X(SHVT_SAMPLER, d3d::SamplerHandle)    \
  X(SHVT_TLAS, RaytraceTopAccelerationStructure *)

class ShaderVarsState
{
  struct VarDescription
  {
    void *ptr;
  };

  static EA_CONSTEXPR_OR_CONST eastl::array<size_t, SHVT_COUNT_> SHVAR_TYPE_SIZES{
#define X(SHVT, Type) sizeof(Type),
    SHVAR_TYPES
#undef X
  };
  static EA_CONSTEXPR_OR_CONST eastl::array<size_t, SHVT_COUNT_> SHVAR_TYPE_ALIGNMENTS{
#define X(SHVT, Type) alignof(Type),
    SHVAR_TYPES
#undef X
  };

  static ptrdiff_t align(ptrdiff_t offset, size_t alignment) { return (offset + alignment - 1) & ~(alignment - 1); }

  // Fixup shaderCompiler's layout of global variables with padding
  // to respect the current platform's alignments
  static dag::Vector<uint32_t, framemem_allocator> recalc_layout(const bindump::Mapper<shader_layout::VarList> &glob_vars)
  {
    dag::Vector<uint32_t, framemem_allocator> offsets;
    offsets.reserve(glob_vars.size());
    uint32_t curOffset = 0;
    for (int idx = 0; idx < glob_vars.size(); ++idx)
    {
      const uint8_t type = glob_vars.v[idx].type;
      curOffset = align(curOffset, SHVAR_TYPE_ALIGNMENTS[type]);
      offsets.push_back(curOffset);
      curOffset += SHVAR_TYPE_SIZES[type];
    }
    offsets.push_back(curOffset);
    return offsets;
  }

public:
  void load(const bindump::Mapper<shader_layout::ScriptedShadersBinDump> &dump)
  {
    variables.reserve(dump.globVars.size());

    const auto offsets = recalc_layout(dump.globVars);

    {
      const size_t storageAlign = *eastl::max_element(SHVAR_TYPE_ALIGNMENTS.begin(), SHVAR_TYPE_ALIGNMENTS.end());
      const size_t storageSize = offsets.back();
      storage.reset(static_cast<char *>(memalloc_aligned_default(storageSize, storageAlign)));
    }

    const d3d::SamplerHandle defaultSamplerHnd = d3d::request_sampler({});

    for (int idx = 0; idx < dump.globVars.size(); ++idx)
    {
      const auto &var = dump.globVars.v[idx];
      VarDescription desc{storage.get() + offsets[idx]};

      // Initial values for shader vars are stored in the dump
      if (var.type == SHVT_SAMPLER)
        *(d3d::SamplerHandle *)desc.ptr = defaultSamplerHnd;
      else
        memcpy(desc.ptr, &var.valPtr.get(), SHVAR_TYPE_SIZES[var.type]);

      variables.push_back(desc);
    }
  }

  void clear()
  {
    variables.clear();
    storage.reset();
  }

  template <class T>
  const T &get(int idx) const
  {
    return *static_cast<const T *>(variables[idx].ptr);
  }

  // NOTE: evil aliasing is performed here for SIMD matrices & vectors.
  template <class T>
  T &get(int idx)
  {
    return *static_cast<T *>(variables[idx].ptr);
  }

  void *get_raw(int idx) const { return variables[idx].ptr; }

  template <class T>
  void set(int idx, const T &value)
  {
    *static_cast<T *>(variables[idx].ptr) = value;
  }

  size_t size() const { return variables.size(); }

private:
  dag::Vector<VarDescription> variables;

  struct AlignedDeleter
  {
    void operator()(void *ptr) const { memfree_aligned_default(ptr); }
  };

  // Single contiguous storage with order preserved from the original
  // layout increases probability of cache hits
  eastl::unique_ptr<char[], AlignedDeleter> storage;
};

#undef SHVAR_TYPES
