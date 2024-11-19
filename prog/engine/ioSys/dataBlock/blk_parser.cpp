// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blk_shared.h"
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_memIo.h>
#include <util/le2be.h>
#include <util/dag_string.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#ifdef _MSC_VER
#pragma warning(disable : 4577)
#endif
#include <fast_float/fast_float.h>
#include "blk_comments_def.h"

static DataBlock::IIncludeFileResolver *fresolve = NULL;
class GenericRootIncludeFileResolver : public DataBlock::IIncludeFileResolver
{
public:
  virtual bool resolveIncludeFile(String &inout_fname)
  {
    if (!inout_fname.empty() && inout_fname[0] == '#')
    {
      inout_fname = String(0, "%s/%s", root.str(), &inout_fname[1]);
      return true;
    }
    if (!inout_fname.empty() && inout_fname[0] == '%')
      return true;
    return false;
  }
  String root;
};
static GenericRootIncludeFileResolver gen_root_inc_resv;


#define EOF_CHAR '\0'
#define INC_CURLINE  \
  {                  \
    ++curLine;       \
    curLineP = curp; \
  }
using dblk::is_ident_char;

struct TempString;

class DataBlockParser
{
public:
  Tab<char> &buffer;
  const char *text, *curp, *textend;
  const char *fileName;
  int curLine;
  const char *curLineP;
  Tab<String> includeStack;
  Tab<int> includeStackPrevCurLine;
  DataBlock *blkRef = nullptr;
  bool robustParsing;
  bool wasNewlineAfterStatement = false;
  int lastStatement = -1; // -1=none, 0=param, 1=block
  struct PendingComment
  {
    const char *ss, *se;
    bool cpp;
  };
  Tab<PendingComment> pendCmnt;
  DataBlock::IFileNotify *fnotify;

  DataBlockParser(Tab<char> &buf, const char *fn, bool robust_parsing, DataBlock::IFileNotify *fnot) :
    buffer(buf),
    text(&buf[0]),
    curp(&buf[0]),
    textend(&buf[buf.size() - 2]),
    curLine(1),
    fileName(fn),
    includeStack(tmpmem),
    curLineP(curp),
    robustParsing(robust_parsing),
    fnotify(fnot)
  {
    for (char *c = buf.data(); c < textend; ++c)
      if (*c == EOF_CHAR)
        *c = ' ';
    includeStack.push_back() = fileName;
    fileName = includeStack.back();
  }

  void updatePointers()
  {
    int pos = curp - text;
    int pos_ln = curLineP - text;

    text = &buffer[0];
    textend = text + buffer.size() - 1;
    curp = text + pos;
    curLineP = text + pos_ln;
  }

  void syntaxError(const char *msg)
  {
    if (!(curLineP >= buffer.data() && curLineP < (buffer.data() + buffer.size())))
    {
      curLineP = buffer.data();
      curLine = 1;
    }
    for (char *p = (char *)curLineP; *p; ++p)
      if (*p == '\n')
      {
        *p = 0;
        break;
      }
    blkRef->issue_error_parsing(fileName, curLine, msg, curLineP);
  }

#define SYNTAX_ERROR(x) \
  do                    \
  {                     \
    syntaxError(x);     \
    return false;       \
  } while (0)

  __forceinline bool endOfText() { return curp >= textend; }

  bool skipWhite(bool allow_crlf = true, bool track_newline_after_param = false);
  bool getIdent(TempString &);
  bool getValue(TempString &);
  bool parse(DataBlock &, bool isTop);

private:
  DataBlockParser &operator=(const DataBlockParser &) = delete;
  void flushPendingComments(DataBlock &blk, bool to_params)
  {
    for (auto &c : pendCmnt)
    {
      const char *nm = c.cpp ? COMMENT_PRE_CPP : COMMENT_PRE_C;
      if (to_params)
        blk.addStr(nm, String::mk_sub_str(c.ss, c.se));
      else
        blkRef->addNewBlock(nm)->addStr(nm, String::mk_sub_str(c.ss, c.se));
    }
    pendCmnt.clear();
  }
};


bool DataBlockParser::skipWhite(bool allow_crlf, bool track_newline_after_param)
{
  for (;;)
  {
    if (endOfText())
      break;

    char c = *curp++;
    if (c == ' ' || c == '\t' || c == '\x1A')
      continue;

    if (c == EOF_CHAR)
    {
      if (includeStack.size() > 1)
      {
        includeStack.pop_back();
        fileName = includeStack.back();
        curLine = includeStackPrevCurLine.back();
        includeStackPrevCurLine.pop_back();
      }
      continue;
    }

    if (!allow_crlf)
    {
      //== stop on \r \n
    }
    else if (c == '\r')
    {
      if (!endOfText() && *curp == '\n')
      {
        ++curp;
        INC_CURLINE;
        if (track_newline_after_param)
          wasNewlineAfterStatement = true;
      }
      continue;
    }
    else if (c == '\n')
    {
      INC_CURLINE;
      if (track_newline_after_param)
        wasNewlineAfterStatement = true;
      continue;
    }

    if (c == '/')
    {
      if (!endOfText())
      {
        char nc = *curp++;
        if (nc == '/')
        {
          const char *cpp_comment_start = curp;
          while (!endOfText())
          {
            char cc = *curp++;
            if (cc == '\r' || cc == '\n')
              break;
          }
          if (DAGOR_UNLIKELY(DataBlock::parseCommentsAsParams))
          {
            if (wasNewlineAfterStatement || lastStatement == -1)
              pendCmnt.push_back(PendingComment{cpp_comment_start, curp - 1, true});
            else if (lastStatement == 0)
              blkRef->addStr(COMMENT_POST_CPP, String::mk_sub_str(cpp_comment_start, curp - 1));
            else
              blkRef->addNewBlock(COMMENT_POST_CPP)->addStr(COMMENT_POST_CPP, String::mk_sub_str(cpp_comment_start, curp - 1));
          }
          continue;
        }
        else if (nc == '*')
        {
          const char *c_comment_start = curp;
          int cnt = 1;
          while (curp + 2 <= textend)
          {
            if (curp[0] == '/' && curp[1] == '*')
            {
              curp += 2;
              ++cnt;
            }
            else if (curp[0] == '*' && curp[1] == '/')
            {
              curp += 2;
              if (--cnt <= 0)
                break;
            }
            else if (*curp == '\r')
            {
              if (*(curp + 1) == '\n')
              {
                curp += 2;
                INC_CURLINE;
              }
            }
            else if (*curp == '\n')
            {
              curp++;
              INC_CURLINE;
            }
            else
              ++curp;
          }

          if (cnt > 0 && curp + 2 > textend)
            SYNTAX_ERROR("unexpected EOF inside comment");

          if (DAGOR_UNLIKELY(DataBlock::parseCommentsAsParams))
          {
            if (wasNewlineAfterStatement || lastStatement == -1)
              pendCmnt.push_back(PendingComment{c_comment_start, curp - 2, false});
            else if (lastStatement == 0)
              blkRef->addStr(COMMENT_POST_C, String::mk_sub_str(c_comment_start, curp - 2));
            else
              blkRef->addNewBlock(COMMENT_POST_C)->addStr(COMMENT_POST_C, String::mk_sub_str(c_comment_start, curp - 2));
          }
          continue;
        }
        else
          --curp;
      }

      --curp;
      break;
    }
    else
    {
      --curp;
      break;
    }
  }

  return true;
}

struct TempString
{
  static constexpr int SSO_SIZE = 32 - 3; // sizeof(bool) + sizeof(unt16_t)
  bool SSOflag = true;
  char buf[SSO_SIZE];
  uint16_t len = 0;
  String str; // can be unioned with buf
  TempString() { buf[0] = 0; }
  bool isSSO() const { return SSOflag; }
  char *data() { return isSSO() ? buf : str.data(); }
  const char *data() const { return isSSO() ? buf : str.data(); }
  const char *c_str() const { return data(); }
  const char *end() const { return data() + len; }
  int append(int cnt, const char *v)
  {
    if (isSSO())
    {
      if (len + cnt <= SSO_SIZE)
      {
        memcpy(buf + len, v, cnt);
        int at = len;
        len += cnt;
        return at;
      }
      else
        convertToHeap(len + cnt);
    }
    len += cnt;
    return append_items(str, cnt, v);
  }
  void erase(int at, int n)
  {
    if (isSSO())
    {
      if (len - at - n > 0)
        memmove(data() + at, data() + at + n, len - at - n);
    }
    else
      erase_items(str, at, n);
    len -= n;
  }
  void convertToHeap(unsigned hint)
  {
    str.reserve((hint + 7) & ~7);
    append_items(str, len, buf);
    SSOflag = false;
  }
  void clear()
  {
    str.clear();
    len = 0;
  }
  void resize(uint32_t sz)
  {
    if (isSSO())
    {
      if (sz <= SSO_SIZE)
      {
        len = sz;
        return;
      }
      else
        convertToHeap(sz);
    }
    str.resize(sz);
  }
  int push(char c) { return append(1, &c); }

  char &operator[](int at) { return data()[at]; }
  const char &operator[](int at) const { return data()[at]; }

  uint32_t size() const { return len; }
  int length() const { return (len == 0) ? 0 : len - 1; }
  operator char *() { return data(); }
  operator const char *() const { return data(); }
};


bool DataBlockParser::getIdent(TempString &name)
{
  for (;;)
  {
    if (!skipWhite())
      return false;

    if (endOfText())
      break;

    char c = *curp;
    if (is_ident_char(c))
    {
      const char *ident = curp;
      for (++curp; !endOfText(); ++curp)
        if (!is_ident_char(*curp))
          break;

      int len = curp - ident;

      name.resize(len + 1);
      memcpy(&name[0], ident, len);
      name[len] = 0;

      return true;
    }
    else if (c == '"' || c == '\'')
      return getValue(name);
    else
      break;
  }
  return false;
}


bool DataBlockParser::getValue(TempString &value)
{
  value.clear();

  char qc = 0;
  bool multi_line_str = false;
  if (*curp == '"' || *curp == '\'')
  {
    qc = *curp;
    ++curp;
    if (curp[0] == qc && curp[1] == qc)
    {
      multi_line_str = true;
      curp += 2;
      // skip first \n (only when follows quotation with possible whitespace)
      for (const char *p = curp; *p; p++)
        if (*p == '\n')
        {
          curp = p + 1;
          break;
        }
        else if (!strchr(" \r\t", *p))
          break;
    }
  }

  const char *multiComment = nullptr, *rewind_to_pos = nullptr;

  for (;;)
  {
    if (endOfText())
      SYNTAX_ERROR("unexpected EOF");

    char c = *curp;

    if (multiComment)
    {
      if (c == '\r')
      {
        if (*(curp + 1) == '\n')
        {
          curp += 2;
          INC_CURLINE;
        }
        rewind_to_pos = multiComment;
        break;
      }
      else if (c == '\n')
      {
        curp++;
        INC_CURLINE;
        rewind_to_pos = multiComment;
        break;
      }
      else if (c == EOF_CHAR)
        SYNTAX_ERROR("unclosed string");
      else if ((c == '*') && (*(curp + 1) == '/'))
      {
        curp += 2;
        c = *curp;
        if (c == '\r' || c == '\n')
          rewind_to_pos = multiComment;
        multiComment = nullptr;
      }
      else
      {
        curp++;
        continue;
      }
    }

    if (qc)
    {
      if (c == qc && !multi_line_str)
      {
        ++curp;
        if (!skipWhite())
          return false;
        if (*curp == ';')
          ++curp;
        break;
      }
      else if (c == qc && multi_line_str && curp[1] == qc && curp[2] == qc)
      {
        // skip last \n
        if (value.size() > 1 && value[value.size() - 1] == '\n')
          value.erase(value.size() - 1, 1);
        curp += 3;
        if (!skipWhite())
          return false;
        if (*curp == ';')
          ++curp;
        break;
      }
      else if (((c == '\r' || c == '\n') && !multi_line_str) || c == EOF_CHAR)
        SYNTAX_ERROR("unclosed string");
      else if (c == '~')
      {
        ++curp;

        if (endOfText())
          SYNTAX_ERROR("unclosed string");

        c = *curp;
        if (c == 'r')
          c = '\r';
        else if (c == 'n')
          c = '\n';
        else if (c == 't')
          c = '\t';
      }
      else if (c == '\r')
      {
        ++curp;
        continue;
      }
    }
    else
    {
      if (c == ';' || c == '\r' || c == '\n' || c == EOF_CHAR || c == '}')
      {
        if (c == ';')
          ++curp;
        break;
      }
      else if (c == '/')
      {
        char nc = *(curp + 1);

        if (nc == '/')
          break;
        else if (nc == '*')
        {
          multiComment = curp - 1;
          curp += 2;
          continue;
        }
      }
    }

    value.push(c);

    ++curp;
  }

  if (multiComment)
    for (;;) // infinite cycle
    {
      const char &c = *curp;
      if (c == EOF_CHAR)
        SYNTAX_ERROR("unclosed string");
      else if ((c == '\r') && (*(curp + 1) == '\n'))
      {
        curp++;
        INC_CURLINE;
      }
      else if (c == '\n')
        INC_CURLINE
      else if ((c == '*') && (*(curp + 1) == '/'))
      {
        curp += 2;
        multiComment = nullptr;
        break;
      }

      curp++;
    }

  if (!qc)
  {
    int i;
    for (i = value.size() - 1; i >= 0; --i)
      if (value[i] != ' ' && value[i] != '\t')
        break;
    ++i;
    if (i < value.size())
      value.erase(i, value.size() - i);
  }

  value.push(0);

  if (rewind_to_pos && DataBlock::parseCommentsAsParams)
    curp = rewind_to_pos;
  return true;
}

static int get_array_idx(char *name)
{
  char *p = strchr(name, '[');
  if (!p)
    return 0;
  *p = '\0';
  return atoi(p + 1) - 1;
}

static void makeFullPathFromRelative(String &path, const char *base_filename)
{
  if (!path.length() || !base_filename)
    return;

  if (path[0] == '/' || path[0] == '\\')
    return;

  if (path.length() > 2 && path[0] == ':' && path[1] == '/')
  {
    erase_items(path, 0, 2);
    return;
  }

  int i;
  for (i = i_strlen(base_filename) - 1; i >= 0; --i)
    if (base_filename[i] == '/' || base_filename[i] == '\\' || base_filename[i] == ':')
      break;

  int baseLen = i + 1;

  if (baseLen > 0)
    path.insert(0, base_filename, baseLen);
}

#define ADD_PARAM_CHECKED(NAME, TYPE, VALUE)                                                              \
  if (blk.addParam((NAME), (TYPE), (VALUE).c_str(), (VALUE).end(), curLine, fileName) < 0)                \
  {                                                                                                       \
    logmessage(blk.shared->blkRobustLoad() ? LOGLEVEL_WARN : LOGLEVEL_ERR,                                \
      "DataBlockParser: invalid value '%s' at line %d of file '%s'", (VALUE).c_str(), curLine, fileName); \
    return false;                                                                                         \
  }

static inline int is_digit(char value) { return value >= '0' && value <= '9'; }

template <typename IntType>
inline const char *parse_naive_int_templ(const char *__restrict value, const char *__restrict eof, IntType &v)
{
  bool is_neg = false;
  if (*value == '-')
  {
    is_neg = true;
    value++;
  }
  else if (*value == '+')
    value++;

  if (value == eof)
    return nullptr;
  typename eastl::make_unsigned<IntType>::type result = 0;

  if (!is_neg && value[0] == '0' && value[1] == 'x') // hex!
  {
    for (value += 2; value < eof; ++value)
    {
      char c = *value;
      if (is_digit(c))
        c = (c - '0');
      else if (c >= 'a' && c <= 'f')
        c = (c - ('a' - 10));
      else if (c >= 'A' && c <= 'F')
        c = (c - ('A' - 10));
      else
        break;
      result = (result << 4) | c;
    }
    v = result;
    return value;
  }
  do
  {
    result *= 10;
    result += *value - '0';
  } while (++value != eof && is_digit(*value));
  v = is_neg ? -IntType(result) : IntType(result);
  return value;
}

inline const char *parse_naive(const char *__restrict value, const char *__restrict eof, int &v)
{
  const char *endPtr = parse_naive_int_templ(value, eof, v);
#ifdef _DEBUG_TAB_
  if (endPtr)
  {
    char *endPtr2 = NULL;
    int i2;
    if (value[0] == '0') // for hex
      i2 = strtoul(value, &endPtr2, 0);
    else
      i2 = strtol(value, &endPtr2, 0);
    if (endPtr != endPtr2 || v != i2)
    {
      logerr("Parsed incorrectly, %d != %d, <%s>", v, i2, value);
      return nullptr;
    }
  }
#endif
  return endPtr;
}

inline const char *parse_naive(const char *__restrict value, const char *__restrict eof, float &v)
{
  v = 0;
  auto ret = fast_float::from_chars(*value == '+' ? value + 1 : value, eof, v);
  return ret.ec == std::errc() ? ret.ptr : nullptr;
}

inline const char *parse_naive(const char *__restrict value, const char *__restrict eof, int64_t &v)
{
  const char *endPtr = parse_naive_int_templ(value, eof, v);
#ifdef _DEBUG_TAB_
  if (endPtr)
  {
    long long int i2 = 0;
    int res = sscanf(value, "%lld", &i2);
    if (res != 1 || v != i2)
    {
      logerr("Parsed incorrectly, %d != %d, <%s>", v, i2, value);
      return nullptr;
    }
  }
#endif
  return endPtr;
}

template <typename NumberType>
const char *parse_naive_number(const char *__restrict value, const char *__restrict eof, NumberType &v)
{
  const char *endPtr = parse_naive(value, eof, v);
  if (endPtr && endPtr < eof && *endPtr)
    endPtr = nullptr;
  return endPtr;
}

static const char *skip_comma(const char *__restrict value, const char *__restrict eof)
{
  for (; value != eof && *value == ' '; ++value)
    ;
  if (value == eof || *value != ',') // skip only one comma
    return nullptr;
  for (++value; value != eof && *value == ' '; ++value)
    ;
  return value;
}

template <class Point>
inline const char *parse_point_int(const char *__restrict value, const char *__restrict eof, Point &v)
{
  constexpr size_t cnt = sizeof(Point) / sizeof(v.x) - 1;
  for (size_t ci = 0; ci < cnt; ++ci)
  {
    value = parse_naive(value, eof, v[ci]);
    if (!value)
      return nullptr;
    value = skip_comma(value, eof);
    if (!value)
      return nullptr;
  }
  return parse_naive(value, eof, v[cnt]);
}

template <class Point>
inline const char *parse_point(const char *__restrict value, const char *__restrict eof, Point &v)
{
  const char *r = parse_point_int(value, eof, v);
  if (!r)
    memset(&v, 0, sizeof(v));
  return r;
}

inline const char *skip_white(const char *__restrict value, const char *__restrict eof)
{
  for (; value != eof && *value == ' '; ++value)
    ;
  return value == eof ? NULL : value;
}


inline const char *parse_matrix(const char *__restrict value, const char *__restrict eof, TMatrix &tm)
{
  // value = skip_white(value);/// first is already skipped
  if (*(value++) != '[')
    return NULL;
  TMatrix ntm;
  for (int i = 0; i < 4; ++i)
  {
    value = skip_white(value, eof);
    if (!value || *value != '[')
      return NULL;
    const char *__restrict end = ++value;
    for (; end != eof && *end != ']'; ++end)
      ;
    if (end == eof)
      return NULL;
    Point3 col;
    value = parse_point(value, end, col);
    if (!value)
      return NULL;
    value = end + 1;
    ntm.setcol(i, col);
  }
  value = skip_white(value, eof);
  if (!value || *value != ']')
    return NULL;
  tm = ntm;
  return value;
}

inline const char *parse_e3dcolor(const char *__restrict value, const char *__restrict eof, E3DCOLOR &col)
{
  IPoint3 v;
  int w = 255;
  value = parse_point(value, eof, v);
  if (!value || (uint32_t(v.x) > 255 || uint32_t(v.y) > 255 || uint32_t(v.z) > 255))
    return NULL;

  const char *e4 = skip_comma(value, eof);
  if (!e4)
  {
    col = E3DCOLOR(v.x, v.y, v.z, w);
    return value;
  }
  const char *end = parse_naive(e4, eof, w);
  if (!end)
    return NULL;
  col = E3DCOLOR(v.x, v.y, v.z, w);
  return end;
}

#define VALUE_SYNTAX_ERROR()                                  \
  {                                                           \
    issue_error_bad_value(name, value, type, filename, line); \
    return -1;                                                \
  }

#define PARSING_VALUE_SYNTAX_ERROR(msg)                                                              \
  {                                                                                                  \
    issue_error_parsing(filename, line, msg, String(0, "%s (type %s) value %f", name, type, value)); \
    return -1;                                                                                       \
  }

int DataBlock::addParam(const char *name, int type, const char *value, const char *eof, int line, const char *filename, int at)
{
  toOwned();
  if (at < 0)
    at = paramsCount;
  G_UNUSED(eof);
  G_ASSERT(this != &emptyBlock);
  int paramNameId = addNameId(name);
  int itemId = findParam(paramNameId);
  if ((itemId >= 0) && (getParamType(itemId) != type))
  {
    issue_error_bad_type(name, type, getParamType(itemId), filename ? filename : resolveFilename());
    return -1;
  }
  int64_t buf[6]; // enough to hold TMatrix
  G_STATIC_ASSERT(sizeof(buf) >= sizeof(TMatrix));

  switch (type)
  {
    case TYPE_STRING:
      if (DAGOR_UNLIKELY(strlen(value) > 8191))
        issue_warning_huge_string(name, value, filename ? filename : resolveFilename(), line);
      break;
    case TYPE_INT:
      if (!parse_naive_number(value, eof, *(int *)buf))
        VALUE_SYNTAX_ERROR();
      break;
    case TYPE_REAL:
      if (!parse_naive_number(value, eof, *(float *)buf))
        VALUE_SYNTAX_ERROR();
      break;
    case TYPE_POINT2:
      if (!parse_point(value, eof, *(Point2 *)buf))
        VALUE_SYNTAX_ERROR();
      break;
    case TYPE_POINT3:
      if (!parse_point(value, eof, *(Point3 *)buf))
        VALUE_SYNTAX_ERROR();
      break;
    case TYPE_POINT4:
      if (!parse_point(value, eof, *(Point4 *)buf))
        VALUE_SYNTAX_ERROR();
      break;
    case TYPE_IPOINT2:
      if (!parse_point(value, eof, *(IPoint2 *)buf))
        VALUE_SYNTAX_ERROR();
      break;
    case TYPE_IPOINT3:
      if (!parse_point(value, eof, *(IPoint3 *)buf))
        VALUE_SYNTAX_ERROR();
      break;
    case TYPE_BOOL:
    {
      if (
        dd_stricmp(value, "yes") == 0 || dd_stricmp(value, "on") == 0 || dd_stricmp(value, "true") == 0 || dd_stricmp(value, "1") == 0)
        *(bool *)buf = true;
      else if (dd_stricmp(value, "no") == 0 || dd_stricmp(value, "off") == 0 || dd_stricmp(value, "false") == 0 ||
               dd_stricmp(value, "0") == 0)
        *(bool *)buf = false;
      else
      {
        *(bool *)buf = false;
        VALUE_SYNTAX_ERROR();
      }
    }
    break;
    case TYPE_E3DCOLOR:
    {
      E3DCOLOR col(255, 255, 255, 255);
      if (!parse_e3dcolor(value, eof, col))
        VALUE_SYNTAX_ERROR();
      *(E3DCOLOR *)buf = col;
    }
    break;
    case TYPE_MATRIX:
    {
      TMatrix &tm = *(TMatrix *)buf;
      tm.identity();
      if (!parse_matrix(value, eof, tm))
        VALUE_SYNTAX_ERROR();
    }
    break;
    case TYPE_INT64:
    {
      if (!parse_naive_number(value, eof, *(int64_t *)buf))
        VALUE_SYNTAX_ERROR();
    }
    break;
    default: G_ASSERT(0);
  }
  if (DAGOR_UNLIKELY(paramCount() == eastl::numeric_limits<decltype(DataBlock::paramsCount)>::max()))
    PARSING_VALUE_SYNTAX_ERROR("parameters count exceeds maximum value");
  if (type == TYPE_STRING)
    insertParamAt(at, paramNameId, value);
  else
    insertNewParamRaw(at, paramNameId, type, dblk::get_type_size(type), (char *)buf);

  return at;
}

bool DataBlockParser::parse(DataBlock &blk, bool isTop)
{
  blkRef = &blk;
  TempString name;
  TempString typeName;
  TempString value;
  String valueStr;
  for (;;)
  {
    if (!skipWhite(true, DataBlock::parseCommentsAsParams))
      return false;

    if (endOfText())
    {
      if (!isTop)
        SYNTAX_ERROR("unexpected EOF");
      break;
    }

    if (*curp == '}')
    {
      if (isTop)
        SYNTAX_ERROR("unexpected '}' in top block");
      ++curp;
      if (DAGOR_UNLIKELY(DataBlock::parseCommentsAsParams))
        flushPendingComments(blk, false);
      break;
    }

    const char *start = curp;
    name.clear();
    if (!getIdent(name))
      SYNTAX_ERROR("expected identifier");

    if (!skipWhite())
      return false;

    if (endOfText())
      SYNTAX_ERROR("unexpected EOF");
    bool name_not_null = true;
    if (strcmp(name, "@null") == 0)
      name_not_null = false, name[0] = '\0';

    if (*curp == '{')
    {
      ++curp;
      DataBlock *nb = NULL;
      if (DAGOR_UNLIKELY(DataBlock::parseCommentsAsParams))
      {
        wasNewlineAfterStatement = false;
        lastStatement = -1;
        flushPendingComments(blk, false);
      }

      if (name[0] != '@' || DataBlock::parseOverridesNotApply)
      {
        if (DAGOR_UNLIKELY(blk.blockCount() == eastl::numeric_limits<decltype(DataBlock::blocksCount)>::max()))
          SYNTAX_ERROR("blocks count exceeds maximum value");
        nb = blk.addNewBlock(name_not_null ? name.c_str() : (const char *)nullptr);
      }
      else if (DataBlock::parseOverridesIgnored)
        ; // do nothing
      else if (strncmp(name, "@clone-last:", 12) == 0)
      {
        nb = blk.addNewBlock(name + 12);
        if (blk.blockCount() > 1)
          nb->setFrom(blk.getBlock(blk.blockCount() - 2));
      }
      else if (strncmp(name, "@override:", 10) == 0)
      {
        char *blk_name = name.data() + 10;
        int idx = get_array_idx(blk_name), ord = idx + 1;
        int nid = blk.getNameId(blk_name);
        if (nid >= 0)
        {
          if (idx >= 0)
          {
            for (int i = 0, ie = blk.blockCount(); i < ie; i++)
              if (blk.getBlock(i)->getBlockNameId() == nid)
              {
                if (idx == 0)
                {
                  nb = blk.getBlock(i);
                  break;
                }
                else
                  idx--;
              }
          }
          else if (idx < -1)
          {
            for (int i = blk.blockCount() - 1; i >= 0; i--)
              if (blk.getBlock(i)->getBlockNameId() == nid)
              {
                if (idx == -2)
                {
                  nb = blk.getBlock(i);
                  break;
                }
                else
                  idx++;
              }
          }
        }
        if (!nb)
        {
          logerr("cannot find block <%s> (ordinal %d) for override in file %s", blk_name, ord, blk.resolveFilename(true));
        }
        (void)ord;
      }
      else if (strncmp(name, "@delete:", 8) == 0)
      {
        char *blk_name = name.data() + 8;
        int idx = get_array_idx(blk_name), ord = idx + 1;
        int nid = blk.getNameId(blk_name);
        if (nid >= 0)
          for (int i = 0, ie = blk.blockCount(); i < ie; i++)
            if (blk.getBlock(i)->getBlockNameId() == nid)
            {
              if (idx == 0)
              {
                blk.removeBlock(i);
                nid = -2;
                break;
              }
              else
                idx--;
            }
        if (nid != -2)
        {
          logerr("cannot find block %s (ordinal %d) for deletion in file %s", blk_name, ord, blk.resolveFilename(true));
        }
        (void)ord;
      }
      else if (strncmp(name, "@delete-all:", 12) == 0)
      {
        const char *blk_name = name.data() + 12;
        int nid = blk.getNameId(blk_name);
        bool found = false;
        if (nid >= 0)
          for (int i = blk.blockCount() - 1; i >= 0; i--)
            if (blk.getBlock(i)->getBlockNameId() == nid)
            {
              blk.removeBlock(i);
              found = true;
            }
      }
      else if (strcmp(name, "@override-last") == 0)
      {
        if (blk.blockCount() > 0)
          nb = blk.getBlock(blk.blockCount() - 1);
        else
          logerr("cannot find block for %s in file %s", name.c_str(), blk.resolveFilename(true));
      }
      else if (strcmp(name, "@delete-last") == 0)
      {
        if (blk.blockCount() > 0)
          blk.removeBlock(blk.blockCount() - 1);
        else
          logerr("cannot find block for %s in file %s", name.c_str(), blk.resolveFilename(true));
      }


      if (nb)
      {
        if (!parse(*nb, false))
          return false;
      }
      else
      {
        DataBlock b;
        b.shared->setBlkRobustLoad(blk.shared->blkRobustLoad());
        if (const char *fn = blk.shared->getSrc())
          b.shared->setSrc(fn);
        if (!parse(b, false))
          return false;
      }
      blkRef = &blk;
      lastStatement = 1;
    }
    else if (*curp == ':')
    {
      if (DAGOR_UNLIKELY(DataBlock::parseCommentsAsParams))
        flushPendingComments(blk, true);
      ++curp;
      typeName.clear();
      if (!getIdent(typeName))
        SYNTAX_ERROR("expected type identifier");

      int type = 0;
      if (typeName.length() == 1)
      {
        if (typeName[0] == 't')
          type = DataBlock::TYPE_STRING;
        else if (typeName[0] == 'i')
          type = DataBlock::TYPE_INT;
        else if (typeName[0] == 'b')
          type = DataBlock::TYPE_BOOL;
        else if (typeName[0] == 'c')
          type = DataBlock::TYPE_E3DCOLOR;
        else if (typeName[0] == 'r')
          type = DataBlock::TYPE_REAL;
        else if (typeName[0] == 'm')
          type = DataBlock::TYPE_MATRIX;
        else
          SYNTAX_ERROR("unknown type");
      }
      else if (typeName.length() == 2)
      {
        if (typeName[0] == 'p')
        {
          if (typeName[1] == '2')
            type = DataBlock::TYPE_POINT2;
          else if (typeName[1] == '3')
            type = DataBlock::TYPE_POINT3;
          else if (typeName[1] == '4')
            type = DataBlock::TYPE_POINT4;
          else
            SYNTAX_ERROR("unknown type");
        }
        else
          SYNTAX_ERROR("unknown type");
      }
      else if (typeName.length() == 3)
      {
        if (typeName[0] == 'i')
        {
          if (typeName[1] == 'p')
          {
            if (typeName[2] == '2')
              type = DataBlock::TYPE_IPOINT2;
            else if (typeName[2] == '3')
              type = DataBlock::TYPE_IPOINT3;
            else
              SYNTAX_ERROR("unknown type");
          }
          else if (typeName[1] == '6' && typeName[2] == '4')
            type = DataBlock::TYPE_INT64;
          else
            SYNTAX_ERROR("unknown type");
        }
        else
          SYNTAX_ERROR("unknown type");
      }
      else
        SYNTAX_ERROR("unknown type");

      if (!skipWhite())
        return false;

      if (endOfText())
        SYNTAX_ERROR("unexpected EOF");

      bool is_array = false;
      if (curp[0] == '[' && curp[1] == ']')
      {
        curp += 2;
        is_array = true;

        if (!skipWhite())
          return false;
      }

      if (*curp++ != '=')
        SYNTAX_ERROR("expected '='");

      if (!skipWhite(false))
        return false;
      if (strchr("\r\n", *curp))
        SYNTAX_ERROR("unexpected CR/LF");

      if (endOfText())
        SYNTAX_ERROR("unexpected EOF");

      if (is_array)
      {
        if (name[0] == '@' && !DataBlock::parseOverridesNotApply)
          SYNTAX_ERROR("wrong identifier");

        if (!skipWhite(false))
          return false;

        if (*curp++ != '[')
          SYNTAX_ERROR("expected '['");

        for (;;)
        {
          if (!skipWhite())
            return false;

          if (*curp == ']')
          {
            curp++;
            break;
          }

          value.clear();
          if (!getValue(value))
            return false;

          ADD_PARAM_CHECKED(name_not_null ? name.c_str() : (const char *)nullptr, type, value);
        }
        wasNewlineAfterStatement = false;
        lastStatement = 0;
        continue;
      }

      value.clear();
      if (!getValue(value))
        return false;

      if (name[0] != '@' || DataBlock::parseOverridesNotApply)
      {
        ADD_PARAM_CHECKED(name_not_null ? name.c_str() : (const char *)nullptr, type, value);
      }
      else if (DataBlock::parseOverridesIgnored)
        ; // do nothing
      else if (strncmp(name, "@override:", 10) == 0)
      {
        char *pname = name.data() + 10;
        int idx = get_array_idx(pname), ord = idx + 1;
        int nid = blk.getNameId(pname);
        if (nid >= 0)
        {
          int i = -1;
          if (idx >= 0)
          {
            for (i = 0; i < blk.paramCount(); i++)
              if (blk.getParamNameId(i) == nid)
              {
                if (idx == 0)
                  break;
                else
                  idx--;
              }
          }
          else if (idx < -1)
          {
            for (i = blk.paramCount() - 1; i >= 0; i--)
              if (blk.getParamNameId(i) == nid)
              {
                if (idx == -2)
                  break;
                else
                  idx++;
              }
          }
          if (i >= 0 && i < blk.paramCount())
          {
            if (type != blk.getParamType(i) && !DataBlock::allowVarTypeChange)
              logerr("different types (%d != %d) of param <%s> (ordinal %d) for override in file %s", type, blk.getParamType(i), pname,
                ord, blk.resolveFilename(true));
            else if (type != blk.getParamType(i))
            {
              for (int j = blk.paramCount() - 1; j >= i; j--)
                if (blk.getParamNameId(j) == nid)
                  blk.removeParam(j);

              ADD_PARAM_CHECKED(pname, type, value);
            }
            else if (blk.addParam(pname, type, value.c_str(), value.end(), curLine, fileName, i) >= 0)
              blk.removeParam(i + 1);
            nid = -2;
          }
        }
        if (nid != -2)
        {
          logerr("cannot find param <%s> (ordinal %d) for override in file %s", pname, ord, blk.resolveFilename(true));
        }
        (void)ord;
      }
      else if (strncmp(name, "@delete:", 8) == 0)
      {
        char *pname = name.data() + 8;
        int idx = get_array_idx(pname), ord = idx + 1;
        int nid = blk.getNameId(pname);
        if (nid >= 0)
          for (int i = 0, ie = blk.paramCount(); i < ie; i++)
            if (blk.getParamNameId(i) == nid)
            {
              if (idx == 0)
              {
                blk.removeParam(i);
                nid = -2;
                break;
              }
              else
                idx--;
            }
        if (nid != -2)
        {
          logerr("cannot find param %s (ordinal %d) for deletion in file %s", pname, ord, blk.resolveFilename(true));
        }
        (void)ord;
      }
      else if (strncmp(name, "@delete-all:", 12) == 0)
      {
        const char *pname = name.data() + 12;
        int nid = blk.getNameId(pname);
        bool found = false;
        if (nid >= 0)
          for (int i = blk.paramCount() - 1; i >= 0; i--)
            if (blk.getParamNameId(i) == nid)
            {
              blk.removeParam(i);
              found = true;
            }
      }
      else if (strcmp(name, "@include") == 0 || CHECK_COMMENT_PREFIX(name))
      {
        ADD_PARAM_CHECKED(name.c_str(), type, value);
      }
      wasNewlineAfterStatement = false;
      lastStatement = 0;
    }
    else if (*curp == '=' && DataBlock::allowSimpleString)
    {
      if (DAGOR_UNLIKELY(DataBlock::parseCommentsAsParams))
        flushPendingComments(blk, true);
      if (!skipWhite(false))
        return false;
      if (strchr("\r\n", *curp))
        SYNTAX_ERROR("unexpected CR/LF");

      if (endOfText())
        SYNTAX_ERROR("unexpected EOF");

      curp++;

      if (!skipWhite())
        return false;

      if (endOfText())
        SYNTAX_ERROR("unexpected EOF");

      value.clear();
      if (!getValue(value))
        return false;

      if (name[0] != '@' || DataBlock::parseOverridesNotApply)
      {
        ADD_PARAM_CHECKED(name_not_null ? name.c_str() : (const char *)nullptr, DataBlock::TYPE_STRING, value);
      }
      else if (DataBlock::parseOverridesIgnored)
        ; // do nothing
      else if (strncmp(name, "@override:", 10) == 0)
      {
        char *pname = name.data() + 10;
        int idx = get_array_idx(pname), ord = idx + 1;
        int nid = blk.getNameId(pname);
        if (nid >= 0)
          for (int i = 0, ie = blk.paramCount(); i < ie; i++)
            if (blk.getParamNameId(i) == nid)
            {
              if (idx == 0)
              {
                if (DataBlock::TYPE_STRING != blk.getParamType(i))
                  logerr("different types (%d != %d) of param <%s> (ordinal %d) for override in file %s", DataBlock::TYPE_STRING,
                    blk.getParamType(i), pname, ord, blk.resolveFilename(true));
                else
                  blk.setStr(i, value);
                nid = -2;
                break;
              }
              else
                idx--;
            }
        if (nid != -2)
          logerr("cannot find param <%s> (ordinal %d) for override in file %s", pname, ord, blk.resolveFilename(true));
        (void)ord;
      }
      else if (strncmp(name, "@delete:", 8) == 0)
      {
        char *pname = name.data() + 8;
        int idx = get_array_idx(pname), ord = idx + 1;
        int nid = blk.getNameId(pname);
        if (nid >= 0)
          for (int i = 0, ie = blk.paramCount(); i < ie; i++)
            if (blk.getParamNameId(i) == nid)
            {
              if (idx == 0)
              {
                blk.removeParam(i);
                nid = -2;
                break;
              }
              else
                idx--;
            }
        if (nid != -2)
          logerr("cannot find param %s (ordinal %d) for deletion in file %s", pname, ord, blk.resolveFilename(true));
        (void)ord;
      }
      wasNewlineAfterStatement = false;
      lastStatement = 0;
    }
    else if (dd_stricmp(name, "include") == 0)
    {
      // We have to cache 'filename' here because 'getValue()' might incorrectly change it
      // in case when this include is last one in file (which breaks relative pathes)
      String cachedFileName(fileName);
      int include_curLine = curLine;
      value.clear();
      if (!getValue(value))
        return false;
      if (DataBlock::parseIncludesAsParams)
      {
        if (DAGOR_UNLIKELY(DataBlock::parseCommentsAsParams))
          flushPendingComments(blk, true);
        ADD_PARAM_CHECKED("@include", DataBlock::TYPE_STRING, value);
        continue;
      }
      valueStr = value.data();
      if (!fresolve || !fresolve->resolveIncludeFile(valueStr))
        if (*valueStr.str() != '%')
          makeFullPathFromRelative(valueStr, cachedFileName.str());

      if (fnotify)
        fnotify->onFileLoaded(valueStr);

      includeStack.push_back(valueStr);
      fileName = includeStack.back();

      file_ptr_t h = df_open(valueStr, DF_READ | (robustParsing ? DF_IGNORE_MISSING : 0));
      if (!h)
      {
        includeStack.pop_back();
        fileName = includeStack.back();
        blkRef->issue_error_parsing(cachedFileName, include_curLine, String(0, "can't open include file '%s'", valueStr), " ");
        return false;
      }

      int len = df_length(h);
      if (len < 0)
      {
        df_close(h);
        includeStack.pop_back();
        fileName = includeStack.back();
        blkRef->issue_error_parsing(cachedFileName, include_curLine, String(0, "error loading include file '%s'", valueStr), " ");
        return false;
      }

      includeStackPrevCurLine.push_back(curLine);
      curLine = 1;

      erase_items(buffer, start - text, curp - start - 1);
      curLineP = curp = start;
      *(char *)curp = EOF_CHAR;

      int pos = curp - text;

      insert_items(buffer, pos, len + 2);

      if (df_read(h, &buffer[pos], len) != len)
      {
        df_close(h);
        SYNTAX_ERROR("error loading include file");
      }

      if ((/*new binaryformats*/ len > 1 && buffer[pos] >= dblk::BBF_full_binary_in_stream &&
            buffer[pos] <= dblk::BBF_binary_with_shared_nm_zd) ||
          (/* old BBF3 format */ len >= 4 && *(int *)&buffer[pos] == _MAKE4C('BBF')))
      {
        logwarn("including binary file '%s', not fastest codepath", valueStr.str());

        DataBlock inc_blk;
        inc_blk.shared->setBlkRobustLoad(blk.shared->blkRobustLoad());
        struct VromfsInPlaceMemLoadCB : public InPlaceMemLoadCB
        {
          file_ptr_t fp = nullptr;
          VromfsInPlaceMemLoadCB(const void *p, int s, file_ptr_t f) : InPlaceMemLoadCB(p, s), fp(f) {}
          const VirtualRomFsData *getTargetVromFs() const override { return df_get_vromfs_for_file_ptr(fp); }
        };
        VromfsInPlaceMemLoadCB crd(&buffer[pos], len, h);
        inc_blk.loadFromStream(crd, valueStr, fnotify, len);
        erase_items(buffer, pos, len + 2);

        DynamicMemGeneralSaveCB cwr(tmpmem, len * 2, 4 << 10);
        inc_blk.saveToTextStream(cwr);
        len = cwr.size();
        insert_items(buffer, pos, len + 2);
        memcpy(&buffer[pos], cwr.data(), len);
      }

      buffer[pos + len + 0] = '\r';
      buffer[pos + len + 1] = '\n';

      for (int i = 0; i < len; ++i)
        if (buffer[pos + i] == EOF_CHAR)
          buffer[pos + i] = ' ';

      // skip UTF-8 BOM
      if (len >= 3 && (unsigned char)buffer[pos] == 0xEF && (unsigned char)buffer[pos + 1] == 0xBB &&
          (unsigned char)buffer[pos + 2] == 0xBF)
        erase_items(buffer, pos, 3);

      df_close(h);

      updatePointers();
      lastStatement = -1;
    }
    else
      SYNTAX_ERROR("syntax error");
  }

  return true;
}


static bool parse_from_text(DataBlock &blk, Tab<char> &text, const char *filename, bool robust_load, DataBlock::IFileNotify *fnotify)
{
  char *end = text.size() ? (char *)memchr(text.data(), 0, text.size()) : (char *)NULL;
  if (end)
    erase_items(text, end - text.data(), text.data() + text.size() - end);
  append_items(text, 3, "\n\0\0");
  if (text.size() >= 3 && memcmp(text.data(), "\xEF\xBB\xBF", 3) == 0)
    memcpy(text.data(), "   ", 3);

  DataBlockParser parser(text, filename, robust_load, fnotify);
  return parser.parse(blk, true);
}


static bool is_patch(const char *fn) { return fn && strcmp(fn, ".patch") == 0; }

bool DataBlock::loadText(const char *text, int len, const char *filename, DataBlock::IFileNotify *fnotify)
{
  if (shared->blkBinOnlyLoad())
    return false;

  if (len >= 3 && memcmp(text, "\xEF\xBB\xBF", 3) == 0)
    len -= 3, text += 3;
  if (!len)
  {
    reset();
    shared->setBlkValid(true);
    return true;
  }

  Tab<char> buf(tmpmem);
  buf.reserve(len + 3); // +3 because of append in loadText
  buf.resize(len);      // don't use copyFrom because it does realloc despite of total (i.e. reserved) memory
  memcpy(buf.data(), text, len);

  if (!is_patch(filename))
  {
    reset();
    if (filename)
      shared->setSrc(filename);
#if DAGOR_DBGLEVEL > 0
    else
      shared->setSrc(String(0, "BLK\n%.*s", min(buf.size(), 512u), buf.size() ? buf.data() : ""));
#endif
  }

  bool ret = parse_from_text(*this, buf, filename, shared->blkRobustLoad(), fnotify);
  shared->setBlkValid(ret);
  return ret;
}

bool DataBlock::load(const char *fname, DataBlock::IFileNotify *fnotify)
{
  reset();

  if (!fname || !*fname)
  {
    shared->setBlkValid(false);
    issue_error_missing_file(fname, "invalid BLK filename");
    return false;
  }

  FullFileLoadCB crd(fname, DF_READ | DF_IGNORE_MISSING);
  String fileName_stor;

  if (!crd.fileHandle && !dd_get_fname_ext(fname))
  {
    fileName_stor.setStrCat(fname, ".blk");
    if (crd.open(fileName_stor, DF_READ | DF_IGNORE_MISSING))
      fname = fileName_stor;
  }
  if (!crd.fileHandle)
  {
    if (!shared->blkRobustLoad())
      logwarn("BLK: failed to open file \"%s\" (%s)", fname, fileName_stor);

    shared->setBlkValid(false);

    if (!shared->blkRobustLoad() && fatalOnMissingFile && ::dag_on_file_not_found)
      ::dag_on_file_not_found(fname);

    issue_error_missing_file(fname, "BLK not found");
    return false;
  }

  if (topMost())
    shared->setSrc(fname);

  const int len = crd.getTargetDataSize() < 0 ? ::df_length(crd.fileHandle) : crd.getTargetDataSize();
  if (len < 0)
  {
    shared->setBlkValid(false);
    issue_error_load_failed(fname, nullptr);
    return false;
  }

  if (fnotify)
    fnotify->onFileLoaded(fname);

  if (len == 0) // Empty file in - empty blk out.
    return true;

  return loadFromStream(crd, fname, fnotify, len);
}

bool DataBlock::loadFromStream(IGenLoad &crd, const char *fname, DataBlock::IFileNotify *fnotify, unsigned hint_size)
{
  reset();
  unsigned blkFlags = shared->blkFlags;
  if (fname)
    shared->setSrc(fname);

  DBNameMap *shared_nm = nullptr;
  ZSTD_DDict_s *ddict = nullptr;

  DAGOR_TRY
  {

    unsigned label = 0;
    if (crd.tryRead(&label, 1) != 1)
      return !shared->blkBinOnlyLoad(); // allow 0-length be read as valid empty BLK only as text format

    constexpr bool zstdtmp = true; // Use framemem if possible
    bool valid = false;
    if (label == dblk::BBF_binary_with_shared_nm || label == dblk::BBF_binary_with_shared_nm_z ||
        label == dblk::BBF_binary_with_shared_nm_zd)
    {
      const VirtualRomFsData *fs = crd.getTargetVromFs();
      const char *err_desc = nullptr;
      if (!dblk::check_shared_name_map_valid(fs, &err_desc))
      {
        shared->setBlkValid(true);
        issue_error_load_failed(fname, err_desc);
        return false;
      }
      dag::ConstSpan<char> rom_file_data = crd.getTargetRomData();
      if (!rom_file_data.data())
      {
        shared->setBlkValid(true);
        issue_error_load_failed(fname,
          String(0, "bad ROM data for file: rom=(%p,%d) crd.tell()=%d)", rom_file_data.data(), rom_file_data.size(), crd.tell()));
        return false;
      }

      shared_nm = dblk::get_vromfs_shared_name_map(fs);
      if (!shared_nm)
      {
        shared->setBlkValid(true);
        issue_error_load_failed(fname, "failed to load shared namemap for binary format (\\3, \\4, \\5)");
        return false;
      }

      ddict = (label == dblk::BBF_binary_with_shared_nm_zd) ? dblk::get_vromfs_blk_ddict(fs) : nullptr;
      if (label == dblk::BBF_binary_with_shared_nm_zd)
      {
        ZstdLoadFromMemCB zcrd(make_span_const(rom_file_data).subspan(crd.tell()), ddict, zstdtmp);
        valid = loadFromBinDump(zcrd, shared_nm);
      }
      else if (label == dblk::BBF_binary_with_shared_nm)
        valid = loadFromBinDump(crd, shared_nm);
      else if (label == dblk::BBF_binary_with_shared_nm_z) //-V547
      {
        ZstdLoadFromMemCB zcrd(make_span_const(rom_file_data).subspan(crd.tell()), nullptr, zstdtmp);
        valid = loadFromBinDump(zcrd, shared_nm);
      }
      dblk::release_vromfs_blk_ddict(ddict);
      ddict = nullptr; // release ddict refcount
      dblk::release_vromfs_shared_name_map(shared_nm);
      shared_nm = nullptr; // release shared_nm refcount
    }
    else if (label == dblk::BBF_full_binary_in_stream)
      valid = loadFromBinDump(crd, nullptr);
    else if (label == dblk::BBF_full_binary_in_stream_z)
    {
      unsigned csz = 0;
      crd.read(&csz, 3);
      ZstdLoadCB zcrd(crd, csz, nullptr, zstdtmp);
      G_VERIFY(zcrd.readIntP<1>() == dblk::BBF_full_binary_in_stream);
      valid = loadFromBinDump(zcrd, nullptr);
    }
    else
    {
      unsigned hdr[3] = {label, 0, 0};

      // try to get first 12 bytes of stream
      int hdr_read = crd.tryRead(1 + (char *)hdr, 11) + 1;
      if (hdr_read < 12)
      {
        if (shared->blkBinOnlyLoad())
          return false;
        return loadText((char *)hdr, hdr_read, fname, fnotify);
      }

      if (hdr[0] == _MAKE4C('blk ') && hdr[1] == _MAKE4C('1.1'))
      {
        issue_error_load_failed(fname, "obsolete binary BLK: format 1.1");
        return false;
      }
      else if (hdr[0] == _MAKE4C('SB') && hdr[2] == _MAKE4C('blk'))
      {
        issue_error_load_failed(fname, "obsolete text BLK in stream");
        return false;
      }

      // if it is binary file
      if (hdr[0] == _MAKE4C('BBF')) // parse old BBF-3 format
      {
        int end = crd.tell() + hdr[2];
        if ((hdr[1] & 0xFFFF) != 0x0003)
        {
          issue_error_load_failed_ver(fname, 3, hdr[1] & 0xFFFF);
          return false;
        }

        valid = doLoadFromStreamBBF3(crd) && crd.tell() == end;
      }
      else if (hdr[0] == _MAKE4C('BBz')) // load old compressed BBF-3 format
      {
        Tab<char> buf(tmpmem);
        buf.resize(hdr[1]);

        {
          ZlibLoadCB zlib_crd(crd, hdr[2]);
          zlib_crd.read(buf.data(), data_size(buf));
        }

        InPlaceMemLoadCB mcrd(buf.data(), data_size(buf));
        return loadFromStream(mcrd, fname, fnotify, data_size(buf));
      }
      else if (shared->blkBinOnlyLoad())
        return false;
      else
      {
        // try to load stream as text file
        static constexpr int BUF_SZ = 16 << 10;
        char buf[BUF_SZ];
        Tab<char> text(tmpmem);
        text.reserve(sizeof(hdr) + hint_size);

        text.resize(sizeof(hdr));
        mem_copy_from(text, hdr);
        if (void *pzero = memchr(text.data(), 0, text.size()))
          text.resize(intptr_t(pzero) - intptr_t(text.data()));
        else
          for (;;)
          {
            int read = crd.tryRead(buf, BUF_SZ);
            if (void *pzero = memchr(buf, 0, read))
              read = intptr_t(pzero) - intptr_t(buf);
            append_items(text, read, buf);
            if (read < BUF_SZ)
              break;
          }

        if (!is_patch(fname))
        {
          reset();
          if (fname)
            shared->setSrc(fname);
#if DAGOR_DBGLEVEL > 0
          else
            shared->setSrc(String(0, "BLK\n%.*s", min(text.size(), 512u), text.size() ? text.data() : ""));
#endif
        }

        bool ret = parse_from_text(*this, text, fname, shared->blkRobustLoad(), fnotify);
        shared->setBlkValid(ret);
        return ret;
      }
    }

    if (fname && valid)
      shared->setSrc(fname);
    if (valid)
    {
      shared->setBlkValid(true);
      compact();
      return true;
    }
  }
  DAGOR_CATCH(DagorException) { reset(); }
  dblk::release_vromfs_blk_ddict(ddict);           // release ddict refcount
  dblk::release_vromfs_shared_name_map(shared_nm); // release shared_nm refcount

  auto restoreShared = [this](unsigned f) { // to cheat PVS since we don't need to check shared anywhere earlier
    if (!this->shared)
    {
      this->shared = new DataBlockShared;
      this->shared->blkFlags = f;
    }
  };
  restoreShared(blkFlags);
  shared->setBlkValid(false);
  issue_error_load_failed(fname, nullptr);
  return false;
}

void DataBlock::setIncludeResolver(IIncludeFileResolver *f_resolver) { fresolve = f_resolver; }

void DataBlock::setRootIncludeResolver(const char *root)
{
  gen_root_inc_resv.root = root && root[0] ? root : ".";
  setIncludeResolver(&gen_root_inc_resv);
}

bool DataBlock::resolveIncludePath(String &inout_fname) { return fresolve->resolveIncludeFile(inout_fname); }

#define EXPORT_PULL dll_pull_iosys_datablock_parser
#include <supp/exportPull.h>
