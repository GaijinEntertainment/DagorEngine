#pragma once

#include <util/dag_globDef.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_genIo.h>

#if !defined(USE_ZLIB_VER)
#include <zlib.h>
#elif USE_ZLIB_VER == 0xFF0000
#include <arc/zlib-ng/zlib.h>
#endif

class ZLibPacker
{
protected:
  z_stream strm;

public:
  ZLibPacker(int level, IMemAlloc *mem, bool raw_inflate)
  {
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = mem;
    int err = raw_inflate
                ? deflateInit2(&strm, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL >= 8 ? 8 : MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY)
                : deflateInit(&strm, level);
    if (err != Z_OK)
      DAG_FATAL("zlib error %d in %s", err, "deflateInit");
  }
  ~ZLibPacker()
  {
    int err = deflateEnd(&strm);
    if (err != Z_OK)
      DAG_FATAL("zlib error %d in %s", err, "deflateEnd");
  }
  // returns Z_OK or Z_STREAM_END
  int pack(void *&inp, unsigned &insz, void *&outp, unsigned &outsz, int flush)
  {
    strm.next_in = (Bytef *)inp;
    strm.avail_in = insz;
    strm.next_out = (Bytef *)outp;
    strm.avail_out = outsz;
    int err = deflate(&strm, flush);
    inp = strm.next_in;
    insz = strm.avail_in;
    outp = strm.next_out;
    outsz = strm.avail_out;
    if (err != Z_OK && err != Z_STREAM_END)
      DAG_FATAL("zlib error %d in %s", err, "deflate");
    return err;
  }
};

class ZLibGeneralWriter : public ZLibPacker
{
protected:
  Tab<char> buf;

public:
  IGenSave *callback;
  unsigned uncompressedTotal;
  unsigned compressedTotal;


  ZLibGeneralWriter(IGenSave &cb, unsigned bufsz, int level, IMemAlloc *mem, bool raw_inflate) :
    ZLibPacker(level, mem, raw_inflate), callback(&cb), buf(mem)
  {
    buf.resize(bufsz);
    uncompressedTotal = 0;
    compressedTotal = 0;
  }

  void pack(void *inp, unsigned sz, bool finish)
  {
    uncompressedTotal += sz;

    while (sz || finish)
    {
      void *op = &buf[0];
      unsigned osz = data_size(buf);
      int err = ZLibPacker::pack(inp, sz, op, osz, finish ? Z_FINISH : Z_NO_FLUSH);
      unsigned l = data_size(buf) - osz;
      if (l != 0)
      {
        callback->write(&buf[0], l);
        compressedTotal += l;
      }

      while (err == Z_OK && osz == 0)
      {
        op = &buf[0];
        osz = data_size(buf);
        err = ZLibPacker::pack(inp, sz, op, osz, finish ? Z_FINISH : Z_NO_FLUSH);
        l = data_size(buf) - osz;
        if (l != 0)
        {
          callback->write(&buf[0], l);
          compressedTotal += l;
        }
      }

      if (err == Z_STREAM_END)
        break;
      if (err != Z_OK)
        DAG_FATAL("pack error %d", err);
    }
  }

  float getCompressionRatio()
  {
    if (uncompressedTotal)
      return (float)compressedTotal / uncompressedTotal;
    else
      return 0;
  }
};
