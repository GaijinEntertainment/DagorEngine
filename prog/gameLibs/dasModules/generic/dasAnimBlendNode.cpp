// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotDasAnimBlendNode.h>
#include <daECS/core/internal/ecsCounterRestorer.h>
#include <anim/dag_animBlend.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_log.h>
#include <ecs/scripts/dasEs.h>


// To force ANIM_PBC_HOT_RELOAD_ENABLED to be 1 or 0, build the game with -sEnableAnimPBCHotReload=yes or -sEnableAnimPBCHotReload=no,
// respectively
#ifndef ANIM_PBC_HOT_RELOAD_ENABLED
#define ANIM_PBC_HOT_RELOAD_ENABLED 0
#endif


das::mutex dasPostBlendControllerContextsMutex;
DasPostBlendControllerContexts dasPostBlendControllerContexts;

static das::mutex das_pbc_reload_mutex;

// A das Context is not thread-safe (shared, non-atomic insideContext + per-context heap), and
// AnimcharBaseComponent::act()/calcAnimWtm() runs post-blend controllers on several thread kinds
// (the main thread, threadpool workers during the parallel animchar update, and during entity
// creation). To avoid any sharing we give EVERY thread its own context (thread_local) that shares the
// original's read-only code & globals via makeWorkerFor() - the same trick parallel ECS systems use.
// The original `source` context is never evaluated on directly; it only serves as the makeWorkerFor
// source. So eval is fully parallel with no lock and no insideContext/heap races. Per-instance mutable
// runtime state must live in the IPureAnimStateHolder (inline ptr), not in the shared das instance.
//
// Only the rare hot-reload rebuild of the shared das instance (validateClassPtr) is serialized, via
// das_pbc_reload_mutex.
static thread_local bind_dascript::EsContext *t_das_blend_node_ctx = nullptr;

static bind_dascript::EsContext *get_blend_node_eval_context(bind_dascript::EsContext *source)
{
  if (DAGOR_UNLIKELY(!t_das_blend_node_ctx))
  {
    // Leaked intentionally: threadpool/main threads live for the app lifetime; the clone shares (does
    // not own) the source's globals, so there is nothing unsafe to free here.
    auto *c = new bind_dascript::EsContext(source->mgr, /*stackSize*/ 0); // 0 -> shared (per-thread) framemem stack
    c->name = "anim ctrl thread ctx";
    c->persistent = false;
    c->heap = das::make_unique<das::LinearHeapAllocator>();
    c->stringHeap = das::make_unique<das::LinearStringAllocator>();
    c->heap->setInitialSize(DAS_INITIAL_HEAP_SIZE);
    c->stringHeap->setInitialSize(DAS_INITIAL_STRING_HEAP_SIZE);
    t_das_blend_node_ctx = c;
  }
  t_das_blend_node_ctx->makeWorkerFor(*source); // share `source` read-only code & globals (early-outs once bound)
  t_das_blend_node_ctx->mgr = source->mgr;
  return t_das_blend_node_ctx;
}

class DasAnimPostBlendCtrl final : public AnimV20::AnimPostBlendCtrl
{
public:
  AnimPostBlendControllerDasContext *ctx;
  char *classPtr = nullptr;
  uint32_t gen = 0;
  const DataBlock *data = nullptr;

  void printUnhandledException(das::Context *c, const char *func_name) const
  {
    logerr("%s: unhandled exception in anim pbc: %s::%s: %s", c->exceptionAt.describe().c_str(), class_name(), func_name,
      c->getException());
  }

  void resetLinks()
  {
    das_aligned_free16(classPtr);
    classPtr = nullptr;
  }

  das::Func *getFuncPtr(uint32_t offset) const
  {
    if (!classPtr || offset == BLEND_NODE_CONTEXT_INVALID_OFFSET)
      return nullptr;
    auto func = (das::Func *)(classPtr + offset);
    return func->PTR == nullptr ? nullptr : func;
  }

  void setHiddenLinks()
  {
    if (ctx->thisCtrlOffset != BLEND_NODE_CONTEXT_INVALID_OFFSET)
      *(AnimV20::AnimPostBlendCtrl **)(classPtr + ctx->thisCtrlOffset) = this;
    if (ctx->srcBlkOffset != BLEND_NODE_CONTEXT_INVALID_OFFSET)
      *(const DataBlock **)(classPtr + ctx->srcBlkOffset) = data;
  }

  // Evaluates `fn(context)` on the current thread's worker context (lockless, parallel), with the
  // nested-query + exception-recovery guard. `fn` must invoke the das method on the passed context.
  template <typename Fn>
  void invokeGuarded(const char *func_name, Fn &&fn)
  {
    // Bind the main-thread das environment on worker threads, else env-dependent builtins used by the
    // controller (ecs query, profiler, rtti) dereference an unbound thread-local environment and crash.
    bind_dascript::RAIIDasEnvBound dasEnv;
    bind_dascript::EsContext *context = get_blend_node_eval_context(bind_dascript::cast_es_context(ctx->context));
    context->tryRestartAndLock();
    ecs::NestedQueryRestorer restorer(context->mgr);
    if (!context->ownStack)
    {
      das::SharedFramememStackGuard guard(*context);
      das::das_try_recover(
        context, [&]() { fn(context); },
        [&]() {
          printUnhandledException(context, func_name);
          restorer.restore(context->mgr);
        });
    }
    else
      das::das_try_recover(
        context, [&]() { fn(context); },
        [&]() {
          printUnhandledException(context, func_name);
          restorer.restore(context->mgr);
        });
    context->unlock();
  }

  DasAnimPostBlendCtrl(AnimV20::AnimationGraph &g, AnimPostBlendControllerDasContext *c) : AnimV20::AnimPostBlendCtrl(g), ctx(c) {}
  ~DasAnimPostBlendCtrl() { das_aligned_free16(classPtr); }

  // (Re)builds the das class instance when the owning context generation changes (hot-reload).
  bool validateClassPtr()
  {
    if (!ctx->context || !ctx->ctor)
    {
      resetLinks();
      logerr("daScript: anim psot blend controller <%s> validation error: %s %s", ctx->name.c_str(),
        !ctx->context ? "context is null." : "",
        !ctx->ctor ? "ctor is null. Add option always_export_initializer=true to export default c-tor." : "");
      return false;
    }
    if (ctx->gen == gen)
      return true;
    // Rebuilding the shared das instance is rare (load time / hot-reload). Serialize it so concurrent
    // lifecycle calls don't race on classPtr; they re-check the generation and skip once it's built.
    das::lock_guard<das::mutex> reloadLock(das_pbc_reload_mutex);
    if (ctx->gen == gen) // re-check after acquiring the lock: another thread may have rebuilt it
      return true;
#if ANIM_PBC_HOT_RELOAD_ENABLED
    const bool reload = gen > 0;
#endif
    resetLinks();
    bind_dascript::RAIIDasEnvBound dasEnv;
    bind_dascript::EsContext *context = get_blend_node_eval_context(bind_dascript::cast_es_context(ctx->context));
    context->tryRestartAndLock();
    ecs::NestedQueryRestorer restorer(context->mgr);
    classPtr = (char *)das_aligned_alloc16(ctx->structInfo->size);
    if (classPtr)
    {
      vec4f args[1];
      auto callCtor = [&]() { context->callWithCopyOnReturn(ctx->ctor, args, classPtr, nullptr); };
      auto onErr = [&]() {
        printUnhandledException(context, "ctor");
        restorer.restore(context->mgr);
      };
      if (!context->ownStack)
      {
        das::SharedFramememStackGuard guard(*context);
        das::das_try_recover(context, callCtor, onErr);
      }
      else
        das::das_try_recover(context, callCtor, onErr);
    }
    else
      logerr("daScript: can't allocate memory for anim ctrl <%s>", ctx->name.c_str());
    context->unlock();
    if (!classPtr)
      return false;
    setHiddenLinks();
#if ANIM_PBC_HOT_RELOAD_ENABLED
    if (reload)
      createNode(); // re-apply config on the freshly built instance
#endif
    gen = ctx->gen;
    return true;
  }

  void createNode()
  {
    // validateClassPtr is purposely missing to avoid deadlocks when called from validateClassPtr itself
    auto func = getFuncPtr(ctx->createNodeOffset);
    if (!func)
      return;
    invokeGuarded("createNode", [&](das::Context *c) {
      das::das_invoke_function<void>::invoke<void *, AnimV20::AnimationGraph &, const DataBlock &>(c, nullptr, *func, classPtr, graph,
        *data);
    });
  }

  void init(AnimV20::IPureAnimStateHolder &st, const GeomNodeTree &tree) override
  {
#if ANIM_PBC_HOT_RELOAD_ENABLED
    if (!validateClassPtr())
      return;
#endif
    auto func = getFuncPtr(ctx->initOffset);
    if (!func)
      return;
    invokeGuarded("init", [&](das::Context *c) {
      das::das_invoke_function<void>::invoke<void *, AnimV20::IPureAnimStateHolder &, const GeomNodeTree &>(c, nullptr, *func,
        classPtr, st, tree);
    });
  }

  void process(AnimV20::IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, Context &pbc_ctx) override
  {
    TIME_PROFILE(process_outer)
#if ANIM_PBC_HOT_RELOAD_ENABLED
    if (!validateClassPtr())
      return;
#endif
    auto func = getFuncPtr(ctx->processOffset);
    if (!func)
      return;
    invokeGuarded("process", [&](das::Context *c) {
      das::das_invoke_function<void>::invoke<void *, AnimV20::IPureAnimStateHolder &, float, GeomNodeTree &,
        AnimV20::AnimPostBlendCtrl::Context &>(c, nullptr, *func, classPtr, st, wt, tree, pbc_ctx);
    });
  }

  void advance(AnimV20::IPureAnimStateHolder &st, real dt) override
  {
#if ANIM_PBC_HOT_RELOAD_ENABLED
    if (!validateClassPtr())
      return;
#endif
    auto func = getFuncPtr(ctx->advanceOffset);
    if (!func)
      return;
    invokeGuarded("advance", [&](das::Context *c) {
      das::das_invoke_function<void>::invoke<void *, AnimV20::IPureAnimStateHolder &, float>(c, nullptr, *func, classPtr, st, dt);
    });
  }

  void setDefaultState(AnimV20::IPureAnimStateHolder &st) override
  {
#if ANIM_PBC_HOT_RELOAD_ENABLED
    if (!validateClassPtr())
      return;
#endif
    auto func = getFuncPtr(ctx->setDefaultStateOffset);
    if (!func)
      return;
    invokeGuarded("setDefaultState", [&](das::Context *c) {
      das::das_invoke_function<void>::invoke<void *, AnimV20::IPureAnimStateHolder &>(c, nullptr, *func, classPtr, st);
    });
  }

  void destroy() override {} // owned & deleted by AnimationGraph, like the built-in controllers

  const char *class_name() const override { return ctx->name.c_str(); }
};

static uint32_t get_offset(const das::StructurePtr &info, const char *funcName)
{
  for (das::Structure::FieldDeclaration &fieldDeclaration : info->fields)
    if (fieldDeclaration.name == funcName)
      return fieldDeclaration.offset;
  return BLEND_NODE_CONTEXT_INVALID_OFFSET;
}

void AnimPostBlendControllerDasContext::reset()
{
  context = nullptr;
  ctor = nullptr;
  createNodeOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  initOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  processOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  advanceOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  setDefaultStateOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  thisCtrlOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  srcBlkOffset = BLEND_NODE_CONTEXT_INVALID_OFFSET;
  ++gen;
}

void AnimPostBlendControllerDasContext::reload(const uint64_t mangled_name_hash)
{
  reset();
  mangledNameHash = mangled_name_hash;
  structInfo = nullptr;
}

void AnimPostBlendControllerDasContext::updateContext(das::Context *ctx, const das::StructurePtr &astStruct)
{
  G_ASSERT(ctx);
  if (context == ctx || !astStruct)
    return;
  G_ASSERT(das::hash_blockz64((uint8_t *)astStruct->getMangledName().c_str()) == mangledNameHash);

  das::SimFunction *newCtor = nullptr;
  for (int32_t i = 0; i != ctx->getTotalFunctions(); ++i)
  {
    das::SimFunction *func = ctx->getFunction(i);
    if (func && astStruct->name == func->name)
    {
      newCtor = func;
      break;
    }
  }
  if (!newCtor)
    return;
  reset();
  ctor = newCtor;
  context = ctx;
  createNodeOffset = get_offset(astStruct, "createNode");
  initOffset = get_offset(astStruct, "init");
  processOffset = get_offset(astStruct, "process");
  advanceOffset = get_offset(astStruct, "advance");
  setDefaultStateOffset = get_offset(astStruct, "setDefaultState");
  thisCtrlOffset = get_offset(astStruct, "thisCtrl");
  srcBlkOffset = get_offset(astStruct, "srcBlk");

  G_ASSERT(context->thisHelper);
  structInfo = context->thisHelper->makeStructureDebugInfo(*astStruct);
}

bool AnimDasPostBlendControlerAnnotation::internalField(const eastl::string &field_name)
{
  return field_name == "__rtti" || field_name == "thisCtrl" || field_name == "srcBlk";
}

static bool func_with_unused_args(const eastl::string &name)
{
  return name == "createNode" || name == "init" || name == "process" || name == "advance" || name == "setDefaultState";
}

bool AnimDasPostBlendControlerAnnotation::touch(const das::StructurePtr &st, das::ModuleGroup &,
  const das::AnnotationArgumentList &args, das::string &err)
{
  auto program = (*das::daScriptEnvironment::bound)->g_Program;
  if (program->thisModule->isModule)
  {
    err = "anim_post_blend_controller shouldn't be placed in a module. Please move the controller to a file without module directive";
    return false;
  }
  if (!st->isClass)
  {
    err = "class is required";
    return false;
  }
  const das::AnnotationArgument *nameAnnotation = args.find("name", das::Type::tString);
  if (!nameAnnotation || nameAnnotation->sValue.empty())
  {
    err = !nameAnnotation ? "name tag is required" : "name is empty";
    return false;
  }
  st->persistent = true;
  for (das::Structure::FieldDeclaration &field : st->fields)
  {
    if (!internalField(field.name) && !field.type->isNoHeapType())
      err.append_sprintf("field '%s' has unsupported type: %s. Only value/pointer types are supported\n", field.name.c_str(),
        field.type->describe().c_str());
    if (func_with_unused_args(field.name) && field.type->isFunction())
    {
      const auto fieldName = eastl::string(eastl::string::CtorSprintf(), "%s`%s", st->name.c_str(), field.name.c_str());
      auto it = st->module->functionsByName.find(das::hash64z(fieldName.c_str()));
      if (it)
        for (auto &function : it->second)
          for (auto &argument : function->arguments)
            argument->marked_used = true;
    }
  }
  return err.empty();
}

bool AnimDasPostBlendControlerAnnotation::look(const das::StructurePtr &st, das::ModuleGroup &,
  const das::AnnotationArgumentList &args, das::string &)
{
  const das::AnnotationArgument *nameAnnotation = args.find("name", das::Type::tString);
  if (!nameAnnotation)
    return false;

  das::lock_guard<das::mutex> lock(dasPostBlendControllerContextsMutex);
  auto prevFactory = dasPostBlendControllerContexts.find(nameAnnotation->sValue);
  const uint64_t mangledNameHash = das::hash_blockz64((uint8_t *)st->getMangledName().c_str());
  if (prevFactory == dasPostBlendControllerContexts.end())
    dasPostBlendControllerContexts[nameAnnotation->sValue] =
      eastl::make_unique<AnimPostBlendControllerDasContext>(nameAnnotation->sValue, mangledNameHash);
  else
  {
    prevFactory->second->reload(mangledNameHash);
    debug("daScript: reuse existed anim ctrl context <%s>", nameAnnotation->sValue.c_str());
  }
  return true;
}

void AnimDasPostBlendControlerAnnotation::complete(das::Context *context, const das::StructurePtr &st)
{
  das::lock_guard<das::mutex> lock(dasPostBlendControllerContextsMutex);
  if (context->category.value != uint32_t(das::ContextCategory::none) &&
      context->category.value != uint32_t(das::ContextCategory::debugger_attached))
    return;
  const uint64_t mangledNameHash = das::hash_blockz64((uint8_t *)st->getMangledName().c_str());
  for (const auto &pair : dasPostBlendControllerContexts)
    if (pair.second->mangledNameHash == mangledNameHash)
    {
      pair.second->updateContext(context, st);
      return;
    }
}

static bool create_das_pbc(AnimV20::AnimationGraph &graph, const DataBlock &blk)
{
  AnimPostBlendControllerDasContext *ctx = nullptr;
  {
    das::lock_guard<das::mutex> lock(dasPostBlendControllerContextsMutex);
    auto it = dasPostBlendControllerContexts.find(eastl::string(blk.getBlockName()));
    if (it == dasPostBlendControllerContexts.end())
      return false; // not ours; let add_bn report "unknown BlendNode class"
    ctx = it->second.get();
  }
  if (!ctx->context || !ctx->ctor)
  {
    logerr("daScript: anim pbc <%s> matched but its script context isn't ready", blk.getBlockName());
    return true; // claim it: a null fallback here would be more confusing than a clear error
  }
  const char *name = blk.getStr("name", nullptr);
  if (!AnimV20::IAnimBlendNode::isAnimNodeNameValid(name))
    return true;
  DasAnimPostBlendCtrl *node = new DasAnimPostBlendCtrl(graph, ctx);
  node->data = &blk; // Store params blk pointer for hot reload. We know blk lives for the life time of graph and all nodes so this
                     // should be always valid.
  node->validateClassPtr();
  node->createNode();
  graph.registerBlendNode(node, name);
  return true;
}

void register_das_blend_node_creator() { AnimV20::register_blend_node_creator(create_das_pbc); }
