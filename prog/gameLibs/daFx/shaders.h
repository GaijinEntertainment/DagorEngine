// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"
#include "buffers.h"
#include <EASTL/sort.h>
#include <EASTL/hash_map.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <util/dag_generationReferencedData.h>

namespace dafx
{
using RenderShaderPtr = eastl::unique_ptr<DynamicShaderHelper>;
using GpuComputeShaderPtr = eastl::unique_ptr<ComputeShaderElement>;
using CpuComputeShaderPtr = cpu_exec_cb_t;

using RenderShaderId = GenerationRefId<8, RenderShaderPtr>;
using GpuComputeShaderId = GenerationRefId<8, GpuComputeShaderPtr>;
using CpuComputeShaderId = GenerationRefId<8, CpuComputeShaderPtr>;

using RenderShaderMap = eastl::hash_map<eastl::string, RenderShaderId>;
using GpuComputeShaderMap = eastl::hash_map<eastl::string, GpuComputeShaderId>;
using CpuComputeShaderMap = eastl::hash_map<eastl::string, CpuComputeShaderId>;

using RenderTagMap = eastl::hash_map<eastl::string, uint32_t>;
using RenderTagInverseMap = eastl::hash_map<uint32_t, eastl::string>;

struct Context;
struct Shaders
{
  RenderTagMap renderTagMap;
  RenderTagInverseMap renderTagInverseMap;

  RenderShaderMap renderShadersMap;
  GpuComputeShaderMap gpuComputeShadersMap;
  CpuComputeShaderMap cpuComputeShadersMap;

  GenerationReferencedData<RenderShaderId, RenderShaderPtr> renderShaders;
  GenerationReferencedData<GpuComputeShaderId, GpuComputeShaderPtr> gpuComputeShaders;
  GenerationReferencedData<CpuComputeShaderId, CpuComputeShaderPtr> cpuComputeShaders;
};

uint32_t register_render_tag(Shaders &dst, const eastl::string &tag, uint32_t max_tags);
RenderShaderId register_render_shader(Shaders &dst, const eastl::string &shader_name, const eastl::string &sys_name, VDECL vdecl);
GpuComputeShaderId register_gpu_compute_shader(Shaders &dst, const eastl::string &shader_name, const eastl::string &sys_name);
CpuComputeShaderId register_cpu_compute_shader_opt(Shaders &dst, const eastl::string &shader_name);

int update_cpu_emission(Context &ctx, const eastl::vector<int> &workers, int start, int count);
int update_cpu_simulation(Context &ctx, const eastl::vector<int> &workers, int start, int count);

void update_gpu_emission(Context &ctx, const eastl::vector<int> &workers);
void update_gpu_simulation(Context &ctx, const eastl::vector<int> &workers);

inline uint32_t get_render_tag(Shaders &dst, const eastl::string &tag, uint32_t max_tags)
{
  RenderTagMap::iterator it = dst.renderTagMap.find(tag);
  return it != dst.renderTagMap.end() ? it->second : max_tags;
}
inline const eastl::string &get_render_tag_by_id(Shaders &dst, uint tag)
{
  static eastl::string missing = "";
  RenderTagInverseMap::iterator it = dst.renderTagInverseMap.find(tag);
  return it != dst.renderTagInverseMap.end() ? it->second : missing;
}

template <typename T_group, int T_cpu_buf_type, int T_val>
void sort_cpu_shader_tasks(T_group &groups, eastl::vector<int> &workers)
{
  eastl::quick_sort(workers.begin(), workers.end(), [&](const int &sid0, const int &sid1) {
    const CpuComputeShaderId &s0 = groups.template cget<T_val>(sid0);
    const CpuComputeShaderId &s1 = groups.template cget<T_val>(sid1);
    if (s0 < s1)
      return true;
    if (s0 > s1)
      return false;

    const CpuBuffer &p0 = groups.template get<T_cpu_buf_type>(sid0);
    const CpuBuffer &p1 = groups.template get<T_cpu_buf_type>(sid1);
    if (p0.pageId < p1.pageId)
      return true;
    if (p0.pageId > p1.pageId)
      return false;

    return p0.offset < p1.offset;
  });
}

template <typename T_ctx, typename T_state, int T_BUFFER, int T_SHADER>
void sort_gpu_shader_tasks(T_ctx &ctx, eastl::vector<int> &workers)
{
  eastl::quick_sort(workers.begin(), workers.end(), [&](const int &sid0, const int &sid1) {
    const GpuComputeShaderId s0 = ctx.instances.groups.template cget<T_SHADER>(sid0);
    const GpuComputeShaderId s1 = ctx.instances.groups.template cget<T_SHADER>(sid1);
    if (s0 < s1)
      return true;
    if (s0 > s1)
      return false;

    GpuPageId p0 = ctx.instances.groups.template cget<T_BUFFER>(sid0).pageId;
    GpuPageId p1 = ctx.instances.groups.template cget<T_BUFFER>(sid1).pageId;
    if (p0 < p1)
      return true;
    if (p0 > p1)
      return false;

    int c0 = ctx.instances.groups.template cget<T_state>(sid0).count;
    int c1 = ctx.instances.groups.template cget<T_state>(sid1).count;

    return c0 < c1;
  });
}
} // namespace dafx