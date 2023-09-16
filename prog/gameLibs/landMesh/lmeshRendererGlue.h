#pragma once


#include <generic/dag_smallTab.h>
#include <generic/dag_carray.h>

namespace landmesh
{
struct VisibilityData;
struct OptimizedScene;

struct VisibilityData
{
public:
  unsigned minUsed, maxUsed;
  SmallTab<unsigned, MidmemAlloc> usedElems;
  OptimizedScene *scn;
  int elemCount;

public:
  VisibilityData() { clear(); }
  ~VisibilityData() { clear(); }

  void init(OptimizedScene *s);

  void clear()
  {
    clear_and_shrink(usedElems);
    elemCount = 0;
    scn = NULL;
    minUsed = elemCount + 1;
    maxUsed = 0;
  }
  void setAll()
  {
    mem_set_ff(usedElems);
    minUsed = 0;
    maxUsed = elemCount - 1;
  }
  void reset()
  {
    if (minUsed < elemCount)
    {
      mem_set_0(make_span(usedElems).subspan(minUsed / 32, maxUsed / 32 + 1 - minUsed / 32));
      minUsed = elemCount + 1;
      maxUsed = 0;
    }
  }
  bool allocated() const { return usedElems.data(); }
  void mark(int idx)
  {
    if (minUsed > idx)
      minUsed = idx;
    if (maxUsed < idx)
      maxUsed = idx;
    usedElems[idx / 32] |= 0x80000000U >> (idx % 32);
  }
};

struct OptimizedScene
{
  struct Elem
  {
    int sv, si, baseVertex;
    unsigned short numv, numf;
  };
  struct Mat
  {
    ShaderElement *e;
    unsigned short se, ee;
    int bottomType;
  };

public:
  SmallTab<Mat, MidmemAlloc> mats;
  SmallTab<Elem, MidmemAlloc> elems;
  VisibilityData visData;
  GlobalVertexData *vData;
  uint16_t usedElemsStride;
  bool inited;
  bool prepareDone;

  OptimizedScene() : usedElemsStride(0), inited(false), prepareDone(false), vData(NULL) {}
};

struct IRosdSetStatesCB
{
  virtual bool applyMat(ShaderElement *mat, int bottom_type) const = 0;
};

void buildOptSceneData(landmesh::OptimizedScene &optScn, LandMeshManager &provider, int lod);
void renderOptSceneData(landmesh::OptimizedScene &optScn, const IRosdSetStatesCB &cb);
} // namespace landmesh
