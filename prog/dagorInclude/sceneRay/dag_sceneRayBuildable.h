//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <sceneRay/dag_sceneRay.h>

template <class LT, int LP>
class CreatableRTHierGrid3 : public RTHierGrid3<LT, LP>
{
protected:
  typedef RTHierGrid3<LT, LP> BASE;
  Tab<typename BASE::TopNode> createdNode;
  IMemAlloc *mem;

public:
  CreatableRTHierGrid3(int cl, IMemAlloc *m = midmem_ptr()) : RTHierGrid3<LT, LP>(cl), mem(m), createdNode(m) {}

  ~CreatableRTHierGrid3() { clear(); }
  void setmem(IMemAlloc *m) { mem = m; }
  void setLevels(int cl)
  {
    BASE::setLevels(cl);
    clear();
  }
  void clear()
  {
    RTHierGrid3<LT, LP>::clear();
    clear_and_shrink(createdNode);
  }
  typename BASE::Leaf **create_leaf(int u, int v, int w)
  {
    int uc = u & ~(BASE::topsz - 1), vc = v & ~(BASE::topsz - 1), wc = w & ~(BASE::topsz - 1);

    typename BASE::TopNode *tn = BASE::findTopNode(uc, vc, wc);
    if (!tn)
    {
      if (BASE::nodeCount() != createdNode.size())
        fatal_x("Deserialized");
      createdNode.push_back(typename BASE::TopNode(uc, vc, wc));
      BASE::nodes.init(&createdNode[0], createdNode.size());
      tn = &createdNode.back();
    }
    return tn->create_leaf((u & (BASE::topsz - 1)) >> LP, (v & (BASE::topsz - 1)) >> LP, (w & (BASE::topsz - 1)) >> LP,
      1 << BASE::clev, mem);
  }
};

class SceneRayBuildContext;

template <typename FI>
class BuildableStaticSceneRayTracerT : public StaticSceneRayTracerT<FI>
{
public:
  using typename StaticSceneRayTracerT<FI>::RTface;
  using typename StaticSceneRayTracerT<FI>::FaceIndex;
  using typename StaticSceneRayTracerT<FI>::Node;
  using StaticSceneRayTracerT<FI>::dump;
  using StaticSceneRayTracerT<FI>::verts;
  using StaticSceneRayTracerT<FI>::faces;
  using StaticSceneRayTracerT<FI>::faceIndices;
  using StaticSceneRayTracerT<FI>::getLeafSize;
  using StaticSceneRayTracerT<FI>::getBox;
  using StaticSceneRayTracerT<FI>::getVertsCount;
  using StaticSceneRayTracerT<FI>::getFacesCount;
  using StaticSceneRayTracerT<FI>::CULL_CCW;
  using StaticSceneRayTracerT<FI>::CULL_BOTH;
  using typename StaticSceneRayTracerT<FI>::index_t;

protected:
  using typename StaticSceneRayTracerT<FI>::Leaf;
  using typename StaticSceneRayTracerT<FI>::LNode;
  using typename StaticSceneRayTracerT<FI>::BNode;
  using typename StaticSceneRayTracerT<FI>::Dump;
  using StaticSceneRayTracerT<FI>::v_rtBBox;

public:
  BuildableStaticSceneRayTracerT(const Point3 &lsz, int lev);
  virtual ~BuildableStaticSceneRayTracerT();

  //! Adds mesh into scene
  bool addmesh(const Point3 *vert, int vcount, const unsigned *face, unsigned stride, int fn, const unsigned *face_flags,
    bool rebuild = true);

  bool addmesh(const uint8_t *vert, unsigned v_stride, int vcount, const index_t *face, unsigned stride, int fcount,
    const unsigned *flags, bool rebuild_now);

  void oneMeshInplace(const Point3_vec4 *vert, int vcount, bool use_external_verts, const index_t *face, int fn,
    bool use_external_faces, const unsigned *flags);

  //! reserve memory (helpful before addmesh)
  bool reserve(int face_count, int vert_count);

  //! Rebuilds object. Should be called after adding meshes, before any ray-tracing
  bool rebuild(bool fast = false);

  struct BuildRTfaceBoundFlags
  {
    Point3 sc;
  };

protected:
  CreatableRTHierGrid3<Leaf, 0> createdGrid;
  Tab<Point3_vec4> vertsTab;
  Tab<RTface> facesTab;
  Tab<FaceIndex> faceIndicesTab;
  Tab<unsigned> faceFlagsTab;               // temporary
  Tab<BuildRTfaceBoundFlags> faceboundsTab; // temporary
  Tab<bbox3f> fboxes;                       // temporary
  Tab<uint8_t> nodesMemory;                 // allocator for nodes

  void notify_leaf_added(int x, int y, int z) { dump.leafLimits.add(x, y, z); }
  void clear_leaf_bounds() { dump.leafLimits.setEmpty(); }

  void buildLNode(LNode *node, FaceIndex *fc, int numf, SceneRayBuildContext &ctx);
  void buildLeaf(Leaf **leaf, SceneRayBuildContext &ctx);
  void replacePointers(Node *);
  void replaceAllPointers();
  __forceinline bool __add_face(const RTface &rf, int ri);
  uintptr_t build_node(FaceIndex *fc, int numf, SceneRayBuildContext &ctx); // index in nodesMemory
};

BuildableStaticSceneRayTracer *create_buildable_staticmeshscene_raytracer(const Point3 &lsz, int lev);
