//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DagorGameScene
{
public:
  // dtor
  virtual ~DagorGameScene() {}

  //! called to act world
  virtual void actScene() = 0;

  //! called to render scene
  virtual void drawScene() = 0;

  //! called just before drawScene()
  virtual void beforeDrawScene(int /*realtime_elapsed_usec*/, float /*gametime_elapsed_sec*/) {}

  //! called when scene has been selected as current
  virtual void sceneSelected(DagorGameScene * /*prev_scene*/) {}
  //! called before current scene is replaced by the new one
  virtual void sceneDeselected(DagorGameScene * /*new_scene*/) {}

  virtual void enableStereo(bool enable) { stereoEnabled = enable; }

  virtual bool canPresentAndReset() { return true; }

protected:
  bool stereoEnabled = false;
};


//! selects game scene
//! during switch (between prev->sceneDeselected() and next->sceneSelected() calls) NULL scene
//! will be set
void dagor_select_game_scene(DagorGameScene *scene);

//! returns currently selected game scene
DagorGameScene *dagor_get_current_game_scene();

//! Sets a secondary game scene, which is running on top of the primary
void dagor_select_secondary_game_scene(DagorGameScene *scene);

//! Swaps the primary and secondary scenes
void dagor_swap_scenes();

//! Returns the secondary game scene. It is the scene which was active before setting a scene which still have loading workload.
DagorGameScene *dagor_get_secondary_game_scene();
