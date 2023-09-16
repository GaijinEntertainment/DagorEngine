#include <daScript/daScript.h>
#include <ecs/game/actions/action.h>
#include <dasModules/aotEcs.h>
#include <dasModules/aotAction.h>

namespace bind_dascript
{

struct EntityActionAnnotation : das::ManagedStructureAnnotation<EntityAction, false>
{
  EntityActionAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("EntityAction", ml)
  {
    cppName = " ::EntityAction";
    addField<DAS_BIND_MANAGED_FIELD(stateIdx)>("stateIdx");
    addField<DAS_BIND_MANAGED_FIELD(actionDefTime)>("actionDefTime");
    addField<DAS_BIND_MANAGED_FIELD(timer)>("timer");
    addField<DAS_BIND_MANAGED_FIELD(actionTime)>("actionTime");
    addField<DAS_BIND_MANAGED_FIELD(applyAtDef)>("applyAtDef");
    addField<DAS_BIND_MANAGED_FIELD(applyAt)>("applyAt");
    addField<DAS_BIND_MANAGED_FIELD(prevRel)>("prevRel");
    addField<DAS_BIND_MANAGED_FIELD(actionPeriod)>("actionPeriod");
    addField<DAS_BIND_MANAGED_FIELD(overridePropsId)>("overridePropsId");
    addField<DAS_BIND_MANAGED_FIELD(interpDelayTicks)>("interpDelayTicks");
    addField<DAS_BIND_MANAGED_FIELD(propsId)>("propsId");
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(blocksSprint)>("blocksSprint");
    addField<DAS_BIND_MANAGED_FIELD(name)>("name");
    addField<DAS_BIND_MANAGED_FIELD(classHash)>("classHash");
  }
};

struct EntityActionsAnnotation : das::ManagedStructureAnnotation<EntityActions>
{
  EntityActionsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("EntityActions", ml)
  {
    cppName = " ::EntityActions";
    addField<DAS_BIND_MANAGED_FIELD(actions)>("actions");
    addField<DAS_BIND_MANAGED_FIELD(anyActionRunning)>("anyActionRunning");
  }
};

class ActionModule final : public das::Module
{
public:
  ActionModule() : das::Module("Action")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));

    addAnnotation(das::make_smart<EntityActionAnnotation>(lib));
    addAnnotation(das::make_smart<EntityActionsAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(check_action_precondition)>(*this, lib, "check_action_precondition", das::SideEffects::accessExternal,
      "::check_action_precondition");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <ecs/game/actions/action.h>\n";
    tw << "#include <dasModules/aotEcs.h>\n";
    tw << "#include <dasModules/aotAction.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(ActionModule, bind_dascript);
