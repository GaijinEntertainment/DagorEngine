//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasDataBlock.h>
#include <gui/dag_imgui.h>
#include <perfMon/dag_statDrv.h>

DAS_BIND_ENUM_CAST(ImGuiState);

namespace bind_dascript
{
das::StackAllocator &get_shared_stack();

struct RegisterFunction
{
  eastl::string group;
  eastl::string name;
  eastl::string hotkey;
  int priority = 100;
  int flags = 0;
  uint64_t mangledNameHash = 0u;
  bool isWindow = true;
  das::Context *context = nullptr;
  eastl::unique_ptr<ImGuiFunctionQueue> queueNode = nullptr;
#if DA_PROFILER_ENABLED
  uint32_t dapToken = 0;
#endif
#if DAGOR_DBGLEVEL > 0
  eastl::string fileName;
#endif

  RegisterFunction() = default;
  RegisterFunction(eastl::string group_, eastl::string name_, eastl::string hotkey_, int priority_, int flags_, uint64_t mhn,
    bool is_window) :
    group(eastl::move(group_)),
    name(eastl::move(name_)),
    hotkey(eastl::move(hotkey_)),
    priority(priority_),
    flags(flags_),
    mangledNameHash(mhn),
    isWindow(is_window)
  {}
};

using ImguiRegistry = eastl::hash_map<eastl::string, RegisterFunction>;

struct RegisterImguiFunctionAnnotation : das::FunctionAnnotation
{
  ImguiRegistry &registry;
  das::mutex &mutex;
  bool defaultIsWindow = false;

  RegisterImguiFunctionAnnotation(ImguiRegistry &reg, das::mutex &mutex_, const das::string &name) :
    FunctionAnnotation(name), registry(reg), mutex(mutex_)
  {}
  RegisterImguiFunctionAnnotation(ImguiRegistry &reg, das::mutex &mutex_) :
    FunctionAnnotation("imgui_func"), registry(reg), mutex(mutex_)
  {}

  bool apply(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &, eastl::string &err) override
  {
    auto program = das::daScriptEnvironment::bound->g_Program;
    if (program->thisModule->isModule)
    {
      err = "imgui function shouldn't be placed in the module. Please move the function to a file without module directive";
      return false;
    }
    fn->exports = true;
    return true;
  }

  bool finalize(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &argTypes,
    const das::AnnotationArgumentList &, eastl::string &err) override
  {
    if (!fn->arguments.empty())
    {
      err += "imgui_window function doesn't support arguments";
      return false;
    }
    const das::AnnotationArgument *nameAnnotation = argTypes.find("name", das::Type::tString);
    eastl::string regName = nameAnnotation ? nameAnnotation->sValue : fn->name;
    const das::AnnotationArgument *groupAnnotation = argTypes.find("group", das::Type::tString);
    eastl::string group = groupAnnotation ? groupAnnotation->sValue : "daScript";
    const das::AnnotationArgument *hotkeyAnnotation = argTypes.find("hotkey", das::Type::tString);
    eastl::string hotkey = hotkeyAnnotation ? hotkeyAnnotation->sValue : "";
    const das::AnnotationArgument *priorityAnnotation = argTypes.find("priority", das::Type::tInt);
    const int priority = priorityAnnotation ? priorityAnnotation->iValue : 100;
    const das::AnnotationArgument *flagsAnnotation = argTypes.find("flags", das::Type::tInt);
    const int flags = flagsAnnotation ? flagsAnnotation->iValue : 0;
    const das::AnnotationArgument *isWindowAnnotation = argTypes.find("is_window", das::Type::tBool);
    const bool isWindow = isWindowAnnotation ? isWindowAnnotation->bValue : defaultIsWindow;

    auto func = RegisterFunction(group, regName, hotkey, priority, flags, fn->getMangledNameHash(), isWindow);

    auto &fileInfo = fn->atDecl.fileInfo;
    const char *fileName = fileInfo ? fileInfo->name.c_str() : nullptr;
    G_UNUSED(fileName);
#if DAGOR_DBGLEVEL > 0
    func.fileName = fileName;
#endif
#if DA_PROFILER_ENABLED
    int line = fileInfo ? (int)fn->atDecl.line : 0;
    func.dapToken = ::da_profiler::add_copy_description(fileName, line, /*flags*/ 0, regName.c_str());
#endif
    das::lock_guard<das::mutex> lock(mutex);
    auto find = registry.find(regName);
    if (find != registry.end())
    {
#if DAGOR_DBGLEVEL > 0
      for (const auto &pair : registry)
      {
        if (pair.second.mangledNameHash == func.mangledNameHash && pair.second.fileName != func.fileName)
          logerr("Imgui bind with same func name '%s' in different files %s vs %s", fn->name.c_str(), pair.second.fileName.c_str(),
            func.fileName.c_str());
        if (pair.first == regName && pair.second.fileName != func.fileName)
          logerr("Same imgui bind '%s' in different files %s vs %s", pair.first.c_str(), pair.second.fileName.c_str(),
            func.fileName.c_str());
      }
      func.fileName = fileName;
#endif
      RegisterFunction &prevFunc = find->second;
      func.queueNode = eastl::move(prevFunc.queueNode);
      unregisterFunc(func);
    }
    registry[regName] = eastl::move(func);
    return true;
  }

  bool apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, eastl::string &err) override
  {
    err = "not a block annotation";
    return false;
  }
  bool finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    eastl::string &err) override
  {
    err = "not a block annotation";
    return false;
  }

  void unloadContext(das::Context *ctx);

  using das::FunctionAnnotation::simulate;

  bool simulate(das::Context *ctx, das::SimFunction *fn) override
  {
    das::lock_guard<das::mutex> lock(mutex);
    for (auto &pair : registry)
    {
      RegisterFunction &regFunction = pair.second;
      if (regFunction.mangledNameHash == fn->mangledNameHash)
      {
        regFunction.context = ctx;
        registerFunc(regFunction);
        break;
      }
    }
    return true;
  }

  static void registerFunc(RegisterFunction &func)
  {
    if (func.queueNode != nullptr)
      return;
    func.queueNode = eastl::make_unique<ImGuiFunctionQueue>(
      func.group.c_str(), func.name.c_str(), func.hotkey.c_str(), func.priority, func.flags, [&func]() { callFunction(func); },
      func.isWindow);
  }

  static void removeFromQueue(ImGuiFunctionQueue *func, ImGuiFunctionQueue **queue)
  {
    ImGuiFunctionQueue **head = queue;
    if (*head == func)
    {
      *queue = (*head)->next;
    }
    else
    {
      while (head && *head)
      {
        if ((*head)->next == func)
          break;
        head = &(*head)->next;
      }
      if (head && *head && (*head)->next == func)
        (*head)->next = func->next;
    }
  }

  virtual void unregisterFunc(RegisterFunction &func)
  {
    if (func.queueNode)
    {
      removeFromQueue(func.queueNode.get(), &ImGuiFunctionQueue::functionHead);
      func.queueNode.reset(nullptr);
    }
  }

  static void callFunction(RegisterFunction &closureInfo)
  {
#if DA_PROFILER_ENABLED
    DA_PROFILE_EVENT_DESC(closureInfo.dapToken);
#endif
    if (closureInfo.mangledNameHash == 0u || closureInfo.context == nullptr)
      return;

    if (das::SimFunction *fn = closureInfo.context->fnByMangledName(closureInfo.mangledNameHash))
    {
      closureInfo.context->tryRestartAndLock();
      if (!closureInfo.context->ownStack)
      {
        das::SharedStackGuard guard(*closureInfo.context, bind_dascript::get_shared_stack());
        closureInfo.context->eval(fn);
      }
      else
        closureInfo.context->eval(fn);
      if (auto exp = closureInfo.context->getException())
        logerr("error: %s\n", exp);
      closureInfo.context->unlock();
    }
  }
};

inline void RegisterImguiFunctionAnnotation::unloadContext(das::Context *ctx)
{
  das::lock_guard<das::mutex> lock(mutex);
  for (auto it = registry.begin(); it != registry.end();)
  {
    if (it->second.context == ctx)
    {
      it->second.context = nullptr;
      unregisterFunc(it->second);
      it = registry.erase(it);
    }
    else
    {
      ++it;
    }
  }
}


struct RegisterImguiWindowAnnotation : RegisterImguiFunctionAnnotation
{
  RegisterImguiWindowAnnotation(ImguiRegistry &reg, das::mutex &mutex_) : RegisterImguiFunctionAnnotation(reg, mutex_, "imgui_window")
  {
    defaultIsWindow = true;
  }

  virtual void unregisterFunc(RegisterFunction &func) override
  {
    if (func.queueNode)
    {
      removeFromQueue(func.queueNode.get(), &ImGuiFunctionQueue::windowHead);
      func.queueNode.reset(nullptr);
    }
  }
};

inline DataBlock &imgui_get_blk() { return *::imgui_get_blk(); }

} // namespace bind_dascript
