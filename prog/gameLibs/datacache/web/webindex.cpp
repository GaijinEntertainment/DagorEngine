// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "webindex.h"
#include "../common/trace.h"
#include "../common/util.h"
#include "webutil.h"
#include <util/dag_string.h>
#include <ioSys/dag_genIo.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef _MSC_VER
#define STRTOK_R strtok_s
#define STRTOULL _strtoui64
#else
#define STRTOK_R strtok_r
#define STRTOULL strtoull
#endif

namespace datacache
{

static void parse_index_line(EntriesMap &entries, char *line)
{
  // expected format:
  // <path> <size> <last_modified> <hex_sha1>
  DOTRACE3("parse line '%s'", line);
  WebEntry newEntry;
  bool error = false;
  int tokenIdx = 0;
  char *savedptr = NULL;
  for (char *token = STRTOK_R(line, " ", &savedptr); token && !error; token = STRTOK_R(NULL, " ", &savedptr), tokenIdx++)
    switch (tokenIdx)
    {
      case 0: // name
        char tmp[DAGOR_MAX_PATH];
        newEntry.path = get_real_key(token, tmp);
        error = *token == '\0';
        break;
      case 1: // size
      case 2: // last_modified
        errno = 0;
        (tokenIdx == 1 ? newEntry.size : newEntry.lastModified) = (int64_t)STRTOULL(token, NULL, 0);
        error = errno != 0;
        break;
      case 3: // sha1
        error = strlen(token) != SHA_DIGEST_LENGTH * 2;
        for (int i = 0; i < SHA_DIGEST_LENGTH && !error; ++i)
        {
          unsigned byte = 0;
          error = sscanf(&token[i * 2], "%02x", &byte) != 1;
          newEntry.hash[i] = (uint8_t)byte;
        }
        break;
    }
  error |= tokenIdx < 4;
  if (error)
    DOTRACE1("skip error line '%s'", line); // yes, line is destroyed by strtok, but at least we will see path (first token)
  else
  {
    char buf[SHA_DIGEST_LENGTH * 2 + 1];
    DOTRACE2("add new entry to index: %s %d %d %s", newEntry.path.str(), (int)newEntry.size, (int)newEntry.lastModified,
      hashstr(newEntry.hash, buf));
    entries[get_entry_hash_key(newEntry.path)] = newEntry;
  }
}

void parse_index(EntriesMap &entries, IGenLoad *stream)
{
  String line;
  char tmp[1024];
  do
  {
    int readed = stream ? stream->tryRead(tmp, (int)sizeof(tmp)) : 0;
    if (readed <= 0)
      break;
    const char *end = tmp + readed;
    const char *c = tmp;
    do
    {
      const char *begin = c;
      for (; c < end; ++c) // scan for EOL
        if (*c == '\n' || *c == '\r')
          break;
      line.append(begin, c - begin);
      if (c != end) // EOL found
      {
        if (line.length())
          parse_index_line(entries, line.str());
        line.clear();
        if (*c == '\r')
          c += 2;
        else
          c++;
      }
    } while (c < end);
  } while (1);
  if (!line.empty())
    parse_index_line(entries, line.str());
}

} // namespace datacache