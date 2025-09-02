//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

enum TransObjectType
{
  TRANS_INVALID,
  TRANS_EFFECT,
  TRANS_FM,
  // free for use
  TRANS_UNUSED,
  TRANS_CABLE,
  TRANS_BALOON,
  TRANS_CABLE_REND,

  TRANS_NUM
};

struct SortedRenderTransItem
{
  SortedRenderTransItem() : object(NULL), type(TRANS_INVALID), distance(0) {}

  SortedRenderTransItem(void *obj, TransObjectType t, float dist) : object(obj), type(t), distance(dist) {}

  void *object;
  TransObjectType type;
  float distance;
};

class SortedRenderTransItemCompare
{
public:
  inline bool operator()(SortedRenderTransItem const &a, SortedRenderTransItem const &b) const { return a.distance > b.distance; }
};