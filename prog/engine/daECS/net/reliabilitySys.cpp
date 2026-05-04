// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/reliabilitySys.h>
#include <debug/dag_assert.h>

namespace net
{

#define SEQ_HISTORY_DEPTH 1024

void ReliabilitySystem::packetSent(int cur_time, int timeout_ms)
{
#if DAGOR_DBGLEVEL > 0
  bool alreadyExist = eastl::find_if(pendingAcks.begin(), pendingAcks.end(),
                        [this](const PendingAckRec &qr) { return qr.sequence == localSequence; }) != pendingAcks.end();
  G_ASSERT(!alreadyExist);
#endif
  pendingAcks.insert(PendingAckRec{localSequence, 0, cur_time + timeout_ms});
  incLocalSequence();
}

void ReliabilitySystem::packetReceived(sequence_t seq)
{
  receivedQueue.insert(seq);
#if DAGOR_DBGLEVEL > 0
  for (int i = 1; i < receivedQueue.size(); ++i)
    G_ASSERT(is_seq_gt(receivedQueue[i], receivedQueue[i - 1]));
#endif
}

void ReliabilitySystem::processAck(sequence_t ack, ack_bits_t ack_bits, packet_event_cb_t packet_acked_cb,
  packet_event_cb_t packet_lost_cb, void *cb_param, int nlost_threshold)
{
  G_FAST_ASSERT(packet_acked_cb != NULL && packet_lost_cb != NULL);
  sequence_t prevAck = ack - 1;
  for (auto it = pendingAcks.begin(); it != pendingAcks.end();)
  {
    packet_event_cb_t cb = NULL;
    if (!is_seq_gt(it->sequence, ack)) // seq <= ack
    {
      if (it->sequence == ack)
        cb = packet_acked_cb;
      else
      {
        int bitIdx = seq_diff(prevAck, it->sequence);
        if (bitIdx < sizeof(ack_bits) * CHAR_BIT)
        {
          cb = (ack_bits & (1 << bitIdx)) ? packet_acked_cb : NULL;
          if (!cb && ++it->nMissing >= nlost_threshold)
            cb = packet_lost_cb;
        }
        // else To consider: mark all packets > this number of packets (32+1) as lost
      }
    }
    if (cb)
    {
      cb(it->sequence, cb_param);
      it = pendingAcks.erase(it);
    }
    else
      ++it;
  }
}

ReliabilitySystem::AckData ReliabilitySystem::genLastSeqAckData() const
{
  G_ASSERT(!receivedQueue.empty()); // this method is expected to be called after 'packetReceived'

  ack_bits_t ackBits = 0;
  sequence_t remoteSequence = receivedQueue.back(), prevRemoteSeq = remoteSequence - 1;
  for (int i = int(receivedQueue.size()) - 2; i >= 0; --i)
  {
    int bitIdx = seq_diff(prevRemoteSeq, receivedQueue[i]);
    if (bitIdx < sizeof(ackBits) * CHAR_BIT)
      ackBits |= 1 << bitIdx;
    else
      break;
  }

  return AckData{remoteSequence, ackBits};
}

void ReliabilitySystem::update(int cur_time, packet_event_cb_t packet_lost_cb, void *cb_param)
{
  // check timeout for pendings acks
  for (auto it = pendingAcks.begin(); it != pendingAcks.end();)
  {
    if (cur_time >= it->timeoutAt)
    {
      packet_lost_cb(it->sequence, cb_param);
      it = pendingAcks.erase(it);
    }
    else
      break;
  }

  // keep only sizeof(ack_bits_t)*8+1 last received entries
  if (!receivedQueue.empty())
  {
    sequence_t numSeqToKeep = SEQ_HISTORY_DEPTH + 1;
    sequence_t minSeq = receivedQueue.back() - numSeqToKeep;
    auto it = receivedQueue.begin();
    while (it != receivedQueue.end() && !is_seq_gt(*it, minSeq))
      it = receivedQueue.erase(it);
  }
}

}; // namespace net
