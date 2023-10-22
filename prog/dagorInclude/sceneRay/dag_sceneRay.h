//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/// @addtogroup AdvTech Advanced technologies
/// @{

/// @addtogroup Clipping Ray-tracing and clipping
/// @{

///  Classes for ray-tracing meshes and clipping capsules with them.

/** @file
  Provides ray-tracing and clipping capabilities for a scene composed
  of meshes and capsules and for arbitrary dynamic objects.
  Defines generic interface for deriving your own classes implementing
  ray-tracing.
*/

#include <sceneRay/dag_sceneRayDecl.h>
#include <math/dag_wooray3d.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_patchTab.h>
#include <sceneRay/dag_rtHierGrid3.h>
#include <math/integer/dag_IBBox3.h>
#include <vecmath/dag_vecMath.h>

// forward declarations for external classes
struct Capsule;
class IGenSave;
class IGenLoad;
class GlobalSharedMemStorage;

// clang-format off
namespace dag
{
template<typename T> struct IndexTypeHelper {};
template<> struct IndexTypeHelper<SceneRayI24F8> { typedef unsigned index_t; };
template<> struct IndexTypeHelper<uint16_t> { typedef uint16_t index_t; };
}
// clang-format on

//! Standard interface for ray-tracing static scene composed of meshes.
//! This class provide no way to create useful object -  use deserialized or buildable one.
template <typename FI>
class StaticSceneRayTracerT
{
public:
  DAG_DECLARE_NEW(midmem)

  // forward declarations for internal classes
  struct GetFacesContext;
  typedef typename dag::IndexTypeHelper<FI>::index_t index_t;

  GetFacesContext *getMainThreadCtx() const;
  class Ray : public WooRay3d
  {
  public:
    real mint, startAt;
    Ray(const Point3 &start, const Point3 &dir, real dist, real start_at, const Point3 &leafSize) :
      startAt(start_at), mint(dist), WooRay3d(start, dir, leafSize)
    {}
  };
  enum
  {
    CULL_CCW = 0x01,
    CULL_CW = 0x02,
    CULL_BOTH = CULL_CCW | CULL_CW,
    USER_VISIBLE = 0x04,
    USER_INVISIBLE = 0x08,
    USER_FLAG3 = 0x10,
    USER_FLAG4 = 0x20,
    USER_FLAG5 = 0x40,
    USER_FLAG6 = 0x80,
  };

  struct RTface
  {
    index_t v[3];
  };

  struct RTfaceBoundFlags
  {
    Point3_vec4 n;
  };
  G_STATIC_ASSERT(alignof(RTfaceBoundFlags) == 16);

  typedef FI FaceIndex;

  __forceinline const Point3 &getLeafSize() const { return dump.leafSize; }
  __forceinline const BBox3 &getBox() const { return dump.rtBox; }
  __forceinline const BSphere3 &getSphere() const { return dump.rtSphere; }
  __forceinline const IBBox3 &getLeafLimits() const { return dump.leafLimits; }
  __forceinline const Point3_vec4 &verts(int i) const { return dump.vertsPtr[i]; }
  __forceinline Point3_vec4 &verts(int i) { return dump.vertsPtr[i]; }
  __forceinline int getVertsCount() const { return dump.vertsCount; }
  __forceinline RTface &faces(int i) { return dump.facesPtr[i]; }
  __forceinline const RTface &faces(int i) const { return dump.facesPtr[i]; }
  __forceinline int getFacesCount() const { return dump.facesCount; }
  __forceinline vec4f getNormal(int i) const
  {
    const RTface &f = faces(i);
    vec3f v0 = v_ld(&verts(f.v[0]).x);
    vec3f v1 = v_ld(&verts(f.v[1]).x);
    vec3f v2 = v_ld(&verts(f.v[2]).x);
    return v_norm3(v_cross3(v_sub(v1, v0), v_sub(v2, v0)));
  }
  __forceinline RTfaceBoundFlags facebounds(int i) const
  {
    RTfaceBoundFlags fn;
    v_st(&fn.n.x, getNormal(i));
    return fn;
  }

  __forceinline FaceIndex &faceIndices(int i) const { return dump.faceIndicesPtr[i]; }
  __forceinline int getFaceIndicesCount() const { return dump.faceIndicesCount; }

  __forceinline void setCullFlags(unsigned cull_flags) { cullFlags = cull_flags << 24; }

  int getFaces(Tab<int> &face, const Point3 &center, real rad) const { return getFaces(face, BBox3(center, rad)); }
  int getFaces(Tab<int> &face, const BBox3 &box) const;

  // template for incremental get faces (usually used in collision data cache)
  template <class T, class BitChk>
  inline void getFacesCached(Tab<T> &list, const BBox3 &box, BitChk &fchk) const;

  template <class CB>
  inline bool getVecFacesCached(bbox3f_cref wbox, CB &ctx) const;

  //! each triangle is vert0, edge1, edge2
  inline bool getVecFacesCached(bbox3f_cref wbox, vec3f *__restrict triangles, int &tri_left) const;

  virtual ~StaticSceneRayTracerT();

  //! Tests ray hit to closest object and returns parameters of hit (if happen)
  /// return -1 if not hit, face index otherwise
  int traceray(const Point3 &p, const Point3 &dir, real &t, int fromFace = -1) const;
  VECTORCALL int traceray(vec3f p, vec3f dir, real &mint, int fromFace = -1) const;

  //! Gets height to closest object, but below point and returns parameters of hit (if happen)
  /// return -1 if there is no face below, face index otherwise
  int getHeightBelow(const Point3 &pos1, float &ht) const;

  //! TraceRay down from point to closest object, but not lower then to mint returns parameters of hit (if happen)
  /// return -1 if there is no face below, face index otherwise
  int traceDown(const Point3 &pos1, float &mint) const;

  //! Tests ray hit to closest object and returns parameters of hit (if happen), for normalized Dir only
  /// return -1 if not hit, face index otherwise
  int tracerayNormalized(const Point3 &p, const Point3 &dir, real &t, int fromFace = -1) const;
  VECTORCALL int tracerayNormalized(vec3f p, vec3f dir, real &mint, int fromFace = -1) const;

  //! Tests ray hit to closest object and returns parameters of hit (if happen), for normalized Dir only
  /// return -1 if not hit, face index otherwise
  int tracerayNormalizedScalar(const Point3 &p, const Point3 &normDir, real &t, int fromFace = -1) const;

  __forceinline void setUseFlagMask(unsigned flag) { useFlags = flag << 24; }

  __forceinline void setSkipFlagMask(unsigned flag) { skipFlags = flag << 24; }

  //! Tests ray hit to any object and returns parameters of hit (if happen)
  VECTORCALL int rayhitNormalizedIdx(vec3f p, vec3f dir, real mint) const;
  VECTORCALL bool rayhitNormalized(vec3f p, vec3f dir, real mint) const { return rayhitNormalizedIdx(p, dir, mint) >= 0; }
  bool rayhitNormalized(const Point3 &p, const Point3 &d, real t) const { return rayhitNormalized(v_ldu(&p.x), v_ldu(&d.x), t); }

  //! Tests for capsule clipping by scene; returns depth of penetration and normal to corresponding face
  int clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized) const;


  bool serialize(IGenSave &cb, bool be_target = false, int *out_dump_sz = NULL, bool zcompr = true);

  __forceinline unsigned getUseFlags() const { return useFlags >> 24; }
  __forceinline unsigned getSkipFlags() const { return skipFlags >> 24; }

protected:
  unsigned useFlags, skipFlags;
  unsigned cullFlags; //< or flags after all
  enum
  {
    VAL_CULL_CCW = CULL_CCW << 24,
    VAL_CULL_CW = CULL_CW << 24,
    VAL_CULL_BOTH = CULL_BOTH << 24,
  };

  StaticSceneRayTracerT();

public:
  struct Node
  {
    DAG_DECLARE_NEW(midmem)

    Point3 bsc;
    real bsr2;
    PatchablePtr<Node> sub0;
    PatchablePtr<Node> sub1;
    Node()
    {
      sub0 = NULL;
      sub1 = NULL;
    }

    bool isLeaf() const { return sub0.get() < sub1.get(); }
    bool isNode() const { return sub0.get() > sub1.get(); }
    const Node *getLeft() const { return sub1.get(); }
    const Node *getRight() const { return sub0.get(); }
    Node *getLeft() { return sub1.get(); }
    Node *getRight() { return sub0.get(); }
    void destroy();
    void serialize(IGenSave &cb, const void *dump_base) const;
  };

protected:
  struct LNode : Node
  {
    FaceIndex *getFaceIndexStart() { return (FaceIndex *)Node::sub0.get(); }
    FaceIndex *getFaceIndexEnd() { return (FaceIndex *)Node::sub1.get(); }
    const FaceIndex *getFaceIndexStart() const { return (const FaceIndex *)Node::sub0.get(); }
    const FaceIndex *getFaceIndexEnd() const { return (const FaceIndex *)Node::sub1.get(); }
  };
  //! Tests for capsule clipping by scene; returns depth of penetration and normal to corresponding face
  bool clipCapsuleFace(FaceIndex findex, const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedir) const;
  inline int clipCapsuleNode(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
    const Node *node) const;
  inline int clipCapsuleLNode(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
    const LNode *node) const;
  inline int tracerayLNode(const Point3 &p, const Point3 &dir, real &t, const LNode *node, int fromFace) const;
  inline int tracerayNode(const Point3 &p, const Point3 &dir, real &t, const Node *node, int fromFace) const;

  // NOINLINE and ref& added for better performance
  template <bool noCull>
  DAGOR_NOINLINE int tracerayLNodeVec(const vec3f &p, const vec3f &dir, float &t, const LNode *node, int fromFace) const;
  template <bool noCull>
  DAGOR_NOINLINE int tracerayNodeVec(const vec3f &p, const vec3f &dir, float &t, const Node *node, int fromFace) const;

  template <bool noCull>
  VECTORCALL inline int rayhitLNodeIdx(vec3f p, vec3f dir, float t, const LNode *node) const;
  template <bool noCull>
  VECTORCALL inline int rayhitNodeIdx(vec3f p, vec3f dir, float t, const Node *node) const;

  inline int getHeightBelowLNode(const Point3 &p, real &ht, const LNode *node) const;
  inline int getHeightBelowNode(const Point3 &p, real &ht, const Node *node) const;
  void rearrangeLegacyDump(char *data, uint32_t n);

  template <typename U>
  VECTORCALL static void getFacesNode(vec3f bmin, vec3f bmax, Tab<int> &face, U &uniqueness, const Node *node,
    const StaticSceneRayTracerT *rt);
  template <typename U>
  VECTORCALL static __forceinline void getFacesLNode(vec3f bmin, vec3f bmax, Tab<int> &face, U &uniqueness, const LNode *node,
    const StaticSceneRayTracerT *rt);

  template <class CB = GetFacesContext>
  static bool getVecFacesCachedNode(const Node *node, CB &ctx);
  template <class CB = GetFacesContext>
  static bool getVecFacesCachedLNode(const LNode &node, CB &ctx);

  typedef Node Leaf;
  struct alignas(16) Dump
  {
    BBox3 rtBox;       //< Bounding box for all objects in raytracer (in local coordinates)
    BSphere3 rtSphere; //< Bounding box for all objects in raytracer (in local coordinates)
    Point3 leafSize;
    IBBox3 leafLimits;
    PatchablePtr<Point3_vec4> vertsPtr;
    PatchablePtr<RTface> facesPtr;
    PatchablePtr<RTfaceBoundFlags> obsolete_faceboundsPtr; // obsolete!
    PatchablePtr<FaceIndex> faceIndicesPtr;
    PatchablePtr<RTHierGrid3<Leaf, 0>> grid;
    int vertsCount, facesCount, faceIndicesCount;
    int version, reserved2, reserved3;

    Dump() : vertsCount(0), facesCount(0), faceIndicesCount(0), version(0), reserved2(0), reserved3(0)
    {
      vertsPtr = NULL;
      facesPtr = NULL;
      faceIndicesPtr = NULL;
      grid = NULL;
    }
    void patch();
  } dump;
  bbox3f v_rtBBox;
  Tab<Point3_vec4> vertsVecLegacy;
};

template <typename FI>
class DeserializedStaticSceneRayTracerT : public StaticSceneRayTracerT<FI>
{
public:
  using typename StaticSceneRayTracerT<FI>::RTface;
  using typename StaticSceneRayTracerT<FI>::FaceIndex;
  using StaticSceneRayTracerT<FI>::dump;
  using StaticSceneRayTracerT<FI>::verts;
  using StaticSceneRayTracerT<FI>::getBox;
  using StaticSceneRayTracerT<FI>::getVertsCount;
  using StaticSceneRayTracerT<FI>::getFacesCount;

protected:
  using typename StaticSceneRayTracerT<FI>::Dump;
  using StaticSceneRayTracerT<FI>::vertsVecLegacy;
  using StaticSceneRayTracerT<FI>::v_rtBBox;

public:
  // Return if loaded from dump
  virtual ~DeserializedStaticSceneRayTracerT();
  DeserializedStaticSceneRayTracerT();
  bool serializedLoad(IGenLoad &);
  void createInMemory(char *data); /// creates tracer in memory. modifies this memory. allocates one bitarray

protected:
  char *loadedDump;
  bool _serializedLoad(IGenLoad &, unsigned block_rest);
};

//! Create new (enhanced) ray-tracer object (via dump load)
DeserializedStaticSceneRayTracer *create_staticmeshscene_raytracer(IGenLoad &cb);

#include <sceneRay/dag_sceneRayBuildable.h>

/// @}

/// @}
