// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class AcesEffect;

struct JetpackExhaust
{
  struct ExhaustNode
  {
    AcesEffect *fx = nullptr;
    dag::Index16 nodeId;
    int fxId = -1;
  };
  Tab<ExhaustNode> exhaustFx;

  bool onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid);
};

ECS_DECLARE_RELOCATABLE_TYPE(JetpackExhaust);
