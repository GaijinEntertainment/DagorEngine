//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <generic/dag_staticTab.h>
#include <math/dag_hlsl_floatx.h>
#include "projective_decals_const.hlsli"
#include "projective_decals.hlsli"
#include "projectiveDecalsBuffer.h"
#include <projectiveDecals/projectiveDecalData.h>
#include <EASTL/vector.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>

struct Frustum;
class Point3;

using ComputeShaderPtr = eastl::unique_ptr<ComputeShaderElement>;
class ProjectiveDecalsBase
{
public:
  ProjectiveDecalsBase() = default;
  ~ProjectiveDecalsBase();

  ProjectiveDecalsBase(const ProjectiveDecalsBase &) = delete;
  ProjectiveDecalsBase &operator=(const ProjectiveDecalsBase &) = delete;
  ProjectiveDecalsBase(ProjectiveDecalsBase &&) = delete;
  ProjectiveDecalsBase &operator=(ProjectiveDecalsBase &&) = delete;

  void close();
  bool init(const char *generator_shader_name, const char *decal_shader_name, const char *decal_cull_shader,
    const char *decal_create_indirect, size_t update_struct_size);

  void init_buffer(uint32_t initial_size, size_t instance_struct_size)
  {
    buffer = DecalsBuffer(initial_size, "decalInstances",
      String(0, "decal_buffer__%s", decalRenderer.shader->getShaderClassName()).c_str(), instance_struct_size);
  }

  void updateDecal(dag::ConstSpan<Point4> decal_data);
  dag::Span<Point4> getSrcData(uint32_t decal_id);

  void prepareRender(const Frustum &frustum); // generate decals and frustum culling. Call it earlier, to hide latency
  void render();
  void afterReset();

  void clear(); // clear current decals


  unsigned getDecalsNum() const { return decalsToRender; }

protected:
  DecalsBuffer buffer;

  DynamicShaderHelper decalRenderer;
  ComputeShaderPtr decalUpdater;
  ComputeShaderPtr decalCuller;
  ComputeShaderPtr decalCreator;

  Tab<Point4> decalsToUpdateData;
  Sbuffer *decalsToUpdateBuf = nullptr;
  BufPtr indirectBuffer;
  uint32_t decalsToRender = 0;
  int updateLengthPerInstance = 0;

  void setUpIndirectBuffer();
  void setDecalId(dag::Span<Point4> decal_data, uint32_t decal_id);
  uint32_t getDecalId(dag::ConstSpan<Point4> decal_data);

private:
  int findDecal(uint32_t decal_id);
};

class ResizableDecalsBase : public ProjectiveDecalsBase
{
private:
  dag::Vector<uint32_t> freeDecalIds;
  uint32_t bufferSize = 0;
  int decalsCount = 0;
  int allocate_decal_id(DecalsBuffer &buffer);
  // unsafe for double removing one decal
  void free_decal_id(int decal_id);

public:
  void init_buffer(uint32_t initial_size, size_t instance_struct_size);
  void clear();
  int addDecal(dag::Span<Point4> decal_data);
  void removeDecal(dag::ConstSpan<Point4> decal_data);
};


class RingBufferDecalsBase : public ProjectiveDecalsBase
{
  unsigned int ringBufferIndex = 0, ringBufferCapacity;

public:
  void init_buffer(uint32_t maximum_size, size_t instance_struct_size);

  int allocate_decal_id();

  void clear();

  int addDecal(dag::Span<Point4> decal_data);
};


template <typename InstanceStruct, typename UpdateStruct>
class ResizableDecalsManagerBase
{
private:
  ResizableDecalsBase resizeableDecals;
  static constexpr int UPDATE_SPAN_LENGTH = sizeof(UpdateStruct) / sizeof(Point4);
  dag::ConstSpan<Point4> convertDataToConstSpan(const UpdateStruct &decal_data)
  {
    return make_span_const(reinterpret_cast<const Point4 *>(&decal_data), UPDATE_SPAN_LENGTH);
  }
  dag::Span<Point4> convertDataToSpan(UpdateStruct &decal_data)
  {
    return make_span(reinterpret_cast<Point4 *>(&decal_data), UPDATE_SPAN_LENGTH);
  }

public:
  void close() { resizeableDecals.close(); }
  void clear() { resizeableDecals.clear(); }
  bool init(const char *generator_shader_name, const char *decal_shader_name, const char *decal_cull_shader,
    const char *decal_create_indirect)
  {
    static_assert(std::is_base_of<DecalDataBase, UpdateStruct>::value, "UpdateStruct must inherit from DecalDataBase");
    static_assert(sizeof(UpdateStruct) % sizeof(Point4) == 0, "UpdateStruct size must be multiples of Point4");
    static_assert(sizeof(InstanceStruct) % sizeof(Point4) == 0, "InstanceStruct size must be multiples of Point4");
    return resizeableDecals.init(generator_shader_name, decal_shader_name, decal_cull_shader, decal_create_indirect,
      sizeof(UpdateStruct));
  }

  void init_buffer(uint32_t initial_size) { resizeableDecals.init_buffer(initial_size, sizeof(InstanceStruct)); }
  void updateDecal(const UpdateStruct &decal_data) { resizeableDecals.updateDecal(convertDataToConstSpan(decal_data)); }

  // Decal Id will be overwritten using the setAllocatedId
  int addDecal(UpdateStruct &decal_data) { return resizeableDecals.addDecal(convertDataToSpan(decal_data)); }

  // Todo: Remove this path once we have time
  void partialUpdate(const UpdateStruct &decal_data)
  {
    dag::Span<Point4> buffData = resizeableDecals.getSrcData(decal_data.decal_id);
    if (!buffData.empty())
      decal_data.partialUpdate(buffData);
    else
      resizeableDecals.updateDecal(convertDataToConstSpan(decal_data));
  }

  void removeDecal(const UpdateStruct &decal_data) { return resizeableDecals.removeDecal(convertDataToConstSpan(decal_data)); };
  void prepareRender(const Frustum &frustum) { resizeableDecals.prepareRender(frustum); }
  void render() { resizeableDecals.render(); }
  void afterReset() { resizeableDecals.afterReset(); }
  unsigned getDecalsNum() const { return resizeableDecals.getDecalsNum(); }
};


template <typename InstanceStruct, typename UpdateStruct>
class RingBufferDecalsManagerBase
{
private:
  RingBufferDecalsBase ringBuffDecals;
  static constexpr int UPDATE_SPAN_LENGTH = sizeof(UpdateStruct) / sizeof(Point4);
  dag::ConstSpan<Point4> convertDataToConstSpan(const UpdateStruct &decal_data)
  {
    return make_span_const(reinterpret_cast<const Point4 *>(&decal_data), UPDATE_SPAN_LENGTH);
  }
  dag::Span<Point4> convertDataToSpan(UpdateStruct &decal_data)
  {
    return make_span(reinterpret_cast<Point4 *>(&decal_data), UPDATE_SPAN_LENGTH);
  }

public:
  void close() { ringBuffDecals.close(); }
  void clear() { ringBuffDecals.clear(); }
  bool init(const char *generator_shader_name, const char *decal_shader_name, const char *decal_cull_shader,
    const char *decal_create_indirect)
  {
    static_assert(std::is_base_of<DecalDataBase, UpdateStruct>::value, "UpdateStruct must inherit from DecalDataBase");
    // DecalDataInterface has size 8 because C++ doesn't allow for empty structs
    static_assert(sizeof(UpdateStruct) % sizeof(Point4) == 0, "UpdateStruct size must be multiples of Point4");
    static_assert(sizeof(InstanceStruct) % sizeof(Point4) == 0, "InstanceStruct size must be multiples of Point4");
    return ringBuffDecals.init(generator_shader_name, decal_shader_name, decal_cull_shader, decal_create_indirect,
      sizeof(UpdateStruct));
  }

  void init_buffer(uint32_t initial_size) { ringBuffDecals.init_buffer(initial_size, sizeof(InstanceStruct)); }

  void updateDecal(const UpdateStruct &decal_data) { ringBuffDecals.updateDecal(convertDataToConstSpan(decal_data)); }

  // Decal Id will be overwritten using the setAllocatedId
  int addDecal(UpdateStruct &decal_data) { return ringBuffDecals.addDecal(convertDataToSpan(decal_data)); }

  void prepareRender(const Frustum &frustum) { ringBuffDecals.prepareRender(frustum); }
  void render() { ringBuffDecals.render(); }
  void afterReset() { ringBuffDecals.afterReset(); }
  unsigned getDecalsNum() const { return ringBuffDecals.getDecalsNum(); }
};

using RingBufferDecalManager = RingBufferDecalsManagerBase<ProjectiveDecalInstance, DefaultDecalData>;
using ResizableDecalManager = ResizableDecalsManagerBase<ProjectiveDecalInstance, DefaultDecalData>;