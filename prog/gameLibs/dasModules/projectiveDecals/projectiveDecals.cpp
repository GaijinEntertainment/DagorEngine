#include <dasModules/aotProjectiveDecals.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>

namespace bind_dascript
{

struct ProjectiveDecalsAnnotation : das::ManagedStructureAnnotation<ProjectiveDecals>
{
  ProjectiveDecalsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ProjectiveDecals", ml, "ProjectiveDecals") {}
};

struct RingBufferDecalsAnnotation : das::ManagedStructureAnnotation<RingBufferDecals>
{
  RingBufferDecalsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RingBufferDecals", ml)
  {
    cppName = "RingBufferDecals";
    addProperty<DAS_BIND_MANAGED_PROP(getDecalsNum)>("decalsCount", "getDecalsNum");
  }
};

struct ResizableDecalsAnnotation : das::ManagedStructureAnnotation<ResizableDecals>
{
  ResizableDecalsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResizableDecals", ml, "ResizableDecals")
  {
    addProperty<DAS_BIND_MANAGED_PROP(getDecalsNum)>("decalsCount", "getDecalsNum");
  }
};

class ProjectiveDecalsModule final : public das::Module
{
public:
  ProjectiveDecalsModule() : das::Module("ProjectiveDecals")
  {
    das::ModuleLibrary lib(this);
    auto projectiveDecals = das::make_smart<ProjectiveDecalsAnnotation>(lib);
    addAnnotation(projectiveDecals);
    add_annotation(this, das::make_smart<RingBufferDecalsAnnotation>(lib), projectiveDecals);
    add_annotation(this, das::make_smart<ResizableDecalsAnnotation>(lib), projectiveDecals);

#define CLASS_MEMBER(FUNC_NAME, SYNONIM, SIDE_EFFECT)                                                                \
  {                                                                                                                  \
    using memberMethod = DAS_CALL_MEMBER(FUNC_NAME);                                                                 \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, DAS_CALL_MEMBER_CPP(FUNC_NAME)); \
  }
#define CLASS_MEMBER_SIGNATURE(FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE)          \
  {                                                                                 \
    using memberMethod = das::das_call_member<SIGNATURE, &FUNC_NAME>;               \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, \
      "das_call_member<" #SIGNATURE ", &" #FUNC_NAME ">::invoke");                  \
  }


    CLASS_MEMBER_SIGNATURE(RingBufferDecals::addDecal, "addDecal", das::SideEffects::modifyArgument,
      int(RingBufferDecals::*)(const TMatrix &, float, uint16_t, uint16_t, const Point4 &))
    CLASS_MEMBER_SIGNATURE(RingBufferDecals::addDecal, "addDecal", das::SideEffects::modifyArgument,
      int(RingBufferDecals::*)(const TMatrix &, float, uint16_t, const Point4 &))
    CLASS_MEMBER(RingBufferDecals::clear, "clear", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecals::afterReset, "afterReset", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecals::init, "init", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecals::init_buffer, "init_buffer", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecals::prepareRender, "prepareRender", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecals::render, "render", das::SideEffects::modifyArgument)

    CLASS_MEMBER(ProjectiveDecals::updateParams, "updateParams", das::SideEffects::modifyArgument)


    das::addConstant<uint16_t>(*this, "UPDATE_ALL", UPDATE_ALL);
    das::addConstant<uint16_t>(*this, "UPDATE_PARAM_Z", UPDATE_PARAM_Z);
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotProjectiveDecals.h>\n";
    tw << "#include <ecs/render/decalsES.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(ProjectiveDecalsModule, bind_dascript);
