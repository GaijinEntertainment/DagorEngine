#include <daECS/core/componentTypes.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <api/das/frameGraphModule.h>
#include <api/das/nodeEcsRegistration.h>
#include <runtime/runtime.h>


bool NodeEcsRegistrationAnnotation::finalize(const das::FunctionPtr &func, das::ModuleGroup &, const das::AnnotationArgumentList &args,
  const das::AnnotationArgumentList &, das::string &err)
{
  const bool isCollection = args.size() == 4;
  const uint32_t funcArgsCnt = isCollection ? 2 : 1;
  const char *macroDesc = isCollection ? "[bfg_ecs_node] (with counter_handle argument)" : "[bfg_ecs_node] (for a single node)";
  const uint32_t macroArgsCnt = isCollection ? 4 : 3;
  if (func->arguments.size() != funcArgsCnt)
  {
    err.append_sprintf("Function %s annotated with %s expects %d parameter, but %d provided.", func->name.c_str(), macroDesc,
      funcArgsCnt, args.size());
    return false;
  }
  if (args.size() != 3 && !isCollection)
  {
    err.append_sprintf("%s expects %d parameter, but %d provided in %s.", macroDesc, macroArgsCnt, args.size(), func->name.c_str());
    return false;
  }
  if (args[0].type != das::tString || args[1].type != das::tString || args[2].type != das::tString ||
      (isCollection && args[3].type != das::tString))
  {
    err.append_sprintf("%s expects strings as parameters for %s", macroDesc, func->name.c_str());
    return false;
  }
  RegistrationArguments annArgs{};
  for (auto &arg : args)
  {
    // We should use full path for correct name resolution
    if (arg.name == "name")
      annArgs.nodeName = !args[0].sValue.empty() && args[0].sValue[0] == '/' ? args[0].sValue : "/" + args[0].sValue;
    else if (arg.name == "entity")
      annArgs.entityName = args[1].sValue;
    else if (arg.name == "handle")
      annArgs.handleName = args[2].sValue;
    else if (isCollection && arg.name == "counter_handle")
      annArgs.counterHandleName = args[3].sValue;
  }
  if (annArgs.nodeName.empty())
  {
    err.append_sprintf("Missing 'name' parameter in [bfg_ecs_node] for %s", func->name.c_str());
    return false;
  }
  if (annArgs.entityName.empty())
  {
    err.append_sprintf("Missing 'entity' parameter in [bfg_ecs_node] for %s", func->name.c_str());
    return false;
  }
  if (annArgs.handleName.empty())
  {
    err.append_sprintf("Missing 'handle' parameter in [bfg_ecs_node] for %s", func->name.c_str());
    return false;
  }
  if (isCollection && annArgs.counterHandleName.empty())
  {
    err.append_sprintf("Missing 'counter_handle' parameter in [bfg_ecs_node] for %s", func->name.c_str());
    return false;
  }
  if (isCollection)
    annArgs.nodeName += "_0"; // We should check an existance of the first node.
  arguments.emplace(func->name, eastl::move(annArgs));
  return true;
}

void NodeEcsRegistrationAnnotation::complete(das::Context *context, const das::FunctionPtr &funcPtr)
{
  // Aot doesn't need this macro. Despite the weird name, the second
  // check actually disables this macro while running the AOT compiler
  // inside an IDE plugin.
  if (das::is_in_aot() || das::is_in_completion())
    return;

  das::lock_guard<das::mutex> lock{dabfgRuntimeMutex};

  if (!dabfg::Runtime::isInitialized())
  {
    logerr("dabfg::startup must be called before loading any das scripts that use daBfg!");
    return;
  }

  auto &runtime = dabfg::Runtime::get();
  auto &registry = runtime.getInternalRegistry();

  G_ASSERT(arguments.contains(funcPtr->name));
  auto &args = arguments[funcPtr->name];
  const auto nodeId = registry.knownNames.getNameId<dabfg::NodeNameId>(args.nodeName.c_str());
  if (nodeId != dabfg::NodeNameId::Invalid && registry.nodes.get(nodeId).declare)
  {
    auto invokeFun = [context, &funcPtr, args]() {
      auto func = context->findFunction(funcPtr->name.c_str());
      if (args.counterHandleName.empty())
      {
        dabfg::NodeHandle handle{};
        das::das_invoke_function<void>::invoke<dabfg::NodeHandle &>(context, nullptr, func, handle);
        if (ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(args.entityName.c_str())))
          g_entity_mgr->set(eid, ECS_HASH_SLOW(args.handleName.c_str()), eastl::move(handle));
      }
      else
      {
        if (ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(args.entityName.c_str())))
        {
          dag::Vector<dabfg::NodeHandle> handles(g_entity_mgr->get<int>(eid, ECS_HASH_SLOW(args.counterHandleName.c_str())));
          for (uint32_t i = 0; i < handles.size(); ++i)
            das::das_invoke_function<void>::invoke<dabfg::NodeHandle &, uint32_t>(context, nullptr, func, handles[i], i);
          if (ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(args.entityName.c_str())))
            g_entity_mgr->set(eid, ECS_HASH_SLOW(args.handleName.c_str()), eastl::move(handles));
        }
      }
    };
    callDasFunction(context, invokeFun);
  }
}