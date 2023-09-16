#pragma once

#include "easing.h"
#include "color.h"

#include <daRg/dag_animation.h>


namespace darg
{

class Element;
class StringKeys;

struct AnimDesc
{
  AnimProp prop;
  int64_t duration;
  int64_t delay;

  Sqrat::Object from, to;

  bool autoPlay;
  bool playFadeOut;
  bool loop;
  bool globalTimer;

  Sqrat::Object trigger;
  easing::Func easingFunc;
  Sqrat::Object sqEasingFunc;
  Sqrat::Object sound;
  Sqrat::Object onExit, onFinish, onAbort, onEnter, onStart;

  AnimDesc();

  void reset();
  bool load(const Sqrat::Table &desc, const StringKeys *csk);

  bool fromElemProp() { return from.IsNull(); }
  bool toElemProp() { return to.IsNull(); }
};


class Animation
{
public:
  Animation(Element *elem);
  virtual ~Animation() {}

  virtual void setup(bool initial) = 0;
  virtual void update(int64_t dt_usec) = 0;
  virtual void apply() = 0;

  void init(const AnimDesc &desc_);
  void start();
  void requestStop();
  void rewind();
  bool isFinished() const;
  void skip();
  void skipOnDelete();
  void skipDelay();

protected:
  bool baseUpdate(int64_t dt_usec);
  void playSound(const Sqrat::Object &key);
  void callHandler(const Sqrat::Object &handler, bool allow_start);
  float applyEasing(float k);

public:
  AnimDesc desc;
  int64_t t = 0, delayLeft = 0;
  bool isPlayingLoop = false;
  Element *elem;
};


class ColorAnim : public Animation
{
public:
  ColorAnim(Element *elem);
  virtual void setup(bool initial) override;
  virtual void update(int64_t dt_usec) override;
  virtual void apply() override;

  static const Sqrat::Object *fieldForAnim(const Element *elem, AnimProp prop, bool assert_on_missing);

private:
  ColorHsva cur;
  E3DCOLOR *output;
  E3DCOLOR baseElemColor;

  ColorHsva cFrom, cTo;
};


class FloatAnim : public Animation
{
public:
  FloatAnim(Element *elem) : Animation(elem) {}
  virtual void setup(bool initial) override;
  virtual void update(int64_t dt_usec) override;
  virtual void apply() override;

private:
  float cur = 0;
  float *output = nullptr;
  float fFrom = 0, fTo = 0;
};


class Point2Anim : public Animation
{
public:
  Point2Anim(Element *elem) : Animation(elem) {}
  virtual void setup(bool initial) override;
  virtual void update(int64_t dt_usec) override;
  virtual void apply() override;

private:
  Point2 cur = Point2(0, 0);
  Point2 p2From = Point2(0, 0), p2To = Point2(0, 0);
};


Animation *create_anim(const AnimDesc &desc, Element *elem);


class Transition
{
public:
  Transition(AnimProp prop_, Element *elem_);
  void update(int64_t dt_usec);
  void setup(const Sqrat::Table &cfg);
  bool isFinished() const { return timeLeft <= 0; }

public:
  AnimProp prop = AP_INVALID;

private:
  void callHandler(const Sqrat::Object &key);
  bool checkStart(AnimProp prop, bool *reset);
  void recalcCurValues(AnimProp prop, float k);

private:
  Element *elem;

  Sqrat::Table tblConfig;

  int64_t cfgDuration = 200000;
  easing::Func easingFunc = easing::functions[easing::Linear];
  Sqrat::Object sqEasingFunc;


  int64_t duration = 200000;
  int64_t timeLeft = 0;

  E3DCOLOR lastElemColor = 0, curColor = 0;
  float lastElemFloat = 0, curFloat = 0;
  Point2 lastElemP2 = Point2(0, 0), curP2 = Point2(0, 0);
  ColorHsva colorFrom, colorTo;
  float floatFrom = 0, floatTo = 0;
  Point2 p2From = Point2(0, 0), p2To = Point2(0, 0);

  bool lastValueValid = false;
};

class AllTransitionsConfig
{
public:
  AllTransitionsConfig(const Sqrat::Object &cfg_) : cfg(cfg_) {}

  Sqrat::Object cfg;

  Sqrat::Object getNoHandlersCfg(const StringKeys *csk) const;
};

} // namespace darg
