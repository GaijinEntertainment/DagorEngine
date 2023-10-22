//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_IRenderWrapperControl.h>
#include <3d/dag_render.h>
#include <math/dag_SHlight.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_patchTab.h>
#include <math/dag_bounds3.h>
#include <vecmath/dag_vecMath.h>
#include <scene/dag_visibility.h>
#include <scene/dag_buildVisTree.h>
#include <generic/dag_carray.h>

class ShadowRenderResources;
class JointShadowCaster;
class IGenLoad;
class IGenSave;
struct DynamicLight;
class Occlusion;
class CachedLeaves;
class BBoxTreeNode;
class BBoxTreeLeaf;
struct Decal;


struct LightingObjectData
{
  unsigned int uniqueId; // To allow fast reliable compare.
  Point3 position;
  float farRange;
  float stationaryRadius;
  Point3 direction;
  float stationaryAngle;

  LightingObjectData() :
    uniqueId(0xFFFFFFFF),
    position(1e6f, 1e6f, 1e6f),
    farRange(0.f),
    stationaryRadius(0.f),
    direction(0.f, 0.f, 0.f),
    stationaryAngle(0.f)
  {}
};


class RenderScene
{
public:
  DAG_DECLARE_NEW(midmem)

  typedef bool (*reset_states_t)(const bbox3f &, bool, bool);

  struct RenderObject
  {
    struct ObjectLod
    {
      PatchablePtr<ShaderMesh> mesh;
      real lodRangeSq;
      int _resv;
    };

    enum
    {
      ROP_FADE = 0x01,
      ROP_WATER = 0x02,
      ROP_FADENULL = 0x04,
      ROP_BACK_FACE_DYNAMIC_LIGHTING = 0x08,
      ROP_NOT_MIX_LODS = 0x10,
    };

    enum
    {
      ROF_Valid = 0x0001,
      ROF_HasOpaq = 0x0004,
      ROF_HasTrans = 0x0008,

      ROF_CastShadows = 0x0010,

      ROF_WRAPPERMASK = 0x0F00,
      ROF_WRAPPERSHIFT = 8
    };

  public:
    //
    // Layout is carefully designed for in-place loading, avoid to modify it!
    //
    PatchablePtr<void> visobj; // not needed - obsolete
    PatchablePtr<void> rctrl;  // not needed - obsolete

    PatchableTab<ObjectLod> lods;
    BSphere3 bsph;
    real visrange;
    BBox3 bbox;

    float maxSubobjRad;
    uint16_t flags, lightingObjectDataNo;
    uint8_t curLod, priority, params;
    uint8_t start_last_plane; ///< can be used in future

    real lodNearVisRangeSq, lodFarVisRangeSq;
    real mixNextLod; ///< if >0 need to render next lod with transparency mixNextLod

    PatchablePtr<const char> name;

  public:
    void freeData();

    void update()
    {
      flags &= ~(ROF_Valid | ROF_HasOpaq | ROF_HasTrans);
      for (int li = 0; li < lods.size(); ++li)
      {
        ShaderMesh *_mesh = lods[li].mesh;

        if (!_mesh)
          continue;

        dag::Span<ShaderMesh::RElem> op_elem = _mesh->getElems(_mesh->STG_opaque, _mesh->STG_atest);
        if (op_elem.size())
          flags |= ROF_HasOpaq;
        if (_mesh->getElems(_mesh->STG_trans).size())
          flags |= ROF_HasTrans;
      }

      if (flags & (ROF_HasOpaq | ROF_HasTrans))
        flags |= ROF_Valid;
    }

    inline dag::ConstSpan<ObjectLod> getLods() const { return lods; }

    // IVisibilityObject.
    BBox3 getBoundingBox() { return bbox; }
    BSphere3 getBoundingSphere() { return bsph; }
    float getVisibilityRange() { return visrange; }


    static void parseParams(unsigned char &_params, const char *text);
  };

protected:
  struct OptimizedScene;


public:
  PatchableTab<RenderObject> obj;
  BBox3 calcBoundingBox()
  {
    BBox3 box;
    for (int i = 0; i < obj.size(); ++i)
      box += obj[i].getBoundingBox();
    return box;
  }
  void *robjMem;

public:
  RenderScene();
  ~RenderScene();

  // void initTransSettings(struct CfgDiv& cfg);

  bool replaceTexture(TEXTUREID old_texid, TEXTUREID new_texid);

  void load(const char *lmdir, bool use_portal_visibility);

  void loadBinary(IGenLoad &crd, dag::ConstSpan<TEXTUREID> texMap, bool use_portal_visibility);

  //! explicit scene visibility preparation (optional, called automaticall from render())
  void prepareVisibility(const VisibilityFinder &vf, int render_id = 0, unsigned render_flags_mask = 0xFFFFFFFFU)
  {
    if (render_id < 0)
      optScn.visData.setAll();
    else if (!RenderScene::checked_frustum && !optScn.prepareDone)
    {
      optScn.visData.reset();
      if (!optScn.visTree.nodes.empty())
        prepareVisibilitySimple(&optScn.visData, render_id, render_flags_mask, vf);
    }
  }

  struct VisibilityData
  {
    // protected:
  public: // to be friend of OptimizedScene::MarkElems class
    SmallTab<unsigned, MidmemAlloc> usedElems;
    carray<uint16_t, 7> lastElemNum;
    OptimizedScene *scn;

  public:
    VisibilityData() { clear(); }
    ~VisibilityData() { clear(); }

    void clear()
    {
      lastElemNum[0] = lastElemNum[1] = lastElemNum[2] = lastElemNum[3] = 0;
      lastElemNum[4] = lastElemNum[5] = lastElemNum[6] = 0;
      clear_and_shrink(usedElems);
      scn = NULL;
    }
    void setAll()
    {
      mem_set_ff(usedElems);
      lastElemNum[0] = lastElemNum[1] = lastElemNum[2] = lastElemNum[3] = 1;
      lastElemNum[4] = lastElemNum[5] = lastElemNum[6] = 1;
    }
    void reset()
    {
      lastElemNum[0] = lastElemNum[1] = lastElemNum[2] = lastElemNum[3] = 0;
      lastElemNum[4] = lastElemNum[5] = lastElemNum[6] = 0;
      mem_set_0(usedElems);
    }
    bool allocated() const { return usedElems.data(); }
  };

  //
  // modern API for async prepare visibility
  //
  void allocPrepareVisCtx(int render_id)
  {
    if (!obj.size())
      return;
    G_ASSERT(optScn.inited);
    render_id = render_id % MAX_CVIS;
    if (cVisData[render_id].allocated())
      return;
    cVisData[render_id].usedElems = optScn.visData.usedElems;
    cVisData[render_id].scn = NULL;
  }
  void doPrepareVisCtx(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask = 0xFFFFFFFFU)
  {
    if (!obj.size())
      return;
    G_ASSERT(optScn.inited);
    VisibilityData *vd = &cVisData[render_id % MAX_CVIS];
    G_ASSERT(vd->allocated());
    if (!vd->allocated())
      return;
    G_ASSERT(!vd->scn);
    if (vd->scn)
      return;

    vd->scn = &optScn;
    vd->reset();
    prepareVisibilitySimple(vd, render_id, render_flags_mask, vf);
    vd->scn = NULL;
  }
  void setPrepareVisCtxToRender(int render_id)
  {
    if (!obj.size())
      return;
    G_ASSERT(optScn.inited);
    if (render_id < 0)
    {
      optScn.prepareDone = false;
      return;
    }

    VisibilityData *vd = &cVisData[render_id % MAX_CVIS];
    G_ASSERT(vd->allocated());
    if (!vd->allocated())
      return;
    G_ASSERT(!vd->scn);
    if (vd->scn)
      return;

    optScn.visData = *vd;
    optScn.visData.scn = &optScn;
    optScn.prepareDone = true;
  }
  void releasePrepareVisCtx(int render_id)
  {
    if (!obj.size())
      return;
    G_ASSERT(optScn.inited);
    render_id = render_id % MAX_CVIS;
    cVisData[render_id].clear();
  }


  //! checks visibility and renders scene;
  //! render_id<0 forces full render, other values (0..7) are context for visibility finder
  void render(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask);
  void render(int render_id = 0, unsigned render_flags_mask = 0xFFFFFFFFU);
  void render_trans();

  static void enablePrerender(bool on) { enable_prerender = on; } // off by default

  bool hasReflectionRefraction() { return hasReflectionRefraction_; }

  static void set_checked_frustum(bool on) { checked_frustum = on; }
  static void set_depth_pass(bool on) { rendering_depth_pass = on; }

  void sort_objects(const Point3 &view_point);
  static bool should_build_optscene_data; // =true by default


protected:
  static bool checked_frustum;
  static bool enable_prerender;
  static bool rendering_depth_pass;

  SmallTab<int, MidmemAlloc> objIndices;

  Ptr<ShaderMatVdata> smvd;

  bool hasReflectionRefraction_;

  // Optimized scene data layout
  struct OptimizedScene
  {
    struct Elem;
    struct Mat;

  public:
    SmallTab<Mat, MidmemAlloc> mats;
    SmallTab<Elem, MidmemAlloc> elems;
    SmallTab<short, MidmemAlloc> elemIdxStor;
    VisibilityData visData;
    VisTree visTree;
    BuildVisTree buildVisTree;
    uint16_t stageEndMatIdx[SC_STAGE_IDX_MASK + 1];
    uint16_t usedElemsStride;
    bool inited;
    bool prepareDone;

    struct MarkElems;
    struct MarkElemsLods;
    struct MarkElemsHorClip;
    struct MarkElemsLodsHorClip;

    int getMatStartIdx(int start_stage) const
    {
      G_ASSERT_RETURN(start_stage >= 0 && start_stage <= SC_STAGE_IDX_MASK, 0xFFFF);
      return start_stage > 0 ? stageEndMatIdx[start_stage - 1] : 0;
    }
    int getMatEndIdx(int end_stage) const
    {
      G_ASSERT_RETURN(end_stage >= 0 && end_stage <= SC_STAGE_IDX_MASK, 0);
      return stageEndMatIdx[end_stage];
    }
  } optScn;

  static constexpr int MAX_CVIS = 8;
  carray<VisibilityData, MAX_CVIS> cVisData;

  void loadTextures(IGenLoad &cb, Tab<TEXTUREID> &tex);


  void clearRenderObjects();

  void buildOptSceneData();
  void prepareVisibilitySimple(VisibilityData *vd, int render_id, unsigned render_flags_mask, const VisibilityFinder &vf);

private:
  RenderScene(const RenderScene &);
  RenderScene &operator=(const RenderScene &);
};
