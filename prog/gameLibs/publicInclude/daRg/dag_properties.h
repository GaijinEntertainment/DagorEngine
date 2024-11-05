//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <util/dag_string.h>
#include <sqrat.h>


namespace darg
{

class StringKeys;
class Picture;
class Cursor;


// Alternative interface to access component script properties table
// scriptDesc references the same table as Style
// (also the same scriptDesc as Component, but life time is different)
class Properties
{
  Properties(const Properties &) = delete;
  Properties(Properties &&) = default;
  void operator=(const Properties &) = delete;
  Properties &operator=(Properties &&) = delete;

public:
  Properties(HSQUIRRELVM vm);

  void load(const Sqrat::Table &desc, const Sqrat::Object &builder, const StringKeys *csk);

  template <typename T>
  T getInt(const Sqrat::Object &key, T def = 0, bool assert_on_type = true) const
  {
    Sqrat::Object val = scriptDesc.RawGetSlot(key);
    SQObjectType type = val.GetType();
    if (type & SQOBJECT_NUMERIC)
      return val.Cast<T>();

    if (type != OT_NULL && assert_on_type)
      trace_error("Expected numeric type or null", scriptDesc, key);

    return def;
  }

  float getFloat(const Sqrat::Object &key, float def = 0, bool assert_on_type = true) const;
  bool getBool(const Sqrat::Object &key, bool def = false, bool assert_on_type = true) const;

  Sqrat::Object getObject(const Sqrat::Object &key, bool *found) const;
  Sqrat::Object getObject(const Sqrat::Object &key) const;

  int getFontId() const;
  float getFontSize() const;

  E3DCOLOR getColor(const Sqrat::Object &key, E3DCOLOR def = E3DCOLOR(255, 255, 255)) const;

  Picture *getPicture(const Sqrat::Object &key) const;

  bool getSound(const StringKeys *csk, const char *field_name, Sqrat::string &out_name, Sqrat::Object &out_params, float &out_vol);
  bool getSound(const StringKeys *csk, const Sqrat::Object &field_name, Sqrat::string &out_name, Sqrat::Object &out_params,
    float &out_vol);

  float getBaseOpacity() const { return baseOpacity; }
  float getCurrentOpacity() const { return currentOpacity >= 0.0f ? currentOpacity : baseOpacity; }
  void setCurrentOpacity(float val) { currentOpacity = clamp(val, 0.0f, 1.0f); }
  void resetCurrentOpacity() { currentOpacity = -1; }

  void overrideFloat(const Sqrat::Object &key, float val) { currentOverrides().SetValue(key, val); }
  void overrideInt(const Sqrat::Object &key, int val) { currentOverrides().SetValue(key, val); }
  void resetOverride(const Sqrat::Object &key) { currentOverrides().RawDeleteSlot(key); }

  float getCurrentFloat(const Sqrat::Object &key, float def = 0, bool assert_on_type = true) const;
  int getCurrentInt(const Sqrat::Object &key, int def = 0, bool assert_on_type = true) const;

  bool getCurrentCursor(const Sqrat::Object &key, Cursor **cursor) const;

  // alias to storage, same data object, used in different context
  Sqrat::Table &currentOverrides() { return storage; }
  const Sqrat::Table &currentOverrides() const { return storage; }

private:
  template <typename T>
  bool getSoundInternal(const StringKeys *csk, T field_name, Sqrat::string &out_name, Sqrat::Object &out_params, float &out_vol);

  void applyTint(const StringKeys *csk, E3DCOLOR &color) const;

  static void trace_error(const char *message, const Sqrat::Object &container, const Sqrat::Object &key);

public:
  String text;
  Sqrat::Object uniqueKey;
  Sqrat::Table scriptDesc;
  Sqrat::Object scriptBuilder;
  Sqrat::Table storage; //< persistent between updates, also used to store current overrides of component values

  float baseOpacity = 1.0f, currentOpacity = -1.0f;
};


} // namespace darg
