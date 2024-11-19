// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vr/vrHands.h>

#include <drv/dag_vr.h>

#include <math/dag_mathAng.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_stdGameResId.h>
#include <render/dynmodelRenderer.h>
#include <shaders/dag_dynSceneRes.h>

#include <gamePhys/phys/animatedPhys.h>
#include <gamePhys/phys/physVars.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/contactData.h>

namespace vr
{

static constexpr float MAX_HAND_PHYS_OFFSET_SQ = 1.f;

VrHands::~VrHands() { clear(); }

void VrHands::clear()
{
  for (VrInput::Hands side : {VrInput::Hands::Left, VrInput::Hands::Right})
  {
    if (animChar[side])
      del_it(animChar[side]);
    del_it(animPhys[side]);
    del_it(physVars[side]);
  }
}

void VrHands::initAnimPhys(VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component)
{
  del_it(animPhys[side]);
  del_it(physVars[side]);

  physVars[side] = new PhysVars;

  animPhys[side] = new AnimatedPhys();
  animPhys[side]->init(animchar_component, *physVars[side]);

  indexFingerAnimVarId[side] = addAnimPhysVar(animchar_component, side, "index_finger_var", 0.f);
  fingersAnimVarId[side] = addAnimPhysVar(animchar_component, side, "fingers_var", 0.f);
  thumbAnimVarId[side] = addAnimPhysVar(animchar_component, side, "thumb_var", 0.f);
}

void VrHands::updateAnimPhys(VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component)
{
  if (!physVars[side])
    return;

  const OneHandState &state = handsState[side];

  const float POINTING_POSE_GRIP_THRESHOLD = 0.85f;
  const bool shouldPoint =
    !float_nonzero(state.indexFinger) && float_nonzero(state.thumb) && state.squeeze > POINTING_POSE_GRIP_THRESHOLD;

  const float indexVal = state.panel.isHoldingPanel ? 1.f : (shouldPoint ? 1.f : state.indexFinger * 0.5f);
  const float thumbVal = state.panel.isHoldingPanel ? 0.41f : (shouldPoint ? 1.f : state.thumb * 0.5f);
  const float squeezeVal = state.panel.isHoldingPanel ? 0.f : state.squeeze;

  setAnimPhysVarVal(side, indexFingerAnimVarId[side], indexVal);
  setAnimPhysVarVal(side, thumbAnimVarId[side], thumbVal);
  setAnimPhysVarVal(side, fingersAnimVarId[side], squeezeVal);

  animPhys[side]->update(animchar_component, *physVars[side]);
}

int VrHands::addAnimPhysVar(AnimV20::AnimcharBaseComponent &animchar_component, VrInput::Hands side, const char *name, float def_val)
{
  if (!physVars[side])
    return -1;

  const int varId = physVars[side]->registerVar(name, def_val);
  if (animPhys[side])
    animPhys[side]->appendVar(name, animchar_component, *physVars[side]);
  return varId;
}

void VrHands::setAnimPhysVarVal(VrInput::Hands side, int var_id, float val)
{
  if (!physVars[side])
    return;

  physVars[side]->setVar(var_id, val);
}

void VrHands::initHand(const char *model_name, VrInput::Hands side)
{
  String animcharResName(model_name);
  animcharResName += "_char";
  GameResHandle animcharHandle = GAMERES_HANDLE_FROM_STRING(animcharResName);

  AnimCharV20::IAnimCharacter2 *resAnimChar =
    (AnimCharV20::IAnimCharacter2 *)::get_one_game_resource_ex(animcharHandle, CharacterGameResClassId);

  if (resAnimChar)
  {
    animChar[side] = resAnimChar->clone();
    ::release_game_resource((GameResource *)resAnimChar);
  }
}


static void init_joints(const GeomNodeTree &node_tree, const VrHands::JointNameMap &joint_node_map,
  VrHands::GeomNodeTreeJointIndices &out_indices)
{
  out_indices.fill(GeomNodeTree::Index16(-1));
  for (const auto &joint : joint_node_map)
  {
    VrHands::Joints jointId = eastl::get<0>(joint);
    const char *nodeName = eastl::get<1>(joint);
    out_indices[(int)jointId] = node_tree.findNodeIndex(nodeName);
  }
}


void VrHands::init(const HandsInitInfo &hands_init_info)
{
  clear();

  grabRadius = hands_init_info.grabRadius;
  for (auto hand : {VrInput::Hands::Left, VrInput::Hands::Right})
  {
    auto &handInitInfo = hands_init_info.hands[hand];
    if (handInitInfo.handModelName)
    {
      initHand(handInitInfo.handModelName, hand);
      if (auto ac = animChar[hand])
      {
        initAnimPhys(hand, ac->baseComp());
        init_joints(ac->getNodeTree(), handInitInfo.jointNodesMap, jointIndices[hand]);
        nodeMaps[hand].init(ac->getNodeTree(), ac->getVisualResource()->getNames().node);
      }
    }
  }
}

void VrHands::init(AnimV20::AnimcharBaseComponent &left_char, AnimV20::AnimcharBaseComponent &right_char)
{
  clear();
  initAnimPhys(VrInput::Hands::Left, left_char);
  initAnimPhys(VrInput::Hands::Right, right_char);
}


void VrHands::convertDistalToFingertipTm(TMatrix &in_out_tm)
{
  in_out_tm.setcol(3, in_out_tm.getcol(3) + in_out_tm.getcol(2) * fingertipLength);
}


static TMatrix make_orthonomalized_tm(const Point3 &a, const Point3 &b, const Point3 &c)
{
  TMatrix tm{};
  tm.setcol(0, normalize(c - b));
  tm.setcol(1, normalize(cross(b - a, tm.getcol(0))));
  tm.setcol(2, normalize(cross(tm.getcol(0), tm.getcol(1))));
  return tm;
}


static TMatrix get_aligned_hand_tm(const Point3 &fist_axis_dir, const Point3 &attachment_dir, const float attachment_radius,
  const Point3 &attachment_pos, const Point3 &palm_normal, const Point3 &grab_pos, const Point3 &fixed_point_pos)
{
  const Point3 &attachmentDir = fist_axis_dir * attachment_dir >= 0.f ? attachment_dir : -attachment_dir;
  const float cosBetweenAxes = fist_axis_dir * attachmentDir;

  constexpr float MAX_ANGLE = DEG_TO_RAD * 15.f;
  const float MAX_ANGLE_COS = cos(MAX_ANGLE);
  TMatrix tiletdTm = (cosBetweenAxes < MAX_ANGLE_COS)
                       ? quat_to_matrix(Quat(fist_axis_dir % attachmentDir, safe_acos(cosBetweenAxes) - MAX_ANGLE))
                       : TMatrix::IDENT;

  const Point3 attachPos = attachment_pos - (tiletdTm * palm_normal) * attachment_radius;
  const Point3 offset = attachPos - tiletdTm * grab_pos;
  tiletdTm.setcol(3, offset);

  // Keep fixed_point_pos projection on attachmenetDir the same for any angle between the hand and the attachment.
  // This way the fixed point doesn't slide along the attachment axis. E.g. index finger stays at the same 'height' on the cylinder
  TMatrix alignedTm = quat_to_matrix(quat_rotation_arc(fist_axis_dir, attachmentDir));
  const Point3 alignedAttachPos = attachment_pos - (alignedTm * palm_normal) * attachment_radius;
  alignedTm.setcol(3, alignedAttachPos - alignedTm * grab_pos);
  const Point3 fixedPointOffset = (alignedTm * fixed_point_pos - tiletdTm * fixed_point_pos) * attachmentDir * attachmentDir;

  tiletdTm.setcol(3, offset + fixedPointOffset);

  return tiletdTm;
}


static TMatrix get_attachment_tm(const GeomNodeTree &tree, const VrHands::GeomNodeTreeJointIndices &joint_indices, VrInput::Hands side,
  const VrHands::OneHandState::Attachment &attachment, Point3 &out_attachment_pos, Point3 &out_grab_axis_dir,
  float &out_attachment_radius)
{
  using AttachmentShape = VrHands::OneHandState::Attachment::Shape;
  using Joints = VrHands::Joints;

  Point3 grabAxisDir = attachment.b - attachment.a;
  const float grabAxisLength = grabAxisDir.length();
  grabAxisDir *= safeinv(grabAxisLength);

  const Point3 &indexProximalWpos = tree.getNodeWposRelScalar(joint_indices[Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL]);
  const Point3 &littleProximalWpos = tree.getNodeWposRelScalar(joint_indices[Joints::VRIN_HAND_JOINT_LITTLE_PROXIMAL]);
  const Point3 &thumbMetacarpalWpos = tree.getNodeWposRelScalar(joint_indices[Joints::VRIN_HAND_JOINT_THUMB_METACARPAL]);
  const Point3 &palmWpos = tree.getNodeWposRelScalar(joint_indices[Joints::VRIN_HAND_JOINT_PALM]);

  const Point3 palmNormal = normalize(cross(indexProximalWpos - thumbMetacarpalWpos, littleProximalWpos - thumbMetacarpalWpos)) *
                            (side == VrInput::Hands::Left ? 1.f : -1.f);

  constexpr float KNUCKLES_PALM_K = 0.4f; // 0 - closest to knuckles, 1 - closest to the middle of the palm
  constexpr float HALF_THICKNESS = 0.02f; // offset perpendicular to the palm, move from palm node to palm surface

  const Point3 midKnuckles = (indexProximalWpos + littleProximalWpos) * 0.5f;
  const Point3 grabPos = (midKnuckles * (1.f - KNUCKLES_PALM_K)) + (palmWpos * KNUCKLES_PALM_K) + (palmNormal * HALF_THICKNESS);
  const Point3 fistAxis = normalize(indexProximalWpos - littleProximalWpos);

  const Point3 palmPos = attachment.pos;
  const Point3 palmPosProjectionOnAxis = (attachment.a + ((palmPos - attachment.a) * grabAxisDir) * grabAxisDir);
  Point3 attachPos = attachment.pos;
  Point3 attachmentAxisAnchorPoint = attachment.a;
  out_attachment_radius = attachment.r;
  TMatrix toNewRootTm = TMatrix::IDENT;

  switch (attachment.shape)
  {
    case AttachmentShape::WHEEL:
    {
      const Point3 attachNormal = normalize(palmPosProjectionOnAxis - palmPos);
      grabAxisDir = cross(attachNormal, grabAxisDir);
      out_attachment_radius = (attachment.b - attachment.a).length() * 0.5;
      attachmentAxisAnchorPoint = (attachment.a + attachment.b) * 0.5f - (attachNormal * (attachment.r - out_attachment_radius));
      toNewRootTm = get_aligned_hand_tm(fistAxis, grabAxisDir, out_attachment_radius, attachmentAxisAnchorPoint, palmNormal, grabPos,
        indexProximalWpos);

      break;
    }
    case AttachmentShape::SPHERE:
    {
      const float cosAxisNorm = grabAxisDir * palmNormal;
      if (cosAxisNorm > 0.f)
      {
        const Point3 normRotAxis = normalize(cross(grabAxisDir, palmNormal));
        const Quat quat(normRotAxis, HALFPI - safe_acos(cosAxisNorm));
        toNewRootTm = quat_to_matrix(quat);
      }
      const Point3 rotatedFistAxis = toNewRootTm * fistAxis;
      const Point3 sideOffset = (palmPos - attachment.a) * rotatedFistAxis * rotatedFistAxis;
      attachPos = attachment.a - (toNewRootTm * palmNormal) * attachment.r + sideOffset;

      const Point3 offset = attachPos - toNewRootTm * grabPos;
      toNewRootTm.setcol(3, offset);
      out_grab_axis_dir = fistAxis;
      break;
    }
    case AttachmentShape::CYLINDER:
    {
      attachmentAxisAnchorPoint = palmPosProjectionOnAxis;
      toNewRootTm =
        get_aligned_hand_tm(fistAxis, grabAxisDir, attachment.r, attachmentAxisAnchorPoint, palmNormal, grabPos, indexProximalWpos);
      break;
    }
  }

  const TMatrix toNewRootItm = inverse(toNewRootTm);
  out_attachment_pos = toNewRootItm * attachmentAxisAnchorPoint;
  if (attachment.shape != AttachmentShape::SPHERE)
    out_grab_axis_dir = toNewRootItm % grabAxisDir;

  return toNewRootTm;
}


static Point3 calc_bone_dir(const Point3 &from, const Point3 &dir, float radius, const Point3 &axis_pos, const Point3 &axis_dir,
  const int side, float bone_length, bool should_force_angle)
{
  const Point3 projOnAxis = axis_pos + ((from - axis_pos) * axis_dir) * axis_dir;
  const Point3 vecToAxis = projOnAxis - from;
  // Wrap around the circle, use the length of the bone as the length of the circular arc
  constexpr float MAX_CENTRAL_ANGLE = HALFPI;
  const float angle = (PI - clamp(bone_length / radius, 0.f, MAX_CENTRAL_ANGLE)) * 0.5f;
  const float currentAngle = safe_acos(dir * normalize(vecToAxis));
  if (should_force_angle || currentAngle < angle)
  {
    const Quat quat(axis_dir, side * angle);
    return normalize(quat * vecToAxis);
  }
  return dir;
}


static Point3 push_dir_out_of_cylinder(const Point3 &from, const Point3 dir, float radius, const Point3 &axis_pos,
  const Point3 &axis_dir, const int side)
{
  const Point3 projOnAxis = axis_pos + ((from - axis_pos) * axis_dir) * axis_dir;
  const float angle = HALFPI - safe_acos(radius / (projOnAxis - from).length());
  const Point3 vecToAxis = projOnAxis - from;
  const Point3 flatDir = normalize(dir - dir * axis_dir * axis_dir);
  const float currentAngleSide = sign(cross(flatDir, vecToAxis) * axis_dir) * -side;
  const float currentAngle = safe_acos(flatDir * normalize(vecToAxis)) * currentAngleSide;
  if (currentAngle < angle)
  {
    const Quat rotToTangent(axis_dir, side * (angle - currentAngle));
    return normalize(rotToTangent * dir);
  }
  return dir;
}


struct Ellipse
{
  Point3 center;
  Point3 axis;
  Point3 normal;
  float a;
  float b;

  Point3 getTangentDir(const Point3 &pos) const
  {
    const float focalAxisDir =
      is_equal_float(abs(axis * (pos - center)), 1.f) ? 1.f : (cross(axis, pos - center) * normal < 0 ? 1.f : -1.f);
    const float f = sqrt(abs(sqr(a) - sqr(b)));
    const Point3 f1 = center - focalAxisDir * axis * f;
    const Point3 f2 = center + focalAxisDir * axis * f;

    auto triangle_cos = [](float a, float b, float c) { return (sqr(a) - sqr(b) - sqr(c)) / (-2.f * b * c); };

    const float f1f2 = (f1 - f2).length();
    const float cf1 = (pos - f1).length();
    const float cf2 = (pos - f2).length();

    const float ellipseAxisLen = 2 * a;
    const float angleF1CF2 = safe_acos(triangle_cos(f1f2, cf1, cf2));
    const float angleF1CF = safe_acos(triangle_cos(ellipseAxisLen, cf1, cf2));

    const float tangentAngle = (angleF1CF - angleF1CF2) * 0.5;
    const Quat rotToTangent(normal, -tangentAngle);
    return normalize(rotToTangent * (f2 - pos));
  }
};


static Ellipse get_cylindrical_section(const Point3 &childPos, const Point3 &normal, const float attachment_radius,
  const Point3 &attachment_pos, const Point3 &attachment_dir)
{
  const float na = attachment_dir * normal;
  const float b = attachment_radius;
  const float a = b / abs(na);

  const float hn = (childPos - attachment_pos) * normal;
  const float ha = hn / na;
  const Point3 ellipseCenter = attachment_pos + attachment_dir * ha;

  const Point3 mainEllispeAxis =
    is_equal_float(abs(na), 1.f) ? ((childPos - attachment_pos) - normal * hn) : normalize(attachment_dir - na * normal);
  return Ellipse{ellipseCenter, mainEllispeAxis, normal, a, b};
}


static void place_fingers_on_flat_plane(GeomNodeTree &tree, const VrHands::GeomNodeTreeJointIndices &joint_indices,
  const Point3 &plane_pos, const Point3 &plane_normal, const float fingertip_length)
{
  using Joints = VrHands::Joints;

  constexpr int JOINTS_PER_FINGER = 3;
  Joints fingers[4][JOINTS_PER_FINGER] = {
    {Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL, Joints::VRIN_HAND_JOINT_INDEX_INTERMEDIATE, Joints::VRIN_HAND_JOINT_INDEX_DISTAL},
    {Joints::VRIN_HAND_JOINT_MIDDLE_PROXIMAL, Joints::VRIN_HAND_JOINT_MIDDLE_INTERMEDIATE, Joints::VRIN_HAND_JOINT_MIDDLE_DISTAL},
    {Joints::VRIN_HAND_JOINT_RING_PROXIMAL, Joints::VRIN_HAND_JOINT_RING_INTERMEDIATE, Joints::VRIN_HAND_JOINT_RING_DISTAL},
    {Joints::VRIN_HAND_JOINT_LITTLE_PROXIMAL, Joints::VRIN_HAND_JOINT_LITTLE_INTERMEDIATE, Joints::VRIN_HAND_JOINT_LITTLE_DISTAL}};

  for (const auto &joints : fingers)
  {
    float fingerLength = 0.f;
    float boneLength[JOINTS_PER_FINGER];
    for (int jointIndex = 0; jointIndex < JOINTS_PER_FINGER; ++jointIndex)
    {
      if ((jointIndex + 1) < JOINTS_PER_FINGER)
      {
        // Be aware of parent scale
        TMatrix parentWtm, childWtm;
        tree.getNodeWtmScalar(joint_indices[joints[jointIndex]], parentWtm);
        tree.getNodeWtmScalar(joint_indices[joints[jointIndex + 1]], childWtm);
        boneLength[jointIndex] = length(parentWtm.getcol(3) - childWtm.getcol(3));
      }
      else
        boneLength[jointIndex] = fingertip_length;
      fingerLength += boneLength[jointIndex];
    }

    TMatrix parentWtm;
    GeomNodeTree::Index16 parentIndex = tree.getParentNodeIdx(joint_indices[joints[0]]);
    tree.getNodeWtmRelScalar(parentIndex, parentWtm);

    for (int jointIndex = 0; jointIndex < JOINTS_PER_FINGER; ++jointIndex)
    {
      const GeomNodeTree::Index16 childIndex = joint_indices[joints[jointIndex]];

      G_ASSERT_RETURN(parentIndex == tree.getParentNodeIdx(childIndex), );

      mat44f &childTm = tree.getNodeTm(childIndex);
      TMatrix childWtm;
      v_mat_43cu_from_mat44(childWtm.array, childTm);
      childWtm = parentWtm * childWtm;
      const Point3 &childPos = childWtm.getcol(3);
      const Point3 &oldBoneDir = -childWtm.getcol(2);
      const Point3 rotationAxis = -normalize(childWtm.getcol(0));

      constexpr float FINGER_RADIUS = 0.01f;
      const Point3 attachmentPosWithOffset = plane_pos + sign((childPos - plane_pos) * plane_normal) * plane_normal * FINGER_RADIUS;
      const Point3 intersectionAxis = normalize(cross(plane_normal, rotationAxis));

      const Point3 childToIntersectionDir = normalize(cross(intersectionAxis, rotationAxis));

      const float t = safediv(((attachmentPosWithOffset - childPos) * plane_normal), (childToIntersectionDir * plane_normal));
      const Point3 intersectionPoint = childPos + childToIntersectionDir * t;
      const float childToIntersectionDist = length(childPos - intersectionPoint);

      const Point3 flatOldBoneDir = normalize(oldBoneDir - (oldBoneDir * rotationAxis) * rotationAxis);
      const float fingerDirSign = sign(flatOldBoneDir * cross(rotationAxis, childToIntersectionDir));
      const float currentAngle = safe_acos(flatOldBoneDir * childToIntersectionDir) * fingerDirSign;
      const float wishAngle = safe_acos(safediv(childToIntersectionDist, fingerLength));

      const Quat rotQuat(rotationAxis, (wishAngle - currentAngle));
      const TMatrix rotTm = quat_to_matrix(rotQuat);

      const TMatrix parentWitm = inverse(parentWtm);

      TMatrix tm = parentWitm * rotTm * childWtm;
      tm.setcol(3, as_point3(&childTm.col3));
      v_mat44_make_from_43cu_unsafe(childTm, tm.array);
      parentWtm = parentWtm * tm;
      parentIndex = childIndex;

      fingerLength -= boneLength[jointIndex];
    }
  }
}


static void place_fingers_on_attachment(GeomNodeTree &tree, VrInput::Hands hand,
  const VrHands::GeomNodeTreeJointIndices &joint_indices, const Point3 &attachment_pos, const Point3 &attachment_dir,
  const float attachment_radius, const float fingertip_length, const bool is_forced)
{
  using Joints = VrHands::Joints;

  const int codirectionMult = (as_point3(&tree.getRootTm().col1) * attachment_dir) > 0 ? 1 : -1;
  const int side = codirectionMult * (hand == VrInput::Hands::Left ? -1 : 1);

  constexpr int JOINTS_PER_FINGER = 3;
  struct
  {
    Joints joints[JOINTS_PER_FINGER];
    bool isForced;
  } fingers[4] = {
    {{Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL, Joints::VRIN_HAND_JOINT_INDEX_INTERMEDIATE, Joints::VRIN_HAND_JOINT_INDEX_DISTAL},
      is_forced},
    {{Joints::VRIN_HAND_JOINT_MIDDLE_PROXIMAL, Joints::VRIN_HAND_JOINT_MIDDLE_INTERMEDIATE, Joints::VRIN_HAND_JOINT_MIDDLE_DISTAL},
      true},
    {{Joints::VRIN_HAND_JOINT_RING_PROXIMAL, Joints::VRIN_HAND_JOINT_RING_INTERMEDIATE, Joints::VRIN_HAND_JOINT_RING_DISTAL}, true},
    {{Joints::VRIN_HAND_JOINT_LITTLE_PROXIMAL, Joints::VRIN_HAND_JOINT_LITTLE_INTERMEDIATE, Joints::VRIN_HAND_JOINT_LITTLE_DISTAL},
      true}};
  for (const auto &finger : fingers)
  {
    auto &joints = finger.joints;
    TMatrix parentWtm;
    GeomNodeTree::Index16 parentIndex = tree.getParentNodeIdx(joint_indices[joints[0]]);
    tree.getNodeWtmRelScalar(parentIndex, parentWtm);

    for (int jointIndex = 0; jointIndex < JOINTS_PER_FINGER; ++jointIndex)
    {
      const GeomNodeTree::Index16 childIndex = joint_indices[joints[jointIndex]];

      G_ASSERT_RETURN(parentIndex == tree.getParentNodeIdx(childIndex), );

      mat44f &childTm = tree.getNodeTm(childIndex);
      TMatrix childWtm;
      v_mat_43cu_from_mat44(childWtm.array, childTm);
      childWtm = parentWtm * childWtm;
      const Point3 &childPos = childWtm.getcol(3);
      const Point3 &oldBoneDir = -childWtm.getcol(2);
      constexpr float FINGER_RADIUS = 0.0025f;
      const float boneLength = (jointIndex + 1) < JOINTS_PER_FINGER
                                 ? v_extract_x(v_length3_x(tree.getNodeTm(joint_indices[joints[jointIndex + 1]]).col3))
                                 : fingertip_length;
      const Point3 &attachedBoneDir = calc_bone_dir(childPos, normalize(-childWtm.getcol(2)), attachment_radius + FINGER_RADIUS,
        attachment_pos, attachment_dir, side, boneLength, finger.isForced);

      const TMatrix rotTm = quat_to_matrix(quat_rotation_arc(oldBoneDir, attachedBoneDir));
      const TMatrix parentWitm = inverse(parentWtm);

      TMatrix tm = parentWitm * rotTm * childWtm;
      tm.setcol(3, as_point3(&childTm.col3));
      v_mat44_make_from_43cu_unsafe(childTm, tm.array);

      parentIndex = childIndex;
      parentWtm = parentWtm * tm;
    }
  }

  const GeomNodeTree::Index16 childIndex = joint_indices[Joints::VRIN_HAND_JOINT_THUMB_METACARPAL];
  TMatrix parentWtm;
  GeomNodeTree::Index16 parentIndex = tree.getParentNodeIdx(childIndex);
  tree.getNodeWtmRelScalar(parentIndex, parentWtm);
  { // Set metacarpal thumb node
    const TMatrix parentWitm = inverse(parentWtm);
    mat44f &childTm = tree.getNodeTm(childIndex);
    TMatrix childWtm;
    v_mat_43cu_from_mat44(childWtm.array, childTm);
    childWtm = parentWtm * childWtm;
    const Point3 &childPos = childWtm.getcol(3);
    const Point3 &actualBoneDir = -normalize(childWtm.getcol(2));
    constexpr float THUMB_METACARPAL_RADIUS = 0.03f;

    Point3 attachedBoneDir = push_dir_out_of_cylinder(childPos, actualBoneDir, attachment_radius + THUMB_METACARPAL_RADIUS,
      attachment_pos, attachment_dir, -side);

    TMatrix rotTm = TMatrix::IDENT;
    if (!is_forced)
      rotTm = quat_to_matrix(quat_rotation_arc(actualBoneDir, attachedBoneDir));
    else
    {
      // For more natural look align proximal nodes at the same (or close) height along the grip axis
      const float boneLength = v_extract_x(v_length3_x(tree.getNodeTm(joint_indices[Joints::VRIN_HAND_JOINT_THUMB_PROXIMAL]).col3));
      const Point3 &indexProximalPos = tree.getNodeWposRelScalar(joint_indices[Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL]);
      constexpr float ADDITIONAL_ELEVATION = 0.02f;
      const float maxElevation = boneLength * 0.5f;
      const float y =
        clamp((indexProximalPos - childPos) * attachment_dir + codirectionMult * ADDITIONAL_ELEVATION, -maxElevation, maxElevation);
      const float x = sqrt(sqr(boneLength) - sqr(y));

      attachedBoneDir = normalize(attachedBoneDir - attachedBoneDir * attachment_dir * attachment_dir) * x + attachment_dir * y;

      // Twist metacarpal and proximal nodes outward a little bit, looks better
      constexpr float ROLL_ANGLE = 7 * DEG_TO_RAD;
      rotTm = quat_to_matrix(quat_rotation_arc(actualBoneDir, attachedBoneDir) * Quat(actualBoneDir, side * ROLL_ANGLE));

      mat44f &proximalTm = tree.getNodeTm(joint_indices[Joints::VRIN_HAND_JOINT_THUMB_PROXIMAL]);
      const Quat fingerRoll(as_point3(&proximalTm.col2), side * ROLL_ANGLE);
      const TMatrix fingerRollTm = quat_to_matrix(fingerRoll);
      mat44f fingerRollVTm;
      v_mat44_make_from_43cu_unsafe(fingerRollVTm, fingerRollTm.array);
      v_mat44_mul(proximalTm, fingerRollVTm, proximalTm);
    }

    TMatrix tm = parentWitm * rotTm * childWtm;
    tm.setcol(3, as_point3(&childTm.col3));
    v_mat44_make_from_43cu_unsafe(childTm, tm.array);

    parentIndex = childIndex;
    parentWtm = parentWtm * tm;
  }

  if (is_forced)
  {
    auto calcThumbNode = [&](const float finger_radius, const GeomNodeTree::Index16 child_index, const Point3 &norm) {
      G_ASSERT_RETURN(parentIndex == tree.getParentNodeIdx(child_index), );

      mat44f &childTm = tree.getNodeTm(child_index);
      TMatrix childWtm;
      v_mat_43cu_from_mat44(childWtm.array, childTm);
      childWtm = parentWtm * childWtm;
      const Point3 &childPos = childWtm.getcol(3);
      const Point3 &oldBoneDir = normalize(-childWtm.getcol(2));

      const float radius = attachment_radius + finger_radius + cvt(attachment_radius, 0.01f, 0.03f, 0.025f, 0.01f);
      const Ellipse &ellipse = get_cylindrical_section(childPos, norm, radius, attachment_pos, attachment_dir);
      const Point3 &attachedBoneDir = ellipse.getTangentDir(childPos);

      const TMatrix rotTm = quat_to_matrix(quat_rotation_arc(oldBoneDir, attachedBoneDir));
      const TMatrix parentWitm = inverse(parentWtm);

      TMatrix tm = parentWitm * rotTm * childWtm;
      tm.setcol(3, as_point3(&childTm.col3));
      v_mat44_make_from_43cu_unsafe(childTm, tm.array);

      parentIndex = child_index;
      parentWtm = parentWtm * tm;
    };

    constexpr float THUMB_PROXIMAL_RADIUS = 0.001f;
    calcThumbNode(THUMB_PROXIMAL_RADIUS, joint_indices[Joints::VRIN_HAND_JOINT_THUMB_PROXIMAL], side * attachment_dir);
    constexpr float THUMB_DISTAL_RADIUS = 0.0005;
    const Point3 fingerBendPlaneNormal =
      normalize(parentWtm % as_point3(&tree.getNodeTm(joint_indices[Joints::VRIN_HAND_JOINT_THUMB_DISTAL]).col0));
    calcThumbNode(THUMB_DISTAL_RADIUS, joint_indices[Joints::VRIN_HAND_JOINT_THUMB_DISTAL], fingerBendPlaneNormal);
  }
}


void VrHands::updatePanelAnim(VrInput::Hands side)
{
  auto &tree = animChar[side]->getNodeTree();
  auto &state = handsState[side];
  if (state.panel.isHoldingPanel && !state.attachment.isAttached)
  {
    auto &panelAngles = state.panel.angles;
    auto &panelPosition = state.panel.position;

    Quat rotation;
    euler_to_quat(panelAngles.y, panelAngles.x, panelAngles.z, rotation);
    TMatrix matRotation = quat_to_matrix(rotation);
    TMatrix matTranslation = TMatrix::IDENT;
    matTranslation.setcol(3, panelPosition);
    TMatrix handTm;
    tree.getNodeWtmRelScalar(GeomNodeTree::Index16(0), handTm);
    handTm.orthonormalize();
    const TMatrix panelTm = handTm * matTranslation * matRotation;

    place_fingers_on_flat_plane(tree, jointIndices[side], panelTm.getcol(3), panelTm.getcol(2), fingertipLength);
    tree.invalidateWtm();
    tree.calcWtm();
  }
}


void VrHands::updateAttachment(VrInput::Hands side, const TMatrix &ref_rot_tm, const bool is_fold_forced)
{
  OneHandState::Attachment attachment = handsState[side].attachment;
  if (!attachment.isAttached)
    return;

  attachment.pos = ref_rot_tm * attachment.pos;
  attachment.a = ref_rot_tm * attachment.a;
  attachment.b = ref_rot_tm * attachment.b;

  auto &tree = animChar[side]->getNodeTree();

  Point3 attachmentPos, attachmentDir;
  float attachmentRadius;
  const TMatrix &toNewRootTm =
    get_attachment_tm(tree, jointIndices[side], side, attachment, attachmentPos, attachmentDir, attachmentRadius);

  place_fingers_on_attachment(tree, side, jointIndices[side], attachmentPos, attachmentDir, attachmentRadius, fingertipLength,
    is_fold_forced);

  mat44f &rootTm = tree.getRootTm();
  mat44f rotTm;
  v_mat44_make_from_43cu_unsafe(rotTm, toNewRootTm.array);
  v_mat44_mul43(rootTm, rotTm, rootTm);
  tree.invalidateWtm();
  tree.calcWtm();
}


void VrHands::update(float dt, const TMatrix &local_ref_space_tm, HandsState &&hands)
{
  handsState = eastl::move(hands);

  TMatrix refRotTm = local_ref_space_tm;
  refRotTm.setcol(3, ZERO<Point3>());
  const TMatrix refRotItm = orthonormalized_inverse(refRotTm);
  const vec3f wofs = v_ldu(&local_ref_space_tm.getcol(3).x);

  auto getTmWithoutRefSpace = [&refRotItm](const GeomNodeTree &tree, GeomNodeTree::Index16 node_idx) {
    TMatrix wtmRel;
    tree.getNodeWtmRelScalar(node_idx, wtmRel);
    TMatrix tm = refRotItm * wtmRel;
    tm.orthonormalize();
    return tm;
  };

  auto makeGrabTmWithoutRefSpace = [&refRotItm](const GeomNodeTree &tree, const GeomNodeTreeJointIndices &joints,
                                     VrInput::Hands hand) {
    Point3 a = refRotItm * tree.getNodeWposRelScalar(joints[Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL]);
    Point3 b = refRotItm * tree.getNodeWposRelScalar(joints[Joints::VRIN_HAND_JOINT_LITTLE_PROXIMAL]);
    Point3 c = refRotItm * tree.getNodeWposRelScalar(joints[Joints::VRIN_HAND_JOINT_PALM]);
    Point3 y = normalize(cross(a - b, c - b)) * (hand == VrInput::Hands::Left ? -1.f : 1.f);
    TMatrix grabTm;
    grabTm.setcol(0, normalize(a - b));
    grabTm.setcol(1, -y);
    grabTm.setcol(2, normalize(cross(grabTm.getcol(0), grabTm.getcol(1))));
    grabTm.setcol(3, ((a + b) * 0.5 + y * 0.05 + c) * 0.5);
    return grabTm;
  };

  for (auto side : {VrInput::Hands::Left, VrInput::Hands::Right})
  {
    auto &state = handsState[side];
    if (!state.isActive || !animChar[side])
    {
      palmTms[side] = TMatrix::IDENT;
      grabTms[side] = TMatrix::IDENT;
      visualIndexFingertipTms[side] = TMatrix::IDENT;
      realIndexFingertipTms[side] = TMatrix::IDENT;
      continue;
    }

    auto &ac = animChar[side]->baseComp();
    auto &tree = animChar[side]->getNodeTree();

    const bool areJointsTracked = !state.joints.empty();
    if (areJointsTracked)
    {
      // Align model and real hand positions and orientation
      TMatrix handTm = refRotTm * state.joints[Joints::VRIN_HAND_JOINT_PALM];
      float handSwitchXyMult = side == VrInput::Hands::Right ? 1.f : -1.f;
      Point3 x = handTm.getcol(0);
      handTm.setcol(0, handSwitchXyMult * handTm.getcol(1));
      handTm.setcol(1, -handSwitchXyMult * x);
      mat44f tm4;
      v_mat44_make_from_43cu_unsafe(tm4, handTm.array);
      tree.setRootTm(tm4, wofs);
      tree.invalidateWtm();
      ac.act(dt, true); // Resets nodes

      // Resize model to fit real hand
      const Point3 &thumbMetacarpalJointPos = state.joints[Joints::VRIN_HAND_JOINT_THUMB_METACARPAL].getcol(3);
      const Point3 &indexProximalJointPos = state.joints[Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL].getcol(3);
      const Point3 &littleProximalJointPos = state.joints[Joints::VRIN_HAND_JOINT_LITTLE_PROXIMAL].getcol(3);
      const float realPalmWidth = length(indexProximalJointPos - littleProximalJointPos);
      const float realPalmLength = length(indexProximalJointPos - thumbMetacarpalJointPos);

      const GeomNodeTree::Index16 indexProximalNodeId = jointIndices[side][Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL];
      const GeomNodeTree::Index16 littleProximalNodeId = jointIndices[side][Joints::VRIN_HAND_JOINT_LITTLE_PROXIMAL];
      const GeomNodeTree::Index16 thumbMetacarpalNodeId = jointIndices[side][Joints::VRIN_HAND_JOINT_THUMB_METACARPAL];
      const Point3 &modelIndexProximalWpos = tree.getNodeWposRelScalar(indexProximalNodeId);
      const Point3 &modelLittleProximalWpos = tree.getNodeWposRelScalar(littleProximalNodeId);
      const Point3 &modelThumbMetacarpalWpos = tree.getNodeWposRelScalar(thumbMetacarpalNodeId);
      const float modelPalmWidth = length(modelIndexProximalWpos - modelLittleProximalWpos);
      const float modelPalmLength = length(modelIndexProximalWpos - modelThumbMetacarpalWpos);

      const float scaleX = 1.f;
      const float scaleY = realPalmWidth / modelPalmWidth;
      const float scaleZ = realPalmLength / modelPalmLength;

      handTm.setcol(0, handTm.getcol(0) * scaleX);
      handTm.setcol(1, handTm.getcol(1) * scaleY);
      handTm.setcol(2, handTm.getcol(2) * scaleZ);

      // Adjust model to align three joints: proximal index, proximal little, metacarpal thumb
      const TMatrix &modelTm = make_orthonomalized_tm(modelThumbMetacarpalWpos, modelIndexProximalWpos, modelLittleProximalWpos);
      const TMatrix &realTm =
        refRotTm * make_orthonomalized_tm(thumbMetacarpalJointPos, indexProximalJointPos, littleProximalJointPos);
      const TMatrix rotTm = realTm * inverse(modelTm);

      handTm = rotTm * handTm;
      v_mat44_make_from_43cu_unsafe(tm4, handTm.array);
      tree.setRootTm(tm4, wofs);
      tree.invalidateWtm();
      tree.calcWtm();
      const Point3 jointsOffset = refRotTm * indexProximalJointPos - tree.getNodeWposRelScalar(indexProximalNodeId);
      tree.translate(v_ld(&jointsOffset.x));

      updateJoints(refRotTm, side);

      if (state.attachment.isAttached) // Move hand to attachment point when attached
      {
        const Point3 gripPos = refRotTm * state.attachment.pos;
        const Point3 offset = gripPos - tree.getRootWposRelScalar();
        tree.translate(v_ld(&offset.x));
      }
    }
    else
    {
      if (state.attachment.isAttached) // override finger state for animation
      {
        state.grip.setcol(3, state.attachment.pos);
        state.squeeze = 0.0f;
        state.indexFinger = 0.5f;
        state.thumb = 0.5f;
      }

      TMatrix handTm;
      updateHand(refRotTm, side, ac, handTm);
      mat44f tm4; // construct matrix manually to avoid losing precision/perfomance to orthonormalization
      v_mat44_make_from_43cu_unsafe(tm4, handTm.array);
      tree.setRootTm(tm4, wofs);
      tree.invalidateWtm();
      tree.calcWtm();

      ac.act(dt, true);
    }
    updateFingerStates(side);
    updatePanelAnim(side);
    updateAttachment(side, refRotTm, !areJointsTracked);

    TMatrix &fingertipRealTm = realIndexFingertipTms[side];
    fingertipRealTm = getTmWithoutRefSpace(ac.getNodeTree(), jointIndices[side][Joints::VRIN_HAND_JOINT_INDEX_DISTAL]);
    convertDistalToFingertipTm(fingertipRealTm);

    if (state.indexFingerAttachmentPoint.has_value())
    {
      const Point3 &indexFingerAttachPos = state.indexFingerAttachmentPoint.value();
      const Point3 offset = refRotTm * (indexFingerAttachPos - fingertipRealTm.getcol(3));
      tree.translate(v_ld(&offset.x));
    }

    animChar[side]->copyNodes();

    palmTms[side] = getTmWithoutRefSpace(ac.getNodeTree(), GeomNodeTree::Index16(0)); // Root
    grabTms[side] = makeGrabTmWithoutRefSpace(ac.getNodeTree(), jointIndices[side], side);

    TMatrix &fingertip = visualIndexFingertipTms[side];
    fingertip = getTmWithoutRefSpace(ac.getNodeTree(), jointIndices[side][Joints::VRIN_HAND_JOINT_INDEX_DISTAL]);
    convertDistalToFingertipTm(fingertip);
  }
}


static Point3 get_collision_pushed_offset(dag::ConstSpan<CollisionObject> pushed_collision_objects, const TMatrix &pushed_object_tm,
  dag::ConstSpan<CollisionObject> collision_objects, const TMatrix &collision_tm, uint64_t active_collision_bit_mask)
{
  Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
  dacoll::test_pair_collision(pushed_collision_objects, ~0ull, pushed_object_tm, collision_objects, active_collision_bit_mask,
    collision_tm, contacts, dacoll::TestPairFlags::Default & ~dacoll::TestPairFlags::CheckInWorld);

  Point3 posOffset = ZERO<Point3>();
  if (!contacts.empty())
  {
    for (int i = 0; i < contacts.size(); ++i)
    {
      const gamephys::CollisionContactData &contact = contacts[i];
      if (contact.depth >= 0.f)
        continue;
      Point3 offs = contact.wnormB * max(-contact.depth - posOffset * contact.wnormB, 0.f);
      posOffset += offs;
    }
  }

  return posOffset;
}


void VrHands::updateCollision(const CollisionResource &collres, const GeomNodeTree *coll_node_tree,
  dag::ConstSpan<CollisionObject> collision_objects, uint64_t active_collision_bit_mask, const TMatrix &tm, const TMatrix &ref_space)
{
  const CollisionObject &collisionObject = dacoll::get_reusable_sphere_collision();
  if (!collisionObject.isValid())
    return;

  dacoll::set_collision_sphere_rad(collisionObject, collisionRadius);
  const auto hands = {VrInput::Hands::Left, VrInput::Hands::Right};
  Point3 handOffsets[2] = {ZERO<Point3>(), ZERO<Point3>()};

  // Palm collision
  for (auto hand : hands)
  {
    if (!handsState[hand].isActive || !animChar[hand])
      continue;

    Point3 &physPos = handPhysPos[hand];

    TMatrix handTm = TMatrix::IDENT;
    handTm.setcol(3, physPos);

    const Point3 &wishPos = ref_space * handsState[hand].grip.getcol(3);
    const Point3 handMovement = wishPos - handTm.getcol(3);
    const float handMoveDistSq = handMovement.lengthSq();

    if (handMoveDistSq > sqr(collisionRadius) && handMoveDistSq < MAX_HAND_PHYS_OFFSET_SQ)
    {
      const float handMoveDist = sqrt(handMoveDistSq);
      float t = handMoveDist;
      const Point3 &traceStart = handTm.getcol(3);
      const Point3 traceDir = handMovement * safeinv(handMoveDist);
      Point3 normal;
      if (collres.traceRay(tm, coll_node_tree, traceStart, traceDir, t, &normal))
        handTm.setcol(3, traceStart + traceDir * t);
      else
        handTm.setcol(3, wishPos);
    }
    else
      handTm.setcol(3, wishPos);

    Tab<gamephys::CollisionContactData> contacts(framemem_ptr());
    dacoll::test_pair_collision(dag::ConstSpan<CollisionObject>(&collisionObject, 1), ~0ull, handTm, collision_objects,
      active_collision_bit_mask, tm, contacts, dacoll::TestPairFlags::Default & ~dacoll::TestPairFlags::CheckInWorld);

    Point3 posOffset = get_collision_pushed_offset(dag::ConstSpan<CollisionObject>(&collisionObject, 1), handTm, collision_objects, tm,
      active_collision_bit_mask);

    physPos = handTm.getcol(3) + posOffset;
    handOffsets[hand] = physPos - wishPos;
  }

  // Fingers collision
  dacoll::set_collision_sphere_rad(collisionObject, fingerCollisionRadius);
  for (auto hand : hands)
  {
    if (!handsState[hand].isActive || !animChar[hand])
      continue;

    const GeomNodeTree &tree = animChar[hand]->getNodeTree();
    const auto &joints = jointIndices[hand];
    Point3 &offset = handOffsets[hand];

    // TODO: Replace with TIP joints when nodes added to the models
    for (auto distalJoint : {Joints::VRIN_HAND_JOINT_THUMB_DISTAL, Joints::VRIN_HAND_JOINT_INDEX_DISTAL,
           Joints::VRIN_HAND_JOINT_MIDDLE_DISTAL, Joints::VRIN_HAND_JOINT_RING_DISTAL, Joints::VRIN_HAND_JOINT_LITTLE_DISTAL})
    {
      TMatrix fingerWtm;
      tree.getNodeWtmScalar(joints[distalJoint], fingerWtm);
      // Till we have separate nodes for fingertips we approximate tip's position
      const Point3 fingertipPos = fingerWtm.getcol(3) - fingerWtm.getcol(2) * fingertipLength;
      fingerWtm.setcol(3, fingertipPos + offset);
      offset += get_collision_pushed_offset(dag::ConstSpan<CollisionObject>(&collisionObject, 1), fingerWtm, collision_objects, tm,
        active_collision_bit_mask);
    }
  }

  for (auto hand : hands)
    if (handOffsets[hand].lengthSq() > 0.f)
      applyOffset(hand, handOffsets[hand]);
}


void VrHands::applyOffset(VrInput::Hands hand, const Point3 &offset)
{
  auto &tree = animChar[hand]->getNodeTree();
  tree.translate(v_ldu_p3(&offset.x));
}


void VrHands::setState(HandsState &&hands) { handsState = eastl::move(hands); }


void VrHands::updateFingerStates(VrInput::Hands hand)
{
  // Note: thumb is special and doesn't have intermediate joint, but it's proximal joint is more flexible
  // So we use thumb's proximal joint as intermediate and metacarpal as proximal
  static constexpr Joints distals[] = {Joints::VRIN_HAND_JOINT_THUMB_DISTAL, Joints::VRIN_HAND_JOINT_INDEX_DISTAL,
    Joints::VRIN_HAND_JOINT_MIDDLE_DISTAL, Joints::VRIN_HAND_JOINT_RING_DISTAL, Joints::VRIN_HAND_JOINT_LITTLE_DISTAL};
  static constexpr Joints intermediates[] = {Joints::VRIN_HAND_JOINT_THUMB_PROXIMAL, Joints::VRIN_HAND_JOINT_INDEX_INTERMEDIATE,
    Joints::VRIN_HAND_JOINT_MIDDLE_INTERMEDIATE, Joints::VRIN_HAND_JOINT_RING_INTERMEDIATE,
    Joints::VRIN_HAND_JOINT_LITTLE_INTERMEDIATE};
  static constexpr Joints proximals[] = {Joints::VRIN_HAND_JOINT_THUMB_METACARPAL, Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL,
    Joints::VRIN_HAND_JOINT_MIDDLE_PROXIMAL, Joints::VRIN_HAND_JOINT_RING_PROXIMAL, Joints::VRIN_HAND_JOINT_LITTLE_PROXIMAL};
  // TODO: remove hardcoded value
  constexpr float foldedThresholdSq = sqr(0.07);
  constexpr float pointingCosThreshold = 0.9; // ~25 deg

  if (handsState[hand].isActive && animChar[hand])
  {
    const auto &joints = jointIndices[hand];
    auto &fingerState = fingerStates[hand].states;

    const GeomNodeTree &tree = animChar[hand]->getNodeTree();
    const Point3 &palmPos = tree.getNodeWposRelScalar(joints[Joints::VRIN_HAND_JOINT_PALM]);

    for (int i = 0; i < countof(distals); ++i)
    {
      fingerState[i] = FingerStates::State::RELAXED;
      const Point3 &distalPos = tree.getNodeWposRelScalar(joints[distals[i]]);
      const float distanceToPalmSq = lengthSq(distalPos - palmPos);
      if (distanceToPalmSq < foldedThresholdSq)
        fingerState[i] = FingerStates::State::FOLDED;
      else
      {
        const Point3 &intermediatePos = tree.getNodeWposRelScalar(joints[intermediates[i]]);
        const Point3 &proximalPos = tree.getNodeWposRelScalar(joints[proximals[i]]);
        const Point3 distalBoneDir = normalize(distalPos - intermediatePos);
        const Point3 proximalBoneDir = normalize(intermediatePos - proximalPos);
        if (dot(distalBoneDir, proximalBoneDir) > pointingCosThreshold)
          fingerState[i] = FingerStates::State::POINTING;
      }
    }
  }
}


void VrHands::updateJoints(const TMatrix &local_ref_space_tm, VrInput::Hands side)
{
  const eastl::vector<TMatrix> &joints = handsState[side].joints;
  auto &tree = animChar[side]->getNodeTree();
  const GeomNodeTreeJointIndices &handNodeIds = jointIndices[side];
  constexpr int JOINT_PER_FINGER = 3;
  struct
  {
    Joints joint[JOINT_PER_FINGER];
  } fingers[] = {
    {Joints::VRIN_HAND_JOINT_THUMB_METACARPAL, Joints::VRIN_HAND_JOINT_THUMB_PROXIMAL, Joints::VRIN_HAND_JOINT_THUMB_DISTAL},
    {Joints::VRIN_HAND_JOINT_INDEX_PROXIMAL, Joints::VRIN_HAND_JOINT_INDEX_INTERMEDIATE, Joints::VRIN_HAND_JOINT_INDEX_DISTAL},
    {Joints::VRIN_HAND_JOINT_MIDDLE_PROXIMAL, Joints::VRIN_HAND_JOINT_MIDDLE_INTERMEDIATE, Joints::VRIN_HAND_JOINT_MIDDLE_DISTAL},
    {Joints::VRIN_HAND_JOINT_RING_PROXIMAL, Joints::VRIN_HAND_JOINT_RING_INTERMEDIATE, Joints::VRIN_HAND_JOINT_RING_DISTAL},
    {Joints::VRIN_HAND_JOINT_LITTLE_PROXIMAL, Joints::VRIN_HAND_JOINT_LITTLE_INTERMEDIATE, Joints::VRIN_HAND_JOINT_LITTLE_DISTAL}};

  for (int i = 0; i < JOINT_PER_FINGER; ++i)
  {
    for (auto ids : fingers)
    {
      const Joints parentJoint = ids.joint[i];
      const GeomNodeTree::Index16 parentJointNodeId = handNodeIds[parentJoint];
      if (parentJointNodeId.valid())
      {
        TMatrix parentJointTm = local_ref_space_tm * joints[parentJoint];
        parentJointTm.setcol(2, -parentJointTm.getcol(2));
        mat44f &parentNodeWtm = tree.getNodeWtmRel(parentJointNodeId);

        float lengthMultSq = 1.f;
        if (i + 1 < JOINT_PER_FINGER && handNodeIds[ids.joint[i + 1]].valid())
        {
          const Joints childJoint = ids.joint[i + 1];
          const GeomNodeTree::Index16 childJointNodeId = handNodeIds[childJoint];
          const mat44f &childNodeWtm = tree.getNodeWtmRel(childJointNodeId);
          const TMatrix childJointTm = local_ref_space_tm * joints[childJoint];
          const float modelBoneLengthSq = v_extract_x(v_length3_sq_x(v_sub(parentNodeWtm.col3, childNodeWtm.col3)));
          const float realBoneLengthSq = lengthSq(childJointTm.getcol(3) - parentJointTm.getcol(3));
          lengthMultSq = realBoneLengthSq / modelBoneLengthSq;
        }

        const vec4f scaleX = v_splat_x(v_length3_x(parentNodeWtm.col0));
        const vec4f scaleY = v_splat_x(v_length3_x(parentNodeWtm.col1));
        const vec4f scaleZ = v_splat_x(v_sqrt_x(v_mul_x(v_length3_sq_x(parentNodeWtm.col2), v_splats(lengthMultSq))));
        parentNodeWtm.col0 = v_mul(v_ldu(&parentJointTm.getcol(0).x), scaleX);
        parentNodeWtm.col1 = v_mul(v_ldu(&parentJointTm.getcol(1).x), scaleY);
        parentNodeWtm.col2 = v_mul(v_ldu(&parentJointTm.getcol(2).x), scaleZ);
        tree.markNodeTmInvalid(parentJointNodeId);
      }
    }
    tree.validateTm();
    tree.invalidateWtm();
    tree.calcWtm();
  }
}


void VrHands::updateHand(const TMatrix &local_ref_space_tm, VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component,
  TMatrix &tm)
{
  updateAnimPhys(side, animchar_component);

  const TMatrix grip = local_ref_space_tm * handsState[side].grip;
  tm = local_ref_space_tm * handsState[side].aim;
  tm.setcol(3, grip.getcol(3));
}


void VrHands::beforeRender(const Point3 &playspace_to_cam_vec, const Point3 &cam_pos_in_world)
{
  vec4f offset = v_ldu(&playspace_to_cam_vec.x);
  for (auto side : {VrInput::Hands::Left, VrInput::Hands::Right})
  {
    if (handsState[side].isActive && animChar[side])
    {
      animChar[side]->beforeRender();
      if (auto scene = animChar[side]->getSceneInstance())
      {
        scene->savePrevNodeWtm();
        scene->setOrigin(cam_pos_in_world);
        auto &nodeMap = nodeMaps[side];
        auto &tree = animChar[side]->getNodeTree();
        for (unsigned int i = 0; i < nodeMap.size(); ++i)
        {
          mat44f wtm = tree.getNodeWtmRel(nodeMap[i].nodeIdx);
          wtm.col3 = v_sub(wtm.col3, offset);
          scene->setNodeWtmRelToOrigin(nodeMap[i].id, wtm);
        }

        scene->setLod(0);
        scene->beforeRender(cam_pos_in_world);
      }
    }
  }
}


void VrHands::render(TexStreamingContext texCtx)
{
  for (auto side : {VrInput::Hands::Left, VrInput::Hands::Right})
    if (handsState[side].isActive && animChar[side])
      if (auto scene = animChar[side]->getSceneInstance())
      {
        G_ASSERT_RETURN(dynrend::can_render(scene), );
        dynrend::render_one_instance(scene, dynrend::RenderMode::Opaque, texCtx, nullptr, nullptr);
      }
}


void VrHands::drawDebugCollision() const
{
  for (auto hand : {VrInput::Hands::Left, VrInput::Hands::Right})
  {
    const Point3 &pos = handPhysPos[hand];
    const CollisionObject &collisionObject = dacoll::get_reusable_sphere_collision();
    dacoll::set_collision_sphere_rad(collisionObject, collisionRadius);
    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, pos);
    dacoll::draw_collision_object(collisionObject, tm);

    const GeomNodeTree &tree = animChar[hand]->getNodeTree();
    const auto &joints = jointIndices[hand];

    for (auto distalJoint : {Joints::VRIN_HAND_JOINT_THUMB_DISTAL, Joints::VRIN_HAND_JOINT_INDEX_DISTAL,
           Joints::VRIN_HAND_JOINT_MIDDLE_DISTAL, Joints::VRIN_HAND_JOINT_RING_DISTAL, Joints::VRIN_HAND_JOINT_LITTLE_DISTAL})
    {
      TMatrix distalWtm;
      tree.getNodeWtmScalar(joints[distalJoint], distalWtm);
      // Till we have separate nodes for fingertips we approximate tip's position
      const Point3 fingertipPos = distalWtm.getcol(3) - distalWtm.getcol(2) * fingertipLength;
      draw_debug_sphere_buffered(fingertipPos, fingerCollisionRadius, E3DCOLOR(255, 0, 0, 255));
    }
  }
}

} // namespace vr
