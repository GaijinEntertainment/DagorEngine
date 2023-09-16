#include "dasBinding.h"

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>
#include <EASTL/type_traits.h>


using namespace das;
using namespace StdGuiRender;

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

#define BIND_MEMBER_SIGNATURE(CLASS_NAME, FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE) \
  {                                                                                   \
    using memberMethod = das::das_call_member<SIGNATURE, &CLASS_NAME::FUNC_NAME>;     \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT,   \
      "das_call_member<" #SIGNATURE ", &CLASS_NAME::" #FUNC_NAME ">::invoke");        \
  }


namespace darg
{


struct GuiContextAnnotation : ManagedStructureAnnotation<GuiContext>
{
  GuiContextAnnotation(ModuleLibrary &ml) : ManagedStructureAnnotation("GuiContext", ml, "GuiContext") {}
};

struct RenderStateAnnotation : ManagedStructureAnnotation<RenderState>
{
  RenderStateAnnotation(ModuleLibrary &ml) : ManagedStructureAnnotation("RenderState", ml, "RenderState") { ADD_FIELD(opacity); }
};

struct ElemRenderDataAnnotation : ManagedStructureAnnotation<ElemRenderData>
{
  ElemRenderDataAnnotation(ModuleLibrary &ml) : ManagedStructureAnnotation("ElemRenderData", ml, "ElemRenderData")
  {
    ADD_FIELD(pos);
    ADD_FIELD(size);
  }
};

struct ElementAnnotation : ManagedStructureAnnotation<Element>
{
  ElementAnnotation(ModuleLibrary &ml) : ManagedStructureAnnotation("Element", ml, "Element") { ADD_FIELD(props); }
};

struct PropertiesAnnotation : ManagedStructureAnnotation<Properties>
{
  PropertiesAnnotation(ModuleLibrary &ml) : ManagedStructureAnnotation("Properties", ml, "Properties") {}
};


struct StdGuiFontContextAnnotation : ManagedValueAnnotation<StdGuiFontContext>
{
  StdGuiFontContextAnnotation(ModuleLibrary &ml) : ManagedValueAnnotation(ml, "StdGuiFontContext", " ::StdGuiFontContext") {}

  virtual bool hasNonTrivialCtor() const override
  {
    // don't need custom ctor in script, default is enough
    static_assert(eastl::is_trivially_destructible<StdGuiFontContext>::value);
    return false;
  }
};


struct GuiVertexTransformAnnotation : ManagedStructureAnnotation<GuiVertexTransform>
{
  GuiVertexTransformAnnotation(ModuleLibrary &ml) : ManagedStructureAnnotation("GuiVertexTransform", ml, "GuiVertexTransform") {}
};


static void render_line_aa(GuiContext &ctx, const das::TArray<Point2> &points, bool is_closed, float line_width,
  const Point2 line_indent, E3DCOLOR color)
{
  ctx.render_line_aa((const Point2 *)points.data, points.size, is_closed, line_width, line_indent, color);
}

static void render_poly(GuiContext &ctx, const das::TArray<Point2> &points, E3DCOLOR fill_color)
{
  ctx.render_poly(make_span_const((const Point2 *)points.data, points.size), fill_color);
}


static void render_inverse_poly(GuiContext &ctx, const das::TArray<Point2> &points_ccw, E3DCOLOR fill_color, const Point2 &left_top,
  const Point2 &right_bottom)
{
  ctx.render_inverse_poly(make_span_const((const Point2 *)points_ccw.data, points_ccw.size), fill_color, left_top, right_bottom);
}

static void get_view_tm(const GuiContext &ctx, GuiVertexTransform &gvtm, bool pure_trans) { ctx.getViewTm(gvtm.vtm, pure_trans); }

static void set_view_tm(GuiContext &ctx, const GuiVertexTransform &gvtm) { ctx.setViewTm(gvtm.vtm); }

/* Properties */

static int props_get_int(const Properties &props, const char *key, int def)
{
  return props.getInt(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

static float props_get_float(const Properties &props, const char *key, float def)
{
  return props.getFloat(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

static bool props_get_bool(const Properties &props, const char *key, bool def)
{
  return props.getBool(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}

static E3DCOLOR props_get_color(const Properties &props, const char *key, E3DCOLOR def)
{
  return props.getColor(Sqrat::Object(key, props.scriptDesc.GetVM()), def);
}


class ModuleDarg : public Module
{
public:
  ModuleDarg() : Module("darg")
  {
    ModuleLibrary lib(this);
    lib.addBuiltInModule();
    addBuiltinDependency(lib, require("DagorMath"));

    addAnnotation(make_smart<GuiContextAnnotation>(lib));
    addAnnotation(make_smart<StdGuiFontContextAnnotation>(lib));
    addAnnotation(make_smart<ElemRenderDataAnnotation>(lib));
    addAnnotation(make_smart<RenderStateAnnotation>(lib));
    addAnnotation(make_smart<PropertiesAnnotation>(lib));
    addAnnotation(make_smart<ElementAnnotation>(lib));
    addAnnotation(make_smart<GuiVertexTransformAnnotation>(lib));

    addEnumeration(make_smart<EnumerationFontFxType>());

    BIND_MEMBER_SIGNATURE(GuiContext, set_color, "set_color", das::SideEffects::modifyArgument,
      void(GuiContext::*)(int, int, int, int));
    BIND_MEMBER_SIGNATURE(GuiContext, set_color, "set_color", das::SideEffects::modifyArgument, void(GuiContext::*)(const E3DCOLOR &));

    BIND_MEMBER(GuiContext, screen_width, "screen_width", das::SideEffects::none);
    BIND_MEMBER(GuiContext, screen_height, "screen_height", das::SideEffects::none);

    BIND_MEMBER(GuiContext, render_frame, "render_frame", das::SideEffects::modifyArgument);
    BIND_MEMBER_SIGNATURE(GuiContext, render_box, "render_box", das::SideEffects::modifyArgument,
      void(GuiContext::*)(real, real, real, real));
    BIND_MEMBER_SIGNATURE(GuiContext, render_box, "render_box", das::SideEffects::modifyArgument,
      void(GuiContext::*)(const Point2 &, const Point2 &));

    BIND_MEMBER(GuiContext, render_rounded_box, "render_rounded_box", das::SideEffects::modifyArgument);
    BIND_MEMBER(GuiContext, render_rounded_frame, "render_rounded_frame", das::SideEffects::modifyArgument);

    BIND_MEMBER_SIGNATURE(GuiContext, draw_line, "draw_line", das::SideEffects::modifyArgument,
      void(GuiContext::*)(real, real, real, real, real));
    BIND_MEMBER_SIGNATURE(GuiContext, draw_line, "draw_line", das::SideEffects::modifyArgument,
      void(GuiContext::*)(const Point2 &, const Point2 &, real));

    addExtern<DAS_BIND_FUN(render_line_aa)>(*this, lib, "render_line_aa", SideEffects::modifyArgument, "render_line_aa");
    addExtern<DAS_BIND_FUN(render_poly)>(*this, lib, "render_poly", SideEffects::modifyArgument, "render_poly");
    addExtern<DAS_BIND_FUN(render_inverse_poly)>(*this, lib, "render_inverse_poly", SideEffects::modifyArgument,
      "render_inverse_poly");

    BIND_MEMBER_SIGNATURE(GuiContext, render_ellipse_aa, "render_ellipse_aa", das::SideEffects::modifyArgument,
      void(GuiContext::*)(Point2, Point2, float, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(GuiContext, render_ellipse_aa, "render_ellipse_aa", das::SideEffects::modifyArgument,
      void(GuiContext::*)(Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(GuiContext, render_sector_aa, "render_sector_aa", das::SideEffects::modifyArgument,
      void(GuiContext::*)(Point2, Point2, Point2, float, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(GuiContext, render_sector_aa, "render_sector_aa", das::SideEffects::modifyArgument,
      void(GuiContext::*)(Point2, Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(GuiContext, render_rectangle_aa, "render_rectangle_aa", das::SideEffects::modifyArgument,
      void(GuiContext::*)(Point2, Point2, float, E3DCOLOR, E3DCOLOR));
    BIND_MEMBER_SIGNATURE(GuiContext, render_rectangle_aa, "render_rectangle_aa", das::SideEffects::modifyArgument,
      void(GuiContext::*)(Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR));

    BIND_MEMBER(GuiContext, render_smooth_round_rect, "render_smooth_round_rect", das::SideEffects::modifyArgument);

    BIND_MEMBER_SIGNATURE(GuiContext, goto_xy, "goto_xy", das::SideEffects::modifyArgument, void(GuiContext::*)(const Point2 &));
    BIND_MEMBER_SIGNATURE(GuiContext, goto_xy, "goto_xy", das::SideEffects::modifyArgument, void(GuiContext::*)(real, real));

    BIND_MEMBER(GuiContext, set_spacing, "set_spacing", das::SideEffects::modifyArgument);
    BIND_MEMBER(GuiContext, set_mono_width, "set_mono_width", das::SideEffects::modifyArgument);
    BIND_MEMBER(GuiContext, get_spacing, "get_spacing", das::SideEffects::none);
    BIND_MEMBER(GuiContext, set_font, "set_font", das::SideEffects::modifyArgument);
    BIND_MEMBER(GuiContext, set_font_ht, "set_font_ht", das::SideEffects::modifyArgument);

    BIND_MEMBER(GuiContext, set_draw_str_attr, "set_draw_str_attr", das::SideEffects::modifyArgument);
    BIND_MEMBER(GuiContext, reset_draw_str_attr, "reset_draw_str_attr", das::SideEffects::modifyArgument);

    // BIND_MEMBER(GuiContext, draw_char_u, "draw_char_u", das::SideEffects::modifyArgument);
    BIND_MEMBER(GuiContext, draw_str_scaled, "draw_str_scaled", das::SideEffects::modifyArgument);
    BIND_MEMBER(GuiContext, draw_str, "draw_str", das::SideEffects::modifyArgument);

    // Vertex transform

    BIND_MEMBER(GuiContext, resetViewTm, "resetViewTm", das::SideEffects::modifyArgument);
    addExtern<DAS_BIND_FUN(set_view_tm)>(*this, lib, "setViewTm", SideEffects::modifyArgument, "setViewTm");
    BIND_MEMBER_SIGNATURE(GuiContext, setViewTm, "setViewTm", das::SideEffects::modifyArgument,
      void(GuiContext::*)(const Point2 &, const Point2 &, const Point2 &, bool));
    BIND_MEMBER(GuiContext, setRotViewTm, "setRotViewTm", das::SideEffects::modifyArgument);
    addExtern<DAS_BIND_FUN(get_view_tm)>(*this, lib, "getViewTm", SideEffects::modifyArgument, "getViewTm");


    // StdGuiRender globals

    addExtern<DAS_BIND_FUN(get_font_id)>(*this, lib, "get_font_id", SideEffects::none, "get_font_id");
    addExtern<DAS_BIND_FUN(get_font_context)>(*this, lib, "get_font_context", SideEffects::modifyArgument, "get_font_context");

    addExtern<DAS_BIND_FUN(get_str_bbox), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_str_bbox", SideEffects::none,
      "get_str_bbox");
    addExtern<Point2 (*)(const StdGuiFontContext &), get_font_cell_size>(*this, lib, "get_font_cell_size", SideEffects::none,
      "get_font_cell_size");
    addExtern<int (*)(const StdGuiFontContext &), get_font_caps_ht>(*this, lib, "get_font_caps_ht", SideEffects::none,
      "get_font_caps_ht");
    addExtern<real (*)(const StdGuiFontContext &), get_font_line_spacing>(*this, lib, "get_font_line_spacing", SideEffects::none,
      "get_font_line_spacing");
    addExtern<int (*)(const StdGuiFontContext &), get_font_ascent>(*this, lib, "get_font_ascent", SideEffects::none,
      "get_font_ascent");
    addExtern<int (*)(const StdGuiFontContext &), get_font_descent>(*this, lib, "get_font_descent", SideEffects::none,
      "get_font_descent");

    // Properties

    addExtern<DAS_BIND_FUN(props_get_int)>(*this, lib, "getInt", SideEffects::none, "getInt");
    addExtern<DAS_BIND_FUN(props_get_float)>(*this, lib, "getFloat", SideEffects::none, "getFloat");
    addExtern<DAS_BIND_FUN(props_get_bool)>(*this, lib, "getBool", SideEffects::none, "getBool");
    addExtern<DAS_BIND_FUN(props_get_color)>(*this, lib, "getColor", SideEffects::none, "getColor");
    BIND_MEMBER(Properties, getFontId, "getFontId", das::SideEffects::none);
    BIND_MEMBER(Properties, getFontSize, "getFontSize", das::SideEffects::none);

    // GuiVertexTransform
    BIND_MEMBER(GuiVertexTransform, resetViewTm, "resetViewTm", das::SideEffects::modifyArgument);
    BIND_MEMBER_SIGNATURE(GuiVertexTransform, setViewTm, "setViewTm", das::SideEffects::modifyArgument,
      void(GuiVertexTransform::*)(const Point2 &, const Point2 &, const Point2 &));
    BIND_MEMBER(GuiVertexTransform, addViewTm, "addViewTm", das::SideEffects::modifyArgument);
  }
};

} // namespace darg

REGISTER_MODULE_IN_NAMESPACE(ModuleDarg, darg);
