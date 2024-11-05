// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "direct_json.h"
#include <debug/dag_debug.h>

#define assert G_ASSERT //-V1059

#include <squirrel.h>
#include <limits>
#include <EASTL/vector.h>
#include <EASTL/sort.h>

#define ZSTD_STATIC_LINKING_ONLY  1
#define ZDICT_STATIC_LINKING_ONLY 1
#include <zstd.h>

typedef bool (*UpdateJsonStreamCB)(const char **start, const char **end, const char **pos, const char **triggerPos, void *user_data);

struct DirectJsonParser
{
  static constexpr int MAX_DEPTH = 200;

  const char *start;
  const char *end;
  const char *pos;
  const char *triggerPos;
  int depth;
  bool endOfInput;
  eastl::string errorString;
  const char *errorPos;
  HSQUIRRELVM vm;
  eastl::string_view token;
  eastl::string unescapeBuf;

  UpdateJsonStreamCB updateStreamCB;
  void *userData;

  DirectJsonParser(const char *start, const char *end, HSQUIRRELVM vm) :
    start(start),
    end(end),
    vm(vm),
    pos(start),
    endOfInput(false),
    depth(0),
    errorPos(nullptr),
    triggerPos(end),
    updateStreamCB(nullptr),
    userData(nullptr)
  {
    if (end - start >= 3 && hasUtf8Bom(start))
    {
      start += 3;
      pos += 3;
    }
  }

  DirectJsonParser(UpdateJsonStreamCB update_stream_cb, void *user_data, HSQUIRRELVM vm) :
    start(nullptr),
    end(nullptr),
    vm(vm),
    pos(nullptr),
    endOfInput(false),
    depth(0),
    errorPos(nullptr),
    triggerPos(nullptr),
    updateStreamCB(update_stream_cb),
    userData(user_data)
  {
    if (!updateStreamCB(&start, &end, &pos, &triggerPos, userData))
    {
      error("Failed to read JSON stream");
      return;
    }

    if (end - start >= 3 && hasUtf8Bom(start))
    {
      start += 3;
      pos += 3;
    }
  }

  bool hasUtf8Bom(const char *ptr) { return (uint8_t)ptr[0] == 0xEF && (uint8_t)ptr[1] == 0xBB && (uint8_t)ptr[2] == 0xBF; }

  char checkHex(char c)
  {
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
      return c;
    error("Invalid hex char");
    return '*';
  }

  void unescapeString(eastl::string_view &v)
  {
    bool isBasicString = true;
    for (size_t i = 1; i < v.size() - 1; ++i)
      if (v[i] == '\\' || uint8_t(v[i]) < 32)
      {
        isBasicString = false;
        break;
      }

    if (isBasicString)
      v = v.substr(1, v.size() - 2);
    else
    {
      unescapeBuf.clear();
      unescapeBuf.reserve(v.size() + 16);
      for (size_t i = 1; i < v.size() - 1; ++i)
      {
        if (v[i] == '\\' && i + 1 < v.size())
        {
          ++i;
          switch (v[i])
          {
            case 'b': unescapeBuf.push_back('\b'); break;
            case 'f': unescapeBuf.push_back('\f'); break;
            case 'n': unescapeBuf.push_back('\n'); break;
            case 'r': unescapeBuf.push_back('\r'); break;
            case 't': unescapeBuf.push_back('\t'); break;
            case 'v': unescapeBuf.push_back('\v'); break;
            case 'u':
              if (i + 4 < v.size())
              {
                char buf[5];
                buf[0] = checkHex(v[i + 1]);
                buf[1] = checkHex(v[i + 2]);
                buf[2] = checkHex(v[i + 3]);
                buf[3] = checkHex(v[i + 4]);
                buf[4] = 0;
                unsigned int c = strtoul(buf, nullptr, 16);
                if (c < 0x80)
                {
                  if (c > 0)
                    unescapeBuf.push_back((char)c);
                }
                else if (c < 0x800)
                {
                  unescapeBuf.push_back((char)(0xC0 | (c >> 6)));
                  unescapeBuf.push_back((char)(0x80 | (c & 0x3F)));
                }
                else // c <= 0xFFFF
                {
                  unescapeBuf.push_back((char)(0xE0 | (c >> 12)));
                  unescapeBuf.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
                  unescapeBuf.push_back((char)(0x80 | (c & 0x3F)));
                }

                i += 4;
              }
              else
                error("Incomplete surrogate, expected 4 hex digits after '\\u'");
              break;
            default:
              if (v[i] != 0)
                unescapeBuf.push_back(v[i]);
              break;
          }
        }
        else
        {
          if (uint8_t(v[i]) >= 32 || v[i] == '\t')
            unescapeBuf.push_back(v[i]);
        }
      }

      v = eastl::string_view(unescapeBuf.data(), unescapeBuf.size());
    }
  }

  void error(const char *message)
  {
    if (errorPos == nullptr)
    {
      if (updateStreamCB)
      {
        errorPos = pos;
        errorString =
          eastl::string(eastl::string::CtorSprintf(), "JSON parse error at position %d: %s", int(errorPos - start), message);

        return;
      }

      errorPos = eastl::max(pos - 1, start);
      int line = 1;
      int col = 1;
      for (const char *p = start; p < errorPos; ++p, ++col)
        if (*p == '\n')
        {
          ++line;
          col = 1;
        }

      errorString = eastl::string(eastl::string::CtorSprintf(), "JSON parse error at line %d, col %d: %s", line, col, message);
    }
  }

  bool isspace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

  bool isdigit(char c) { return c >= '0' && c <= '9'; }

  bool isalnum(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

  void nextToken()
  {
    while (pos < triggerPos && isspace(*pos))
      ++pos;

    if (pos >= triggerPos)
    {
      if (pos >= end || !updateStreamCB)
      {
        endOfInput = true;
        token = eastl::string_view();
      }
      else
      {
        if (!updateStreamCB(&start, &end, &pos, &triggerPos, userData))
        {
          endOfInput = true;
          token = eastl::string_view();
          error("Failed to read JSON stream");
          return;
        }

        while (pos < triggerPos && isspace(*pos))
          ++pos;

        if (pos >= triggerPos)
        {
          endOfInput = true;
          token = eastl::string_view();
          error("Too long span without progress in JSON parsing");
          return;
        }
      }
    }

    // skip comments
    bool repeatSkipComment = true;
    while (*pos == '/' && pos + 1 < end && repeatSkipComment)
    {
      repeatSkipComment = false;
      if (pos[1] == '/')
      {
        repeatSkipComment = true;
        while (pos < end && *pos != '\r' && *pos != '\n')
          ++pos;
      }
      else if (pos[1] == '*')
      {
        repeatSkipComment = true;
        pos += 2;
        while (pos < end && (pos[-1] != '*' || pos[0] != '/'))
          ++pos;
        if (pos < end)
          ++pos;
        else
          error("Unterminated comment");
      }

      while (pos < end && isspace(*pos))
        ++pos;

      if (pos >= end)
      {
        endOfInput = true;
        token = eastl::string_view();
      }
    }

    if (*pos == '"')
    {
      const char *tokenStart = pos++;
      while (pos < end && *pos != '"')
      {
        if (*pos == '\\')
          ++pos;
        ++pos;
      }
      if (pos >= end)
      {
        endOfInput = true;
        token = eastl::string_view();
        error("Unterminated string");
      }
      ++pos;
      token = eastl::string_view(tokenStart, pos - tokenStart);
    }
    else if (*pos == '{' || *pos == '}' || *pos == '[' || *pos == ']' || *pos == ':' || *pos == ',')
    {
      token = eastl::string_view(pos++, 1);
    }
    else if ((*pos >= '0' && *pos <= '9') || *pos == '-' || *pos == '+' || *pos == '.')
    {
      const char *tokenStart = pos;
      while (pos < end && ((*pos >= '0' && *pos <= '9') || *pos == '-' || *pos == '+' || *pos == '.' || *pos == 'e' || *pos == 'E'))
        ++pos;
      token = eastl::string_view(tokenStart, pos - tokenStart);
    }
    else
    {
      const char *tokenStart = pos;
      while (pos < end && (isalnum(*pos) || *pos == '-' || *pos == '+' || *pos == '.'))
        ++pos;
      token = eastl::string_view(tokenStart, pos - tokenStart);
    }
  }

  bool eq(const char *str)
  {
    return token.size() > 0 && token[0] == str[0] && token.size() == strlen(str) && memcmp(token.data(), str, token.size()) == 0;
  }

  bool eq(char c) { return token.size() == 1 && token[0] == c; }

  bool parseArrayContent()
  {
    nextToken();
    if (endOfInput)
    {
      error("Unexpected end of JSON");
      return false;
    }

    if (eq(']'))
      return true;

    while (true)
    {
      if (!parseValue()) // value
      {
        sq_pop(vm, 1);
        return false;
      }

      // append value to array
      sq_arrayappend(vm, -2);

      nextToken();
      if (endOfInput)
      {
        error("Unexpected end of JSON");
        return false;
      }

      if (eq(']'))
        return true;

      if (!eq(','))
      {
        error("Expected ','");
        return false;
      }

      nextToken();
      if (eq(']')) // allow trailing commas
        return true;
    }
  }

  bool parseTableContent()
  {
    nextToken();
    if (endOfInput)
    {
      error("Unexpected end of JSON");
      return false;
    }

    if (eq('}'))
      return true;

    while (true)
    {
      if (!token.size() || token[0] != '"')
      {
        error("Expected '\"'");
        return false;
      }

      unescapeString(token);
      sq_pushstring(vm, token.data(), SQInteger(token.size())); // key

      nextToken();
      if (endOfInput)
      {
        error("Unexpected end of JSON");
        sq_pop(vm, 1);
        return false;
      }

      if (!eq(':'))
      {
        error("Expected ':'");
        sq_pop(vm, 1);
        return false;
      }

      nextToken();
      if (!parseValue()) // value
      {
        sq_pop(vm, 2);
        return false;
      }

      sq_rawset(vm, -3); // table[key] = value

      nextToken();
      if (endOfInput)
      {
        if (depth == 0)
          return true;
        else
        {
          error("Unexpected end of JSON");
          return false;
        }
      }

      if (eq('}'))
        return true;

      if (!eq(','))
      {
        error("Expected ','");
        return false;
      }

      nextToken();
      if (eq('}')) // allow trailing commas
        return true;
    }
  }

  bool parseValue()
  {
    if (endOfInput)
    {
      error("Unexpected end of JSON");
      sq_pushnull(vm);
      return false;
    }

    if (token.size() && token[0] == '"')
    {
      unescapeString(token);
      sq_pushstring(vm, token.data(), SQInteger(token.size()));
      return true;
    }
    else if (eq('{'))
    {
      depth++;
      if (depth > MAX_DEPTH)
      {
        error("Too many nested objects");
        sq_pushnull(vm);
        return false;
      }

      sq_newtable(vm);
      if (!parseTableContent())
        return false;
      depth--;
      return true;
    }
    else if (eq('['))
    {
      depth++;
      if (depth > MAX_DEPTH)
      {
        error("Too many nested objects");
        sq_pushnull(vm);
        return false;
      }

      sq_newarray(vm, 0);
      if (!parseArrayContent())
        return false;

      // shrink array to fit
      sq_arrayresize(vm, -1, sq_getsize(vm, -1));

      depth--;
      return true;
    }
    else if (eq("true"))
    {
      sq_pushbool(vm, true);
      return true;
    }
    else if (eq("false"))
    {
      sq_pushbool(vm, false);
      return true;
    }
    else if (eq("null"))
    {
      sq_pushnull(vm);
      return true;
    }
    else if (token.size() == 1 && token[0] >= '0' && token[0] <= '9') // 20-40% of all integers
    {
      sq_pushinteger(vm, SQInteger(token[0] - '0'));
      return true;
    }
    else if (token.size())
    {
      bool negative = token[0] == '-';
      if (negative)
      {
        token.remove_prefix(1);
        while (token.size() && token[0] == ' ' || token[0] == '\t')
          token.remove_prefix(1);
      }

      if (token.size() && (token[0] != '.' && !isdigit(token[0])))
      {
        error("Invalid number");
        sq_pushnull(vm);
        return false;
      }

      constexpr uint64_t overflowThreshold = std::numeric_limits<uint64_t>::max() / 10;
      constexpr uint64_t overflowDigit = std::numeric_limits<uint64_t>::max() % 10;

      char *e = nullptr;
      uint64_t uv = 0;
      bool isInteger = true;
      for (char c : token)
      {
        if (c >= '0' && c <= '9')
        {
          uint64_t digit = c - '0';
          if (uv > overflowThreshold || (uv == overflowThreshold && digit > overflowDigit))
          {
            isInteger = false;
            break;
          }

          uv = uv * 10 + digit;
        }
        else
        {
          isInteger = false;
          break;
        }
      }

      if (isInteger)
      {
        // try to fit this into SQInteger
        if (uv <= std::numeric_limits<SQInteger>::max())
        {
          sq_pushinteger(vm, negative ? -SQInteger(uv) : SQInteger(uv));
          return true;
        }
        else if (negative && uv - 1 == uint64_t(-(std::numeric_limits<SQInteger>::min() + 1)))
        {
          sq_pushinteger(vm, std::numeric_limits<SQInteger>::min());
          return true;
        }
      }

      double dv = strtod(token.data(), &e);
      if (e == token.data() + token.size())
      {
        sq_pushfloat(vm, negative ? -dv : dv);
        return true;
      }
      else
      {
        error("Invalid number");
        sq_pushnull(vm);
        return false;
      }
    }
    else
    {
      error("Empty token");
      sq_pushnull(vm);
      return false;
    }
  }

  bool parse() // parsed value is left on the stack
  {
    int prevTop = sq_gettop(vm);
    nextToken();
    if (parseValue())
    {
      nextToken();
      if (token.size())
        error("Garbage after end of JSON");
    }
    G_ASSERT(sq_gettop(vm) == prevTop + 1);
    G_UNUSED(prevTop);
    return errorString.empty();
  }
};

bool parse_json_directly_to_quirrel(HSQUIRRELVM vm, const char *start, const char *end, eastl::string &error_string)
{
  DirectJsonParser parser(start, end, vm);
  if (!parser.parse())
  {
    error_string = parser.errorString;
    return false;
  }
  return true;
}


#define ZSTD_STREAM_BUFFER_SIZE 128000

struct ZstdStreamData
{
  ZSTD_DStream *dstream;
  ZSTD_inBuffer in;
  ZSTD_outBuffer out;
  eastl::vector<char> streamBuffer;
  eastl::string *errorString;

  ZstdStreamData() : dstream(ZSTD_createDStream()), errorString(nullptr), in({nullptr, 0, 0}), out({nullptr, 0, 0}) {}
  ~ZstdStreamData() { ZSTD_freeDStream(dstream); }
};

bool update_stream_cb(const char **start, const char **end, const char **pos, const char **triggerPos, void *user_data)
{
  ZstdStreamData *streamData = (ZstdStreamData *)user_data;
  if (!*pos)
  {
    *start = streamData->streamBuffer.data();
    *end = *start + ZSTD_STREAM_BUFFER_SIZE * 2;
    *pos = *start;
    *triggerPos = *start + ZSTD_STREAM_BUFFER_SIZE;
    return true;
  }

  memmove(streamData->streamBuffer.data(), streamData->streamBuffer.data() + ZSTD_STREAM_BUFFER_SIZE, ZSTD_STREAM_BUFFER_SIZE);
  streamData->out = {streamData->streamBuffer.data() + ZSTD_STREAM_BUFFER_SIZE, ZSTD_STREAM_BUFFER_SIZE, 0};
  size_t ret = ZSTD_decompressStream(streamData->dstream, &streamData->out, &streamData->in);
  if (ZSTD_isError(ret))
  {
    streamData->errorString->assign("ZSTD decompression error");
    return false;
  }

  *start -= ZSTD_STREAM_BUFFER_SIZE;
  *end = streamData->streamBuffer.data() + ZSTD_STREAM_BUFFER_SIZE * 2;
  *pos -= ZSTD_STREAM_BUFFER_SIZE;
  *triggerPos = streamData->streamBuffer.data() + ZSTD_STREAM_BUFFER_SIZE;
  if (ret == 0)
  {
    *end = streamData->streamBuffer.data() + ZSTD_STREAM_BUFFER_SIZE + streamData->out.pos;
    *triggerPos = *end;
    return true;
  }

  return true;
}

bool parse_json_zstd_stream_directly_to_quirrel(HSQUIRRELVM vm, void *zstd_stream_data, size_t zstd_stream_size,
  eastl::string &error_string)
{
  ZstdStreamData streamData;
  streamData.streamBuffer.resize(ZSTD_STREAM_BUFFER_SIZE * 2 + 16);
  streamData.errorString = &error_string;
  streamData.in = {zstd_stream_data, zstd_stream_size, 0};
  streamData.out = {streamData.streamBuffer.data(), ZSTD_STREAM_BUFFER_SIZE * 2, 0};

  size_t ret = ZSTD_decompressStream(streamData.dstream, &streamData.out, &streamData.in);
  if (ZSTD_isError(ret))
  {
    error_string = "ZSTD decompression error";
    return false;
  }

  if (streamData.out.pos == 0)
  {
    error_string = "ZSTD decompression error: no data";
    return false;
  }

  if (ret == 0)
  {
    bool res = parse_json_directly_to_quirrel(vm, streamData.streamBuffer.data(), streamData.streamBuffer.data() + streamData.out.pos,
      error_string);
    return res;
  }

  DirectJsonParser parser(update_stream_cb, (void *)&streamData, vm);
  if (!parser.parse())
  {
    error_string = parser.errorString;
    return false;
  }

  return true;
}
