//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/hid/dag_hiJoyData.h>

#include <util/dag_stdint.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_Quat.h>
#include <math/dag_TMatrix.h>

#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>

class DataBlock;

namespace HumanInput
{

struct VrInput
{
  virtual ~VrInput() = default;

  virtual bool isInUsableState() const { return true; }

  using InitializationCallback = eastl::function<void()>;
  virtual void setInitializationCallback(InitializationCallback /* callback */) {}
  virtual void setAndCallInitializationCallback(InitializationCallback /* callback */) {}

  virtual bool isInited() { return false; }
  virtual bool canCustomizeBindings() = 0;

  using ActionId = uint16_t;
  static const ActionId INVALID_ACTION_ID = 0xFFFF;

  using ActionBindingId = uint64_t;
  static const ActionBindingId INVALID_ACTION_BINDING_ID = 0;
  static const uint8_t MAX_ACTION_BINDING_SOURCES = 8;
  using ActionBindings = eastl::array<ActionBindingId, MAX_ACTION_BINDING_SOURCES>;
  using Bindings = eastl::vector<eastl::pair<ActionId, ActionBindings>>;

  using ActionSetId = uint16_t;
  static const ActionSetId INVALID_ACTION_SET_ID = 0xFFFF;

  virtual bool activateActionSet(ActionSetId action_set_id) = 0;
  virtual void updateActionsState() = 0;
  virtual bool clearInputActions() = 0;

  using ActionSetIndex = int;
  static const ActionSetIndex INVALID_ACTION_SET = -1;
  virtual ActionSetIndex addActionSet(ActionSetId action_set_id, const char *name, int priority = 0,
    const char *localized_name = nullptr) = 0;

  using ActionIndex = int;
  static const ActionIndex INVALID_ACTION = -1;
  enum class ActionType
  {
    DIGITAL,
    ANALOG,
    STICK,
    POSE
  };
  virtual ActionIndex addAction(ActionSetId action_set_id, ActionId action_id, ActionType type, const char *name,
    const char *localized_name = nullptr) = 0;

  virtual void suggestBinding(ActionId a_id, const char *controller, const char *path) = 0;

  virtual void setupHapticsForBoundControllers(ActionSetId /*action_set_id*/, const char * /*loc*/) {}
  virtual bool completeActionsInit() = 0;

  virtual Bindings getCurrentBindings() const = 0;
  virtual ButtonBits getCurrentBindingsMask(ActionId a) const = 0;
  virtual ActionBindings getCurrentBindings(ActionId a) const = 0;
  virtual eastl::string getBindingName(ActionBindingId b) const = 0;
  virtual eastl::string getLocalizedBindingName(ActionBindingId b) const = 0;

  // Get Last Action States
  struct DigitalAction
  {
    bool isActive = false;
    bool state = false;
    bool hasChanged = false;
  };
  virtual DigitalAction getDigitalActionState(ActionId a) const = 0;

  struct AnalogAction
  {
    bool isActive = false;
    float state = 0.f;
    bool hasChanged = false;
  };
  virtual AnalogAction getAnalogActionState(ActionId a) const = 0;

  struct StickAction
  {
    bool isActive = false;
    Point2 state = {0.f, 0.f};
    bool hasChanged = false;
  };
  virtual StickAction getStickActionState(ActionId a) const = 0;

  struct PoseAction
  {
    bool isActive = false;
    DPoint3 pos{0.0, 0.0, 0.0};
    Quat rot{0.f, 0.f, 0.f, 1.f};
    DPoint3 angularVel{0.0, 0.0, 0.0};
    DPoint3 linearVel{0.0, 0.0, 0.0};
    bool hasChanged = false;

    friend bool operator==(const PoseAction &lhs, const PoseAction &rhs)
    {
      return lhs.isActive == rhs.isActive && lhs.rot == rhs.rot && lhs.pos == rhs.pos;
    }
    friend bool operator!=(const PoseAction &lhs, const PoseAction &rhs) { return !(lhs == rhs); }
  };
  virtual PoseAction getPoseActionState(ActionId a) const = 0;


  enum Hands
  {
    Left,
    Right,
    Total
  };

  struct Pose
  {
    DPoint3 pos{0.0, 0.0, 0.0};
    Quat rot{0.f, 0.f, 0.f, 1.f};
    DPoint3 angularVel{0.0, 0.0, 0.0};
    DPoint3 linearVel{0.0, 0.0, 0.0};

    TMatrix getTm() const
    {
      TMatrix m = makeTM(rot);
      m.setcol(3, pos);
      return m;
    }
  };


  // Compatibility only! Do not use in new code!
  virtual bool setupHands(ActionSetId action_set_id, const char *localized_grip_pose_name, const char *localized_aim_pose_name,
    const char *localized_haptic_name) = 0;
  struct Controller
  {
    bool isActive = false;
    Pose grip;
    Pose aim;
    bool hasChanged = false;

    friend bool operator==(const Controller &lhs, const Controller &rhs)
    { // aim is connected to grip, but is more sensitive and can be ignored
      return lhs.isActive == rhs.isActive && lhs.grip.rot == rhs.grip.rot && lhs.grip.pos == rhs.grip.pos;
    }
    friend bool operator!=(const Controller &lhs, const Controller &rhs) { return !(lhs == rhs); }
  };
  virtual Controller getControllerState(Hands side) const = 0;
  // End of compatiblity interface


  // Order and quantity matches those of OpenXr, which is our only implementation atm
  enum Joints
  {
    VRIN_HAND_JOINT_PALM = 0,
    VRIN_HAND_JOINT_WRIST = 1,

    VRIN_HAND_JOINT_THUMB_METACARPAL = 2,
    VRIN_HAND_JOINT_THUMB_PROXIMAL = 3,
    VRIN_HAND_JOINT_THUMB_DISTAL = 4,
    VRIN_HAND_JOINT_THUMB_TIP = 5,

    VRIN_HAND_JOINT_INDEX_METACARPAL = 6,
    VRIN_HAND_JOINT_INDEX_PROXIMAL = 7,
    VRIN_HAND_JOINT_INDEX_INTERMEDIATE = 8,
    VRIN_HAND_JOINT_INDEX_DISTAL = 9,
    VRIN_HAND_JOINT_INDEX_TIP = 10,

    VRIN_HAND_JOINT_MIDDLE_METACARPAL = 11,
    VRIN_HAND_JOINT_MIDDLE_PROXIMAL = 12,
    VRIN_HAND_JOINT_MIDDLE_INTERMEDIATE = 13,
    VRIN_HAND_JOINT_MIDDLE_DISTAL = 14,
    VRIN_HAND_JOINT_MIDDLE_TIP = 15,

    VRIN_HAND_JOINT_RING_METACARPAL = 16,
    VRIN_HAND_JOINT_RING_PROXIMAL = 17,
    VRIN_HAND_JOINT_RING_INTERMEDIATE = 18,
    VRIN_HAND_JOINT_RING_DISTAL = 19,
    VRIN_HAND_JOINT_RING_TIP = 20,

    VRIN_HAND_JOINT_LITTLE_METACARPAL = 21,
    VRIN_HAND_JOINT_LITTLE_PROXIMAL = 22,
    VRIN_HAND_JOINT_LITTLE_INTERMEDIATE = 23,
    VRIN_HAND_JOINT_LITTLE_DISTAL = 24,
    VRIN_HAND_JOINT_LITTLE_TIP = 25,

    VRIN_HAND_JOINTS_TOTAL
  };

  struct Hand
  {
    struct Joint
    {
      Pose pose;
      float radius = 0.f;
    };
    eastl::vector<Joint> joints;
    bool isActive = false;
  };
  virtual Hand getTrackedHand(Hands side) const = 0;


  struct HapticSettings
  {
    static constexpr int MIN_DURATION = -1;
    static constexpr float UNSPECIFIED_FREQUENCY = 0;

    int durationNs = MIN_DURATION;
    float frequency = UNSPECIFIED_FREQUENCY;
    float amplitude = 0.1f;
  };
  virtual void applyHapticFeedback(Hands /*side*/, const HapticSettings & /*settings*/) const {};
  virtual void stopHapticFeedback(Hands /*side*/) const {};
  virtual void stopAllHapticFeedback() const
  {
    stopHapticFeedback(Hands::Left);
    stopHapticFeedback(Hands::Right);
  }
};

} // namespace HumanInput
