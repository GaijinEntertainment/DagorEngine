#include <supp/_platform.h>
#include <util/dag_localization.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_log.h>
#include <generic/dag_initOnDemand.h>
#include <generic/dag_tab.h>
#include <util/dag_strTable.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fastStrMap.h>
#include <util/dag_hash.h>
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

bool (*dag_on_duplicate_loc_key)(const char *key) = NULL;
#if _TARGET_PC
static char *force_def_lang = NULL;
#endif
static FastNameMapEx fakeLocKeys;
static FastStrStrMap addRtLoc;
static FastNameMap addRtLocStrings;
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

static StrTable locStrTable;

class LocalizedTextEntry
{
public:
  char *text = nullptr;

  LocalizedTextEntry() = default;
  LocalizedTextEntry(LocalizedTextEntry &&e) : text(e.text) { e.text = nullptr; }
  LocalizedTextEntry(const LocalizedTextEntry &e) : text(locStrTable.is_belong(e.text) ? e.text : str_dup(e.text, inimem)) {}
  LocalizedTextEntry &operator=(LocalizedTextEntry &&e)
  {
    eastl::swap(text, e.text);
    return *this;
  }

  ~LocalizedTextEntry()
  {
    if (text && !locStrTable.is_belong(text))
      memfree(text, inimem);
  }
};
DAG_DECLARE_RELOCATABLE(LocalizedTextEntry);


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
// typedef FastNameMap LocHashNameMapType;
typedef int (*PluralFormFn)(int);

class LocalizationTable
{
public:
  Tab<LocalizedTextEntry> table;
  LocHashNameMapType map;
  PluralFormFn pluralFormFn;

  LocHashNameMapType mapCI;
  Tab<int> mapCItoCS;

  static constexpr int NOT_ALIGNED_POINTER_OFS = 3;

  Tab<LocalizedTextEntry> tableFull;
  LocHashNameMapType mapFull;
  LocHashNameMapType fullLangList;

  Tab<PluralFormFn> langToPluralIdxFunctions;

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
  static SimpleString lastFileNameForDebug;
#endif

  LocalizationTable() : table(inimem), tableFull(inimem), pluralFormFn(NULL) {}

  ~LocalizationTable()
  {
    if (locStrTable.data)
    {
      mem_set_0(table);
      mem_set_0(tableFull);
      locStrTable.clear();
    }
  }

  int getTextIndex(const char *key, bool ci = false)
  {
    int idx = -1;
    if (ci)
    {
      char *data = str_dup(key, tmpmem);
      dd_strlwr(data);
      int idxCI = mapCI.getNameId(key);
      idx = (idxCI >= 0) ? mapCItoCS[idxCI] : -1;
      memfree(data, tmpmem);
    }
    else
      idx = map.getNameId(key);
    return idx;
  }

  LocTextId getTextId(const char *key, bool ci = false)
  {
    int idx = getTextIndex(key, ci);
    if (idx < 0)
    {
      if (const char *text = addRtLoc.getStrId(key))
        return LocTextId(uintptr_t(text) | NOT_ALIGNED_POINTER_OFS);
      return NULL;
    }
    G_ASSERT(idx < table.size());
    return &table[idx];
  }

  const char *getFullText(const char *key, int lang_idx)
  {
    int idx = mapFull.getNameId(key);
    if (idx < 0)
      return NULL;
    G_ASSERT(idx < tableFull.size());
    return tableFull[idx * fullLangList.nameCount() + lang_idx].text;
  }

  static const char *getText(LocTextId id)
  {
    if (!id)
      return "";
    return (uintptr_t(id) & NOT_ALIGNED_POINTER_OFS) == 0
             ? id->text
             : (const char *)(void *)(uintptr_t(id) & ~uintptr_t(NOT_ALIGNED_POINTER_OFS));
  }

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
      int n = i_strlen(dbg_string);
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
          fatal("key longer than 4096 bytes in CSV");
        else
        {
          char tmp[4096];
          l = parseCsvString(ptr, tmp, true);
          tmp[4095] = 0;
          fatal("Too long key in CSV, len = %d, key = %s", l, tmp);
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
          fatal("key longer than 4096 bytes in CSV");
        else
        {
          char tmp[4096];
          l = parseCsvString(ptr, tmp, true);
          tmp[4095] = 0;
          fatal("Too long key in CSV, len = %d, key = %s", l, tmp);
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

  bool loadCsv(MemGeneralLoadCB *cb, int lang_col) { return loadCsv((char *)cb->data(), cb->size(), lang_col); }

  bool loadCsv(MemGeneralLoadCB *cb, const char *lang)
  {
    char *buffer = (char *)cb->data();
    int len = cb->size();

    int lang_col = getColForLang(buffer, len, lang);
    if (lang_col < 0)
      lang_col = getColForLang(buffer, len, get_default_lang());
    if (lang_col < 0)
      lang_col = 0;
    return loadCsvV2(buffer, len, lang_col);
  }

  bool loadCsv(const char *filename, int lang_col)
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
    bool result = loadCsv(buffer, len, lang_col);
    memfree(buffer, tmpmem);
    return result;
  }
  bool loadCsv(const char *filename, const char *lang)
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
      lang_col = getColForLang(buffer, len, get_default_lang());
    if (lang_col < 0)
      lang_col = 0;
    bool result = loadCsvV2(buffer, len, lang_col);
    memfree(buffer, tmpmem);
    return result;
  }

  bool loadCsvFull(const char *filename)
  {
    FullFileLoadCB cb(filename);
    if (!cb.fileHandle)
      return false;

    int len = df_length(cb.fileHandle);
    if (len == -1)
      return false;

    char *buffer = (char *)memalloc(len, tmpmem);
    if (!buffer)
      return false;

    if (cb.tryRead(buffer, len) != len)
    {
      memfree(buffer, tmpmem);
      return false;
    }

#if DAGOR_DBGLEVEL > 0 || defined(DAGOR_FORCE_LOGS)
    lastFileNameForDebug = filename;
#endif

    Tab<int> lang_col;
    lang_col.resize(fullLangList.nameCount());
    for (int i = 0; i < lang_col.size(); i++)
    {
      lang_col[i] = getColForLang(buffer, len, fullLangList.getName(i));
      G_ASSERTF(lang_col[i] >= 0, "Language '%s' not found.", fullLangList.getName(i));
    }
    bool result = loadCsvV2Full(buffer, len, lang_col);
    memfree(buffer, tmpmem);
    return result;
  }

  bool loadCsv(char *buffer, int len, int lang_col);
  bool loadCsvV2(char *buffer, int len, int lang_col);
  bool loadCsvV2Full(char *buffer, int len, dag::ConstSpan<int> lang_col);
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
  G_ASSERT(l < MAX_KEY_LEN);
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
    G_ASSERT(l < MAX_KEY_LEN);
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

class LocTableParserCB : public ICsvParserCB
{
public:
  LocTableParserCB(LocalizationTable *loc_tbl) : locTbl(loc_tbl) {}
  virtual void allocateColumns(int /*num*/) {}
  virtual int getCount(bool full)
  {
    LocHashNameMapType &map = full ? locTbl->mapFull : locTbl->map;
    return map.nameCount();
  }
  virtual char *addKey(int /*n*/, int len)
  {
    G_ASSERT(len);
    return (char *)memalloc(len + 1, tmpmem);
  }
  virtual int getKey(const char *key, bool full)
  {
    LocHashNameMapType &map = full ? locTbl->mapFull : locTbl->map;
    G_ASSERT(key);
    int idx = map.getNameId(key);
    return idx;
  }
  virtual void onKeyProcessed(int /* n */, char *data, bool full)
  {
    Tab<LocalizedTextEntry> &tbl = full ? locTbl->tableFull : locTbl->table;
    LocHashNameMapType &map = full ? locTbl->mapFull : locTbl->map;
    int mul = full ? locTbl->fullLangList.nameCount() : 1;
    G_ASSERT(data);
    int idx = map.addNameId(data);
    G_ASSERT(idx >= 0);
    if (tbl.size() < (idx + 1) * mul)
      append_items(tbl, (idx + 1) * mul - tbl.size());
    if (!full)
    {
      dd_strlwr(data);
      int idxCI = locTbl->mapCI.addNameId(data);
      G_ASSERT(idxCI <= locTbl->mapCItoCS.size());
      if (idxCI == locTbl->mapCItoCS.size())
        locTbl->mapCItoCS.push_back(idx);
    }
    memfree(data, tmpmem); // there's strdup in addNameId
  }
  virtual char *addValue(int n, int len, int lang_idx = -1)
  {
    Tab<LocalizedTextEntry> &tbl = lang_idx < 0 ? locTbl->table : locTbl->tableFull;
    int mul = lang_idx < 0 ? 1 : locTbl->fullLangList.nameCount();

    G_ASSERTF(tbl.size() == (lang_idx < 0 ? locTbl->map : locTbl->mapFull).nameCount() * mul,
      "tbl.size() = %d lang_idx %d mul %d nameCount()=%d", tbl.size(), lang_idx, mul,
      (lang_idx < 0 ? locTbl->map : locTbl->mapFull).nameCount());
    if (lang_idx >= 0)
      n = n * mul + lang_idx;
    if (tbl[n].text && !locStrTable.is_belong(tbl[n].text))
      memfree(tbl[n].text, inimem);
    tbl[n].text = (char *)memalloc(len + 1, inimem);
    return tbl[n].text;
  }
  virtual void onValueProcessed(int /*n*/, char * /*data*/) {}

  virtual void reserve(int rows) { locTbl->tableFull.reserve(rows * locTbl->fullLangList.nameCount()); }

  LocalizationTable *locTbl;
};

static InitOnDemand<WinCritSec> locCs;

bool LocalizationTable::loadCsv(char *buffer, int len, int lang_col)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  LocTableParserCB cb(this);
  return parseCsvV2(buffer, len, lang_col, &cb, false);
}

bool LocalizationTable::loadCsvV2(char *buffer, int len, int lang_col)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  LocTableParserCB cb(this);
  return parseCsvV2(buffer, len, lang_col, &cb);
}
bool LocalizationTable::loadCsvV2Full(char *buffer, int len, dag::ConstSpan<int> lang_col)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  LocTableParserCB cb(this);
  return parseCsvV2Full(buffer, len, lang_col, &cb);
}

static InitOnDemand<LocalizationTable> locTable;

static eastl::vector_set<uint32_t> missing_loc_keys;
static bool should_report_missing_key(const char *key)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  return missing_loc_keys.insert(str_hash_fnv1(key)).second;
}

LocTextId get_localized_text_id(const char *key, bool ci)
{
  if (!key || !*key || !locTable)
    return NULL;
  LocTextId result = locTable->getTextId(key, ci);
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

LocTextId get_optional_localized_text_id(const char *key, bool ci)
{
  if (!key || !*key || !locTable)
    return NULL;
  LocTextId result = locTable->getTextId(key, ci);
  if (result)
    return result;
#if DAGOR_DBGLEVEL > 0
  if (should_report_missing_key(key))
    debug("[LANG] no key '%s' in localization table", key);
#endif
  return nullptr;
}

bool does_localized_text_exist(const char *key, bool ci)
{
  if (!key || !*key || !locTable)
    return false;
  return locTable->getTextIndex(key, ci) >= 0;
}

const char *get_localized_text(LocTextId id) { return locTable ? locTable->getText(id) : ""; }

struct StrTblPred
{
  char *&operator()(char **s) { return *s; }
};

static void rebuild_str_table()
{
  Tab<char **> allstrs(tmpmem);
  allstrs.reserve(locTable->table.size() * 2);
  for (int i = 0; i < locTable->table.size(); ++i)
    if (locTable->table[i].text)
      allstrs.push_back(&locTable->table[i].text);
  for (int i = 0; i < locTable->tableFull.size(); ++i)
    if (locTable->tableFull[i].text)
      allstrs.push_back(&locTable->tableFull[i].text);
  locStrTable.build<true>(allstrs, StrTblPred(), inimem);
}

const char *get_localized_text_for_lang(const char *key, const char *lang)
{
  if (!key || !*key || !lang || !*lang || !locTable)
    return NULL;
  int id = locTable->fullLangList.getNameId(lang);
  if (id < 0)
    return NULL;
  return locTable->getFullText(key, id);
}

const char *get_fake_loc_for_missing_key(const char *key)
{
  if (!key)
    return NULL;
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  return fakeLocKeys.getName(fakeLocKeys.addNameId(key));
}

bool load_localization_table_from_csv(const char *filename, int lang_col)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  locTable.demandInit();

  bool result = locTable->loadCsv(filename, lang_col);

  // if (result) locTable->dump();

  return result;
}

bool load_localization_table_from_csv(MemGeneralLoadCB *cb, int lang_col)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  locTable.demandInit();

  bool result = locTable->loadCsv(cb, lang_col);

  // if (result) locTable->dump();

  return result;
}

bool load_localization_table_from_csv_V2(const char *filename, const char *lang)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  locTable.demandInit();

  if (!lang)
    lang = get_default_lang();
  lang = ::remap_english_lang(lang, NULL);

  bool result = locTable->loadCsv(filename, lang);

  if (result)
    rebuild_str_table();
  // if (result) locTable->dump();

  return result;
}

bool load_localization_table_from_csv_V2(MemGeneralLoadCB *cb, const char *lang)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  locTable.demandInit();

  if (!lang)
    lang = get_default_lang();
  lang = ::remap_english_lang(lang, NULL);

  bool result = locTable->loadCsv(cb, lang);

  if (result)
    rebuild_str_table();
  // if (result) locTable->dump();

  return result;
}

// maybe replace Tab<char*> *col with Tab<String> ?
bool load_col_from_csv(const char *file_name, int col_no, NameMap *ids, Tab<char *> *col)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
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
        fatal("Duplicate localization table entry '%s'", data);

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

bool startup_localization(const class DataBlock &blk, int lang_col)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  locTable.demandInit();

  const DataBlock *csvListBlk = blk.getBlockByNameEx("locTable");
  int fileNameId = blk.getNameId("file");

  bool ok = true;
  for (int i = 0; i < csvListBlk->paramCount(); i++)
  {
    if (csvListBlk->getParamNameId(i) != fileNameId)
      continue;

    const char *filename = csvListBlk->getStr(i);

    if (!locTable->loadCsv(filename, lang_col))
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

  return null_plural;
}


bool startup_localization_V2(const class DataBlock &blk, const char *lang)
{
  locCs.demandInit();
  WinAutoLock lock(*locCs);
  locTable.demandInit();

#if _TARGET_PC
  if (force_def_lang)
    strmem->free(force_def_lang);
  force_def_lang = NULL;
  if (blk.getStr("forceDefLang", NULL))
    force_def_lang = str_dup(blk.getStr("forceDefLang"), strmem);
#endif

  if (!lang)
    lang = get_default_lang(blk);
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

    if (!locTable->loadCsv(filename, lang))
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

      if (!locTable->loadCsvFull(filename))
      {
        logerr("can't load full localization table from %s", filename);
        ok = false;
      }
    }
  }

  rebuild_str_table();

  return ok;
}

void shutdown_localization()
{
  locCs.demandInit();
  {
    WinAutoLock lock(*locCs);
    locTable.demandDestroy();
    fakeLocKeys.reset(false);
  }
  locCs.demandDestroy();
}

void get_all_localization(Tab<const char *> &loc_keys, Tab<const char *> &loc_vals)
{
  if (!locTable)
  {
    loc_keys.resize(0);
    loc_vals.resize(0);
    return;
  }

  loc_keys.resize(locTable->table.size());
  loc_vals.resize(locTable->table.size());
  for (int i = 0; i < loc_keys.size(); i++)
  {
    loc_keys[i] = locTable->map.getName(i);
    G_ASSERTF(loc_keys[i], "%d: key <%s>, val <%s>", i, loc_keys[i], locTable->table[i].text);
    loc_vals[i] = locTable->table[i].text;
  }
}


int get_plural_form_id_for_lang(const char *lang, int num)
{
  if (!lang || !*lang || !locTable)
    return DEFAULT_PLURAL_FORM_INDEX;
  int langId = locTable->fullLangList.getNameId(lang);
  if (langId < 0 || locTable->langToPluralIdxFunctions.size() <= langId)
    return DEFAULT_PLURAL_FORM_INDEX;
  return locTable->langToPluralIdxFunctions[langId](num);
}


int get_plural_form_id(int num)
{
  if (locTable->pluralFormFn)
    return locTable->pluralFormFn(num);
  return DEFAULT_PLURAL_FORM_INDEX;
}


void set_default_lang_override(const char *lang) { defLangOverride = lang; }
bool is_default_lang_override_set() { return !defLangOverride.empty(); }

const char *get_default_lang()
{
  if (is_default_lang_override_set())
  {
    static SimpleString lang;
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

  return lang;

#endif
  return "English";
}

const char *get_default_lang(const class DataBlock &blk)
{
  const char *system_default = get_default_lang();

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


// additional runtime localization support
void add_rta_localized_key_text(const char *key, const char *text)
{
  locTable.demandInit();
  if (locTable->map.getNameId(key) < 0)
  {
    int id = addRtLocStrings.addNameId(text);
    addRtLoc.addStrId(key, (char *)addRtLocStrings.getName(id));
  }
}
void add_rta_localized_key_text(const DataBlock &key_text)
{
  locTable.demandInit();
  for (int i = 0; i < key_text.paramCount(); i++)
    if (key_text.getParamType(i) == DataBlock::TYPE_STRING && locTable->map.getNameId(key_text.getParamName(i)) < 0)
    {
      int id = addRtLocStrings.addNameId((char *)key_text.getStr(i));
      addRtLoc.addStrId(key_text.getParamName(i), (char *)addRtLocStrings.getName(id));
    }
}
void clear_rta_localization()
{
  addRtLoc.reset(false);
  addRtLocStrings.reset(false);
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
  };

  for (int i = 0; i < countof(language_to_locale_code_map); ++i)
    if (stricmp(language_to_locale_code_map[i].language, language) == 0)
      return language_to_locale_code_map[i].locale_code;

  return "en-US";
}

const char *get_current_language()
{
#if _TARGET_XBOX | _TARGET_C1 | _TARGET_C2 | _TARGET_C3
  bool const force_default_system_lang = true;
#else
  bool const force_default_system_lang = false;
#endif

#if _TARGET_PC | _TARGET_ANDROID | _TARGET_IOS
  bool const allow_custom_language = true;
#else
  bool const allow_custom_language = false;
#endif

  const char *lang = ::get_default_lang();
  if (is_default_lang_override_set() || force_default_system_lang)
  {
    debug("settings language by override: %s", lang);
    const_cast<DataBlock *>(::dgs_get_settings())->setStr("language", lang);
  }

  if (allow_custom_language)
    return ::dgs_get_settings()->getStr("language", lang);
  else
    return lang;
}
