// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDagorStdGuiRender.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/aotDagorDriver3d.h>


DAS_BASE_BIND_ENUM_98(::FontFxType, FontFxType, FFT_NONE, FFT_SHADOW, FFT_GLOW, FFT_BLUR, FFT_OUTLINE);

DAS_BASE_BIND_ENUM_98(::BlendMode, BlendMode, NO_BLEND, PREMULTIPLIED, NONPREMULTIPLIED, ADDITIVE);

MAKE_TYPE_FACTORY(GuiVertexTransform, GuiVertexTransform);
MAKE_TYPE_FACTORY(StdGuiFontContext, StdGuiFontContext);


struct StdGuiShaderAnnotation : das::ManagedStructureAnnotation<StdGuiRender::StdGuiShader, false>
{
  StdGuiShaderAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("StdGuiShader", ml)
  {
    cppName = " ::StdGuiRender::StdGuiShader";

    addField<DAS_BIND_MANAGED_FIELD(material)>("material");
  }
};

struct GuiContextAnnotation : das::ManagedStructureAnnotation<::StdGuiRender::GuiContext>
{
  GuiContextAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("GuiContext", ml, " ::StdGuiRender::GuiContext") {}
};

struct StdGuiFontContextAnnotation : das::ManagedStructureAnnotation<StdGuiFontContext>
{
  StdGuiFontContextAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("StdGuiFontContext", ml, " ::StdGuiFontContext") {}

  virtual bool hasNonTrivialCtor() const override
  {
    // don't need custom ctor in script, default is enough
    static_assert(eastl::is_trivially_destructible<StdGuiFontContext>::value);
    return false;
  }

  virtual bool isLocal() const override { return true; } // is trivially destructible
};


struct GuiVertexTransformAnnotation : das::ManagedStructureAnnotation<GuiVertexTransform>
{
  GuiVertexTransformAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("GuiVertexTransform", ml, "GuiVertexTransform") {}
};

#define BIND_MEMBER(CLASS_NAME, FUNC_NAME, SYNONIM, SIDE_EFFECT)                                                                 \
  {                                                                                                                              \
    using memberMethod = DAS_CALL_MEMBER(CLASS_NAME::FUNC_NAME);                                                                 \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, DAS_CALL_MEMBER_CPP(CLASS_NAME::FUNC_NAME)); \
  }

#define BIND_MEMBER_SIGNATURE(CLASS_NAME, FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE)    \
  {                                                                                      \
    using memberMethod = das::das_call_member<SIGNATURE, &CLASS_NAME::FUNC_NAME>;        \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT,      \
      "das::das_call_member<" #SIGNATURE ", &" #CLASS_NAME "::" #FUNC_NAME ">::invoke"); \
  }

namespace bind_dascript
{

void render_line_aa(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points, bool is_closed, float line_width,
  const Point2 line_indent, E3DCOLOR color)
{
  ctx.render_line_aa((const Point2 *)points.data, points.size, is_closed, line_width, line_indent, color);
}

void render_poly(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points, E3DCOLOR fill_color)
{
  ctx.render_poly(make_span_const((const Point2 *)points.data, points.size), fill_color);
}

void render_inverse_poly(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points_ccw, E3DCOLOR fill_color, Point2 left_top,
  Point2 right_bottom)
{
  ctx.render_inverse_poly(make_span_const((const Point2 *)points_ccw.data, points_ccw.size), fill_color, left_top, right_bottom);
}

void get_view_tm(const ::StdGuiRender::GuiContext &ctx, GuiVertexTransform &gvtm, bool pure_trans)
{
  ctx.getViewTm(gvtm.vtm, pure_trans);
}

void draw_char_u(::StdGuiRender::GuiContext &ctx, uint16_t ch) { ctx.draw_char_u(ch); }

void set_view_tm(::StdGuiRender::GuiContext &ctx, const GuiVertexTransform &gvtm) { ctx.setViewTm(gvtm.vtm); }

struct DagorStdGuiRender final : public das::Module
{
  DagorStdGuiRender() : das::Module("DagorStdGuiRender")
  {
    das::ModuleLibrary lib(this);
    lib.addBuiltInModule();
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("DagorShaders"));
    addBuiltinDependency(lib, require("DagorDriver3D"));

    addEnumeration(das::make_smart<EnumerationFontFxType>());
    addEnumeration(das::make_smart<EnumerationBlendMode>());
    addAnnotation(das::make_smart<GuiContextAnnotation>(lib));
    addAnnotation(das::make_smart<StdGuiFontContextAnnotation>(lib));
    addAnnotation(das::make_smart<GuiVertexTransformAnnotation>(lib));
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
      das::SideEffects::accessExternal, "::bind_dascript::get_str_bbox");
    das::addExtern<DAS_BIND_FUN(render_rect)>(*this, lib, "StdGuiRender_render_rect", das::SideEffects::modifyExternal,
      "::bind_dascript::render_rect");
    das::addExtern<DAS_BIND_FUN(draw_line)>(*this, lib, "StdGuiRender_draw_line", das::SideEffects::modifyExternal,
      "::bind_dascript::draw_line");
    das::addExtern<DAS_BIND_FUN(StdGuiRender::screen_size)>(*this, lib, "StdGuiRender_screen_size", das::SideEffects::accessExternal,
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
    BIND(reset_textures, das::SideEffects::modifyExternal);
    BIND(end_render, das::SideEffects::modifyExternal);
    BIND(reset_shader, das::SideEffects::modifyExternal);
#undef BIND

#define BIND(fn, proto, se) das::addExtern<proto, StdGuiRender::fn>(*this, lib, "StdGuiRender_" #fn, se, "::StdGuiRender::" #fn)
    BIND(goto_xy, void (*)(real, real), das::SideEffects::modifyExternal);
    BIND(render_box, void (*)(real, real, real, real), das::SideEffects::modifyExternal);
    BIND(set_color, void (*)(E3DCOLOR), das::SideEffects::modifyExternal);
    BIND(start_render, void (*)(), das::SideEffects::modifyExternal);
    BIND(set_shader, void (*)(ShaderMaterial *), das::SideEffects::modifyExternal);
    BIND(set_shader, void (*)(const char *), das::SideEffects::modifyExternal);
#undef BIND


    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, set_color, "set_color", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(int, int, int, int));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, set_color, "set_color", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(E3DCOLOR));

    BIND_MEMBER(::StdGuiRender::GuiContext, screen_width, "screen_width", das::SideEffects::none);
    BIND_MEMBER(::StdGuiRender::GuiContext, screen_height, "screen_height", das::SideEffects::none);

    BIND_MEMBER(::StdGuiRender::GuiContext, render_frame, "render_frame", das::SideEffects::modifyArgument);
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_box, "render_box", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real, real, real));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_box, "render_box", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2));

    BIND_MEMBER(::StdGuiRender::GuiContext, render_rounded_box, "render_rounded_box", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, render_rounded_frame, "render_rounded_frame", das::SideEffects::modifyArgument);

    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, draw_line, "draw_line", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real, real, real, real));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, draw_line, "draw_line", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, real));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_rect, "render_rect", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real, real, real, Point2, Point2, Point2));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_rect_t, "render_rect_t", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real, real, real, Point2, Point2));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_rect_t, "render_rect_t", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, Point2, Point2));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, set_textures, "set_textures", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(TEXTUREID, d3d::SamplerHandle, TEXTUREID, d3d::SamplerHandle, bool, bool));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, set_texture, "set_texture", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(TEXTUREID, d3d::SamplerHandle, bool, bool));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, reset_textures, "reset_textures", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)());

    BIND_MEMBER(::StdGuiRender::GuiContext, hdpx, "hdpx", das::SideEffects::modifyArgument);

    das::addExtern<DAS_BIND_FUN(render_line_aa)>(*this, lib, "render_line_aa", das::SideEffects::modifyArgument,
      "bind_dascript::render_line_aa");
    das::addExtern<DAS_BIND_FUN(render_poly)>(*this, lib, "render_poly", das::SideEffects::modifyArgument,
      "bind_dascript::render_poly");
    das::addExtern<DAS_BIND_FUN(render_inverse_poly)>(*this, lib, "render_inverse_poly", das::SideEffects::modifyArgument,
      "bind_dascript::render_inverse_poly");
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_dashed_line, "render_line_dashed", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(const Point2, const Point2, const float, const float, const float, E3DCOLOR));

    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_ellipse_aa, "render_ellipse_aa", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, float, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_ellipse_aa, "render_ellipse_aa", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_sector_aa, "render_sector_aa", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, Point2, float, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_sector_aa, "render_sector_aa", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_rectangle_aa, "render_rectangle_aa", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, float, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_rectangle_aa, "render_rectangle_aa", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR));

    BIND_MEMBER(::StdGuiRender::GuiContext, render_smooth_round_rect, "render_smooth_round_rect", das::SideEffects::modifyArgument);

    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, goto_xy, "goto_xy", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, goto_xy, "goto_xy", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real));

    BIND_MEMBER(::StdGuiRender::GuiContext, get_text_pos, "get_text_pos", das::SideEffects::none);
    BIND_MEMBER(::StdGuiRender::GuiContext, set_spacing, "set_spacing", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, set_mono_width, "set_mono_width", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, get_spacing, "get_spacing", das::SideEffects::none);
    BIND_MEMBER(::StdGuiRender::GuiContext, set_font, "set_font", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, set_font_ht, "set_font_ht", das::SideEffects::modifyArgument);

    BIND_MEMBER(::StdGuiRender::GuiContext, set_draw_str_attr, "set_draw_str_attr", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, reset_draw_str_attr, "reset_draw_str_attr", das::SideEffects::modifyArgument);

    das::addExtern<DAS_BIND_FUN(draw_char_u)>(*this, lib, "draw_char_u", das::SideEffects::modifyExternal,
      "bind_dascript::draw_char_u");
    BIND_MEMBER(::StdGuiRender::GuiContext, draw_str_scaled, "draw_str_scaled", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, draw_str, "draw_str", das::SideEffects::modifyArgument);

    // Vertex transform

    BIND_MEMBER(::StdGuiRender::GuiContext, resetViewTm, "resetViewTm", das::SideEffects::modifyArgument);
    das::addExtern<DAS_BIND_FUN(set_view_tm)>(*this, lib, "setViewTm", das::SideEffects::modifyArgument, "setViewTm");
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, setViewTm, "setViewTm", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(Point2, Point2, Point2, bool));
    BIND_MEMBER(::StdGuiRender::GuiContext, setRotViewTm, "setRotViewTm", das::SideEffects::modifyArgument);
    das::addExtern<DAS_BIND_FUN(get_view_tm)>(*this, lib, "getViewTm", das::SideEffects::modifyArgument, "getViewTm");
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, set_viewport, "set_viewport", das::SideEffects::modifyExternal,
      void(::StdGuiRender::GuiContext::*)(real, real, real, real));
    BIND_MEMBER(::StdGuiRender::GuiContext, restore_viewport, "restore_viewport", das::SideEffects::modifyExternal);

    // StdGuiRender globals

    das::addExtern<DAS_BIND_FUN(::StdGuiRender::get_font_id)>(*this, lib, "get_font_id", das::SideEffects::accessExternal,
      "::StdGuiRender::get_font_id");
    das::addExtern<DAS_BIND_FUN(::StdGuiRender::get_font_context)>(*this, lib, "get_font_context", das::SideEffects::modifyArgument,
      "::StdGuiRender::get_font_context");

    das::addExtern<DAS_BIND_FUN(::StdGuiRender::get_str_bbox), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_str_bbox",
      das::SideEffects::none, "::StdGuiRender::get_str_bbox");
    das::addExtern<Point2 (*)(const StdGuiFontContext &), ::StdGuiRender::get_font_cell_size>(*this, lib, "get_font_cell_size",
      das::SideEffects::none, "::StdGuiRender::get_font_cell_size");
    das::addExtern<int (*)(const StdGuiFontContext &), ::StdGuiRender::get_font_caps_ht>(*this, lib, "get_font_caps_ht",
      das::SideEffects::none, "::StdGuiRender::get_font_caps_ht");
    das::addExtern<real (*)(const StdGuiFontContext &), ::StdGuiRender::get_font_line_spacing>(*this, lib, "get_font_line_spacing",
      das::SideEffects::none, "::StdGuiRender::get_font_line_spacing");
    das::addExtern<int (*)(const StdGuiFontContext &), ::StdGuiRender::get_font_ascent>(*this, lib, "get_font_ascent",
      das::SideEffects::none, "::StdGuiRender::get_font_ascent");
    das::addExtern<int (*)(const StdGuiFontContext &), ::StdGuiRender::get_font_descent>(*this, lib, "get_font_descent",
      das::SideEffects::none, "::StdGuiRender::get_font_descent");

    // GuiVertexTransform
    BIND_MEMBER(GuiVertexTransform, resetViewTm, "resetViewTm", das::SideEffects::modifyArgument);
    BIND_MEMBER_SIGNATURE(GuiVertexTransform, setViewTm, "setViewTm", das::SideEffects::modifyArgument,
      void(GuiVertexTransform::*)(Point2, Point2, Point2));
    BIND_MEMBER(GuiVertexTransform, addViewTm, "addViewTm", das::SideEffects::modifyArgument);

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