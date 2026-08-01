// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <debug/dag_log.h>
#include <render/lruCollision.h>
#include <daGI2/lruCollisionVoxelization.h>
#include <scene/dag_tiledScene.h>
#include <rendInst/riCollisionDump.h>

// todo: add tiledScene or something
struct LRUCollision
{
  LRUCollisionVoxelization voxelize;

  dag::Vector<eastl::unique_ptr<CollisionResource>> collRes;
  dag::Vector<dag::Vector<mat43f>> instances;
  dag::Vector<dag::Vector<vec4f>> instancesSph;
  dag::Vector<bbox3f> typeBoxes;
  size_t count = 0;
  scene::TiledScene scene;
  size_t size() const { return collRes.size(); }
  size_t addCollRes(const dag::Vector<Point3_vec4> &vertices, const dag::Vector<uint32_t> &indices)
  {
    bbox3f bbox;
    v_bbox3_init_empty(bbox);
    for (auto &v : vertices)
      v_bbox3_add_pt(bbox, v_ld(&v.x));
    alignas(16) BBox3 bBox;
    v_st(&bBox[0].x, bbox.bmin);
    v_stu_p3(&bBox[1].x, bbox.bmax);
    BSphere3 sph;
    sph += bBox;
    eastl::unique_ptr<CollisionResource> coll(CollisionResource::createSingleMesh(vertices, indices, bBox, sph, 0));
    collRes.push_back(eastl::move(coll));
    return collRes.size() - 1;
  }
  void load(IGenLoad &cb)
  {
    // Keeps every record: widens legacy 16-bit indices to uint32, builds a
    // CollisionResource per mesh and the per-instance boxes/spheres.
    struct LoadHandler
    {
      LRUCollision &self;
      dag::Vector<uint32_t> indices;   // wide dest, or legacy widened in endMesh
      dag::Vector<uint16_t> indices16; // legacy read scratch
      dag::Vector<Point3> verts;       // raw read scratch
      dag::Vector<Point3_vec4> verts4; // createSingleMesh input

      bool wantMesh(int, bool) { return true; }
      void *indexBuffer(int count, bool wide)
      {
        if (wide)
        {
          indices.resize(count);
          return indices.data();
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
        auto &inst = self.instances.push_back();
        inst.resize(count);
        return inst.data();
      }
      void endMesh(int, bool wide, int indexCount, int vertCount, int)
      {
        if (!wide)
        {
          indices.resize(indexCount);
          for (int j = 0; j < indexCount; ++j)
            indices[j] = indices16[j];
        }
        verts4.resize(vertCount);
        for (int j = 0; j < vertCount; ++j)
          verts4[j] = verts[j];
        self.addCollRes(verts4, indices);

        const dag::Vector<mat43f> &inst = self.instances.back();
        v_bbox3_init_empty(self.typeBoxes.push_back());
        bbox3f ibox = self.collRes.back()->vFullBBox;
        auto &sph = self.instancesSph.push_back();
        sph.resize(inst.size());
        for (size_t j = 0, je = inst.size(); j != je; ++j)
        {
          mat44f m;
          v_mat43_transpose_to_mat44(m, inst[j]);
          bbox3f boxAABB;
          v_bbox3_init(boxAABB, m, ibox);
          v_bbox3_add_box(self.typeBoxes.back(), boxAABB);
          sph[j] = v_perm_xyzd(v_bbox3_center(boxAABB), v_splat_x(v_bbox3_outer_rad(boxAABB)));
        }
        self.count += inst.size();
      }
    } handler{*this};
    if (!read_ri_collision_dump(cb, handler))
    {
      // Corrupt/truncated dump: reject wholesale rather than build a partial scene from it.
      logerr("ri_collisions: corrupt or truncated collision dump; rejecting load");
      collRes.clear();
      instances.clear();
      instancesSph.clear();
      typeBoxes.clear();
      count = 0;
      return;
    }
    init();
  }

  eastl::unique_ptr<LRURendinstCollision> lruColl;

  ~LRUCollision() {}

  void init()
  {
    lruColl.reset(new LRURendinstCollision);
    voxelize.init();
    buildScene();
  }

  void rasterize(dag::Span<uint64_t> handles, VolTexture *color, VolTexture *alpha, ShaderElement *e, int instMul, bool prim)
  {
    if (lruColl)
      voxelize.rasterize(*lruColl, handles, color, alpha, e, instMul, prim);
  }

  void gatherBox(bbox3f_cref box, dag::Vector<uint64_t, framemem_allocator> &handles) const
  {
    scene.boxCull<false, false>(box, 0, 0, [&](scene::node_index ni, mat44f_cref node) {
      handles.push_back((uint64_t(scene::get_node_pool(node)) << 32UL) | uint64_t(scene::get_node_flags(node)));
    });
    stlsort::sort(handles.begin(), handles.end(),
      [](auto a, auto b) { return rendinst::handle_to_ri_type(a) < rendinst::handle_to_ri_type(b); });
  }
  template <class InstCb>
  void boxCull(bbox3f_cref box, const InstCb &cb) const
  {
    scene.boxCull<false, false>(box, 0, 0, [&](scene::node_index ni, mat44f_cref node) { cb(ni, node); });
  }
  enum class ObjectClass
  {
    Accept,
    Skip
  };
  struct AlwaysAccept
  {
    ObjectClass operator()(uint32_t) const { return ObjectClass::Accept; }
  };

  template <class Cb, class TypeCb>
  void gatherBox(bbox3f_cref box, const Cb &cb, const TypeCb &tcb) const
  {
    DA_PROFILE;
    for (size_t i = 0, e = instances.size(); i != e; ++i)
    {
      if (tcb(i) != ObjectClass::Accept)
        continue;
      if (!v_bbox3_test_box_intersect(box, typeBoxes[i]))
        continue;
      bbox3f ibox = collRes[i]->vFullBBox;
      for (size_t j = 0, je = instances[i].size(); j != je; ++j)
      {
        vec4f sph = instancesSph[i][j], r = v_splat_w(sph);
        if (!v_bbox3_test_sph_intersect(box, sph, v_mul_x(r, r)))
          continue;
        bbox3f instSphBB;
        v_bbox3_init_by_bsph(instSphBB, sph, r);
        if (!v_bbox3_test_box_intersect(box, instSphBB))
          continue;
        mat44f m;
        v_mat43_transpose_to_mat44(m, instances[i][j]);
        bbox3f boxAABB;
        v_bbox3_init(boxAABB, m, ibox);
        if (v_bbox3_test_box_intersect(box, boxAABB))
          cb(i, instances[i][j], ibox, boxAABB);
      }
    }
  }
  void doMaintenance()
  {
    if (!lruColl)
      return;
    DA_PROFILE;
    scene.doMaintenance(ref_time_ticks(), 10000000);
  }

  void rasterizePrepared(mat44f_cref viewproj, const uint64_t *handles, uint32_t cnt)
  {
    if (!lruColl || !cnt)
      return;
    DA_PROFILE;
    const int old = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(ShaderGlobal::getBlockId("global_frame", ShaderGlobal::LAYER_FRAME), ShaderGlobal::LAYER_FRAME);
    voxelize.rasterize(*lruColl, dag::ConstSpan<uint64_t>(handles, cnt), nullptr, nullptr, voxelize.renderCollisionElem, 1, false);
    ShaderGlobal::setBlock(old, ShaderGlobal::LAYER_FRAME);
  }
  void rasterize(mat44f_cref viewproj)
  {
    if (!lruColl)
      return;
    DA_PROFILE;
    FRAMEMEM_REGION;
    dag::Vector<uint32_t, framemem_allocator> poolsCount, poolsOfs;
    poolsCount.resize(scene.getPoolsCount());
    poolsOfs.resize(scene.getPoolsCount());
    dag::Vector<uint64_t, framemem_allocator> handles, handles2;
    uint64_t *handlesPtr = nullptr;
    uint32_t handlesCnt = 0;
    {
      scene.frustumCull<false, false, false>(viewproj, v_zero(), 0, 0, nullptr, [&](scene::node_index ni, mat44f_cref node, vec4f) {
        uint32_t pool = scene::get_node_pool(node), inst = scene::get_node_flags(node);
        poolsCount.data()[pool]++;
        handles.push_back((uint64_t(pool) << 32UL) | uint64_t(inst));
      });
      if (handles.size() > poolsCount.size() * 2) // use radix sort
      {
        uint32_t ofs = 0, pool = 0;
        for (auto p : poolsCount)
        {
          poolsOfs.data()[pool++] = ofs;
          ofs += p;
        }
        handles2.resize(handles.size());
        for (auto i : handles)
        {
          const uint32_t pool = i >> 32UL;
          handles2.data()[poolsOfs.data()[pool]++] = i;
        }
        handlesPtr = handles2.data();
        handlesCnt = handles2.size();
      }
      else
      {
        stlsort::sort(handles.begin(), handles.end(),
          [](auto a, auto b) { return rendinst::handle_to_ri_type(a) < rendinst::handle_to_ri_type(b); });
        handlesPtr = handles.data();
        handlesCnt = handles.size();
      }
    }
    rasterizePrepared(viewproj, handlesPtr, handlesCnt);
  }
  void buildScene()
  {
    scene.init(256);
    scene.reserve(count);
    scene.doMaintenance(ref_time_ticks(), 10000000);
    for (size_t i = 0, e = instances.size(); i != e; ++i)
    {
      scene.setPoolBBox(i, collRes[i]->vFullBBox);
    }
    for (size_t i = 0, e = instances.size(); i != e; ++i)
      for (size_t j = 0, je = min<size_t>(65535, instances[i].size()); j != je; ++j)
      {
        mat44f m;
        v_mat43_transpose_to_mat44(m, instances[i][j]);
        scene.allocate(m, i, j);
      }
  }
  bbox3f calcBox() const
  {
    bbox3f ret;
    v_bbox3_init_empty(ret);
    for (auto &box : typeBoxes)
      v_bbox3_add_box(ret, box);
    return ret;
  }

  eastl::optional<LRURendinstCollision::MeshData> getModelData(int modelId) const { return lruColl->getModelData(modelId); }
};
