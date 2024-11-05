//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <supp/dag_define_KRNLIMP.h>

#include <string.h> // for memcpy

// FIFO circular buffer
template <uint32_t N>
class DaFixedRingBuffer
{
public:
  int write(const void *data, uint32_t len, bool force_overwrite);
  int read(void *data, uint32_t len);
  int tail(void *dst, uint32_t max_bytes);

  uint32_t size() const { return dataCnt; }
  uint32_t capacity() const { return N; }

private:
  void discard(uint32_t bytes);
  void advance(uint32_t bytes);

  int32_t writeIndex = 0;
  int32_t readIndex = 0;
  uint32_t dataCnt = 0;

  uint8_t buffer[N];
};

template <uint32_t N>
void DaFixedRingBuffer<N>::discard(uint32_t bytes)
{
  dataCnt -= bytes;
  readIndex = (readIndex + bytes) % N;
}

template <uint32_t N>
void DaFixedRingBuffer<N>::advance(uint32_t bytes)
{
  dataCnt += bytes;
  writeIndex = (writeIndex + bytes) % N;
}

template <uint32_t N>
int DaFixedRingBuffer<N>::write(const void *data, uint32_t len, bool force_overwrite)
{
  const uint32_t freeSpace = N - dataCnt;
  const uint32_t availForWrite = force_overwrite ? N : freeSpace;
  if (len > availForWrite)
    len = availForWrite;
  if (len == 0)
    return 0;
  if (force_overwrite)
  {
    if (len > freeSpace)
      discard(len - freeSpace);
  }
  const char *inBytes = (const char *)data;
  const uint32_t bufSize1 = N - writeIndex;
  memcpy(&buffer[writeIndex], &inBytes[0], min(len, bufSize1));
  if (len > bufSize1)
    memcpy(&buffer[0], &inBytes[bufSize1], len - bufSize1);
  advance(len);
  return len;
}

template <uint32_t N>
int DaFixedRingBuffer<N>::read(void *data, uint32_t len)
{
  if (dataCnt == 0)
    return 0;
  if (len > dataCnt)
    len = dataCnt;
  const uint32_t bufSize1 = N - readIndex;
  char *outBytes = (char *)data;
  memcpy(&outBytes[0], &buffer[readIndex], min<int>(len, bufSize1));
  if (len > bufSize1)
    memcpy(&outBytes[bufSize1], &buffer[0], len - bufSize1);
  discard(len);
  return len;
}


template <uint32_t N>
int DaFixedRingBuffer<N>::tail(void *dst, uint32_t max_bytes)
{
  max_bytes = min<uint32_t>(max_bytes, dataCnt);

  const uint32_t firstChunkSize = max<int32_t>(max_bytes - writeIndex, 0);
  const uint32_t firstChunkStart = N - firstChunkSize;
  memcpy(dst, buffer + firstChunkStart, firstChunkSize);

  const uint32_t lastChunkSize = min<uint32_t>(max_bytes, writeIndex);
  const uint32_t lastChunkStart = writeIndex - lastChunkSize;
  memcpy((char *)dst + firstChunkSize, buffer + lastChunkStart, lastChunkSize);

  return max_bytes;
}

#include <supp/dag_undef_KRNLIMP.h>
