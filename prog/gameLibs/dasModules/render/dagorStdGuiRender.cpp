#include <dasModules/aotDagorStdGuiRender.h>
#include <dasModules/aotDagorMath.h>


DAS_BASE_BIND_ENUM_98(::FontFxType, FontFxType, FFT_NONE, FFT_SHADOW, FFT_GLOW, FFT_BLUR, FFT_OUTLINE);

DAS_BASE_BIND_ENUM_98(::BlendMode, BlendMode, NO_BLEND, PREMULTIPLIED, NONPREMULTIPLIED, ADDITIVE);

struct StdGuiShaderAnnotation : das::ManagedStructureAnnotation<StdGuiRender::StdGuiShader, false>
{
  StdGuiShaderAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("StdGuiShader", ml)
  {
    cppName = " ::StdGuiRender::StdGuiShader";

    addField<DAS_BIND_MANAGED_FIELD(material)>("material");
  }
};

namespace bind_dascript
{

struct DagorStdGuiRender final : public das::Module
{
  DagorStdGuiRender() : das::Module("DagorStdGuiRender")
  {
    das::ModuleLibrary lib(this);
    lib.addBuiltInModule();
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("DagorShaders"));

    addEnumeration(das::make_smart<EnumerationFontFxType>());
    addEnumeration(das::make_smart<EnumerationBlendMode>());
    addAnnotation(das::make_smart<StdGuiShaderAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(bind_dascript::StdGuiShader_ctor)>(*this, lib, "StdGuiShader_ctor", das::SideEffects::accessExternal,
      "::bind_dascript::StdGuiShader_ctor");
    das::addExtern<DAS_BIND_FUN(bind_dascript::StdGuiShader_dtor)>(*this, lib, "StdGuiShader_dtor", das::SideEffects::modifyArgument,
      "::bind_dascript::StdGuiShader_dtor");
    das::addExtern<DAS_BIND_FUN(bind_dascript::StdGuiShader_init)>(*this, lib, "init", das::SideEffects::modifyArgument,
      "::bind_dascript::StdGuiShader_init");
    das::addExtern<DAS_BIND_FUN(bind_dascript::StdGuiShader_close)>(*this, lib, "close", das::SideEffects::modifyArgument,
      "::bind_dascript::StdGuiShader_close");
    das::addExtern<DAS_BIND_FUN(bind_dascript::StdGuiRender_set_shader)>(*this, lib, "StdGuiRender_set_shader",
      das::SideEffects::modifyArgument, "::bind_dascript::StdGuiRender_set_shader");

    das::addExtern<DAS_BIND_FUN(get_str_bbox), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "StdGuiRender_get_str_bbox",
      das::SideEffects::accessExternal, "::StdGuiRender::get_str_bbox");
    das::addExtern<DAS_BIND_FUN(render_rect)>(*this, lib, "StdGuiRender_render_rect", das::SideEffects::modifyExternal,
      "::StdGuiRender::render_rect");
    das::addExtern<DAS_BIND_FUN(draw_line)>(*this, lib, "StdGuiRender_draw_line", das::SideEffects::modifyExternal,
      "::StdGuiRender::draw_line");
    das::addExtern<DAS_BIND_FUN(screen_size)>(*this, lib, "StdGuiRender_screen_size", das::SideEffects::accessExternal,
      "::StdGuiRender::screen_size");

// GuiVertex
#define BIND(fn, se) das::addExtern<DAS_BIND_FUN(GuiVertex::fn)>(*this, lib, "GuiVertex_" #fn, se, "::GuiVertex::" #fn)
    BIND(resetViewTm, das::SideEffects::modifyExternal);
    BIND(setRotViewTm, das::SideEffects::modifyExternal);
#undef BIND

// StdGuiRender
#define BIND(fn, se) das::addExtern<DAS_BIND_FUN(StdGuiRender::fn)>(*this, lib, "StdGuiRender_" #fn, se, "::StdGuiRender::" #fn)
    BIND(render_line_aa, das::SideEffects::modifyExternal);
    BIND(render_quad, das::SideEffects::modifyExternal);
    BIND(draw_str, das::SideEffects::modifyExternal);
    BIND(draw_str_scaled, das::SideEffects::modifyExternal);
    BIND(flush_data, das::SideEffects::modifyExternal);
    BIND(render_frame, das::SideEffects::modifyExternal);
    BIND(reset_draw_str_attr, das::SideEffects::modifyExternal);
    BIND(set_draw_str_attr, das::SideEffects::modifyExternal);
    BIND(set_font, das::SideEffects::modifyExternal);
    BIND(set_alpha_blend, das::SideEffects::modifyExternal);
    BIND(get_alpha_blend, das::SideEffects::accessExternal);
    BIND(set_texture, das::SideEffects::modifyExternal);
    BIND(end_render, das::SideEffects::modifyExternal);
    BIND(reset_shader, das::SideEffects::modifyExternal);
#undef BIND

#define BIND(fn, proto, se) das::addExtern<proto, StdGuiRender::fn>(*this, lib, "StdGuiRender_" #fn, se, "::StdGuiRender::" #fn)
    BIND(goto_xy, void (*)(real, real), das::SideEffects::modifyExternal);
    BIND(render_box, void (*)(real, real, real, real), das::SideEffects::modifyExternal);
    BIND(set_color, void (*)(const E3DCOLOR &), das::SideEffects::modifyExternal);
    BIND(start_render, void (*)(), das::SideEffects::modifyExternal);
    BIND(set_shader, void (*)(ShaderMaterial *), das::SideEffects::modifyExternal);
    BIND(set_shader, void (*)(const char *), das::SideEffects::modifyExternal);
#undef BIND

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorStdGuiRender.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DagorStdGuiRender, bind_dascript);