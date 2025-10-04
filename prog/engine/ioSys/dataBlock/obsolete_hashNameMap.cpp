// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_PC_WIN
#undef _DEBUG_TAB_
#endif

#include "obsolete_hashNameMap.h"
#include <ioSys/dag_genIo.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/le2be.h>
#include <math/dag_mathBase.h> // GCC_HOT
#include <math/dag_adjpow2.h>

using obsolete::HashNameMap;
void HashNameMap::save(IGenSave &cb, bool save_le) const
{
  int count = 0;
  for (int j = 0; j < buckets.size(); ++j)
    count += buckets[j].size();
  G_ASSERT(count <= MAX_STRING_COUNT);
  writeInt32(cb, count, save_le);

  int start = cb.tell();

  writeInt16(cb, buckets.size(), save_le);
  for (int j = 0; j < buckets.size(); ++j)
  {
    const Tab<char *> &names = buckets[j];
    writeInt16(cb, names.size(), save_le);

    for (int i = 0; i < names.size(); ++i)
    {
      const char *str = names[i];
      int len32 = str ? (int)strlen(str) : 0;
      G_ASSERT(len32 >= 0 && len32 < MAX_STRING_SIZE);
      writeInt16(cb, (uint16_t)len32, save_le);
    }
  }

  for (int j = 0; j < buckets.size(); ++j)
  {
    const Tab<char *> &names = buckets[j];

    for (int i = 0; i < names.size(); ++i)
    {
      const char *str = names[i];
      int len32 = str ? (int)strlen(str) : 0;
      G_ASSERT(len32 >= 0 && len32 < MAX_STRING_SIZE);
      if (len32)
        cb.write(str, len32);
    }
  }

  cb.alignOnDword(cb.tell() - start);
}

void HashNameMap::saveCompact(class IGenSave &cwr, bool save_le) const
{
  int nm_cnt = 0;
  for (int i = 0; i < buckets.size(); ++i)
    nm_cnt += buckets[i].size();

  unsigned pk = get_count_storage_type(nm_cnt), total_sz = 2 + get_storage_type_sz(pk);

  writeInt16(cwr, buckets.size() | (pk << 14), save_le);
  write_count_compact(cwr, save_le, pk, nm_cnt);
  for (int i = 0; i < buckets.size(); ++i)
    for (int j = 0; j < buckets[i].size(); ++j)
    {
      char buf[4];
      int len = (int)strlen(buckets[i][j]);
      int sz_len = encode_len(len, buf);
      cwr.write(buf, sz_len);
      cwr.write(buckets[i][j], len);
      total_sz += sz_len + len;
    }
  cwr.alignOnDword(total_sz);
}
