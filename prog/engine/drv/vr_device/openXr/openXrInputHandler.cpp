// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "openXrInputHandler.h"

#include <debug/dag_assert.h>
#include <ioSys/dag_dataBlock.h>

#include <EASTL/map.h>

#include "openXrErrorReporting.h"
#define OXR_INSTANCE instance
#define OXR_MODULE   "HID"

#include "openXrMath.h"

using VrInput = HumanInput::VrInput;
using ActionSetIndex = VrInput::ActionSetIndex;
using ActionIndex = VrInput::ActionIndex;
using DigitalAction = VrInput::DigitalAction;
using AnalogAction = VrInput::AnalogAction;
using StickAction = VrInput::StickAction;
using PoseAction = VrInput::PoseAction;
using Hands = VrInput::Hands;
using ActionBindings = VrInput::ActionBindings;

extern bool oxrHadRuntimeFailure;

namespace /* anonymous */
{

VrInput::InitializationCallback inputInitializatonCallback;

static XrPath as_xr_path(XrInstance instance, const char *str)
{
  XrPath path;
  XR_REPORTF(xrStringToPath(instance, str, &path), "Path is: %s", str);
  return path;
}


static eastl::string xr_path_as_string(XrInstance instance, XrPath path)
{
  char buf[512];
  uint32_t size = 0;
  XR_REPORT_RETURN(xrPathToString(instance, path, sizeof(buf), &size, buf), eastl::string{});
  return buf;
}


static eastl::string sanitize(const char *src)
{
  int len = strlen(src);
  eastl::string result;
  result.reserve(len + 1);
  for (int i = 0; i < len; ++i)
  {
    char c = src[i];
    if (isalnum(c) == 0 && c != '_' && c != '-' && c != '.' && c != '/')
      c = '_';
    result.push_back(c);
  }

  result.make_lower();
  return result;
}


static XrActionType as_xr_action(VrInput::ActionType t)
{
  using ActionType = VrInput::ActionType;
  switch (t)
  {
    case ActionType::DIGITAL: return XR_ACTION_TYPE_BOOLEAN_INPUT;
    case ActionType::ANALOG: return XR_ACTION_TYPE_FLOAT_INPUT;
    case ActionType::STICK: return XR_ACTION_TYPE_VECTOR2F_INPUT;
    case ActionType::POSE: return XR_ACTION_TYPE_POSE_INPUT;
  }
  return XR_ACTION_TYPE_MAX_ENUM;
}


struct BindingProfile
{
  bool addBinding(XrInstance instance, XrAction action, const char *binding)
  {
    XrActionSuggestedBinding b{action, as_xr_path(instance, binding)};
    bindings.emplace_back(b);
    return true;
  }

  XrPath path;
  eastl::vector<XrActionSuggestedBinding> bindings;
};
static eastl::map<eastl::string, BindingProfile> profiles;

static XrPath sub_paths[Hands::Total];


ActionBindings get_binding_sources(XrSession session, XrInstance instance, XrAction action)
{
  XrBoundSourcesForActionEnumerateInfo info{XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
  info.action = action;
  uint32_t numSources = VrInput::MAX_ACTION_BINDING_SOURCES;
  ActionBindings bindings;
  bindings.fill(VrInput::INVALID_ACTION_BINDING_ID);
  XR_REPORT_RETURN(xrEnumerateBoundSourcesForAction(session, &info, bindings.size(), &numSources, bindings.data()), {});
  eastl::sort(bindings.begin(), bindings.end());
  return bindings;
}

} // namespace


void OpenXrInputHandler::init(XrInstance i, XrSession s, bool enable_hand_tracking)
{
  instance = i;
  session = s;

  const eastl::vector<eastl::pair<eastl::string, eastl::string>> profile_paths = {
    {"khr_simple", "/interaction_profiles/khr/simple_controller"},
    {"google_daydream", "/interaction_profiles/google/daydream_controller"},
    {"htc_vive", "/interaction_profiles/htc/vive_controller"},
    {"microsoft_motion", "/interaction_profiles/microsoft/motion_controller"},
    {"xbox_controller", "/interaction_profiles/microsoft/xbox_controller"},
    {"oculus_go", "/interaction_profiles/oculus/go_controller"},
    {"oculus_touch", "/interaction_profiles/oculus/touch_controller"},
    {"valve_index", "/interaction_profiles/valve/index_controller"},
  };

  for (const auto &pp : profile_paths)
  {
    debug("[XR][HID] adding profile '%s' at '%s'", pp.first, pp.second);
    profiles[pp.first] = {as_xr_path(instance, pp.second.c_str())};
  }

  sub_paths[Hands::Left] = as_xr_path(instance, "/user/hand/left");
  sub_paths[Hands::Right] = as_xr_path(instance, "/user/hand/right");

  if (enable_hand_tracking)
  {
    debug("[XR][HID] attempting hand trackers initialization");
    XR_REPORT(xrGetInstanceProcAddr(instance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction *)(&createHandTracker)));
    XR_REPORT(xrGetInstanceProcAddr(instance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction *)(&locateHandJoints)));
    XR_REPORT(xrGetInstanceProcAddr(instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction *)(&destroyHandTracker)));
    for (auto side : {Hands::Left, Hands::Right})
    {
      XrHandTrackerCreateInfoEXT info{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
      info.hand = side == VrInput::Hands::Left ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
      info.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
      XR_REPORT(createHandTracker(session, &info, &handTrackers[side]));
    }
    G_ASSERT(handTrackers[Hands::Left] && handTrackers[Hands::Right]);
  }

  inited = true;

  if (inputInitializatonCallback)
    inputInitializatonCallback();
}


void OpenXrInputHandler::shutdown()
{
  debug("[XR][HID] shutting down input handler");
  clearInputActions();

  if (destroyHandTracker)
    for (auto tracker : handTrackers)
      if (tracker != XR_NULL_HANDLE)
        destroyHandTracker(tracker);

  session = 0;
  instance = 0;
  inited = false;
  isActionsInitComplete = false;
}


bool OpenXrInputHandler::activateActionSet(ActionSetId action_set_id)
{
  ActionSetIndex idx = findActionSetById(action_set_id);
  G_ASSERTF_RETURN(idx != INVALID_ACTION_SET, false, "[XR][HID] could not find action set 0x%8f, unable to activate it",
    action_set_id);
  activeActionSet = idx;
  debug("[XR][HID] activated action set %d: %s", activeActionSet, actionSetNames[activeActionSet]);
  return activeActionSet != INVALID_ACTION_SET;
}


void OpenXrInputHandler::updateActionsState()
{
  if (!isInited() || activeActionSet == INVALID_ACTION_SET)
    return;

  setupLegacyControllerSpaces();

  for (int i = 0; i < poseActionIndices.size(); ++i)
  {
    const auto action = actions[poseActionIndices[i]];
    XrSpace &space = poseActionSpaces[i];
    XrActionSpaceCreateInfo asi{XR_TYPE_ACTION_SPACE_CREATE_INFO};
    asi.action = action;
    asi.poseInActionSpace.orientation.w = 1.f;

    if (action != XR_NULL_HANDLE && space == XR_NULL_HANDLE)
      XR_REPORTF(xrCreateActionSpace(session, &asi, &space), "Failed to create grip space #%d", i);
  }

  XrActiveActionSet as[] = {
    {actionSets[activeActionSet], XR_NULL_PATH},
  };
  XrActionsSyncInfo asi{XR_TYPE_ACTIONS_SYNC_INFO};
  asi.countActiveActionSets = countof(as);
  asi.activeActionSets = as;
  XR_REPORT(xrSyncActions(session, &asi));
}


void OpenXrInputHandler::updatePoseSpaces(XrSpace ref_space, XrTime t)
{
  for (int i = 0; i < poseActionIndices.size(); ++i)
  {
    auto update = [ref_space, t, session = this->session, instance = this->instance](XrAction action, XrSpace space, PoseAction &out) {
      out.linearVel = out.angularVel = DPoint3{0, 0, 0}; // But keep previous pos and rot
      if (!action)
        return false;

      XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};
      gi.action = action;

      XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};
      if (XR_REPORTF(xrGetActionStatePose(session, &gi, &state), "Failed to get pose for %d", action) && state.isActive)
      {
        XrSpaceVelocity vel{XR_TYPE_SPACE_VELOCITY};
        XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION, &vel};
        if (XR_REPORT(xrLocateSpace(space, ref_space, t, &loc)))
        {
          if ((loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0)
            out.pos = as_dpoint3(loc.pose.position);

          if ((loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
            out.rot = as_quat(loc.pose.orientation);

          if ((vel.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) != 0)
            out.linearVel = as_dpoint3(vel.linearVelocity);
          if ((vel.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) != 0)
            out.angularVel = as_dpoint3(vel.angularVelocity);

          return true;
        }
      }
      return false;
    };

    const auto action = actions[poseActionIndices[i]];
    const auto space = poseActionSpaces[i];
    const PoseAction previousState = lastPoseActionStates[i];
    PoseAction &state = lastPoseActionStates[i];
    state.isActive = update(action, space, state);
    state.hasChanged = previousState != state;
  }
}


void OpenXrInputHandler::setupLegacyControllerSpaces()
{
  for (auto hand : {Hands::Left, Hands::Right})
  {
    XrActionSpaceCreateInfo asi{XR_TYPE_ACTION_SPACE_CREATE_INFO};
    asi.action = gripPoseAction;
    asi.poseInActionSpace.orientation.w = 1.f;
    asi.subactionPath = sub_paths[hand];
    if (gripPoseAction != XR_NULL_HANDLE && gripSpaces[hand] == XR_NULL_HANDLE)
      XR_REPORTF(xrCreateActionSpace(session, &asi, &gripSpaces[hand]), "Failed to create grip space #%d", hand);

    asi.action = aimPoseAction;
    if (aimPoseAction != XR_NULL_HANDLE && aimSpaces[hand] == XR_NULL_HANDLE)
      XR_REPORTF(xrCreateActionSpace(session, &asi, &aimSpaces[hand]), "Failed to create grip space #%d", hand);
  }
}


void OpenXrInputHandler::updateLegacyControllerPoses(XrSpace ref_space, XrTime t)
{
  for (auto hand : {Hands::Left, Hands::Right})
  {
    auto update = [ref_space, t, hand, session = this->session, instance = this->instance](XrAction action, XrSpace space, Pose &out) {
      out.linearVel = out.angularVel = DPoint3{0, 0, 0}; // But keep previous pos and rot
      if (!action)
        return false;

      constexpr const char *sides[] = {"left", "right"};
      XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};
      gi.subactionPath = sub_paths[hand];
      gi.action = action;

      XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};
      if (XR_REPORTF(xrGetActionStatePose(session, &gi, &state), "Failed to get %s pose", sides[hand]) && state.isActive)
      {
        XrSpaceVelocity vel{XR_TYPE_SPACE_VELOCITY};
        XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION, &vel};
        if (XR_REPORT(xrLocateSpace(space, ref_space, t, &loc)))
        {
          if ((loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0)
            out.pos = as_dpoint3(loc.pose.position);

          if ((loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
            out.rot = as_quat(loc.pose.orientation);

          if ((vel.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) != 0)
            out.linearVel = as_dpoint3(vel.linearVelocity);
          if ((vel.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) != 0)
            out.angularVel = as_dpoint3(vel.angularVelocity);

          return true;
        }
      }
      return false;
    };

    const Controller previousState = lastControllerStates[hand];
    Controller &state = lastControllerStates[hand];
    const bool gotGrip = update(gripPoseAction, gripSpaces[hand], state.grip);
    const bool gotAim = update(aimPoseAction, aimSpaces[hand], state.aim);
    state.isActive = gotGrip || gotAim;
    state.hasChanged = previousState != state;
  }
}


void OpenXrInputHandler::updateHandJoints(XrSpace ref_space, XrTime t)
{
  if (!locateHandJoints)
    return;

  for (auto hand : {Hands::Left, Hands::Right})
  {
    static_assert(VrInput::Joints::VRIN_HAND_JOINTS_TOTAL >= XR_HAND_JOINT_COUNT_EXT, "More hand joints required, adjust");
    XrHandJointLocationEXT trackedJoints[VrInput::Joints::VRIN_HAND_JOINTS_TOTAL];
    if (auto tracker = handTrackers[hand])
    {
      XrHandJointsLocateInfoEXT info{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT, nullptr, ref_space, t};
      XrHandJointLocationsEXT locations{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
      locations.isActive = false;
      locations.jointCount = countof(trackedJoints);
      locations.jointLocations = trackedJoints;

      XR_REPORT(locateHandJoints(tracker, &info, &locations));
      lastTrackedHands[hand].isActive = locations.isActive;

      auto &joints = lastTrackedHands[hand].joints;
      joints.resize(locations.jointCount);
      for (int i = 0; i < locations.jointCount; ++i)
      {
        const auto &joint = locations.jointLocations[i];
        joints[i].radius = joint.radius;

        const auto &p = joint.pose;
        if (joint.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
          joints[i].pose.pos = as_dpoint3(p.position);
        if (joint.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
          joints[i].pose.rot = as_quat(p.orientation);
      }
    }
  }
}


bool OpenXrInputHandler::clearInputActions()
{
  debug("[XR][HID] clearing all actions");
  activeActionSet = INVALID_ACTION_SET;
  for (auto &actionSet : actionSets)
    XR_REPORT(xrDestroyActionSet(actionSet));

  for (const auto space : poseActionSpaces)
    if (space != XR_NULL_HANDLE)
      xrDestroySpace(space);
  poseActionSpaces.clear();
  poseActionIndices.clear();
  lastPoseActionStates.clear();

  for (auto &p : profiles)
    p.second.bindings.clear();

  actions.clear();
  actionIds.clear();
  actionTypes.clear();
  actionNames.clear();
  actionSets.clear();
  actionSetIds.clear();
  actionSetNames.clear();
  currentBindingMasks.clear();
  poseActionIndices.clear();
  poseActionSpaces.clear();
  return true;
}


ActionSetIndex OpenXrInputHandler::findActionSetById(ActionSetId id, XrActionSet &out_action_set) const
{
  auto it = eastl::find(actionSetIds.begin(), actionSetIds.end(), id);
  G_ASSERTF_RETURN(it != actionSetIds.end(), INVALID_ACTION_SET, "[XR][HID] could not find action set %d", id);
  ActionSetIndex idx = it - actionSetIds.begin();
  out_action_set = actionSets[idx];
  return idx;
}


ActionIndex OpenXrInputHandler::findActionById(ActionId id, XrAction &out_action) const
{
  auto it = eastl::find(actionIds.begin(), actionIds.end(), id);
  G_ASSERTF_RETURN(it != actionIds.end(), INVALID_ACTION, "[XR][HID] could not find action %d", id);
  ActionIndex idx = it - actionIds.begin();
  out_action = actions[idx];
  return idx;
}


ActionSetIndex OpenXrInputHandler::addActionSet(ActionSetId action_set_id, const char *n, int priority, const char *localized_name)
{
  XrActionSetCreateInfo asi{XR_TYPE_ACTION_SET_CREATE_INFO};
  eastl::string name{n};
  name.make_lower();
  strcpy(asi.actionSetName, name.c_str());
  strcpy(asi.localizedActionSetName, localized_name ? localized_name : name.c_str());
  asi.priority = priority;

  XrActionSet as;
  XrResult r = xrCreateActionSet(instance, &asi, &as);
  debug("[XR][HID] create action set [%d: '%s'/'%s'], result %d", action_set_id, asi.actionSetName, asi.localizedActionSetName, r);
  if (XR_FAILED(r))
    return INVALID_ACTION_SET;

  actionSets.emplace_back(as);
  actionSetIds.emplace_back(action_set_id);
  actionSetNames.emplace_back(name);
  return actionSets.size() - 1;
}


ActionIndex OpenXrInputHandler::addAction(ActionSetId action_set_id, ActionId action_id, ActionType type, const char *n,
  const char *localized_name)
{
  XrActionSet as;
  if (INVALID_ACTION_SET == findActionSetById(action_set_id, as))
    return INVALID_ACTION;

  XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};
  eastl::string name = sanitize(n);
  strcpy(ai.actionName, name.c_str());
  ai.actionType = as_xr_action(type);
  strcpy(ai.localizedActionName, localized_name ? localized_name : name.c_str());

  XrAction action;
  XrResult r = xrCreateAction(as, &ai, &action);
  debug("[XR][HID] create action [%d:%d '%s'/'%s', type %d], result %d", action_set_id, action_id, ai.actionName,
    ai.localizedActionName, ai.actionType, r);
  if (XR_FAILED(r))
    return INVALID_ACTION;

  actions.emplace_back(action);
  actionNames.emplace_back(name);
  actionTypes.emplace_back(ai.actionType);
  actionIds.emplace_back(action_id);
  currentBindings.push_back({action_id, {}});
  const ActionIndex idx = actions.size() - 1;
  if (type == ActionType::POSE)
  {
    poseActionIndices.emplace_back(idx);
    poseActionSpaces.push_back({});
    lastPoseActionStates.push_back({});
  }

  return idx;
}


void OpenXrInputHandler::suggestBinding(XrAction action, const eastl::string &name, const char *controller, const char *path)
{
  auto profile = profiles.find(controller);
  G_ASSERTF_RETURN(profile != profiles.end(), , "[XRIN] %s references unknown profile %s", name.c_str(), controller);

  debug("[XR][HID] binding '%s' to '%s: %s'", name.c_str(), controller, path);
  profile->second.addBinding(instance, action, path);
}


void OpenXrInputHandler::suggestBinding(ActionId a_id, const char *controller, const char *path)
{
  XrAction action;
  ActionIndex idx = findActionById(a_id, action);
  G_ASSERTF_RETURN(idx != INVALID_ACTION, , "[XR][HID] could not find action to bind to '%s:%s'", controller, path);
  suggestBinding(action, actionNames[idx], controller, path);
}


void OpenXrInputHandler::updateCurrentBindings()
{
  debug("[XR][HID] updating bindings");
  currentBindingMasks.clear();
  currentBindingMasks.resize(actions.size());

  // Runtime can dynamically re-assign our suggestions and in general we don't know the full possible set of src paths
  // so we gather all unique currently bound sources and mask according to their order.
  int boundActions = 0;
  eastl::vector_set<XrPath> uniqueSources{};
  for (int i = 0; i < actions.size(); ++i)
  {
    int boundSources = 0;
    currentBindings[i].second = get_binding_sources(session, instance, actions[i]);
    for (ActionBindingId src : currentBindings[i].second)
      if (src != INVALID_ACTION_BINDING_ID)
      {
        uniqueSources.insert(src);
        boundSources++;
      }

    if (boundSources > 0)
      boundActions++;
  }
  debug("[XR][HID] %llu unique action paths currently bound for %d actions", uniqueSources.size(), boundActions);

  for (int i = 0; i < actions.size(); ++i)
    for (XrPath src : currentBindings[i].second)
      if (src != INVALID_ACTION_BINDING_ID)
        updateBindingMask(i, src, uniqueSources);

  for (const char *path : {"/user/hand/left", "/user/hand/right"})
  {
    XrInteractionProfileState xips{XR_TYPE_INTERACTION_PROFILE_STATE, nullptr, XR_NULL_PATH};
    XrPath xrPath = as_xr_path(instance, path);
    if (XrResult r = xrGetCurrentInteractionProfile(session, xrPath, &xips);
        r == XR_SUCCESS && xips.interactionProfile != XR_NULL_PATH)
      debug("[XR][HID] current interaction profile for '%s' is '%s'", path,
        xr_path_as_string(instance, xips.interactionProfile).c_str());
    else if (xips.interactionProfile == XR_NULL_PATH)
      debug("[XR][HID] no current interaction profile for '%s'", path);
    else
      debug("[XR][HID] failed to get interaction profile for '%s': 0x%x", path, r);
  }
}


void OpenXrInputHandler::updateBindingMask(ActionIndex action_idx, XrPath binding, const eastl::vector_set<XrPath> &known_bindings)
{
  auto found = eastl::find(known_bindings.begin(), known_bindings.end(), binding);
  G_ASSERTF_RETURN(found != known_bindings.end(), , "[XR][HID] unknown binding %llu: %s", binding,
    getLocalizedBindingName(binding)); // workaround Meta Runtime v66 failing xrPathToString for re-bound sources

  const uint16_t bit = found - known_bindings.begin();
  currentBindingMasks[action_idx].set(bit);
}


HumanInput::ButtonBits OpenXrInputHandler::getCurrentBindingsMask(ActionId a) const
{
  if (currentBindingMasks.empty())
    return {};

  XrAction action;
  ActionIndex idx = findActionById(a, action);
  G_ASSERTF_RETURN(idx != INVALID_ACTION, {}, "[XR][HID] could not find action '%d' to get bindings mask", a);
  return currentBindingMasks[idx];
}


OpenXrInputHandler::ActionBindings OpenXrInputHandler::getCurrentBindings(ActionId a) const
{
  XrAction action;
  ActionIndex idx = findActionById(a, action);
  G_ASSERTF_RETURN(idx != INVALID_ACTION, {}, "[XR][HID] could not find action '%d' to get bindings", a);
  return currentBindings[idx].second;
}


eastl::string OpenXrInputHandler::getBindingName(ActionBindingId b) const
{
  G_STATIC_ASSERT(sizeof(b) == sizeof(XrPath));
  G_ASSERTF_RETURN(b != VrInput::INVALID_ACTION_BINDING_ID, {}, "[XR][HID] Invalid binding Id");
  return xr_path_as_string(instance, static_cast<XrPath>(b));
}


eastl::string OpenXrInputHandler::getLocalizedBindingName(ActionBindingId b) const
{
  G_STATIC_ASSERT(sizeof(b) == sizeof(XrPath));
  G_ASSERTF_RETURN(b != VrInput::INVALID_ACTION_BINDING_ID, {}, "[XR][HID] Invalid binding Id");

  XrInputSourceLocalizedNameGetInfo info{XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO};
  info.sourcePath = static_cast<XrPath>(b);
  info.whichComponents = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT | XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

  eastl::string name;
  uint32_t expectedSize = 0;
  XR_REPORT_RETURN(xrGetInputSourceLocalizedName(session, &info, 0, &expectedSize, nullptr), name);
  if (expectedSize > 0)
  {
    name.resize(expectedSize);
    XR_REPORT_RETURN(xrGetInputSourceLocalizedName(session, &info, name.size(), &expectedSize, name.begin()), name);
  }

  return name;
}


bool OpenXrInputHandler::setupHands(ActionSetId action_set_id, const char *localized_grip_pose_name,
  const char *localized_aim_pose_name, const char *localized_haptic_name)
{
  struct PoseBinding
  {
    const char *controller;
    const char *path;
  };
  const PoseBinding gripPoses[Hands::Total][7] = {
    {{"khr_simple", "/user/hand/left/input/grip/pose"}, {"google_daydream", "/user/hand/left/input/grip/pose"},
      {"htc_vive", "/user/hand/left/input/grip/pose"}, {"microsoft_motion", "/user/hand/left/input/grip/pose"},
      {"oculus_go", "/user/hand/left/input/grip/pose"}, {"oculus_touch", "/user/hand/left/input/grip/pose"},
      {"valve_index", "/user/hand/left/input/grip/pose"}},

    {{"khr_simple", "/user/hand/right/input/grip/pose"}, {"google_daydream", "/user/hand/right/input/grip/pose"},
      {"htc_vive", "/user/hand/right/input/grip/pose"}, {"microsoft_motion", "/user/hand/right/input/grip/pose"},
      {"oculus_go", "/user/hand/right/input/grip/pose"}, {"oculus_touch", "/user/hand/right/input/grip/pose"},
      {"valve_index", "/user/hand/right/input/grip/pose"}}};
  const PoseBinding aimPoses[Hands::Total][7] = {
    {{"khr_simple", "/user/hand/left/input/aim/pose"}, {"google_daydream", "/user/hand/left/input/aim/pose"},
      {"htc_vive", "/user/hand/left/input/aim/pose"}, {"microsoft_motion", "/user/hand/left/input/aim/pose"},
      {"oculus_go", "/user/hand/left/input/aim/pose"}, {"oculus_touch", "/user/hand/left/input/aim/pose"},
      {"valve_index", "/user/hand/left/input/aim/pose"}},

    {{"khr_simple", "/user/hand/right/input/aim/pose"}, {"google_daydream", "/user/hand/right/input/aim/pose"},
      {"htc_vive", "/user/hand/right/input/aim/pose"}, {"microsoft_motion", "/user/hand/right/input/aim/pose"},
      {"oculus_go", "/user/hand/right/input/aim/pose"}, {"oculus_touch", "/user/hand/right/input/aim/pose"},
      {"valve_index", "/user/hand/right/input/aim/pose"}}};

  ActionSetIndex actionSetIdx = findActionSetById(action_set_id);
  XrActionSet actionSet = actionSets[actionSetIdx];

  XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};
  ai.countSubactionPaths = countof(sub_paths);
  ai.subactionPaths = sub_paths;

  ai.actionType = XR_ACTION_TYPE_POSE_INPUT;
  strcpy(ai.actionName, "grip_pose");
  strcpy(ai.localizedActionName, localized_grip_pose_name);
  XR_REPORT(xrCreateAction(actionSet, &ai, &gripPoseAction));
  for (const auto &hand : gripPoses)
    for (const auto &binding : hand)
      suggestBinding(gripPoseAction, "grip_pose", binding.controller, binding.path);

  strcpy(ai.actionName, "aim_pose");
  strcpy(ai.localizedActionName, localized_aim_pose_name);
  XR_REPORT(xrCreateAction(actionSet, &ai, &aimPoseAction));
  for (const auto &hand : aimPoses)
    for (const auto &binding : hand)
      suggestBinding(aimPoseAction, "aim_pose", binding.controller, binding.path);

  setupHapticsForBoundControllers(action_set_id, localized_haptic_name);

  return true;
}


void OpenXrInputHandler::setupHapticsForBoundControllers(ActionSetId action_set_id, const char *localized_name)
{
  ActionSetIndex actionSetIdx = findActionSetById(action_set_id);
  XrActionSet actionSet = actionSets[actionSetIdx];

  XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};
  ai.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
  ai.countSubactionPaths = countof(sub_paths);
  ai.subactionPaths = sub_paths;
  strcpy(ai.actionName, "haptic_feedback");
  strcpy(ai.localizedActionName, localized_name);
  XR_REPORT(xrCreateAction(actionSet, &ai, &hapticAction));

  struct KnownBinding
  {
    const char *controller;
    const char *path;
  };
  constexpr KnownBinding known_bindings[Hands::Total][5] = {
    {{"khr_simple", "/user/hand/left/output/haptic"}, {"htc_vive", "/user/hand/left/output/haptic"},
      {"microsoft_motion", "/user/hand/left/output/haptic"}, {"oculus_touch", "/user/hand/left/output/haptic"},
      {"valve_index", "/user/hand/left/output/haptic"}},

    {{"khr_simple", "/user/hand/right/output/haptic"}, {"htc_vive", "/user/hand/right/output/haptic"},
      {"microsoft_motion", "/user/hand/right/output/haptic"}, {"oculus_touch", "/user/hand/right/output/haptic"},
      {"valve_index", "/user/hand/right/output/haptic"}},
  };

  for (const auto &hand : known_bindings)
    for (const auto &binding : hand)
    {
      const auto candidate = profiles.find(binding.controller);
      if (candidate != profiles.end() && !candidate->second.bindings.empty())
        suggestBinding(hapticAction, "haptic_feedback", binding.controller, binding.path);
    }
}


bool OpenXrInputHandler::completeActionsInit()
{
  XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
  for (auto &profile : profiles)
  {
    uint32_t bindingsCount = profile.second.bindings.size();
    if (bindingsCount == 0)
      continue;

    suggested.interactionProfile = profile.second.path;
    suggested.suggestedBindings = profile.second.bindings.data();
    suggested.countSuggestedBindings = bindingsCount;
    XrResult suggestResult = xrSuggestInteractionProfileBindings(instance, &suggested);
    debug("[XR][HID] suggested profile '%s' with %d bindings, result - %d", profile.first.c_str(), bindingsCount, suggestResult);
  }

  XrSessionActionSetsAttachInfo asai{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
  asai.countActionSets = (uint32_t)actionSets.size();
  asai.actionSets = actionSets.data();
  XrResult attachResult = xrAttachSessionActionSets(session, &asai);
  debug("[XR][HID] attaching %d action sets: result - %d", asai.countActionSets, attachResult);

  isActionsInitComplete = XR_SUCCEEDED(attachResult) && actions.size() != 0;
  return isActionsInitComplete;
}


DigitalAction OpenXrInputHandler::getDigitalActionState(ActionId a) const
{
  XrAction action;
  ActionIndex idx = findActionById(a, action);
  G_ASSERTF_RETURN(idx != INVALID_ACTION, {}, "[XR][HID] action for id 0x%8f not found", a);
  G_ASSERTF_RETURN(actionTypes[idx] == XR_ACTION_TYPE_BOOLEAN_INPUT, {}, "[XR][HID] %s type is %d, expected %d", actionNames[idx],
    actionTypes[idx], XR_ACTION_TYPE_BOOLEAN_INPUT);

  XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
  XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, action, XR_NULL_PATH};
  if (XR_REPORTF(xrGetActionStateBoolean(session, &getInfo, &state), "failed to get state for '%s'", actionNames[idx].data()))
  {
    DigitalAction action;
    action.isActive = state.isActive;
    action.hasChanged = state.changedSinceLastSync;
    action.state = state.currentState;
    return action;
  }

  return {};
}


AnalogAction OpenXrInputHandler::getAnalogActionState(ActionId a) const
{
  XrAction action;
  ActionIndex idx = findActionById(a, action);
  G_ASSERTF_RETURN(idx != INVALID_ACTION, {}, "[XR][HID] action for id 0x%8f not found", a);
  G_ASSERTF_RETURN(actionTypes[idx] == XR_ACTION_TYPE_FLOAT_INPUT, {}, "[XR][HID] %s type is %d, expected %d", actionNames[idx],
    actionTypes[idx], XR_ACTION_TYPE_FLOAT_INPUT);

  XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
  XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, action, XR_NULL_PATH};
  if (XR_REPORTF(xrGetActionStateFloat(session, &getInfo, &state), "failed to get state for '%s'", actionNames[idx].data()))
  {
    AnalogAction action;
    action.isActive = state.isActive;
    action.hasChanged = state.changedSinceLastSync;
    action.state = state.currentState;
    return action;
  }

  return {};
}


StickAction OpenXrInputHandler::getStickActionState(ActionId a) const
{
  XrAction action;
  ActionIndex idx = findActionById(a, action);
  G_ASSERTF_RETURN(idx != INVALID_ACTION, {}, "[XR][HID] action for id 0x%8f not found", a);
  G_ASSERTF_RETURN(actionTypes[idx] == XR_ACTION_TYPE_VECTOR2F_INPUT, {}, "[XR][HID] %s type is %d, expected %d", actionNames[idx],
    actionTypes[idx], XR_ACTION_TYPE_VECTOR2F_INPUT);

  XrActionStateVector2f state{XR_TYPE_ACTION_STATE_VECTOR2F};
  XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, action, XR_NULL_PATH};
  if (XR_REPORTF(xrGetActionStateVector2f(session, &getInfo, &state), "failed to get state for '%s'", actionNames[idx].data()))
  {
    StickAction action;
    action.isActive = state.isActive;
    action.hasChanged = state.changedSinceLastSync;
    action.state = {state.currentState.x, state.currentState.y};
    return action;
  }

  return {};
}


PoseAction OpenXrInputHandler::getPoseActionState(ActionId a) const
{
  XrAction action;
  ActionIndex idx = findActionById(a, action);
  G_ASSERTF_RETURN(idx != INVALID_ACTION, {}, "[XR][HID] action for id 0x%8f not found", a);
  G_ASSERTF_RETURN(actionTypes[idx] == XR_ACTION_TYPE_POSE_INPUT, {}, "[XR][HID] %s type is %d, expected %d", actionNames[idx],
    actionTypes[idx], XR_ACTION_TYPE_POSE_INPUT);
  const auto stateIdx = eastl::find(poseActionIndices.begin(), poseActionIndices.end(), idx);
  if (stateIdx != poseActionIndices.end())
    return lastPoseActionStates[stateIdx - poseActionIndices.begin()];
  return {};
}

bool OpenXrInputHandler::isInUsableState() const { return !oxrHadRuntimeFailure; }

void OpenXrInputHandler::setInitializationCallback(InitializationCallback callback) { inputInitializatonCallback = callback; }

void OpenXrInputHandler::setAndCallInitializationCallback(InitializationCallback callback)
{
  setInitializationCallback(callback);
  if (inputInitializatonCallback)
    inputInitializatonCallback();
}


void OpenXrInputHandler::applyHapticFeedback(Hands side, const HapticSettings &settings) const
{
  G_STATIC_ASSERT(HapticSettings::MIN_DURATION == XR_MIN_HAPTIC_DURATION);
  G_STATIC_ASSERT(HapticSettings::UNSPECIFIED_FREQUENCY == XR_FREQUENCY_UNSPECIFIED);

  if (!session || hapticAction == XR_NULL_HANDLE)
    return;

  XrHapticActionInfo hai{XR_TYPE_HAPTIC_ACTION_INFO, nullptr, hapticAction, sub_paths[side]};
  XrHapticVibration props{XR_TYPE_HAPTIC_VIBRATION};
  props.duration = settings.durationNs;
  props.frequency = settings.frequency;
  props.amplitude = settings.amplitude;

  XR_REPORTF(xrApplyHapticFeedback(session, &hai, (const XrHapticBaseHeader *)&props),
    "Failed to apply haptics for hand %d, {%dns, %.g, %g}", (int)side, settings.durationNs, settings.frequency, settings.amplitude);
}


void OpenXrInputHandler::stopHapticFeedback(Hands side) const
{
  if (session && hapticAction != XR_NULL_HANDLE)
  {
    XrHapticActionInfo hai{XR_TYPE_HAPTIC_ACTION_INFO, nullptr, hapticAction, sub_paths[side]};
    XR_REPORT(xrStopHapticFeedback(session, &hai));
  }
}
