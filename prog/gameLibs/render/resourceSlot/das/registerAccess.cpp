// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/das/resourceSlotCoreModule.h>
#include <detail/storage.h>

void bind_dascript::NodeHandleWithSlotsAccess_reset(::resource_slot::NodeHandleWithSlotsAccess &handle) { handle.reset(); }

::resource_slot::NodeHandleWithSlotsAccess bind_dascript::prepare_access(
  const ::bind_dascript::ResSlotPrepareCallBack &prepare_callback, ::das::Context *context, das::LineInfoArg *at)
{
  ::resource_slot::NodeHandleWithSlotsAccess handle;
  ::resource_slot::detail::ActionList list;
  vec4f args[] = {das::cast<::resource_slot::NodeHandleWithSlotsAccess &>::from(handle),
    das::cast<::resource_slot::detail::ActionList &>::from(list)};
  context->invoke(prepare_callback, args, nullptr, at);
  return handle;
}

void bind_dascript::register_access(resource_slot::NodeHandleWithSlotsAccess &handle, dafg::NameSpaceNameId ns, const char *name,
  resource_slot::detail::ActionList &action_list, ::bind_dascript::ResSlotDeclarationCallBack declaration_callback,
  das::Context *context)
{
  dafg::NameSpace nameSpace = dafg::NameSpace::_make_namespace(ns);
  handle = resource_slot::detail::register_access(nameSpace, name, eastl::move(action_list),
    [declCb = das::GcRootLambda(eastl::move(declaration_callback), context), context, node_name = eastl::string(name)](
      resource_slot::State state) -> dafg::NodeHandle {
      dafg::NodeHandle handle;

      context->tryRestartAndLock();

      if (!context->ownStack)
      {
        das::SharedFramememStackGuard guard(*context);
        das::das_invoke_lambda<void>::invoke<dafg::NodeHandle &, resource_slot::State &>(context, nullptr, declCb, handle, state);
      }
      else
        das::das_invoke_lambda<void>::invoke<dafg::NodeHandle &, resource_slot::State &>(context, nullptr, declCb, handle, state);

      if (auto exp = context->getException())
        logerr("error while register resource_slot access %s: %s\n", node_name.c_str(), exp);

      context->unlock();

      return eastl::move(handle);
    });
  handle._noteContext(context);
}