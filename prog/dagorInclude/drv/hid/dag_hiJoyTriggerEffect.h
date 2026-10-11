//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

namespace HumanInput
{
// constraint: byte for byte the layout of Sony's ScePadTriggerEffectParam (pad_trigger_effect.h, isteamdualsense.h)
enum TriggerEffectMode : uint32_t
{
  TRIGGER_EFFECT_MODE_OFF,
  TRIGGER_EFFECT_MODE_FEEDBACK,
  TRIGGER_EFFECT_MODE_WEAPON,
  TRIGGER_EFFECT_MODE_VIBRATION,
  TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_FEEDBACK,
  TRIGGER_EFFECT_MODE_SLOPE_FEEDBACK,
  TRIGGER_EFFECT_MODE_MULTIPLE_POSITION_VIBRATION,
};

enum
{
  TRIGGER_EFFECT_MASK_L2 = 0x01,
  TRIGGER_EFFECT_MASK_R2 = 0x02,
  TRIGGER_EFFECT_INDEX_L2 = 0,
  TRIGGER_EFFECT_INDEX_R2 = 1,
  TRIGGER_EFFECT_TRIGGER_NUM = 2,
  TRIGGER_EFFECT_CONTROL_POINT_NUM = 10,
};

struct TriggerEffectCommand
{
  TriggerEffectMode mode = TRIGGER_EFFECT_MODE_OFF;
  uint8_t padding[4] = {};
  union
  {
    struct
    {
      uint8_t position; // 0..9, where the resistance starts
      uint8_t strength; // 0..8, 0 is off
    } feedback;
    struct
    {
      uint8_t startPosition; // 2..7
      uint8_t endPosition;   // startPosition+1..8
      uint8_t strength;      // 0..8, 0 is off
    } weapon;
    struct
    {
      uint8_t position;  // 0..9
      uint8_t amplitude; // 0..8, 0 is off
      uint8_t frequency; // 0..255 Hz, 0 is off
    } vibration;
    struct
    {
      uint8_t strength[TRIGGER_EFFECT_CONTROL_POINT_NUM]; // 0..8 per position
    } multiplePositionFeedback;
    struct
    {
      uint8_t startPosition; // 0..endPosition-1
      uint8_t endPosition;   // startPosition+1..9
      uint8_t startStrength; // 1..8
      uint8_t endStrength;   // 1..8
    } slopeFeedback;
    struct
    {
      uint8_t frequency;                                   // 0..255 Hz, 0 is off
      uint8_t amplitude[TRIGGER_EFFECT_CONTROL_POINT_NUM]; // 0..8 per position
    } multiplePositionVibration;
    uint8_t raw[48] = {};
  };
};

// only the triggers in triggerMask change, the others keep their effect; a zero mask changes nothing, and
// TRIGGER_EFFECT_MODE_OFF on a selected trigger stops its effect
struct TriggerEffectParam
{
  uint8_t triggerMask = 0; // TRIGGER_EFFECT_MASK_L2 | TRIGGER_EFFECT_MASK_R2
  uint8_t padding[7] = {};
  TriggerEffectCommand command[TRIGGER_EFFECT_TRIGGER_NUM];
};
static_assert(sizeof(TriggerEffectCommand) == 56);
static_assert(sizeof(TriggerEffectParam) == 120);
} // namespace HumanInput
