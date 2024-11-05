// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dasModules/aotDagorConsole.h>
#include <EASTL/hash_map.h>

struct ConsoleProcessorFunction
{
  uint64_t mangledNameHash = 0u;
  das::Context *context = nullptr;

  ConsoleProcessorFunction() = default;
  ConsoleProcessorFunction(uint64_t mhn) : mangledNameHash(mhn) {}
};

using ConsoleProcessorRegistry = eastl::hash_map<eastl::string, ConsoleProcessorFunction>;

struct ConsoleProcessorHintAnnotation : das::ManagedStructureAnnotation<ConsoleProcessorHint, false>
{
  ConsoleProcessorHintAnnotation(das::ModuleLibrary &ml);
};

struct ConsoleProcessorFunctionAnnotation : das::FunctionAnnotation, console::ICommandProcessor
{
  ConsoleProcessorRegistry &registry;
  das::recursive_mutex &registryMutex;

  ConsoleProcessorFunctionAnnotation(ConsoleProcessorRegistry &reg, das::recursive_mutex &regMutex) :
    FunctionAnnotation("console_processor"), registry(reg), registryMutex(regMutex)
  {}

  ~ConsoleProcessorFunctionAnnotation() { del_con_proc(this); }

  bool apply(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &, eastl::string &err) override;

  bool finalize(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &argTypes,
    const das::AnnotationArgumentList &, eastl::string &err) override;

  bool apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, eastl::string &err) override;
  bool finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    eastl::string &err) override;

  using das::FunctionAnnotation::simulate;

  bool simulate(das::Context *ctx, das::SimFunction *fn) override;

  void destroy() override {}

  bool processCommand(const char *argv[], int argc) override;

  static bool callFunction(ConsoleProcessorFunction &closureInfo, vec4f args[3]);
};
