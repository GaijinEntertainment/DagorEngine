#include "das_ecs.h"
#include <dasModules/dasEvent.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcsContainer.h>


namespace bind_dascript
{
void ECS::addListFn(das::ModuleLibrary &lib)
{
#define BIND_LIST_FUNCS(T)                                                                                                           \
  das::addExtern<DAS_BIND_FUN(emptyList<T>)>(*this, lib, "empty", das::SideEffects::none, "bind_dascript::emptyList< ::" #T ">");    \
  das::addExtern<DAS_BIND_FUN(lengthList<T>)>(*this, lib, "length", das::SideEffects::none, "bind_dascript::lengthList< ::" #T ">"); \
  das::addExtern<DAS_BIND_FUN(resizeList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "resize",                   \
    das::SideEffects::modifyArgument, "bind_dascript::resizeList< ::" #T ">");                                                       \
  das::addExtern<DAS_BIND_FUN(reserveList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "reserve",                 \
    das::SideEffects::modifyArgument, "bind_dascript::reserveList< ::" #T ">");                                                      \
  das::addExtern<DAS_BIND_FUN(clearList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "clear",                     \
    das::SideEffects::modifyArgument, "bind_dascript::clearList< ::" #T ">");                                                        \
  das::addExtern<DAS_BIND_FUN(eraseList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "erase",                     \
    das::SideEffects::modifyArgument, "bind_dascript::eraseList< ::" #T ">");                                                        \
  das::addExtern<DAS_BIND_FUN(eraseListRange<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "erase",                \
    das::SideEffects::modifyArgument, "bind_dascript::eraseListRange< ::" #T ">");                                                   \
  das::addExtern<DAS_BIND_FUN(pushList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "push",                       \
    das::SideEffects::modifyArgument, "bind_dascript::pushList< ::" #T ">");                                                         \
  das::addExtern<DAS_BIND_FUN(popList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "pop",                         \
    das::SideEffects::modifyArgument, "bind_dascript::popList< ::" #T ">");                                                          \
  das::addExtern<DAS_BIND_FUN(emplaceList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "emplace",                 \
    das::SideEffects::modifyArgument, "bind_dascript::emplaceList< ::" #T ">");                                                      \
  das::addExtern<DAS_BIND_FUN(dataPtrList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "data_ptr",                \
    das::SideEffects::unsafe, "bind_dascript::dataPtrList< ::" #T ">");                                                              \
  das::addExtern<DAS_BIND_FUN(moveList<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "move",                       \
    das::SideEffects::modifyArgument, "bind_dascript::moveList< ::" #T ">");                                                         \
  das::addExtern<DAS_BIND_FUN(swapLists<T>), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "swap",                      \
    das::SideEffects::modifyArgument, "bind_dascript::swapLists< ::" #T ">");                                                        \
  das::addExtern<DAS_BIND_FUN(getEntityComponentRefList<T>), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,                      \
    "getEntityComponentRef", das::SideEffects::none, "bind_dascript::getEntityComponentRefList< ::" #T ">");                         \
  das::addExtern<DAS_BIND_FUN(list_ctor<T>)>(*this, lib, "using", das::SideEffects::worstDefault,                                    \
    "bind_dascript::list_ctor< ::" #T ">");

#define DECL_LIST_TYPE(lt, t) BIND_LIST_FUNCS(ecs::lt)
  ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

#undef BIND_LIST_FUNCS

  das::addExtern<DAS_BIND_FUN(pushStringList)>(*this, lib, "push", das::SideEffects::modifyArgument, "bind_dascript::pushStringList");
  das::addExtern<DAS_BIND_FUN(emplaceStringList)>(*this, lib, "emplace", das::SideEffects::modifyArgument,
    "bind_dascript::emplaceStringList");
}
} // namespace bind_dascript
