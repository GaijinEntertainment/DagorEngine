#include <dasModules/aotHumanAnim.h>
#include <dasModules/aotHumanPhys.h>

struct HumanAnimStateLowerStateAnnotation : das::ManagedStructureAnnotation<HumanAnimState::LowerState, false>
{
  HumanAnimStateLowerStateAnnotation(das::ModuleLibrary &ml) :
    das::ManagedStructureAnnotation<HumanAnimState::LowerState, false>("HumanAnimLowerState", ml)
  {
    cppName = " ::HumanAnimState::LowerState";

    addField<DAS_BIND_MANAGED_FIELD(stand)>("stand");
    addField<DAS_BIND_MANAGED_FIELD(run)>("run");
    addField<DAS_BIND_MANAGED_FIELD(walk)>("walk");
    addField<DAS_BIND_MANAGED_FIELD(crawlMove)>("crawlMove");
    addField<DAS_BIND_MANAGED_FIELD(crawlStill)>("crawlStill");
    addField<DAS_BIND_MANAGED_FIELD(rotateLeft)>("rotateLeft");
    addField<DAS_BIND_MANAGED_FIELD(rotateRight)>("rotateRight");
    addField<DAS_BIND_MANAGED_FIELD(crouchRotateLeft)>("crouchRotateLeft");
    addField<DAS_BIND_MANAGED_FIELD(crouchRotateRight)>("crouchRotateRight");
    addField<DAS_BIND_MANAGED_FIELD(jumpStand)>("jumpStand");
    addField<DAS_BIND_MANAGED_FIELD(jumpRun)>("jumpRun");
    addField<DAS_BIND_MANAGED_FIELD(lowerSprint)>("lowerSprint");
  }
};

struct HumanAnimStateUpperStateAnnotation : das::ManagedStructureAnnotation<HumanAnimState::UpperState, false>
{
  HumanAnimStateUpperStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanAnimUpperState", ml)
  {
    cppName = " ::HumanAnimState::UpperState";

    addField<DAS_BIND_MANAGED_FIELD(aim)>("aim");
    addField<DAS_BIND_MANAGED_FIELD(fireReady)>("fireReady");
    addField<DAS_BIND_MANAGED_FIELD(changeWeapon)>("changeWeapon");
    addField<DAS_BIND_MANAGED_FIELD(reload)>("reload");
    addField<DAS_BIND_MANAGED_FIELD(gunDown)>("gunDown");
    addField<DAS_BIND_MANAGED_FIELD(fastThrow)>("fastThrow");
    addField<DAS_BIND_MANAGED_FIELD(throwGrenade)>("throwGrenade");
    addField<DAS_BIND_MANAGED_FIELD(deflect)>("deflect");
    addField<DAS_BIND_MANAGED_FIELD(aimCrouch)>("aimCrouch");
    addField<DAS_BIND_MANAGED_FIELD(fireReadyCrouch)>("fireReadyCrouch");
    addField<DAS_BIND_MANAGED_FIELD(changeWeaponCrouch)>("changeWeaponCrouch");
    addField<DAS_BIND_MANAGED_FIELD(reloadCrouch)>("reloadCrouch");
    addField<DAS_BIND_MANAGED_FIELD(heal)>("heal");
    addField<DAS_BIND_MANAGED_FIELD(healCrawl)>("healCrawl");
    addField<DAS_BIND_MANAGED_FIELD(swimEatStill)>("swimEatStill");
    addField<DAS_BIND_MANAGED_FIELD(upperSprint)>("upperSprint");
  }
};

struct HumanAnimStateComboStateAnnotation : das::ManagedStructureAnnotation<HumanAnimState::ComboState, false>
{
  HumanAnimStateComboStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanAnimComboState", ml)
  {
    cppName = " ::HumanAnimState::ComboState";

    addField<DAS_BIND_MANAGED_FIELD(crawlReload)>("crawlReload");
    addField<DAS_BIND_MANAGED_FIELD(crawlMove)>("crawlMove");
    addField<DAS_BIND_MANAGED_FIELD(crawlRotateLeft)>("crawlRotateLeft");
    addField<DAS_BIND_MANAGED_FIELD(crawlRotateRight)>("crawlRotateRight");
    addField<DAS_BIND_MANAGED_FIELD(crawlAim)>("crawlAim");
    addField<DAS_BIND_MANAGED_FIELD(crawlFireReady)>("crawlFireReady");
    addField<DAS_BIND_MANAGED_FIELD(crawlChangeWeapons)>("crawlChangeWeapons");
    addField<DAS_BIND_MANAGED_FIELD(healInjured)>("healInjured");
    addField<DAS_BIND_MANAGED_FIELD(injuredCrawlStill)>("injuredCrawlStill");
    addField<DAS_BIND_MANAGED_FIELD(injuredCrawlMove)>("injuredCrawlMove");
    addField<DAS_BIND_MANAGED_FIELD(attackedState)>("attackedState");
    addField<DAS_BIND_MANAGED_FIELD(swimStill)>("swimStill");
    addField<DAS_BIND_MANAGED_FIELD(swimStillUnderwater)>("swimStillUnderwater");
    addField<DAS_BIND_MANAGED_FIELD(swimOnWater)>("swimOnWater");
    addField<DAS_BIND_MANAGED_FIELD(swimUnderWater)>("swimUnderWater");
    addField<DAS_BIND_MANAGED_FIELD(climbHigh)>("climbHigh");
    addField<DAS_BIND_MANAGED_FIELD(climbLadder)>("climbLadder");
  }
};

struct HumanAnimStateAnnotation : das::ManagedStructureAnnotation<HumanAnimState, false>
{
  HumanAnimStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanAnimState", ml)
  {
    cppName = " ::HumanAnimState";
    addField<DAS_BIND_MANAGED_FIELD(lowerState)>("lowerState");
    addField<DAS_BIND_MANAGED_FIELD(upperState)>("upperState");
    addField<DAS_BIND_MANAGED_FIELD(comboState)>("comboState");

    addField<DAS_BIND_MANAGED_FIELD(dead)>("dead");
  }
};

struct HumanAnimStateResultAnnotation : das::ManagedStructureAnnotation<HumanAnimState::StateResult, false>
{
  HumanAnimStateResultAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("HumanAnimStateResult", ml)
  {
    cppName = " ::HumanAnimState::StateResult";
    addField<DAS_BIND_MANAGED_FIELD(upper)>("upper");
    addField<DAS_BIND_MANAGED_FIELD(lower)>("lower");
  }
};

namespace bind_dascript
{
class HumanAnimModule final : public das::Module
{
public:
  HumanAnimModule() : das::Module("HumanAnim")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("HumanPhys"));

    addAnnotation(das::make_smart<HumanAnimStateLowerStateAnnotation>(lib));
    addAnnotation(das::make_smart<HumanAnimStateUpperStateAnnotation>(lib));
    addAnnotation(das::make_smart<HumanAnimStateComboStateAnnotation>(lib));
    addAnnotation(das::make_smart<HumanAnimStateAnnotation>(lib));
    addAnnotation(das::make_smart<HumanAnimStateResultAnnotation>(lib));

    using method_updateState = das::das_call_member<HumanAnimState::StateResult (HumanAnimState::*)(HumanStatePos, HumanStateMove,
                                                      StateJump, HumanStateUpperBody, HumanAnimStateFlags),
      &::HumanAnimState::updateState>;
    das::addExtern<DAS_CALL_METHOD(method_updateState), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "updateState",
      das::SideEffects::modifyArgumentAndExternal, DAS_CALL_MEMBER_CPP(HumanAnimState::updateState));
    das::addExtern<DAS_BIND_FUN(bind_dascript::human_anim_state_update_state), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "updateState", das::SideEffects::modifyArgumentAndExternal, "bind_dascript::human_anim_state_update_state");

    das::addCtor<HumanAnimState::StateResult, int, int>(*this, lib, "HumanAnimStateResult", "::HumanAnimState::StateResult");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotHumanAnim.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(HumanAnimModule, bind_dascript);
