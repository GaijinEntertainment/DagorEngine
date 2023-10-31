#include "scriptBinding.h"
#include "guiScene.h"
#include "dargDebugUtils.h"

#include <daRg/dag_element.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_picture.h>
#include <daRg/dag_stringKeys.h>


#include "animation.h"
#include "behaviors/bhvButton.h"
#include "behaviors/bhvComboPopup.h"
#include "behaviors/bhvTextArea.h"
#include "behaviors/bhvTextInput.h"
#include "behaviors/bhvSlider.h"
#include "behaviors/bhvPannable.h"
#include "behaviors/bhvPannable2Touch.h"
#include "behaviors/bhvSwipeScroll.h"
#include "behaviors/bhvMoveResize.h"
#include "behaviors/bhvSmoothScrollStack.h"
#include "behaviors/bhvMarquee.h"
#include "behaviors/bhvWheelScroll.h"
#include "behaviors/bhvScrollEvent.h"
#include "behaviors/bhvInspectPicker.h"
#include "behaviors/bhvRtPropUpdate.h"
#include "behaviors/bhvRecalcHandler.h"
#include "behaviors/bhvDragAndDrop.h"
#include "behaviors/bhvEatInput.h"
#include <daRg/bhvFpsBar.h>
#include <daRg/bhvLatencyBar.h>
#include "behaviors/bhvOverlayTransparency.h"
#include "behaviors/bhvBoundToArea.h"
#include "behaviors/bhvMovie.h"
#include "behaviors/bhvParallax.h"
#include "behaviors/bhvPieMenu.h"
#include "behaviors/bhvTransitionSize.h"
#include "behaviors/bhvTrackMouse.h"
#include "behaviors/bhvMoveToArea.h"
#include "behaviors/bhvProcessPointingInput.h"
#include "guiGlobals.h"

#include "scriptUtil.h"
#include "elemGroup.h"
#include "scrollHandler.h"
#include "eventData.h"
#include "hotkeys.h"
#include "joystickAxisObservable.h"
#include "cursor.h"
#include "elementRef.h"

#include "textLayout.h"
#include "textUtil.h"

#include "dasScripts.h"

#include <videoPlayer/dag_videoPlayer.h>
#include <sound/dag_genAudio.h>

#include <sqModules/sqModules.h>
#include <humanInput/dag_hiXInputMappings.h>

using namespace sqfrp;

namespace Sqrat
{

template <>
struct InstanceToString<darg::Picture>
{
  static SQInteger Format(HSQUIRRELVM vm)
  {
    if (!Sqrat::check_signature<darg::Picture *>(vm))
      return SQ_ERROR;

    Sqrat::Var<darg::Picture *> var(vm, 1);
    sq_pushstring(vm, var.value->getName(), -1);
    return 1;
  }
};

template <>
struct InstanceToString<darg::PictureImmediate>
{
  static SQInteger Format(HSQUIRRELVM vm)
  {
    if (!Sqrat::check_signature<darg::PictureImmediate *>(vm))
      return SQ_ERROR;

    Sqrat::Var<darg::PictureImmediate *> var(vm, 1);
    sq_pushstring(vm, var.value->getName(), -1);
    return 1;
  }
};

} // namespace Sqrat


namespace darg
{

namespace binding
{


static void register_constants(HSQUIRRELVM vm)
{
#define CONST(x)             .Const(#x, x)
#define EASING(x)            .Const(#x, easing::x)
#define ELEM_CONST(x)        .Const(#x, Element::x)
#define MOVE_RESIZE(x)       .Const(#x, BhvMoveResize::x)
#define PREFORMATTED_FLAG(x) .Const(#x, textlayout::x)
#define AXIS(x)              .Const(#x, HumanInput::JOY_XINPUT_REAL_##x)

  Sqrat::ConstTable constTbl = Sqrat::ConstTable(vm);
  ///@module daRg
  constTbl CONST(FLOW_PARENT_RELATIVE) CONST(FLOW_HORIZONTAL) CONST(FLOW_VERTICAL)

    .Const("ALIGN_LEFT", ALIGN_LEFT)
    .Const("ALIGN_TOP", ALIGN_TOP)
    .Const("ALIGN_RIGHT", ALIGN_RIGHT)
    .Const("ALIGN_BOTTOM", ALIGN_BOTTOM)
    .Const("ALIGN_CENTER", ALIGN_CENTER)

      CONST(VECTOR_WIDTH) CONST(VECTOR_COLOR) CONST(VECTOR_FILL_COLOR) CONST(VECTOR_MID_COLOR) CONST(VECTOR_OUTER_LINE)
        CONST(VECTOR_CENTER_LINE) CONST(VECTOR_INNER_LINE) CONST(VECTOR_TM_OFFSET) CONST(VECTOR_TM_SCALE) CONST(VECTOR_LINE)
          CONST(VECTOR_LINE_INDENT_PX) CONST(VECTOR_LINE_INDENT_PCT) CONST(VECTOR_ELLIPSE) CONST(VECTOR_SECTOR) CONST(VECTOR_RECTANGLE)
            CONST(VECTOR_POLY) CONST(VECTOR_INVERSE_POLY) CONST(VECTOR_OPACITY) CONST(VECTOR_LINE_DASHED) CONST(VECTOR_NOP)
              CONST(VECTOR_QUADS)

                CONST(FFT_NONE) CONST(FFT_SHADOW) CONST(FFT_GLOW) CONST(FFT_BLUR) CONST(FFT_OUTLINE)

                  CONST(O_HORIZONTAL) CONST(O_VERTICAL)

                    CONST(TOVERFLOW_CLIP) CONST(TOVERFLOW_CHAR) CONST(TOVERFLOW_WORD) CONST(TOVERFLOW_LINE)

                      CONST(DIR_UP) CONST(DIR_DOWN) CONST(DIR_LEFT) CONST(DIR_RIGHT)

    .Const("EVENT_BREAK", GuiScene::EVENT_BREAK)
    .Const("EVENT_CONTINUE", GuiScene::EVENT_CONTINUE)

    ///@const Linear
    /// Easing function for prebuilt animations functions.
    EASING(Linear)
    ///@const InQuad
    /// Easing function for prebuilt animations functions.
    EASING(InQuad)
    ///@const OutQuad
    /// Easing function for prebuilt animations functions.
    EASING(OutQuad)
    ///@const InOutQuad
    /// Easing function for prebuilt animations functions.
    EASING(InOutQuad)
    ///@const InCubic
    /// Easing function for prebuilt animations functions.
    EASING(InCubic)
    ///@const OutCubic
    /// Easing function for prebuilt animations functions.
    EASING(OutCubic)
    ///@const InOutCubic
    /// Easing function for prebuilt animations functions.
    EASING(InOutCubic)
    ///@const InQuintic
    /// Easing function for prebuilt animations functions.
    EASING(InQuintic)
    ///@const OutQuintic
    /// Easing function for prebuilt animations functions.
    EASING(OutQuintic)
    ///@const InOutQuintic
    /// Easing function for prebuilt animations functions.
    EASING(InOutQuintic)
    ///@const InQuart
    /// Easing function for prebuilt animations functions.
    EASING(InQuart)
    ///@const OutQuart
    /// Easing function for prebuilt animations functions.
    EASING(OutQuart)
    ///@const InOutQuart
    /// Easing function for prebuilt animations functions.
    EASING(InOutQuart)
    ///@const InSine
    /// Easing function for prebuilt animations functions.
    EASING(InSine)
    ///@const OutSine
    /// Easing function for prebuilt animations functions.
    EASING(OutSine)
    ///@const InOutSine
    /// Easing function for prebuilt animations functions.
    EASING(InOutSine)
    ///@const InCirc
    /// Easing function for prebuilt animations functions.
    EASING(InCirc)
    ///@const OutCirc
    /// Easing function for prebuilt animations functions.
    EASING(OutCirc)
    ///@const InOutCirc
    /// Easing function for prebuilt animations functions.
    EASING(InOutCirc)
    ///@const InExp
    /// Easing function for prebuilt animations functions.
    EASING(InExp)
    ///@const OutExp
    /// Easing function for prebuilt animations functions.
    EASING(OutExp)
    ///@const InOutExp
    /// Easing function for prebuilt animations functions.
    EASING(InOutExp)
    ///@const InElastic
    /// Easing function for prebuilt animations functions.
    EASING(InElastic)
    ///@const OutElastic
    /// Easing function for prebuilt animations functions.
    EASING(OutElastic)
    ///@const InOutElastic
    /// Easing function for prebuilt animations functions.
    EASING(InOutElastic)
    ///@const InBack
    /// Easing function for prebuilt animations functions.
    EASING(InBack)
    ///@const OutBack
    /// Easing function for prebuilt animations functions.
    EASING(OutBack)
    ///@const InOutBack
    /// Easing function for prebuilt animations functions.
    EASING(InOutBack)
    ///@const InBounce
    /// Easing function for prebuilt animations functions.
    EASING(InBounce)
    ///@const OutBounce
    /// Easing function for prebuilt animations functions.
    EASING(OutBounce)
    ///@const InOutBounce
    /// Easing function for prebuilt animations functions.
    EASING(InOutBounce)
    ///@const InOutBezier
    /// Easing function for prebuilt animations functions.
    EASING(InOutBezier)
    ///@const CosineFull
    /// Easing function for prebuilt animations functions.
    EASING(CosineFull)
    ///@const InStep
    /// Easing function for prebuilt animations functions.
    EASING(InStep)
    ///@const OutStep
    /// Easing function for prebuilt animations functions.
    EASING(OutStep)
    ///@const Blink
    /// Easing function for prebuilt animations functions.
    EASING(Blink)
    ///@const DoubleBlink
    /// Easing function for prebuilt animations functions.
    EASING(DoubleBlink)
    ///@const BlinkSin
    /// Easing function for prebuilt animations functions.
    EASING(BlinkSin)
    ///@const BlinkCos
    /// Easing function for prebuilt animations functions.
    EASING(BlinkCos)
    ///@const Discrete8
    /// Easing function for prebuilt animations functions.
    EASING(Discrete8)
    ///@const Shake4
    /// Easing function for prebuilt animations functions.
    EASING(Shake4)
    ///@const Shake6
    /// Easing function for prebuilt animations functions.
    EASING(Shake6)

      ELEM_CONST(S_KB_FOCUS)
    /// State flag for keyboard focus.
    ELEM_CONST(S_HOVER)
    /// state flag for hover with pointing device (mouse)
    ELEM_CONST(S_TOP_HOVER)
    /// state flag for hover with pointing device (mouse)
    ELEM_CONST(S_ACTIVE)
    /// state flag for 'pressed' button.
    ELEM_CONST(S_MOUSE_ACTIVE)
    /// state flag for 'pressed' button with mouse.
    ELEM_CONST(S_KBD_ACTIVE)
    /// state flag for 'pressed' button with keyboard.
    ELEM_CONST(S_HOTKEY_ACTIVE)
    /// state flag for 'pressed' button with hotkey.
    ELEM_CONST(S_TOUCH_ACTIVE)
    /// state flag for pressed button with touch device.
    ELEM_CONST(S_JOYSTICK_ACTIVE)
    /// state flag for pressed button with joystick/gamepad device.
    ELEM_CONST(S_VR_ACTIVE)
    /// state flag for pressed button with VR controller.
    ELEM_CONST(S_DRAG)
    /// state flag for dragged state.

    ///@const MR_NONE
    /// MOVE_RESIZE Beahvior constants, to define where was point started move or resize
    /// R(ight), L(eft), B(ottom), T(op). Area is for point in the box (for Move).
    MOVE_RESIZE(MR_NONE)
    ///@const MR_T
    MOVE_RESIZE(MR_T)
    ///@const MR_R
    MOVE_RESIZE(MR_R)
    ///@const MR_B
    MOVE_RESIZE(MR_B)
    ///@const MR_L
    MOVE_RESIZE(MR_L)
    ///@const MR_LT
    MOVE_RESIZE(MR_LT)
    ///@const MR_RT
    MOVE_RESIZE(MR_RT)
    ///@const MR_LB
    MOVE_RESIZE(MR_LB)
    ///@const MR_RB
    MOVE_RESIZE(MR_RB)
    ///@const MR_AREA
    MOVE_RESIZE(MR_AREA)

    ///@const FMT_NO_WRAP
    PREFORMATTED_FLAG(FMT_NO_WRAP)
    ///@const FMT_KEEP_SPACES
    PREFORMATTED_FLAG(FMT_KEEP_SPACES)
    ///@const FMT_IGNORE_TAGS
    PREFORMATTED_FLAG(FMT_IGNORE_TAGS)
    ///@const FMT_HIDE_ELLIPSIS
    PREFORMATTED_FLAG(FMT_HIDE_ELLIPSIS)
    ///@const FMT_AS_IS
    PREFORMATTED_FLAG(FMT_AS_IS)

      CONST(DEVID_KEYBOARD) CONST(DEVID_MOUSE) CONST(DEVID_JOYSTICK) CONST(DEVID_TOUCH) CONST(DEVID_VR)

    ///@const KEEP_ASPECT_NONE
    CONST(KEEP_ASPECT_NONE)
    /** @const KEEP_ASPECT_FIT
For Picture.

Keeps Aspect ratio and fit component size (so there can be fields around picture,
but only in one direction, like on top and bottom or right and left.

    */
    CONST(KEEP_ASPECT_FIT)
    /** @const KEEP_ASPECT_FILL
For Picture.

Keeps Aspect ratio and cover all component size. So image can be outside component box.
(You can clip them with clipChildren = true in component above)

    */
    CONST(KEEP_ASPECT_FILL)
    ///@const AXIS_L_THUMB_H
    AXIS(AXIS_L_THUMB_H)
    ///@const AXIS_L_THUMB_V
    AXIS(AXIS_L_THUMB_V)
    ///@const AXIS_R_THUMB_H
    AXIS(AXIS_R_THUMB_H)
    ///@const AXIS_R_THUMB_V
    AXIS(AXIS_R_THUMB_V)
    ///@const AXIS_L_TRIGGER
    AXIS(AXIS_L_TRIGGER)
    ///@const AXIS_R_TRIGGER
    AXIS(AXIS_R_TRIGGER)
    ///@const AXIS_LR_TRIGGER
    AXIS(AXIS_LR_TRIGGER)

      CONST(XMB_STOP) CONST(XMB_CONTINUE)

        CONST(R_PROCESSED);

  ///@const daRg/SIZE_TO_CONTENT
  script_set_userpointer_to_slot(constTbl, "SIZE_TO_CONTENT", (SQUserPointer)SizeSpec::CONTENT); //-V566

#undef CONST
#undef EASING
#undef ELEM_CONST
#undef MOVE_RESIZE
#undef AXIS
}


static SQInteger get_def_font(HSQUIRRELVM vm)
{
  const SQChar *name;
  sq_getstring(vm, 2, &name);
  logerr("UI font %s not found", name ? name : "<n/a>");
  sq_pushinteger(vm, 0);
  return 1;
}


static void register_fonts(Sqrat::Table &exports)
{
  HSQUIRRELVM vm = exports.GetVM();

  SqStackChecker chk(vm);

  (void)sq_newclass(vm, SQFalse);
  for (auto &f : StdGuiRender::get_fonts_list())
  {
    sq_pushstring(vm, f.fontName, -1);
    sq_pushinteger(vm, f.fontId);
    sq_rawset(vm, -3);
  }

  sq_pushstring(vm, "_get", -1);
  sq_newclosure(vm, get_def_font, 0);
  sq_setparamscheck(vm, 2, ".s");
  sq_rawset(vm, -3);

  sq_pushnull(vm); // this
  SQRAT_VERIFY(SQ_SUCCEEDED(sq_call(vm, 1, SQTrue, SQTrue)));

  Sqrat::Var<Sqrat::Object> fonts(vm, -1);
  exports.SetValue("Fonts", fonts.value);

  sq_pop(vm, 2); // instance and class
}


static void register_anim_props(HSQUIRRELVM vm)
{
  Sqrat::Table tblAnimProp(vm);
  ///@enum daRg/AnimProp
  tblAnimProp.SetValue("color", AP_COLOR)
    .SetValue("bgColor", AP_BG_COLOR)
    .SetValue("fgColor", AP_FG_COLOR)
    .SetValue("fillColor", AP_FILL_COLOR)
    .SetValue("borderColor", AP_BORDER_COLOR)
    .SetValue("opacity", AP_OPACITY)
    .SetValue("rotate", AP_ROTATE)
    .SetValue("scale", AP_SCALE)
    .SetValue("translate", AP_TRANSLATE)
    .SetValue("fValue", AP_FVALUE)
    .SetValue("picSaturate", AP_PICSATURATE)
    .SetValue("brightness", AP_BRIGHTNESS);
  ///@resetscope
  Sqrat::Table(Sqrat::ConstTable(vm)).SetValue("AnimProp", tblAnimProp);
}

static float calc_screen_w_percent(float percent) { return floor(StdGuiRender::screen_width() * percent * 0.01f + 0.5f); }


float calc_screen_h_percent(float percent) { return floor(StdGuiRender::screen_height() * percent * 0.01f + 0.5f); }


template <SizeSpec::Mode mode>
static SQInteger make_size(HSQUIRRELVM vm)
{
  float val = 0;
  sq_getfloat(vm, 2, &val);

  SQUserPointer up = sq_newuserdata(vm, sizeof(SizeSpec));
  SizeSpec *ss = reinterpret_cast<SizeSpec *>(up);
  ss->mode = mode;
  ss->value = val;
  return 1;
}


template <>
SQInteger make_size<SizeSpec::FLEX>(HSQUIRRELVM vm)
{
  float val = 1;
  if (sq_gettop(vm) >= 2)
    sq_getfloat(vm, 2, &val);

  SQUserPointer up = sq_newuserdata(vm, sizeof(SizeSpec));
  SizeSpec *ss = reinterpret_cast<SizeSpec *>(up);
  ss->mode = SizeSpec::FLEX;
  ss->value = val;
  return 1;
}


static void register_std_behaviors(HSQUIRRELVM vm)
{
  Sqrat::Table tblBhv = Sqrat::Table(Sqrat::ConstTable(vm)).RawGetSlot("Behaviors");
  if (tblBhv.IsNull())
    tblBhv = Sqrat::Table(vm);

#define BHV(name, obj) tblBhv.SetValue(#name, (darg::Behavior *)&obj);
  ///@table daRg/Behaviors
  ///@const Button
  BHV(Button, bhv_button)
  ///@const TextArea
  BHV(TextArea, bhv_text_area)
  ///@const TextInput
  BHV(TextInput, bhv_text_input)
  ///@const Slider
  BHV(Slider, bhv_slider)
  ///@const Pannable
  BHV(Pannable, bhv_pannable)
  ///@const Pannable2touch
  BHV(Pannable2touch, bhv_pannable_2touch)
  ///@const SwipeScroll
  BHV(SwipeScroll, bhv_swipe_scroll)
  ///@const MoveResize
  BHV(MoveResize, bhv_move_resize)
  ///@const ComboPopup
  BHV(ComboPopup, bhv_combo_popup)
  ///@const SmoothScrollStack
  BHV(SmoothScrollStack, bhv_smooth_scroll_stack)
  ///@const Marquee
  BHV(Marquee, bhv_marquee)
  ///@const WheelScroll
  BHV(WheelScroll, bhv_wheel_scroll)
  ///@const ScrollEvent
  BHV(ScrollEvent, bhv_scroll_event)
  ///@const InspectPicker
  BHV(InspectPicker, bhv_inspect_picker)
  ///@const RtPropUpdate
  BHV(RtPropUpdate, bhv_rt_prop_update)
  ///@const RecalcHandler
  BHV(RecalcHandler, bhv_recalc_handler)
  ///@const DragAndDrop
  BHV(DragAndDrop, bhv_drag_and_drop)
  ///@const FpsBar
  BHV(FpsBar, bhv_fps_bar)
  ///@const LatencyBar
  BHV(LatencyBar, bhv_latency_bar)
  ///@const OverlayTransparency
  BHV(OverlayTransparency, bhv_overlay_transparency)
  ///@const BoundToArea
  BHV(BoundToArea, bhv_bound_to_area)
  ///@const Movie
  BHV(Movie, bhv_movie)
  ///@const Parallax
  BHV(Parallax, bhv_parallax)
  ///@const PieMenu
  BHV(PieMenu, bhv_pie_menu)
  ///@const TransitionSize
  BHV(TransitionSize, bhv_transition_size)
  ///@const TrackMouse
  BHV(TrackMouse, bhv_track_mouse)
  ///@const MoveToArea
  BHV(MoveToArea, bhv_move_to_area)
  ///@const ProcessPointingInput
  BHV(ProcessPointingInput, bhv_process_pointing_input)
  ///@const EatInput
  BHV(EatInput, bhv_eat_input)

#undef BHV
  ///@resetscope
  Sqrat::Table(Sqrat::ConstTable(vm)).SetValue("Behaviors", tblBhv);
}


static SQInteger get_font_metrics(HSQUIRRELVM vm)
{
  SQInteger fontId;
  sq_getinteger(vm, 2, &fontId);

  SQFloat fontHt = 0;
  if (sq_gettop(vm) > 2)
    sq_getfloat(vm, 3, &fontHt);

  DagorFontBinDump *font = StdGuiRender::get_font(fontId);
  if (!font)
    fontId = 0;

  StdGuiFontContext fctx;
  StdGuiRender::get_font_context(fctx, fontId, 0, 0, (int)floorf(fontHt + 0.5f));
  BBox2 visBbox = StdGuiRender::get_char_visible_bbox_u('x', fctx);

  Sqrat::Table res(vm);
  res.SetValue("_def_fontHt", StdGuiRender::get_def_font_ht(fontId));
  res.SetValue("fontHt", fctx.fontHt);
  res.SetValue("capsHt", StdGuiRender::get_font_caps_ht(fctx));
  res.SetValue("lineSpacing", StdGuiRender::get_font_line_spacing(fctx));
  res.SetValue("ascent", StdGuiRender::get_font_ascent(fctx));
  res.SetValue("descent", StdGuiRender::get_font_descent(fctx));
  res.SetValue("lowercaseHeight", visBbox.width().y);

  sq_pushobject(vm, res);
  return 1;
}

static bool sq_set_def_font_ht(const char *font_name, int pix_ht)
{
  int fontId = StdGuiRender::get_font_id(font_name);
  if (fontId < 0)
    return false;
  return StdGuiRender::set_def_font_ht(fontId, pix_ht);
}
static int sq_get_def_font_ht(const char *font_name)
{
  int fontId = StdGuiRender::get_font_id(font_name);
  if (fontId < 0)
    return 0;
  return StdGuiRender::get_def_font_ht(fontId);
}
static int sq_get_initial_font_ht(const char *font_name)
{
  int fontId = StdGuiRender::get_font_id(font_name);
  if (fontId < 0)
    return 0;
  return StdGuiRender::get_initial_font_ht(fontId);
}


static void dbg_set_require_font_size_slot(bool on) { require_font_size_slot = on; }


SQInteger calc_str_box(HSQUIRRELVM vm)
{
  const char *text = nullptr;
  SQInteger textLen = -1;
  Sqrat::Object params(vm);

  SQInteger nArgs = sq_gettop(vm);
  SQObjectType arg2type = sq_gettype(vm, 2);

  SQInteger paramsStartArg = 2;
  if (arg2type == OT_STRING)
  {
    G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, 2, &text, &textLen)));
    paramsStartArg = 3;
  }

  if (nArgs >= 3)
  {
    SQObjectType arg3type = sq_gettype(vm, 3);
    if ((arg2type == OT_TABLE || arg2type == OT_CLOSURE || arg2type == OT_NATIVECLOSURE) &&
        (arg3type == OT_TABLE || arg3type == OT_CLOSURE || arg3type == OT_NATIVECLOSURE))
      return sq_throwerror(vm, "Invalid arguments: 2 tables or functions provided");
  }

  for (SQInteger arg = paramsStartArg; arg <= nArgs; ++arg)
  {
    SQObjectType tp = sq_gettype(vm, arg);
    if (tp == OT_NULL)
      continue;

    G_ASSERT(params.IsNull());

    if (tp == OT_TABLE)
      params = Sqrat::Var<Sqrat::Table>(vm, arg).value;
    else
    {
      G_ASSERT(tp == OT_CLOSURE || tp == OT_NATIVECLOSURE);
      Sqrat::Function f(vm, Sqrat::Object(vm), Sqrat::Var<Sqrat::Object>(vm, arg).value);
      if (!f.Evaluate(params))
        return sq_throwerror(vm, "Function call failed");
      if (params.GetType() != OT_TABLE && params.GetType() != OT_NULL)
        return sq_throwerror(vm, "Function must return table or null");
    }
  }


  GuiScene *scene = GuiScene::get_from_sqvm(vm);
  G_ASSERT_RETURN(scene, sq_throwerror(vm, "Internal error: no scene"));

  StringKeys *csk = scene->getStringKeys();

  if (text == nullptr)
  {
    Sqrat::Object objText = params.RawGetSlot(csk->text);
    if (objText.GetType() != OT_STRING)
      return sq_throwerror(vm, "No text string provided");
    Sqrat::Var<const char *> varText = objText.GetVar<const char *>();
    text = varText.value;
    textLen = varText.valueLen;
  }
  G_ASSERT(text);

  G_ASSERT(csk->font.GetType() == OT_STRING);
  int fontId = params.RawGetSlotValue(csk->font, scene->getConfig()->defaultFont);
  float fontSize = params.RawGetSlotValue(csk->fontSize, scene->getConfig()->defaultFontSize);
  int fontHt = (int)floorf(fontSize + 0.5f);
  int spacing = params.RawGetSlotValue(csk->spacing, 0);
  int monoWidth = font_mono_width_from_sq(fontId, fontHt, params.RawGetSlot(csk->monoWidth));

  Point2 box = calc_text_size(text, textLen, fontId, spacing, monoWidth, fontHt);

  Offsets padding;
  Sqrat::Object objPadding = params.RawGetSlot(csk->padding);
  const char *errMsg = nullptr;
  if (!script_parse_offsets(nullptr, objPadding, &padding.x, &errMsg))
    darg_assert_trace_var(errMsg, params, csk->padding);

  Sqrat::Array res(vm, 2);
  res.SetValue(SQInteger(0), box.x + padding.left() + padding.right());
  res.SetValue(SQInteger(1), box.y + padding.top() + padding.bottom());
  sq_pushobject(vm, res);
  return 1;
}


static SQInteger scrollhandler_ctor(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ScrollHandler *>(vm))
    return sq_throwerror(vm, "ScrollHandler constructor bad 'this'");
  ScrollHandler *inst = new ScrollHandler(vm);
  Sqrat::ClassType<ScrollHandler>::SetManagedInstance(vm, 1, inst);
  return 0;
}


void bind_script_classes(SqModules *module_mgr, Sqrat::Table &exports)
{
  HSQUIRRELVM sqvm = module_mgr->getVM();
  G_ASSERT(sqvm);
  G_ASSERT(sqvm == exports.GetVM());

  ///@class daRg/ScrollHandler
  Sqrat::DerivedClass<ScrollHandler, BaseObservable> scrollHandler(sqvm, "ScrollHandler");
  scrollHandler.SquirrelCtor(scrollhandler_ctor, 1, "x")
    .Func("scrollToX", &ScrollHandler::scrollToX)
    .Func("scrollToY", &ScrollHandler::scrollToY)
    .Func("scrollToChildren", &ScrollHandler::scrollToChildren)
    .Prop("elem", &ScrollHandler::getElem);

  ///@class daRg/JoystickAxisObservable
  Sqrat::DerivedClass<JoystickAxisObservable, BaseObservable> joyAxisObservable(sqvm, "JoystickAxisObservable");
  joyAxisObservable.Prop("value", &JoystickAxisObservable::getValue)
    .Var("resolution", &JoystickAxisObservable::resolution)
    .Var("deadzone", &JoystickAxisObservable::deadzone);

  ///@class daRg/ElemGroup
  Sqrat::Class<ElemGroup> elemGroup(sqvm, "ElemGroup");

  ///@class daRg/Behavior
  Sqrat::Class<Behavior, Sqrat::NoConstructor<Behavior>> behaviorClass(sqvm, "Behavior");

  ///@class daRg/Picture
  Sqrat::Class<Picture> pictureClass(sqvm, "Picture");
  pictureClass.SquirrelCtor(picture_script_ctor<Picture>, 0, ".o|s");

  ///@class daRg/Immediate
  ///@extends Picture
  Sqrat::DerivedClass<PictureImmediate, Picture> immediatePictureClass(sqvm, "PictureImmediate");
  immediatePictureClass.SquirrelCtor(picture_script_ctor<PictureImmediate>, 0, ".o|s");

  ///@class daRg/FormattedText
  Sqrat::Class<textlayout::FormattedText, Sqrat::NoConstructor<textlayout::FormattedText>> formattedTextClass(sqvm, "FormattedText");

  ///@class daRg/IGenVideoPlayer
  Sqrat::Class<IGenVideoPlayer, Sqrat::NoConstructor<IGenVideoPlayer>> sqVideoPlayer(sqvm, "IGenVideoPlayer");

  ///@class daRg/IGenSound
  Sqrat::Class<IGenSound, Sqrat::NoConstructor<IGenSound>> sqSoundPlayer(sqvm, "IGenSound");

  ///@class daRg/DragAndDropState
  Sqrat::Class<DragAndDropState, Sqrat::NoConstructor<DragAndDropState>> dragAndDropState(sqvm, "DragAndDropState");

  ///@class daRg/EventDataRect
  Sqrat::Class<EventDataRect, Sqrat::CopyOnly<EventDataRect>> sqRect(sqvm, "EventDataRect");
  sqRect.ConstVar("l", &EventDataRect::l)
    .ConstVar("t", &EventDataRect::t)
    .ConstVar("r", &EventDataRect::r)
    .ConstVar("b", &EventDataRect::b);

  ///@class daRg/MouseClickEventData
  Sqrat::Class<MouseClickEventData, Sqrat::DefaultAllocator<MouseClickEventData>> sqMouseClickEventData(sqvm, "MouseClickEventData");
#define V(n) .Var(#n, &MouseClickEventData::n)
  sqMouseClickEventData V(screenX) V(screenY) V(button) V(ctrlKey) V(shiftKey) V(altKey) V(devId) V(target).Prop("targetRect",
    &MouseClickEventData::getTargetRect);
#undef V

  ///@class daRg/HotkeyEventData
  Sqrat::Class<HotkeyEventData, Sqrat::DefaultAllocator<HotkeyEventData>> sqHotkeyEventData(sqvm, "HotkeyEventData");
  sqHotkeyEventData.Prop("targetRect", &HotkeyEventData::getTargetRect);

  ///@class daRg/HoverEventData
  Sqrat::Class<HoverEventData, Sqrat::DefaultAllocator<HoverEventData>> sqHoverEventData(sqvm, "HoverEventData");
  sqHoverEventData.Prop("targetRect", &HoverEventData::getTargetRect);

  Sqrat::Class<AllTransitionsConfig> sqTransitAll(sqvm, "TransitAll");
  sqTransitAll.Ctor<const Sqrat::Object &>();

#define REG_BHV_DATA(name) Sqrat::Class<name, Sqrat::NoConstructor<name>> sq##name(sqvm, #name);
  REG_BHV_DATA(BhvButtonData)
  REG_BHV_DATA(BhvPannableData)
  REG_BHV_DATA(BhvSwipeScrollData)
  REG_BHV_DATA(BhvPannable2touchData)
  REG_BHV_DATA(BhvTransitionSizeData)
  REG_BHV_DATA(BhvMarqueeData)
  REG_BHV_DATA(BhvSliderData)
  REG_BHV_DATA(BhvMoveResizeData)
  REG_BHV_DATA(BhvPieMenuData)
#undef REG_BHV_DATA

  ///@class daRg/MoveToAreaTarget
  Sqrat::Class<BhvMoveToAreaTarget> sqBhvMoveToAreaTarget(sqvm, "BhvMoveToAreaTarget");
  sqBhvMoveToAreaTarget // comments to supress clang-format and allow qdox to generate doc
    .Func("set", &BhvMoveToAreaTarget::set)
    .Func("clear", &BhvMoveToAreaTarget::clear);

  exports.Bind("ElemGroup", elemGroup);
  exports.Bind("ScrollHandler", scrollHandler);
  exports.Bind("Picture", pictureClass);
  exports.Bind("PictureImmediate", immediatePictureClass);
  exports.Bind("TransitAll", sqTransitAll);
  exports.Bind("MoveToAreaTarget", sqBhvMoveToAreaTarget);
  ///@module daRg
  exports.SquirrelFunc("Color", encode_e3dcolor, -4, ".nnnn")
    .Func("sw", calc_screen_w_percent)
    .Func("sh", calc_screen_h_percent)
    .SquirrelFunc("flex", make_size<SizeSpec::FLEX>, -1, ".n")
    .SquirrelFunc("fontH", make_size<SizeSpec::FONT_H>, 2, ".n")
    .SquirrelFunc("pw", make_size<SizeSpec::PARENT_W>, 2, ".n")
    .SquirrelFunc("ph", make_size<SizeSpec::PARENT_H>, 2, ".n")
    .SquirrelFunc("elemw", make_size<SizeSpec::ELEM_SELF_W>, 2, ".n")
    .SquirrelFunc("elemh", make_size<SizeSpec::ELEM_SELF_H>, 2, ".n")
    .SquirrelFunc("locate_element_source", locate_element_source, 2, ".x")
    .SquirrelFunc("get_element_info", get_element_info, 2, ".x")
    .SquirrelFunc("get_font_metrics", get_font_metrics, -2, ".i")
    /**
    @param fontId i
    @param fontHt f : optional, default is _def_fontHt font height set in font
    @return t : font_params
    @code font_params
      {
        _def_fontHt : def height
        fontHt : height
        capsHt : height of H
        lineSpacing : linespacing
        ascent
        descent
        lowercaseHeight : height of x
      }
    @code
    */
    .Func("setFontDefHt", sq_set_def_font_ht)
    .Func("getFontDefHt", sq_get_def_font_ht)
    .Func("getFontInitialHt", sq_get_initial_font_ht)
    .SquirrelFunc("calc_str_box", calc_str_box, -2, ". s|t|c t|c|o");
  /**
    @param element_or_text s|c|t : should be string of text or darg component description with 'text' in it
    @param element_or_style t|c|o : optional, if first element is text, it should be provided with font properties
    @return a : return array of two objects with width and height of text
  */

  register_constants(sqvm);
  register_fonts(exports);
  register_anim_props(sqvm);

  register_std_behaviors(sqvm);

  register_rendobj_script_ids(sqvm);

  Cursor::bind_script(exports);
  ElementRef::bind_script(sqvm);
  Sqrat::Class<Element, Sqrat::NoConstructor<Element>> sqElement(sqvm, "@Element");

  bind_das(exports);

  Sqrat::Table dargDebug(sqvm);
  ///@module daRg.debug
  dargDebug.Func("requireFontSizeSlot", dbg_set_require_font_size_slot);
  module_mgr->addNativeModule("daRg.debug", dargDebug);
}

} // namespace binding

} // namespace darg
