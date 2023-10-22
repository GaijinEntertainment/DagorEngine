// this file is for dump processing (i.e. reading)
#pragma once
#include "daProfilerTypes.h"
#include "daProfilerDescriptions.h"
#include "dumpLayer.h"

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_memIo.h>

namespace da_profiler
{

inline uint32_t read_vlq_uint(IGenLoad &c)
{
  uint8_t b = c.readIntP<1>();

  // the first byte has 7 bits of the raw value
  uint32_t raw = (uint32_t)(b & 0x7F);

  // first continue flag is the second bit from the top, shift it into the sign
  bool cont = (b & 0x80) != 0;

  for (int shift = 7; shift < 32 && cont; shift += 7)
  {
    b = c.readIntP<1>();
    cont = (b & 0x80) != 0;
    b &= 0x7F;
    raw |= ((uint32_t)b & 0x7F) << shift;
  }
  return (uint32_t)raw;
}

inline int read_vlq_int(IGenLoad &c)
{
  uint8_t b = c.readIntP<1>();

  // the first byte has 6 bits of the raw value
  uint32_t raw = (uint32_t)(b & 0x3F);
  const bool neg = (b & 0x40);

  // first continue flag is the second bit from the top, shift it into the sign
  bool cont = (b & 0x80) != 0;

  for (int shift = 6; shift < 32 && cont; shift += 7)
  {
    b = c.readIntP<1>();
    cont = (b & 0x80) != 0;
    b &= 0x7F;
    raw |= ((uint32_t)b & 0x7F) << shift;
  }
  return neg ? -(int)raw : (int)raw;
}

inline void read_short_string(IGenLoad &stream, char *val, const uint32_t max_length)
{
  uint32_t length = read_vlq_uint(stream);
  G_ASSERT(length + 1 < max_length);
  if (length)
    stream.read(val, length);
  val[length] = 0;
}

inline void read_modules(IGenLoad &symbolsStream, SymbolsCache &symbols)
{
  for (;;)
  {
    uint64_t base = symbolsStream.readInt64();
    if (base == 0)
      return;
    uint64_t size = symbolsStream.readInt64();
    char name[512];
    read_short_string(symbolsStream, name, sizeof(name));
    symbols.modules.emplace_back(SymbolsCache::ModuleInfo{base, size, symbols.strings.addName(name)});
  }
}

inline void read_symbols(IGenLoad &symbolsStream, SymbolsCache &cache)
{
  for (;;)
  {
    uint64_t addr = symbolsStream.readInt64();
    if (addr == 0)
      return;
    auto em = cache.symbolMap.emplace_if_missing(addr);
    if (em.second)
      *em.first = ~0u;
    int line = symbolsStream.readInt();
    char fun[512], file[512];
    if (line != -1)
    {
      read_short_string(symbolsStream, fun, sizeof(fun));
      read_short_string(symbolsStream, file, sizeof(file));
      uint32_t &ind = *cache.symbolMap.findVal(addr);
      if (ind == ~0u)
        ind = cache.symbols.size();
      if (cache.symbols.size() <= ind)
        cache.symbols.resize(ind + 1);
      cache.symbols[ind] = SymbolsCache::SymbolInfo{cache.strings.addNameId(fun), cache.strings.addNameId(file), (uint32_t)line};
    }
  }
}

inline bool copy_cb(IGenLoad &cb, IGenSave &save_cb, size_t size)
{
  for (size_t sz = size; sz > 0;)
  {
    char buf[256];
    const size_t readSz = sz < sizeof(buf) ? sz : sizeof(buf);
    if (cb.tryRead(buf, readSz) != readSz)
    {
      printf("can't read %d bytes from source stream of size = %d\n", readSz, size);
      return false;
    }
    save_cb.write(buf, readSz);
    sz -= readSz;
  }
  return true;
}

enum class ResponseProcessorResult
{
  Stop,
  Copy,
  Skip
};

template <typename ResponseProcessor>
inline void copy_to_file_dump(IGenLoad &load_cb_no_header, IGenSave &save_cb, ResponseProcessor &proc)
{
  ZlibSaveCB zcb(save_cb, 9);
  DumpHeader header;
  header.flags |= DumpHeader::Zlib; // actually zlib
  save_cb.write((const char *)&header, sizeof(header));
  if (read_responses(load_cb_no_header, [&](IGenLoad &cb, const DataResponse &resp) {
        ResponseProcessorResult ret = proc(cb, zcb, resp);
        if (ret == ResponseProcessorResult::Stop)
          return ReadResponseCbStatus::Stop;
        if (ret == ResponseProcessorResult::Copy)
        {
          zcb.write(&resp, sizeof(resp));
          if (!copy_cb(cb, zcb, resp.size))
            return ReadResponseCbStatus::Stop;
        }
        return ReadResponseCbStatus::Continue;
      }) != ReadResponseStatus::Ok)
    printf("can't process dump\n");
  proc(load_cb_no_header, zcb, DataResponse(DataResponse::NullFrame, 0)); // finishing
  zcb.finish();
}

template <typename ResponseProcessor>
inline bool file_to_file_dump(IGenLoad &load_cb, IGenSave &save_cb, ResponseProcessor &pr)
{
  using namespace da_profiler;
  DumpHeader header;
  if (load_cb.tryRead(&header, sizeof(header)) != sizeof(header) || header.magic != DumpHeader::MAGIC)
    return false;
  if (header.flags & DumpHeader::Zlib)
  {
    ZlibLoadCB zlcb(load_cb, INT_MAX);
    copy_to_file_dump(zlcb, save_cb, pr);
  }
  else if (header.flags & DumpHeader::Reserved0)
  {
    printf("unsupported zip stream. Re-save with a new daProfiler.exe\n");
    return false;
  }
  else
    copy_to_file_dump(load_cb, save_cb, pr);
  return true;
}

}; // namespace da_profiler