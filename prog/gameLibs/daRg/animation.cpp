#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_uiSound.h>
#include <daRg/dag_scriptHandlers.h>
#include <daRg/dag_helpers.h>
#include "animation.h"
#include "scriptUtil.h"
#include "guiScene.h"
#include "dargDebugUtils.h"
#include <perfMon/dag_cpuFreq.h>
#include <atomic>


namespace darg
{

static int64_t sec_to_usec(float t) { return int64_t(t * 1000000); }


static float lerp_hue(float from, float to, float k)
{
  float d = to - from;
  if (d > 180.f)
    d = d - 360.0f;
  else if (d < -180.f)
    d = 360.0f + d;
  return fmodf(from + d * k + 360.0f, 360.0f);
}

static ColorHsva lerp_hsva(const ColorHsva &from, const ColorHsva &to, float k)
{
  ColorHsva res;
  res.h = lerp_hue(from.h, to.h, k);
  res.s = lerp(from.s, to.s, k);
  res.v = lerp(from.v, to.v, k);
  res.a = lerp(from.a, to.a, k);
  return res;
}


AnimDesc::AnimDesc() { reset(); }


void AnimDesc::reset()
{
  prop = AP_INVALID;
  duration = 0;
  delay = 0;

  from.Release();
  to.Release();

  autoPlay = false;
  playFadeOut = false;
  loop = false;
  globalTimer = false;

  easingFunc = easing::functions[easing::Linear];
  sqEasingFunc.Release();

  trigger.Release();

  sound.Release();
  onExit.Release();
  onFinish.Release();
  onAbort.Release();
  onEnter.Release();
  onStart.Release();
}


// Provide shortest interpolation path in HSV in case
// if calculated hue or saturation is undefined
static void adjust_color_for_animation(ColorHsva &a, ColorHsva &b)
{
  const float eps = 1e-5f;
  if (a.s < eps && b.s >= eps)
    a.h = b.h;
  else if (b.s < eps && a.s >= eps)
    b.h = a.h;

  if (a.v < eps && b.v >= eps)
    a.s = b.s;
  else if (b.v < eps && a.v >= eps)
    b.s = a.s;
}


static bool read_easing(const Sqrat::Table &desc, const StringKeys *csk, easing::Func &easingFunc, Sqrat::Object &sqEasingFunc)
{
  easing::Mode easingMode = easing::Linear;
  sqEasingFunc.Release();

  bool ok = true;
  Sqrat::Object objEasing = desc.RawGetSlot(csk->easing);
  if (objEasing.GetType() == OT_INTEGER)
  {
    easingMode = objEasing.Cast<easing::Mode>();
    if (easingMode < 0 || easingMode >= easing::NUM_FUNCTIONS)
    {
      darg_assert_trace_var(String(0, "Invalid easing mode %d", easingMode), desc, csk->easing);
      easingMode = easing::Linear;
    }
  }
  else if (objEasing.GetType() == OT_CLOSURE || objEasing.GetType() == OT_NATIVECLOSURE)
  {
    sqEasingFunc = objEasing;
  }
  else if (objEasing.GetType() != OT_NULL)
  {
    darg_assert_trace_var("Invalid type for easing", desc, csk->easing);
    easingMode = easing::Linear;
    ok = false;
  }

  easingFunc = easing::functions[easingMode];
  return ok;
}


bool AnimDesc::load(const Sqrat::Table &desc, const StringKeys *csk)
{
  reset();

  prop = desc.RawGetSlotValue<AnimProp>(csk->prop, AP_INVALID);
  if (prop <= AP_INVALID || prop >= NUM_ANIM_PROPS)
  {
    darg_assert_trace_var(String(0, "Unexpected anim prop %d", prop), desc, csk->prop);
    return false;
  }

  duration = sec_to_usec(desc.RawGetSlotValue<float>(csk->duration, 10.0f));
  delay = sec_to_usec(desc.RawGetSlotValue<float>(csk->delay, 0.0f));
  autoPlay = desc.RawGetSlotValue<bool>(csk->play, false);
  playFadeOut = desc.RawGetSlotValue<bool>(csk->playFadeOut, false);
  loop = desc.RawGetSlotValue<bool>(csk->loop, false);
  globalTimer = desc.RawGetSlotValue<bool>(csk->globalTimer, false);
  trigger = desc.RawGetSlot(csk->trigger);
  sound = desc.RawGetSlot(csk->sound);
  onEnter = desc.RawGetSlot(csk->onEnter);
  onStart = desc.RawGetSlot(csk->onStart);
  onExit = desc.RawGetSlot(csk->onExit);
  onFinish = desc.RawGetSlot(csk->onFinish);
  onAbort = desc.RawGetSlot(csk->onAbort);

  from = desc.RawGetSlot(csk->from);
  to = desc.RawGetSlot(csk->to);

  read_easing(desc, csk, easingFunc, sqEasingFunc);

  return true;
}


Animation *create_anim(const AnimDesc &desc, Element *elem)
{
  Animation *anim = NULL;
  switch (desc.prop)
  {
    case AP_COLOR:
    case AP_BG_COLOR:
    case AP_FG_COLOR:
    case AP_FILL_COLOR:
    case AP_BORDER_COLOR: anim = new ColorAnim(elem); break;
    case AP_OPACITY:
    case AP_ROTATE:
    case AP_FVALUE:
    case AP_PICSATURATE:
    case AP_BRIGHTNESS: anim = new FloatAnim(elem); break;
    case AP_SCALE:
    case AP_TRANSLATE: anim = new Point2Anim(elem); break;
    default: G_ASSERTF(0, "Unexpected anim prop %d", desc.prop); break;
  }

  if (anim)
  {
    anim->init(desc);
  }

  return anim;
}


Animation::Animation(Element *elem_) : elem(elem_) {}


// In practice we should be fine without forcing atomicity, but just in case
static int64_t get_anim_ref_time()
{
  static int64_t refTime = 0;
  if (refTime != 0)
    return refTime;

  static std::atomic_flag lock = ATOMIC_FLAG_INIT;
  while (lock.test_and_set(std::memory_order_acquire))
    ;
  refTime = ref_time_ticks();
  lock.clear(std::memory_order_release);
  return refTime;
}


bool Animation::baseUpdate(int64_t dt)
{
  if (isFinished())
    return false;

  if (delayLeft > 0)
  {
    delayLeft -= dt;
    if (delayLeft <= 0)
    {
      t -= delayLeft;
      callHandler(desc.onStart, true);
      playSound(elem->csk->stop);
    }
  }
  else
  {
    t += dt;
  }

  if (isPlayingLoop)
  {
    if (desc.globalTimer)
      t = get_time_usec(get_anim_ref_time()) % desc.duration;
    else
      t = t % desc.duration;
  }

  if (isFinished())
  {
    callHandler(desc.onFinish, true);
    callHandler(desc.onExit, true);
    playSound(elem->csk->stop);
  }

  return true;
}


void Animation::callHandler(const Sqrat::Object &handler, bool allow_start)
{
  if (!handler.IsNull())
  {
    GuiScene *guiScene = GuiScene::get_from_elem(elem);
    SQObjectType tp = handler.GetType();
    if (tp == OT_ARRAY)
    {
      Sqrat::Array arr(handler);
      for (SQInteger i = 0, n = arr.Length(); i < n; ++i)
        callHandler(arr.RawGetSlot(i), allow_start);
    }
    else if (tp == OT_CLOSURE || tp == OT_NATIVECLOSURE)
    {
      HSQUIRRELVM vm = handler.GetVM();
      Sqrat::Function f(vm, Sqrat::Object(vm), handler.GetObject());
      guiScene->queueScriptHandler(new ScriptHandlerSqFunc<>(f));
    }
    else if (allow_start)
    {
      guiScene->getElementTree()->startAnimation(handler);
    }
  }
}


void Animation::skip()
{
  if (!isFinished())
  {
    isPlayingLoop = false;
    t = desc.duration;
    apply();
    callHandler(desc.onAbort, true);
    callHandler(desc.onExit, true);
  }
}


void Animation::skipOnDelete()
{
  if (!isFinished())
  {
    callHandler(desc.onAbort, false);
    callHandler(desc.onExit, false);
  }
}


void Animation::skipDelay()
{
  if (!isFinished())
    delayLeft = 0;
}


void Animation::init(const AnimDesc &desc_)
{
  desc = desc_;
  if (desc.autoPlay)
    start();
  else
    t = desc.duration;
}


bool Animation::isFinished() const { return t >= desc.duration; }


void Animation::start()
{
  bool doStart = false;

  if (desc.loop)
  {
    if (!isPlayingLoop)
    {
      isPlayingLoop = true;
      doStart = true;
    }
  }
  else
  {
    doStart = true;
  }

  if (doStart)
  {
    rewind();
    callHandler(desc.onEnter, true);
    if (delayLeft <= 0)
    {
      callHandler(desc.onStart, true);
      playSound(elem->csk->start);
    }
  }
}


void Animation::requestStop() { isPlayingLoop = false; }


void Animation::rewind()
{
  t = 0;
  delayLeft = desc.delay;
}


void Animation::playSound(const Sqrat::Object &key)
{
  if (!elem->etree->guiScene->isActive())
    return;

  if (desc.sound.IsNull())
    return;
  Sqrat::Object src = desc.sound.RawGetSlot(key);
  if (src.IsNull())
    return;

  Sqrat::string name;
  Sqrat::Object params;
  float vol;
  if (load_sound_info(src, name, params, vol))
  {
    Point2 sndPos;
    bool posOk = elem->calcSoundPos(sndPos);
    ui_sound_player->play(name.c_str(), params, vol, posOk ? &sndPos : nullptr);
  }
}


float Animation::applyEasing(float k)
{
  Sqrat::Object &sqfunc = desc.sqEasingFunc;
  if (sqfunc.IsNull())
    return desc.easingFunc(k);
  else
  {
    Sqrat::Function f(sqfunc.GetVM(), Sqrat::Object(sqfunc.GetVM()), sqfunc.GetObject());
    float res = k;
    f.Evaluate(k, res);
    return res;
  }
}


ColorAnim::ColorAnim(Element *elem) : Animation(elem), output(nullptr), baseElemColor(255, 255, 255, 255) {}


void ColorAnim::setup(bool /*initial*/)
{
  E3DCOLOR *ptr = nullptr;
  output = (elem->robjParams && elem->robjParams->getAnimColor(desc.prop, &ptr)) ? ptr : nullptr;

  if (!desc.fromElemProp())
    cFrom = rgb2hsv(color4(script_decode_e3dcolor(desc.from.Cast<SQInteger>())));
  else
    cFrom = ColorHsva(0, 0, 0, 0);

  if (!desc.toElemProp())
    cTo = rgb2hsv(color4(script_decode_e3dcolor(desc.to.Cast<SQInteger>())));
  else
    cTo = ColorHsva(0, 0, 0, 0);

  if (!desc.fromElemProp() && !desc.toElemProp())
    adjust_color_for_animation(cFrom, cTo);

  const Sqrat::Object *field = fieldForAnim(elem, desc.prop, true);
  if (field)
    baseElemColor = elem->props.getColor(*field);
}


const Sqrat::Object *ColorAnim::fieldForAnim(const Element *elem, AnimProp prop, bool assert_on_missing)
{
  switch (prop)
  {
    case AP_COLOR: return &elem->csk->color;
    case AP_BG_COLOR: return &elem->csk->bgColor;
    case AP_FG_COLOR: return &elem->csk->fgColor;
    case AP_FILL_COLOR: return &elem->csk->fillColor;
    case AP_BORDER_COLOR: return &elem->csk->borderColor;
    default: (void)assert_on_missing; G_ASSERTF(!assert_on_missing, "Unsupported anim prop %d", prop);
  }
  return nullptr;
}


void ColorAnim::update(int64_t dt)
{
  if (!Animation::baseUpdate(dt) || delayLeft > 0)
    return;
  apply();
}


void ColorAnim::apply()
{
  ColorHsva valFrom = cFrom;
  ColorHsva valTo = cTo;

  if (desc.fromElemProp())
    valFrom = rgb2hsv(baseElemColor);
  if (desc.toElemProp())
    valTo = rgb2hsv(baseElemColor);
  if (desc.fromElemProp() || desc.toElemProp())
    adjust_color_for_animation(valFrom, valTo);

  float k = clamp(float(t) / float(desc.duration), 0.0f, 1.0f);
  k = applyEasing(k);

  ColorHsva res = lerp_hsva(valFrom, valTo, k);
  E3DCOLOR val = e3dcolor(hsv2rgb(res));

  if (isFinished() && !desc.playFadeOut)
  {
    if (output)
      *output = baseElemColor;
  }
  else
  {
    if (output)
      *output = val;
  }
}


void FloatAnim::setup(bool /*initial*/)
{
  float *ptr = nullptr;
  output = (elem->robjParams && elem->robjParams->getAnimFloat(desc.prop, &ptr)) ? ptr : nullptr;
  fFrom = desc.fromElemProp() ? 0 : desc.from.Cast<float>();
  fTo = desc.toElemProp() ? 0 : desc.to.Cast<float>();
}


void FloatAnim::update(int64_t dt)
{
  if (!Animation::baseUpdate(dt) || delayLeft > 0)
    return;
  apply();
}


void FloatAnim::apply()
{
  float valFrom = fFrom;
  float valTo = fTo;

  switch (desc.prop)
  {
    case AP_OPACITY:
      if (desc.fromElemProp())
        valFrom = elem->props.getBaseOpacity();
      if (desc.toElemProp())
        valTo = elem->props.getBaseOpacity();
      break;

    case AP_ROTATE:
      if (desc.fromElemProp() && elem->transform)
        valFrom = elem->transform->rotate;
      if (desc.toElemProp() && elem->transform)
        valTo = elem->transform->rotate;
      break;

    case AP_FVALUE:
      if (desc.fromElemProp())
        valFrom = elem->props.getFloat(elem->csk->fValue, 0.0f);
      if (desc.toElemProp())
        valTo = elem->props.getFloat(elem->csk->fValue, 0.0f);
      break;

    case AP_PICSATURATE:
      if (desc.fromElemProp())
        valFrom = elem->props.getFloat(elem->csk->picSaturate, 1.0f);
      if (desc.toElemProp())
        valTo = elem->props.getFloat(elem->csk->picSaturate, 1.0f);
      break;

    case AP_BRIGHTNESS:
      if (desc.fromElemProp())
        valFrom = elem->props.getFloat(elem->csk->brightness, 1.0f);
      if (desc.toElemProp())
        valTo = elem->props.getFloat(elem->csk->brightness, 1.0f);
      break;

    default: G_ASSERTF(0, "Unsupported anim prop %d", desc.prop);
  }

  float k = clamp(float(t) / float(desc.duration), 0.0f, 1.0f);
  k = applyEasing(k);
  float val = lerp(valFrom, valTo, k);
  if (output)
    *output = val;

  switch (desc.prop)
  {
    case AP_OPACITY:
      if (isFinished() && !desc.playFadeOut)
        elem->props.resetCurrentOpacity();
      else
        elem->props.setCurrentOpacity(val);
      break;

    case AP_ROTATE:
      if (elem->transform)
      {
        if (isFinished() && !desc.playFadeOut)
          elem->transform->haveCurRotate = false;
        else
        {
          elem->transform->curRotate = DegToRad(val);
          elem->transform->haveCurRotate = true;
        }
      }
      break;

    case AP_FVALUE:
      if (isFinished() && !desc.playFadeOut)
        elem->props.resetOverride(elem->csk->fValue);
      else
        elem->props.overrideFloat(elem->csk->fValue, val);
      break;

    case AP_PICSATURATE:
    case AP_BRIGHTNESS:
      // it affects only rendobj native data, no need to adjust styles
      break;

    default: G_ASSERTF(0, "Unsupported anim prop %d", desc.prop);
  }
}


void Point2Anim::setup(bool /*initial*/)
{
  if (!desc.fromElemProp())
    p2From = script_get_point2(desc.from);
  else
    p2From.zero();

  if (!desc.toElemProp())
    p2To = script_get_point2(desc.to);
  else
    p2To.zero();
}


void Point2Anim::update(int64_t dt)
{
  if (!Animation::baseUpdate(dt) || delayLeft > 0)
    return;
  apply();
}


void Point2Anim::apply()
{
  Point2 valFrom = p2From;
  Point2 valTo = p2To;

  Point2 *srcField = NULL;
  if (elem->transform && desc.prop == AP_SCALE)
    srcField = &elem->transform->scale;
  else if (elem->transform && desc.prop == AP_TRANSLATE)
    srcField = &elem->transform->translate;

  if (srcField)
  {
    if (desc.fromElemProp())
      valFrom = *srcField;
    if (desc.toElemProp())
      valTo = *srcField;
  }


  float k = clamp(float(t) / float(desc.duration), 0.0f, 1.0f);
  k = applyEasing(k);
  Point2 val = lerp(valFrom, valTo, k);

  switch (desc.prop)
  {
    case AP_SCALE:
      if (elem->transform)
      {
        if (isFinished() && !desc.playFadeOut)
          elem->transform->haveCurScale = false;
        else
        {
          elem->transform->curScale = val;
          elem->transform->haveCurScale = true;
        }
      }

      break;

    case AP_TRANSLATE:
      if (elem->transform)
      {
        if (isFinished() && !desc.playFadeOut)
          elem->transform->haveCurTranslate = false;
        else
        {
          elem->transform->curTranslate = val;
          elem->transform->haveCurTranslate = true;
        }
      }
      break;

    default: G_ASSERTF(0, "Unsupported anim prop %d", desc.prop);
  }
}


Transition::Transition(AnimProp prop_, Element *elem_) : prop(prop_), elem(elem_) {}


void Transition::setup(const Sqrat::Table &cfg)
{
  tblConfig = cfg;

  cfgDuration = sec_to_usec(cfg.RawGetSlotValue(elem->csk->duration, 0.2f));

  read_easing(cfg, elem->csk, easingFunc, sqEasingFunc);
}


void Transition::callHandler(const Sqrat::Object &key)
{
  Sqrat::Function f(tblConfig.GetVM(), tblConfig.GetObject(), tblConfig.RawGetSlot(key).GetObject());
  if (!f.IsNull())
  {
    GuiScene *guiScene = GuiScene::get_from_sqvm(tblConfig.GetVM());
    guiScene->queueScriptHandler(new ScriptHandlerSqFunc<>(f));
  }
}


bool Transition::checkStart(AnimProp p, bool *reset)
{
  *reset = false;
  const Sqrat::Object *colorField = ColorAnim::fieldForAnim(elem, p, false);
  E3DCOLOR *colorOutput = nullptr;
  if (colorField)
  {
    E3DCOLOR *ptr = nullptr;
    colorOutput = (elem->robjParams && elem->robjParams->getAnimColor(p, &ptr)) ? ptr : nullptr;

    if (colorOutput)
    {
      E3DCOLOR elemColor = elem->props.getColor(*colorField, E3DCOLOR(0, 0, 0, 0));

      if (elemColor != lastElemColor || !lastValueValid)
      {
        if (!lastValueValid)
        {
          lastValueValid = true;
          lastElemColor = curColor = elemColor;
          *reset = true;
          return true;
        }

        colorFrom = rgb2hsv(curColor);
        colorTo = rgb2hsv(elemColor);
        adjust_color_for_animation(colorFrom, colorTo);
        lastElemColor = elemColor;

        return true;
      }
    }
  }
  else if (p == AP_OPACITY || p == AP_FVALUE || p == AP_ROTATE || p == AP_PICSATURATE || p == AP_BRIGHTNESS)
  {
    float elemFloat = 0.f;
    if (p == AP_OPACITY)
      elemFloat = elem->props.getBaseOpacity();
    else if (p == AP_FVALUE)
      elemFloat = elem->props.getFloat(elem->csk->fValue, 0.f);
    else if (p == AP_PICSATURATE)
      elemFloat = elem->props.getFloat(elem->csk->picSaturate, 1.f);
    else if (p == AP_BRIGHTNESS)
      elemFloat = elem->props.getFloat(elem->csk->brightness, 1.f);
    else if (p == AP_ROTATE && elem->transform)
      elemFloat = elem->transform->rotate;

    if (elemFloat != lastElemFloat || !lastValueValid)
    {
      if (!lastValueValid)
      {
        lastValueValid = true;
        lastElemFloat = curFloat = elemFloat;
        *reset = true;
        return true;
      }

      floatFrom = curFloat;
      floatTo = elemFloat;
      lastElemFloat = elemFloat;

      return true;
    }
  }
  else if (p == AP_SCALE || p == AP_TRANSLATE)
  {
    if (elem->transform)
    {
      const Point2 &elemP2 = (p == AP_SCALE) ? elem->transform->scale : elem->transform->translate;
      if (lastElemP2 != elemP2 || !lastValueValid)
      {
        if (!lastValueValid)
        {
          lastValueValid = true;
          lastElemP2 = curP2 = elemP2;
          *reset = true;
          return true;
        }

        p2From = curP2;
        p2To = elemP2;
        lastElemP2 = elemP2;

        return true;
      }
    }
  }

  return false;
}


void Transition::recalcCurValues(AnimProp p, float k)
{
  const Sqrat::Object *colorField = ColorAnim::fieldForAnim(elem, p, false);
  E3DCOLOR *colorOutput = nullptr;
  if (colorField)
  {
    E3DCOLOR *ptr = nullptr;
    colorOutput = (elem->robjParams && elem->robjParams->getAnimColor(p, &ptr)) ? ptr : nullptr;
  }

  if (colorOutput)
  {
    ColorHsva val = lerp_hsva(colorFrom, colorTo, k);
    *colorOutput = e3dcolor(hsv2rgb(val));
    curColor = *colorOutput;
  }
  else if (p == AP_OPACITY || p == AP_FVALUE || p == AP_ROTATE)
  {
    float val = lerp(floatFrom, floatTo, k);
    curFloat = val;
    if (p == AP_OPACITY)
      elem->props.setCurrentOpacity(val);
    else if (p == AP_FVALUE)
      elem->props.overrideFloat(elem->csk->fValue, val);
    else if (p == AP_ROTATE && elem->transform)
    {
      elem->transform->curRotate = curFloat;
      elem->transform->haveCurRotate = true;
    }
  }
  else if (p == AP_PICSATURATE || p == AP_BRIGHTNESS)
  {
    float *outFloat = nullptr;
    if (elem->robjParams)
      if (elem->robjParams->getAnimFloat(p, &outFloat))
        if (outFloat)
          *outFloat = curFloat = lerp(floatFrom, floatTo, k);
  }
  else if (p == AP_SCALE)
  {
    curP2 = lerp(p2From, p2To, k);
    if (elem->transform)
    {
      elem->transform->curScale = curP2;
      elem->transform->haveCurScale = true;
    }
  }
  else if (p == AP_TRANSLATE)
  {
    curP2 = lerp(p2From, p2To, k);
    if (elem->transform)
    {
      elem->transform->curTranslate = curP2;
      elem->transform->haveCurTranslate = true;
    }
  }
}


void Transition::update(int64_t dt)
{
  if (cfgDuration <= 0)
    return;

  bool reset = false;
  if (checkStart(prop, &reset))
  {
    if (reset)
      return;

    if (!isFinished())
    {
      callHandler(elem->csk->onAbort);
      callHandler(elem->csk->onExit);
    }

    timeLeft = duration = cfgDuration;
    callHandler(elem->csk->onEnter);
  }

  if (!isFinished())
  {
    timeLeft -= dt;

    float k = clamp(1.0f - float(timeLeft) / float(duration), 0.0f, 1.0f);

    if (sqEasingFunc.IsNull())
      k = easingFunc(k);
    else
    {
      Sqrat::Function f(sqEasingFunc.GetVM(), Sqrat::Object(sqEasingFunc.GetVM()), sqEasingFunc.GetObject());
      float res = k;
      f.Evaluate(k, res);
      k = res;
    }

    recalcCurValues(prop, k);

    if (isFinished())
    {
      if (prop == AP_OPACITY)
        elem->props.resetCurrentOpacity();
      else if (prop == AP_SCALE && elem->transform)
        elem->transform->haveCurScale = false;
      else if (prop == AP_TRANSLATE && elem->transform)
        elem->transform->haveCurTranslate = false;
      else if (prop == AP_ROTATE && elem->transform)
        elem->transform->haveCurRotate = false;

      callHandler(elem->csk->onFinish);
      callHandler(elem->csk->onExit);
    }
  }
}

#if 0
Sqrat::Object AllTransitionsConfig::getNoHandlersCfg(const StringKeys *csk) const
{
  HSQUIRRELVM vm = cfg.GetVM();
  int prevTop = sq_gettop(vm);
  sq_pushobject(vm, cfg.GetObject());
  if (SQ_FAILED(sq_clone(vm, -1)))
  {
    sq_settop(vm, prevTop);
    return Sqrat::Object(vm);
  }
  else
  {
    HSQOBJECT hRes;

#define DEL_SLOT(key)                      \
  sq_pushobject(vm, csk->key.GetObject()); \
  sq_deleteslot(vm, -2, false);
    DEL_SLOT(onEnter)
    DEL_SLOT(onExit)
    DEL_SLOT(onFinish)
    DEL_SLOT(onAbort)
#undef DEL_SLOT
    G_ASSERT(sq_gettype(vm, -1) == OT_TABLE);

    sq_getstackobj(vm, -1, &hRes);
    Sqrat::Object res(hRes, vm);
    sq_settop(vm, prevTop);
    return res;
  }
}
#endif


} // namespace darg
