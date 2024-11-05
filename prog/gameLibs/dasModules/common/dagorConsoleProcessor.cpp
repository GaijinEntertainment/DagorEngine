// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dagorConsoleProcessor.h"
#include <dasModules/dasSystem.h>

#include <util/dag_console.h>

#include "daScript/ast/ast.h"


ConsoleProcessorHintAnnotation::ConsoleProcessorHintAnnotation(das::ModuleLibrary &ml) :
  ManagedStructureAnnotation("ConsoleProcessorHint", ml)
{
  cppName = " ::ConsoleProcessorHint";
  addField<DAS_BIND_MANAGED_FIELD(name)>("name");
  addField<DAS_BIND_MANAGED_FIELD(minArgs)>("minArgs");
  addField<DAS_BIND_MANAGED_FIELD(maxArgs)>("maxArgs");
  addField<DAS_BIND_MANAGED_FIELD(argsDescription)>("argsDescription");
  addField<DAS_BIND_MANAGED_FIELD(description)>("description");
}

bool ConsoleProcessorFunctionAnnotation::apply(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &,
  eastl::string &)
{
  fn->exports = true;
  return true;
}

bool ConsoleProcessorFunctionAnnotation::finalize(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &,
  const das::AnnotationArgumentList &, eastl::string &)
{
  das::lock_guard<das::recursive_mutex> lock(registryMutex);
  registry[fn->name] = ConsoleProcessorFunction(fn->getMangledNameHash());
  return true;
}

bool ConsoleProcessorFunctionAnnotation::apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &,
  eastl::string &err)
{
  err = "not a block annotation";
  return false;
}
bool ConsoleProcessorFunctionAnnotation::finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &,
  const das::AnnotationArgumentList &, eastl::string &err)
{
  err = "not a block annotation";
  return false;
}

bool ConsoleProcessorFunctionAnnotation::simulate(das::Context *ctx, das::SimFunction *fn)
{
  das::lock_guard<das::recursive_mutex> lock(registryMutex);
  for (auto &pair : registry)
  {
    if (pair.second.mangledNameHash == fn->mangledNameHash)
    {
      ConsoleProcessorFunction &function = pair.second;
      function.context = ctx;
      break;
    }
  }
  return true;
}

bool ConsoleProcessorFunctionAnnotation::processCommand(const char *argv[], int argc)
{
  das::lock_guard<das::recursive_mutex> lock(registryMutex);
  bool collect = ICommandProcessor::cmdCollector;

  eastl::vector<SimpleString> args;
  args.resize(argc);
  for (int i = 0; i < argc; ++i)
    args[i] = argv[i];

  das::Array argsArr;
  argsArr.data = (char *)&args[0];
  argsArr.size = uint32_t(args.size());
  argsArr.capacity = argsArr.size;
  argsArr.lock = 1;
  argsArr.flags = 0;

  ConsoleProcessorHints hints;
  vec4f funcArgs[3] = {
    das::cast<das::Array *>::from(&argsArr),
    das::cast<bool>::from(collect),
    das::cast<ConsoleProcessorHints *>::from(&hints),
  };

  if (collect)
  {
    for (auto func : registry)
    {
      callFunction(func.second, funcArgs);
      for (const ConsoleProcessorHint &hint : hints)
        console::collector_cmp(argv[0], argc, hint.name.c_str(), hint.minArgs, hint.maxArgs, hint.description.c_str(),
          hint.argsDescription.c_str());
      hints.clear();
    }

    return false;
  }

  for (auto func : registry)
  {
    if (callFunction(func.second, funcArgs))
      return true;
  }
  return false;
}

bool ConsoleProcessorFunctionAnnotation::callFunction(ConsoleProcessorFunction &closureInfo, vec4f args[3])
{
  if (closureInfo.mangledNameHash == 0u || closureInfo.context == nullptr)
    return false;

  if (das::SimFunction *fn = closureInfo.context->fnByMangledName(closureInfo.mangledNameHash))
  {
    bool result = false;

    closureInfo.context->tryRestartAndLock();
    if (!closureInfo.context->ownStack)
    {
      das::SharedStackGuard guard(*closureInfo.context, bind_dascript::get_shared_stack());
      result = das::cast<bool>::to(closureInfo.context->eval(fn, args));
    }
    else
      result = das::cast<bool>::to(closureInfo.context->eval(fn, args));
    if (auto exp = closureInfo.context->getException())
      logerr("error: %s\n", exp);
    closureInfo.context->unlock();

    return result;
  }

  return false;
}