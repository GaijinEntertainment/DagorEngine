//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <anim/dag_animDecl.h>

class IAnimCharController
{
public:
  static constexpr unsigned HUID = 0x9E4074FFu; // IAnimCharController

  //! returns states count
  virtual int getStatesCount() = 0;
  //! returns state name
  virtual const char *getStateName(int idx) = 0;
  //! returns state index by name
  virtual int getStateIdx(const char *name) = 0;
  //! enqueues state
  virtual void enqueueState(int idx, float force_spd = -1.f) = 0;

  //! returns animation graph
  virtual AnimV20::AnimationGraph *getAnimGraph() const = 0;
  //! returns animation params state
  virtual AnimV20::IAnimStateHolder *getAnimState() const = 0;
  //! returns animated character
  virtual AnimV20::IAnimCharacter2 *getAnimChar() const = 0;

  //! returns true when animchar update is paused
  virtual bool isPaused() const = 0;
  //! pauses or resumes animchar update
  virtual void setPaused(bool paused = true) = 0;
  //! returns current timescale used for animchar update
  virtual float getTimeScale() const = 0;
  //! sets timescale for animchar update
  virtual void setTimeScale(float t_scale) = 0;
  //! resets animchar to initial state
  virtual void restart() = 0;

  //! returns IAnimBlendNode count
  virtual int getAnimNodeCount() = 0;
  //! returns IAnimBlendNode name
  virtual const char *getAnimNodeName(int idx) = 0;
  //! enqueues IAnimBlendNode
  virtual bool enqueueAnimNode(const char *anim_node_nm) = 0;

  //! returns true when specified asset is referenced from attachments
  virtual bool hasReferencesTo(const DagorAsset *a) = 0;
};
