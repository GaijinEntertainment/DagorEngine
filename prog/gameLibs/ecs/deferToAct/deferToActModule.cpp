// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/deferToAct/deferToAct.h>

#include <daECS/core/componentType.h>
#include <dasModules/dasModulesCommon.h>

struct DeferToActAnnotation : das::FunctionAnnotation
{
  DeferToActAnnotation() : FunctionAnnotation("defer_to_act") {}

  bool apply(const das::FunctionPtr &func, das::ModuleGroup &, const das::AnnotationArgumentList &, eastl::string &err) override
  {
    if (!func->arguments.empty())
    {
      err.append_sprintf("Function %s annotated with [defer_to_act] expects no parameters, but %d provided.", func->name.c_str(),
        func->arguments.size());
      return false;
    }

    func->exports = true;
    return true;
  }

  bool apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, eastl::string &err) override
  {
    err = "not a block annotation";
    return false;
  }

  void complete(das::Context *context, const das::FunctionPtr &func)
  {
    if (das::is_in_aot() || das::is_in_completion())
      return;

    defer_to_act(context, func->name);
  }

  bool finalize(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &) override
  {
    return true;
  }

  bool finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &err) override
  {
    err = "not a block annotation";
    return false;
  }
};

namespace bind_dascript
{

class DeferToActModule final : public das::Module
{
public:
  DeferToActModule() : das::Module("DeferToAct")
  {
    das::ModuleLibrary lib(this);
    das::onDestroyCppDebugAgent(name.c_str(), [](das::Context *ctx) { clear_deferred_to_act(ctx); });

    addAnnotation(das::make_smart<DeferToActAnnotation>());
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter & /*tw*/) const override { return das::ModuleAotType::cpp; }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DeferToActModule, bind_dascript);
