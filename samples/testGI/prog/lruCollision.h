// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <render/lruCollision.h>
#include <daGI2/lruCollisionVoxelization.h>
#include <scene/dag_tiledScene.h>
#include <execution>
#include <algorithm>

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
  size_t addCollRes(const dag::Vector<Point3_vec4> &vertices, const dag::Vector<uint16_t> &indices)
  {
    eastl::unique_ptr<CollisionResource> coll(new CollisionResource());
    coll->numMeshNodes = 1;
    auto &n = coll->createNode();
    coll->meshNodesHead = &n;
    n.flags = n.IDENT;
    n.tm = TMatrix::IDENT;
    n.resetVertices(make_span(memalloc_typed<Point3_vec4>(vertices.size(), midmem), vertices.size()));
    mem_copy_from(n.vertices, vertices.data());
    n.resetIndices(make_span(memalloc_typed<uint16_t>(indices.size(), midmem), indices.size()));
    mem_copy_from(n.indices, indices.data());
    v_bbox3_init_empty(coll->vFullBBox);
    for (auto &v : vertices)
      v_bbox3_add_pt(coll->vFullBBox, v_ld(&v.x));
    collRes.push_back(eastl::move(coll));
    return collRes.size() - 1;
  }
  void load(IGenLoad &cb)
  {
    cb.readInt();
    struct Mesh
    {
      dag::Vector<uint16_t> indices;
      dag::Vector<Point3> vertices;
    };
    Mesh mesh;
    dag::Vector<Point3_vec4> vertices;
    for (int i = 0; cb.tell() < cb.getTargetDataSize(); ++i)
    {
      mesh.indices.resize(cb.readInt());
      cb.read(mesh.indices.begin(), mesh.indices.size() * sizeof(*mesh.indices.data()));
      mesh.vertices.resize(cb.readInt());
      cb.read(mesh.vertices.begin(), mesh.vertices.size() * sizeof(*mesh.vertices.data()));
      vertices.resize(mesh.vertices.size());
      for (int j = 0; j < mesh.vertices.size(); ++j)
        vertices[j] = mesh.vertices[j];
      addCollRes(vertices, mesh.indices);

      int instCount = cb.readInt();
      instances.push_back().resize(instCount);
      cb.read(instances.back().data(), instances.back().size() * sizeof(mat43f));
      v_bbox3_init_empty(typeBoxes.push_back());
      bbox3f ibox = collRes.back()->vFullBBox;
      instancesSph.push_back().resize(instCount);
      for (size_t j = 0, je = instances.back().size(); j != je; ++j)
      {
        mat44f m;
        v_mat43_transpose_to_mat44(m, instances.back()[j]);
        bbox3f boxAABB;
        v_bbox3_init(boxAABB, m, ibox);
        v_bbox3_add_box(typeBoxes.back(), boxAABB);
        instancesSph.back()[j] = v_perm_xyzd(v_bbox3_center(boxAABB), v_splat_x(v_bbox3_outer_rad(boxAABB)));
      }
      count += instances.back().size();
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
