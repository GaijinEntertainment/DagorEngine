// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>
#include <generic/dag_span.h>
#include <util/dag_compilerDefs.h>

// Reliable sync state of a phys actor as known by one connection, stored packed
// in BasePhysActor::allSyncStates (see phys_sync_states_packed_t). Kept free of
// framework dependencies so unit tests can exercise it (samples/unittest).
enum class PhysActorSyncState // Note: shall fit in 2 bits
{
  Normal,
  Asleep,
  Hidden,
  Invalid // can't happen
};

static constexpr unsigned PHYS_SYNC_STATES_PER_BYTE = 4; // CHAR_BIT/2 bits

// Ids beyond stored size read as Normal (nothing was sent reliably yet).
// conn_became_responsive also reads as Normal: after a period of unresponsiveness
// all reliable state changes (asleep/hidden) must be resent.
inline PhysActorSyncState read_phys_actor_sync_state(
  dag::ConstSpan<uint8_t> sync_states, unsigned net_phys_id, bool conn_became_responsive)
{
  unsigned byteIdx = net_phys_id / PHYS_SYNC_STATES_PER_BYTE;
  if (byteIdx >= (unsigned)sync_states.size() || conn_became_responsive)
    return PhysActorSyncState::Normal;
  unsigned bshift = net_phys_id % PHYS_SYNC_STATES_PER_BYTE * 2;
  return PhysActorSyncState((sync_states[byteIdx] >> bshift) % PHYS_SYNC_STATES_PER_BYTE);
}

template <typename SyncStates>
inline void write_phys_actor_sync_state(SyncStates &sync_states, unsigned net_phys_id, PhysActorSyncState state)
{
  unsigned byteIdx = net_phys_id / PHYS_SYNC_STATES_PER_BYTE;
  if (DAGOR_UNLIKELY(byteIdx >= sync_states.size()))
    sync_states.resize(byteIdx + 1); // new bytes are zero-filled, i.e. Normal
  unsigned bshift = net_phys_id % PHYS_SYNC_STATES_PER_BYTE * 2;
  sync_states[byteIdx] = (sync_states[byteIdx] & ~(0b11 << bshift)) | (unsigned(state) << bshift);
}

// While the client already knows an actor as hidden its visibility is re-queried
// at 1/4 of send rate, staggered by net_phys_id so queries spread evenly across
// send ticks (delays hidden -> min visible flip by a few send ticks at most).
inline bool should_query_phys_actor_min_visible(PhysActorSyncState prev_sync_state, unsigned send_seq, unsigned net_phys_id)
{
  return prev_sync_state != PhysActorSyncState::Hidden || ((send_seq + net_phys_id) & 3u) == 0;
}
