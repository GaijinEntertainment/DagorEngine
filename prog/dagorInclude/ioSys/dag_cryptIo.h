//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_staticTab.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_fileIo.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

// IO stream encryption adapter.
// Using AES-128 symmetric cipher
// Key generated from one int value with pseudo-random generator
// Not intented to use for high-security encryption
// But can hide sensitive information

template <class IBase>
class CryptedIOStreamAdaptor : public IBase
{
public:
  CryptedIOStreamAdaptor(unsigned key_seed, bool encrypt)
  {
    union
    {
      uint8_t asBytes[16];
      unsigned asInts[4];
    } key; // aes128
    for (int i = 0; i < 4; ++i)
    {
      key.asInts[i] = key_seed;
      key_seed = ((key_seed * 0x41C64E6D + 0x3039) >> 16) & 0x7FFF;
    }

    evpCtx = EVP_CIPHER_CTX_new();
    EVP_CipherInit_ex(evpCtx, EVP_aes_128_cbc(), NULL, key.asBytes, /*iv*/ NULL, encrypt ? 1 : 0);
  }

  virtual ~CryptedIOStreamAdaptor() { EVP_CIPHER_CTX_free(evpCtx); }

  virtual void write(const void *ptr0, int size)
  {
    const uint8_t *ptr = (const uint8_t *)ptr0;
    uint8_t crypted[AES_BLOCK_SIZE * 2 - 1];
    while (size > 0)
    {
      int blockSize = (size > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : size;
      size -= blockSize;
      int written = 0;
      EVP_CipherUpdate(evpCtx, crypted, &written, ptr, blockSize);
      ptr += blockSize;
      if (written > 0)
        ioWrite(crypted, written);
    }
  }

  virtual void read(void *ptr, int size) { (void)tryRead(ptr, size); }

  virtual int tryRead(void *ptr0, int size)
  {
    if (eof)
      return 0;

    uint8_t *ptr = (uint8_t *)ptr0;

    int totalread = 0;
    int n = readFromDecodeBuf(ptr, size);
    if (n == size)
      return n;
    if (n > 0)
    {
      totalread = n;
      ptr += n;
      size -= n;
    }

    while (size > 0)
    {
      uint8_t fileBuf[AES_BLOCK_SIZE];
      int readCnt = ioTryRead(fileBuf, sizeof(fileBuf));

      int written = 0;
      if (readCnt < AES_BLOCK_SIZE)
      {
        decodeBuf.resize(decodeBuf.capacity());
        if (readCnt)
          EVP_CipherUpdate(evpCtx, decodeBuf.data(), &written, fileBuf, readCnt);
        int written2 = 0;
        EVP_CipherFinal_ex(evpCtx, decodeBuf.data() + written, &written2);
        decodeBuf.resize(written + written2);
        eof = true;
        return readFromDecodeBuf(ptr, size) + totalread;
      }

      decodeBuf.resize(decodeBuf.capacity());
      EVP_CipherUpdate(evpCtx, decodeBuf.data(), &written, fileBuf, readCnt);
      if (!written)
      {
        decodeBuf.clear();
        continue;
      }
      decodeBuf.resize(written);
      n = readFromDecodeBuf(ptr, size);
      if (n == 0)
        return totalread;
      totalread += n;
      ptr += n;
      size -= n;
    }
    return totalread;
  }

protected:
  virtual int ioTryRead(void *ptr, int size) = 0;
  virtual void ioWrite(const void *ptr, int size) = 0;

  void writeFinalize()
  {
    uint8_t encBuffer[AES_BLOCK_SIZE];
    int written = 0;
    EVP_CipherFinal_ex(evpCtx, encBuffer, &written);
    if (written > 0)
      ioWrite(encBuffer, written);
  }

private:
  int readFromDecodeBuf(uint8_t *ptr, int size)
  {
    if (data_size(decodeBuf) == 0)
      return 0;
    if (size <= data_size(decodeBuf) && size <= decodeBuf.capacity()) // need to silent GCC's stringop overflow error
    {
      memcpy(ptr, decodeBuf.data(), size);
      erase_items(decodeBuf, 0, size);
      return size;
    }
    mem_copy_to(decodeBuf, ptr);
    int written = data_size(decodeBuf);
    decodeBuf.clear();
    return written;
  }

private:
  EVP_CIPHER_CTX *evpCtx;
  StaticTab<uint8_t, 3 * AES_BLOCK_SIZE> decodeBuf;
  bool eof = false;
};

template <class FileStream, class IBase>
class CryptedFileStreamAdaptor : public CryptedIOStreamAdaptor<IBase>
{
public:
  CryptedFileStreamAdaptor(const char *fn, unsigned key_seed, bool encrypt) :
    fstream(fn), CryptedIOStreamAdaptor<IBase>(key_seed, encrypt)
  {}

  int tell() { return fstream.tell(); }
  void seekto(int ofs) { return fstream.seekto(ofs); }
  void seektoend(int ofs) { return fstream.seektoend(ofs); }
  void seekrel(int ofs) { fstream.seekrel(ofs); }
  const char *getTargetName() { return fstream.getTargetName(); }
  int64_t getTargetDataSize() { return fstream.getTargetDataSize(); }
  void flush() { fstream.flush(); }

protected:
  FileStream fstream;
};

template <class SaveCB = FullFileSaveCB>
class CryptedFileSaveIO : public CryptedFileStreamAdaptor<SaveCB, IBaseSave>
{
public:
  using CryptedFileStreamAdaptor<SaveCB, IBaseSave>::writeFinalize;
  using CryptedFileStreamAdaptor<SaveCB, IBaseSave>::fstream;

  CryptedFileSaveIO(const char *fn, unsigned key_seed) : CryptedFileStreamAdaptor<SaveCB, IBaseSave>(fn, key_seed, true) {}

  virtual ~CryptedFileSaveIO() { writeFinalize(); }

private:
  virtual int ioTryRead(void * /*ptr*/, int /*size*/) { return 0; }
  virtual void ioWrite(const void *ptr, int size) { fstream.write(ptr, size); }
};

template <class LoadCB = FullFileLoadCB>
class CryptedFileLoadIO : public CryptedFileStreamAdaptor<LoadCB, IBaseLoad>
{
public:
  using CryptedFileStreamAdaptor<LoadCB, IBaseLoad>::fstream;

  CryptedFileLoadIO(const char *fn, unsigned key_seed) : CryptedFileStreamAdaptor<LoadCB, IBaseLoad>(fn, key_seed, false) {}

private:
  virtual int ioTryRead(void *ptr, int size) { return fstream.tryRead(ptr, size); }
  virtual void ioWrite(const void * /*ptr*/, int /*size*/) {}
};
