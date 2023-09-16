//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <vecmath/dag_vecMath.h>

// forward declarations of external classes
class RenderScene;
class Occlusion;
class VisibilityFinder;


class RenderSceneManager
{

  struct RenderSceneInfo
  {
    unsigned rsId;
    bool renderable;
  };

  Tab<RenderScene *> rs;
  Tab<RenderSceneInfo> rsInfo;

public:
  typedef bool (*reset_states_t)(const bbox3f &, bool, bool);

public:
  RenderSceneManager(int max_dump_reserve = 32);
  ~RenderSceneManager();

  void set_checked_frustum(bool checked);
  void allocPrepareVisCtx(int render_id);
  void doPrepareVisCtx(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask);
  void setPrepareVisCtxToRender(int render_id);
  void render(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask);
  void render(int render_id, unsigned render_flags_mask);
  void renderTrans();

  int loadScene(const char *lmdir, unsigned rs_id);
  int addScene(RenderScene *rs, unsigned rs_id);
  void delScene(unsigned rs_id);

  void delAllScenes();

  bool setSceneRenderable(unsigned rs_id, bool renderable); // false - not found
  bool isSceneRenderable(unsigned rs_id);

  dag::Span<RenderScene *> getScenes() { return make_span(rs); }
};
