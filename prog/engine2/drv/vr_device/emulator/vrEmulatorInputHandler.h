#pragma once
#include <humanInput/dag_hiVrInput.h>


class VrEmulatorInputHandler : public HumanInput::VrInput
{
public: // VrInput interface
  void setInitializationCallback(InitializationCallback callback) override;
  void setAndCallInitializationCallback(InitializationCallback callback) override;

  bool isInited() override { return true; }
  bool canCustomizeBindings() override;

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
  bool completeActionsInit() override;

  Bindings getCurrentBindings() const override { return {}; }
  HumanInput::ButtonBits getCurrentBindingsMask(ActionId a) const override;
  ActionBindings getCurrentBindings(ActionId a) const override;
  eastl::string getBindingName(ActionBindingId b) const override;
  eastl::string getLocalizedBindingName(ActionBindingId b) const override;

  DigitalAction getDigitalActionState(ActionId a) const override;
  AnalogAction getAnalogActionState(ActionId a) const override;
  Controller getControllerState(Hands side) const override;
  StickAction getStickActionState(ActionId a) const override;
  PoseAction getPoseActionState(ActionId a) const override;
  Hand getTrackedHand(Hands side) const override;

  void applyHapticFeedback(Hands side, const HapticSettings &settings = {}) const override;
  void stopHapticFeedback(Hands side) const override;

private:
  mutable float rX = 0.919999480;
  mutable float rY = -0.719999671;
}; // class VrEmulatorInputHandler
