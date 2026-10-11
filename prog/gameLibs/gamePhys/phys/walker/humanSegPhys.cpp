// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/walker/humanSegPhys.h>
#include <util/dag_oaHashNameMap.h>

static SegPhysCheckType StrToSegCheckType(const char *str)
{
  if (!str || str[0] == '\0')
    return SEGPHYS_ON_INVALID;
  if (!strcmp(str, "ALWAYS"))
    return SEGPHYS_ON_ALWAYS;
  if (!strcmp(str, "IS_CLIMB_THRU"))
    return SEGPHYS_ON_IS_CLIMB_THRU;
  if (!strcmp(str, "CLIMB_CHECK_FLOOR"))
    return SEGPHYS_ON_CLIMB_CHECK_FLOOR;
  if (!strcmp(str, "CLIMB_HEIGHT_LESS"))
    return SEGPHYS_ON_CLIMB_HEIGHT_LESS;
  if (!strcmp(str, "STARTED_ON_LADDER"))
    return SEGPHYS_ON_STARTED_ON_LADDER;
  return SEGPHYS_ON_INVALID;
}

static SegPhysSegmentType StrToSegType(const char *str)
{
  if (!str || str[0] == '\0')
    return SEGPHYS_INVALID;
  if (!strcmp(str, "End"))
    return SEGPHYS_END;
  if (!strcmp(str, "Pass"))
    return SEGPHYS_PASS;
  if (!strcmp(str, "Wait"))
    return SEGPHYS_WAIT;
  if (!strcmp(str, "ClimbMoveByTrajectory"))
    return SEGPHYS_CLIMB_MOVE_BY_TRAJECTORY;
  if (!strcmp(str, "ClimbMoveToTrajectory"))
    return SEGPHYS_CLIMB_MOVE_TO_TRAJECTORY;
  if (!strcmp(str, "ClimbStart"))
    return SEGPHYS_CLIMB_START;
  if (!strcmp(str, "ClimbEnd"))
    return SEGPHYS_CLIMB_END;
  if (!strcmp(str, "ClimbResumeDefault"))
    return SEGPHYS_CLIMB_RESUME_DEFAULT;
  return SEGPHYS_INVALID;
}

static int loadSegPhysTrajectory(eastl::vector<SegPhysTrajectoryPoint> &trajectory, const DataBlock *trajBlk, const char *segName)
{
  int numErrors = 0;
  Point3 trajDeltaPos = trajBlk->getPoint3("dp", Point3::ZERO);
  for (int i = 0; i < trajBlk->blockCount(); ++i)
  {
    const DataBlock *keyBlk = trajBlk->getBlock(i);
    const char *keyName = keyBlk->getBlockName();
    if (!keyName || strcmp(keyName, "key") != 0)
    {
      numErrors += 1;
      logerr("SegmentedHumanPhysics: Segment trajectory key name '%s' invalid in segment '%s'", keyName, segName);
      continue;
    }

    SegPhysTrajectoryPoint pt;
    pt.t = keyBlk->getReal("t");
    pt.h = keyBlk->getReal("h");
    pt.p = keyBlk->getPoint3("p");
    pt.p += trajDeltaPos;

    if (trajectory.empty() && pt.t != 0.f)
    {
      numErrors += 1;
      logerr("SegmentedHumanPhysics: Trajectory time starts not from 0 (%f) in segment '%s'", pt.t, segName);
      continue;
    }
    else if (!trajectory.empty() && trajectory.back().t >= pt.t)
    {
      numErrors += 1;
      logerr("SegmentedHumanPhysics: Trajectory time goes back or same (%f -> %f) in segment '%s'", trajectory.back().t, pt.t,
        segName);
      continue;
    }

    trajectory.emplace_back(pt);
    // logerr("!!! LoadFromTemplate: trajectory point (%f %f %f) t=%f in segment '%s'", pt.p.x, pt.p.y, pt.p.z, pt.t, segName);
  }

  return numErrors;
}

bool SegmentedHumanPhysics::LoadFromTemplate(const DataBlock &blk)
{
  const int wasReloadVersion = reloadVersion;
  *this = SegmentedHumanPhysics(); // clear data
  reloadVersion = wasReloadVersion + 1;

  int numErrors = 0;
  NameMap segNames;
  NameMap animNames;

  const DataBlock *segmentsBlk = blk.getBlockByName("segments");
  if (segmentsBlk)
  {
    for (int i = 0; i < segmentsBlk->blockCount(); ++i)
    {
      const DataBlock *segBlk = segmentsBlk->getBlock(i);

      const char *segName = segBlk->getBlockName();
      int segIdx = segNames.getNameId(segName);
      if (segIdx != -1 && segs[segIdx].type != SEGPHYS_INVALID)
      {
        numErrors += 1;
        logerr("SegmentedHumanPhysics: Segment with duplicate name '%s' ignored", segName);
        continue;
      }

      if (segIdx < 0)
      {
        segIdx = segNames.addNameId(segName);
        G_ASSERT(segIdx == segs.size());
        segs.emplace_back(SegPhysSegment());
        segs.back().name = segName;
        // logerr("!!! LoadFromTemplate: added seg '%s' idx=%d", segName, segIdx);
      }
      else
      {
        // logerr("!!! LoadFromTemplate: finish seg '%s' idx=%d", segName, segIdx);
      }

      {
        SegPhysSegment &seg = segs[segIdx];
        const char *typeStr = segBlk->getStr("type", nullptr);
        seg.type = StrToSegType(typeStr);
        if (seg.type == SEGPHYS_INVALID)
        {
          numErrors += 1;
          logerr("SegmentedHumanPhysics: Segment type invalid (%s) for segment '%s'", typeStr ? typeStr : "<empty>", segName);
          continue;
        }

        const char *anim = segBlk->getStr("anim", nullptr);
        if (anim && anim[0] != '\0')
        {
          seg.animID = animNames.getNameId(anim);
          if (seg.animID < 0)
          {
            seg.animID = animNames.addNameId(anim);
            G_ASSERT(seg.animID == anims.size());
            anims.push_back(anim);
          }
        }

        const DataBlock *paramsBlk = segBlk->getBlockByName("params");
        if (paramsBlk)
        {
          seg.inSkip = paramsBlk->getBool("inSkip", seg.inSkip);
          seg.inTime = paramsBlk->getReal("inTime", seg.inTime);
          seg.speedCoef = paramsBlk->getReal("speedCoef", seg.speedCoef);
          seg.speedCoef2 = paramsBlk->getReal("speedCoef2", seg.speedCoef);
          seg.stuckDist = paramsBlk->getReal("stuckDist", seg.stuckDist);
          seg.timeout = paramsBlk->getReal("timeout", seg.timeout);
          seg.maxCorrectionSpeed = paramsBlk->getReal("maxCorrectionSpeed", seg.maxCorrectionSpeed);
          seg.noGravity = paramsBlk->getBool("noGravity", seg.noGravity);
          seg.setRemoteClimbing = paramsBlk->getBool("setRemoteClimbing", seg.setRemoteClimbing);
          seg.blendTwoTrajectories = paramsBlk->getBool("blendTwoTrajectories", seg.blendTwoTrajectories);
          seg.blendTrajMinHeight = paramsBlk->getReal("blendTrajMinHeight", seg.blendTrajMinHeight);
          seg.blendTrajMaxHeight = paramsBlk->getReal("blendTrajMaxHeight", seg.blendTrajMaxHeight);
          seg.smoothFirstPersonTimeToTau = paramsBlk->getPoint3("smoothFirstPersonTimeToTau", seg.smoothFirstPersonTimeToTau);
          seg.smoothFirstPersonOffsetMultVel =
            paramsBlk->getReal("smoothFirstPersonOffsetMultVel", seg.smoothFirstPersonOffsetMultVel);
          seg.smoothFirstPersonOffsetMult = paramsBlk->getReal("smoothFirstPersonOffsetMult", seg.smoothFirstPersonOffsetMult);
        }

        const DataBlock *trajBlk = segBlk->getBlockByName("trajectory");
        if (trajBlk)
        {
          numErrors += loadSegPhysTrajectory(seg.trajectory, trajBlk, segName);
        }
        if (seg.blendTwoTrajectories)
        {
          const DataBlock *trajBlk2 = segBlk->getBlockByName("trajectory2");
          if (trajBlk2)
          {
            numErrors += loadSegPhysTrajectory(seg.trajectory2, trajBlk2, segName);
          }
          if (seg.trajectory.size() == 0 || seg.trajectory2.size() == 0)
          {
            numErrors += 1;
            logerr("SegmentedHumanPhysics: Trajectories have to be non-zero length to be blended (segment: '%s', trajectory size: %@, "
                   "trajectory2 size: %@)",
              seg.trajectory.size(), seg.trajectory2.size(), segName);
          }
          else if (seg.trajectory.size() != seg.trajectory2.size())
          {
            numErrors += 1;
            logerr("SegmentedHumanPhysics: Trajectories have to be the same length to be blended (segment: '%s', trajectory size: %@, "
                   "trajectory2 size: %@)",
              seg.trajectory.size(), seg.trajectory2.size(), segName);
          }
          else if (seg.trajectory.back().t != seg.trajectory2.back().t)
          {
            numErrors += 1;
            logerr("SegmentedHumanPhysics: Trajectories have to be the same duration to be blended (segment: '%s', trajectory "
                   "duration: %@, trajectory2 duration: %@)",
              seg.trajectory.back().t, seg.trajectory2.back().t, segName);
          }
        }
        if (seg.type == SEGPHYS_CLIMB_MOVE_TO_TRAJECTORY && seg.trajectory.size() != 2)
        {
          numErrors += 1;
          logerr("SegmentedHumanPhysics: ClimbMoveToTrajectory should contain trajectory with exactly two keys to compute direction "
                 "and speed of the pull (segment: '%s')",
            segName);
        }
      }

      const int trNameId = segBlk->getNameId("transition");
      for (int i = 0; i < segBlk->blockCount(); ++i)
      {
        const DataBlock *trBlk = segBlk->getBlock(i);
        if (trBlk->getBlockNameId() == trNameId)
        {
          SegPhysTransition tr;
          const char *cond = trBlk->getStr("on", nullptr);
          if (cond && cond[0] == '!')
          {
            tr.not_on = true;
            ++cond;
          }
          tr.on = StrToSegCheckType(cond);
          if (tr.on == SEGPHYS_ON_INVALID)
          {
            numErrors += 1;
            logerr("SegmentedHumanPhysics: Transition condition invalid (%s) for segment '%s'", cond ? cond : "<empty>", segName);
            continue;
          }
          tr.offs = trBlk->getPoint3("offs", tr.offs);
          tr.len = trBlk->getReal("len", 0.f);
          const char *toSegName = trBlk->getStr("to", nullptr);
          if (!toSegName || toSegName[0] == '\0')
          {
            numErrors += 1;
            logerr("SegmentedHumanPhysics: Transition target name not defined or empty (to) for segment '%s'", segName);
            continue;
          }
          tr.to = segNames.getNameId(toSegName);
          if (tr.to < 0)
          {
            tr.to = segNames.addNameId(toSegName);
            G_ASSERT(tr.to == segs.size());
            segs.emplace_back(SegPhysSegment());
            segs.back().name = toSegName;
            // logerr("!!! LoadFromTemplate: pre-made seg '%s' idx=%d", toSegName, tr.to);
          }
          tr.disp = trBlk->getPoint3("disp", Point3::ZERO);

          segs[segIdx].transitions.emplace_back(tr);
          // logerr("!!! LoadFromTemplate: added transition to '%s' (inv=%d on=%d to=%d len=%f disp=%f %f %f) in segment '%s'",
          //   toSegName, tr.inv, tr.on, tr.to, tr.len, tr.disp.x, tr.disp.y, tr.disp.z, segs[segIdx].name.c_str());
        }
      }

      // SegPhysSegment &seg = segs[segIdx];
      // const int trajKeys = seg.trajectory.size();
      // const float trajTime = seg.trajectory.empty() ? 0.f : seg.trajectory.back().t;
      // const char *animName = seg.animID >= 0 ? anims[seg.animID].c_str() : "<no anim>";
      // logerr("!!! LoadFromTemplate: (%d) seg '%s' type=%d animID=%d animName=%s trajKeys=%d trajTime=%f speedCoef=%f threshold=%f
      // maxTime=%f endTime=%f",
      //   i, segName, seg.type, seg.animID, animName, trajKeys, trajTime, seg.speedCoef, seg.threshold, seg.maxTime, seg.endTime);
    }

    for (int i = 0; i < segs.size(); ++i)
    {
      if (segs[i].type == SEGPHYS_INVALID)
      {
        numErrors += 1;
        logerr("SegmentedHumanPhysics: Segment '%s' was not defined, but used in transitions", segs[i].name.c_str());
        segs[i].type = SEGPHYS_PASS;
      }
      else if ((segs[i].type == SEGPHYS_END || segs[i].type == SEGPHYS_CLIMB_END) && !segs[i].transitions.empty())
      {
        numErrors += 1;
        logerr("SegmentedHumanPhysics: Segment '%s' of end-type has transitions", segs[i].name.c_str());
        segs[i].transitions.clear();
        continue;
      }
    }
  }
  else
  {
    numErrors += 1;
    logerr("SegmentedHumanPhysics: No segments");
  }

  initSeg_performClimb = segNames.getNameId(blk.getStr("initSeg_performClimb", nullptr));

  isLoaded = numErrors == 0;
  return isLoaded;
}
