//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

// All functions here are main thread only.

//! state of the hinge of a foldable device
enum class HingeStatus
{
  NONE,           //!< the device has no hinge, or the platform gives no hinge state
  UNKNOWN,        //!< the device has a hinge, but its state is not known now
  CLOSED,         //!< folded shut, the app is on the outer display
  PARTIALLY_OPEN, //!< open at an angle, the fold divides the inner display
  FULLY_OPEN,     //!< open as far as the device allows
};

struct HingeState
{
  HingeStatus status = HingeStatus::NONE;
  float angle = 0.f; //!< in radians; the platform sets the update rate and precision
};

class IHingeListener
{
public:
  virtual ~IHingeListener() = default;
  //! called on the main thread when the status or the angle changes
  virtual void onHingeChanged(const HingeState &state) = 0;
};

//! returns the last state that the platform reported
KRNLIMP HingeState get_hinge_state();

//! the platform layer reports a new state; the listeners are called if it differs from the last one
KRNLIMP void set_hinge_state(const HingeState &state);

//! stores the raw pointer: call del_hinge_listener() before the listener is destroyed. Up to 8 listeners, more are ignored
KRNLIMP void add_hinge_listener(IHingeListener *listener);
KRNLIMP void del_hinge_listener(IHingeListener *listener);

#include <supp/dag_undef_KRNLIMP.h>
