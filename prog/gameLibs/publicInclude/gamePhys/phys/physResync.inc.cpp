#include <perfMon/dag_statDrv.h>
#include <gamePhys/phys/utils.h>
#if !DISABLE_SYNC_DEBUG
#include <syncDebug/syncDebug.h>
#endif
#include <memory/dag_framemem.h>
#include <EASTL/algorithm.h>
#include <osApiWrappers/dag_atomic.h>


#ifndef APPROVED_STATES_COUNT_IN_STATS
#define APPROVED_STATES_COUNT_IN_STATS 0
#endif

template <typename PhysActor, typename PhysImpl>
void apply_authority_state(PhysActor *unit, PhysImpl &phys, float current_time, bool is_desync_reported = true,
  bool client_smoothing = true, bool dump_phys_update_info = false, danet::BitStream **sync_dump_output_stream = NULL,
  bool update_by_controls = false)
{
  if (!phys.isAuthorityApprovedStateProcessed && phys.authorityApprovedState->atTick < phys.currentState.atTick &&
      (update_by_controls ? (phys.currentState.lastAppliedControlsForTick <= 0 ||
                              phys.currentState.lastAppliedControlsForTick > phys.authorityApprovedState->lastAppliedControlsForTick)
                          : (phys.unapprovedCT.size() && phys.unapprovedCT[0].producedAtTick <= phys.authorityApprovedState->atTick)))
  {
    TIME_PROFILE(apply_authority_state);
    if (phys.authorityApprovedStateDumpFromTime >= 0.f)
      unit->onAuthorityUnitVersion(phys.authorityApprovedStateUnitVersion);
    if (!unit->isUnitVersionMatching(phys.authorityApprovedStateUnitVersion))
    {
      unit->debugUnitVersionMismatch("Windflower", phys.authorityApprovedStateUnitVersion);
    }
    else
    {
      G_ASSERT(phys.authorityApprovedState); // Assume that it cant be nullptr if !isAuthorityApprovedStateProcessed
      const typename PhysImpl::PhysStateBase &incomingAAS = *phys.authorityApprovedState;
      phys.desyncStats.processed++;

      bool isStateMatching = false;
      bool isDesyncExpected = false;
      int oldHistoryStatesCount = phys.historyStates.size();
      int numMatches = 0;
      int matchingStateIndex = -1;
      for (int i = 0; i < phys.historyStates.size(); ++i)
      {
        if (phys.historyStates[i].atTick < incomingAAS.atTick)
        {
          continue;
        }
        if (phys.historyStates[i].atTick > incomingAAS.atTick)
        {
          oldHistoryStatesCount = i;
          break;
        }
        matchingStateIndex = i;
        numMatches++;
        if (phys.historyStates[i] == incomingAAS && phys.forEachCustomStateSyncer([&](auto ss) {
              return ss->isHistoryStateEqualAuth(phys.historyStates[i].atTick, incomingAAS.atTick);
            }))
        {
          oldHistoryStatesCount = matchingStateIndex + 1;
          isStateMatching = true;
          break;
        }
        else
        {
          // Num packets with duplicates, no matter how much dups exist 1 or 100, only matters that it actually
          // exist for this state/packet
          if (numMatches > 1)
            phys.desyncStats.dups += (numMatches == 2) ? 1 : 0;
          else if (incomingAAS.canBeCheckedForSync && phys.historyStates[i].canBeCheckedForSync)
          {
            if (phys.historyStates[i].hasLargeDifferenceWith(incomingAAS))
            {
              phys.desyncStats.largeDifference++;
              // Large desync detected!
              // G_ASSERTF(0, "Large aircraft desync detected for authority state at %f", float(incomingAAS.atTime) );
            }
            else
            {
              phys.desyncStats.smallDifference++;
              // Small desync detected!
              // G_ASSERTF(0, "Small aircraft desync detected for authority state at %f", float(incomingAAS.atTime) );

              // For now let's pretend that we expect small desyncs and try debugging only large ones.
              isDesyncExpected = true;
            }
          }
          else
          {
            phys.desyncStats.untestable++;
            isDesyncExpected = true;
          }
        }
      }
      phys.desyncStats.unmatched += numMatches ? 0 : 1;

      phys.lastProcessedAuthorityApprovedStateAtTick = incomingAAS.atTick;
      int controlsTick = (update_by_controls && incomingAAS.lastAppliedControlsForTick > 0)
                           ? incomingAAS.lastAppliedControlsForTick + 1 // [start from] next after last applied
                           : incomingAAS.atTick;
      phys.applyUnapprovedCTAsAt(controlsTick, true, 0, unit->getAuthorityUnitVersion()); // discard old ones

      if (isStateMatching)
      {
        // States are matching, prepare for the next authority approved state.
        if (phys.fmsyncCurrentData)
          phys.fmsyncCurrentData->SetWriteOffset(0);
      }
      else
      {
        Tab<gamephys::Loc> previousVisualLocations(framemem_ptr());
        phys.calcVisualLocFromLocation(current_time, phys.currentState.location, previousVisualLocations);

        dag::Vector<typename PhysImpl::PhysStateBase, framemem_allocator> alternativeHistoryStates;
        if (int numStatesToErase = phys.historyStates.size() - oldHistoryStatesCount; numStatesToErase > 0) // Erase alt history (tail)
        {
          int tickToEraseSince =
            oldHistoryStatesCount < phys.historyStates.size() ? phys.historyStates[oldHistoryStatesCount].atTick : -1;
          if (eastl::is_trivially_destructible<typename PhysImpl::PhysStateBase>::value)
            alternativeHistoryStates = make_span_const(safe_at(phys.historyStates, oldHistoryStatesCount), numStatesToErase);
          else
          {
            alternativeHistoryStates.resize(numStatesToErase);
            eastl::move_n(safe_at(phys.historyStates, oldHistoryStatesCount), numStatesToErase, alternativeHistoryStates.begin());
          }
          if (tickToEraseSince >= 0)
            phys.forEachCustomStateSyncer([&](auto ss) { ss->eraseHistoryStateTail(tickToEraseSince); });
          phys.historyStates.erase(phys.historyStates.begin() + oldHistoryStatesCount, phys.historyStates.end());
        }

        auto pCustomDesyncCb = phys.getCustomResyncCb();

        typename PhysImpl::PhysStateBase desyncedState, prevDesyncedState;
        phys.saveCurrentStateTo(desyncedState, phys.currentState.atTick);
        if (pCustomDesyncCb)
          phys.saveCurrentStateTo(prevDesyncedState, phys.previousState.atTick);

        int32_t updateControlsUpToTick = (update_by_controls && phys.currentState.lastAppliedControlsForTick > 0)
                                           ? phys.currentState.lastAppliedControlsForTick + 1
                                           : phys.currentState.atTick;
        // Move aircraft to the approved position
        phys.setCurrentState(incomingAAS);
        unit->teleportToPos(true, Point3::xyz(incomingAAS.location.P));
        phys.forEachCustomStateSyncer([&](auto ss) { return ss->applyAuthState(incomingAAS.atTick); });

        // State is restored as we have a desync.
        // If we don't expect it here, then it's time to send update data for the first update
        // after the previous synced state to server for analysis.
        // And then we can reuse it once again.
        // Send only non-empty data, as empty data is useless.
        if (!isDesyncExpected && !is_desync_reported && phys.fmsyncCurrentData && phys.fmsyncCurrentData->GetNumberOfBitsUsed())
        {
          float dumpFromTime = 0;
#if !DISABLE_SYNC_DEBUG
          acesfm::sync_dump_get_time(*phys.fmsyncCurrentData);
#endif
          if (fabsf(dumpFromTime - phys.authorityApprovedStateDumpFromTime) < phys.timeStep * 0.5f)
            unit->sendDesyncData(*phys.fmsyncCurrentData);
        }

        // On client, we don't store previous data, so just clear current data and that's it
        if (phys.fmsyncCurrentData)
          phys.fmsyncCurrentData->SetWriteOffset(0);

        // We should dump fmsync data
        if (sync_dump_output_stream)
        {
          if (is_desync_reported)
            *sync_dump_output_stream = NULL;
          else
          {
            if (!phys.fmsyncCurrentData)
              phys.fmsyncCurrentData = new danet::BitStream();
            *sync_dump_output_stream = phys.fmsyncCurrentData;
          }
        }

        // Reapply 'yet to be approved' controls with fixed dt
        int currentStateIndex = -1;
        int currentAlternativeHistoryStateIndex = 0;
        for (int32_t tick = phys.currentState.atTick; controlsTick < updateControlsUpToTick; ++tick, ++controlsTick)
        {
          int newStateIndex = phys.applyUnapprovedCTAsAt(controlsTick, false, currentStateIndex, unit->getAuthorityUnitVersion());
          phys.saveCurrentToPreviousState(tick);

          if (dump_phys_update_info)
            debug("REacting cur=%d, ctl=%d, pos=(%f,%f,%f)", tick, phys.appliedCT.producedAtTick,
              (float)phys.currentState.location.P.x, (float)phys.currentState.location.P.y, (float)phys.currentState.location.P.z);

          for (; currentAlternativeHistoryStateIndex < alternativeHistoryStates.size(); ++currentAlternativeHistoryStateIndex)
          {
            if (alternativeHistoryStates[currentAlternativeHistoryStateIndex].atTick > tick)
              break;
          }
          if (currentAlternativeHistoryStateIndex < alternativeHistoryStates.size())
            phys.currentState.applyAlternativeHistoryState(alternativeHistoryStates[currentAlternativeHistoryStateIndex]);

          unit->prePhysUpdate(tick, phys.timeStep, false);
          phys.updatePhys(float(tick) * phys.timeStep, phys.timeStep, false);
          unit->postPhysUpdate(tick, phys.timeStep, false);
          phys.currentState.lastAppliedControlsForTick = controlsTick;

          if (sync_dump_output_stream)
            *sync_dump_output_stream = NULL;

          currentStateIndex = newStateIndex;
          if (phys.historyStates.size() < 4.f / phys.timeStep)
          {
            phys.saveCurrentStateToHistory();
            phys.forEachCustomStateSyncer([&](auto ss) { ss->saveHistoryState(phys.currentState.atTick); });
          }
        }
        // atTime of the flight model is now updateToTick, not updateToTick-1 !!!

        phys.interpolateVisualPosition(current_time);
        unit->postPhysInterpolate(current_time, phys.timeStep);
        phys.updateVisualError(client_smoothing, current_time, previousVisualLocations);


        phys.desyncStats.lastPosDifference.substract(desyncedState.location, phys.currentState.location);

        if (pCustomDesyncCb)
          (*pCustomDesyncCb)(&phys, prevDesyncedState, desyncedState, incomingAAS, safe_at(phys.historyStates, matchingStateIndex));
      }

      if (oldHistoryStatesCount)
      {
        int tickToEraseUpTo =
          oldHistoryStatesCount < phys.historyStates.size() ? phys.historyStates[oldHistoryStatesCount].atTick : -1;
        phys.forEachCustomStateSyncer([&](auto ss) { ss->eraseHistoryStateHead(tickToEraseUpTo); });
        phys.historyStates.erase(phys.historyStates.begin(), phys.historyStates.begin() + oldHistoryStatesCount);
      }

      // Profit!
      phys.isAuthorityApprovedStateProcessed = true;
      phys.currentState.canBeCheckedForSync = true;

      int countInStats = APPROVED_STATES_COUNT_IN_STATS;
      if (countInStats && phys.desyncStats.processed >= countInStats)
      {
        unit->sendDesyncStats(phys.desyncStats);
        phys.desyncStats.reset();
      }
    }
  }

  if (!phys.authorityApprovedPartialState.isProcessed && phys.authorityApprovedPartialState.atTick < phys.currentState.atTick &&
      phys.lastProcessedAuthorityApprovedStateAtTick > 0 &&
      phys.authorityApprovedPartialState.atTick > phys.lastProcessedAuthorityApprovedStateAtTick)
  {
    TIME_PROFILE(apply_authority_partial_state);
    unit->onAuthorityUnitVersion(phys.authorityApprovedPartialState.unitVersion);
    if (!unit->isUnitVersionMatching(phys.authorityApprovedPartialState.unitVersion))
    {
      unit->debugUnitVersionMismatch("Alstroemeria", phys.authorityApprovedPartialState.unitVersion);
    }
    else
    {
      const typename PhysImpl::PhysPartialState &incomingPAAS = phys.authorityApprovedPartialState;
      phys.partialStateDesyncStats.processed++;

      int oldHistoryStatesCount = -1;
      for (int i = 0; i < phys.historyStates.size(); ++i)
      {
        if (phys.historyStates[i].atTick < incomingPAAS.atTick)
          continue;
        if (phys.historyStates[i].atTick > incomingPAAS.atTick)
          break;

        typename PhysImpl::PhysStateBase augmentedState = phys.historyStates[i];
        augmentedState.applyPartialState(incomingPAAS);
        if (augmentedState == phys.historyStates[i])
          break;
        else
        {
          oldHistoryStatesCount = i + 1;
          // partialStateDesyncStats can log more specific stats, currently it isn't implemented here, but in a future it may be
          phys.partialStateDesyncStats.untestable++;
        }
      }

      if (oldHistoryStatesCount >= 0) // No match found
      {
        Tab<gamephys::Loc> previousVisualLocations(framemem_ptr());
        phys.calcVisualLocFromLocation(current_time, phys.currentState.location, previousVisualLocations);

        // Clear the 'alternative future' part of the history
        dag::Vector<typename PhysImpl::PhysStateBase, framemem_allocator> alternativeHistoryStates;
        if (int numStatesToErase = phys.historyStates.size() - oldHistoryStatesCount; numStatesToErase > 0)
        {
          // int tickToEraseSince = oldHistoryStatesCount < phys.historyStates.size() ?
          // phys.historyStates[oldHistoryStatesCount].atTick : -1;
          if (eastl::is_trivially_destructible<typename PhysImpl::PhysStateBase>::value)
            alternativeHistoryStates = make_span_const(safe_at(phys.historyStates, oldHistoryStatesCount), numStatesToErase);
          else
          {
            alternativeHistoryStates.resize(numStatesToErase);
            eastl::move_n(safe_at(phys.historyStates, oldHistoryStatesCount), numStatesToErase, alternativeHistoryStates.begin());
          }
          phys.historyStates.erase(phys.historyStates.begin() + oldHistoryStatesCount, phys.historyStates.end());
        }

        auto pCustomDesyncCb = phys.getCustomResyncCb();

        typename PhysImpl::PhysStateBase desyncedState, prevDesyncedState;
        phys.saveCurrentStateTo(desyncedState, phys.currentState.atTick);
        if (pCustomDesyncCb)
          phys.saveCurrentStateTo(prevDesyncedState, phys.previousState.atTick);

        typename PhysImpl::PhysStateBase augmentedState = phys.historyStates[oldHistoryStatesCount - 1];
        augmentedState.applyPartialState(incomingPAAS);
        augmentedState.applyDesyncedState(desyncedState);

        int controlsTick = (update_by_controls && incomingPAAS.lastAppliedControlsForTick > 0)
                             ? incomingPAAS.lastAppliedControlsForTick + 1 // [start from] next after last applied
                             : incomingPAAS.atTick;
        int updateControlsUpToTick = (update_by_controls && phys.currentState.lastAppliedControlsForTick > 0)
                                       ? phys.currentState.lastAppliedControlsForTick + 1
                                       : phys.currentState.atTick;
        // Move aircraft to the approved position
        phys.setCurrentState(augmentedState);
        unit->teleportToPos(true, Point3::xyz(augmentedState.location.P));
        phys.forEachCustomStateSyncer([&](auto ss) { return ss->applyPartialAuthStateState(augmentedState.atTick); });

        // We should not dump fmsync data
        if (sync_dump_output_stream)
          *sync_dump_output_stream = NULL;

        // Reapply 'yet to be approved' controls with fixed dt
        int currentStateIndex = -1;
        int currentAlternativeHistoryStateIndex = 0;
        for (int32_t tick = phys.currentState.atTick; controlsTick < updateControlsUpToTick; ++tick, ++controlsTick)
        {
          int newStateIndex = phys.applyUnapprovedCTAsAt(controlsTick, false, currentStateIndex, unit->getAuthorityUnitVersion());
          phys.saveCurrentToPreviousState(tick);

          if (dump_phys_update_info)
            debug("REacting partial cur=%f, ctl=%f, pos=(%f,%f,%f)", float(tick), float(phys.appliedCT.producedAtTick),
              (float)phys.currentState.location.P.x, (float)phys.currentState.location.P.y, (float)phys.currentState.location.P.z);

          for (; currentAlternativeHistoryStateIndex < alternativeHistoryStates.size(); ++currentAlternativeHistoryStateIndex)
          {
            if (alternativeHistoryStates[currentAlternativeHistoryStateIndex].atTick > tick)
              break;
          }
          if (currentAlternativeHistoryStateIndex < alternativeHistoryStates.size())
            phys.currentState.applyAlternativeHistoryState(alternativeHistoryStates[currentAlternativeHistoryStateIndex]);
          unit->prePhysUpdate(tick, phys.timeStep, false);
          phys.updatePhys(float(tick) * phys.timeStep, phys.timeStep, false);
          unit->postPhysUpdate(tick, phys.timeStep, false);
          phys.currentState.lastAppliedControlsForTick = controlsTick;

          currentStateIndex = newStateIndex;

          if (phys.historyStates.size() < 4.f / phys.timeStep)
            phys.saveCurrentStateToHistory();
        }
        // atTime of the flight model is now forceUpdateUpToTick, not forceUpdateUpToTick-1 !!!

        phys.interpolateVisualPosition(current_time);
        unit->postPhysInterpolate(current_time, phys.timeStep);
        phys.updateVisualError(client_smoothing, current_time, previousVisualLocations);


        phys.partialStateDesyncStats.lastPosDifference.substract(desyncedState.location, phys.currentState.location);

        if (pCustomDesyncCb)
          (*pCustomDesyncCb)(&phys, prevDesyncedState, desyncedState, augmentedState,
            safe_at(phys.historyStates, oldHistoryStatesCount - 1));
      }

      phys.authorityApprovedPartialState.isProcessed = true;
    }
  }
}

template <typename PhysImpl>
void send_authority_state(IPhysActor *unit, PhysImpl &phys, float state_send_period,
  void (*send_auth_state_cb)(IPhysActor *, danet::BitStream &&, float),
  void (*send_part_auth_state_cb)(IPhysActor *, danet::BitStream &&, int))
{
  G_ASSERT(send_auth_state_cb);
  if (!phys.authorityApprovedState)
  {
    phys.authorityApprovedState = eastl::make_unique<typename PhysImpl::PhysStateBase>();
    return; // Wait for next update
  }
  if (phys.authorityApprovedState->atTick <= 0) // not updated yet
    return;
  // Send authority approved state to the controlling client
  if (phys.physicsTimeToSendState < 0.f)
  {
    {
      danet::BitStream stateData(phys.sizeOfAuthorityState());
      phys.authorityApprovedState->serialize(stateData);
      phys.forEachCustomStateSyncer([&](auto ss) { ss->serializeAuthState(stateData, phys.authorityApprovedState->atTick); });

      float dumpFromTime = 0;
#if !DISABLE_SYNC_DEBUG
      if (phys.fmsyncCurrentData)
        // can't use default -1.f here because client use non negative value as indicator of received aa-state
        dumpFromTime = acesfm::sync_dump_get_time(*phys.fmsyncCurrentData);
#endif
      send_auth_state_cb(unit, eastl::move(stateData), dumpFromTime);
    }
    // Authority approved state is sent to client, so we should swap current and previous desync data.
    danet::BitStream *tmpBitStream = phys.fmsyncCurrentData;
    if (tmpBitStream)
    {
      phys.fmsyncCurrentData = phys.fmsyncPreviousData;
      if (!phys.fmsyncCurrentData)
        phys.fmsyncCurrentData = new danet::BitStream();
      else
        phys.fmsyncCurrentData->SetWriteOffset(0);
      phys.fmsyncPreviousData = tmpBitStream;
    }
    // Save authority approved state production time, effectively
    // marking current state as a good state to start recording desync data.
    phys.lastProcessedAuthorityApprovedStateAtTick = phys.authorityApprovedState->atTick;

    phys.physicsTimeToSendState = state_send_period;
    phys.physicsTimeToSendPartialState = state_send_period * 0.25f;
    //
    if (phys.unapprovedCT.size() > 0)
    {
      phys.currentState.canBeCheckedForSync = true;
      for (int i = 1; i < phys.unapprovedCT.size(); ++i)
      {
        if (phys.unapprovedCT[i].getSequenceNumber() - phys.unapprovedCT[i - 1].getSequenceNumber() > 1)
        {
          phys.currentState.canBeCheckedForSync = false;
          break;
        }
      }
    }
  }
  else if (phys.physicsTimeToSendPartialState <= 0.f && send_part_auth_state_cb)
  {
    typename PhysImpl::PhysPartialState savedState;
    phys.savePartialStateTo(savedState, unit->getAuthorityUnitVersion());

    danet::BitStream stateData(phys.sizeOfPartialState());
    savedState.serialize(stateData);
    phys.forEachCustomStateSyncer([&](auto ss) {
      ss->savePartialAuthState(phys.authorityApprovedState->atTick);
      ss->serializePartialAuthState(stateData, phys.authorityApprovedState->atTick);
    });
    send_part_auth_state_cb(unit, eastl::move(stateData), savedState.atTick);

    phys.physicsTimeToSendPartialState = state_send_period * 0.25f;
  }
}

template <typename PhysImpl>
ResyncState start_update_phys_for_multiplayer(IPhysActor *unit, PhysImpl &phys, float current_time, bool is_player,
  float time_speed = 1.f, bool *is_model_leaping = NULL)
{
  const int32_t deferredControllsGracePeriod = max(0, int32_t(ceilf(phys.getMaxTimeDeferredControls() / phys.timeStep)));
  const int32_t forceUpdatesGracePeriod = int32_t(ceilf(0.7f / phys.timeStep));
  float maxTotalUpdateDurationPerFrame = 0.11f;
#if DAGOR_ADDRESS_SANITIZER
  maxTotalUpdateDurationPerFrame *= 2.f;
#endif
  int32_t timespeededMaxUpdatesPerFrame = int32_t(ceilf(maxTotalUpdateDurationPerFrame / phys.timeStep * max(time_speed, 1.f)));

  int32_t currentTick = gamephys::ceilPhysicsTickNumber(current_time, phys.timeStep);

  // Do not update physics from really old states up to current time, let them leap.
  if (currentTick - phys.currentState.atTick >= forceUpdatesGracePeriod)
  {
    int32_t newAtTick = currentTick - deferredControllsGracePeriod - max(timespeededMaxUpdatesPerFrame - 1, 0);
    if (is_model_leaping && !interlocked_compare_exchange(*is_model_leaping, true, false))
      debug("leaping network unit %d from %u to %u", (int)unit->getId(), phys.currentState.atTick, newAtTick);
    phys.currentState.atTick = newAtTick;
    phys.currentState.canBeCheckedForSync = false;
  }

  int32_t maxGraceTick = currentTick - deferredControllsGracePeriod;
  int32_t latestControlledTick = (phys.unapprovedCT.size() == 0) ? maxGraceTick : phys.unapprovedCT.back().producedAtTick + 1;

  float stateTimeBefore = float(phys.currentState.atTick) * phys.timeStep;

  // We need to have the latest state that is after updatedUpToTime but before forceUpdatesUpToTime.
  // When updating for bots, we need something like previousState.atTime < currentTime and currentState.atTime > currentTime
  // When updating for players, we force updates if there are no controls for a long time, but we still want previousState.atTime <
  // currentTime
  int32_t forceUpdatesUpToTick = is_player ? max(maxGraceTick, min(latestControlledTick, currentTick)) : currentTick;
  if (forceUpdatesUpToTick == maxGraceTick)
    phys.currentState.canBeCheckedForSync = false;

  // Don't update more than maxUpdatesPerFrame times in a single frame to avoid long frames!
  forceUpdatesUpToTick = min(forceUpdatesUpToTick, phys.currentState.atTick + timespeededMaxUpdatesPerFrame);

  bool isCorrectedStateObtained = false;
  ExtrapolatedPhysState correctedExtrapolatedState;
  correctedExtrapolatedState = phys.extrapolatedState;
  if (correctedExtrapolatedState.atTime <= float(phys.currentState.atTick) * phys.timeStep)
    isCorrectedStateObtained = true;

  bool isNewCorrrectionRequired = (phys.currentState.atTick + 1 < forceUpdatesUpToTick);

  ResyncState state;
  state.needUpdateToTick = forceUpdatesUpToTick;
  state.maxGraceTick = maxGraceTick;
  state.isNewCorrrectionRequired = isNewCorrrectionRequired;
  state.isCorrectedStateObtained = isCorrectedStateObtained;
  state.stateTimeBefore = stateTimeBefore;
  state.correctedExtrapolatedState = correctedExtrapolatedState;

  return state;
}

template <typename PhysImpl>
void tick_phys_for_multiplayer(IPhysActor *unit, PhysImpl &phys, int32_t tick, int controls_tick, ResyncState &resync_state,
  bool is_player, bool is_desync_reported = true, bool dump_phys_update_info = false,
  danet::BitStream **sync_dump_output_stream = NULL)
{
  // Apply queued controls for players/just copy produced state to applied for bots
  if (is_player)
  {
    phys.applyUnapprovedCTAsAt(controls_tick, unit->isAuthority(), 0, unit->getAuthorityUnitVersion());
    if (phys.appliedCT.isControlsForged())
      phys.currentState.canBeCheckedForSync = false;
  }
  else
    phys.applyProducedState();

  // We are about to get the next state, so it's last chance so save it as the previousState
  phys.saveCurrentToPreviousState(tick);

  if (dump_phys_update_info)
    debug("acting cur=%d, ctl=%d, pos=(%f,%f,%f)", tick, phys.appliedCT.producedAtTick, (float)phys.currentState.location.P.x,
      (float)phys.currentState.location.P.y, (float)phys.currentState.location.P.z);

  // Check, whether we should dump fmsync data
  if (sync_dump_output_stream)
  {
    if (tick == phys.lastProcessedAuthorityApprovedStateAtTick && !is_desync_reported)
    {
      if (!phys.fmsyncCurrentData)
        phys.fmsyncCurrentData = new danet::BitStream();
      interlocked_release_store_ptr(*sync_dump_output_stream, phys.fmsyncCurrentData);
    }
    else
      interlocked_release_store_ptr(*sync_dump_output_stream, (danet::BitStream *)nullptr);
  }

  float curTime = tick * phys.timeStep;

  // Act physics, updating currentState up to updatedUpToTime
  unit->prePhysUpdate(tick, phys.timeStep, true);
  phys.updatePhys(curTime, phys.timeStep, true);
  unit->postPhysUpdate(tick, phys.timeStep, true);
  if (sync_dump_output_stream)
    interlocked_release_store_ptr(*sync_dump_output_stream, (danet::BitStream *)nullptr);

  // Authority keeps an extrapolated fakeState of remotely controlled ships,
  // If we update through time, where fakeState is, we can calculate the extrapolation error and fix it slowly
  if (unit->getRole() == IPhysActor::ROLE_REMOTELY_CONTROLLED_AUTHORITY &&
      phys.extrapolatedState.atTime >= float(phys.currentState.atTick) * phys.timeStep - phys.timeStep &&
      phys.extrapolatedState.atTime <= float(phys.currentState.atTick) * phys.timeStep && !resync_state.isCorrectedStateObtained)
  {
    phys.interpolateVisualPosition(phys.extrapolatedState.atTime);
    resync_state.correctedExtrapolatedState.location = phys.visualLocation;
    resync_state.correctedExtrapolatedState.atTime = phys.extrapolatedState.atTime;
    resync_state.isCorrectedStateObtained = true;
  }
  if (unit->isShadow() && phys.historyStates.size() < 4.f / phys.timeStep)
  {
    phys.saveCurrentStateToHistory();
    phys.forEachCustomStateSyncer([&](auto ss) { ss->saveHistoryState(phys.currentState.atTick); });
  }
}

template <typename PhysImpl>
void finish_update_phys_for_multiplayer(IPhysActor *unit, PhysImpl &phys, ResyncState &resync_state, float current_time,
  void (*on_frame_end_cb)(IPhysActor *net_unit) = NULL)
{
  if (on_frame_end_cb != NULL)
    on_frame_end_cb(unit);

  // If ship physics state weas updated, calculate how much.
  // We want to periodically send authority state to clients, but periodically in 'the ship time', not in real one.
  float stateTimeAfter = float(phys.currentState.atTick) * phys.timeStep;
  float updatedBySeconds = stateTimeAfter - resync_state.stateTimeBefore;
  // G_ASSERT(updatedBySeconds >= 0.f);
  phys.physicsTimeToSendState -= updatedBySeconds;
  phys.physicsTimeToSendPartialState -= updatedBySeconds;

  // save the state we've just obtained
  if (unit->isAuthority() && phys.physicsTimeToSendState < 0.f && phys.authorityApprovedState) // Note: allocated on first send
  {
    phys.saveCurrentStateTo(*phys.authorityApprovedState, phys.currentState.atTick);
    phys.forEachCustomStateSyncer([&](auto ss) { ss->saveAuthState(phys.currentState.atTick); });
  }

  // Calculate visual location error in order to smoothen visual location of the aircraft on the authority.
  // This will result in some sort of lag-compensation.
  if (unit->getRole() == IPhysActor::ROLE_REMOTELY_CONTROLLED_AUTHORITY)
  {
    if (resync_state.isNewCorrrectionRequired)
    {
      // Obtain visual location as it were back then
      // We have old extrapolatedState and new correctedExtrapolatedState, for two versions of the same time.
      Tab<gamephys::Loc> oldSmoothenedLocations(framemem_ptr());
      phys.calcVisualLocFromLocation(phys.extrapolatedState.atTime, phys.extrapolatedState.location, oldSmoothenedLocations);

      // If we haven't obtained a corrected state yet, obtain the best we can.
      // Extrapolate to the previous extrapolatedState time,
      // getting a correctedExtrapolatedState that is better according to our current knowledge of aircraft controls.
      if (!resync_state.isCorrectedStateObtained)
      {
        phys.extrapolateMinimalState(phys.currentState, phys.extrapolatedState.atTime, resync_state.correctedExtrapolatedState);
        // it changes visual location for SubVehicles of unit, but don't need to return them back: it will be done below, after
        // inerpolation/extrapolation of phys
        phys.postPhysExtrapolation(phys.extrapolatedState.atTime, resync_state.correctedExtrapolatedState);
      }

      // Obtain visual location error.
      phys.updateVisualErrorForNewCorrection(phys.extrapolatedState.atTime, oldSmoothenedLocations,
        resync_state.correctedExtrapolatedState.location);
    }
  }

  // Obtain extrapolated state.
  // We want to have extrapolatedState that contains extra/interpolated position of all the aircrafts at currentTime
  // For locally controlled aircrafts we need to interpolate visual position between pervious and current states
  // every time updateInThreads is called, so the aircrafts move smoothly.
  const bool isExtrapolation = current_time > float(phys.currentState.atTick) * phys.timeStep;
  if (isExtrapolation)
  {
    // Extrapolate to currentTime
    phys.extrapolateMinimalState(phys.currentState, current_time, phys.extrapolatedState);
    phys.postPhysExtrapolation(current_time, phys.extrapolatedState);
  }
  else
  {
    // We're lucky, we can use interpolation, not extrapolation
    phys.extrapolatedState.copyFrom(phys.currentState, float(phys.currentState.atTick) * phys.timeStep);
    phys.interpolateVisualPosition(current_time);
    phys.extrapolatedState.location = phys.visualLocation;
    phys.extrapolatedState.atTime = current_time;
    unit->postPhysInterpolate(current_time, phys.timeStep);
  }

  // Obtain visual location.
  // Apply current extrapolation error, getting visual location
  phys.fixVisualErrors(current_time);
}

template <typename PhysImpl>
void update_phys_for_multiplayer(IPhysActor *unit, PhysImpl &phys, float current_time, bool is_player, float time_speed = 1.f,
  bool *is_model_leaping = NULL, bool is_desync_reported = true, bool dump_phys_update_info = false,
  danet::BitStream **sync_dump_output_stream = NULL, void (*on_frame_end_cb)(IPhysActor *net_unit) = NULL)
{
  ResyncState resyncState = start_update_phys_for_multiplayer(unit, phys, current_time, is_player, time_speed, is_model_leaping);
  for (int32_t tick = phys.currentState.atTick; tick < resyncState.needUpdateToTick; ++tick)
    tick_phys_for_multiplayer(unit, phys, tick, tick, resyncState, is_player, is_desync_reported, dump_phys_update_info,
      sync_dump_output_stream);
  finish_update_phys_for_multiplayer(unit, phys, resyncState, current_time, on_frame_end_cb);
}
