// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_localization.h>
#include <util/dag_string.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqrat.h>
#include <quirrel/sqModules/sqModules.h>
#include "hypenation.h"


static void handle_plural_form(String &string_to_handle, int64_t num, const char *const num_name)
{
  String tmpKey;
  tmpKey.printf(32, "{%s=", num_name);
  const char *tokenStart = NULL, *tokenEnd = NULL;
  while (true)
  {
    tokenStart = string_to_handle.find(tmpKey, tokenStart);
    tokenEnd = string_to_handle.find("}", tokenStart);
    if (!tokenStart || !tokenEnd)
      return;
    tokenEnd += 1;

    String pluralStr;
    pluralStr.setSubStr(tokenStart, tokenEnd);
    int formId = get_plural_form_id(num);

    const char *wordStart = pluralStr.find("=") + 1;
    const char *wordEnd = pluralStr.find("}");

    const char *tmpPos = NULL;

    // When formId is -1, the latter form should be picked.
    for (int i = 0; formId == -1 || i < formId; i++)
    {
      tmpPos = pluralStr.find("/", wordStart);
      if (tmpPos)
        wordStart = tmpPos + 1;
      else
        break;
    }

    tmpPos = pluralStr.find("/", wordStart);
    if (tmpPos)
      wordEnd = tmpPos;

    String word = String::mk_sub_str(wordStart, wordEnd);
    string_to_handle.replace(pluralStr.str(), word);
  }
}
static SQInteger localize_with_params(HSQUIRRELVM v, int top, const char *res, int paramsTblIdx);


static SQInteger localize(HSQUIRRELVM v)
{
  const char *key = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &key)))
    return 0;

  int top = sq_gettop(v);
  bool caseInsensitive = false;
  const char *defVal = key;
  int paramsTblIdx = -1;

  for (int idx = 3; idx <= top; ++idx)
  {
    HSQOBJECT argObj;
    if (SQ_FAILED(sq_getstackobj(v, idx, &argObj)) || argObj._type == OT_NULL)
      continue;

    if (argObj._type == OT_BOOL)
      caseInsensitive = sq_objtobool(&argObj);
    else if (argObj._type == OT_STRING)
      defVal = sq_objtostring(&argObj);
    else if (argObj._type == OT_TABLE || argObj._type == OT_CLASS || argObj._type == OT_INSTANCE)
      paramsTblIdx = idx;
    else
      return sq_throwerror(v, String(0, "Unexpected argument #%d type %X for loc(%s)", idx, argObj._type, key));
  }

  return localize_with_params(v, top, get_localized_text(key, defVal, caseInsensitive), paramsTblIdx);
}

static SQInteger localize_with_params(HSQUIRRELVM v, int top, const char *res, int paramsTblIdx)
{
  if (!res)
  {
    sq_push(v, 2);
    return 1;
  }

  if (paramsTblIdx < 3)
  {
    G_ASSERT(paramsTblIdx < 0);
    sq_pushstring(v, res, -1);
    return 1;
  }

  String s(res);
  String tmpKey;

  int preIterationTop = sq_gettop(v);
  sq_push(v, paramsTblIdx);

  // replace all {key} by values
  sq_pushnull(v);
  while (SQ_SUCCEEDED(sq_next(v, -2)))
  {
    const char *paramKey = NULL, *paramVal = NULL;
    if (SQ_FAILED(sq_getstring(v, -2, &paramKey)))
    {
      sq_pop(v, 2);
      continue;
    }

    sq_tostring(v, -1);
    HSQOBJECT paramValueObj;
    sq_getstackobj(v, -1, &paramValueObj);
    if (paramValueObj._type != OT_STRING)
    {
      sq_pop(v, 3);
      continue;
    }

    if (SQ_SUCCEEDED(sq_getstring(v, -1, &paramVal)))
    {
      tmpKey.printf(32, "{%s}", paramKey);
      s.replace(tmpKey.str(), paramVal);
    }
    else
      G_ASSERT(0);

    sq_pop(v, 3); // pops key, val and val.tostring() before the next iteration
  }

  sq_pop(v, 1); // pops the iterator

  // apply plural forms
  sq_pushnull(v);
  while (SQ_SUCCEEDED(sq_next(v, -2)))
  {
    const char *paramKey = NULL;
    if (SQ_FAILED(sq_getstring(v, -2, &paramKey)))
    {
      sq_pop(v, 2);
      continue;
    }

    int64_t paramNumValue;
    if (SQ_SUCCEEDED(sq_getinteger(v, -1, &paramNumValue)))
      handle_plural_form(s, paramNumValue, paramKey);

    sq_pop(v, 2); // pops key, val before the next iteration
  }

  sq_pop(v, 2); // pops the iterator and table

  G_UNUSED(preIterationTop);
  G_ASSERT(preIterationTop == sq_gettop(v));
  G_ASSERT(preIterationTop == top);
  G_UNUSED(top);

  sq_pushstring(v, s, s.length());

  return 1;
}


static SQInteger localize_for_lang(HSQUIRRELVM v)
{
  const char *key = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &key)))
    return 0;
  const char *lang = nullptr;
  if (SQ_FAILED(sq_getstring(v, 3, &lang)))
    return 0;

  int top = sq_gettop(v);
  const char *defVal = nullptr;
  int paramsTblIdx = -1;

  for (int idx = 4; idx <= top; ++idx)
  {
    HSQOBJECT argObj;
    if (SQ_FAILED(sq_getstackobj(v, idx, &argObj)) || argObj._type == OT_NULL)
      continue;

    if (argObj._type == OT_STRING)
      defVal = sq_objtostring(&argObj);
    else if (argObj._type == OT_TABLE || argObj._type == OT_CLASS || argObj._type == OT_INSTANCE)
      paramsTblIdx = idx;
    else
      return sq_throwerror(v, String(0, "Unexpected argument #%d type %X for loc(%s)", idx, argObj._type, key));
  }

  const char *res = get_localized_text_for_lang(key, lang);
  return localize_with_params(v, top, res ? res : defVal, paramsTblIdx);
}


template <bool ci>
static SQInteger script_get_loc_text_ex_(HSQUIRRELVM v)
{
  const char *key = NULL;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 2, &key)));
  LocTextId id = (sq_gettop(v) > 2) ? get_optional_localized_text_id(key, ci) : get_localized_text_id(key, ci);
  if (id)
    sq_pushstring(v, get_localized_text(id), -1);
  else
  {
    const char *def = NULL;
    if (sq_gettop(v) > 2)
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 3, &def)));
    sq_pushstring(v, def ? def : key, -1);
  }
  return 1;
}

static SQInteger script_does_localized_text_exist(HSQUIRRELVM v)
{
  const char *key = NULL;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 2, &key)));
  sq_pushbool(v, does_localized_text_exist(key, false));
  return 1;
}


static SQInteger init_localization(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<DataBlock *>(vm, 2))
    return SQ_ERROR;

  Sqrat::Var<const DataBlock *> locBlk(vm, 2);
  const char *curLang = nullptr;
  if (sq_gettop(vm) > 2)
    sq_getstring(vm, 3, &curLang);

  shutdown_localization();
  if (!startup_localization_V2(*locBlk.value, curLang))
    return sq_throwerror(vm, "Failed to init localization - see log for details");

  return 0;
}


static SQInteger process_chinese_string_with_tab(HSQUIRRELVM vm)
{
  const char *str;
  sq_getstring(vm, 2, &str);
  SimpleString ret = process_chinese_string(str);
  sq_pushstring(vm, ret, strlen(ret));
  return 1;
}
static SQInteger process_japanese_string_with_tab(HSQUIRRELVM vm)
{
  const char *str;
  sq_getstring(vm, 2, &str);
  SimpleString ret = process_japanese_string(str);
  sq_pushstring(vm, ret, strlen(ret));
  return 1;
}

namespace bindquirrel
{

void register_dagor_localization_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  ///@module dagor.localize
  exports //
    .SquirrelFunc("loc", &localize, -2, ".s|o", nullptr, 0, nullptr, Sqrat::FunctionPurity::Pure)
    .SquirrelFunc("getLocTextForLang", &localize_for_lang, -3, ".ss|o", nullptr, 0, nullptr, Sqrat::FunctionPurity::Pure)
    .SquirrelFunc("processHypenationsCN", process_chinese_string_with_tab, 2, ".s", nullptr, 0, nullptr, Sqrat::FunctionPurity::Pure)
    .SquirrelFunc("processHypenationsJP", process_japanese_string_with_tab, 2, ".s", nullptr, 0, nullptr, Sqrat::FunctionPurity::Pure)
    .SquirrelFunc("doesLocTextExist", script_does_localized_text_exist, 2, ".s", nullptr, 0, nullptr, Sqrat::FunctionPurity::Pure)
    .Func("getCurrentLanguage", get_current_language)
    .Func("getForceLanguage", get_force_language)
    .Func("setLanguageToSettings", set_language_to_settings)
    .Func("getLangId", getLangId)
    .SquirrelFunc("initLocalization", init_localization, -2, ".xs")
    /**/;
  module_mgr->addNativeModule("dagor.localize", exports);
}

} // namespace bindquirrel
