// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/delta/deltaCompression.h>

#include <util/dag_hash.h>
#include <memory/dag_framemem.h>
#include <daNet/daNetTypes.h>
#include <daNet/delta/rle.h>
#include <daNet/delta/history.h>


#define VALIDATE_COMPRESSION_HASH 0 // NOTE: used for debug only, changing this changes protocol

namespace net
{

DeltaComp::DeltaComp(uint32_t history_bits, uint32_t index_bits) : historyBits(history_bits), indexBits(index_bits) {}

void DeltaComp::initHistory(History &history, bool is_enabled, bool use_cache)
{
  history = History();
  history.isEnabled = is_enabled;
  history.baseHist.resize(is_enabled ? 1u << historyBits : 1u);
  if (use_cache && is_enabled)
    history.compressedCache.resize(history.baseHist.size());
}

danet::BitStream &DeltaComp::writeNextData(History &history)
{
  history.curPacketTotalNo++;
  history.curPacketNo = (history.curPacketTotalNo) & ((1u << indexBits) - 1u);
  for (History::CacheEntry &c : history.compressedCache)
    c.clear();
  danet::BitStream &bs = history.baseHist[history.curPacketNo & (history.baseHist.size() - 1u)];
  bs.SetWriteOffset(0);
  return bs;
}

const danet::BitStream &DeltaComp::getData(const History &history)
{
  return history.baseHist[history.curPacketNo & (history.baseHist.size() - 1u)];
}

uint32_t DeltaComp::getHistoryIdxForPacket(const History &history, uint32_t packet_no)
{
  return packet_no & (history.baseHist.size() - 1u);
}

static void write_aligned_bytes(danet::BitStream &dst, const uint8_t *data, uint32_t size_bytes)
{
#if 0
  dst.WriteAlignedBytes(data, size_bytes);
#else
  dst.AlignWriteToByteBoundary();
  dst.reserveBits(BYTES_TO_BITS(size_bytes));
  memcpy(dst.GetData() + BITS_TO_BYTES(dst.GetWriteOffset()), data, size_bytes);
  dst.SetWriteOffset(BYTES_TO_BITS(BITS_TO_BYTES(dst.GetWriteOffset()) + size_bytes));
#endif
}

void DeltaComp::writeDelta(danet::BitStream &out_data, History &history, State &state, bool reliable_channel, bool use_cache)
{
  if (state.validLastConfirmedPacketNo)
  {
    const uint32_t kHalfIndexPeriodPackets = (1 << indexBits) / 2u;
    if ((history.curPacketTotalNo - state.lastSentPacketTotalNo) > kHalfIndexPeriodPackets)
      resetState(state);
  }
  state.lastSentPacketTotalNo = history.curPacketTotalNo;

  const uint32_t basePacketNo = state.lastConfirmedPacketNo;
  const uint32_t nextPacketNo = history.curPacketNo;
  const uint32_t basePacketIdx = basePacketNo & (history.baseHist.size() - 1u);
  const uint32_t nextPacketIdx = nextPacketNo & (history.baseHist.size() - 1u);

  bool isValidBase = state.validLastConfirmedPacketNo;
  if (!history.isValidBasePacket(basePacketNo)) // last confirmed packet is too old, resend full diff
  {
    state.validLastConfirmedPacketNo = false;
    isValidBase = false;
  }

  danet::BitStream &uncompressedData = history.baseHist[nextPacketIdx];

  // reliable channel means we can confirm it the moment it is written to (writing to replay is an example)
  if (reliable_channel)
  {
    state.lastConfirmedPacketNo = nextPacketNo;
    state.validLastConfirmedPacketNo = true;
  }

  // check & use cache
  use_cache &= isValidBase && !history.compressedCache.empty() && history.isEnabled;
  if (use_cache)
  {
    History::CacheEntry &cache = history.compressedCache[basePacketIdx];
    if (!cache.isEmpty()) // cache valid
    {
      out_data.Write(cache.isFullDiff);
      const uint32_t packetNoDelta = cache.isFullDiff ? 0 : cache.nextPacketNo - cache.basePacketNo;
      out_data.WriteBits(reinterpret_cast<const uint8_t *>(&packetNoDelta), historyBits);
      out_data.WriteBits(reinterpret_cast<const uint8_t *>(&cache.nextPacketNo), indexBits);
      out_data.WriteCompressed(uint16_t(cache.bs.GetNumberOfBytesUsed()));
#if VALIDATE_COMPRESSION_HASH
      out_data.Write(
        uint32_t(mem_hash_fnv1a(reinterpret_cast<const char *>(uncompressedData.GetData()), uncompressedData.GetNumberOfBytesUsed())));
#endif
      write_aligned_bytes(out_data, cache.bs.GetData(), cache.bs.GetNumberOfBytesUsed());
      return;
    }
  }

  // try to compress
  bool fullDiff = !isValidBase;
  const danet::BitStream &baseVersion = history.baseHist[basePacketIdx];
  const danet::BitStream delta = !fullDiff ? net::delta::get_compressed_delta(baseVersion, uncompressedData) : danet::BitStream();
  if (delta.GetNumberOfBytesUsed() > uncompressedData.GetNumberOfBytesUsed())
    fullDiff = true; // it is cheaper to send full diff
  const danet::BitStream &dataToWrite = fullDiff ? uncompressedData : delta;

  // write to stream
  {
    out_data.Write(fullDiff);
    const uint32_t packetNoDelta = fullDiff ? 0 : nextPacketNo - basePacketNo;
    out_data.WriteBits(reinterpret_cast<const uint8_t *>(&packetNoDelta), historyBits);
    out_data.WriteBits(reinterpret_cast<const uint8_t *>(&nextPacketNo), indexBits);
    out_data.WriteCompressed(uint16_t(dataToWrite.GetNumberOfBytesUsed()));
#if VALIDATE_COMPRESSION_HASH
    out_data.Write(
      uint32_t(mem_hash_fnv1a(reinterpret_cast<const char *>(uncompressedData.GetData()), uncompressedData.GetNumberOfBytesUsed())));
#endif
    write_aligned_bytes(out_data, dataToWrite.GetData(), dataToWrite.GetNumberOfBytesUsed());
  }

  // store cache
  if (use_cache)
  {
    History::CacheEntry &cache = history.compressedCache[basePacketIdx];
    cache.isFullDiff = fullDiff;
    cache.basePacketNo = basePacketNo;
    cache.nextPacketNo = nextPacketNo;
    cache.bs.SetWriteOffset(0);
    write_aligned_bytes(cache.bs, dataToWrite.GetData(), dataToWrite.GetNumberOfBytesUsed());
  }
}

// check if idx1 goes before idx2 for N bit unsigned integers, considering wraparound
static bool is_idx_less_then(uint32_t idx1, uint32_t idx2, uint32_t bits)
{
  const uint32_t d = (idx2 - idx1) & ((1u << bits) - 1u);
  return d < 1u << (bits - 1u);
}

DeltaComp::ReadResult DeltaComp::readDelta(const danet::BitStream &data, History *history)
{
  bool ok = true;
  uint32_t packetNoDelta = 0;
  uint32_t nextPacketNo = 0;
  uint16_t compressedSize = 0;
  bool fullDiff = false;
  ok &= data.Read(fullDiff);
  ok &= data.ReadBits(reinterpret_cast<uint8_t *>(&packetNoDelta), historyBits);
  ok &= data.ReadBits(reinterpret_cast<uint8_t *>(&nextPacketNo), indexBits);
  ok &= data.ReadCompressed(compressedSize);
#if VALIDATE_COMPRESSION_HASH
  uint32_t srvHash = 0u;
  ok &= data.Read(srvHash);
#endif
  data.AlignReadToByteBoundary();
  danet::BitStream compressedDelta(data.GetData() + BITS_TO_BYTES(data.GetReadOffset()), compressedSize, false, framemem_ptr());
  data.SetReadOffset(BYTES_TO_BITS(BITS_TO_BYTES(data.GetReadOffset()) + compressedSize));
  if (history == nullptr || !ok)
    return {};

  G_ASSERT(packetNoDelta < history->baseHist.size());
  ReadResult result;
  const uint32_t basePacketNo = nextPacketNo - packetNoDelta;
  const uint32_t basePacketIdx = basePacketNo & (history->baseHist.size() - 1u);
  const uint32_t nextPacketIdx = nextPacketNo & (history->baseHist.size() - 1u);

  history->stats.lastReceivedBasePacketNo = basePacketNo;
  history->stats.lastReceivedNextPacketNo = nextPacketNo;
  const danet::BitStream &baseVersion = history->baseHist[basePacketIdx];
  if (!fullDiff && !history->isValidBasePacket(basePacketNo)) // no base packet - diff is broken
  {
    history->stats.isHistoryBroken = true;
    result.isResetNeeded = true; // int32_t(nextPacketNo) - int32_t(history->curPacketNo) >= int32_t(history->baseHist.size());
    return result;               // we don't have base packet
  }
  history->pushDbgPacketHistory(nextPacketNo);

  danet::BitStream decompressedData =
    !fullDiff ? net::delta::apply_compressed_patch(baseVersion, compressedDelta) : danet::BitStream();
  danet::BitStream &resultData = fullDiff ? compressedDelta : decompressedData;

  bool isInOrder = is_idx_less_then(history->curPacketNo, nextPacketNo, indexBits); // if nextPacketNo is greater than curPacketNo
  if (!isInOrder)
  {
    result.isOutOfOrder = true;
    // new packet is received out of order, and too old, we can't read it, request reset
    if (is_idx_less_then(nextPacketNo + (history->baseHist.size() - 1u), history->curPacketNo, indexBits))
    {
      if (fullDiff)
      {
        // In the case full packet is received, that is considered too old, proceed to fully reset local history to it.
        // If reset is requested here instead, it can end up in a broken state, where each new full packet will
        // be considered out-of-order and too old, because curPacketNo is not updated and is very different from server
        result.isOutOfOrder = false;
        isInOrder = true;
      }
      else
      {
        // partial and too old diff - something broken, request reset
        result.isResetNeeded = true;
        history->stats.isHistoryBroken = true;
        return result;
      }
    }
  }

  // advance curPacketNo, if nextPacketNo is greater than curPacketNo
  if (isInOrder)
  {
    // clear all history data as we advance to the next packet, range (curPacketNo; nextPacketNo] is cleared
    // (because baseHist is a ring buffer, these are old items, that are ejected from it)
    // don't do more than baseHist.size() iterations, because at that point we will clear all items
    uint32_t iter = 0;
    while (history->curPacketNo++ != nextPacketNo && iter++ < history->baseHist.size())
    {
      const uint32_t curPacketIdx = history->curPacketNo & (history->baseHist.size() - 1u);
      history->baseHist[curPacketIdx].SetWriteOffset(0);
    }
    history->curPacketNo = nextPacketNo;
  }

  // copy data to new base version
  {
    danet::BitStream &to = history->baseHist[nextPacketIdx];
    to.SetWriteOffset(0);
    write_aligned_bytes(to, resultData.GetData(), resultData.GetNumberOfBytesUsed());
  }

#if VALIDATE_COMPRESSION_HASH
  const uint32_t clientHash = mem_hash_fnv1a(reinterpret_cast<const char *>(resultData.GetData()), resultData.GetNumberOfBytesUsed());
  G_ASSERT(srvHash == clientHash);
#endif

  // store result
  history->stats.isHistoryBroken = false;
  if (history->curPacketNo == nextPacketNo)
    history->stats.lastCompressedDataSizeBytes = compressedDelta.GetNumberOfBytesUsed();
  result.ok = true;
  result.bs.swap(resultData);
  result.isFullDiff = fullDiff;
  result.confirmPacketNo = fullDiff ? nextPacketNo : history->curPacketNo;
  return result;
}

void DeltaComp::writeConfirm(danet::BitStream &bs, const ReadResult &result)
{
  G_ASSERT(result.ok);
  bs.WriteBits(reinterpret_cast<const uint8_t *>(&result.confirmPacketNo), indexBits);
}

bool DeltaComp::readConfirm(const danet::BitStream &bs, const History *history, State *state)
{
  bool ok = true;
  uint32_t confirmedPacketNo = 0;
  ok &= bs.ReadBits(reinterpret_cast<uint8_t *>(&confirmedPacketNo), indexBits);
  if (history == nullptr || state == nullptr || !ok)
    return ok;
  if (!history->isValidBasePacket(confirmedPacketNo)) // we can't send it anyway
    return true;
  if (is_idx_less_then(state->lastConfirmedPacketNo, confirmedPacketNo, indexBits) || !state->validLastConfirmedPacketNo)
  {
    state->lastConfirmedPacketNo = confirmedPacketNo;
    state->validLastConfirmedPacketNo = true;
    history->pushDbgPacketHistory(confirmedPacketNo);
  }
  return true;
}

void DeltaComp::resetState(State &state)
{
  state.validLastConfirmedPacketNo = false;
  state.lastConfirmedPacketNo = 0;
}

void DeltaComp::writeReplaySnapshot(danet::BitStream &bs, const History &history)
{
  bs.Write(history.isEnabled);
  if (!history.isEnabled)
    return;
  bs.WriteBits(reinterpret_cast<const uint8_t *>(&history.curPacketNo), indexBits);
  for (const danet::BitStream &base : history.baseHist)
    bs.Write(base);
}

bool DeltaComp::readReplaySnapshot(const danet::BitStream &bs, History *history)
{
  bool ok = true;
  bool isEnabled = false;
  uint32_t curPacketNo = 0;
  ok &= bs.Read(isEnabled);
  if (ok && !isEnabled)
  {
    if (history)
      initHistory(*history, false, false);
    return ok;
  }
  ok &= bs.ReadBits(reinterpret_cast<uint8_t *>(&curPacketNo), indexBits);
  History h;
  initHistory(h, isEnabled, false);
  for (danet::BitStream &base : h.baseHist)
  {
    base.SetWriteOffset(0);
    ok &= bs.Read(base);
  }
  if (!ok)
    return false;
  if (history)
  {
    history->baseHist = eastl::move(h.baseHist);
    history->curPacketNo = curPacketNo;
    history->isEnabled = isEnabled;
  }
  return ok;
}

} // namespace net
