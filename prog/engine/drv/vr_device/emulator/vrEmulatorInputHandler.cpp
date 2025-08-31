// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vrEmulatorInputHandler.h"
#include "vrEmulator.h"

#if _TARGET_PC_WIN
#include <Windows.h>
#endif // _TARGET_PC_WIN

using VrInput = HumanInput::VrInput;
using ActionSetIndex = VrInput::ActionSetIndex;
using ActionIndex = VrInput::ActionIndex;
using DigitalAction = VrInput::DigitalAction;
using AnalogAction = VrInput::AnalogAction;
using StickAction = VrInput::StickAction;
using PoseAction = VrInput::PoseAction;
using Hands = VrInput::Hands;
using Hand = VrInput::Hand;
using Controller = VrInput::Controller;
using ActionBindings = VrInput::ActionBindings;

static VrInput::InitializationCallback inputInitializatonCallback;

void VrEmulatorInputHandler::setInitializationCallback(InitializationCallback callback) { inputInitializatonCallback = callback; }

void VrEmulatorInputHandler::setAndCallInitializationCallback(InitializationCallback callback)
{
  setInitializationCallback(callback);
  if (inputInitializatonCallback)
    inputInitializatonCallback();
}

bool VrEmulatorInputHandler::canCustomizeBindings() { return true; }

bool VrEmulatorInputHandler::activateActionSet(ActionSetId) { return true; }
void VrEmulatorInputHandler::updateActionsState() { /* STUB */ }
bool VrEmulatorInputHandler::clearInputActions() { return true; }

ActionSetIndex VrEmulatorInputHandler::addActionSet(ActionSetId, const char *, int, const char *) { return 1; }
ActionIndex VrEmulatorInputHandler::addAction(ActionSetId, ActionId, ActionType, const char *, const char *) { return 0; }
bool VrEmulatorInputHandler::setupHands(ActionSetId, const char *, const char *, const char *) { return true; }

void VrEmulatorInputHandler::suggestBinding(ActionId, const char *, const char *) { /* STUB */ }
bool VrEmulatorInputHandler::completeActionsInit() { return true; }

HumanInput::ButtonBits VrEmulatorInputHandler::getCurrentBindingsMask(ActionId) const { return {}; }
ActionBindings VrEmulatorInputHandler::getCurrentBindings(ActionId) const { return {}; }
eastl::string VrEmulatorInputHandler::getBindingName(ActionBindingId) const { return "undefined"; }
eastl::string VrEmulatorInputHandler::getLocalizedBindingName(ActionBindingId) const { return "undefined"; }

DigitalAction VrEmulatorInputHandler::getDigitalActionState(ActionId) const { return {}; }
AnalogAction VrEmulatorInputHandler::getAnalogActionState(ActionId) const { return {}; }
StickAction VrEmulatorInputHandler::getStickActionState(ActionId) const { return {}; }

PoseAction VrEmulatorInputHandler::getPoseActionState(ActionId a) const
{
  PoseAction pose = {};

  const Point3 &position = static_cast<VREmulatorDevice *>(VRDevice::getInstance())->GetPosition();
  const TMatrix &orientation = static_cast<VREmulatorDevice *>(VRDevice::getInstance())->GetOrientation();

  bool isLeft = a == 0;

#if _TARGET_PC_WIN
  bool isCtrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;
  if (isCtrl)
  {
    if (GetAsyncKeyState(VK_NUMPAD4) & 0x8000)
      rY += 0.01f;
    if (GetAsyncKeyState(VK_NUMPAD6) & 0x8000)
      rY -= 0.01f;
    if (GetAsyncKeyState(VK_NUMPAD8) & 0x8000)
      rX += 0.01f;
    if (GetAsyncKeyState(VK_NUMPAD2) & 0x8000)
      rX -= 0.01f;
  }
#endif // _TARGET_PC_WIN

  TMatrix rotX, rotY;
  rotX.rotxTM(rX);
  rotY.rotyTM(rY * (isLeft ? -1 : 1));

  Point3 side = orientation.getcol(0) * (isLeft ? -1 : 5);

  pose.isActive = true;
  pose.hasChanged = true;
  pose.pos = position + orientation.getcol(2) * 0.3 + side * 0.2;
  pose.rot = orientation * rotY * rotX;
  pose.linearVel = pose.angularVel = Point3(1, 0, 0);

  return pose;
}

Controller VrEmulatorInputHandler::getControllerState(Hands side) const
{
  static bool reportHandChanged[Hands::Total] = {true, true};

  constexpr float sideOffset = .30f;
  constexpr float aimOffset = 0.05f;
  Controller c;
  c.isActive = true;
  c.aim.pos = c.grip.pos = DPoint3{side == Hands::Left ? -sideOffset : sideOffset, -0.2f, 0.5f};
  c.aim.pos.z += aimOffset;
  c.grip.rot.identity();
  c.aim.rot.identity();
  c.aim.angularVel = c.aim.linearVel = c.grip.angularVel = c.grip.linearVel = DPoint3{1.0f, 0, 0}; // To skip idle detection
  c.hasChanged = reportHandChanged[side];

  reportHandChanged[side] = false;

  return c;
}

Hand VrEmulatorInputHandler::getTrackedHand(Hands side) const { return {}; }

void VrEmulatorInputHandler::applyHapticFeedback(Hands, const HapticSettings &) const { /* STUB */ }
void VrEmulatorInputHandler::stopHapticFeedback(Hands) const { /* STUB */ }
