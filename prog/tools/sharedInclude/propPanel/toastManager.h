//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <util/dag_simpleString.h>

namespace PropPanel
{

enum class ToastType
{
  None,
  Alert,
  Success,
};

class ToastMessage
{
public:
  ToastType type = ToastType::None;
  uint64_t opaqueIntervalMs = 3000;
  uint64_t fadeoutIntervalMs = 500;
  bool fadeoutOnlyOnMouseLeave = false;

  SimpleString text;

  Point2 moveRequestPosition = Point2::ZERO;
  Point2 moveRequestPivot = Point2::ZERO;

  void set(uint64_t _id, uint64_t set_time_ms);

  void setPosition(const IPoint2 &position, const Point2 &pivot);
  void setToMousePos(const Point2 &pivot);
  void setToCenterPos(const Point2 &pivot);
  void setSizeHint(Point2 size_hint);

  // The toast message will disappear instantly instead of fading out.
  void disableFadeOutAnimation() { fadeoutIntervalMs = 0; }
  bool isFadeOutAnimationDisabled() const { return fadeoutIntervalMs == 0; }

  // The toast message will not disappear after a certain amount of time has passed.
  void keepVisibleIndefinitely() { opaqueIntervalMs = 0; }
  bool isVisibleIndefinitely() const { return opaqueIntervalMs == 0; }

  // If enabled then moving the mouse will trigger hiding the toast message.
  void setHideOnMouseMove(bool hide_on_mouse_move) { hideOnMouseMove = hide_on_mouse_move; }

  void startFadeout(uint64_t fadeout_time_ms);
  bool isFadeoutStarted() const { return fadeoutTimeMs > 0; }

  uint64_t getId() const { return id; }
  uint64_t getSetTimeMs() const { return setTimeMs; }
  uint64_t getFadeoutTimeMs() const { return fadeoutTimeMs; }

  bool beforeToast(uint64_t time_ms, float &alpha);
  void updateToast(uint64_t time_ms);

  static constexpr uint64_t INVALID_ID = -1;

private:
  uint64_t id = INVALID_ID;
  uint64_t setTimeMs = 0;
  uint64_t fadeoutTimeMs = 0;
  uint32_t updates = 0;
  Point2 sizeHint = Point2::ZERO;
  Point2 mouseStartPosition = Point2(VERY_BIG_NUMBER, VERY_BIG_NUMBER);
  bool moveRequested = false;
  bool hideOnMouseMove = false;
};

void load_toast_message_icons();

// Returns with the ID of the toast message.
uint64_t set_toast_message(const ToastMessage &message);

// Instantly destroy the toast message. The fade out animation will not be played.
void remove_toast_message(uint64_t id);

const ToastMessage *get_toast_message(uint64_t id);

void render_toast_messages();

void show_toast_debug_panel();

} // namespace PropPanel