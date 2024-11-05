// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>
#include <debug/dag_assert.h>
#include <daNet/daNetPeerInterface.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <util/dag_hash.h>
#include <generic/dag_carray.h>
#include <osApiWrappers/dag_critSec.h>
#include <daECS/net/netbase.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <enet/win32.h> // _EnetBuffer
#else
#include <enet/unix.h> // _EnetBuffer
#endif
#include <crypto/cipher.h>

namespace net
{

class EncryptionCtx final : public IDaNetTrafficEncoder
{
public:
  static constexpr size_t KEY_LEN = 16;

private:
  static constexpr size_t AUTH_TAG_LEN = 4;
  WinCritSec mutex;
  eastl::unique_ptr<crypto::ISymmetricCipher> gcmCipherCtx, cfbCipherCtx;
  uint64_t sessionRand;
  struct PeerRecord
  {
    net::EncryptionKeyBits bits = EncryptionKeyBits::None;
    uint32_t rand = 0;
    carray<uint8_t, KEY_LEN> key;
  };
  uint32_t nPeers = 0;
  PeerRecord peerRecords[1]; // varlen

  EncryptionCtx(uint32_t np, uint64_t session_rand) :
    nPeers(np),
    sessionRand(session_rand),
    gcmCipherCtx(crypto::create_cipher_aes(crypto::Mode::GCM)),
    cfbCipherCtx(crypto::create_cipher_aes(crypto::Mode::CFB))
  {}

public:
  static EncryptionCtx *create(int max_peers, uint64_t fixed_iv)
  {
    G_ASSERT(max_peers >= 1);
    size_t memSize = sizeof(EncryptionCtx) + sizeof(PeerRecord) * (max_peers - 1);
    void *mem = memalloc(memSize, midmem);
    memset(mem, 0, memSize);
    return new (mem, _NEW_INPLACE) EncryptionCtx(max_peers, fixed_iv);
  }

  void setPeerRandom(SystemIndex peer_id, uint32_t prand)
  {
    WinAutoLock lock(mutex);
    peerRecords[peer_id].rand = prand;
  }

  void setPeerKey(SystemIndex peer_id, dag::ConstSpan<uint8_t> key, net::EncryptionKeyBits ebits)
  {
    WinAutoLock lock(mutex);
    peerRecords[peer_id].bits = ebits;
    if (ebits == net::EncryptionKeyBits::None)
      mem_set_0(peerRecords[peer_id].key);
    else if (key.size() >= KEY_LEN)
      return mem_copy_from(peerRecords[peer_id].key, key.data());
    else
    {
      mem_set_0(peerRecords[peer_id].key);
      memcpy(peerRecords[peer_id].key.data(), key.data(), data_size(key));
    }
  }

private:
  template <crypto::EncryptionDir edir>
  crypto::ISymmetricCipher *setupCipher(SystemIndex peer_id, size_t &packet_size)
  {
    WinAutoLock lock(mutex);
    auto testBit = (edir == crypto::ENCRYPT) ? net::EncryptionKeyBits::Encryption : net::EncryptionKeyBits::Decryption;
    if ((peerRecords[peer_id].bits & testBit) == net::EncryptionKeyBits::None)
      return nullptr;

    crypto::ISymmetricCipher *cipher;
    if (nPeers == 1)
      cipher = (edir == crypto::ENCRYPT) ? gcmCipherCtx.get() : cfbCipherCtx.get();
    else
    {
      if (edir == crypto::ENCRYPT)
        cipher = cfbCipherCtx.get();
      else
      {
        cipher = gcmCipherCtx.get();
        if (DAGOR_UNLIKELY(packet_size <= AUTH_TAG_LEN))
          return nullptr;
        packet_size -= AUTH_TAG_LEN;
      }
    }

    union
    {
      uint8_t asBytes[16];
      uint32_t asInts[4];
    } iv;
    iv.asInts[0] = hash_int((uint32_t)packet_size);
    iv.asInts[1] = peerRecords[peer_id].rand;
    iv.asInts[2] = hash_int((uint32_t)sessionRand);
    iv.asInts[3] = hash_int((uint32_t)(sessionRand >> 32)); // Note: only 12 bytes used by default for GCM's IV (can be extended
                                                            // though)

    cipher->setup(peerRecords[peer_id].key, make_span_const(iv.asBytes), edir);

    return cipher;
  }

  size_t encode(SystemIndex peer_id, const _ENetBuffer *buffers, size_t bufCount, size_t inLimit, uint8_t *outData,
    size_t outLimit) override
  {
    auto cipherCtx = setupCipher<crypto::ENCRYPT>(peer_id, inLimit);
    if (!cipherCtx)
      return 0;

    if (cipherCtx == gcmCipherCtx.get())
    {
      outData += AUTH_TAG_LEN;
      outLimit -= AUTH_TAG_LEN;
    }

    int outlen;
    unsigned char *ptr = outData;
    for (size_t i = 0; i < bufCount; ++i)
    {
      int len = (int)buffers[i].dataLength;
      G_FAST_ASSERT(ptr + len <= (outData + outLimit));
      cipherCtx->update(ptr, &outlen, (const unsigned char *)buffers[i].data, len);
      G_FAST_ASSERT(len == outlen);
      ptr += outlen;
    }
    cipherCtx->finalize(ptr, &outlen);
    ptr += outlen;

    int total = ptr - outData;
    G_ASSERTF(total == (int)inLimit, "%d != %d", total, (int)inLimit); // required for correct IV generation

    if (cipherCtx == gcmCipherCtx.get())
    {
      cipherCtx->getAuthTag(dag::Span<uint8_t>(outData - AUTH_TAG_LEN, AUTH_TAG_LEN)); // write final auth tag in first AUTH_TAG_LEN
                                                                                       // bytes
      total += AUTH_TAG_LEN;
    }

    return total;
  }

  size_t decode(SystemIndex peer_id, const uint8_t *inData, size_t inLimit, uint8_t *outData, size_t /*outLimit*/) override
  {
    if (peer_id >= nPeers) // peer_id == ENET_PROTOCOL_MAXIMUM_PEER_ID in enet connect message. But we don't send this message
                           // encrypted.
      return 0;

    auto cipherCtx = setupCipher<crypto::DECRYPT>(peer_id, inLimit);
    if (!cipherCtx)
      return 0;

    bool gcmCipher = cipherCtx == gcmCipherCtx.get();
    if (gcmCipher)
    {
      cipherCtx->setAuthTag(dag::ConstSpan<uint8_t>(inData, AUTH_TAG_LEN));
      inData += AUTH_TAG_LEN;
    }

    int outlen;
    cipherCtx->update(outData, &outlen, inData, inLimit);
    G_FAST_ASSERT(outlen == inLimit);

    if (gcmCipher)
    {
      if (DAGOR_UNLIKELY(!cipherCtx->finalize(outData + outlen, &outlen))) // check auth tag
        return 0;
    }
    else
      ; // finalize is no-op for CFB cipher

    return inLimit;
  }
};

} // namespace net
