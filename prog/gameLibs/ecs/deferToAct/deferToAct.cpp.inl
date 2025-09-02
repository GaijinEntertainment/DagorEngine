// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/deferToAct/deferToAct.h>

#include <daECS/core/entitySystem.h>
#include <dasModules/dasScriptsLoader.h>
#include <dasModules/dasSharedStack.h>

namespace
{
struct DeferToActCall
{
  das::Context *context = nullptr;
  das::SimFunction *fn;

  DeferToActCall() = default;

  DeferToActCall(das::Context *ctx, das::SimFunction *func) : context(ctx), fn(func) {}
};
} // namespace

static dag::Vector<DeferToActCall> deferred_calls;
static das::mutex deferToActMutex;

void defer_to_act(das::Context *context, const eastl::string &func_name)
{
  das::SimFunction *fn = context->findFunction(func_name.c_str());
  G_ASSERT_RETURN(fn, );

  // Called from one of several script-reload threads, need a mutex
  das::lock_guard<das::mutex> lock(deferToActMutex);
  deferred_calls.emplace_back(context, fn);
}

void clear_deferred_to_act(das::Context *context)
{
  // Called from one of several script-reload threads, need a mutex
  das::lock_guard<das::mutex> lock(deferToActMutex);

  DeferToActCall *newEnd = eastl::remove_if(deferred_calls.begin(), deferred_calls.end(),
    [context](const DeferToActCall &f) { return f.context == context; });
  deferred_calls.resize(newEnd - deferred_calls.begin());
}

ECS_NO_ORDER
static inline void defer_to_act_es(const ecs::UpdateStageInfoAct &)
{
  // Called from main thread, when reload is stopped,
  // so it has no conflict with clear_deferred_to_act
  dag::Vector<DeferToActCall> grabbedCalls;
  {
    das::lock_guard<das::mutex> lock(deferToActMutex);
    eastl::swap(grabbedCalls, deferred_calls);
  }

  for (const DeferToActCall &f : grabbedCalls)
  {
    das::Context *context = f.context;

    context->tryRestartAndLock();

    if (!context->ownStack)
    {
      das::SharedFramememStackGuard guard(*context);
      das::das_invoke_function<void>::invoke(context, nullptr, f.fn);
    }
    else
      das::das_invoke_function<void>::invoke(context, nullptr, f.fn);

    if (auto exp = context->getException())
      logerr("error in defer_to_act: %s\n", exp);

    context->unlock();
  }

  deferred_calls.clear();
}