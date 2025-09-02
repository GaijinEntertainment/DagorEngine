//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_vector.h>
#include <util/dag_console.h>
#include <gui/dag_visualLog.h>
#include <gui/dag_visConsole.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <memory/dag_framemem.h>
#include <util/dag_simpleString.h>
#include <util/dag_convar.h>

DAS_BIND_ENUM_CAST(ConVarType);

struct ConsoleProcessorHint
{
  eastl::string name;
  int minArgs;
  int maxArgs;
  eastl::string argsDescription;
  eastl::string description;
};

MAKE_TYPE_FACTORY(ConsoleProcessorHint, ConsoleProcessorHint);

using ConsoleProcessorHints = dag::Vector<ConsoleProcessorHint, framemem_allocator>;

DAS_BIND_VECTOR(ConsoleProcessorHints, ConsoleProcessorHints, ConsoleProcessorHint, " ::ConsoleProcessorHints")

namespace bind_dascript
{
void das_add_con_proc_dagor_console();

inline void add_hint(ConsoleProcessorHints &hints, const char *name, int min_arg, int max_arg, const char *args_description,
  const char *description)
{
  hints.push_back({
    eastl::string(name ? name : ""),
    min_arg,
    max_arg,
    eastl::string(args_description ? args_description : ""),
    eastl::string(description ? description : ""),
  });
}

inline void console_get_command_history(const das::TBlock<void, const das::TTemporary<const das::TArray<const char *>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  auto history = console::get_command_history();

  das::Array arr;
  arr.data = (char *)history.data();
  arr.size = uint32_t(history.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline const char *console_get_edit_text_before_modify()
{
  console::IVisualConsoleDriver *console = console::get_visual_driver();
  return console ? console->getEditTextBeforeModify() : nullptr;
}

inline bool find_console_var(const das::TBlock<bool, const das::TTemporary<const char *>, ConVarType, float> &block,
  das::Context *context, das::LineInfoArg *at)
{
  bool found = false;
  vec4f args[3];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      ConVarIterator it;
      while (auto v = it.nextVar())
      {
        args[0] = das::cast<const char *>::from(v->getName());
        args[1] = das::cast<ConVarType>::from(v->getType());
        args[2] = das::cast<float>::from(v->getBaseValue());
        if (code->evalBool(*context))
        {
          found = true;
          break;
        }
      }
    },
    at);
  return found;
}

inline void visual_log_get_history(const das::TBlock<void, const das::TTemporary<const das::TArray<const char *>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  dag::ConstSpan<SimpleString> history = visuallog::getHistory();

  das::Array arr;
  arr.data = (char *)history.data();
  arr.size = uint32_t(history.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

} // namespace bind_dascript
