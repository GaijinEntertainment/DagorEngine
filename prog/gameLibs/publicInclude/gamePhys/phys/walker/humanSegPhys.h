//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>

enum SegPhysCheckType
{
  SEGPHYS_ON_INVALID,
  SEGPHYS_ON_ALWAYS,            // no check, must always succeed
  SEGPHYS_ON_IS_CLIMB_THRU,     // check if climbing isClimbThrough == 'true'
  SEGPHYS_ON_CLIMB_CHECK_FLOOR, // trace floor down <= transition.len
  SEGPHYS_ON_CLIMB_HEIGHT_LESS, // height to climb < transition.len
  SEGPHYS_ON_STARTED_ON_LADDER, // check if currentState.attachedToLadder was 'true' at start
};
struct SegPhysTransition
{
  bool not_on = false;
  SegPhysCheckType on = SEGPHYS_ON_INVALID;
  Point3 offs = Point3::ZERO;
  float len = 0.f;
  int to = -1;

  // displacement (optional origin correction after transition happens)
  Point3 disp = Point3::ZERO;
};

struct SegPhysTrajectoryPoint
{
  float t = 0.f; // time of trajectory, should start with 0
  float h = 1.f; // human height, -1 = prone, 0 = crouch, 1 = stand
  Point3 p;
};

enum SegPhysSegmentType
{
  SEGPHYS_INVALID,
  SEGPHYS_END,  // forceably finish segmented physics
  SEGPHYS_PASS, // execute instantly i.e. resolve transition at same tick (avoid loops!)
  SEGPHYS_WAIT, // wait for currTime >= segment.timeout before resolving transition

  // blendTrajMinHeight = min height is used to blend trajectory and trajectory2
  // blendTrajMaxHeight = max height is used to blend trajectory and trajectory2
  SEGPHYS_CLIMB_START,

  // inSkip = align by skipping part of trajectory
  // inTime = time to interpolate into trajectory
  // speedCoef = time speed along timed trajectory
  // speedCoef2 = time speed for trajectory2, blended with speedCoef by blendTrajParam (only if blendTwoTrajectories)
  // stuckDist = distance when move considered stuck
  // timeout = max time not moving due to obstacles
  // blendTwoTrajectories = enables blend of trajectory and trajectory2 with the blendTrajParam computed in SEGPHYS_CLIMB_START
  // maxCorrectionSpeed = speed on top of the trajectory speed allowed for pulling the human onto the trajectory
  SEGPHYS_CLIMB_MOVE_BY_TRAJECTORY,

  // Moves the human to the first point of the trajectory along the velocity provided by the two keys.
  // trajectory and dp are required and have to correspond to the two keys of the next segment trajectory
  // speedCoef = time speed to compute correct pull speed
  // timeout = max time to pull towards trajectory
  SEGPHYS_CLIMB_MOVE_TO_TRAJECTORY,

  SEGPHYS_CLIMB_RESUME_DEFAULT,
  SEGPHYS_CLIMB_END,
};
struct SegPhysSegment
{
  eastl::string name;
  SegPhysSegmentType type = SEGPHYS_INVALID;

  // Use animID to index into anims[] array in SegmentedHumanPhysics
  // also use it to remap animID to custom indices for AnimTree/etc.
  // if -1 no special animation assigned to this segment
  int animID = -1;

  // These parameters could be used by any segment type if needed
  // default values correspond to "no special effects" variant
  bool inSkip = false;
  float inTime = 0.f;
  float speedCoef = 1.f;
  float speedCoef2 = 1.f;
  float stuckDist = 1.f;
  float timeout = 0.f;
  float maxCorrectionSpeed = 3.f; // m/s
  bool noGravity = false;
  bool setRemoteClimbing = false;
  bool blendTwoTrajectories = false;
  float blendTrajMinHeight = 0.f;
  float blendTrajMaxHeight = 0.f;

  // Allows smoothing of player first-person view when acting in segmented
  // human physics segments by currTime (maxTime, zeroTimeTau, maxTimeTau)
  Point3 smoothFirstPersonTimeToTau = Point3(1.f, 0.f, 0.f);
  float smoothFirstPersonOffsetMultVel = -1.f; // to smoothen entering "real eyes"
  float smoothFirstPersonOffsetMult = 0.5f;    // "strict vs real eyes" effect coef

  // Trajectory of timed point should only be used for specific
  // segment types i.e. SEGPHYS_CLIMB_MOVE_BY_TRAJECTORY and alike
  eastl::vector<SegPhysTrajectoryPoint> trajectory;
  // Trajectory 2 is used when you need the second trajectory for
  // some purpose. One such purpose might be blending between trajectories.
  eastl::vector<SegPhysTrajectoryPoint> trajectory2;

  // GUIDELINE: When out of transitions in current segment we should
  // instead finish execution of segmented physics (make currSeg -1)
  eastl::vector<SegPhysTransition> transitions;
};

struct SegmentedHumanPhysics
{
  bool isLoaded = false;
  int reloadVersion = 0;

  eastl::vector<SegPhysSegment> segs;
  eastl::vector<eastl::string> anims;

  int initSeg_performClimb = -1;

  bool LoadFromTemplate(const DataBlock &blk);
};
struct SegmentedHumanPhysicsState
{
  int prevSeg = -1;
  int currSeg = -1;
  float prevTime = 0.f;
  float currTime = 0.f;
  float prevDuration = 0.f;
  float currDuration = 0.f;
  TMatrix currFromTM = TMatrix::IDENT;
  float currTimer = 0.f;
  int currIndex = 0;
  int syncVersion = 0;

  float blendTrajParam = 0.f; // Computed once at the specific climb segment
  bool startedOnLadder = false;

  void reset()
  {
    currSeg = -1;
    syncVersion += 1;
  }
};
