// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "levelCollision.h"
#include <debug/dag_log.h>
#include <gameRes/collisionResourceBuilder.h>
#include <ioSys/dag_genIo.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <perfMon/dag_cpuFreq.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/riCollisionDump.h>
#include <util/dag_stlqsort.h>
#include <util/dag_threadPool.h>
#include <EASTL/algorithm.h>

namespace
{
void drop_destroyed(dag::Vector<mat43f> &instances)
{
  instances.erase(
    eastl::remove_if(instances.begin(), instances.end(), [](const mat43f &m) { return !rendinst::riex_is_instance_valid(m); }),
    instances.end());
}

Ptr<CollisionResource> build_mesh_resource(dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint32_t> indices)
{
  bbox3f bbox;
  v_bbox3_init_empty(bbox);
  for (const Point3_vec4 &v : verts)
    v_bbox3_add_pt(bbox, v_ld(&v.x));
  alignas(16) BBox3 bBox;
  v_st(&bBox[0].x, bbox.bmin);
  v_stu_p3(&bBox[1].x, bbox.bmax);
  CollisionResourceBuilder builder;
  builder.addMeshNode("mesh", -1, TMatrix::IDENT, bBox, verts, indices);
  builder.recomputeBounds();
  builder.collapse("mesh");
  return Ptr<CollisionResource>(builder.build("mesh"));
}

// a batch is built on workers while the next one is read; added in dump order
struct RiCollisionsReader
{
  static constexpr uint32_t BATCH = 256;
  struct Record final : cpujobs::IJob
  {
    dag::Vector<Point3_vec4> verts;
    dag::Vector<uint32_t> indices;
    dag::Vector<mat43f> instances;
    Ptr<CollisionResource> res;
    void doJob() override { res = build_mesh_resource(verts, indices); }
    const char *getJobName(bool &) const override { return DAPROFILER_STRING("level_collision_record"); }
  };
  LevelCollision &level;
  dag::Vector<Record> batch, building;
  dag::Vector<uint16_t> indices16;
  dag::Vector<Point3> verts;
  ~RiCollisionsReader() { place(); }

  bool wantMesh(int, bool)
  {
    batch.push_back();
    return true;
  }
  void *indexBuffer(int count, bool wide)
  {
    if (wide)
    {
      batch.back().indices.resize(count);
      return batch.back().indices.data();
    }
    indices16.resize(count);
    return indices16.data();
  }
  Point3 *vertexBuffer(int count)
  {
    verts.resize(count);
    return verts.data();
  }
  mat43f *instanceBuffer(int count)
  {
    batch.back().instances.resize(count);
    return batch.back().instances.data();
  }
  void endMesh(int, bool wide, int index_count, int vert_count, int)
  {
    Record &r = batch.back();
    if (!wide)
      r.indices.assign(indices16.begin(), indices16.begin() + index_count);
    r.verts.resize(vert_count);
    for (int i = 0; i < vert_count; ++i)
      r.verts[i] = verts[i];
    if (batch.size() >= BATCH)
      submitBatch();
  }
  // the batch under build is placed first, so the records keep the dump order; the destructor places the last one on any exit
  void submitBatch()
  {
    place();
    for (Record &r : batch)
      threadpool::add(&r, threadpool::PRIO_HIGH, false);
    threadpool::wake_up_all();
    eastl::swap(batch, building);
  }
  void place()
  {
    for (Record &r : building)
    {
      threadpool::wait(&r, 0, threadpool::PRIO_HIGH);
      drop_destroyed(r.instances);
      level.addResource(eastl::move(r.res), eastl::move(r.instances));
    }
    building.clear();
  }
};
} // namespace

LevelCollision::LevelCollision(float tile_size) : tileSize(tile_size) { clear(); }

void LevelCollision::clear()
{
  scene.term();
  scene.init(tileSize);
  resources.clear();
  names.clear();
  resFirst.assign(1, 0);
  tms.clear();
  spheres.clear();
  poolFirst.clear();
  poolResource.clear();
  v_bbox3_init_empty(box);
  mats.clear();
  lightTransparentMats.resize(0);
}

bool LevelCollision::load(IGenLoad &cb)
{
  clear();
  const int64_t reft = ref_time_ticks();
  const bool collres = cb.getTargetDataSize() >= (int)sizeof(uint32_t) && (uint32_t)cb.readInt() == COLLRES_DUMP_MAGIC;
  cb.seekto(0);
  const bool ok = collres ? loadCollRes(cb) : loadRiCollisions(cb);
  if (!ok)
  {
    logerr("level collision: <%s> did not load", cb.getTargetName());
    clear();
  }
  const int loadUsec = get_time_usec(reft);
  finalize();
  if (ok)
    debug("level collision: <%s> %s, %u resources, %u instances, loaded in %d ms, finalized in %d ms", cb.getTargetName(),
      collres ? "collres" : "ri_collisions", resourceCount(), instanceCount(), loadUsec / 1000,
      (get_time_usec(reft) - loadUsec) / 1000);
  return ok;
}

bool LevelCollision::loadCollRes(IGenLoad &cb)
{
  CollResDump dump;
  if (!load_collres_dump(cb, dump))
    return false;
  uint32_t count = 0;
  for (const CollResDump::Pool &p : dump.pools)
    count += p.instances.size();
  scene.reserve(count);
  tms.reserve(count);
  spheres.reserve(count);
  for (CollResDump::Pool &p : dump.pools)
  {
    drop_destroyed(p.instances);
    const dag::ConstSpan<CollisionNode> nodes = p.collRes->getAllNodes();
    if (p.instances.empty() || eastl::none_of(nodes.begin(), nodes.end(),
                                 [](const CollisionNode &n) { return n.checkBehaviorFlags(CollisionNode::TRACEABLE); }))
      continue;
    addResource(eastl::move(p.collRes), eastl::move(p.instances), p.name.str());
  }
  mats = eastl::move(dump.materials);
  lightTransparentMats.resize(mats.size());
  for (int i = 0, e = mats.size(); i < e; ++i)
    lightTransparentMats.set(i, mats[i].lightTransparent);
  return true;
}

bool LevelCollision::loadRiCollisions(IGenLoad &cb)
{
  RiCollisionsReader reader{*this};
  reader.batch.reserve(RiCollisionsReader::BATCH);
  reader.building.reserve(RiCollisionsReader::BATCH);
  const bool ok = read_ri_collision_dump(cb, reader);
  if (ok) // a parse failure (a torn record, a short tail, an unknown version) discards the batch still pending
    reader.submitBatch();
  return ok;
}

void LevelCollision::addResource(Ptr<CollisionResource> res, dag::Vector<mat43f> &&instances, const char *name)
{
  const uint32_t first = tms.size();
  if (!res)
    instances.clear();
  const uint32_t nodeCapacity = scene.getBaseSimpleScene().getNodesCapacity();
  // the scene grows by a fixed step past 128K nodes: a copy per step
  if (poolFirst.size() < 0xFFFF && first + instances.size() > nodeCapacity)
    scene.reserve(eastl::max<uint32_t>(2 * nodeCapacity, first + instances.size()));
  for (uint32_t c = 0; c < instances.size(); c += POOL_SPAN)
  {
    if (poolFirst.size() >= 0xFFFF) // TiledScene pools stop below 65535
    {
      logerr("level collision: out of scene pools, %d instances of <%s> and all later resources are dropped",
        int(instances.size() - c), name ? name : "");
      instances.resize(c);
      break;
    }
    const uint32_t pool = poolFirst.size();
    const bbox3f resBox = res->vFullBBox;
    poolFirst.push_back(first + c);
    poolResource.push_back(resources.size());
    scene.setPoolBBox(pool, resBox);
    for (uint32_t j = c, je = eastl::min<uint32_t>(c + POOL_SPAN, instances.size()); j < je; ++j)
    {
      mat44f m;
      v_mat43_transpose_to_mat44(m, instances[j]);
      scene.allocate(m, pool, j - c);
      bbox3f worldBox;
      v_bbox3_init(worldBox, m, resBox);
      v_bbox3_add_box(box, worldBox);
      spheres.push_back(v_perm_xyzd(v_bbox3_center(worldBox), v_splat_x(v_bbox3_outer_rad(worldBox))));
    }
  }
  tms.insert(tms.end(), instances.begin(), instances.end());
  resources.push_back(eastl::move(res));
  names.push_back(SimpleString(name));
  resFirst.push_back(tms.size());
}

void LevelCollision::finalize()
{
  scene.flushDeferredTransformUpdates();
  while (!scene.doMaintenance(ref_time_ticks(), 10000000))
    ;
}

void LevelCollision::sortByPool(dag::Span<uint64_t> handles) const
{
  const uint32_t pools = poolFirst.size();
  if (handles.size() <= pools * 2)
  {
    stlsort::sort(handles.begin(), handles.end(), [](uint64_t a, uint64_t b) { return (a >> 32) < (b >> 32); });
    return;
  }
  dag::Vector<uint32_t, framemem_allocator> start(pools + 1, 0);
  for (uint64_t h : handles)
    ++start[(h >> 32) + 1];
  for (uint32_t p = 1; p <= pools; ++p)
    start[p] += start[p - 1];
  dag::Vector<uint64_t, framemem_allocator> sorted(handles.size());
  for (uint64_t h : handles)
    sorted[start[h >> 32]++] = h;
  eastl::copy(sorted.begin(), sorted.end(), handles.begin());
}
