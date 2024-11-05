// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{

struct BhvMarqueeData;

class BhvMarquee : public darg::Behavior
{
public:
  enum Phase
  {
    PHASE_INITIAL_DELAY,
    PHASE_MOVE,
    PHASE_FADE_OUT
  };

  BhvMarquee();

  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;
  virtual int update(UpdateStage stage, darg::Element *elem, float /*dt*/) override;
  virtual void onElemSetup(Element *, SetupMode setup_mode) override;
};


struct BhvMarqueeData
{
  // variables
  BhvMarquee::Phase phase = BhvMarquee::PHASE_INITIAL_DELAY;
  float delay = 0;
  float pos = 0; // to keep position on recalc
  bool haveSavedPos = false;

  // params
  bool loop = false;
  float threshold = 5.0f;
  Orientation orient = O_HORIZONTAL;
  float scrollSpeed = defSpeed;
  float initialDelay = 1.0f;
  float fadeOutDelay = 1.0f;
  bool scrollOnHover = false;

  void resetPhase()
  {
    phase = BhvMarquee::PHASE_INITIAL_DELAY;
    delay = initialDelay;
  }

  void resetPhase(BhvMarquee::Phase phase_, float delay_)
  {
    phase = phase_;
    delay = delay_;
  }

  static const float defSpeed;
};


extern BhvMarquee bhv_marquee;


} // namespace darg
