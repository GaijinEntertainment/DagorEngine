#ifndef _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_JSONWRITER_H_
#define _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_JSONWRITER_H_
#pragma once


#include "rapidjsonConfig.h"
#include <rapidjson/writer.h>
#include <daScript/daScript.h>

#include <EASTL/type_traits.h>

class RapidJsonWriter
{
  typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<char>> TStringBuffer;
  typedef rapidjson::Writer<TStringBuffer, rapidjson::UTF8<char>, rapidjson::UTF8<char>> TJsonWriter;

public:
  RapidJsonWriter(das::Context * context_)
    : writer(buf)
    , context(context_)
  {
  }

#if DAS_RAPIDJSON_ENABLE_EXCEPTIONS
  template <typename F, typename ... Args>
  bool wrap(F && f, Args && ... args)
  {
    bool r = false;
    if (shouldIgnoreExceptions)
    {
      r = (writer.*f)(das::forward<Args>(args)...);
    }
    else
    {
      try
      {
        r = (writer.*f)(das::forward<Args>(args)...);
      }
      catch (das::exception & e)
      {
        setLastError(e.what());
        r = false;
      }
      catch (...)
      {
        setLastError("Unknown exception caught");
        r = false;
      }
    }
    return r;
  }
#else
  template <typename F, typename ... Args>
  bool wrap(F && f, Args && ... args)
  {
    return (writer.*f)(das::forward<Args>(args)...);
  }
#endif

  void setLastError(const das::string & str)
  {
    lastError = str;
    if (!str.empty())
    {
      if (context)
      {
        das::string callstack = context->getStackWalk(nullptr, false, false);
        DAS_FATAL_LOG("Improper use of JsonWriter: %s\n%s", str.c_str(), callstack.c_str());
      }
      else
      {
        DAS_FATAL_LOG("Improper use of JsonWriter: %s", str.c_str());
      }
    }
  }

  template <typename F, typename ... Args>
  bool das_ctx_wrap(das::Context & ctx, F && f, Args && ... args)
  {
    return (this->*f)(das::forward<Args>(args) ...);
  }

  void reset()
  {
    buf.Clear();
    writer.Reset(buf);
    lastError.clear();
  }

  bool startObj()
  {
    return wrap(&TJsonWriter::StartObject);
  }

  bool endObj()
  {
    return wrap(&TJsonWriter::EndObject, 0);
  }

  bool startArray()
  {
    return wrap(&TJsonWriter::StartArray);
  }

  bool endArray()
  {
    return wrap(&TJsonWriter::EndArray, 0);
  }

  bool key(const char * value)
  {
    // Key has two overloads, have to select one explicitly
    return wrap(static_cast<bool (TJsonWriter::*)(const char *)>(&TJsonWriter::Key),
                value ? value : "");
  }

  bool null()
  {
    return wrap(&TJsonWriter::Null);
  }

  bool append(bool value)
  {
    return wrap(&TJsonWriter::Bool, value);
  }

  bool append(int32_t value)
  {
    return wrap(&TJsonWriter::Int, value);
  }

  bool append(uint32_t value)
  {
    return wrap(&TJsonWriter::Uint, value);
  }

  bool append(int64_t value)
  {
    return wrap(&TJsonWriter::Int64, value);
  }

  bool append(uint64_t value)
  {
    return wrap(&TJsonWriter::Uint64, value);
  }

  bool appendPtr(intptr_t value)
  {
    using ValueType = typename eastl::conditional_t<sizeof(intptr_t) == sizeof(int32_t), int32_t, int64_t>;
    G_STATIC_ASSERT(sizeof(ValueType) == sizeof(intptr_t));
    return append(static_cast<ValueType>(value));
  }

  bool append(float value)
  {
    return wrap(&TJsonWriter::Double, value);
  }

  bool append(double value)
  {
    return wrap(&TJsonWriter::Double, value);
  }

  bool append(const char * value)
  {
    // String has two overloads, have to select one explicitly
    return wrap(static_cast<bool (TJsonWriter::*)(const char *)>(&TJsonWriter::String),
                value ? value : "");
  }

  bool append(char * value)
  {
    return append((const char *)value);
  }

  const char * result()
  {
    buf.Flush();
    return buf.GetString();
  }

  size_t result_size()
  {
    buf.Flush();
    return buf.GetSize();
  }

  char * das_result()
  {
    G_ASSERT(context);
    return context->allocateString(result(), /*at*/nullptr);
  }

  char * das_getLastError() const
  {
    G_ASSERT(context);
    return context->allocateString(lastError.c_str(), /*at*/nullptr);
  }

  const das::string & getLastError() const
  {
    return lastError;
  }

  void ignoreExceptions(bool value)
  {
    shouldIgnoreExceptions = value;
  }

private:
  TStringBuffer buf;
  TJsonWriter writer;
  das::string lastError;
  das::Context * context = nullptr;
  bool shouldIgnoreExceptions = false;
};

MAKE_TYPE_FACTORY(JsonWriter, RapidJsonWriter)

#endif // _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_JSONWRITER_H_
