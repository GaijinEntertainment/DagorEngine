// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/_platform.h>
#include <util/dag_localization.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_rwSpinLock.h>
#include <osApiWrappers/dag_rwLock.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_log.h>
#include <generic/dag_initOnDemand.h>
#include <generic/dag_tab.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_hashedKeyMap.h>
#include <util/dag_stringTableAllocator.h>
#include <util/dag_hash.h>
#include <hash/wyhash.h>
#include <EASTL/vector_set.h>
#include <ctype.h>
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_C3

#elif _TARGET_XBOX
#include "lang_xbox.h"
#endif
#include <startup/dag_globalSettings.h>

#define MAX_KEY_LEN                 256
#define DEFAULT_PLURAL_FORM_INDEX   -1
#define _DEBUG_INCONVENIENT_SYMBOLS 0

extern const char *os_get_default_lang();

// Defined below. Caller must hold locRwLock (read or write). The
// LocalizationTable::loadCsv* overloads call these from inside the
// startup/load write critical section; the public get_default_lang*()
// would re-acquire locRwLock for read and deadlock the non-reentrant
// NoWritersSpinLockReadWriteLock.
static const char *get_default_lang_locked();
static const char *get_default_lang_locked(const class DataBlock &blk);

bool (*dag_on_duplicate_loc_key)(const char *key) = NULL;
#if _TARGET_PC
static char *force_def_lang = NULL;
#endif

// Compile-time wyhash seed for localization keys. Changing it forces a full
// asset+binary rebuild; do so only if a real collision is observed in dev.
// Only dev builds (DAGOR_DBGLEVEL > 0) run collision detection; release and
// ForceLogs telemetry builds trust wyhash. Probability of a wyhash collision
// against a 1M-entry table is ~3e-8 per insert; failure mode is wrong
// translation, not crash.
static constexpr uint64_t LOC_HASH_SEED = 0xa31d4f2b5e7c9018ull;

static inline uint64_t loc_hash(const char *s, size_t len)
{
  uint64_t h = wyhash(s, len, LOC_HASH_SEED);
  return h ? h : (1ULL << 63UL); // 0 is reserved as HashedKeyMap's EmptyKey
}

// Shared shape: hash -> StringTableAllocator offset.
// Used three times: locTable->main values, RTA additions, optional LocKeyDump.
struct LocStringTable
{
  HashedKeyMap<uint64_t, uint32_t> hashToOffset;
  // 4KB start / 64KB max page: loc tables are MB-scale, default 256B/8KB pages would mean thousands of page pointers.
  StringTableAllocator strings{12, 16};

  const char *find(uint64_t hash) const
  {
    if (const uint32_t *ofs = hashToOffset.findVal(hash))
      return strings.getDataRawUnsafe(*ofs);
    return nullptr;
  }

  // Reserves `len` bytes (caller writes value + trailing NUL into the returned
  // pointer) and records the (hash -> offset) mapping. If `hash` was already
  // present, the entry is overwritten -- old bytes become unreachable until
  // clear() reclaims the whole allocator.
  char *reserveBytes(uint64_t hash, size_t len, uint32_t *out_offset = nullptr)
  {
    if (strings.head.left < len)
      strings.addPage((uint32_t)len);
    const uint32_t ofs = strings.getCurrentHeadOffset();
    char *buf = strings.head.data.get() + strings.head.used;
    strings.head.used += (uint32_t)len;
    strings.head.left -= (uint32_t)len;
    auto p = hashToOffset.emplace_if_missing(hash);
    *p.first = ofs;
    if (out_offset)
      *out_offset = ofs;
    return buf;
  }

  // Returns pointer to the stored bytes for `hash`. On a hash hit, returns
  // the existing entry unchanged and, if its bytes differ from (str, len),
  // sets *out_collided (when non-null) so the caller can surface a wyhash
  // collision -- the new bytes are discarded. On a miss, copies (str, len)
  // plus a trailing NUL and returns the freshly stored pointer.
  const char *tryAddString(uint64_t hash, const char *str, size_t len, bool *out_collided = nullptr)
  {
    if (const uint32_t *ofs = hashToOffset.findVal(hash))
    {
      const char *existing = strings.getDataRawUnsafe(*ofs);
      if (out_collided)
        *out_collided = strncmp(existing, str, len) != 0 || existing[len] != 0;
      return existing;
    }
    char *buf = reserveBytes(hash, len + 1);
    memcpy(buf, str, len);
    buf[len] = 0;
    return buf;
  }

  void clear()
  {
    hashToOffset.clear();
    strings.clear();
  }
};

// LocKeyDump is just a LocStringTable holding (key_hash -> key string).
struct LocKeyDump : LocStringTable
{};

LocKeyDump *make_loc_key_dump() { return new LocKeyDump(); }
void destroy_loc_key_dump(LocKeyDump *d) { delete d; }

// Interned placeholder strings handed out by get_fake_loc_for_missing_key for
// keys absent from the main table. Cold path; rare wyhash collision would
// surface as a wrong placeholder string (same failure mode as the main table).
static LocStringTable fakeLocKeys;

// RTA (runtime localization additions): hot read path (every getTextId may
// consult it after a main-table miss), bursty writes during UGC ingest.
static LocStringTable rtaTable;
// rtaLock guards rtaTable only. Reader-preferring spinlock matches the
// access pattern; 4 bytes, no OS handle.
// rtaTable may legitimately contain entries whose hash also exists in
// locTable->main -- getTextId checks main first, so such shadow entries are
// harmless until clear_rta_localization() reclaims them.
static NoWritersSpinLockReadWriteLock rtaLock;
using RtaReadLock = ScopedLockReadTemplate<NoWritersSpinLockReadWriteLock>;
using RtaWriteLock = ScopedLockWriteTemplate<NoWritersSpinLockReadWriteLock>;
static SimpleString defLangOverride;

static const char *remap_english_lang(const char *lang, const class DataBlock *blk)
{
  static SimpleString english_us("English"), english_gb("English"), english_other("English");
  if (blk)
  {
    english_us = blk->getStr("english_us", "English");
    english_gb = blk->getStr("english_gb", "English");
    english_other = blk->getStr("english_other", "English");
  }

#if _TARGET_PC_WIN
  if (strcmp(lang, "English") == 0)
  {
    if (SUBLANGID(GetUserDefaultUILanguage()) == SUBLANG_ENGLISH_US)
      lang = english_us;
    else if (SUBLANGID(GetUserDefaultUILanguage()) == SUBLANG_ENGLISH_UK)
      lang = english_gb;
    else
      lang = english_other;
  }
#endif

  return lang;
}

class ICsvParserCB
{
public:
  virtual void allocateColumns(int num) = 0;
  virtual int getCount(bool full = false) = 0;
  virtual char *addKey(int n, int len) = 0;
  virtual int getKey(const char *key, bool full = false) = 0;
  virtual void onKeyProcessed(int n, char *data, bool full = false) = 0;
  virtual char *addValue(int n, int len, int lang_idx = -1) = 0;
  virtual void onValueProcessed(int n, char *data) = 0;
  virtual void reserve(int rows) = 0;
};

typedef OAHashNameMap<false> LocHashNameMapType;
typedef int (*PluralFormFn)(int);

class LocalizationTable
{
public:
  // Main runtime table: hash -> value string. Keys are not stored; tooling that
  // needs them passes a LocKeyDump through startup_localization*.
  LocStringTable main;
  PluralFormFn pluralFormFn = nullptr;

  // Editor multi-language path. Less hot than `main` but follows the same
  // hash-only model: mapFull holds key_hash -> row_idx, and the (rows x langs)
  // grid of translation offsets lives in tableFull. fullLangList must keep
  // stored language names because getLangById and loadCsvFull walk names back
  // from indices for diagnostics.
  Tab<uint32_t> tableFull; // row_idx * fullLangList.nameCount() + lang_idx -> offset
  StringTableAllocator stringsFull{12, 16};
  HashedKeyMap<uint64_t, uint32_t> mapFull;
  LocHashNameMapType fullLangList;

  Tab<PluralFormFn> langToPluralIdxFunctions;

#if DAGOR_DBGLEVEL > 0
  // Dev-build wyhash collision dump. tryAddString detects collisions only
  // against keys already in the dump; a per-loadCsv local dump would scope
  // detection to a single file (silently missing cross-file collisions
  // that the persistent main map otherwise treats as well-formed duplicates
  // via the LocTableParserCB::getKey fallback). Lives on the table so it
  // accumulates across every loadCsv call; cleared by demandDestroy in
  // shutdown_localization.
  LocKeyDump devCollisionDump;
#endif

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
  static SimpleString lastFileNameForDebug;
#else
  constexpr static const char *lastFileNameForDebug = "n/a";
#endif

  // Caller holds locRwLock for read; rtaLock is taken internally only if the
  // main map misses and we need to consult the runtime additions.
  // LocTextId is an opaque pointer that, in this implementation, is exactly
  // the pointer to the localized text bytes -- get_localized_text() casts back.
  LocTextId getTextId(uint64_t key_hash)
  {
    if (const char *text = main.find(key_hash))
      return (LocTextId)(void *)text;
    RtaReadLock rtaReadLock(rtaLock);
    return (LocTextId)(void *)rtaTable.find(key_hash);
  }

  LocTextId getTextId(const char *key) { return getTextId(loc_hash(key, strlen(key))); }

  bool hasMainKey(uint64_t key_hash) const { return main.hashToOffset.findVal(key_hash) != nullptr; }

  const char *getFullText(const char *key, int lang_idx)
  {
    const uint32_t *idx_p = mapFull.findVal(loc_hash(key, strlen(key)));
    if (!idx_p)
      return NULL;
    const int nLangs = fullLangList.nameCount();
    const int slot = (int)*idx_p * nLangs + lang_idx;
    G_ASSERT(slot < tableFull.size());
    return stringsFull.getDataRawUnsafe(tableFull[slot]);
  }

  static const char *getText(LocTextId id) { return id ? (const char *)(void *)id : ""; }

  static __forceinline int parseCsvString(const char *&p, char *out, bool key)
  {
    int len = 0;

    bool quoted = false;
    const char *p0 = p;
    G_UNUSED(p0);

    if (*p == '"')
    {
      quoted = true;
      ++p;
    }

    while (*p)
    {
      if (quoted)
      {
        if (*p == '"')
        {
          ++p;
          if (*p != '"')
          {
            quoted = false;
            break;
          }
        }
      }
      else
      {
        if (*p == ';' || *p == '\r' || *p == '\n')
          break;
      }

      if (*p == '\\')
      {
        if (p[1] == 'r')
        {
          p += 2;
          len++;
          if (out)
            *out++ = '\r';
        }
        else if (p[1] == 'n')
        {
          p += 2;
          len++;
          if (out)
            *out++ = '\n';
        }
        else if (p[1] == 't')
        {
          p += 2;
          len++;
          if (out)
            *out++ = '\t';
        }
        else
        {
          if (out)
            *out++ = *p;
          ++p;
          ++len;
        }
      }
      else
      {
        if (out)
          *out++ = *p;
        ++p;
        ++len;
      }
    }

#if _DEBUG_INCONVENIENT_SYMBOLS
    if (!key)
    {
      const char *dbg_string = "'\"^`";
      int n = (int)strlen(dbg_string);
      if (out)
      {
        strncpy(out, dbg_string, n);
        out += n;
      }
      len += n;
    }
#else
    (void)key;
#endif

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
    if (quoted)
      logwarn("missing terminating quote in CSV item (%s):\n%.*s", lastFileNameForDebug, int(p - p0), p0);
#endif
    if (out)
      *out = 0;
    if (*p == ';' || *p == '\r' || *p == '\n' || *p == '\0')
      return len;

    while (strchr(" \t\v", *p))
      p++;
    if (!memchr(";\r\n\0", *p, 4))
      logerr("parse error at <%.*s>, missing ';' or <CR> after field", p - p0 + 16, p0);
    if (*p == ';')
      p++;
    return len;
  }

  static bool parseCsvV2(char *buffer, int len, int lang_col, ICsvParserCB *cb, bool skip_first = true)
  {
    int numRows = 0;

    // Ignore first 3 bytes of UTF8 format
    if (len >= 3 && (unsigned char)buffer[0] == 0xEF && (unsigned char)buffer[1] == 0xBB && (unsigned char)buffer[2] == 0xBF)
    {
      buffer += 3;
      len -= 3;
    }

    const char quoteSymbol = '\"';

    char *tmp_buf = buffer;

    for (int i = 0; i < len; ++i)
    {
      if (buffer[i] == quoteSymbol && i + 1 < len && buffer[i + 1] == quoteSymbol)
        i++;
      else if (buffer[i] == quoteSymbol && i + 1 < len && buffer[i + 1] != quoteSymbol)
        for (i++; i + 1 < len; i++)
          if (buffer[i] == quoteSymbol)
          {
            i++;
            if (buffer[i] == quoteSymbol)
              continue; // ignore double quote symbol
            else
              break;
          }

      if (buffer[i] == '\r' || buffer[i] == '\n')
      {
        if (buffer[i] == '\r' && i + 1 < len && buffer[i + 1] == '\n')
          ++i;
        ++numRows;
        if (numRows == 1)
          tmp_buf = &buffer[i + 1];
      }
    }

    // If last CVS row have no \n at the end it is still valid CSV row
    if (len && buffer[len - 1] != '\n')
      ++numRows;

    // V2: skip first row
    if (skip_first)
    {
      len -= (tmp_buf - buffer);
      buffer = tmp_buf;
      if (len <= 0)
        return false;
      numRows--;
    }

    const char *ptr = buffer;
    for (int i = 0; i < numRows; ++i)
    {
      static char key_buffer[MAX_KEY_LEN];
      const char *p = ptr;

      if (p >= buffer + len)
        break;

      int l = parseCsvString(p, NULL, true);
      if (l >= MAX_KEY_LEN)
      {
        if (l > 4096)
          DAG_FATAL("key longer than 4096 bytes in CSV");
        else
        {
          char tmp[4096];
          l = parseCsvString(ptr, tmp, true);
          tmp[4095] = 0;
          DAG_FATAL("Too long key in CSV, len = %d, key = %s", l, tmp);
        }
      }
      l = parseCsvString(ptr, key_buffer, true);

      bool skip_key = false;
      if (!key_buffer[0])
        skip_key = true;
      int idx = cb->getKey(key_buffer);

      if (idx >= 0 && !skip_key)
        if (dag_on_duplicate_loc_key && !dag_on_duplicate_loc_key(key_buffer))
          skip_key = true;

      if (idx < 0 && !skip_key)
      {
        char *key = cb->addKey(idx, l + 1);
        strcpy(key, key_buffer);
        cb->onKeyProcessed(idx, key); // key is invalid after call
        idx = cb->getKey(key_buffer);
      }

      if (ptr >= buffer + len)
      {
        logerr("unexpected end in row %d (%s), %s", i + 1, key_buffer, lastFileNameForDebug);
        return false;
      }
      for (int langColNo = 0; langColNo < lang_col; langColNo++)
      {
        if (*ptr == ';')
          ++ptr;
        l = parseCsvString(ptr, NULL, false);
        if (ptr >= buffer + len)
        {
          logerr("unexpected end in row %d (%s), lang column %d, %s", i + 1, key_buffer, langColNo + 1, lastFileNameForDebug);
          return false;
        }
      }

      if (*ptr == ';')
        ++ptr;

      if (ptr >= buffer + len)
      {
        logerr("unexpected end in row %d (%s), %s", i + 1, key_buffer, lastFileNameForDebug);
        return false;
      }
      if (skip_key)
        l = parseCsvString(ptr, NULL, false);
      else
      {
        p = ptr;
        l = parseCsvString(p, NULL, false);
        char *text = cb->addValue(idx, l + 1);
        parseCsvString(ptr, text, false);
        cb->onValueProcessed(idx, text);
      }

      bool insideQuote = false;
      for (;;)
      {
#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
        if (ptr >= buffer + len && insideQuote)
        {
          logerr("Non-paired quotation marks in row %d (%s), %s", i + 1, key_buffer, lastFileNameForDebug);
          return false;
        }
#endif
        if (ptr >= buffer + len)
          return true; // we can't read more from this file
        if (*ptr == '\"')
        {
          ptr++;
          if (*ptr != '\"')
            insideQuote = !insideQuote;
        }

        if ((*ptr == '\r' || *ptr == '\n') && !insideQuote)
          break;

        ptr++;
      }

      if (*ptr == '\r')
      {
        ++ptr;
        if (ptr < buffer + len && *ptr == '\n')
          ++ptr;
      }
      else if (*ptr == '\n')
        ++ptr;
    }

    return true;
  }

  static bool parseCsvV2Full(char *buffer, int len, dag::ConstSpan<int> lang_col, ICsvParserCB *cb, bool skip_first = true)
  {
    int numRows = 0;
    int max_col = -1;
    for (int i = 0; i < lang_col.size(); i++)
      if (max_col < lang_col[i])
        max_col = lang_col[i];

    if (max_col < 0)
    {
      logerr("parseCsvV2Full found no required translations");
      return false;
    }
    Tab<int> bmap_col;
    bmap_col.resize(max_col + 1);
    mem_set_ff(bmap_col);
    for (int i = 0; i < lang_col.size(); i++)
      if (lang_col[i] >= 0)
        bmap_col[lang_col[i]] = i;

    // Ignore first 3 bytes of UTF8 format
    if (len >= 3 && (unsigned char)buffer[0] == 0xEF && (unsigned char)buffer[1] == 0xBB && (unsigned char)buffer[2] == 0xBF)
    {
      buffer += 3;
      len -= 3;
    }

    const char quoteSymbol = '\"';

    char *tmp_buf = buffer;

    for (int i = 0; i < len; ++i)
    {
      if (buffer[i] == quoteSymbol && i + 1 < len && buffer[i + 1] == quoteSymbol)
        i++;
      else if (buffer[i] == quoteSymbol && i + 1 < len && buffer[i + 1] != quoteSymbol)
        while ((++i) + 1 < len)
          if (buffer[i] == quoteSymbol)
          {
            if (buffer[i + 1] == quoteSymbol)
            {
              i++;
              continue; // ignore double quote symbol
            }
            else
              break;
          }

      if (buffer[i] == '\r' || buffer[i] == '\n')
      {
        if (buffer[i] == '\r' && i + 1 < len && buffer[i + 1] == '\n')
          ++i;
        ++numRows;
        if (numRows == 1)
          tmp_buf = &buffer[i + 1];
      }
    }
    // V2: skip first row
    if (skip_first)
    {
      len -= (tmp_buf - buffer);
      buffer = tmp_buf;
      if (len <= 0)
        return false;
      numRows--;
    }

    const char *ptr = buffer;
    cb->reserve(numRows);
    for (int i = 0; i < numRows; ++i)
    {
      static char key_buffer[MAX_KEY_LEN];
      const char *p = ptr;

      if (p >= buffer + len)
        break;

      int l = parseCsvString(p, NULL, true);
      if (l >= MAX_KEY_LEN)
      {
        if (l > 4096)
          DAG_FATAL("key longer than 4096 bytes in CSV");
        else
        {
          char tmp[4096];
          l = parseCsvString(ptr, tmp, true);
          tmp[4095] = 0;
          DAG_FATAL("Too long key in CSV, len = %d, key = %s", l, tmp);
        }
      }
      l = parseCsvString(ptr, key_buffer, true);

      bool skip_key = false;
      if (!key_buffer[0])
        skip_key = true;
      int idx = cb->getKey(key_buffer, true);

      if (idx >= 0 && !skip_key)
        if (dag_on_duplicate_loc_key && !dag_on_duplicate_loc_key(key_buffer))
          skip_key = true;

      if (idx < 0 && !skip_key)
      {
        char *key = cb->addKey(idx, l + 1);
        strcpy(key, key_buffer);
        cb->onKeyProcessed(idx, key, true); // key is invalid after call
        idx = cb->getKey(key_buffer, true);
      }

      for (int langColNo = 0; langColNo <= max_col; langColNo++)
      {
        if (*ptr == ';')
          ++ptr;
        if (bmap_col[langColNo] < 0 || skip_key)
          l = parseCsvString(ptr, NULL, false);
        else
        {
          p = ptr;
          l = parseCsvString(p, NULL, false);
          char *text = cb->addValue(idx, l + 1, bmap_col[langColNo]);
          parseCsvString(ptr, text, false);
          cb->onValueProcessed(idx, text);
        }
      }

      bool insideQuote = false;
      for (;;)
      {
#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
        G_ASSERTF(ptr < buffer + len, "Non-paired quotation marks in csv file %s", lastFileNameForDebug.str());
#endif
        if (ptr >= buffer + len)
          return true; // we can't read more from this file
        if (*ptr == '\"')
        {
          ptr++;
          if (*ptr != '\"')
            insideQuote = !insideQuote;
        }

        if ((*ptr == '\r' || *ptr == '\n') && !insideQuote)
          break;

        ptr++;
      }

      if (*ptr == '\r')
      {
        ++ptr;
        if (ptr < buffer + len && *ptr == '\n')
          ++ptr;
      }
      else if (*ptr == '\n')
        ++ptr;
    }

    return true;
  }

  bool loadCsv(MemGeneralLoadCB *cb, int lang_col, LocKeyDump *key_dump = nullptr)
  {
    return loadCsv((char *)cb->data(), cb->size(), lang_col, key_dump);
  }

  bool loadCsv(MemGeneralLoadCB *cb, const char *lang, LocKeyDump *key_dump = nullptr)
  {
    char *buffer = (char *)cb->data();
    int len = cb->size();

    int lang_col = getColForLang(buffer, len, lang);
    if (lang_col < 0)
      lang_col = getColForLang(buffer, len, get_default_lang_locked());
    if (lang_col < 0)
      lang_col = 0;
    return loadCsvV2(buffer, len, lang_col, key_dump);
  }

  bool loadCsv(const char *filename, int lang_col, LocKeyDump *key_dump = nullptr)
  {
    FullFileLoadCB cb(filename);
    if (!cb.fileHandle)
      return false;

    int len = df_length(cb.fileHandle);
    if (len == -1)
      return false;

    char *buffer = (char *)memalloc(len + 2, tmpmem);
    if (!buffer)
      return false;

    if (cb.tryRead(buffer, len) != len)
    {
      memfree(buffer, tmpmem);
      return false;
    }
    buffer[len] = '\n';
    len++;
    buffer[len] = '\0';

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
    lastFileNameForDebug = filename;
#endif
    bool result = loadCsv(buffer, len, lang_col, key_dump);
    memfree(buffer, tmpmem);
    return result;
  }
  bool loadCsv(const char *filename, const char *lang, LocKeyDump *key_dump = nullptr)
  {
    FullFileLoadCB cb(filename);
    if (!cb.fileHandle)
      return false;

    int len = df_length(cb.fileHandle);
    if (len == -1)
      return false;

    char *buffer = (char *)memalloc(len + 2, tmpmem);
    if (!buffer)
      return false;

    if (cb.tryRead(buffer, len) != len)
    {
      memfree(buffer, tmpmem);
      return false;
    }
    buffer[len] = '\n';
    len++;
    buffer[len] = '\0';

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
    lastFileNameForDebug = filename;
#endif

    int lang_col = getColForLang(buffer, len, lang);
    if (lang_col < 0)
      lang_col = getColForLang(buffer, len, get_default_lang_locked());
    if (lang_col < 0)
      lang_col = 0;
    bool result = loadCsvV2(buffer, len, lang_col, key_dump);
    memfree(buffer, tmpmem);
    return result;
  }

  bool loadCsvFull(const char *filename, LocKeyDump *key_dump = nullptr)
  {
    FullFileLoadCB cb(filename);
    if (!cb.fileHandle)
      return false;

    int len = df_length(cb.fileHandle);
    if (len == -1)
      return false;

    char *buffer = (char *)memalloc(len + 2, tmpmem);
    if (!buffer)
      return false;

    if (cb.tryRead(buffer, len) != len)
    {
      memfree(buffer, tmpmem);
      return false;
    }
    buffer[len] = '\n';
    len++;
    buffer[len] = '\0';

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
    lastFileNameForDebug = filename;
#endif

    Tab<int> lang_col;
    lang_col.resize(fullLangList.nameCount());
    for (int i = 0; i < lang_col.size(); i++)
    {
      lang_col[i] = getColForLang(buffer, len, fullLangList.getName(i));
      G_ASSERT_LOG(lang_col[i] >= 0, "Language '%s' not found.", fullLangList.getName(i));
    }
    bool result = loadCsvV2Full(buffer, len, lang_col, key_dump);
    memfree(buffer, tmpmem);
    return result;
  }

  bool loadCsv(char *buffer, int len, int lang_col, LocKeyDump *key_dump = nullptr);
  bool loadCsvV2(char *buffer, int len, int lang_col, LocKeyDump *key_dump = nullptr);
  bool loadCsvV2Full(char *buffer, int len, dag::ConstSpan<int> lang_col, LocKeyDump *key_dump = nullptr);
  int getColForLang(char *buffer, int len, const char *lang);
};

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
SimpleString LocalizationTable::lastFileNameForDebug;
#endif

int LocalizationTable::getColForLang(char *buffer, int len, const char *lang)
{
  if (!len || !buffer)
    return -1;

  if ((unsigned char)buffer[0] == 0xEF && (unsigned char)buffer[1] == 0xBB && (unsigned char)buffer[2] == 0xBF)
  {
    buffer += 3;
    len -= 3;
  }

  const char quoteSymbol = '\"';

  char *tmp_buf = buffer;

  for (int i = 0; i < len; ++i)
  {
    if (buffer[i] == quoteSymbol && i + 1 < len && buffer[i + 1] != quoteSymbol)
      while ((++i) + 1 < len)
        if (buffer[i] == quoteSymbol)
        {
          if (buffer[i + 1] == quoteSymbol)
          {
            i++;
            continue; // ignore double quote symbol
          }
          else
            break;
        }

    if (buffer[i] == '\r' || buffer[i] == '\n')
    {
      tmp_buf = &buffer[i];
      break;
    }
  }
  len = (tmp_buf - buffer);
  if (len <= 0)
    return -1;

  const char *ptr = buffer;
  static char key_buffer[MAX_KEY_LEN];

  const char *p = ptr;
  int l = parseCsvString(p, NULL, true);
  if (l >= MAX_KEY_LEN)
    DAG_FATAL("CSV header is too long, make sure the CSV separator is semicolon");
  (void)l;
  (void)p;
  l = parseCsvString(ptr, key_buffer, true);

  int result = 0;
  while (*ptr && ptr < buffer + len)
  {
    if (*ptr == ';')
      ++ptr;
    p = ptr;
    l = parseCsvString(p, NULL, true);
    if (l >= MAX_KEY_LEN)
      DAG_FATAL("CSV header is too long, make sure the CSV separator is semicolon");
    l = parseCsvString(ptr, key_buffer, true);

    if ((p = strchr(key_buffer, '<')) != NULL)
      if (strncmp(p + 1, lang, strlen(lang)) == 0)
        return result;
    if (strncmp(key_buffer, lang, strlen(lang)) == 0)
      return result;
    result++;
  }
  return -1;
}

// CSV parse state. The legacy ICsvParserCB threads `int idx` through the
// parser as a stable row token: getKey returns it, addKey/onKeyProcessed/
// addValue use it. With hash-keyed storage there is no natural per-row index,
// so each path keeps its own hash -> idx map:
//   main path: idxToHash + hashToIdx (parser-local; main's value reservation
//              needs to look the hash back up from the row index).
//   full path: locTbl->mapFull alone (full's addValue indexes tableFull
//              directly from the row index, no hash lookback needed).
class LocTableParserCB : public ICsvParserCB
{
public:
  LocTableParserCB(LocalizationTable *loc_tbl, LocKeyDump *key_dump) : locTbl(loc_tbl), keyDump(key_dump) {}

  void allocateColumns(int /*num*/) override {}
  int getCount(bool full) override { return full ? (int)locTbl->mapFull.size() : (int)idxToHash.size(); }

  char *addKey(int /*n*/, int len) override
  {
    G_ASSERT(len);
    return (char *)memalloc(len + 1, tmpmem);
  }

  int getKey(const char *key, bool full) override
  {
    G_ASSERT(key);
    const size_t key_len = strlen(key);
    const uint64_t h = loc_hash(key, key_len);
    const uint32_t *idx = full ? locTbl->mapFull.findVal(h) : hashToIdx.findVal(h);
    if (idx)
    {
      // Hash hit. parseCsvV2 short-circuits before reaching onKeyProcessed
      // when getKey returns a valid idx, so the LocKeyDump validation must
      // run here too -- otherwise a wyhash collision against an earlier
      // intra-file key would be silently treated as a duplicate (new value
      // overwriting the first). tryAddString on an existing hash with
      // matching bytes is a no-op; on a real collision it sets *collided.
      if (keyDump && checkCollisionAndLog(h, key, key_len))
        return -1;
      return (int)*idx;
    }
    // Cross-file duplicate detection (main path only): hashToIdx is
    // parser-local and a fresh LocTableParserCB is built per loadCsv call,
    // so a key repeated in a later CSV would miss here and silently
    // overwrite the earlier value without firing dag_on_duplicate_loc_key.
    // Consult the persistent main map and lazy-add to the parser-local map
    // so parseCsvV2 sees idx >= 0 (callback fires) and a subsequent
    // addValue can resolve idx -> hash via idxToHash. The full path
    // already uses locTbl->mapFull which persists across files.
    if (!full && locTbl->main.hashToOffset.findVal(h))
    {
      // Collision check BEFORE lazy-add: on a real cross-file wyhash
      // collision (different key bytes but same hash) we return -1 and
      // leave the parser-local maps untouched, so the new key's value is
      // dropped rather than overwriting the prior file's value in main.
      // The keyDump persists across loadCsv calls (LocalizationTable's
      // devCollisionDump when no caller dump), so this fires for
      // cross-file collisions too.
      if (keyDump && checkCollisionAndLog(h, key, key_len))
        return -1;
      const uint32_t newIdx = (uint32_t)idxToHash.size();
      idxToHash.push_back(h);
      auto inserted = hashToIdx.emplace_if_missing(h);
      *inserted.first = newIdx;
      return (int)newIdx;
    }
    return -1;
  }

  void onKeyProcessed(int /*n*/, char *data, bool full) override
  {
    G_ASSERT(data);
    const size_t len = strlen(data);
    const uint64_t h = loc_hash(data, len);
    // Collision check BEFORE committing the new key to parser/table maps.
    // On a wyhash collision against an earlier-seen key (intra- or cross-
    // file), skip the row entirely: do not push to idxToHash/hashToIdx or
    // mapFull, and do not extend tableFull. The subsequent getKey will then
    // return -1, and addValue (which checks n < 0) writes into discardBuf
    // so the parser-local row data is dropped without corrupting main.
    if (keyDump && checkCollisionAndLog(h, data, len))
    {
      memfree(data, tmpmem);
      return;
    }
    if (full)
    {
      // Pre-reserve offset 0 -> '\0' on the very first full-path key so that
      // any tableFull slot left untouched (e.g. a row missing a column whose
      // language was not found in this CSV, see loadCsvFull + parseCsvV2Full
      // bmap_col[langColNo] < 0 path) reads as the empty string when
      // getFullText -> stringsFull.getDataRawUnsafe(0) is called. Without
      // this, the slot is value-initialized to 0 (see resize below) and
      // offset 0 would alias into the first stored string -- or into
      // uninitialized page bytes before any string is stored.
      if (locTbl->stringsFull.empty())
      {
        if (locTbl->stringsFull.head.left < 1)
          locTbl->stringsFull.addPage(1);
        locTbl->stringsFull.head.data.get()[locTbl->stringsFull.head.used] = '\0';
        locTbl->stringsFull.head.used += 1;
        locTbl->stringsFull.head.left -= 1;
      }
      const uint32_t newIdx = (uint32_t)locTbl->mapFull.size();
      locTbl->mapFull.emplace_if_missing(h, newIdx);
      const int nLangs = locTbl->fullLangList.nameCount();
      // Value-initialize new slots to 0 so any slot the parser skips (missing
      // language column) resolves to the offset-0 NUL byte reserved above.
      if ((int)locTbl->tableFull.size() < ((int)newIdx + 1) * nLangs)
        locTbl->tableFull.resize(((int)newIdx + 1) * nLangs, 0u);
    }
    else
    {
      const uint32_t newIdx = (uint32_t)idxToHash.size();
      idxToHash.push_back(h);
      hashToIdx.emplace_if_missing(h, newIdx);
    }
    memfree(data, tmpmem);
  }

  char *addValue(int n, int len, int lang_idx = -1) override
  {
    // n < 0 signals that the row was dropped earlier (collision detected in
    // getKey or onKeyProcessed). Return a throwaway buffer so the parser
    // can write its value into something safe; the bytes are discarded on
    // the next addValue / parser exit.
    if (n < 0)
    {
      if ((int)discardBuf.size() < len)
        discardBuf.resize((size_t)len);
      return discardBuf.data();
    }
    if (lang_idx < 0)
    {
      G_ASSERTF(n < (int)idxToHash.size(), "n=%d size=%d", n, (int)idxToHash.size());
      return locTbl->main.reserveBytes(idxToHash[n], (size_t)len);
    }
    const int nLangs = locTbl->fullLangList.nameCount();
    G_ASSERTF((int)locTbl->tableFull.size() == (int)locTbl->mapFull.size() * nLangs,
      "tableFull.size() = %d mapFull.size()=%u nLangs=%d", (int)locTbl->tableFull.size(), locTbl->mapFull.size(), nLangs);
    const int slot = n * nLangs + lang_idx;
    if (locTbl->stringsFull.head.left < (uint32_t)len)
      locTbl->stringsFull.addPage((uint32_t)len);
    const uint32_t ofs = locTbl->stringsFull.getCurrentHeadOffset();
    char *buf = locTbl->stringsFull.head.data.get() + locTbl->stringsFull.head.used;
    locTbl->stringsFull.head.used += (uint32_t)len;
    locTbl->stringsFull.head.left -= (uint32_t)len;
    locTbl->tableFull[slot] = ofs;
    return buf;
  }

  void onValueProcessed(int /*n*/, char * /*data*/) override {}
  void reserve(int rows) override { locTbl->tableFull.reserve(rows * locTbl->fullLangList.nameCount()); }

  // Returns true if a wyhash collision was detected against an earlier
  // intern of this hash in keyDump (different bytes for the same hash);
  // logs the collision in that case. Returns false when keyDump's stored
  // bytes match (legitimate duplicate) or when the hash is fresh and gets
  // interned. Callers must have non-null keyDump.
  bool checkCollisionAndLog(uint64_t h, const char *key, size_t key_len)
  {
    bool collided = false;
    keyDump->tryAddString(h, key, key_len, &collided);
    if (collided)
    {
      logerr("[LANG] wyhash collision on key '%s' (hash 0x%llx) -- bump LOC_HASH_SEED and rebuild", key, (unsigned long long)h);
      return true;
    }
    return false;
  }

  LocalizationTable *locTbl;
  LocKeyDump *keyDump;
  Tab<uint64_t> idxToHash;
  HashedKeyMap<uint64_t, uint32_t> hashToIdx;
  // Throwaway buffer used by addValue when the row was dropped due to a
  // wyhash collision. Lives in tmpmem implicitly via Tab default.
  Tab<char> discardBuf;
};

// locRwLock guards the main localization state: locTable, fakeLocKeys,
// defLangOverride, force_def_lang.
// NoWritersSpinLockReadWriteLock: 4 bytes, reader fast-path is a single
// interlocked_increment (no kernel transition). Ideal here because writes
// only happen during startup/shutdown; at runtime it is read-only.
// Non-reentrant: caller-supplied dag_on_duplicate_loc_key must not call back
// into localization, and locked helpers (suffix _locked) are used to avoid
// re-acquiring the lock from inside a write critical section.
static NoWritersSpinLockReadWriteLock locRwLock;
using LocReadLock = ScopedLockReadTemplate<NoWritersSpinLockReadWriteLock>;
using LocWriteLock = ScopedLockWriteTemplate<NoWritersSpinLockReadWriteLock>;

// rtaLock (declared earlier next to rtaTable) guards rtaTable only.
// Spinlock variant: low-contention, no OS handle.
// Lock order: locRwLock -> rtaLock. Two sites nest them:
//   - LocalizationTable::getTextId takes rtaLock for read inside a locRwLock
//     read, to consult RTA additions when the main map misses.
//   - add_rta_localized_key_text holds locRwLock for read + rtaLock for write
//     across its check-and-insert step, so a concurrent locTable writer can't
//     add the same key between our miss-check and our RTA insert, and
//     clear_rta_localization / shutdown_localization can't interleave a
//     reset mid-batch.
// RTA mutations therefore briefly block startup loaders and language-override
// writers (which take locRwLock for write); reciprocally a long-running
// loader stalls add_rta_localized_key_text. Both are rare paths.

// LocalizationTable::loadCsv* are only ever called from functions that already
// hold locRwLock for write (load_localization_table_from_csv*, startup_
// localization*), so they do not acquire the lock themselves. The internal
// fallback to get_default_lang_locked() in the (filename, lang) overloads
// relies on that invariant -- get_default_lang() would re-enter locRwLock.
// In dev builds, if the caller did not supply a LocKeyDump, the table's own
// devCollisionDump is used so wyhash collision detection works across every
// loadCsv call against the same locTable (not just within one file).
bool LocalizationTable::loadCsv(char *buffer, int len, int lang_col, LocKeyDump *key_dump)
{
#if DAGOR_DBGLEVEL > 0
  if (!key_dump)
    key_dump = &devCollisionDump;
#endif
  LocTableParserCB cb(this, key_dump);
  return parseCsvV2(buffer, len, lang_col, &cb, false);
}

bool LocalizationTable::loadCsvV2(char *buffer, int len, int lang_col, LocKeyDump *key_dump)
{
#if DAGOR_DBGLEVEL > 0
  if (!key_dump)
    key_dump = &devCollisionDump;
#endif
  LocTableParserCB cb(this, key_dump);
  return parseCsvV2(buffer, len, lang_col, &cb);
}
bool LocalizationTable::loadCsvV2Full(char *buffer, int len, dag::ConstSpan<int> lang_col, LocKeyDump *key_dump)
{
#if DAGOR_DBGLEVEL > 0
  if (!key_dump)
    key_dump = &devCollisionDump;
#endif
  LocTableParserCB cb(this, key_dump);
  return parseCsvV2Full(buffer, len, lang_col, &cb);
}

static InitOnDemand<LocalizationTable> locTable;

static eastl::vector_set<uint32_t> missing_loc_keys;
static SpinLockReadWriteLock missing_loc_keys_lock;
static bool should_report_missing_key(const char *key)
{
  auto hash_key = str_hash_fnv1(key);
  {
    ScopedLockReadTemplate readLock(missing_loc_keys_lock);
    if (missing_loc_keys.find(hash_key) != missing_loc_keys.end())
      return false;
  }
  ScopedLockWriteTemplate writeLock(missing_loc_keys_lock);
  return missing_loc_keys.insert(hash_key).second;
}

LocTextId get_localized_text_id(const char *key)
{
  if (!key || !*key)
    return NULL;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return NULL;
  LocTextId result = locTable->getTextId(key);
  if (result)
    return result;
#if DAGOR_DBGLEVEL > 0
  if (!dgs_get_settings() || dgs_get_settings()->getBlockByNameEx("debug")->getBool("L10nMsg", true))
    if (should_report_missing_key(key))
      logerr("[LANG] error: no key '%s' in localization table", key);
#else
  if (should_report_missing_key(key))
    debug("[LANG] error: no key '%s' in localization table", key);
#endif
  return nullptr;
}

LocTextId get_localized_text_id_silent(const char *key)
{
  if (!key || !*key)
    return NULL;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return NULL;
  return locTable->getTextId(key);
}

LocTextId get_optional_localized_text_id(const char *key)
{
  if (!key || !*key)
    return NULL;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return NULL;
  LocTextId result = locTable->getTextId(key);
  if (result)
    return result;
#if DAGOR_DBGLEVEL > 0
  if (should_report_missing_key(key))
    debug("[LANG] no key '%s' in localization table", key);
#endif
  return nullptr;
}

bool does_localized_text_exist(const char *key)
{
  if (!key || !*key)
    return false;
  const uint64_t h = loc_hash(key, strlen(key));
  LocReadLock lock(locRwLock);
  return locTable && locTable->hasMainKey(h);
}

const char *get_localized_text(LocTextId id)
{
  // Lock preserves prior lifetime contract: while this call is in progress,
  // locTable (and the StringTableAllocator that `id` points into) cannot be
  // destroyed by shutdown_localization. RTA entries are not similarly pinned
  // here -- same as before this refactor; callers must avoid racing
  // clear_rta_localization with held LocTextIds.
  LocReadLock lock(locRwLock);
  if (!locTable)
    return "";
  return LocalizationTable::getText(id);
}

const char *get_localized_text_for_lang(const char *key, const char *lang)
{
  if (!key || !*key || !lang || !*lang)
    return NULL;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return NULL;
  int id = locTable->fullLangList.getNameId(lang);
  if (id < 0)
    return NULL;
  return locTable->getFullText(key, id);
}

const char *get_localized_text_for_lang_id(const char *key, int lang_id)
{
  if (!key || !*key || lang_id < 0)
    return nullptr;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return nullptr;
  return locTable->getFullText(key, lang_id);
}

const char *get_fake_loc_for_missing_key(const char *key)
{
  if (!key)
    return NULL;
  const size_t key_len = strlen(key);
  const uint64_t key_hash = loc_hash(key, key_len);
  {
    LocReadLock r(locRwLock);
    if (const char *p = fakeLocKeys.find(key_hash))
      return p;
  }
  LocWriteLock w(locRwLock);
  bool collided = false;
  const char *interned = fakeLocKeys.tryAddString(key_hash, key, key_len, &collided);
  // On collision, the colliding key gets the placeholder for the first-interned
  // key; subsequent calls hit the read-lock fast path with no further logerr.
  if (collided)
    logerr("[LANG] wyhash collision on fake key '%s' (hash 0x%llx) -- bump LOC_HASH_SEED and rebuild", key,
      (unsigned long long)key_hash);
  return interned;
}

bool load_localization_table_from_csv(const char *filename, int lang_col, LocKeyDump *out_keys)
{
  LocWriteLock lock(locRwLock);
  locTable.demandInit();
  return locTable->loadCsv(filename, lang_col, out_keys);
}

bool load_localization_table_from_csv(MemGeneralLoadCB *cb, int lang_col, LocKeyDump *out_keys)
{
  LocWriteLock lock(locRwLock);
  locTable.demandInit();
  return locTable->loadCsv(cb, lang_col, out_keys);
}

bool load_localization_table_from_csv_V2(const char *filename, const char *lang, LocKeyDump *out_keys)
{
  LocWriteLock lock(locRwLock);
  locTable.demandInit();

  if (!lang)
    lang = get_default_lang_locked();
  lang = ::remap_english_lang(lang, NULL);

  return locTable->loadCsv(filename, lang, out_keys);
}

bool load_localization_table_from_csv_V2(MemGeneralLoadCB *cb, const char *lang, LocKeyDump *out_keys)
{
  LocWriteLock lock(locRwLock);
  locTable.demandInit();

  if (!lang)
    lang = get_default_lang_locked();
  lang = ::remap_english_lang(lang, NULL);

  return locTable->loadCsv(cb, lang, out_keys);
}

// maybe replace Tab<char*> *col with Tab<String> ?
bool load_col_from_csv(const char *file_name, int col_no, NameMap *ids, Tab<char *> *col)
{
  LocWriteLock lock(locRwLock);
  class LoadColParserCB : public ICsvParserCB
  {
  public:
    LoadColParserCB(NameMap *ids_, Tab<char *> *col_) : ids(ids_), col(col_) {}
    NameMap *ids;
    Tab<char *> *col;

    virtual void allocateColumns(int num) { col->reserve(num); }
    virtual char *addKey(int /*n*/, int len) { return new char[len]; }
    virtual int getCount(bool) { return col->size(); }
    virtual int getKey(const char *, bool) { return -1; }
    virtual void onKeyProcessed(int n, char *data, bool)
    {
      if (ids->getNameId(data) >= 0)
        DAG_FATAL("Duplicate localization table entry '%s'", data);

      G_VERIFY(ids->addNameId(data) == n);
      delete[] data;
    }
    virtual char *addValue(int /*n*/, int len, int) { return (char *)memalloc(len, inimem); }
    virtual void onValueProcessed(int n, char *data)
    {
      G_ASSERT(col->size() == n);
      G_UNUSED(n);
      col->push_back(data);
    }
    virtual void reserve(int /*rows*/) {}
  };

  FullFileLoadCB fcb(file_name);
  if (!fcb.fileHandle)
    return false;
  int len = df_length(fcb.fileHandle);
  if (len < 0)
    return false;
  char *buffer = new char[len + 2];
  fcb.read(buffer, len);
  buffer[len] = '\n';
  len++;
  buffer[len] = '\0';

  LoadColParserCB parserCb(ids, col);
  bool res = LocalizationTable::parseCsvV2(buffer, len, col_no, &parserCb, false);
  delete[] buffer;
  return res;
}

bool startup_localization(const class DataBlock &blk, int lang_col, LocKeyDump *out_keys)
{
  LocWriteLock lock(locRwLock);
  locTable.demandInit();

  const DataBlock *csvListBlk = blk.getBlockByNameEx("locTable");
  int fileNameId = blk.getNameId("file");

  bool ok = true;
  for (int i = 0; i < csvListBlk->paramCount(); i++)
  {
    if (csvListBlk->getParamNameId(i) != fileNameId)
      continue;

    const char *filename = csvListBlk->getStr(i);

    if (!locTable->loadCsv(filename, lang_col, out_keys))
    {
      logerr("can't load localization table from %s", filename);
      ok = false;
    }
  }
  if (blk.getBlockByName("locTableFull"))
    logerr("%s block not supported for %s", "locTableFull", "startup_localization");
  return ok;
}


static int indo_europian_plural(int num) { return num == 1 ? 0 : 1; }


static int slavic_plural(int num)
{
  if (num % 10 == 1 && num % 100 != 11)
    return 0;
  else if (num % 10 >= 2 && num % 10 <= 4 && (num % 100 < 10 || num % 100 >= 20))
    return 1;
  else
    return 2;
}


static int polish_plural(int num)
{
  if (num == 1)
    return 0;
  else if (num % 10 >= 2 && num % 10 <= 4 && (num % 100 < 10 || num % 100 >= 20))
    return 1;
  else
    return 2;
}


static int czech_plural(int num)
{
  if (num == 1)
    return 0;
  else if (num >= 2 && num <= 4)
    return 1;
  else
    return 2;
}


static int romanian_plural(int num)
{
  if (num == 1)
    return 0;
  else if (num == 0 || (num % 100 > 0 && num % 100 < 20))
    return 1;
  else
    return 2;
}


static int arabic_plural(int num)
{
  if (num >= 0 && num <= 2)
    return num;
  if ((num % 100) >= 3 && (num % 100) <= 10)
    return 3;
  if ((num % 100) >= 11)
    return 4;
  return 5;
}


static int null_plural(int) { return DEFAULT_PLURAL_FORM_INDEX; }

// http://docs.translatehouse.org/projects/localization-guide/en/latest/l10n/pluralforms.html
static PluralFormFn get_plural_form_function_for_lang(const char *lang)
{
  if (strcmp(lang, "English") == 0 || strcmp(lang, "French") == 0 || strcmp(lang, "Italian") == 0 || strcmp(lang, "German") == 0 ||
      strcmp(lang, "Spanish") == 0 || strcmp(lang, "Turkish") == 0 || strcmp(lang, "Portuguese") == 0 ||
      strcmp(lang, "Hungarian") == 0 || strcmp(lang, "Georgian") == 0 || strcmp(lang, "Greek") == 0)
    return indo_europian_plural;

  if (strcmp(lang, "Russian") == 0 || strcmp(lang, "Serbian") == 0 || strcmp(lang, "Ukrainian") == 0 ||
      strcmp(lang, "Belarusian") == 0 || strcmp(lang, "Croatian") == 0)
    return slavic_plural;

  if (strcmp(lang, "Polish") == 0)
    return polish_plural;

  if (strcmp(lang, "Czech") == 0)
    return czech_plural;

  if (strcmp(lang, "Romanian") == 0)
    return romanian_plural;

  if (strcmp(lang, "Arabic") == 0)
    return arabic_plural;

  return null_plural;
}


bool startup_localization_V2(const class DataBlock &blk, const char *lang, LocKeyDump *out_keys)
{
  LocWriteLock lock(locRwLock);
  locTable.demandInit();

#if _TARGET_PC
  if (force_def_lang)
    strmem->free(force_def_lang);
  force_def_lang = NULL;
  if (blk.getStr("forceDefLang", NULL))
    force_def_lang = str_dup(blk.getStr("forceDefLang"), strmem);
#endif

  if (!lang)
    lang = get_default_lang_locked(blk);
  lang = ::remap_english_lang(lang, &blk);

  const DataBlock *csvListBlk = blk.getBlockByNameEx("locTable");
  int fileNameId = blk.getNameId("file");

  bool reverseLoad = blk.getBool("reverseLoad", false);

  bool ok = true;
  for (int i = 0; i < csvListBlk->paramCount(); i++)
  {
    int idx = reverseLoad ? (csvListBlk->paramCount() - 1 - i) : i;

    if (csvListBlk->getParamNameId(idx) != fileNameId)
      continue;

    const char *filename = csvListBlk->getStr(idx);

    if (!locTable->loadCsv(filename, lang, out_keys))
    {
      logerr("can't load localization table from %s", filename);
      ok = false;
    }
  }
  locTable->pluralFormFn = get_plural_form_function_for_lang(lang);

  csvListBlk = blk.getBlockByNameEx("locTableFull");
  if (csvListBlk->paramCount())
  {
    const DataBlock *langsBlk = blk.getBlockByNameEx("text_translation");
    if (const DataBlock *override = langsBlk->getBlockByName(get_platform_string_id()))
      langsBlk = override;
    locTable->fullLangList.reset();
    clear_and_shrink(locTable->langToPluralIdxFunctions);
    for (int i = 0; i < langsBlk->paramCount(); i++)
    {
      const char *langToAdd = langsBlk->getStr(i);
      locTable->fullLangList.addNameId(langToAdd);
      locTable->langToPluralIdxFunctions.push_back(get_plural_form_function_for_lang(langToAdd));
    }

    for (int i = 0; i < csvListBlk->paramCount(); i++)
    {
      int idx = reverseLoad ? (csvListBlk->paramCount() - 1 - i) : i;

      if (csvListBlk->getParamNameId(idx) != fileNameId)
        continue;

      const char *filename = csvListBlk->getStr(idx);

      if (!locTable->loadCsvFull(filename, out_keys))
      {
        logerr("can't load full localization table from %s", filename);
        ok = false;
      }
    }
  }

  return ok;
}

void shutdown_localization()
{
  LocWriteLock lock(locRwLock);
  locTable.demandDestroy();
  fakeLocKeys.clear();
}

void get_all_localization(const LocKeyDump *key_dump, Tab<const char *> &loc_keys, Tab<const char *> &loc_vals)
{
  loc_keys.resize(0);
  loc_vals.resize(0);
  if (!key_dump)
    return;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return;
  loc_keys.reserve(key_dump->hashToOffset.size());
  loc_vals.reserve(key_dump->hashToOffset.size());
  key_dump->hashToOffset.iterate([&](uint64_t hash, const uint32_t &key_ofs) {
    const uint32_t *val_ofs = locTable->main.hashToOffset.findVal(hash);
    if (!val_ofs)
      return;
    loc_keys.push_back(key_dump->strings.getDataRawUnsafe(key_ofs));
    loc_vals.push_back(locTable->main.strings.getDataRawUnsafe(*val_ofs));
  });
}


int get_plural_form_id_for_lang(const char *lang, int num)
{
  if (!lang || !*lang)
    return DEFAULT_PLURAL_FORM_INDEX;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return DEFAULT_PLURAL_FORM_INDEX;
  int langId = locTable->fullLangList.getNameId(lang);
  if (langId < 0 || locTable->langToPluralIdxFunctions.size() <= langId)
    return DEFAULT_PLURAL_FORM_INDEX;
  return locTable->langToPluralIdxFunctions[langId](num);
}


int get_plural_form_id(int num)
{
  LocReadLock lock(locRwLock);
  if (!locTable || !locTable->pluralFormFn)
    return DEFAULT_PLURAL_FORM_INDEX;
  return locTable->pluralFormFn(num);
}


void set_default_lang_override(const char *lang)
{
  SimpleString newOverride(lang);
  LocWriteLock lock(locRwLock);
  defLangOverride.swap(newOverride);
  // newOverride dtor frees the previous buffer outside the lock
}
bool is_default_lang_override_set()
{
  LocReadLock lock(locRwLock);
  return !defLangOverride.empty();
}

// Caller must hold locRwLock (read or write).
static const char *get_default_lang_locked()
{
  if (!defLangOverride.empty())
  {
    thread_local static SimpleString lang;
    lang = ::dgs_get_settings() ? ::dgs_get_settings()->getBlockByNameEx("langAlias")->getStr(defLangOverride, defLangOverride)
                                : defLangOverride.str();
    if (!lang.empty() && !isupper(lang[0]) && strcmp(lang, defLangOverride) == 0)
      lang[0] = toupper(lang[0]);
    return lang;
  }

#if _TARGET_C3


#elif _TARGET_C1 | _TARGET_C2



#elif _TARGET_XBOX
  if (const char *lang = get_default_lang_xbox())
    return lang;

#elif _TARGET_PC_WIN
  if (force_def_lang)
    return force_def_lang;

  LANGID curId = GetUserDefaultUILanguage();
  switch (PRIMARYLANGID(curId))
  {
    case LANG_ENGLISH: return "English";
    case LANG_JAPANESE: return "Japanese";
    case LANG_GERMAN: return "German";
    case LANG_FRENCH: return "French";
    case LANG_SPANISH: return "Spanish";
    case LANG_ITALIAN: return "Italian";
    case LANG_KOREAN: return "Korean";
    case LANG_PORTUGUESE: return "Portuguese";
    case LANG_POLISH: return "Polish";
    case LANG_RUSSIAN: return "Russian";
    case LANG_CZECH: return "Czech";
    case LANG_TURKISH: return "Turkish";
    case LANG_BELARUSIAN: return "Belarusian";
    case LANG_UKRAINIAN: return "Ukrainian";
    case LANG_SERBIAN: return "Serbian";
    case LANG_SERBIAN_NEUTRAL: return "Serbian";
    case LANG_HUNGARIAN: return "Hungarian";
    case LANG_ROMANIAN: return "Romanian";
    case LANG_VIETNAMESE: return "Vietnamese";
    case LANG_GREEK: return "Greek";
    case LANG_INDONESIAN: return "Indonesian";
    case LANG_THAI: return "Thai";
    case LANG_ARABIC: return "Arabic";
    case LANG_CHINESE:
      switch (SUBLANGID(curId))
      {
        case SUBLANG_CHINESE_TRADITIONAL: return "TChinese";
        case SUBLANG_CHINESE_SIMPLIFIED: return "Chinese";
      }
      break;
  };

#elif _TARGET_APPLE | _TARGET_ANDROID
  const char *lang = os_get_default_lang();
  if (strcmp(lang, "ru") == 0)
    return "Russian";
  if (strcmp(lang, "en") == 0)
    return "English";
  if (strcmp(lang, "ja") == 0)
    return "Japanese";
  if (strcmp(lang, "fr") == 0)
    return "French";
  if (strcmp(lang, "es") == 0)
    return "Spanish";
  if (strcmp(lang, "it") == 0)
    return "Italian";
  if (strcmp(lang, "ko") == 0)
    return "Korean";
  if (strcmp(lang, "pl") == 0)
    return "Polish";
  if (strcmp(lang, "cs") == 0)
    return "Czech";
  if (strcmp(lang, "de") == 0)
    return "German";
  if (strcmp(lang, "ro") == 0)
    return "Romanian";
  if (strcmp(lang, "pt") == 0)
    return "Portuguese";
  if (strcmp(lang, "tr") == 0)
    return "Turkish";
  if (strcmp(lang, "hu") == 0)
    return "Hungarian";
  if (strcmp(lang, "be") == 0)
    return "Belarusian";
  if (strcmp(lang, "uk") == 0)
    return "Ukrainian";
  if (strcmp(lang, "sr") == 0)
    return "Serbian";
  if (strcmp(lang, "zh") == 0)
    return "Chinese";
  if (strcmp(lang, "vi") == 0)
    return "Vietnamese";
  if (strcmp(lang, "el") == 0)
    return "Greek";
  if (strcmp(lang, "id") == 0)
    return "Indonesian";
  if (strcmp(lang, "th") == 0)
    return "Thai";
  if (strcmp(lang, "ar") == 0)
    return "Arabic";

#endif
  return "English";
}

const char *get_default_lang()
{
  LocReadLock lock(locRwLock);
  return get_default_lang_locked();
}

// Caller must hold locRwLock (read or write).
static const char *get_default_lang_locked(const DataBlock &blk)
{
  const char *system_default = get_default_lang_locked();

  const DataBlock *cb = blk.getBlockByNameEx("full_translation");
  const DataBlock *override = cb->getBlockByName(get_platform_string_id());
  if (override)
    cb = override;
  int nameId = blk.getNameId("lang");

  bool ok = false;
  for (int i = 0; i < cb->paramCount(); i++)
  {
    if (cb->getParamNameId(i) != nameId)
      continue;

    const char *lang = cb->getStr(i);

    if (dd_stricmp(lang, system_default) == 0)
      ok = true;
  }
  if (!ok)
  {
    system_default = blk.getStr("default_lang", "English");
  }
  return system_default;
}

const char *get_default_lang(const class DataBlock &blk)
{
  LocReadLock lock(locRwLock);
  return get_default_lang_locked(blk);
}


// Additional runtime localization support.
//
// Pattern (both overloads):
//   Phase 1: ensure locTable exists, briefly under locRwLock-write. Readers
//     short-circuit on a null locTable (see get_localized_text_id), so without
//     this RTA additions would be invisible during the brief
//     shutdown_localization() -> startup_localization() reload window or
//     pre-startup.
//   Phase 2: hold locRwLock-read + rtaLock-write together for check + insert
//     (lock order locRwLock -> rtaLock per the doc above). Two races this
//     closes vs. taking the locks separately:
//       - locTable getting the same key hash added between our check and our
//         insert (would leave a shadow RTA entry for the lifetime of the
//         locTable revision).
//       - clear_rta_localization() / shutdown_localization() interleaving
//         mid-batch on the DataBlock overload (would yield a partial batch).
// Between phase 1 and phase 2 the locks are released, so shutdown can still
// race in and destroy locTable -- phase 2 re-checks under the read lock.
// With hash-keyed storage, the skip-if-present check tests hash presence, not
// actual key match; a wyhash collision against an unrelated main key would
// cause the new RTA key to be silently skipped (resolving to the main key's
// text). The same applies RTA-vs-RTA: rtaTable.tryAddString returns the
// existing entry on a hash hit, so a colliding new RTA insert resolves to
// the prior RTA value with the same identical probability and failure mode.
// The probability is negligible (~3e-8 per insert against a 1M-entry
// table) and the failure mode is "wrong translation," not crash.
void add_rta_localized_key_text(const char *key, const char *text)
{
  const size_t key_len = strlen(key);
  const uint64_t key_hash = loc_hash(key, key_len);
  const size_t text_len = strlen(text);

  if (!locTable)
  {
    LocWriteLock w(locRwLock);
    locTable.demandInit();
  }

  LocReadLock mainLock(locRwLock);
  if (!locTable)
    return;
  if (locTable->hasMainKey(key_hash))
    return;
  RtaWriteLock rtaLockGuard(rtaLock);
  rtaTable.tryAddString(key_hash, text, text_len);
}

void add_rta_localized_key_text(const DataBlock &key_text)
{
  if (!key_text.paramCount())
    return;

  if (!locTable)
  {
    LocWriteLock w(locRwLock);
    locTable.demandInit();
  }

  LocReadLock mainLock(locRwLock);
  if (!locTable)
    return;
  RtaWriteLock rtaLockGuard(rtaLock);
  for (int i = 0; i < key_text.paramCount(); i++)
    if (key_text.getParamType(i) == DataBlock::TYPE_STRING)
    {
      const char *key = key_text.getParamName(i);
      const size_t key_len = strlen(key);
      const uint64_t key_hash = loc_hash(key, key_len);
      if (locTable->hasMainKey(key_hash))
        continue;

      const char *text = key_text.getStr(i);
      const size_t text_len = strlen(text);
      rtaTable.tryAddString(key_hash, text, text_len);
    }
}

void clear_rta_localization()
{
  RtaWriteLock lock(rtaLock);
  rtaTable.clear();
}

const char *language_to_locale_code(const char *language)
{
  static const struct
  {
    const char *language, *locale_code;
  } language_to_locale_code_map[] = {
    {"Spanish", "es-ES"},
    {"English", "en-US"},
    {"German", "de-DE"},
    {"French", "fr-FR"},
    {"Dutch", "nl-NL"},
    {"Portuguese", "pt-PT"},
    {"Chinese", "zh-CN"},
    {"TChinese", "zh-SG"},
    {"Danish", "da-DK"},
    {"Finnish", "fi-FI"},
    {"Italian", "it-IT"},
    {"Japanese", "ja-JP"},
    {"Norwegian", "no-NO"},
    {"Polish", "pl-PL"},
    {"Russian", "ru-RU"},
    {"Korean", "ko-KR"},
    {"Swedish", "sv-SE"},
    {"Turkish", "tr-TR"},
    {"HChinese", "zh-CN"},
    {"Czech", "cs-CZ"},
    {"Ukrainian", "uk-UA"},
    {"Serbian", "sr-RS"},
    {"Hungarian", "hu-HU"},
    {"Belarusian", "be-BY"},
    {"Romanian", "ro-RO"},
    {"Hebrew", "he-IL"},
    {"Vietnamese", "vi-VN"},
    {"Greek", "el-GR"},
    {"Indonesian", "id-ID"},
    {"Thai", "th-TH"},
    {"Arabic", "ar-SA"},
  };

  for (int i = 0; i < countof(language_to_locale_code_map); ++i)
    if (stricmp(language_to_locale_code_map[i].language, language) == 0)
      return language_to_locale_code_map[i].locale_code;

  return "en-US";
}

static bool need_force_language()
{
#if _TARGET_XBOX | _TARGET_C1 | _TARGET_C2 | _TARGET_C3
  bool const force_default_system_lang = true;
#else
  bool const force_default_system_lang = false;
#endif

  return force_default_system_lang || is_default_lang_override_set();
}

const char *get_current_language()
{
#if _TARGET_PC | _TARGET_ANDROID | _TARGET_IOS
  bool const allow_custom_language = true;
#else
  bool const allow_custom_language = false;
#endif

  const char *lang = ::get_default_lang();
  if (!allow_custom_language || need_force_language())
    return lang;

  return ::dgs_get_settings()->getStr("language", lang);
}

const char *get_force_language()
{
  if (need_force_language())
    return ::get_default_lang();
  else
    return "";
}

void set_language_to_settings(const char *lang)
{
  debug("settings language by override: %s", lang);
  const_cast<DataBlock *>(::dgs_get_settings())->setStr("language", lang);
}

int getLangId(const char *lang)
{
  if (!lang || !*lang)
    return -1;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return -1;
  return locTable->fullLangList.getNameId(lang);
}

const char *getLangById(int id)
{
  if (id < 0)
    return nullptr;
  LocReadLock lock(locRwLock);
  if (!locTable)
    return nullptr;
  return locTable->fullLangList.getName(id);
}
