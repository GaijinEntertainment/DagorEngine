#include "gzip.h"
#include <ioSys/dag_zlibIo.h>

// http://forensicswiki.org/wiki/Gzip
struct GZipHeader
{
  enum
  {
    MAGIC_0 = 0x1f,
    MAGIC_1 = 0x8b
  };
  uint8_t magic[2];
  uint8_t method;
  uint8_t flags;
  uint32_t time;
  uint8_t xflags;
  uint8_t os;
};

IGenLoad *gzip_stream_create(IGenLoad *stream)
{
  if (!stream)
    return NULL;
  GZipHeader hdr;
  if (stream->tryRead(&hdr, 10) != 10)
    return NULL;
  if (hdr.magic[0] != GZipHeader::MAGIC_0 || hdr.magic[1] != GZipHeader::MAGIC_1)
    return NULL;
  if (hdr.method != 8) // unknown compression method
    return NULL;
  if (hdr.flags) // no any flags supported (FNAME is probably most used one)
    return NULL;
  return new ZlibLoadCB(*stream, 0x7FFFFFFF, true, false);
}
