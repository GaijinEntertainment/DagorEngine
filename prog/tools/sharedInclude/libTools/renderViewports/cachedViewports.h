// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <3d/dag_texMgr.h>

class RenderViewport;
class BaseTexture;
typedef BaseTexture Texture;
struct Driver3dRenderTarget;


class CachedRenderViewports
{
public:
  CachedRenderViewports();
  ~CachedRenderViewports();


  // addes new viewport to list and returns its index
  int addViewport(RenderViewport *vp, void *userdata, bool enabled);

  // removes viewport from list
  void removeViewport(int vp_idx);

  // enables viewport caching
  inline void enableViewportCache(int vp_idx, bool use) { viewports[vp_idx].useCache = use; }

  // forces viewport caching
  inline void forceViewportCache(int vp_idx, bool force) { viewports[vp_idx].forceCache = force; }

  // disables viewport
  inline void disableViewport(int vp_idx)
  {
    viewports[vp_idx].needDraw = false;
    viewports[vp_idx].useCache = false;
  }

  // invalidates cache (forces redraw on next render)
  inline void invalidateViewportCache(int vp_idx) { viewports[vp_idx].needDraw = true; }

  // number of viewports (useful for interating startRended/endRender)
  inline int viewportsCount() { return viewports.size(); }

  // returns pointer to RenderViewport data
  inline RenderViewport *getViewport(int vp_idx) { return viewports[vp_idx].vp; }

  // returns pointer to user data
  inline void *getViewportUserData(int vp_idx) { return viewports[vp_idx].userData; }

  // returns whether cache used for viewport
  inline bool isViewportCacheUsed(int vp_idx) { return viewports[vp_idx].useCache; }

  // returns index of currently rendering viewport or -1 when not in render
  inline int getCurRenderVp() { return curRenderVp; }

  // Sets scale of each viewport texture
  inline void setVpTexScale(const Point2 &scale) { texScale = scale; }

  // Returns scale viewport texture
  inline Point2 getVpTexScale() const { return texScale; }

  // setups d3d for each viewport before rendering;
  // returns "true" when scene MUST BE rendered
  // "false" means that cache is valid and used and scene redraw not needed
  bool startRender(int vp_idx);

  // ends viewport rendering (must be called only if startRender returns "true")
  void endRender();


protected:
  struct VpRec
  {
    RenderViewport *vp;
    void *userData;
    bool needDraw;
    bool useCache;
    bool forceCache;
  };
  Tab<VpRec> viewports;
  int curRenderVp;

  // context for render viewports
  int rtWidth, rtHeight;
  int prevX, prevY, prevW, prevH;
  float minz, maxz;
  Point2 texScale;
  Driver3dRenderTarget *lastRt;
};
