// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_picture.h>
#include <math/dag_color.h>

#include "scriptUtil.h"
#include "dargDebugUtils.h"
#include "guiGlobals.h"
#include "guiScene.h"
#include "color.h"


namespace darg
{


Properties::Properties(HSQUIRRELVM vm) : storage(vm) {}


void Properties::load(const Sqrat::Table &desc, const Sqrat::Object &builder, const StringKeys *csk)
{
  G_ASSERTF(desc.GetType() == OT_TABLE || desc.GetType() == OT_CLASS, "Unexpected properties type %s (%X)",
    sq_objtypestr(desc.GetType()), desc.GetType());

  SqStackChecker stackCheck(desc.GetVM());

  scriptDesc = desc;
  scriptBuilder = builder;

  uniqueKey = desc.RawGetSlot(csk->key);

  Sqrat::Object textObj = desc.RawGetSlot(csk->text);
  if (textObj.IsNull())
    text = "";
  else
  {
    auto textVar = textObj.GetVar<const char *>();
    text = String(textVar.value, textVar.valueLen);
  }

  baseOpacity = clamp(getFloat(csk->opacity, 1.0f), 0.0f, 1.0f);
}


bool Properties::getBool(const Sqrat::Object &key, bool def, bool assert_on_type) const
{
  if (scriptDesc.RawHasKey(key))
  {
    Sqrat::Object val = scriptDesc.RawGetSlot(key);
    SQObjectType type = val.GetType();
    if (type & (SQOBJECT_CANBEFALSE | SQOBJECT_NUMERIC))
      return val.Cast<bool>();

    if (assert_on_type)
      trace_error("Expected bool or null", scriptDesc, key);

    return true; // any non-bool and non-null
  }

  return def;
}


float Properties::getFloat(const Sqrat::Object &key, float def, bool assert_on_type) const
{
  Sqrat::Object val = scriptDesc.RawGetSlot(key);
  SQObjectType type = val.GetType();
  if (type & SQOBJECT_NUMERIC)
    return val.Cast<float>();

  if (type != OT_NULL && assert_on_type)
    trace_error("Expected numeric type or null", scriptDesc, key);

  return def;
}


int Properties::getCurrentInt(const Sqrat::Object &key, int def, bool assert_on_type) const
{
  Sqrat::Object val = currentOverrides().RawGetSlot(key);
  SQObjectType type = val.GetType();
  if (type & SQOBJECT_NUMERIC)
    return val.Cast<int>();

  if (type != OT_NULL && assert_on_type)
    trace_error("Expected numeric type or null", scriptDesc, key);

  return getInt(key, def, assert_on_type);
}


float Properties::getCurrentFloat(const Sqrat::Object &key, float def, bool assert_on_type) const
{
  Sqrat::Object val = currentOverrides().RawGetSlot(key);
  SQObjectType type = val.GetType();
  if (type & SQOBJECT_NUMERIC)
    return val.Cast<float>();

  if (type != OT_NULL && assert_on_type)
    trace_error("Expected numeric type or null", scriptDesc, key);

  return getFloat(key, def, assert_on_type);
}


Sqrat::Object Properties::getObject(const Sqrat::Object &key, bool *found) const
{
  if (scriptDesc.RawHasKey(key))
  {
    *found = true;
    return scriptDesc.RawGetSlot(key);
  }

  *found = false;
  return Sqrat::Object(scriptDesc.GetVM());
}


Sqrat::Object Properties::getObject(const Sqrat::Object &key) const { return scriptDesc.RawGetSlot(key); }


bool Properties::getCurrentCursor(const Sqrat::Object &key, Cursor **cursor) const
{
  const Sqrat::Table *tables[] = {&currentOverrides(), &scriptDesc};
  for (const Sqrat::Table *tbl : tables)
  {
    if (tbl->RawHasKey(key))
    {
      *cursor = tbl->GetSlotValue<Cursor *>(key, nullptr);
      return true;
    }
  }

  *cursor = nullptr;
  return false;
}


E3DCOLOR Properties::getColor(const Sqrat::Object &key, E3DCOLOR def) const
{
  E3DCOLOR res = def;

  Sqrat::Object val = scriptDesc.RawGetSlot(key);
  if (!val.IsNull())
  {
    const char *errMsg = nullptr;
    if (!decode_e3dcolor(val, res, &errMsg))
      darg_assert_trace_var(errMsg, scriptDesc, key);
  }

  GuiScene *guiScene = GuiScene::get_from_sqvm(scriptDesc.GetVM());
  StringKeys *csk = guiScene->getStringKeys();

  applyTint(csk, res);

  return res;
}


Picture *Properties::getPicture(const Sqrat::Object &key) const
{
  Sqrat::Object val = scriptDesc.RawGetSlot(key);
  SQObjectType type = val.GetType();

  if (type == OT_NULL)
    return NULL;

  if (type == OT_INSTANCE)
    return val.Cast<Picture *>();

  trace_error("Expected instance or null", scriptDesc, key);

  return NULL;
}


template <typename T>
bool Properties::getSoundInternal(const StringKeys *csk, T field_name, Sqrat::string &out_name, Sqrat::Object &out_params,
  float &out_vol)
{
  out_name.clear();
  Sqrat::Object tbl = scriptDesc.RawGetSlot(csk->sound);
  if (tbl.IsNull())
    return false;

  Sqrat::Object slot = tbl.RawGetSlot(field_name);
  return load_sound_info(slot, out_name, out_params, out_vol);
}


bool Properties::getSound(const StringKeys *csk, const char *field_name, Sqrat::string &out_name, Sqrat::Object &out_params,
  float &out_vol)
{
  return getSoundInternal(csk, field_name, out_name, out_params, out_vol);
}


bool Properties::getSound(const StringKeys *csk, const Sqrat::Object &field_name, Sqrat::string &out_name, Sqrat::Object &out_params,
  float &out_vol)
{
  return getSoundInternal(csk, field_name, out_name, out_params, out_vol);
}


int Properties::getFontId() const
{
  GuiScene *guiScene = GuiScene::get_from_sqvm(scriptDesc.GetVM());
  StringKeys *csk = guiScene->getStringKeys();
  if (require_font_size_slot)
  {
    if (!scriptDesc.RawGetSlot(csk->font).IsNull() && scriptDesc.RawGetSlot(csk->fontSize).IsNull())
      trace_error("font is specified, but fontSize is missing", scriptDesc, csk->font);
  }

  int fontId = getInt(csk->font, guiScene->getConfig()->defaultFont);
  if (!StdGuiRender::get_font(fontId))
  {
    trace_error("Invalid font", scriptDesc, csk->font);
    fontId = 0;
  }
  return fontId;
}


float Properties::getFontSize() const
{
  GuiScene *guiScene = GuiScene::get_from_sqvm(scriptDesc.GetVM());
  StringKeys *csk = guiScene->getStringKeys();
  return getFloat(csk->fontSize, guiScene->getConfig()->defaultFontSize);
}


void Properties::applyTint(const StringKeys *csk, E3DCOLOR &base_color) const
{
  int tintColor = getInt(csk->tint, 0);

  // if tint strength (alpha) is not zero
  if ((tintColor & 0xFF000000) != 0)
  {
    real srcAlpha = (tintColor >> 24) & 0xFF;
    srcAlpha /= 255.0;
    real oneMinusSrcAlpha = 1.0 - srcAlpha;

#define ALPHA_BLEND(DST, SRC, CHANNEL, SRC_ALPHA, ONE_MINUS_SRC_ALPHA) \
  floor((((DST) >> CHANNEL) & 0xFF) * (ONE_MINUS_SRC_ALPHA) + (((SRC) >> (CHANNEL)) & 0xFF) * (SRC_ALPHA))
    int r = ALPHA_BLEND(base_color.u, tintColor, 16, srcAlpha, oneMinusSrcAlpha);
    int g = ALPHA_BLEND(base_color.u, tintColor, 8, srcAlpha, oneMinusSrcAlpha);
    int b = ALPHA_BLEND(base_color.u, tintColor, 0, srcAlpha, oneMinusSrcAlpha);
#undef ALPHA_BLEND

    base_color.u = (base_color.u & 0xFF000000) | (r << 16) | (g << 8) | (b << 0);
  }
}


void Properties::trace_error(const char *message, const Sqrat::Object &container, const Sqrat::Object &key)
{
  darg_assert_trace_var(message, container, key);
}

} // namespace darg
