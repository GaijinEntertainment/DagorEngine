//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/phys/commonPhysBase.h>
#include <daECS/core/componentType.h>
#include <EASTL/vector_map.h>
#include <dag/dag_vector.h>

namespace danet
{
class BitStream;
}

class ECSCustomPhysStateSyncer final : public ICustomPhysStateSyncer
{
public:
  ECSCustomPhysStateSyncer() = default;
  ECSCustomPhysStateSyncer(const ECSCustomPhysStateSyncer &) = delete;
  ECSCustomPhysStateSyncer &operator=(const ECSCustomPhysStateSyncer &) = delete;
  bool operator==(ECSCustomPhysStateSyncer &) const = delete;

  void init(ecs::EntityId eid_, int resv = 0);
  void registerSyncComponent(const char *comp_name, float &comp_ref);
  void registerSyncComponent(const char *comp_name, int &comp_ref);
  void registerSyncComponent(const char *comp_name, bool &comp_ref);

private:
  template <typename T>
  void regSyncComp(const char *cn, T &cr);
  void save(const eastl::pair<int, int> &state);
  void clearHistory();

  void saveHistoryState(int tick) override;
  void saveAuthState(int tick) override
  {
    AAS.first = tick;
    save(AAS);
  }
  bool isHistoryStateEqualAuth(int tick, int aas_tick) override;
  void serializeAuthState(danet::BitStream &bs, int tick) const override;
  bool deserializeAuthState(const danet::BitStream &bs, int tick) override;
  void eraseHistoryStateTail(int tick) override;
  void eraseHistoryStateHead(int tick) override;
  void applyAuthState(int tick) override;
  void clear() override;
  bool isHomogeneousData() const { return !(numDataBits & 7) && numDataBits == data_size(data) * CHAR_BIT; }

private:
  ecs::EntityId eid;
  int numDataBits = 0;
  dag::Vector<eastl::pair<ecs::hash_str_t, ecs::component_type_t>> scheme;
  // To consider: we can actually store hashes of history states instead of states itself
  // (but it would be harder to debug/trace)
  eastl::vector_map</*tick*/ int, /*data's index*/ int> history;
  eastl::pair</*tick*/ int, /*data's index*/ int> AAS = {0, 0};
  dag::Vector<int, EASTLAllocatorType, /*init*/ false> data; // Note: First `scheme.size` elements is AAS's ones
};

ECS_DECLARE_BOXED_TYPE(ECSCustomPhysStateSyncer); // Physics might ref instances of this class therefore it can't be relocatable
