// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "webutil.h"
#include <stdio.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>

namespace datacache
{

const char *hashstr(const uint8_t hash[SHA_DIGEST_LENGTH], char buf[SHA_DIGEST_LENGTH * 2 + 1])
{
  for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
    _snprintf(&buf[i * 2], 3, "%02x", (int)hash[i]);
  buf[SHA_DIGEST_LENGTH * 2] = '\0';
  return buf;
}

bool hashstream(IGenLoad *stream, uint8_t out_hash[SHA_DIGEST_LENGTH], IGenSave *save)
{
  G_ASSERT(stream);
  if (!save)
    return false;
  Tab<char> tmpBuf(framemem_ptr());
  tmpBuf.resize(64 << 10);
  SHA_CTX sha1ctx;
  SHA1_Init(&sha1ctx);
  do
  {
    int readed = stream->tryRead(tmpBuf.data(), tmpBuf.size());
    if (readed <= 0)
      break;
    if (save->tryWrite(tmpBuf.data(), readed) != readed)
      return false; // write failed
    SHA1_Update(&sha1ctx, tmpBuf.data(), readed);
  } while (1);
  SHA1_Final(out_hash, &sha1ctx);
  return true;
}

bool copy_stream(IGenLoad *read, IGenSave *write)
{
  G_ASSERT(read);
  if (!write)
    return false;
  Tab<char> tmpBuf(framemem_ptr());
  tmpBuf.resize(64 << 10);
  do
  {
    int readed = read->tryRead(tmpBuf.data(), tmpBuf.size());
    if (readed > 0)
    {
      if (write->tryWrite(tmpBuf.data(), readed) != readed)
        return false; // write failed
    }
    else
      break;
  } while (1);
  return true;
}

} // namespace datacache
