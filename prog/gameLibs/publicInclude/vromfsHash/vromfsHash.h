//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_vromfs.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_zstdIo.h>
#include <supp/dag_zstdObfuscate.h>
#include <hashUtils/hashUtils.h>


template <typename Hasher, typename HasherDataUnit = uint8_t>
class VromfsHashCalculator
{
  G_STATIC_ASSERT(sizeof(HasherDataUnit) == 1);

  enum EState
  {
    ES_HDR,
    ES_HDR_EXT,
    ES_HDR_EXT_REMAINDER,
    ES_BODY_PLAIN,
    ES_BODY_COMPRESSED,
    ES_BODY_SKIP,
    ES_MD5,
    ES_MD5_SKIP,
    ES_SIGNATURE,
    ES_RAW,
    ES_INVALID
  };

  using TVromfsHashCalculator = VromfsHashCalculator<Hasher, HasherDataUnit>;

  struct IGenSaveMinimal : public IGenSave
  {
    int tell(void) override { G_ASSERT_RETURN(false, 0); }
    void seekto(int) override { G_ASSERT_RETURN(false, ); }
    void seektoend(int) override { G_ASSERT_RETURN(false, ); }
    const char *getTargetName() override { return "Vromfs hash calculator"; }
    void beginBlock() override { G_ASSERT_RETURN(false, ); }
    void endBlock(unsigned) override { G_ASSERT_RETURN(false, ); }
    int getBlockLevel(void) override { G_ASSERT_RETURN(false, 0); }
    void flush(void) override { G_ASSERT_RETURN(false, ); }
  };

  struct BodyProcessor : public IGenSaveMinimal
  {
    explicit BodyProcessor(TVromfsHashCalculator &owner_) : owner(owner_) {}

    void write(const void *ptr, int size) override
    {
      owner.processBodyElement(make_span_const<HasherDataUnit>((const HasherDataUnit *)ptr, size));
    }

    void flush() override {}

    TVromfsHashCalculator &owner;
  };

  struct IGenSaveWithFinish : public IGenSaveMinimal
  {
    virtual void finish() = 0;
  };

  struct ZstdDeobfuscatingDecompressor : public ZstdDecompressSaveCB
  {
    template <typename... Args>
    explicit ZstdDeobfuscatingDecompressor(int total_size, Args &&...args) :
      ZstdDecompressSaveCB(eastl::forward<Args>(args)...), totalSize(total_size)
    {}

    void write(const void *ptr, int size) override
    {
      char buf[RD_BUFFER_SIZE];
      while (size > sizeof(buf))
      {
        memcpy(buf, ptr, sizeof(buf));
        DEOBFUSCATE_ZSTD_DATA_PARTIAL(buf, sizeof(buf), processedSize, totalSize);
        ZstdDecompressSaveCB::write(buf, sizeof(buf));
        size -= sizeof(buf);
        ptr = (const char *)ptr + sizeof(buf);
        processedSize += sizeof(buf);
      }
      if (size)
      {
        memcpy(buf, ptr, size);
        DEOBFUSCATE_ZSTD_DATA_PARTIAL(buf, size, processedSize, totalSize);
        ZstdDecompressSaveCB::write(buf, size);
        processedSize += size;
      }
    }

    const int totalSize;
    int processedSize = 0;
  };

  template <typename T>
  struct DecompressorWrapper : public IGenSaveWithFinish
  {
    template <typename... Args>
    explicit DecompressorWrapper(Args &&...args) : wrapped(eastl::forward<Args>(args)...)
    {}

    void write(const void *ptr, int size) override { wrapped.write(ptr, size); }

    void finish() override { wrapped.finish(); }

    T wrapped;
  };

public:
  using Hash = GenericHash<Hasher>;
  using Value = typename Hasher::Value;

  template <typename... Args>
  explicit VromfsHashCalculator(Args &&...args) : hasher(eastl::forward<Args>(args)...), bodyProcessor(*this)
  {}

  void setIgnoreVersion(bool ignore_version)
  {
    G_ASSERT(state == ES_HDR);
    ignoreVersion = ignore_version;
  }

  void setForceRawHash(bool force_raw_hash)
  {
    G_ASSERT(state == ES_HDR);
    forceRawHash = force_raw_hash;
  }

  void update(dag::ConstSpan<char> data)
  {
    while (data.size() > 0)
    {
      if (!updateSingleStep(data))
        return;
    }
  }

  Value finalizeRaw()
  {
    if (state == ES_INVALID)
      return {};
    return hasher.finalize();
  }

  Hash finalize() { return Hash(finalizeRaw()); }

private:
  bool updateSingleStep(dag::ConstSpan<char> &data)
  {
    switch (state)
    {
      case ES_HDR:
        data = consumeHeader(data, hdrData, sizeof(hdrData));
        G_ASSERT(processedInCurState <= sizeof(hdrData));
        if (processedInCurState == sizeof(hdrData))
          processHdr();
        return true;
      case ES_HDR_EXT:
        data = consumeHeader(data, hdrExtData, sizeof(hdrExtData));
        G_ASSERT(processedInCurState <= sizeof(hdrExtData));
        if (processedInCurState == sizeof(hdrExtData))
          processHdrExt();
        return true;
      case ES_HDR_EXT_REMAINDER: data = consumeHdrExtRemainder(data); return true;
      case ES_BODY_PLAIN:
      case ES_BODY_COMPRESSED:
      case ES_BODY_SKIP: data = consumeBody(data); return true;
      case ES_MD5:
      case ES_MD5_SKIP: data = consumeMd5(data); return true;
      case ES_SIGNATURE: data = consumeSignature(data); return true;
      case ES_RAW: data = consumeRaw(data); return true;
      case ES_INVALID:
      default: return false;
    }
  }

  void setState(EState newState)
  {
    processedInCurState = 0;
    state = newState;
  }

  dag::ConstSpan<char> consumeHeader(dag::ConstSpan<char> data, char *dst, int dstSize)
  {
    if (processedInCurState < dstSize)
    {
      const int dataSize = min((int)data.size(), dstSize - processedInCurState);
      memcpy(&dst[processedInCurState], data.data(), dataSize);
      processedInCurState += dataSize;
      data = make_span_const(data.data() + dataSize, data.size() - dataSize);
    }
    return data;
  }

  void processHdr()
  {
    hdr = reinterpret_cast<VirtualRomFsDataHdr *>(&hdrData[0]);
    if ((hdr->label != _MAKE4C('VRFs') && hdr->label != _MAKE4C('VRFx')))
    {
      hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdrData[0]), sizeof(hdrData));
      setState(ES_RAW);
      return;
    }

    if (!hdr->signedContents())
      setForceRawHash(true);

    if (hdr->label == _MAKE4C('VRFs'))
    {
      if (forceRawHash)
      {
        hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdrData[0]), sizeof(hdrData));
        setState(ES_RAW);
        return;
      }
      unsigned backupHw32 = hdr->hw32;
      if (hdr->packedSz())
        hdr->hw32 = 0x80000000; // compatibility
      hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdrData[0]), sizeof(hdrData));
      hdr->hw32 = backupHw32;
      beginBody();
      return;
    }

    setState(ES_HDR_EXT);
  }

  void processHdrExt()
  {
    hdrExt = reinterpret_cast<VirtualRomFsExtHdr *>(&hdrExtData[0]);
    if (forceRawHash)
    {
      hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdrData[0]), sizeof(hdrData));

      uint32_t versionBackup = hdrExt->version;
      if (ignoreVersion)
        hdrExt->version = 0;
      hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdrExtData[0]), sizeof(hdrExtData));
      hdrExt->version = versionBackup;
      setState(ES_RAW);
      return;
    }

    if (hdrExt->flags & EVFSEF_SIGN_PLAIN_DATA)
    {
      hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdr->label), sizeof(hdr->label));
      hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdr->target), sizeof(hdr->target));
    }
    else
    {
      unsigned backupHw32 = hdr->hw32;
      if (hdr->packedSz())
        hdr->hw32 = 0x80000000;
      hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdrData[0]), sizeof(hdrData));
      hdr->hw32 = backupHw32;
    }

    uint32_t versionBackup = hdrExt->version;
    if (ignoreVersion)
      hdrExt->version = 0;
    hasher.update(reinterpret_cast<const HasherDataUnit *>(&hdrExtData[0]), sizeof(hdrExtData));
    hdrExt->version = versionBackup;

    if (hdrExt->size > sizeof(hdrExtData))
      setState(ES_HDR_EXT_REMAINDER);
    else
      beginBody();
  }

  dag::ConstSpan<char> consumeHdrExtRemainder(dag::ConstSpan<char> data)
  {
    G_ASSERT(hdrExt);
    if (!hdrExt)
    {
      setState(ES_INVALID);
      return data;
    }

    int fullRemainder = hdrExt->size - sizeof(hdrExtData);
    if (!fullRemainder)
    {
      beginBody();
      return data;
    }

    int remainderOfRemainder = fullRemainder - processedInCurState;
    int bytesToConsume = min((int)data.size(), remainderOfRemainder);
    if (bytesToConsume > 0)
    {
      processedInCurState += bytesToConsume;
      if (!(hdrExt->flags & EVFSEF_SIGN_PLAIN_DATA))
        hasher.update(reinterpret_cast<const HasherDataUnit *>(data.data()), bytesToConsume);
      return make_span_const(data.data() + bytesToConsume, data.size() - bytesToConsume);
    }

    beginBody();
    return data;
  }

  void beginBody()
  {
    G_ASSERT(hdr);
    if (!hdr)
    {
      setState(ES_INVALID);
      return;
    }
    int packedSz = hdr->packedSz();
    rawBodySize = packedSz ? packedSz : hdr->fullSz;
    plainBodySize = hdr->fullSz;
    if (hdrExt && (hdrExt->flags & EVFSEF_SIGN_PLAIN_DATA))
    {
      setState(ES_BODY_SKIP);
      return;
    }

    if (packedSz)
    {
      if (hdr->zstdPacked())
        beginZstdPackedBody();
      else
        beginZlibPackedBody();
      setState(ES_BODY_COMPRESSED);
    }
    else
    {
      setState(ES_BODY_PLAIN);
    }
  }

  void beginZlibPackedBody()
  {
    bodyDecompressor = eastl::make_unique<DecompressorWrapper<ZlibDecompressSaveCB>>(bodyProcessor, true, false);
  }

  void beginZstdPackedBody()
  {
    bodyDecompressor = eastl::make_unique<DecompressorWrapper<ZstdDeobfuscatingDecompressor>>(rawBodySize, bodyProcessor);
  }

  dag::ConstSpan<char> consumeBody(dag::ConstSpan<char> data)
  {
    int expectedBodyLeft = rawBodySize - processedInCurState;
    if (!expectedBodyLeft)
    {
      switch (state)
      {
        case ES_BODY_PLAIN: setState(ES_MD5); break;
        case ES_BODY_COMPRESSED:
          G_ASSERT(bodyDecompressor);
          bodyDecompressor->finish();
          setState(ES_MD5);
          break;
        case ES_BODY_SKIP: setState(ES_MD5_SKIP); break;
        default: G_VERIFY(false); break;
      }
      return data;
    }

    int bytesToConsume = min((int)data.size(), expectedBodyLeft);
    if (!bytesToConsume)
      return data;

    switch (state)
    {
      case ES_BODY_PLAIN: bodyProcessor.write(data.data(), bytesToConsume); break;
      case ES_BODY_COMPRESSED:
        G_ASSERT(bodyDecompressor);
        bodyDecompressor->write(data.data(), bytesToConsume);
        break;
      case ES_BODY_SKIP: break;
      default: G_VERIFY(false); break;
    }

    processedInCurState += bytesToConsume;
    return make_span_const(data.data() + bytesToConsume, data.size() - bytesToConsume);
  }

  void processBodyElement(dag::ConstSpan<HasherDataUnit> data) { hasher.update(data.data(), data.size()); }

  dag::ConstSpan<char> consumeMd5(dag::ConstSpan<char> data)
  {
    constexpr int MD5_HASH_SIZE = 16;
    int expectedMd5Left = MD5_HASH_SIZE - processedInCurState;
    if (!expectedMd5Left)
    {
      setState(ES_SIGNATURE);
      return data;
    }

    int bytesToConsume = min((int)data.size(), expectedMd5Left);
    if (!bytesToConsume)
      return data;

    if (state == ES_MD5)
    {
      hasher.update(reinterpret_cast<const HasherDataUnit *>(data.data()), bytesToConsume);
    }
    else
    {
      G_ASSERT(state == ES_MD5_SKIP);
    }

    processedInCurState += bytesToConsume;
    return make_span_const(data.data() + bytesToConsume, data.size() - bytesToConsume);
  }

  dag::ConstSpan<char> consumeSignature(dag::ConstSpan<char> data) { return consumeRaw(data); }

  dag::ConstSpan<char> consumeRaw(dag::ConstSpan<char> data)
  {
    hasher.update(reinterpret_cast<const HasherDataUnit *>(data.data()), data.size());
    return make_span_const<char>(nullptr, 0);
  }

private:
  Hasher hasher;
  BodyProcessor bodyProcessor;
  char hdrData[sizeof(VirtualRomFsDataHdr)];
  char hdrExtData[sizeof(VirtualRomFsExtHdr)];
  eastl::unique_ptr<IGenSaveWithFinish> bodyDecompressor;
  VirtualRomFsDataHdr *hdr = nullptr;
  VirtualRomFsExtHdr *hdrExt = nullptr;
  int processedInCurState = 0;
  int rawBodySize = 0;
  int plainBodySize = 0;
  EState state = ES_HDR;
  bool ignoreVersion = false;
  bool forceRawHash = false;
};
