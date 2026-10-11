// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/dargMcpPlugin.h>
#include <webui/helpers.h>
#include <debug/dag_debug.h>
#include <json/json.h>
#include <image/dag_png.h>
#include <image/dag_texPixel.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_d3dResource.h>
#include <util/dag_base64.h>
#include <squirrel.h>
#include <daRg/dag_guiScene.h>
#include <daRg/dag_inputIds.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiMouseIds.h>
#include <quirrel/sqPrintCollector.h>
#include <math.h>

using namespace webui;
using namespace darg;


static darg::IGuiScene *(*scene_provider)() = nullptr;

static darg::IGuiScene *get_scene() { return scene_provider ? scene_provider() : nullptr; }

void webui::set_darg_mcp_scene_provider(darg::IGuiScene *(*provider)()) { scene_provider = provider; }


static const int JSONRPC_INVALID_REQUEST = -32600;
static const int JSONRPC_METHOD_NOT_FOUND = -32601;
static const int JSONRPC_INVALID_PARAMS = -32602;
static const int JSONRPC_PARSE_ERROR = -32700;


static void set_error(Json::Value &response, int code, const eastl::string &message)
{
  response["error"]["code"] = code;
  response["error"]["message"] = message;
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


static int resolve_mouse_button(const char *name)
{
  if (!name || !*name || strcmp(name, "left") == 0)
    return HumanInput::DBUTTON_LEFT;
  if (strcmp(name, "right") == 0)
    return HumanInput::DBUTTON_RIGHT;
  if (strcmp(name, "middle") == 0)
    return HumanInput::DBUTTON_MIDDLE;
  return -1;
}


static int resolve_key_name(const char *name)
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


static void handle_initialize(const Json::Value & /*request*/, Json::Value &response)
{
  Json::Value result;
  result["protocolVersion"] = "2024-11-05";

  Json::Value capabilities;
  capabilities["tools"] = Json::Value(Json::objectValue);
  capabilities["resources"] = Json::Value(Json::objectValue);

  Json::Value serverInfo;
  serverInfo["name"] = "daRg server";
  serverInfo["version"] = "0.0.3";

  result["capabilities"] = capabilities;
  result["serverInfo"] = serverInfo;

  response["result"] = result;
}


static void handle_tools_list(Json::Value &response)
{
  Json::Value tools(Json::arrayValue);

  {
    Json::Value getErrorTool;
    getErrorTool["name"] = "get_last_error";
    getErrorTool["description"] = "Returns the last error message and script call stack, or \"[NO_ERROR]\" string";
    getErrorTool["inputSchema"]["type"] = "object";
    getErrorTool["inputSchema"]["properties"] = Json::Value(Json::objectValue);
    tools.append(getErrorTool);
  }

  {
    Json::Value runScriptTool;
    runScriptTool["name"] = "run_ui_script";
    runScriptTool["description"] = "Opens and executes the given UI script in the application or reloads if it is being run now";
    runScriptTool["inputSchema"]["type"] = "object";
    runScriptTool["inputSchema"]["properties"]["filename"]["type"] = "string";
    runScriptTool["inputSchema"]["properties"]["filename"]["description"] = "Path to the UI script file to execute";
    runScriptTool["inputSchema"]["required"] = Json::Value(Json::arrayValue);
    runScriptTool["inputSchema"]["required"].append("filename");
    tools.append(runScriptTool);
  }

  {
    Json::Value screenshotTool;
    screenshotTool["name"] = "make_screenshot";
    screenshotTool["description"] =
      "Captures a screenshot and returns it as a base64-encoded PNG image. "
      "By default also returns the UI element tree with exact screen bounding boxes [x,y wxh] for every element. "
      "Use these bounding boxes to compute click positions for mouse_click.";
    screenshotTool["inputSchema"]["type"] = "object";
    screenshotTool["inputSchema"]["properties"]["scene_tree"]["type"] = "boolean";
    screenshotTool["inputSchema"]["properties"]["scene_tree"]["description"] =
      "Return the UI element tree with exact coordinates. Default: true. Set to false to save bandwidth if not needed.";
    screenshotTool["inputSchema"]["properties"]["scene_tree_max_depth"]["type"] = "integer";
    screenshotTool["inputSchema"]["properties"]["scene_tree_max_depth"]["description"] = "Max tree depth (default: 50)";
    screenshotTool["inputSchema"]["properties"]["scene_tree_max_elements"]["type"] = "integer";
    screenshotTool["inputSchema"]["properties"]["scene_tree_max_elements"]["description"] =
      "Max number of elements to report (default: 2000)";
    screenshotTool["inputSchema"]["properties"]["scene_tree_filter"]["type"] = "string";
    screenshotTool["inputSchema"]["properties"]["scene_tree_filter"]["description"] =
      "Only show branches containing elements with this text (case-insensitive)";
    tools.append(screenshotTool);
  }

  {
    Json::Value checkSyntaxTool;
    checkSyntaxTool["name"] = "check_quirrel_syntax";
    checkSyntaxTool["description"] =
      "Compiles the provided string as a Quirrel script, returns error message if any or empty string in case of success.\n"
      "To be used only for basic language syntax verification. No libraries available to use here.";
    checkSyntaxTool["inputSchema"]["type"] = "object";
    checkSyntaxTool["inputSchema"]["properties"]["source"]["type"] = "string";
    checkSyntaxTool["inputSchema"]["properties"]["source"]["description"] = "Quirrel script source to check compilation";
    checkSyntaxTool["inputSchema"]["required"] = Json::Value(Json::arrayValue);
    checkSyntaxTool["inputSchema"]["required"].append("source");
    tools.append(checkSyntaxTool);
  }

  {
    Json::Value tool;
    tool["name"] = "mouse_click";
    tool["description"] = "Simulates a mouse click (press + release) at the given coordinates in the game's render resolution space "
                          "(same coordinate space as reported by make_screenshot). "
                          "By default returns the stack of UI elements at the click position so you can verify the click target.";
    tool["inputSchema"]["type"] = "object";
    tool["inputSchema"]["properties"]["x"]["type"] = "integer";
    tool["inputSchema"]["properties"]["x"]["description"] = "X coordinate in render resolution space";
    tool["inputSchema"]["properties"]["y"]["type"] = "integer";
    tool["inputSchema"]["properties"]["y"]["description"] = "Y coordinate in render resolution space";
    tool["inputSchema"]["properties"]["button"]["type"] = "string";
    tool["inputSchema"]["properties"]["button"]["description"] = "Mouse button: \"left\" (default), \"right\", or \"middle\"";
    tool["inputSchema"]["properties"]["return_hit_info"]["type"] = "boolean";
    tool["inputSchema"]["properties"]["return_hit_info"]["description"] =
      "Return the stack of UI elements at the click position. Default: true.";
    tool["inputSchema"]["required"] = Json::Value(Json::arrayValue);
    tool["inputSchema"]["required"].append("x");
    tool["inputSchema"]["required"].append("y");
    tools.append(tool);
  }

  {
    Json::Value tool;
    tool["name"] = "mouse_move";
    tool["description"] = "Moves the mouse pointer to the given coordinates in the game's render resolution space "
                          "(same coordinate space as reported by make_screenshot)";
    tool["inputSchema"]["type"] = "object";
    tool["inputSchema"]["properties"]["x"]["type"] = "integer";
    tool["inputSchema"]["properties"]["x"]["description"] = "X coordinate in render resolution space";
    tool["inputSchema"]["properties"]["y"]["type"] = "integer";
    tool["inputSchema"]["properties"]["y"]["description"] = "Y coordinate in render resolution space";
    tool["inputSchema"]["required"] = Json::Value(Json::arrayValue);
    tool["inputSchema"]["required"].append("x");
    tool["inputSchema"]["required"].append("y");
    tools.append(tool);
  }

  {
    Json::Value tool;
    tool["name"] = "send_keyboard_input";
    tool["description"] = "Sends a sequence of keyboard input actions.\n"
                          "Each action in the array is an object with a \"type\" field:\n"
                          "- {\"type\": \"text\", \"text\": \"Hello\"} — types each character as key press + release\n"
                          "- {\"type\": \"key\", \"key\": \"enter\"} — presses and releases a named key\n"
                          "- {\"type\": \"combo\", \"keys\": [\"ctrl\", \"a\"]} — presses keys in order, releases in reverse\n"
                          "A bad action anywhere in the batch is an Invalid params error and no key of the batch "
                          "is sent, so a corrected call repeats nothing.\n"
                          "Named keys: a-z, 0-9, enter, tab, escape/esc, backspace, delete, space, "
                          "up, down, left, right, home, end, pageup, pagedown, insert, f1-f12, "
                          "ctrl/lctrl/rctrl, shift/lshift/rshift, alt/lalt/ralt";
    tool["inputSchema"]["type"] = "object";
    tool["inputSchema"]["properties"]["actions"]["type"] = "array";
    tool["inputSchema"]["properties"]["actions"]["description"] = "Array of keyboard input actions";
    tool["inputSchema"]["required"] = Json::Value(Json::arrayValue);
    tool["inputSchema"]["required"].append("actions");
    tools.append(tool);
  }

  {
    Json::Value tool;
    tool["name"] = "get_scene_tree";
    tool["description"] = "Returns a compact text representation of the UI element tree.\n"
                          "Each line: [x,y wxh] ROBJ_TYPE \"text\" {Behaviors} [hidden] -- source:line\n"
                          "Indentation encodes depth. Fields are omitted when empty.";
    tool["inputSchema"]["type"] = "object";
    tool["inputSchema"]["properties"]["max_depth"]["type"] = "integer";
    tool["inputSchema"]["properties"]["max_depth"]["description"] = "Max tree depth to report (default: 50)";
    tool["inputSchema"]["properties"]["filter_text"]["type"] = "string";
    tool["inputSchema"]["properties"]["filter_text"]["description"] =
      "Only show branches containing elements with matching text (case-insensitive substring)";
    tool["inputSchema"]["properties"]["include_hidden"]["type"] = "boolean";
    tool["inputSchema"]["properties"]["include_hidden"]["description"] = "Include hidden/clipped-out elements (default: false)";
    tool["inputSchema"]["properties"]["skip_non_visual"]["type"] = "boolean";
    tool["inputSchema"]["properties"]["skip_non_visual"]["description"] =
      "Skip leaf elements with no render object, no text, and no behaviors (default: false)";
    tool["inputSchema"]["properties"]["max_elements"]["type"] = "integer";
    tool["inputSchema"]["properties"]["max_elements"]["description"] = "Max number of elements to report (default: 2000)";
    tools.append(tool);
  }

  {
    Json::Value tool;
    tool["name"] = "find_elements_at";
    tool["description"] = "Hit-tests at a screen position and returns the stack of UI elements at that point (front to back).\n"
                          "Coordinates are in the same render resolution space as make_screenshot/mouse_click.\n"
                          "Each result shows: screen box, render object type, text, behaviors, source location.";
    tool["inputSchema"]["type"] = "object";
    tool["inputSchema"]["properties"]["x"]["type"] = "integer";
    tool["inputSchema"]["properties"]["x"]["description"] = "X coordinate in render resolution space";
    tool["inputSchema"]["properties"]["y"]["type"] = "integer";
    tool["inputSchema"]["properties"]["y"]["description"] = "Y coordinate in render resolution space";
    tool["inputSchema"]["required"] = Json::Value(Json::arrayValue);
    tool["inputSchema"]["required"].append("x");
    tool["inputSchema"]["required"].append("y");
    tools.append(tool);
  }

  {
    Json::Value tool;
    tool["name"] = "find_elements_by_text";
    tool["description"] = "Searches all visible UI elements for text content matching the query.\n"
                          "Returns matching elements with their screen boxes, properties, and ancestor paths.";
    tool["inputSchema"]["type"] = "object";
    tool["inputSchema"]["properties"]["text"]["type"] = "string";
    tool["inputSchema"]["properties"]["text"]["description"] = "Substring to search for in element text content";
    tool["inputSchema"]["properties"]["case_sensitive"]["type"] = "boolean";
    tool["inputSchema"]["properties"]["case_sensitive"]["description"] = "Case-sensitive matching (default: false)";
    tool["inputSchema"]["properties"]["max_results"]["type"] = "integer";
    tool["inputSchema"]["properties"]["max_results"]["description"] = "Max number of results (default: 50)";
    tool["inputSchema"]["required"] = Json::Value(Json::arrayValue);
    tool["inputSchema"]["required"].append("text");
    tools.append(tool);
  }

  response["result"]["tools"] = tools;
}

static void tool_get_last_error(Json::Value &response)
{
  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene || !scene->getScriptVM())
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "Error: no UI scene";
  }
  else
  {
    eastl::string errText;
    bool isError = scene->getErrorText(errText);
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = isError ? errText.c_str() : "[NO_ERROR]";
  }
  response["result"] = result;
}


static void tool_run_ui_script(ToolArgs &args, Json::Value &response)
{
  eastl::string filename = args.requireString("filename");
  if (!args.ok)
    return;

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene || !scene->getScriptVM())
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "Error: no UI scene";
  }
  else
  {
    scene->reloadScript(filename.c_str());
    eastl::string errText;
    bool isError = scene->getErrorText(errText);
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = isError ? errText.c_str() : "Success: script loaded";
  }
  response["result"] = result;
}


static void tool_make_screenshot(ToolArgs &args, Json::Value &response)
{
  bool wantTree = args.getBool("scene_tree", true);
  int treeMaxDepth = args.getInt("scene_tree_max_depth", 50);
  int treeMaxElems = args.getInt("scene_tree_max_elements", 2000);
  eastl::string filterText = args.getString("scene_tree_filter", "");
  if (!args.ok)
    return;

  Json::Value result;
  YAMemSave save;
  int screenW = 0, screenH = 0;

#if _TARGET_PC
  bool success = false;

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

  d3d::get_screen_size(screenW, screenH);

  // Cap max dimension at 1568 to avoid hidden API-side image downscaling
  int imgW = screenW, imgH = screenH;
  const int maxDim = 1568;
  if (imgW > maxDim || imgH > maxDim)
  {
    float s = float(maxDim) / float(imgW > imgH ? imgW : imgH);
    imgW = int(imgW * s);
    imgH = int(imgH * s);
  }

  BaseTexture *rt = d3d::create_tex(nullptr, imgW, imgH, TEXCF_RTARGET, 1, "mcp_screenshot_rt");
  if (rt)
  {
    Texture *backbuf = d3d::get_backbuffer_tex();
    d3d::stretch_rect(backbuf, rt);

    // Read back pixels and encode as PNG
    void *pixPtr = nullptr;
    int stride = 0;
    if (rt->lockimg(&pixPtr, stride, 0, TEXLOCK_READ) && pixPtr)
    {
      success = save_png32((const TexPixel32 *)pixPtr, imgW, imgH, stride, save);
      rt->unlockimg();
    }

    del_d3dres(rt);
  }

  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP, NULL, NULL, NULL);
#else
  const bool success = false;
#endif

  if (success && save.offset)
  {
    Base64 b64Coder;
    b64Coder.encode((const uint8_t *)save.mem, save.offset);

    eastl::string desc = "Screenshot captured. Resolution: " + eastl::to_string(screenW) + "x" + eastl::to_string(screenH) + ".";

    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = desc;
    result["content"][1]["type"] = "image";
    result["content"][1]["data"] = b64Coder.c_str();
    result["content"][1]["mimeType"] = "image/png";

    if (wantTree)
    {
      IGuiScene *scene = get_scene();
      if (scene)
      {
        eastl::string treeOut;
        scene->inspectSceneTree(treeOut, treeMaxDepth, filterText.empty() ? nullptr : filterText.c_str(), false, false, treeMaxElems);
        if (!treeOut.empty())
        {
          result["content"][2]["type"] = "text";
          result["content"][2]["text"] = treeOut.c_str();
        }
      }
    }
  }
  else
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: failed to capture screenshot";
  }

  response["result"] = result;
}


static void tool_check_quirrel_syntax(ToolArgs &args, Json::Value &response)
{
  eastl::string source = args.requireString("source");
  if (!args.ok)
    return;

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene || !scene->getScriptVM())
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no script VM available";
  }
  else
  {
    HSQUIRRELVM vm = scene->getScriptVM();

    SQPrintCollector printCollector(vm);

    if (SQ_SUCCEEDED(sq_compile(vm, source.c_str(), source.length(), "test_script", true, nullptr)))
    {
      sq_pop(vm, 1); // pop closure
      result["content"][0]["type"] = "text";
      result["content"][0]["text"] = "SUCCESS: no errors";
    }
    else
    {
      result["content"][0]["type"] = "text";
      result["content"][0]["text"] = printCollector.output.c_str();
    }
  }
  response["result"] = result;
}


static void tool_mouse_click(ToolArgs &args, Json::Value &response)
{
  int x = args.requireInt("x");
  int y = args.requireInt("y");
  eastl::string buttonName = args.getString("button", "left");
  bool returnHitInfo = args.getBool("return_hit_info", true);
  if (!args.ok)
    return;

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  int btnId = resolve_mouse_button(buttonName.c_str());
  if (btnId < 0)
  {
    args.fail("button", ("must be 'left', 'right' or 'middle', not '" + buttonName + "'").c_str());
    return;
  }

  scene->onMouseEvent(INP_EV_POINTER_MOVE, 0, short(x), short(y), 0);
  scene->onMouseEvent(INP_EV_PRESS, btnId, short(x), short(y), 0);
  scene->onMouseEvent(INP_EV_RELEASE, btnId, short(x), short(y), 0);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = "OK";

  if (returnHitInfo)
  {
    eastl::string hitOut;
    scene->inspectElementsAtPos(hitOut, x, y);
    if (!hitOut.empty())
    {
      result["content"][1]["type"] = "text";
      result["content"][1]["text"] = hitOut.c_str();
    }
  }

  response["result"] = result;
}


static void tool_mouse_move(ToolArgs &args, Json::Value &response)
{
  int x = args.requireInt("x");
  int y = args.requireInt("y");
  if (!args.ok)
    return;

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  scene->onMouseEvent(INP_EV_POINTER_MOVE, 0, short(x), short(y), 0);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = "OK";
  response["result"] = result;
}


static void set_text_result(Json::Value &response, const eastl::string &text)
{
  Json::Value result;
  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = text;
  response["result"] = result;
}

// a "text" action carries its text; a "key" or "combo" action carries the resolved keys
struct KeyboardAction
{
  bool isText = false;
  eastl::string text;
  eastl::vector<int> dkeys;
};

// reads one action; on a bad one the response already carries the error
static bool read_keyboard_action(const Json::Value &actionVal, Json::ArrayIndex i, Json::Value &response, KeyboardAction &out)
{
  eastl::string actionPath = "actions[" + eastl::to_string(i) + "]";
  if (!actionVal.isObject())
  {
    set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: '" + actionPath + "' must be an object");
    return false;
  }
  ToolArgs action(actionVal, response, (actionPath + ".").c_str());
  eastl::string type = action.requireString("type");
  if (!action.ok)
    return false;

  if (type == "text")
  {
    out.isText = true;
    out.text = action.getString("text", "");
    return action.ok;
  }
  if (type == "key")
  {
    eastl::string keyName = action.getString("key", "");
    if (!action.ok)
      return false;
    int dkey = resolve_key_name(keyName.c_str());
    if (dkey < 0)
    {
      action.fail("key", ("is not a known key: '" + keyName + "'").c_str());
      return false;
    }
    out.dkeys.push_back(dkey);
    return true;
  }
  if (type == "combo")
  {
    const Json::Value *keys = action.requireArray("keys");
    if (!keys)
      return false;
    if (keys->empty())
    {
      action.fail("keys", "must not be empty");
      return false;
    }
    out.dkeys.reserve(keys->size());
    for (Json::ArrayIndex k = 0; k < keys->size(); ++k)
    {
      const Json::Value &keyVal = (*keys)[k];
      eastl::string keyField = "keys[" + eastl::to_string(k) + "]";
      if (!keyVal.isString())
      {
        action.fail(keyField.c_str(), "must be a string");
        return false;
      }
      eastl::string keyName = keyVal.asString();
      int dkey = resolve_key_name(keyName.c_str());
      if (dkey < 0)
      {
        action.fail(keyField.c_str(), ("is not a known key: '" + keyName + "'").c_str());
        return false;
      }
      out.dkeys.push_back(dkey);
    }
    return true;
  }
  action.fail("type", ("is not a known action type: '" + type + "'").c_str());
  return false;
}

static void send_text(IGuiScene *scene, const eastl::string &text)
{
  for (const char *p = text.c_str(); *p;)
  {
    // Decode UTF-8 to wchar_t
    wchar_t wc = 0;
    unsigned char c = (unsigned char)*p;
    if (c < 0x80)
    {
      wc = c;
      p += 1;
    }
    else if (c < 0xE0)
    {
      wc = (c & 0x1F) << 6;
      if ((unsigned char)p[1] >= 0x80)
        wc |= ((unsigned char)p[1] & 0x3F);
      p += 2;
    }
    else if (c < 0xF0)
    {
      wc = (c & 0x0F) << 12;
      if ((unsigned char)p[1] >= 0x80)
        wc |= ((unsigned char)p[1] & 0x3F) << 6;
      if ((unsigned char)p[2] >= 0x80)
        wc |= ((unsigned char)p[2] & 0x3F);
      p += 3;
    }
    else
    {
      // Skip 4-byte sequences (outside BMP)
      p += 4;
      continue;
    }
    scene->onKbdEvent(INP_EV_PRESS, 0, 0, false, wc);
    scene->onKbdEvent(INP_EV_RELEASE, 0, 0, false, wc);
  }
}

// every action is read before the first key goes to the scene, so a bad action later in the
// batch leaves the scene untouched and the corrected call does not repeat delivered keys
static void tool_send_keyboard_input(ToolArgs &args, Json::Value &response)
{
  const Json::Value *actions = args.requireArray("actions");
  if (!actions)
    return;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    set_text_result(response, "ERROR: no UI scene");
    return;
  }

  eastl::vector<KeyboardAction> parsed;
  parsed.reserve(actions->size());
  for (Json::ArrayIndex i = 0; i < actions->size(); ++i)
    if (!read_keyboard_action((*actions)[i], i, response, parsed.push_back()))
      return;

  for (const KeyboardAction &action : parsed)
  {
    if (action.isText)
    {
      send_text(scene, action.text);
      continue;
    }
    // a combo presses its keys in order and releases them in reverse; a single key is a combo of one
    for (int dk : action.dkeys)
      scene->onKbdEvent(INP_EV_PRESS, dk, 0, false);
    for (int j = (int)action.dkeys.size() - 1; j >= 0; --j)
      scene->onKbdEvent(INP_EV_RELEASE, action.dkeys[j], 0, false);
  }

  set_text_result(response, "OK");
}


static void tool_get_scene_tree(ToolArgs &args, Json::Value &response)
{
  int maxDepth = args.getInt("max_depth", 50);
  eastl::string filterText = args.getString("filter_text", "");
  bool includeHidden = args.getBool("include_hidden", false);
  bool skipNonVisual = args.getBool("skip_non_visual", false);
  int maxElements = args.getInt("max_elements", 2000);
  if (!args.ok)
    return;

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  eastl::string out;
  scene->inspectSceneTree(out, maxDepth, filterText.empty() ? nullptr : filterText.c_str(), includeHidden, skipNonVisual, maxElements);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = out.empty() ? "Empty scene" : out.c_str();
  response["result"] = result;
}


static void tool_find_elements_at(ToolArgs &args, Json::Value &response)
{
  int x = args.requireInt("x");
  int y = args.requireInt("y");
  if (!args.ok)
    return;

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  eastl::string out;
  scene->inspectElementsAtPos(out, x, y);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = out.c_str();
  response["result"] = result;
}


static void tool_find_elements_by_text(ToolArgs &args, Json::Value &response)
{
  eastl::string text = args.requireString("text");
  bool caseSensitive = args.getBool("case_sensitive", false);
  int maxResults = args.getInt("max_results", 50);
  if (!args.ok)
    return;

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  eastl::string out;
  scene->findElementsByText(out, text.c_str(), caseSensitive, maxResults);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = out.c_str();
  response["result"] = result;
}


static void handle_tool_call(const Json::Value &request, Json::Value &response)
{
  const Json::Value &params = request["params"];
  if (!params.isObject())
  {
    set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: 'params' must be an object");
    return;
  }
  const Json::Value &name = params["name"];
  if (name.isNull())
  {
    set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: 'name' is required");
    return;
  }
  if (!name.isString())
  {
    set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: 'name' must be a string");
    return;
  }
  const Json::Value &arguments = params["arguments"];
  if (!arguments.isObject())
  {
    set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: 'arguments' must be an object");
    return;
  }

  eastl::string toolName = name.asString();
  debug("MCP tool use call: %s", toolName.c_str());

  ToolArgs args(arguments, response);
  if (toolName == "get_last_error")
    tool_get_last_error(response);
  else if (toolName == "run_ui_script")
    tool_run_ui_script(args, response);
  else if (toolName == "make_screenshot")
    tool_make_screenshot(args, response);
  else if (toolName == "check_quirrel_syntax")
    tool_check_quirrel_syntax(args, response);
  else if (toolName == "mouse_click")
    tool_mouse_click(args, response);
  else if (toolName == "mouse_move")
    tool_mouse_move(args, response);
  else if (toolName == "send_keyboard_input")
    tool_send_keyboard_input(args, response);
  else if (toolName == "get_scene_tree")
    tool_get_scene_tree(args, response);
  else if (toolName == "find_elements_at")
    tool_find_elements_at(args, response);
  else if (toolName == "find_elements_by_text")
    tool_find_elements_by_text(args, response);
  else
    set_error(response, JSONRPC_INVALID_PARAMS, "Unknown tool: " + toolName);
}


static void handle_resources_list(Json::Value &response)
{
  Json::Value resources(Json::arrayValue);
  response["result"]["resources"] = resources;
}


static void handle_resource_read(const Json::Value &request, Json::Value &response)
{
  const Json::Value &params = request["params"];
  if (!params.isObject())
  {
    set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: 'params' must be an object");
    return;
  }
  const Json::Value &uri = params["uri"];
  if (uri.isNull())
  {
    set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: 'uri' is required");
    return;
  }
  if (!uri.isString())
  {
    set_error(response, JSONRPC_INVALID_PARAMS, "Invalid params: 'uri' must be a string");
    return;
  }

  set_error(response, JSONRPC_INVALID_PARAMS, "Unknown resource: " + uri.asString());
}


static void darg_mcp(RequestInfo *params)
{
  if (!params->content || !params->content_len)
  {
    text_response(params->conn, "Must be a POST request with data");
    return;
  }

  // MCP-compliant JSON response
  Json::Value response;
  response["jsonrpc"] = "2.0";

  Json::Value request;
  Json::Reader reader;
  char *body = (char *)params->read_POST_content();
  const bool parsed = body && reader.parse(body, request);
  tmpmem->free(body);
  if (!parsed)
  {
    response["id"] = Json::Value();
    set_error(response, JSONRPC_PARSE_ERROR, "Parse error: " + reader.getFormattedErrorMessages());
  }
  else if (!request.isObject() || !request["method"].isString())
  {
    response["id"] = request.isObject() ? request["id"] : Json::Value();
    set_error(response, JSONRPC_INVALID_REQUEST, "Invalid Request: expected an object with a string 'method'");
  }
  else
  {
    response["id"] = request["id"];
    eastl::string method = request["method"].asString();

    debug("MCP call, method: %s", method.c_str());

    if (method == "initialize")
      handle_initialize(request, response);
    else if (method == "tools/list")
      handle_tools_list(response);
    else if (method == "tools/call")
      handle_tool_call(request, response);
    else if (method == "resources/list")
      handle_resources_list(response);
    else if (method == "resources/read")
      handle_resource_read(request, response);
    else
      set_error(response, JSONRPC_METHOD_NOT_FOUND, "Method not found");
  }

  eastl::string respData = response.toStyledString();
  json_response(params->conn, respData.c_str(), respData.length());
}


webui::HttpPlugin webui::darg_mcp_http_plugins[] = {{"darg-llm-mcp", "daRg MCP API for LLMs", NULL, darg_mcp}, {NULL}};
