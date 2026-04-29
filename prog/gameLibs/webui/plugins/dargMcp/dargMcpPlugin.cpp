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

using namespace webui;
using namespace darg;


static darg::IGuiScene *(*scene_provider)() = nullptr;

static darg::IGuiScene *get_scene() { return scene_provider ? scene_provider() : nullptr; }

void webui::set_darg_mcp_scene_provider(darg::IGuiScene *(*provider)()) { scene_provider = provider; }


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

static void tool_get_last_error(const Json::Value &request, Json::Value &response)
{
  Json::Value params = request["params"];
  Json::Value arguments = params["arguments"];

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


static void tool_run_ui_script(const Json::Value &request, Json::Value &response)
{
  Json::Value params = request["params"];
  Json::Value arguments = params["arguments"];
  eastl::string filename = arguments.get("filename", "").asString();

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


static void tool_make_screenshot(const Json::Value &request, Json::Value &response)
{
  Json::Value arguments = request["params"]["arguments"];

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

    bool wantTree = arguments.get("scene_tree", true).asBool();
    if (wantTree)
    {
      IGuiScene *scene = get_scene();
      if (scene)
      {
        int treeMaxDepth = arguments.get("scene_tree_max_depth", 50).asInt();
        int treeMaxElems = arguments.get("scene_tree_max_elements", 2000).asInt();
        eastl::string filterText = arguments.get("scene_tree_filter", "").asString();
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


static void tool_check_quirrel_syntax(const Json::Value &request, Json::Value &response)
{
  Json::Value params = request["params"];
  Json::Value arguments = params["arguments"];

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene || !scene->getScriptVM())
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no script VM available";
  }
  else
  {
    eastl::string source = arguments.get("source", "").asString();
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


static void tool_mouse_click(const Json::Value &request, Json::Value &response)
{
  Json::Value arguments = request["params"]["arguments"];

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  int x = arguments.get("x", 0).asInt();
  int y = arguments.get("y", 0).asInt();
  eastl::string buttonName = arguments.get("button", "left").asString();
  int btnId = resolve_mouse_button(buttonName.c_str());
  if (btnId < 0)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: unknown button '" + buttonName + "', expected 'left', 'right', or 'middle'";
    response["result"] = result;
    return;
  }

  scene->onMouseEvent(INP_EV_POINTER_MOVE, 0, short(x), short(y), 0);
  scene->onMouseEvent(INP_EV_PRESS, btnId, short(x), short(y), 0);
  scene->onMouseEvent(INP_EV_RELEASE, btnId, short(x), short(y), 0);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = "OK";

  bool returnHitInfo = arguments.get("return_hit_info", true).asBool();
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


static void tool_mouse_move(const Json::Value &request, Json::Value &response)
{
  Json::Value arguments = request["params"]["arguments"];

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  int x = arguments.get("x", 0).asInt();
  int y = arguments.get("y", 0).asInt();

  scene->onMouseEvent(INP_EV_POINTER_MOVE, 0, short(x), short(y), 0);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = "OK";
  response["result"] = result;
}


static void tool_send_keyboard_input(const Json::Value &request, Json::Value &response)
{
  Json::Value arguments = request["params"]["arguments"];

  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  const Json::Value &actions = arguments["actions"];
  if (!actions.isArray())
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: 'actions' must be an array";
    response["result"] = result;
    return;
  }

  for (Json::ArrayIndex i = 0; i < actions.size(); ++i)
  {
    const Json::Value &action = actions[i];
    eastl::string type = action.get("type", "").asString();

    if (type == "text")
    {
      eastl::string text = action.get("text", "").asString();
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
    else if (type == "key")
    {
      eastl::string keyName = action.get("key", "").asString();
      int dkey = resolve_key_name(keyName.c_str());
      if (dkey < 0)
      {
        result["content"][0]["type"] = "text";
        result["content"][0]["text"] = "ERROR: unknown key '" + keyName + "' in action " + eastl::to_string(i);
        response["result"] = result;
        return;
      }
      scene->onKbdEvent(INP_EV_PRESS, dkey, 0, false);
      scene->onKbdEvent(INP_EV_RELEASE, dkey, 0, false);
    }
    else if (type == "combo")
    {
      const Json::Value &keys = action["keys"];
      if (!keys.isArray() || keys.empty())
      {
        result["content"][0]["type"] = "text";
        result["content"][0]["text"] = "ERROR: 'combo' action requires non-empty 'keys' array in action " + eastl::to_string(i);
        response["result"] = result;
        return;
      }

      // Resolve all key names first
      eastl::vector<int> dkeys;
      dkeys.reserve(keys.size());
      for (Json::ArrayIndex k = 0; k < keys.size(); ++k)
      {
        eastl::string keyName = keys[k].asString();
        int dkey = resolve_key_name(keyName.c_str());
        if (dkey < 0)
        {
          result["content"][0]["type"] = "text";
          result["content"][0]["text"] = "ERROR: unknown key '" + keyName + "' in combo, action " + eastl::to_string(i);
          response["result"] = result;
          return;
        }
        dkeys.push_back(dkey);
      }

      // Press all keys in order
      for (int dk : dkeys)
        scene->onKbdEvent(INP_EV_PRESS, dk, 0, false);

      // Release in reverse order
      for (int j = (int)dkeys.size() - 1; j >= 0; --j)
        scene->onKbdEvent(INP_EV_RELEASE, dkeys[j], 0, false);
    }
    else
    {
      result["content"][0]["type"] = "text";
      result["content"][0]["text"] = "ERROR: unknown action type '" + type + "' in action " + eastl::to_string(i);
      response["result"] = result;
      return;
    }
  }

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = "OK";
  response["result"] = result;
}


static void tool_get_scene_tree(const Json::Value &request, Json::Value &response)
{
  Json::Value arguments = request["params"]["arguments"];
  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  int maxDepth = arguments.get("max_depth", 50).asInt();
  eastl::string filterText = arguments.get("filter_text", "").asString();
  bool includeHidden = arguments.get("include_hidden", false).asBool();
  bool skipNonVisual = arguments.get("skip_non_visual", false).asBool();
  int maxElements = arguments.get("max_elements", 2000).asInt();

  eastl::string out;
  scene->inspectSceneTree(out, maxDepth, filterText.empty() ? nullptr : filterText.c_str(), includeHidden, skipNonVisual, maxElements);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = out.empty() ? "Empty scene" : out.c_str();
  response["result"] = result;
}


static void tool_find_elements_at(const Json::Value &request, Json::Value &response)
{
  Json::Value arguments = request["params"]["arguments"];
  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  int x = arguments.get("x", 0).asInt();
  int y = arguments.get("y", 0).asInt();

  eastl::string out;
  scene->inspectElementsAtPos(out, x, y);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = out.c_str();
  response["result"] = result;
}


static void tool_find_elements_by_text(const Json::Value &request, Json::Value &response)
{
  Json::Value arguments = request["params"]["arguments"];
  Json::Value result;

  IGuiScene *scene = get_scene();
  if (!scene)
  {
    result["content"][0]["type"] = "text";
    result["content"][0]["text"] = "ERROR: no UI scene";
    response["result"] = result;
    return;
  }

  eastl::string text = arguments.get("text", "").asString();
  bool caseSensitive = arguments.get("case_sensitive", false).asBool();
  int maxResults = arguments.get("max_results", 50).asInt();

  eastl::string out;
  scene->findElementsByText(out, text.c_str(), caseSensitive, maxResults);

  result["content"][0]["type"] = "text";
  result["content"][0]["text"] = out.c_str();
  response["result"] = result;
}


static void handle_tool_call(const Json::Value &request, Json::Value &response)
{
  Json::Value params = request["params"];
  eastl::string toolName = params["name"].asString();

  debug("MCP tool use call: %s", toolName.c_str());

  if (toolName == "get_last_error")
    tool_get_last_error(request, response);
  else if (toolName == "run_ui_script")
    tool_run_ui_script(request, response);
  else if (toolName == "make_screenshot")
    tool_make_screenshot(request, response);
  else if (toolName == "check_quirrel_syntax")
    tool_check_quirrel_syntax(request, response);
  else if (toolName == "mouse_click")
    tool_mouse_click(request, response);
  else if (toolName == "mouse_move")
    tool_mouse_move(request, response);
  else if (toolName == "send_keyboard_input")
    tool_send_keyboard_input(request, response);
  else if (toolName == "get_scene_tree")
    tool_get_scene_tree(request, response);
  else if (toolName == "find_elements_at")
    tool_find_elements_at(request, response);
  else if (toolName == "find_elements_by_text")
    tool_find_elements_by_text(request, response);
  else
  {
    response["error"]["code"] = -32602;
    response["error"]["message"] = "Unknown tool: " + toolName;
  }
}


static void handle_resources_list(Json::Value &response)
{
  Json::Value resources(Json::arrayValue);
  response["result"]["resources"] = resources;
}


static void handle_resource_read(const Json::Value &request, Json::Value &response)
{
  eastl::string uri = request["params"]["uri"].asString();

  response["error"]["code"] = -32602;
  response["error"]["message"] = "Unknown resource: " + uri;
}


static void darg_mcp(RequestInfo *params)
{
  Json::Value request;
  Json::Reader reader;
  if (!params->content || !params->content_len)
  {
    text_response(params->conn, "Must be a POST request with data");
    return;
  }

  reader.parse(params->content, request);

  // MCP-compliant JSON response
  Json::Value response;
  response["jsonrpc"] = "2.0";
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
  {
    // Unknown method
    response["error"]["code"] = -32601;
    response["error"]["message"] = "Method not found";
  }

  eastl::string respData = response.toStyledString();
  json_response(params->conn, respData.c_str(), respData.length());
}


webui::HttpPlugin webui::darg_mcp_http_plugins[] = {{"darg-llm-mcp", "daRg MCP API for LLMs", NULL, darg_mcp}, {NULL}};
