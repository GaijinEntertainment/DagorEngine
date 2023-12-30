#include <daRg/dasBinding.h>

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <EASTL/type_traits.h>


MAKE_TYPE_FACTORY(StdGuiFontContext, StdGuiFontContext);
MAKE_TYPE_FACTORY(GuiVertexTransform, GuiVertexTransform);


namespace das
{
template <>
struct cast<StdGuiFontContext> : cast_fVec<StdGuiFontContext>
{};
template <>
struct WrapType<StdGuiFontContext>
{
  enum
  {
    value = false
  };
  typedef int type;
  typedef int rettype;
};
} // namespace das


DAS_BIND_ENUM_CAST_98(FontFxType);
DAS_BASE_BIND_ENUM_98(FontFxType, FontFxType, FFT_NONE, FFT_SHADOW, FFT_GLOW, FFT_BLUR, FFT_OUTLINE);

#define ADD_FIELD(field) addField<DAS_BIND_MANAGED_FIELD(field)>(#field);

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


namespace darg
{


struct GuiContextAnnotation : das::ManagedStructureAnnotation<::StdGuiRender::GuiContext>
{
  GuiContextAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("GuiContext", ml, "::StdGuiRender::GuiContext") {}
};

struct RenderStateAnnotation : das::ManagedStructureAnnotation<::darg::RenderState>
{
  RenderStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RenderState", ml, "::darg::RenderState")
  {
    ADD_FIELD(opacity);
  }
};

struct ElemRenderDataAnnotation : das::ManagedStructureAnnotation<::darg::ElemRenderData>
{
  ElemRenderDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ElemRenderData", ml, "::darg::ElemRenderData")
  {
    ADD_FIELD(pos);
    ADD_FIELD(size);
  }
};

struct ElementAnnotation : das::ManagedStructureAnnotation<Element>
{
  ElementAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Element", ml, "Element") { ADD_FIELD(props); }
};

struct PropertiesAnnotation : das::ManagedStructureAnnotation<::darg::Properties>
{
  PropertiesAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Properties", ml, "::darg::Properties") {}
};


struct StdGuiFontContextAnnotation : das::ManagedValueAnnotation<StdGuiFontContext>
{
  StdGuiFontContextAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "StdGuiFontContext", " ::StdGuiFontContext") {}

  virtual bool hasNonTrivialCtor() const override
  {
    // don't need custom ctor in script, default is enough
    static_assert(eastl::is_trivially_destructible<StdGuiFontContext>::value);
    return false;
  }
};


struct GuiVertexTransformAnnotation : das::ManagedStructureAnnotation<GuiVertexTransform>
{
  GuiVertexTransformAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("GuiVertexTransform", ml, "GuiVertexTransform") {}
};


void render_line_aa(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points, bool is_closed, float line_width,
  const Point2 line_indent, E3DCOLOR color)
{
  ctx.render_line_aa((const Point2 *)points.data, points.size, is_closed, line_width, line_indent, color);
}

void render_poly(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points, E3DCOLOR fill_color)
{
  ctx.render_poly(make_span_const((const Point2 *)points.data, points.size), fill_color);
}

void render_inverse_poly(::StdGuiRender::GuiContext &ctx, const das::TArray<Point2> &points_ccw, E3DCOLOR fill_color,
  const Point2 &left_top, const Point2 &right_bottom)
{
  ctx.render_inverse_poly(make_span_const((const Point2 *)points_ccw.data, points_ccw.size), fill_color, left_top, right_bottom);
}

static void get_view_tm(const ::StdGuiRender::GuiContext &ctx, GuiVertexTransform &gvtm, bool pure_trans)
{
  ctx.getViewTm(gvtm.vtm, pure_trans);
}

static void set_view_tm(::StdGuiRender::GuiContext &ctx, const GuiVertexTransform &gvtm) { ctx.setViewTm(gvtm.vtm); }

/* Properties */

int props_get_int(const ::darg::Properties &props, const char *key, int def)
{
  return props.getInt(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

float props_get_float(const ::darg::Properties &props, const char *key, float def)
{
  return props.getFloat(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

bool props_get_bool(const ::darg::Properties &props, const char *key, bool def)
{
  return props.getBool(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

E3DCOLOR props_get_color(const ::darg::Properties &props, const char *key, E3DCOLOR def)
{
  return props.getColor(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

class ModuleDarg : public das::Module
{
public:
  ModuleDarg() : das::Module("darg")
  {
    das::ModuleLibrary lib(this);
    lib.addBuiltInModule();
    addBuiltinDependency(lib, require("DagorMath"));

    addAnnotation(das::make_smart<GuiContextAnnotation>(lib));
    addAnnotation(das::make_smart<StdGuiFontContextAnnotation>(lib));
    addAnnotation(das::make_smart<ElemRenderDataAnnotation>(lib));
    addAnnotation(das::make_smart<RenderStateAnnotation>(lib));
    addAnnotation(das::make_smart<PropertiesAnnotation>(lib));
    addAnnotation(das::make_smart<ElementAnnotation>(lib));
    addAnnotation(das::make_smart<GuiVertexTransformAnnotation>(lib));

    addEnumeration(das::make_smart<EnumerationFontFxType>());

    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, set_color, "set_color", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(int, int, int, int));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, set_color, "set_color", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(const E3DCOLOR &));

    BIND_MEMBER(::StdGuiRender::GuiContext, screen_width, "screen_width", das::SideEffects::none);
    BIND_MEMBER(::StdGuiRender::GuiContext, screen_height, "screen_height", das::SideEffects::none);

    BIND_MEMBER(::StdGuiRender::GuiContext, render_frame, "render_frame", das::SideEffects::modifyArgument);
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_box, "render_box", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real, real, real));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_box, "render_box", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(const Point2 &, const Point2 &));

    BIND_MEMBER(::StdGuiRender::GuiContext, render_rounded_box, "render_rounded_box", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, render_rounded_frame, "render_rounded_frame", das::SideEffects::modifyArgument);

    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, draw_line, "draw_line", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real, real, real, real));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, draw_line, "draw_line", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(const Point2 &, const Point2 &, real));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, render_rect, "render_rect", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real, real, real, const Point2 &, const Point2 &, const Point2 &));

    BIND_MEMBER(::StdGuiRender::GuiContext, hdpx, "hdpx", das::SideEffects::modifyArgument);

    das::addExtern<DAS_BIND_FUN(render_line_aa)>(*this, lib, "render_line_aa", das::SideEffects::modifyArgument,
      "darg::render_line_aa");
    das::addExtern<DAS_BIND_FUN(render_poly)>(*this, lib, "render_poly", das::SideEffects::modifyArgument, "darg::render_poly");
    das::addExtern<DAS_BIND_FUN(render_inverse_poly)>(*this, lib, "render_inverse_poly", das::SideEffects::modifyArgument,
      "darg::render_inverse_poly");
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
      void(::StdGuiRender::GuiContext::*)(const Point2 &));
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, goto_xy, "goto_xy", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(real, real));

    BIND_MEMBER(::StdGuiRender::GuiContext, set_spacing, "set_spacing", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, set_mono_width, "set_mono_width", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, get_spacing, "get_spacing", das::SideEffects::none);
    BIND_MEMBER(::StdGuiRender::GuiContext, set_font, "set_font", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, set_font_ht, "set_font_ht", das::SideEffects::modifyArgument);

    BIND_MEMBER(::StdGuiRender::GuiContext, set_draw_str_attr, "set_draw_str_attr", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, reset_draw_str_attr, "reset_draw_str_attr", das::SideEffects::modifyArgument);

    // BIND_MEMBER(::StdGuiRender::GuiContext, draw_char_u, "draw_char_u", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, draw_str_scaled, "draw_str_scaled", das::SideEffects::modifyArgument);
    BIND_MEMBER(::StdGuiRender::GuiContext, draw_str, "draw_str", das::SideEffects::modifyArgument);

    // Vertex transform

    BIND_MEMBER(::StdGuiRender::GuiContext, resetViewTm, "resetViewTm", das::SideEffects::modifyArgument);
    das::addExtern<DAS_BIND_FUN(set_view_tm)>(*this, lib, "setViewTm", das::SideEffects::modifyArgument, "setViewTm");
    BIND_MEMBER_SIGNATURE(::StdGuiRender::GuiContext, setViewTm, "setViewTm", das::SideEffects::modifyArgument,
      void(::StdGuiRender::GuiContext::*)(const Point2 &, const Point2 &, const Point2 &, bool));
    BIND_MEMBER(::StdGuiRender::GuiContext, setRotViewTm, "setRotViewTm", das::SideEffects::modifyArgument);
    das::addExtern<DAS_BIND_FUN(get_view_tm)>(*this, lib, "getViewTm", das::SideEffects::modifyArgument, "getViewTm");


    // StdGuiRender globals

    das::addExtern<DAS_BIND_FUN(::StdGuiRender::get_font_id)>(*this, lib, "get_font_id", das::SideEffects::none,
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

    // Properties

    das::addExtern<DAS_BIND_FUN(props_get_int)>(*this, lib, "getInt", das::SideEffects::none, "::darg::props_get_int");
    das::addExtern<DAS_BIND_FUN(props_get_float)>(*this, lib, "getFloat", das::SideEffects::none, "::darg::props_get_float");
    das::addExtern<DAS_BIND_FUN(props_get_bool)>(*this, lib, "getBool", das::SideEffects::none, "::darg::props_get_bool");
    das::addExtern<DAS_BIND_FUN(props_get_color)>(*this, lib, "getColor", das::SideEffects::none, "::darg::props_get_color");
    BIND_MEMBER(::darg::Properties, getFontId, "getFontId", das::SideEffects::none);
    BIND_MEMBER(::darg::Properties, getFontSize, "getFontSize", das::SideEffects::none);

    // GuiVertexTransform
    BIND_MEMBER(GuiVertexTransform, resetViewTm, "resetViewTm", das::SideEffects::modifyArgument);
    BIND_MEMBER_SIGNATURE(GuiVertexTransform, setViewTm, "setViewTm", das::SideEffects::modifyArgument,
      void(GuiVertexTransform::*)(const Point2 &, const Point2 &, const Point2 &));
    BIND_MEMBER(GuiVertexTransform, addViewTm, "addViewTm", das::SideEffects::modifyArgument);
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <daRg/dasBinding.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace darg

REGISTER_MODULE_IN_NAMESPACE(ModuleDarg, darg);
