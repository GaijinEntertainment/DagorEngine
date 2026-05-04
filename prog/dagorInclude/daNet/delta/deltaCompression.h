//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>
#include <generic/dag_tab.h>
#include <math/dag_adjpow2.h>
#include <daNet/bitStream.h>

namespace net
{

struct DeltaComp
{
  uint32_t historyBits;
  uint32_t indexBits;

  struct History
  {
    dag::Vector<danet::BitStream> baseHist;
    struct CacheEntry
    {
      danet::BitStream bs;
      bool isFullDiff = false;
      uint32_t basePacketNo = 0;
      uint32_t nextPacketNo = 0;
      bool isEmpty() const { return bs.GetWriteOffset() == 0; }
      void clear() { bs.SetWriteOffset(0); }
    };
    dag::Vector<CacheEntry> compressedCache;
    uint32_t curPacketTotalNo = 0;
    uint32_t curPacketNo = 0;
    bool isEnabled = false;
    bool isInited() const { return !baseHist.empty(); }

    struct Stats
    {
      bool isHistoryBroken = false;
      uint32_t lastReceivedBasePacketNo = 0;
      uint32_t lastReceivedNextPacketNo = 0;
      uint32_t lastCompressedDataSizeBytes = 0;
    } stats;

    bool isValidBasePacket(uint32_t packet) const
    {
      return curPacketNo - packet < baseHist.size() && baseHist[packet & (baseHist.size() - 1u)].GetNumberOfBitsUsed() > 0 &&
             isEnabled;
    }

#if 0
    mutable dag::Vector<uint32_t> dbgPacketHistory;
    void pushDbgPacketHistory(uint32_t packetNo) const
    {
      if (dbgPacketHistory.size() == 1024)
        dbgPacketHistory.pop_back();
      dbgPacketHistory.emplace(dbgPacketHistory.begin(), packetNo);
    }
#else
    void pushDbgPacketHistory(uint32_t) const {}
#endif
  };

  struct State
  {
    uint32_t lastConfirmedPacketNo : 31 = 0;
    bool validLastConfirmedPacketNo : 1 = false;
    uint32_t lastSentPacketTotalNo = 0;
  };

  struct ReadResult
  {
    danet::BitStream bs;
    bool ok = false;
    bool isResetNeeded = false;
    bool isOutOfOrder = false;
    bool isFullDiff = false;
    uint32_t confirmPacketNo = 0;
  };

  DeltaComp(uint32_t history_bits, uint32_t index_bits);

  void initHistory(History &history, bool is_enabled, bool use_cache);

  danet::BitStream &writeNextData(History &history);
  const danet::BitStream &getData(const History &history);
  uint32_t getHistoryIdxForPacket(const History &history, uint32_t packet_no);

  void writeDelta(danet::BitStream &bs, History &history, State &state, bool reliable_channel, bool use_cache = true);
  ReadResult readDelta(const danet::BitStream &bs, History *history);

  void writeConfirm(danet::BitStream &bs, const ReadResult &result);
  bool readConfirm(const danet::BitStream &bs, const History *history, State *state);
  void resetState(State &state);

  void writeReplaySnapshot(danet::BitStream &bs, const History &history);
  bool readReplaySnapshot(const danet::BitStream &bs, History *history);
};

} // namespace net
