//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_patchTab.h>
#include <math/dag_bounds3.h>
#include <math/integer/dag_IBBox3.h>
#include <memory/dag_mem.h>

class IGenLoad;

template <typename FI>
class StaticSceneRayTracerT;

struct SceneRayI24F8
{
  unsigned index : 24;
  unsigned flags : 8;
};

typedef StaticSceneRayTracerT<SceneRayI24F8> StaticSceneRayTracer;

// clang-format off
namespace dag
{
  template<typename T> struct IndexTypeHelper {};
  template<> struct IndexTypeHelper<SceneRayI24F8> { typedef unsigned index_t; };
  template<> struct IndexTypeHelper<uint16_t> { typedef uint16_t index_t; };
}
// clang-format on

// A serialized FRT dump as loaded: its vertices, its faces, and its face entries. A SceneRayI24F8 entry's
// top byte carries a face's flags, and a face with no entry carries none.
template <typename FI>
class StaticSceneRayTracerT
{
public:
  DAG_DECLARE_NEW(midmem)

  enum
  {
    CULL_CCW = 0x01,
    CULL_CW = 0x02,
    CULL_BOTH = CULL_CCW | CULL_CW,
    USER_INVISIBLE = 0x08,
  };

  typedef typename dag::IndexTypeHelper<FI>::index_t index_t;
  typedef FI FaceIndex;

  struct RTface
  {
    index_t v[3];
  };

  const Point3_vec4 &verts(int i) const { return dump.vertsPtr[i]; }
  Point3_vec4 &verts(int i) { return dump.vertsPtr[i]; }
  int getVertsCount() const { return dump.vertsCount; }
  RTface &faces(int i) { return dump.facesPtr[i]; }
  const RTface &faces(int i) const { return dump.facesPtr[i]; }
  int getFacesCount() const { return dump.facesCount; }
  const FaceIndex &faceIndices(int i) const { return dump.faceIndicesPtr[i]; }
  int getFaceIndicesCount() const { return dump.faceIndicesCount; }

  StaticSceneRayTracerT() = default;
  void destroy() const { delete this; }

  static StaticSceneRayTracerT *load(IGenLoad &cb); // Return new instance with data loaded from `cb`

protected:
  // the serialized layout: fields no reader asks for keep their place
  struct alignas(16) Dump
  {
    BBox3 rtBox;
    BSphere3 rtSphere;
    Point3 leafSize;
    IBBox3 leafLimits;
    PatchablePtr<Point3_vec4> vertsPtr;
    PatchablePtr<RTface> facesPtr;
    PatchablePtr<char> obsolete_faceboundsPtr;
    PatchablePtr<FaceIndex> faceIndicesPtr;
    PatchablePtr<char> obsolete_gridPtr;
    int vertsCount = 0, facesCount = 0, faceIndicesCount = 0;
    int version = 0, reserved2 = 0, reserved3 = 0;

    Dump()
    {
      vertsPtr = nullptr;
      facesPtr = nullptr;
      faceIndicesPtr = nullptr;
    }
  };
  Dump dump; // constraint: the only member, load() hands out the loaded dump as the tracer

  ~StaticSceneRayTracerT() = default; // Call `destroy` if you need to delete new'd/loaded instance
};

extern template class StaticSceneRayTracerT<SceneRayI24F8>;
extern template class StaticSceneRayTracerT<uint16_t>;

// A level bin's FRT block, read from its version word on, which goes unchecked: the tracer, and its per-face physmat ids
// resolved through PhysMat against the block's own name map. Null when the tracer does not read, leaving out_face_pmid
// untouched; the caller destroys what it gets.
StaticSceneRayTracer *load_frt_block(IGenLoad &crd, Tab<uint8_t> &out_face_pmid);
