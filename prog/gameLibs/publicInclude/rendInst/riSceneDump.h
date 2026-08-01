//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <vecmath/dag_vecMathDecl.h>

class IGenSave;
class Occlusion;

// On-disk format for the riExtra scene dump written by rendinst::dumpAllScenes
// (prog/gameLibs/rendInst/dumpScenes.cpp), consumed by the offline scene-representation
// benchmark. It is a full, reconstructable snapshot of the current riExtra tiled scenes plus
// the per-pool object metadata, captured from a running game via the `ri.dump_scenes` console
// command. The tiling/kdtree structures are intentionally NOT stored: they are derived from the
// node list and are rebuilt on load (see scene::load_scene), so persisting them would only pin the
// benchmark to today's layout - the whole point of the dump is to feed alternative layouts the
// same input.
//
// The magic has its MSB set so a stale/foreign file desyncs on the first word instead of loading
// as a plausible-but-wrong scene. Keep MAGIC/VERSION and every record layout below in sync with
// the writer. All records are raw little-endian POD; vector types (vec4f/bbox3f/mat44f) are written
// with their natural size (including any tail padding of the enclosing struct), so a reader that
// includes this header and reads sizeof(struct) stays byte-compatible with the writer.
//
// File layout:
//   [uint32 MAGIC][int32 VERSION]
//   [int32 sceneCount][int32 poolCount]
//   sceneCount * RiSceneDumpSceneConfig                       // group-level per-scene config
//   poolCount  * { RiSceneDumpPoolMeta head; char name[head.nameLen]; }  // object metadata
//   [int32 tiledPoolCount][int32 elemSize]  tiledPoolCount * TiledScenePoolInfo  // group scene-pool info
//   [int32 boxOccluderCount][int32 elemSize] boxOccluderCount * bbox3f           // group box occluders
//   [int32 quadOccluderCount][int32 elemSize] quadOccluderCount * mat44f         // group quad occluders
// (Each group array is prefixed with its element size so a reader can skip it as count*elemSize bytes
//  without depending on the private TiledScenePoolInfo layout.)
//   sceneCount * per-scene node block:                        // one block per scene, in order
//       [float tileSize]
//       [int32 poolBoxCount]
//         poolBoxCount * { bbox3f poolBox; float sphereVerticalCenter; }
//       [int32 aliveNodeCount]
//         aliveNodeCount * mat44f node
//
// A scene node's mat44f carries everything a culler needs: the 4x3 transform, plus in the .w lanes
// the pool index | (flags << 16) (col2.w), the bounding-sphere vertical-center factor (col1.w), the
// bounding-sphere radius (col3.w) and the squared disappear distance (col0.w). poolBox[].bmin.w is
// the pool distance-scale-squared and poolBox[].bmax.w is the pool sphere radius. So a scene can be
// rebuilt by allocating each node under scene::get_node_pool/flags and restoring poolBox/dist scale.
static constexpr uint32_t RI_SCENE_DUMP_MAGIC = 0xD05CE4E5u; // 'scenes', MSB set
static constexpr int RI_SCENE_DUMP_VERSION = 1;

// Bits packed into RiSceneDumpPoolMeta::flags. These mirror the corresponding rendinst::RiExtraPool
// booleans at dump time; they are metadata for analysis (e.g. splitting trees/impostors/walls into
// their own layout), not required to reconstruct culling.
enum RiSceneDumpPoolFlag : uint32_t
{
  RI_SDPF_POS_INST = 1u << 0,        // posInst (impostor / point instance)
  RI_SDPF_IS_TREE = 1u << 1,         // isTree (vertex-animated bbox extension)
  RI_SDPF_IMMORTAL = 1u << 2,        // immortal (never destroyed)
  RI_SDPF_DYNAMIC = 1u << 3,         // isDynamicRendinst (lives in the dynamic scene)
  RI_SDPF_HAS_OCCLUDER = 1u << 4,    // hasOccluder
  RI_SDPF_LARGE_OCCLUDER = 1u << 5,  // largeOccluder
  RI_SDPF_IS_WALLS = 1u << 6,        // isWalls
  RI_SDPF_USING_CLIPMAP = 1u << 7,   // usingClipmap
  RI_SDPF_IS_GRASSIFY = 1u << 8,     // isGrassify
  RI_SDPF_UNDERWATER_ONLY = 1u << 9, // underwaterOnly
  RI_SDPF_USE_SHADOW = 1u << 10,     // useShadow
  RI_SDPF_PATCHES_HMAP = 1u << 11,   // patchesHeightmap
};

// Group-level per-scene config. maxDist/maxSize are the thresholds used to pick a pool's scene
// (rendinst::riExTiledSceneMaxDist / riExTiledSceneMaxSize); nodeCount is informational.
struct RiSceneDumpSceneConfig
{
  float tileSize;
  float maxDist;
  float maxSize;
  uint32_t nodeCount;
};

// Fixed head of one pool (represented object) record. Followed immediately by `nameLen` raw name
// bytes (no terminator). Vector members are grouped first so the struct has no interior padding.
struct RiSceneDumpPoolMeta
{
  bbox3f lbb;            // local-space bbox
  bbox3f collBb;         // collision bbox
  bbox3f fullWabb;       // full world-aligned bbox (spans all instances)
  vec4f bsphXYZR;        // bounding sphere: xyz center, w radius
  float distSqLOD[8];    // squared LOD switch distances (RI_MAX_LODS == 8)
  uint32_t lodLimits;    // packed LOD limits
  int32_t tsIndex;       // tiled scene this pool's instances live in (-1 == none/never rendered)
  uint32_t flags;        // RiSceneDumpPoolFlag bits
  float sphereRadius;    // RiExtraPool::sphereRadius
  float sphCenterY;      // RiExtraPool::sphCenterY
  int32_t instanceCount; // riTm.size() (total instances, including destroyed slots)
  uint32_t nameLen;      // bytes of name that follow this record
};


// --- Culling reference -----------------------------------------------------------------------
// A ground-truth snapshot of what the CURRENT scenes cull to, for a captured camera + HZB. Written
// by rendinst::dumpCullingReference (paired with a save_occlusion() dump that holds the same camera
// and HZB). The offline benchmark reproduces these counts/hashes with each candidate representation:
// the counts catch gross under/over-culling and the hashes catch a wrong *set* at the same count.
//
// The per-scene visible-node hash is an order-INDEPENDENT additive combine of a hash of each visible
// node's transform (ri_scene_dump_hash_node below), so a candidate that visits nodes in a different
// traversal order - or assigns different node indices after reconstruction - still matches as long as
// it returns the same set of instances. Keying on the transform (not the node index) is what makes the
// hash comparable across representations, since node indices are an artifact of allocation order.
//
// File layout:
//   [uint32 MAGIC][int32 VERSION]
//   [int32 sceneCount]
//   [mat44f globtm][vec4f vpos_distscale]     // exact cull inputs (globtm may differ from occlusion
//                                             //   camera under a frustum-stop; vpos_distscale.w is
//                                             //   the global distance-scale multiplier)
//   sceneCount * RiCullRefSceneRec            // one per scene, in scene order
static constexpr uint32_t RI_CULLREF_DUMP_MAGIC = 0xD0C0114Fu; // 'cull-ref', MSB set
// v2: visible-node hash keys on the 4x3 transform + pool index (reconstruction-stable). v1 hashed the
// whole node mat44f, whose derived .w lanes (bsphere/dist) do not survive reconstruction bit-for-bit, so
// v1 hashes never validated. The record layout is unchanged, so a reader can still parse v1 for counts.
static constexpr int RI_CULLREF_DUMP_VERSION = 2;

struct RiCullRefSceneRec
{
  uint32_t totalNodes;             // alive nodes (== visible with no culling at all)
  uint32_t frustumVisible;         // pass frustum + distance cull (no occlusion)
  uint32_t frustumOccludedVisible; // pass frustum + distance + HZB occlusion cull
  uint32_t _pad;
  uint64_t frustumVisibleHash;  // additive combine of ri_scene_dump_hash_node over frustum-visible nodes
  uint64_t occludedVisibleHash; // additive combine of ri_scene_dump_hash_node over frustum+occlusion-visible nodes
};

// Reconstruction-stable identity of a scene node: a hash of its 4x3 world transform (the xyz of each
// column, which scene allocation stores verbatim) plus the pool index. The writer (reference cull) and
// the offline benchmark MUST use this exact function so their additively-combined per-scene hashes are
// comparable. The four .w lanes are deliberately EXCLUDED: pool|flags (col2.w), bsphere radius (col3.w),
// vertical-center factor (col1.w) and disappear-dist (col0.w) are derived caches recomputed on load from
// pool state (see scene::SimpleScene::reallocate), so they can differ bit-for-bit from the live node
// without changing the visible set - and the flags half of col2.w is per-frame volatile. Only the pool
// index (low half of col2.w) is included, as the stable "which asset" part of identity.
inline uint64_t ri_scene_dump_hash_node(mat44f_cref node)
{
  const uint32_t *w = reinterpret_cast<const uint32_t *>(&node); // mat44f == 4 contiguous columns of {x,y,z,w}
  const int transformWords[12] = {0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14};
  uint64_t h = 0xCBF29CE484222325ull; // FNV-1a offset basis
  for (int i = 0; i < 12; ++i)
  {
    h ^= w[transformWords[i]];
    h *= 0x100000001B3ull; // FNV-1a prime
  }
  h ^= (w[11] & 0xFFFFu); // col2.w low half == pool index (high half = flags, volatile, excluded)
  h *= 0x100000001B3ull;
  // splitmix64 finalizer so the additive combine across nodes avoids trivial cancellation
  h += 0x9E3779B97F4A7C15ull;
  h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ull;
  h = (h ^ (h >> 27)) * 0x94D049BB133111EBull;
  return h ^ (h >> 31);
}

namespace rendinst
{
// Dumps all riExtra tiled scenes plus per-pool object metadata (RI_SCENE_DUMP layout above).
void dumpAllScenes(IGenSave &cb);
// Runs the reference cull over every scene for the given camera/occlusion and writes RI_CULLREF_DUMP.
// occl may be null (then only the frustum-only pass is recorded and the occlusion pass mirrors it).
void dumpCullingReference(IGenSave &cb, mat44f_cref globtm, vec4f vpos_distscale, Occlusion *occl);
// Writes the scene dump AND the culling reference under ONE riExtra read lock, so both files see the
// same node set even while other threads mutate riExtra - the property that makes the pair a valid
// benchmark capture. The caller writes any riExtra-independent prefix (save_occlusion) to cull_cb first.
void dumpBenchmarkCapture(IGenSave &scenes_cb, IGenSave &cull_cb, mat44f_cref globtm, vec4f vpos_distscale, Occlusion *occl);
} // namespace rendinst
