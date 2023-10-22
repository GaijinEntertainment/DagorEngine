#pragma once

#include <rendInst/rendInstExtra.h>
#include <rendInst/riexHandle.h>
#include <rendInst/riExtraRenderer.h>
#include "riGen/riExtraPool.h"
#include "riGen/rendInstTiledScene.h"

#include <vecmath/dag_vecMathDecl.h>
#include <osApiWrappers/dag_rwLock.h>
#include <util/dag_oaHashNameMap.h>
#include <scene/dag_physMat.h>
#include <dag/dag_vector.h>
#include <3d/dag_texStreamingContext.h>
#include <util/dag_multicastEvent.h>


struct MaterialRayStrat
{
  PhysMat::MatID rayMatId;
  bool processPosRI = false;

  MaterialRayStrat(PhysMat::MatID ray_mat, bool process_pos_ri = false) : rayMatId(ray_mat), processPosRI(process_pos_ri) {}

  bool shouldIgnoreRendinst(bool is_pos, bool /* is_immortal */, PhysMat::MatID mat_id) const
  {
    if (is_pos && !processPosRI)
      return true;

    if (rayMatId == PHYSMAT_INVALID)
      return false;

    return !PhysMat::isMaterialsCollide(rayMatId, mat_id);
  }

  inline bool isCollisionRequired() const { return true; }
};


class RenderableInstanceLodsResource;
class ShaderElement;

namespace rendinst::render
{
struct VbExtraCtx;
}

namespace rendinst
{
inline constexpr int RIEXTRA_VECS_COUNT = 4;

class RiExtraPoolsVec : public dag::Vector<RiExtraPool> // To consider: move this methods to dag::Vector?
{
public:
  using dag::Vector<RiExtraPool>::Vector;
  uint32_t size_interlocked() const;
  uint32_t interlocked_increment_size();
};
extern RiExtraPoolsVec riExtra;
extern FastNameMap riExtraMap;

extern SmartReadWriteFifoLock ccExtra;
extern int maxExtraRiCount;

// 0th is main, 1th is for async opaque render
inline constexpr size_t RIEX_RENDERING_CONTEXTS = 2;

extern int maxRenderedRiEx[RIEX_RENDERING_CONTEXTS];
extern int perDrawInstanceDataBufferType;
extern int instancePositionsBufferType;
extern float riExtraLodDistSqMul;
extern float riExtraCullDistSqMul;
extern Tab<uint16_t> riExPoolIdxPerStage[RIEX_STAGE_COUNT];

void initRIGenExtra(bool need_render = true, const DataBlock *level_blk = nullptr);
void optimizeRIGenExtra();
void termRIGenExtra();

bool isRIGenExtraObstacle(const char *nm);
bool isRIGenExtraUsedInDestr(const char *nm);
bool rayHitRIGenExtraCollidable(const Point3 &p0, const Point3 &norm_dir, float len, rendinst::RendInstDesc &ri_desc,
  const MaterialRayStrat &strategy, float min_r);
void reapplyLodRanges();
void addRiExtraRefs(DataBlock *b, const DataBlock *riConf, const char *name);

enum
{
  DYNAMIC_SCENE = 0,
  STATIC_SCENES_START = 1
}; // first tiled scene is for dynamic objects
typedef TiledScenesGroup<4> RITiledScenes;
extern RITiledScenes riExTiledScenes;
extern float riExTiledSceneMaxDist[RITiledScenes::MAX_COUNT];

struct PerInstanceParameters
{
  uint32_t materialOffset;
  uint32_t matricesOffset;
};

void init_tiled_scenes(const DataBlock *level_blk);
void term_tiled_scenes();
void add_ri_pool_to_tiled_scenes(RiExtraPool &pool, int pool_id, const char *name, float max_lod_dist);
scene::node_index alloc_instance_for_tiled_scene(RiExtraPool &pool, int pool_idx, int inst_idx, mat44f &tm44, vec4f &out_sphere);
void move_instance_in_tiled_scene(RiExtraPool &pool, int pool_idx, scene::node_index &nodeId, mat44f &tm44, bool do_not_wait);
void move_instance_to_original_scene(rendinst::RiExtraPool &pool, int pool_idx, scene::node_index &nodeId);
void set_user_data(rendinst::riex_render_info_t, uint32_t start, const uint32_t *add_data, uint32_t count);
void set_cas_user_data(rendinst::riex_render_info_t, uint32_t start, const uint32_t *was_data, const uint32_t *new_data,
  uint32_t count);
const mat44f &get_tiled_scene_node(rendinst::riex_render_info_t);
void remove_instance_from_tiled_scene(scene::node_index nodeId);
void init_instance_user_data_for_tiled_scene(rendinst::RiExtraPool &pool, scene::node_index ni, vec4f sphere, int add_data_dwords,
  const int32_t *add_data);
void set_instance_user_data(scene::node_index ni, int add_data_dwords, const int32_t *add_data);
const uint32_t *get_user_data(rendinst::riex_handle_t);
} // namespace rendinst

DAG_DECLARE_RELOCATABLE(rendinst::RiExtraPool);
