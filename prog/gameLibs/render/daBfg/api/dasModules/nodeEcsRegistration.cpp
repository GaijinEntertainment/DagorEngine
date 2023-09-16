#include <render/daBfg/ecs/frameGraphNode.h>
#include <api/dasModules/frameGraphModule.h>
#include <api/dasModules/nodeEcsRegistration.h>
#include <backend.h>


das::mutex nodeEcsRegistrationMutex;

bool NodeEcsRegistrationAnnotation::finalize(const das::FunctionPtr &func, das::ModuleGroup &, const das::AnnotationArgumentList &args,
  const das::AnnotationArgumentList &, das::string &err)
{
  if (func->arguments.size() != 1)
  {
    err.append_sprintf("Function %s annotated with [bfg_ecs_node] expects 1 parameter, but %d provided.", func->name.c_str(),
      args.size());
    return false;
  }
  if (args.size() != 3)
  {
    err.append_sprintf("[bfg_ecs_node] expects 3 parameter, but %d provided in %s.", args.size(), func->name.c_str());
    return false;
  }
  if (args[0].type != das::tString || args[1].type != das::tString || args[2].type != das::tString)
  {
    err.append_sprintf("[bfg_ecs_node] expects strings as parameters for %s", func->name.c_str());
    return false;
  }
  RegistrationArguments annArgs{};
  for (auto &arg : args)
  {
    if (arg.name == "name")
      annArgs.nodeName = args[0].sValue;
    else if (arg.name == "entity")
      annArgs.entityName = args[1].sValue;
    else if (arg.name == "handle")
      annArgs.handleName = args[2].sValue;
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

  das::lock_guard<das::mutex> regLock(nodeEcsRegistrationMutex);

  if (!dabfg::Backend::isInitialized())
  {
    logerr("dabfg::startup must be called before loading any das scripts that use daBfg!");
    return;
  }

  auto &tracker = dabfg::NodeTracker::get();

  G_ASSERT(arguments.contains(funcPtr->name));
  auto &args = arguments[funcPtr->name];
  const auto nodeId = tracker.registry.knownNodeNames.getNameId(args.nodeName);
  if (nodeId != dabfg::NodeNameId::Invalid && tracker.registry.nodes.get(nodeId).declare)
  {
    auto invokeFun = [this, context, &funcPtr, args]() {
      auto func = context->findFunction(funcPtr->name.c_str());
      dabfg::NodeHandle handle{};
      das::das_invoke_function<void>::invoke<dabfg::NodeHandle &>(context, nullptr, func, handle);
      if (ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(args.entityName.c_str())))
        g_entity_mgr->set(eid, ECS_HASH_SLOW(args.handleName.c_str()), eastl::move(handle));
    };
    callDasFunction(context, invokeFun);
  }
}