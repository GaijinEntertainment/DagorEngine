// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

struct AnimChanPoint3
{
  Tab<PosKey> key;
};

struct AnimChanQuat
{
  Tab<RotKey> key;
};

struct AnimKeyPoint3
{
  Point3 p, k1, k2, k3;
};

void get_tm_anim_2(Tab<AnimChanPoint3 *> &pos, Tab<AnimChanQuat *> &rot, Tab<AnimChanPoint3 *> &scl, const Interval &limit,
  const Tab<INode *> &node, const Tab<ExpTMAnimCB *> &cb, float pos_thr, float rot_eps, float scl_thr, float ort_eps, int explags);

void get_node_vel(Tab<AnimKeyPoint3> &lin_vel, Tab<AnimKeyPoint3> &ang_vel, Tab<int> &lin_vel_t, Tab<int> &ang_vel_t,
  const Interval &limit, ExpTMAnimCB &cb, float pos_thr, float rot_eps, float ort_eps, int explags);
