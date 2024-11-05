//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <3d/dag_occlusionSystem.h>
#include <3d/dag_textureIDHolder.h>
#include <generic/dag_span.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#endif

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN && defined(_DEBUG_TAB_) && !DAGOR_THREAD_SANITIZER
#define CAN_DEBUG_OCCLUSION 1
#endif

#if CAN_DEBUG_OCCLUSION
#include <generic/dag_tabWithLock.h>
#include <osApiWrappers/dag_atomic.h>
#endif

class Occlusion
{
public:
  Occlusion() = default;
  void init()
  {
    occ = OcclusionSystem::create();
    occ->init();
  }
  void close() { del_it(occ); }
  ~Occlusion() { close(); }
  inline void reset() { occ->reset(); }
  inline operator bool() const { return occ != nullptr; }
  inline void prepareDebug()
  {
    if (!occ)
      return;
    occ->prepareDebug();
    occ->prepareDebugSWRasterization();
  }
  inline void setReprojectionUseCameraTranslatedSpace(bool enabled)
  {
    if (!occ)
      return;
    occ->setReprojectionUseCameraTranslatedSpace(enabled);
  }
  inline bool getReprojectionUseCameraTranslatedSpace() const
  {
    if (!occ)
      return false;
    return occ->getReprojectionUseCameraTranslatedSpace();
  }
  inline vec3f getCurViewPos() const { return curViewPos; }
  inline mat44f getCurViewProj() const { return curViewProj; }
  inline mat44f getCurView() const { return curView; }
  inline mat44f getCurProj() const { return curProj; }
  inline float getCockpitDistance() const { return cockpitDistance; }
  inline mat44f getCockpitAnim() const { return cockpitAnim; }
  inline CockpitReprojectionMode getCockpitReprojectionMode() const { return cockpitMode; }
  inline void setFrameParams(vec3f viewPos, mat44f_cref view, mat44f_cref proj, mat44f_cref viewProj)
  {
    curView = view;
    curProj = proj;
    curViewPos = viewPos;
    curViewProj = viewProj;
  }
  inline void setFrameParams(vec3f viewPos, mat44f_cref view, mat44f_cref proj, mat44f_cref viewProj, float cockpit_distance)
  {
    curView = view;
    curProj = proj;
    curViewProj = viewProj;
    curViewPos = viewPos;
    cockpitDistance = cockpit_distance;
  }
  inline void resetStats()
  {
#if CAN_DEBUG_OCCLUSION
    objects[0] = objects[1] = objects[2] = 0;
    if (storeOccludees)
    {
      debugNotOccludedBoxes.clear();
      debugOccludedBoxes.clear();
    }
#endif
  }
  inline void startFrame(vec3f viewPos, mat44f_cref view, mat44f_cref proj, mat44f_cref viewProj, float cockpit_distance,
    CockpitReprojectionMode cockpit_mode, const mat44f &cockpit_anim, float, float)
  {
    rasterizedQuadOccluders = rasterizedBoxOccluders = rasterizedTriOccluders = 0;
    cockpitMode = cockpit_mode;
    cockpitAnim = cockpit_anim;
    hasMergedOcclusion = false;
    resetStats();
    setFrameParams(viewPos, view, proj, viewProj, cockpit_distance);
  }
  inline void readbackGPU() { occ->readGPUframe(); }

  inline void reprojectGPUFrame()
  {
    occ->prepareGPUDepthFrame(curViewPos, curView, curProj, curViewProj, cockpitDistance, cockpitMode, cockpitAnim);
  }
  inline void clear() { occ->clear(); }

  inline void prepare(vec3f viewPos, mat44f_cref view, mat44f_cref proj, mat44f_cref viewProj, float cockpit_distance,
    CockpitReprojectionMode cockpit_mode, const mat44f &cockpit_anim, float threshold_x, float threshold_y)
  {
    startFrame(viewPos, view, proj, viewProj, cockpit_distance, cockpit_mode, cockpit_anim, threshold_x, threshold_y);
    readbackGPU();
    reprojectGPUFrame();
  }

  bool hasGPUFrame() const { return occ->hasGPUFrame(); }
  void prepareNextFrame(float zn, float zf, TextureIDPair mipped_depth, Texture *depth = nullptr)
  {
    occ->prepareNextFrame(curViewPos, curView, curProj, curViewProj, zn, zf, mipped_depth, depth);
  }

  void prepareNextFrame(float zn, float zf, TextureIDPair mipped_depth, Texture *depth, StereoIndex stereo_index)
  {
    occ->prepareNextFrame(curViewPos, curView, curProj, curViewProj, zn, zf, mipped_depth, depth, stereo_index);
  }

  VECTORCALL bool isVisibleSphere(vec3f sph_center, vec4f radius, vec4f threshold = v_zero()) const
  {
    // todo: replace with correct sphere rect determination
    return isVisibleBox(v_sub(sph_center, radius), v_add(sph_center, radius), threshold);
  }
  // inline bool isVisibleExtent2Quad(vec3f center2, vec3f extent2, uint32_t mask) const{return false;}
  // inline bool isVisibleExtent2Box(vec3f center2, vec3f extent2, uint32_t mask) const{return false;}
  VECTORCALL bool isVisibleBoxExtent2(vec3f center2, vec3f extent2, vec4f threshold = v_zero()) const
  {
    center2 = v_mul(center2, V_C_HALF);
    extent2 = v_mul(extent2, V_C_HALF);
    return isVisibleBox(v_sub(center2, extent2), v_add(center2, extent2), threshold);
  }

  VECTORCALL bool isVisibleBox(vec3f bmin, vec3f bmax, vec4f threshold = v_zero()) const
  {
    int visibility = occ->testVisibility(bmin, bmax, threshold, curViewProj);
#if CAN_DEBUG_OCCLUSION
    interlocked_increment(objects[visibility]);
#endif
    return visibility == occ->VISIBLE;
  }
  VECTORCALL bool isVisibleBox(vec3f bmin, vec3f bmax, mat44f_cref worldViewProjTm, vec4f threshold = v_zero()) const
  {
    int visibility = occ->testVisibility(bmin, bmax, threshold, worldViewProjTm);
#if CAN_DEBUG_OCCLUSION
    interlocked_increment(objects[visibility]);
#endif
    return (visibility == occ->VISIBLE);
  }
  VECTORCALL bool isVisibleBox(bbox3f_cref box, vec4f threshold = v_zero()) const
  {
    return isVisibleBox(box.bmin, box.bmax, threshold);
  }


  VECTORCALL bool isOccludedSphere(vec3f sph_center, vec4f radius) const
  {
    // todo: replace with correct sphere rect determination
    return isOccludedBox(v_sub(sph_center, radius), v_add(sph_center, radius));
  }
  VECTORCALL bool isOccludedBoxExtent(vec3f center, vec3f extent) const
  {
    return isOccludedBox(v_sub(center, extent), v_add(center, extent));
  }
  VECTORCALL bool isOccludedBoxExtent2(vec3f center2, vec3f extent2) const
  {
    return isOccludedBoxExtent(v_mul(center2, V_C_HALF), v_mul(extent2, V_C_HALF));
  }

  VECTORCALL bool isOccludedBox(vec3f bmin, vec3f bmax) const
  {
    const int visibility = occ->testVisibility(bmin, bmax, v_zero(), curViewProj);
#if CAN_DEBUG_OCCLUSION
    if (visibility != occ->CULL_OCCLUSION)
    {
      if (storeOccludees && storeOccludeesNotOccluded)
      {
        debugNotOccludedBoxes.lock();
        bbox3f box;
        box.bmin = bmin;
        box.bmax = bmax;
        debugNotOccludedBoxes.push_back(box);
        debugNotOccludedBoxes.unlock();
      }
      interlocked_increment(objects[occ->VISIBLE]);
      return false;
    }
    if (storeOccludees)
    {
      debugOccludedBoxes.lock();
      bbox3f box;
      box.bmin = bmin;
      box.bmax = bmax;
      debugOccludedBoxes.push_back(box);
      debugOccludedBoxes.unlock();
    }
    interlocked_increment(objects[occ->CULL_OCCLUSION]);
    return true;
#else
    return visibility == occ->CULL_OCCLUSION;
#endif
  }
  VECTORCALL bool isOccludedBox(vec3f bmin, vec3f bmax, mat44f_cref worldViewProjTm) const
  {
    const int visibility = occ->testVisibility(bmin, bmax, v_zero(), worldViewProjTm);
#if CAN_DEBUG_OCCLUSION
    if (visibility != occ->CULL_OCCLUSION)
    {
      interlocked_increment(objects[occ->VISIBLE]);
      return false;
    }
    interlocked_increment(objects[occ->CULL_OCCLUSION]);
    return true;
#else
    return visibility == occ->CULL_OCCLUSION;
#endif
  }
  VECTORCALL bool isOccludedBox(bbox3f_cref box) const { return isOccludedBox(box.bmin, box.bmax); }

  void startSWOcclusion(float zn)
  {
    occ->initSWRasterization();
    occ->startRasterization(zn);
  }
  void finalizeBoxes(const bbox3f *box, const mat44f *mat, int cnt)
  {
    rasterizedBoxOccluders += cnt;
    occ->rasterizeBoxes(curViewProj, box, mat, cnt);
  }
  void finalizeQuads(const vec4f *verts, int cnt) // verts size is cnt*4
  {
    rasterizedQuadOccluders += cnt;
    occ->rasterizeQuads(curViewProj, verts, cnt);
  }
  void rasterizeMesh(mat44f_cref viewproj, vec3f bmin, vec3f bmax, const vec4f *verts, int vert_cnt, const unsigned short *faces,
    int tri_count)
  {
    rasterizedTriOccluders += tri_count;
    occ->rasterizeMesh(viewproj, bmin, bmax, verts, vert_cnt, faces, tri_count);
  }
  void finalizeSWOcclusion()
  {
    if (rasterizedQuadOccluders || rasterizedBoxOccluders || rasterizedTriOccluders || hasMergedOcclusion)
      occ->combineWithSWRasterization();
  }
  void finalizeOcclusion() { occ->buildMips(); }
  void mergeOcclusionsZmin(MaskedOcclusionCulling **another_occl, uint32_t occl_count, uint32_t first_tile, uint32_t last_tile)
  {
    occ->mergeOcclusionsZmin(another_occl, occl_count, first_tile, last_tile);
  }
  void finalizeSWOcclusionMerge() { hasMergedOcclusion = true; }
  void getMaskedResolution(uint32_t &width, uint32_t &height) const { occ->getMaskedResolution(width, height); }

#if CAN_DEBUG_OCCLUSION
  dag::ConstSpan<bbox3f> getDebugOccludedBoxes() const { return debugOccludedBoxes; }
  dag::ConstSpan<bbox3f> getDebugNotOccludedBoxes() const { return debugNotOccludedBoxes; }
  inline void setStoreOccludees(bool on) { storeOccludees = on; }
  inline bool getStoreOccludees() const { return storeOccludees; }
  inline void setStoreNotOccluded(bool on) { storeOccludeesNotOccluded = on; }
  inline bool getStoreNotOccluded() const { return storeOccludeesNotOccluded; }
  inline int getOccludedObjectsCount() const { return objects[occ->CULL_OCCLUSION]; }
  inline int getFrustumCulledObjectsCount() const { return objects[occ->CULL_FRUSTUM]; }
  inline int getVisibleObjectsCount() const { return objects[occ->VISIBLE]; }
  inline int getRasterizedBoxOccluders() const { return rasterizedBoxOccluders; }
  inline int getRasterizedQuadOccluders() const { return rasterizedQuadOccluders; }
#else
  dag::ConstSpan<bbox3f> getDebugOccludedBoxes() const { return {}; }
  dag::ConstSpan<bbox3f> getDebugNotOccludedBoxes() const { return {}; }
  inline void setStoreOccludees(bool) {}
  inline bool getStoreOccludees() const { return false; }
  inline void setStoreNotOccluded(bool) {}
  inline bool getStoreNotOccluded() const { return false; }
  inline int getOccludedObjectsCount() const { return 0; }
  inline int getFrustumCulledObjectsCount() const { return 0; }
  inline int getVisibleObjectsCount() const { return 0; }
  inline int getRasterizedBoxOccluders() const { return 0; }
  inline int getRasterizedQuadOccluders() const { return 0; }
#endif
protected:
  mat44f curViewProj = {};
  mat44f curView = {};
  mat44f curProj = {};
  vec3f curViewPos = v_zero();
  OcclusionSystem *occ = 0;
  float cockpitDistance = 0;
  CockpitReprojectionMode cockpitMode = COCKPIT_NO_REPROJECT;
  mat44f cockpitAnim = {};
  int rasterizedBoxOccluders = 0, rasterizedQuadOccluders = 0, rasterizedTriOccluders = 0;
  bool hasMergedOcclusion = false;
#if CAN_DEBUG_OCCLUSION
  bool storeOccludees = false, storeOccludeesNotOccluded = false;
  mutable TabWithLock<bbox3f> debugOccludedBoxes;
  mutable TabWithLock<bbox3f> debugNotOccludedBoxes;
  mutable int objects[3] = {0};
#endif
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#undef CAN_DEBUG_OCCLUSION

#include <supp/dag_define_KRNLIMP.h>

extern KRNLIMP Occlusion *current_occlusion;

void save_occlusion(class IGenSave &cb, const Occlusion &occlusion); // for debug purposes
bool load_occlusion(class IGenLoad &cb, Occlusion &occlusion);       // for debug purposes

#include <supp/dag_undef_KRNLIMP.h>
