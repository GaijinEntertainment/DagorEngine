#include <dasModules/aotDagorDriver3d.h>
#include <dasModules/dagorTexture3d.h>

DAS_BIND_ENUM_CAST(DepthAccess);
DAS_BASE_BIND_ENUM(DepthAccess, DepthAccess, RW, SampledRO);

void bind_driver_consts(das::Module &module);

struct ShadersECSAnnotation : das::ManagedStructureAnnotation<ShadersECS, false>
{
  ShadersECSAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ShadersECS", ml) { cppName = " ::ShadersECS"; }
};

struct PostFxRendererAnnotation : das::ManagedStructureAnnotation<PostFxRenderer, false>
{
  PostFxRendererAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PostFxRenderer", ml, "PostFxRenderer") {}
};


struct Driver3dPerspectiveAnnotation : das::ManagedStructureAnnotation<Driver3dPerspective, false>
{
  Driver3dPerspectiveAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Driver3dPerspective", ml)
  {
    cppName = " ::Driver3dPerspective";

    addField<DAS_BIND_MANAGED_FIELD(wk)>("wk");
    addField<DAS_BIND_MANAGED_FIELD(hk)>("hk");
    addField<DAS_BIND_MANAGED_FIELD(zn)>("zn");
    addField<DAS_BIND_MANAGED_FIELD(zf)>("zf");
    addField<DAS_BIND_MANAGED_FIELD(ox)>("ox");
    addField<DAS_BIND_MANAGED_FIELD(oy)>("oy");
  }
};

struct OverrideStateAnnotation : das::ManagedStructureAnnotation<shaders::OverrideState, false>
{
  OverrideStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("OverrideState", ml, "shaders::OverrideState")
  {
    addField<DAS_BIND_MANAGED_FIELD(bits)>("bits");
    addField<DAS_BIND_MANAGED_FIELD(zFunc)>("zFunc");
    addField<DAS_BIND_MANAGED_FIELD(forcedSampleCount)>("forcedSampleCount");
    addField<DAS_BIND_MANAGED_FIELD(blendOp)>("blendOp");
    addField<DAS_BIND_MANAGED_FIELD(blendOpA)>("blendOpA");
    addField<DAS_BIND_MANAGED_FIELD(sblend)>("sblend");
    addField<DAS_BIND_MANAGED_FIELD(dblend)>("dblend");
    addField<DAS_BIND_MANAGED_FIELD(sblenda)>("sblenda");
    addField<DAS_BIND_MANAGED_FIELD(dblenda)>("dblenda");
    addField<DAS_BIND_MANAGED_FIELD(colorWr)>("colorWr");
  }
};

static char dagorDriver3d_das[] =
#include "dagorDriver3d.das.inl"
  ;

namespace bind_dascript
{
class DagorDriver3DModule final : public das::Module
{
public:
  DagorDriver3DModule() : das::Module("DagorDriver3D")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorTexture3D"));
    addAnnotation(das::make_smart<Driver3dPerspectiveAnnotation>(lib));
    addAnnotation(das::make_smart<ShadersECSAnnotation>(lib));
    addAnnotation(das::make_smart<PostFxRendererAnnotation>(lib));
    addAnnotation(das::make_smart<OverrideStateAnnotation>(lib));
    addEnumeration(das::make_smart<EnumerationDepthAccess>());

    das::addUsing<shaders::OverrideState>(*this, lib, "shaders::OverrideState");


#define CLASS_MEMBER(FUNC_NAME, SYNONIM, SIDE_EFFECT)                                                                \
  {                                                                                                                  \
    using memberMethod = DAS_CALL_MEMBER(FUNC_NAME);                                                                 \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, DAS_CALL_MEMBER_CPP(FUNC_NAME)); \
  }

#define BIND_FUNC(FUNC, NAME, SIDE_EFFECT) das::addExtern<DAS_BIND_FUN(FUNC)>(*this, lib, NAME, das::SideEffects::SIDE_EFFECT, #FUNC)

#define BIND_FUNC_SIGNATURE(FUNC, NAME, SIDE_EFFECT, SIGNATURE) \
  das::addExtern<SIGNATURE, FUNC>(*this, lib, NAME, das::SideEffects::SIDE_EFFECT, #FUNC)

#define BIND_WRAP_FUNC(FUNC, NAME, SIDE_EFFECT) \
  das::addExtern<DAS_BIND_FUN(FUNC)>(*this, lib, NAME, das::SideEffects::SIDE_EFFECT, "bind_dascript::" #FUNC)

    BIND_WRAP_FUNC(get_Driver3dPerspective, "get_Driver3dPerspective", modifyArgumentAndExternal);

    BIND_FUNC_SIGNATURE(d3d::set_depth, "d3d_set_depth", modifyArgumentAndExternal, bool (*)(BaseTexture *, DepthAccess));
    BIND_FUNC_SIGNATURE(d3d::set_depth, "d3d_set_depth", modifyArgumentAndExternal, bool (*)(BaseTexture *, int, DepthAccess));
    BIND_FUNC_SIGNATURE(d3d::set_render_target, "d3d_set_render_target", modifyArgumentAndExternal, bool (*)());
    BIND_FUNC_SIGNATURE(d3d::set_render_target, "d3d_set_render_target", modifyArgumentAndExternal, bool (*)(BaseTexture *, int));
    BIND_FUNC_SIGNATURE(d3d::set_render_target, "d3d_set_render_target", modifyArgumentAndExternal, bool (*)(int, BaseTexture *, int));
    BIND_FUNC_SIGNATURE(d3d::set_render_target, "d3d_set_render_target", modifyArgumentAndExternal,
      bool (*)(int, BaseTexture *, int, int));
    BIND_FUNC(d3d::get_screen_size, "d3d_get_screen_size", modifyArgument);
    BIND_FUNC(d3d::setwire, "d3d_setwire", modifyExternal);
    BIND_FUNC(d3d::clearview, "d3d_clearview", modifyExternal);
    BIND_FUNC(d3d::draw, "d3d_draw", modifyExternal);
    BIND_FUNC(d3d::setvsrc, "d3d_setvsrc", modifyExternal);
    BIND_FUNC(d3d::get_target_size, "d3d_get_target_size", accessExternal);
    BIND_FUNC(d3d::get_render_target_size, "d3d_get_render_target_size", accessExternal);
    BIND_FUNC(d3d::setview, "d3d_setview", modifyExternal);
    BIND_FUNC(d3d::getview, "d3d_getview", accessExternal);
    BIND_FUNC(d3d::get_driver_name, "d3d_get_driver_name", accessExternal);
    BIND_WRAP_FUNC(get_grs_draw_wire, "get_grs_draw_wire", accessExternal);
    BIND_WRAP_FUNC(shader_setStates, "setStates", modifyExternal);
    BIND_WRAP_FUNC(postfx_setStates, "setStates", modifyExternal);
    BIND_WRAP_FUNC(d3d_resource_barrier_tex, "d3d_resource_barrier", modifyExternal);
    BIND_WRAP_FUNC(d3d_resource_barrier_buf, "d3d_resource_barrier", modifyExternal);
    BIND_WRAP_FUNC(d3d_get_vsync_refresh_rate, "d3d_get_vsync_refresh_rate", accessExternal);


    CLASS_MEMBER(PostFxRenderer::render, "render", das::SideEffects::modifyExternal)

#undef CLASS_MEMBER
#undef BIND_FUNC
#undef BIND_FUNC_SIGNATURE
#undef BIND_WRAP_FUNC

    bind_driver_consts(*this);

    compileBuiltinModule("dagorDriver3d.das", (unsigned char *)dagorDriver3d_das, sizeof(dagorDriver3d_das));
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorDriver3d.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorDriver3DModule, bind_dascript);
