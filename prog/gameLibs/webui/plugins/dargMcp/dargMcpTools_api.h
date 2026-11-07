// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <json/json.h>
#include <EASTL/string.h>
#include <generic/dag_span.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <math.h>
#include <string.h>

namespace webui
{
namespace mcp
{
static constexpr int JSONRPC_INVALID_PARAMS = -32602;


inline void set_error(Json::Value &response, int code, const eastl::string &message)
{
  response["error"]["code"] = code;
  response["error"]["message"] = message;
}

inline void set_text_result(Json::Value &response, const eastl::string &text)
{
  response["result"]["content"][0]["type"] = "text";
  response["result"]["content"][0]["text"] = text;
}

inline void set_schema(Json::Value &tool, const char *description)
{
  tool["description"] = description;
  tool["inputSchema"]["type"] = "object";
  tool["inputSchema"]["properties"] = Json::Value(Json::objectValue);
}

// HumanInput::DKEY_* code of a key name: a-z, 0-9, enter, tab, escape/esc, backspace, delete, space, up, down, left,
// right, home, end, pageup, pagedown, insert, f1-f12, ctrl/lctrl/rctrl, shift/lshift/rshift, alt/lalt/ralt; -1 if unknown
inline int resolve_key_name(const char *name)
{
  if (!name || !*name)
    return -1;

  // Single character: letter or digit
  if (name[1] == '\0')
  {
    char c = name[0];
    if (c >= 'a' && c <= 'z')
    {
      static const int letter_keys[] = {HumanInput::DKEY_A, HumanInput::DKEY_B, HumanInput::DKEY_C, HumanInput::DKEY_D,
        HumanInput::DKEY_E, HumanInput::DKEY_F, HumanInput::DKEY_G, HumanInput::DKEY_H, HumanInput::DKEY_I, HumanInput::DKEY_J,
        HumanInput::DKEY_K, HumanInput::DKEY_L, HumanInput::DKEY_M, HumanInput::DKEY_N, HumanInput::DKEY_O, HumanInput::DKEY_P,
        HumanInput::DKEY_Q, HumanInput::DKEY_R, HumanInput::DKEY_S, HumanInput::DKEY_T, HumanInput::DKEY_U, HumanInput::DKEY_V,
        HumanInput::DKEY_W, HumanInput::DKEY_X, HumanInput::DKEY_Y, HumanInput::DKEY_Z};
      return letter_keys[c - 'a'];
    }
    if (c >= '0' && c <= '9')
    {
      if (c == '0')
        return HumanInput::DKEY_0;
      return HumanInput::DKEY_1 + (c - '1');
    }
  }

  struct KeyMapping
  {
    const char *name;
    int key;
  };
  static const KeyMapping mappings[] = {
    {"enter", HumanInput::DKEY_RETURN},
    {"tab", HumanInput::DKEY_TAB},
    {"escape", HumanInput::DKEY_ESCAPE},
    {"esc", HumanInput::DKEY_ESCAPE},
    {"backspace", HumanInput::DKEY_BACK},
    {"delete", HumanInput::DKEY_DELETE},
    {"space", HumanInput::DKEY_SPACE},
    {"up", HumanInput::DKEY_UP},
    {"down", HumanInput::DKEY_DOWN},
    {"left", HumanInput::DKEY_LEFT},
    {"right", HumanInput::DKEY_RIGHT},
    {"home", HumanInput::DKEY_HOME},
    {"end", HumanInput::DKEY_END},
    {"pageup", HumanInput::DKEY_PRIOR},
    {"pagedown", HumanInput::DKEY_NEXT},
    {"insert", HumanInput::DKEY_INSERT},
    {"f1", HumanInput::DKEY_F1},
    {"f2", HumanInput::DKEY_F2},
    {"f3", HumanInput::DKEY_F3},
    {"f4", HumanInput::DKEY_F4},
    {"f5", HumanInput::DKEY_F5},
    {"f6", HumanInput::DKEY_F6},
    {"f7", HumanInput::DKEY_F7},
    {"f8", HumanInput::DKEY_F8},
    {"f9", HumanInput::DKEY_F9},
    {"f10", HumanInput::DKEY_F10},
    {"f11", HumanInput::DKEY_F11},
    {"f12", HumanInput::DKEY_F12},
    {"ctrl", HumanInput::DKEY_LCONTROL},
    {"lctrl", HumanInput::DKEY_LCONTROL},
    {"rctrl", HumanInput::DKEY_RCONTROL},
    {"shift", HumanInput::DKEY_LSHIFT},
    {"lshift", HumanInput::DKEY_LSHIFT},
    {"rshift", HumanInput::DKEY_RSHIFT},
    {"alt", HumanInput::DKEY_LALT},
    {"lalt", HumanInput::DKEY_LALT},
    {"ralt", HumanInput::DKEY_RALT},
  };

  for (const auto &m : mappings)
    if (strcmp(name, m.name) == 0)
      return m.key;

  return -1;
}

inline void add_param(Json::Value &tool, const char *name, const char *type, const char *description, bool required = false)
{
  Json::Value &param = tool["inputSchema"]["properties"][name];
  param["type"] = type;
  param["description"] = description;
  if (required)
    tool["inputSchema"]["required"].append(name);
}


// jsoncpp asserts on a type mismatch (asString on a number, asInt on a string),
// so tool arguments are read only through these checked accessors. The first
// bad field records a JSON-RPC "Invalid params" error and clears ok; a tool
// reads all its fields, then checks ok before it acts.
struct ToolArgs
{
  const Json::Value &args; // object or null (isObject() accepts both); operator[] asserts on any other type
  Json::Value &response;
  eastl::string path; // prefix of field names in messages, e.g. "actions[2]."
  bool ok = true;

  ToolArgs(const Json::Value &args_, Json::Value &response_, const char *path_ = "") : args(args_), response(response_), path(path_) {}

  void fail(const char *name, const char *problem)
  {
    if (ok)
      set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: '" + path + name + "' " + problem);
    ok = false;
  }

  const Json::Value *lookup(const char *name, bool required)
  {
    const Json::Value &v = args[name];
    if (!v.isNull())
      return &v;
    if (required)
      fail(name, "is required");
    return nullptr;
  }

  int readInt(const char *name, int def, bool required)
  {
    const Json::Value *v = lookup(name, required);
    if (!v)
      return def;
    if (!v->isInt() && !v->isUInt() && !v->isDouble())
    {
      fail(name, "must be an integer");
      return def;
    }
    // a real with no fractional part, such as the 50.0 a float computation serializes to, is an integer
    double d = v->asDouble();
    if (d != floor(d))
    {
      fail(name, "must be an integer");
      return def;
    }
    // asInt silently wraps an int64 or uint64 value; the double comparison rejects it
    if (d < Json::Value::minInt || d > Json::Value::maxInt)
    {
      fail(name, "is out of the 32-bit integer range");
      return def;
    }
    return int(d);
  }
  int getInt(const char *name, int def) { return readInt(name, def, false); }
  int requireInt(const char *name) { return readInt(name, 0, true); }

  bool getBool(const char *name, bool def)
  {
    const Json::Value *v = lookup(name, false);
    if (!v)
      return def;
    if (!v->isBool())
    {
      fail(name, "must be a boolean");
      return def;
    }
    return v->asBool();
  }

  eastl::string readString(const char *name, const char *def, bool required)
  {
    const Json::Value *v = lookup(name, required);
    if (!v)
      return def;
    if (!v->isString())
    {
      fail(name, "must be a string");
      return def;
    }
    return v->asString();
  }
  eastl::string getString(const char *name, const char *def) { return readString(name, def, false); }
  eastl::string requireString(const char *name) { return readString(name, "", true); }

  const Json::Value *requireArray(const char *name)
  {
    const Json::Value *v = lookup(name, true);
    if (v && !v->isArray())
    {
      fail(name, "must be an array");
      return nullptr;
    }
    return v;
  }
};


struct Tool
{
  const char *name;
  void (*describe)(Json::Value &tool); // sets "description" and "inputSchema"
  void (*call)(ToolArgs &args);        // writes args.response
};
} // namespace mcp

// keeps the span: tools must outlive the server
void set_darg_mcp_game_tools(dag::ConstSpan<mcp::Tool> tools);
} // namespace webui
