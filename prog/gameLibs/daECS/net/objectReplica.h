// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_tab.h>
#include <daECS/net/sequence.h>
#include <daECS/net/object.h>

namespace net
{

class Object;
class Connection;

#define MAX_OBJ_CREATION_SEQ ((1 << 24) - 1)

// Object's view from connection POV
struct ObjectReplica
{
  enum Flags // Sorted by update priority
  {
    Free = 0,                  // Free slot
    Allocated = 1 << 0,        // To differentiate from free slot
    InScope = 1 << 1,          // Object in scope for this connection (i.e. will get updates)
    AlwaysInScope = 1 << 2,    // Won't loose InScope flag after updates
    NotYetReplicated = 1 << 3, // Creation packet wasn't sent yet
    ToKill = 1 << 4,           // Object will be killed
  };
  uint16_t arrayIndex; // dynamic index of this replica in 'replicas' array
  uint16_t gen;        // generation of this replica (needed for handles build)

  union
  {
    uint32_t cmp;
    struct
    {
      uint8_t low[3];
      uint8_t flags; // MSB (little-endianess assumed)
    };
  };

  ecs::entity_id_t eidStorage; // entity_id instead of EntityId to avoid init by default
  Connection *conn;
  CompVersMap remoteCompVers;         // remote versions of all components for this object (from server's POV)
  ObjectReplica *prevRepl, *nextRepl; // double linklist of all replicas for this particular object. TODO: replace with index in
                                      // connection

  template <unsigned F>
  bool isFlagT() const
  {
    return (flags & F) != 0;
  }
  bool isNeedInitialUpdate() const { return isFlagT<NotYetReplicated>(); }
  bool isToKill() const { return isFlagT<ToKill>(); }
  bool isFree() const { return flags == Free; }
  int getPriority() const { return flags; }
  ecs::EntityId getEid() const { return ecs::EntityId(eidStorage); }

  void debugVerifyRemoteCompVers(const CompVersMap &local_comp_vers, bool shallow_check = false) const;

  void detachFromObj(Object &obj)
  {
#if DAECS_EXTENSIVE_CHECKS
    G_ASSERT(obj.getEid() == getEid());
#endif
    if (prevRepl)
      prevRepl->nextRepl = nextRepl;
    else
    {
      G_ASSERT(obj.replicasLinkList == this);
      obj.replicasLinkList = nextRepl;
    }
    if (nextRepl)
      nextRepl->prevRepl = prevRepl;
  }
};

}; // namespace net
