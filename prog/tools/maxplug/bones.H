// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Interface to Bones

#define BONESOBJ_CID Class_ID(0x40203170, 0x1ec45730)
#define BONESMOD_CID Class_ID(0x1bbc3fc5, 0x744b2909)
#define I_BONES      0x40203170
#define I_BONES_WARP 0x744b2909

struct Bone
{
  INode *node;
  float zone, str;
  int exnum;
  short *excl;
};

struct BoneData
{
  int node;
  Matrix3 tm, nodetm;
};

class BonesModData : public LocalModData
{
public:
  Tab<BoneData> bone;
  Tab<float> pt;
  int VN, BN;
};
