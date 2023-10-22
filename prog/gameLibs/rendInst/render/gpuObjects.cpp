#include "gpuObjects.h"

#include <rendInst/gpuObjects.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtraRender.h>

#include "visibility/genVisibility.h"

#include <gpuObjects/placingParameters.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <shaders/dag_computeShaders.h>


namespace rendinst::gpuobjects
{

static eastl::unique_ptr<gpu_objects::GpuObjects> manager;
static dag::Vector<GpuObjectsEntry> gpu_objects_to_add;

static int useRiTrackdirtOffsetVarId = -1;


void startup()
{
  manager = nullptr;
  manager = eastl::make_unique<gpu_objects::GpuObjects>();

  useRiTrackdirtOffsetVarId = ::get_shader_glob_var_id("use_ri_trackdirt_offset", true);
}
void shutdown() { manager = nullptr; }
void after_device_reset()
{
  if (manager)
    manager->invalidate();
}

void clear_all()
{
  if (manager)
    manager->clearObjects();
}

void rebuild_gpu_instancing_relem_params()
{
  if (manager)
    manager->rebuildGpuInstancingRelemParams();
}

bool has_pending() { return !gpu_objects_to_add.empty(); }

void add(const String &name, int cell_tile, int grid_size, float cell_size, const gpu_objects::PlacingParameters &parameters)
{
  if (manager)
  {
    if (!isRiExtraLoaded())
    {
      GpuObjectsEntry entry = {name, cell_tile, grid_size, cell_size, parameters};
      gpu_objects_to_add.emplace_back(entry);
      return;
    }
    int id = rendinst::riExtraMap.getNameId(name);
    if (id == -1)
    {
      debug("GPUObjects: auto adding <%s> as riExtra.", name);
      id = addRIGenExtraResIdx(name, -1, -1, AddRIFlag::UseShadow);
      if (id < 0)
        return;
    }

    float boundingSphereRadius = rendinst::riExtra[id].sphereRadius;
    dag::ConstSpan<float> distSqLod(rendinst::riExtra[id].distSqLOD, rendinst::riExtra[id].res->lods.size());
    manager->addObject(name.c_str(), id, cell_tile, grid_size, cell_size, boundingSphereRadius, distSqLod, parameters);
    gpu_objects::PlacingParameters nodeBasedMetadataAppliedParams;
    // if node based shader has metadata block with parameters, this values are used instead of stored in entity description
    manager->getParameters(id, nodeBasedMetadataAppliedParams);
    manager->getCellData(id, cell_tile, grid_size, cell_size);
    rendinst::riExtra[id].radiusFade = grid_size * cell_size * 0.5;
    rendinst::riExtra[id].radiusFadeDrown = 0.5;
    rendinst::riExtra[id].hardness = clamp(1.0f - nodeBasedMetadataAppliedParams.hardness, 0.0f, 1.0f);
    rendinst::render::update_per_draw_gathered_data(id);
  }
}

void erase_inside_sphere(const Point3 &center, const float radius)
{
  if (!manager)
    return;

  manager->addBombHole(center, radius);
  d3d::GpuAutoLock gpu_al;
  BBox2 bbox{center.x - radius, center.z + radius, center.x + radius, center.z - radius};
  manager->invalidateBBox(bbox);
}

void update(const Point3 &origin)
{
  if (!manager)
    return;

  if (isRiExtraLoaded())
    for (auto it = gpu_objects_to_add.rbegin(); it != gpu_objects_to_add.rend(); ++it)
    {
      add(it->name, it->grid_tile, it->grid_size, it->cell_size, it->parameters);
      gpu_objects_to_add.erase(it);
    }
  manager->update(origin);
}

void invalidate()
{
  if (manager)
    manager->invalidate();
}

void before_draw(RenderPass render_pass, const RiGenVisibility *visibility, const Frustum &frustum, const Occlusion *occlusion,
  const char *mission_name, const char *map_name, bool gpu_instancing)
{
  if (manager && visibility && visibility->gpuObjectsCascadeId != -1)
    manager->beforeDraw(render_pass, visibility->gpuObjectsCascadeId, frustum, occlusion, mission_name, map_name, gpu_instancing);
}

void validate_displaced(float displacement_tex_range)
{
  if (manager)
    manager->validateDisplacedGPUObjs(displacement_tex_range);
}

void change_parameters(const String &name, const gpu_objects::PlacingParameters &parameters)
{
  d3d::GpuAutoLock lock;
  if (manager)
    manager->changeParameters(rendinst::riExtraMap.getNameId(name), parameters);
}

void change_grid(const String &name, int cell_tile, int grid_size, float cell_size)
{
  d3d::GpuAutoLock lock;
  if (manager)
    manager->changeGrid(rendinst::riExtraMap.getNameId(name), cell_tile, grid_size, cell_size);
}

void invalidate_inside_bbox(const BBox2 &bbox)
{
  if (!manager)
    return;

  d3d::GpuAutoLock lock;
  manager->invalidateBBox(bbox);
}

CONSOLE_BOOL_VAL("render", gpu_objects_enable, true);

void render_optimization_depth(RenderPass render_pass, const RiGenVisibility *visibility,
  IgnoreOptimizationLimits ignore_optimization_instances_limits, uint32_t instance_count_mul)
{
  if (gpuobjects::manager->getGpuInstancing(visibility[0].gpuObjectsCascadeId))
    return;

  rendinst::render::renderRIGenExtraFromBuffer(gpuobjects::manager->getBuffer(visibility[0].gpuObjectsCascadeId, LayerFlag::Opaque),
    manager->getObjectsOffsetsAndCount(visibility[0].gpuObjectsCascadeId, LayerFlag::Opaque),
    manager->getObjectsIds(visibility[0].gpuObjectsCascadeId, LayerFlag::Opaque),
    manager->getObjectsLodOffsets(visibility[0].gpuObjectsCascadeId, LayerFlag::Opaque), render_pass, OptimizeDepthPass::Yes,
    OptimizeDepthPrepass::Yes, ignore_optimization_instances_limits, LayerFlag::Opaque, nullptr, instance_count_mul);
}

void render_layer(RenderPass render_pass, const RiGenVisibility *visibility, LayerFlags layer_flags, LayerFlag layer)
{
  if (visibility[0].gpuObjectsCascadeId != -1 && gpu_objects_enable.get())
  {
    ShaderGlobal::set_int(useRiTrackdirtOffsetVarId, 1);
    // TODO: this seems nonsensical, why would we need both layer flags and some particular layer?
    // Possibly some obscure corner-case is involved.
    rendinst::render::renderRIGenExtraFromBuffer(manager->getBuffer(visibility[0].gpuObjectsCascadeId, layer),
      manager->getObjectsOffsetsAndCount(visibility[0].gpuObjectsCascadeId, layer),
      manager->getObjectsIds(visibility[0].gpuObjectsCascadeId, layer),
      manager->getObjectsLodOffsets(visibility[0].gpuObjectsCascadeId, layer), render_pass,
      !(layer_flags & LayerFlag::ForGrass) ? OptimizeDepthPass::Yes : OptimizeDepthPass::No, OptimizeDepthPrepass::No,
      IgnoreOptimizationLimits::No, (layer_flags & layer) ? layer : LayerFlag{}, nullptr, 1,
      manager->getGpuInstancing(visibility[0].gpuObjectsCascadeId), manager->getIndirectionBuffer(visibility[0].gpuObjectsCascadeId),
      manager->getOffsetsBuffer(visibility[0].gpuObjectsCascadeId));
    ShaderGlobal::set_int(useRiTrackdirtOffsetVarId, 0);
  }
}

void enable_for_visibility(RiGenVisibility *visibility)
{
  if (manager)
    visibility[0].gpuObjectsCascadeId = visibility[1].gpuObjectsCascadeId = manager->addCascade();
}

void disable_for_visibility(RiGenVisibility *visibility)
{
  if (manager && visibility[0].gpuObjectsCascadeId != -1)
    manager->releaseCascade(visibility[0].gpuObjectsCascadeId);
  visibility[0].gpuObjectsCascadeId = visibility[1].gpuObjectsCascadeId = -1;
}

void clear_from_visibility(RiGenVisibility *visibility)
{
  if (manager && visibility[0].gpuObjectsCascadeId != -1)
    manager->clearCascade(visibility[0].gpuObjectsCascadeId);
}

} // namespace rendinst::gpuobjects

static bool ri_gpu_objects_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("ri_gpu_objects", "invalidate", 1, 1)
  {
    d3d::GpuAutoLock lock;
    if (rendinst::gpuobjects::manager)
      rendinst::gpuobjects::manager->invalidate();
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(ri_gpu_objects_console_handler);
