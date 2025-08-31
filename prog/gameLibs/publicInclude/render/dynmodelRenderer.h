//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <3d/dag_textureIDHolder.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_tab.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include <math/dag_declAlign.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix4.h>
#include <shaders/dag_overrideStateId.h>
#include <3d/dag_texStreamingContext.h>
#include <drv/3d/dag_resId.h>
#include <shaders/dag_shaderMesh.h>


#include "dynmodel_consts.hlsli"

class DynamicRenderableSceneInstance;
class DynamicRenderableSceneResource;
class BaseTexture;
class GeomNodeTree;
class ShaderElement;
class ShaderMaterial;

#ifndef INVALID_INST_NODE_ID
#define INVALID_INST_NODE_ID 0
#endif

namespace dynrend
{

struct NodeExtraData
{
  float flt[2];
};

struct InitialNodes
{
  SmallTab<mat44f> nodesModelTm;
  SmallTab<NodeExtraData> extraData;

  InitialNodes() = default;
  InitialNodes(const DynamicRenderableSceneInstance *instance, const GeomNodeTree *initial_skeleton);
  InitialNodes(const DynamicRenderableSceneInstance *instance, const TMatrix &root_tm);
};


enum class ContextId : int
{
  INVALID = -1,
  MAIN = 0,
  IMMEDIATE = 1,
  FIRST_USER_CONTEXT = 2, // User contexts follow.
};


enum RenderFlags
{
  RENDER_OPAQUE = 0x00000001,
  RENDER_DECAL = 0x00000002,
  RENDER_TRANS = 0x00000004,
  RENDER_DISTORTION = 0x00000008,
  APPLY_OVERRIDE_STATE_ID_TO_OPAQUE_ONLY = 0x00000010,
  MERGE_OVERRIDE_STATE = 0x00000020,
  OVERRIDE_RENDER_SKINNED_CHECK = 0x00000040,
};

struct Interval
{
  int varId = 0;
  int setValue = 0;   // Set this value when applying the interval.
  int unsetValue = 0; // Restore this value afterward.
  int instNodeId = INVALID_INST_NODE_ID;
};

typedef StaticTab<Interval, 8> Intervals;
struct BasePerInstanceRenderData
{
  uint32_t flags = RENDER_OPAQUE | RENDER_DECAL | RENDER_TRANS | RENDER_DISTORTION; // For compatibility with deprecated renderer.
  Intervals intervals;
  shaders::OverrideStateId overrideStateId;
  D3DRESID constDataBuf;
};
struct PerInstanceRenderData : public BasePerInstanceRenderData
{
  dag::RelocatableFixedVector<Point4, NUM_GENERIC_PER_INSTANCE_PARAMS, true, framemem_allocator> params;
  PerInstanceRenderData() = default;
  PerInstanceRenderData(const struct AddedPerInstanceRenderData &o);
};
struct AddedPerInstanceRenderData : public BasePerInstanceRenderData
{
  dag::RelocatableFixedVector<Point4, NUM_GENERIC_PER_INSTANCE_PARAMS, false> inplaceParams;
  dag::ConstSpan<Point4> params;
  AddedPerInstanceRenderData() = default;
  AddedPerInstanceRenderData(const PerInstanceRenderData &o, dag::Vector<Point4> &extraParams);
};

struct InstanceContextData
{
  const DynamicRenderableSceneInstance *instance = NULL;
  ContextId contextId = ContextId(-1);
  int instanceOffsetRenderData = -1;
  int nodeOffsetRenderData = -1;
};

struct Statistics
{
  int dips;
  int triangles;
};


void init();
void close();
bool is_initialized();

void set_check_shader_names(bool check);


// Should not be changed prior to the render call:
//    node matrices
//    instance origin
//    opacity of nodes (if used in shader)
//    the instance must not be unloaded
//    optional_initial_nodes must be valid
//    create_context/delete_context are not thread-safe
//
// Can be changed after the add call:
//    instance LOD
//    visibility of nodes (hidden or zero opacity)
//    optional_render_data can be deleted

void add(ContextId context_id, const DynamicRenderableSceneInstance *instance, const InitialNodes *optional_initial_nodes = NULL,
  const dynrend::PerInstanceRenderData *optional_render_data = NULL, dag::Span<int> *node_list = NULL,
  const TMatrix4 *customProj = NULL, bool relative_to_camera = false);

void prepare_render_begin(ContextId context_id, const TMatrix4 &view, const TMatrix4 &proj);
void prepare_render_instances(ContextId context_id, const TMatrix4 &view, const TMatrix4 &proj, int &instanceToChunkOffset,
  const Point3 &offset_to_origin = Point3(0.f, 0.f, 0.f), TexStreamingContext texCtx = TexStreamingContext(0),
  dynrend::InstanceContextData *instanceContextData = NULL);
void prepare_render_finalize(ContextId context_id);

// offset_to_origin is used as a hint to the expected offset of the current view position
// relative to the current origin in view space. The same offset is used for motion vectors calculations as well
// so it is assumed that this offset doesn't change between frames.
void prepare_render(ContextId context_id, const TMatrix4 &view, const TMatrix4 &proj,
  const Point3 &offset_to_origin = Point3(0.f, 0.f, 0.f), TexStreamingContext texCtx = TexStreamingContext(0),
  dynrend::InstanceContextData *instanceContextData = NULL);

void render(ContextId context_id, ShaderMesh::Stage shader_mesh_stage);
void clear(ContextId context_id);

void clear_all_contexts();

void set_reduced_render(ContextId context_id, float min_elem_radius, bool render_skinned);
void set_prev_view_proj(const TMatrix4_vec4 &prev_view, const TMatrix4_vec4 &prev_proj);
void get_prev_view_proj(TMatrix4_vec4 &prev_view, TMatrix4_vec4 &prev_proj);
void set_local_offset_hint(const Point3 &hint);
void enable_separate_atest_pass(bool enable);

ShaderElement *get_replaced_shader();
void replace_shader(ShaderElement *element);

struct ReplacedShaderScope
{
  ReplacedShaderScope(ShaderElement *element)
  {
    G_ASSERTF(get_replaced_shader() == nullptr, "ReplacedShaderScope reentry not allowed!");
    replace_shader(element);
  }

  ~ReplacedShaderScope() { replace_shader(nullptr); }
};

// scoped material filtering
const Tab<const char *> &get_filtered_material_names();
void set_material_filters_by_name(Tab<const char *> &&material_names);
struct MaterialFilterScope
{
  MaterialFilterScope(Tab<const char *> &&material_names)
  {
    G_ASSERTF(get_filtered_material_names().size() == 0, "MaterialFilterScope reentry not allowed!");
    set_material_filters_by_name(std::move(material_names));
  }

  ~MaterialFilterScope() { set_material_filters_by_name({}); }
};

ContextId create_context(const char *name);
ContextId get_or_create_context(const char *name);
void delete_context(ContextId context_id);

void update_reprojection_data(ContextId contextId);

bool set_instance_data_buffer(unsigned stage, ContextId contextId, int node_offset_render_data, int instance_offset_render_data);

const Point4 *get_per_instance_render_data(ContextId contextId, int indexToPerInstanceRenderData);

void verify_is_empty(ContextId context_id);
void render_one_instance(const DynamicRenderableSceneInstance *instance, ShaderMesh::Stage shader_mesh_stage,
  TexStreamingContext texCtx, const InitialNodes *optional_initial_nodes = NULL,
  const dynrend::PerInstanceRenderData *optional_render_data = NULL, bool relative_to_camera = false);
void render_one_instance(const DynamicRenderableSceneInstance *instance, ShaderMesh::Stage shader_mesh_stage, const TMatrix4 &vtm,
  const TMatrix4 &ptm, TexStreamingContext texCtx, const InitialNodes *optional_initial_nodes = NULL,
  const dynrend::PerInstanceRenderData *optional_render_data = NULL, bool relative_to_camera = false);

void opaque_flush(ContextId context_id, TexStreamingContext texCtx, bool include_atest = false);

void set_shaders_forced_render_order(const eastl::vector<eastl::string> &shader_names);

bool can_render(const DynamicRenderableSceneInstance *instance);

Statistics &get_statistics();
void reset_statistics();

using InstanceIterator = void(ContextId, const DynamicRenderableSceneResource &, const DynamicRenderableSceneInstance &,
  const dynrend::AddedPerInstanceRenderData &, const Tab<bool> &, int, float, int, int, int, void *);
void iterate_instances(dynrend::ContextId context_id, InstanceIterator iter, void *user_data);
} // namespace dynrend
