//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "math/dag_Point3.h"
#include "math/dag_Quat.h"

#include <generic/dag_span.h>
#include <debug/dag_assert.h>

class NetCrew;
class NetWeapon;
class NetWeaponControl;
struct PhysDesyncStats;
class Point3;
namespace danet
{
class BitStream;
}

class IPhysActor
{
public:
  enum RoleFlags : uint8_t
  {
    URF_INITIALIZED = 1 << 0, // If this bit is not set, than role is not defined.

    URF_LOCAL_CONTROL = 1 << 1, // Set when and only when the user controlling the unit is
                                // directly manipulating controllers of this computer or
                                // is an AI being executed on this computer

    URF_AUTHORITY = 1 << 2, // Set when and only when this instance of the unit is being executed
                            // on a trusted machine and can deal and receive damage, respawn etc.
                            // This instance confirms requests when being controlled remotely.
  };

  enum NetRole : uint8_t
  {
    // Default initial value to catch uninitialized role.
    ROLE_UNINITIALIZED = 0,

    ROLE_REMOTELY_CONTROLLED_SHADOW = URF_INITIALIZED,
    // Representation of some player/bot on another players computer.
    // Should perform no independent actions, just reflect the remote authority behaviour.
    // May never get any bonuses/spend any currency.

    ROLE_LOCALLY_CONTROLLED_SHADOW = URF_INITIALIZED | URF_LOCAL_CONTROL,
    // Players representation of himself on his computer.
    // May predict and perform some actions in order to eliminate lag,
    // but actually should request the authority that actually performs all actions
    // and then just reflect authority state.
    // Watch out for cheaters! Never get/spend any bonuses/currency directly.

    ROLE_REMOTELY_CONTROLLED_AUTHORITY = URF_INITIALIZED | URF_AUTHORITY,
    // This is server representation of a player.
    // Should check all input coming from it's controller as he may be trying to cheat.
    // Performs remotely-requested actions, deals and receives damage, etc.

    ROLE_LOCALLY_CONTROLLED_AUTHORITY = URF_INITIALIZED | URF_LOCAL_CONTROL | URF_AUTHORITY
    // This is server representation of a player playing locally on server (bot on server, dyn.campaign)
    // Can deal damage, respawn etc.
  };

protected:
  mutable NetRole netRole = ROLE_UNINITIALIZED;
// Note: explicit padding to defeat tail padding optimization (same layout across majority of platforms for compat with das aot
// compiler)
#if !_TARGET_PC_LINUX // Linux have it's own das aot compiler
private:
  char _pad[sizeof(void *) - sizeof(NetRole)]; //-V730_NOINIT
#endif
public:
  virtual ~IPhysActor() {}

  NetRole getRole() const
  {
    G_ASSERT(netRole & URF_INITIALIZED);
    return netRole;
  }
  void setRole(NetRole role_) const { netRole = role_; }
  virtual void initRole() const = 0;

  bool isAuthority() const { return getRole() & URF_AUTHORITY; }
  bool isLocalControl() const { return getRole() & URF_LOCAL_CONTROL; }
  bool isRemoteControl() const
  {
    NetRole rol3 = getRole();
    return (rol3 & URF_INITIALIZED) && !(rol3 & URF_LOCAL_CONTROL);
  }
  bool isShadow() const
  {
    NetRole rol3 = getRole();
    return (rol3 & URF_INITIALIZED) && !(rol3 & URF_AUTHORITY);
  }

  virtual bool isHumanPlayer() const { return false; }
  virtual bool isControlledHero() const { return false; }

  virtual bool isInvulnerable() const { return false; }

  virtual int getId() const { return -1; }
  virtual int getArmy() const { return 0; }
  virtual void teleportToPos(bool /*is_soft*/, const Point3 & /*pos*/) {}
  virtual void sendDesyncData(const danet::BitStream & /*sync_cur_data*/) {}
  virtual void sendDesyncStats(const PhysDesyncStats &) {}
  virtual void postPhysUpdate(int32_t /*tick*/, float /*dt*/, bool /*is_for_real*/ = true) {}
  virtual void postPhysInterpolate(float /*at_time*/, float /*dt*/) {}

  virtual uint8_t getAuthorityUnitVersion() const { return 0; }
  virtual bool isUnitVersionMatching(uint8_t) const { return true; }
  virtual void onAuthorityUnitVersion(uint8_t) {}
  virtual void debugUnitVersionMismatch(const char * /*message*/, uint8_t /*version*/) {}

  virtual void calcPosVelQuatAtTime(double /*at_time*/, DPoint3 & /*out_pos*/, Point3 & /*out_vel*/, Quat & /*out_quat*/) const {}
  inline void calcPosVelQuatAtTime(double at_time, Point3 &out_pos, Point3 &out_vel, Quat &out_quat) const
  {
    DPoint3 pos;
    calcPosVelQuatAtTime(at_time, pos, out_vel, out_quat);
    out_pos = Point3::xyz(pos);
  }
  virtual DPoint3 calcPosAtTime(double /*at_time*/) const { return DPoint3(0.0, 0.0, 0.0); }
  virtual Quat calcQuatAtTime(double /*at_time*/) const { return Quat(0.0f, 0.0f, 0.0f, 1.0f); }

  virtual void repair() {}

  virtual NetCrew *getCrew() { return NULL; }
  virtual dag::ConstSpan<NetWeapon *> getAllWeapons() const { return {}; }
  virtual dag::ConstSpan<NetWeaponControl *> getAllWeaponControls() const { return {}; }
  virtual void validateGunsLists(){};
};
