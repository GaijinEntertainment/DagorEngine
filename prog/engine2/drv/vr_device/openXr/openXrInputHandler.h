#pragma once
#include <humanInput/dag_hiVrInput.h>

#include "openXr.h"
#include <EASTL/string.h>
#include <EASTL/vector.h>


class OpenXrInputHandler : public HumanInput::VrInput
{
public: // VrInput interface
  bool isInUsableState() const override;

  void setInitializationCallback(InitializationCallback callback) override;
  void setAndCallInitializationCallback(InitializationCallback callback) override;

  bool isInited() override { return inited; }
  bool canCustomizeBindings() override { return !isActionsInitComplete; }

  bool activateActionSet(ActionSetId action_set_id) override;
  void updateActionsState() override;
  bool clearInputActions() override;

  ActionSetIndex addActionSet(ActionSetId action_set_id, const char *name, int priority = 0,
    const char *localized_name = nullptr) override;
  ActionIndex addAction(ActionSetId action_set_id, ActionId action_id, ActionType type, const char *name,
    const char *localized_name = nullptr) override;
  bool setupHands(ActionSetId action_set_id, const char *localized_grip_pose_name, const char *localized_aim_pose_name,
    const char *localized_haptic_name) override;

  void suggestBinding(ActionId a_id, const char *controller, const char *path) override;
  void setupHapticsForBoundControllers(ActionSetId action_set_id, const char *localized_name) override;

  bool completeActionsInit() override;

  Bindings getCurrentBindings() const override { return currentBindings; }
  HumanInput::ButtonBits getCurrentBindingsMask(ActionId a) const override;
  ActionBindings getCurrentBindings(ActionId a) const override;
  eastl::string getBindingName(ActionBindingId b) const override;
  eastl::string getLocalizedBindingName(ActionBindingId b) const override;

  DigitalAction getDigitalActionState(ActionId a) const override;
  AnalogAction getAnalogActionState(ActionId a) const override;
  StickAction getStickActionState(ActionId a) const override;
  PoseAction getPoseActionState(ActionId a) const override;
  Hand getTrackedHand(Hands side) const override { return lastTrackedHands[side]; }
  Controller getControllerState(Hands side) const override { return lastControllerStates[side]; }

  void applyHapticFeedback(Hands side, const HapticSettings &settings = {}) const override;
  void stopHapticFeedback(Hands side) const override;

public:
  void init(XrInstance i, XrSession s, bool enable_hand_tracking);
  void shutdown();
  void updatePoseSpaces(XrSpace ref_space, XrTime t);
  void updateHandJoints(XrSpace ref_space, XrTime t);
  void updateLegacyControllerPoses(XrSpace ref_space, XrTime t);
  void updateCurrentBindings();

private:
  bool inited = false;
  bool isActionsInitComplete = false;
  XrInstance instance = 0;
  XrSession session = 0;
  ActionSetIndex activeActionSet = INVALID_ACTION_SET;

  Controller lastControllerStates[Hands::Total];
  Hand lastTrackedHands[Hands::Total];

  eastl::vector<XrAction> actions;
  eastl::vector<ActionId> actionIds;
  eastl::vector<XrActionType> actionTypes;
  eastl::vector<eastl::string> actionNames;
  ActionIndex findActionById(ActionId id, XrAction &out_action) const;
  ActionIndex findActionById(ActionId id) const
  {
    XrAction dummy;
    return findActionById(id, dummy);
  }

  eastl::vector<ActionIndex> poseActionIndices;
  eastl::vector<XrSpace> poseActionSpaces;
  eastl::vector<PoseAction> lastPoseActionStates;

  Bindings currentBindings;
  eastl::vector<HumanInput::ButtonBits> currentBindingMasks;
  void updateBindingMask(ActionIndex action, XrPath binding);

  eastl::vector<XrActionSet> actionSets;
  eastl::vector<ActionSetId> actionSetIds;
  eastl::vector<eastl::string> actionSetNames;
  ActionSetIndex findActionSetById(ActionSetId id, XrActionSet &out_action_set) const;
  ActionSetIndex findActionSetById(ActionSetId id) const
  {
    XrActionSet dummy;
    return findActionSetById(id, dummy);
  }

  void suggestBinding(XrAction action, const eastl::string &name, const char *controller, const char *path);

  // Legacy controller poses
  void setupLegacyControllerSpaces();
  XrAction gripPoseAction = XR_NULL_HANDLE;
  XrSpace gripSpaces[Hands::Total] = {XR_NULL_HANDLE, XR_NULL_HANDLE};

  XrAction aimPoseAction = XR_NULL_HANDLE;
  XrSpace aimSpaces[Hands::Total] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
  // End of legacy poses

  XrAction hapticAction = XR_NULL_HANDLE;

  XrHandTrackerEXT handTrackers[Hands::Total] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
  eastl::vector<XrHandJointLocationEXT> handJoints[Hands::Total];
  PFN_xrCreateHandTrackerEXT createHandTracker = nullptr;
  PFN_xrDestroyHandTrackerEXT destroyHandTracker = nullptr;
  PFN_xrLocateHandJointsEXT locateHandJoints = nullptr;
}; // class OpenXrInputHandler
