// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_localization.h>
#include <util/dag_string.h>
#include <util/dag_stlqsort.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqrat.h>
#include <sqmodules/sqmodules.h>
#include "hypenation.h"


static void handle_plural_form(String &string_to_handle, int64_t num, const char *const num_name)
{
  String tmpKey(framemem_ptr());
  tmpKey.printf(32, "{%s=", num_name);
  int searchOfs = 0;
  while (true)
  {
    const char *tokenStart = string_to_handle.find(tmpKey, string_to_handle.str() + searchOfs);
    const char *tokenEnd = string_to_handle.find("}", tokenStart);
    if (!tokenStart || !tokenEnd)
      return;
    tokenEnd += 1;
    searchOfs = int(tokenStart - string_to_handle.str());

    String pluralStr(framemem_ptr());
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

    String word(framemem_ptr());
    word.setSubStr(wordStart, wordEnd);
    string_to_handle.replace(pluralStr.str(), word);
  }
}

struct LocParam
{
  const char *name = nullptr; // owned by the params table on the stack
  String value;               // a copy: the sq_tostring result dies when popped
  int64_t num = 0;
  bool isNum = false;
};


static void read_loc_params(HSQUIRRELVM v, int tbl_idx, Tab<LocParam> &out)
{
  int top = sq_gettop(v);
  sq_push(v, tbl_idx);
  sq_pushnull(v);
  while (SQ_SUCCEEDED(sq_next(v, -2)))
  {
    const char *name = nullptr;
    if (SQ_SUCCEEDED(sq_getstring(v, -2, &name)) && SQ_SUCCEEDED(sq_tostring(v, -1)))
    {
      const char *str = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, -1, &str)));
      LocParam &p = out.push_back(LocParam{name, String(framemem_ptr())});
      p.value = str;
      p.isNum = SQ_SUCCEEDED(sq_getinteger(v, -2, &p.num));
      sq_pop(v, 1);
    }
    sq_pop(v, 2);
  }
  sq_pop(v, 2);
  G_ASSERT(sq_gettop(v) == top);
  G_UNUSED(top);
}


static void apply_loc_params(String &text, const Tab<LocParam> &params)
{
  String token(framemem_ptr());
  for (const LocParam &p : params)
  {
    token.printf(32, "{%s}", p.name);
    text.replace(token, p.value);
  }
  for (const LocParam &p : params)
    if (p.isNum)
      handle_plural_form(text, p.num, p.name);
}


static void fallback_loc_text(String &out, const char *key, const Tab<LocParam> &params)
{
  out = key;
  for (int i = 0; i < params.size(); i++)
    out.aprintf(0, "%s%s=%s", i == 0 ? ": " : ", ", params[i].name, params[i].value.str());
}


static SQRESULT parse_loc_args(HSQUIRRELVM v, int first_idx, const char *key, const char *&def_val, int &params_idx)
{
  def_val = nullptr;
  params_idx = -1;
  for (int idx = first_idx, top = sq_gettop(v); idx <= top; ++idx)
  {
    HSQOBJECT arg;
    if (SQ_FAILED(sq_getstackobj(v, idx, &arg)) || arg._type == OT_NULL)
      continue;

    if (arg._type == OT_STRING)
      def_val = sq_objtostring(&arg);
    else if (arg._type == OT_TABLE || arg._type == OT_CLASS || arg._type == OT_INSTANCE)
      params_idx = idx;
    else
      return sq_throwerror(v, String(0, "Unexpected argument #%d type %X for loc(%s)", idx, arg._type, key));
  }
  return SQ_OK;
}


static SQInteger push_loc_text(HSQUIRRELVM v, const char *key, const char *text, int params_idx)
{
  if (params_idx < 0 || (!text && !*key)) // scripts pass "" with params when there is no text to show
  {
    sq_pushstring(v, text ? text : key, -1);
    return 1;
  }

  FRAMEMEM_REGION;
  Tab<LocParam> params(framemem_ptr());
  read_loc_params(v, params_idx, params);

  String s(framemem_ptr());
  if (text)
  {
    s = text;
    apply_loc_params(s, params);
  }
  else
  {
    stlsort::sort(params.begin(), params.end(), [](const LocParam &a, const LocParam &b) { return strcmp(a.name, b.name) < 0; });
    fallback_loc_text(s, key, params);
  }

  sq_pushstring(v, s, s.length());
  return 1;
}


static SQInteger localize(HSQUIRRELVM v)
{
  const char *key = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &key)))
    return 0;

  const char *defVal;
  int paramsIdx;
  if (SQ_FAILED(parse_loc_args(v, 3, key, defVal, paramsIdx)))
    return SQ_ERROR;

  return push_loc_text(v, key, get_localized_text(key, defVal), paramsIdx);
}


static SQInteger localize_for_lang(HSQUIRRELVM v)
{
  const char *key = nullptr;
  if (SQ_FAILED(sq_getstring(v, 2, &key)))
    return 0;
  const char *lang = nullptr;
  if (SQ_FAILED(sq_getstring(v, 3, &lang)))
    return 0;

  const char *defVal;
  int paramsIdx;
  if (SQ_FAILED(parse_loc_args(v, 4, key, defVal, paramsIdx)))
    return SQ_ERROR;

  const char *res = get_localized_text_for_lang(key, lang);
  return push_loc_text(v, key, res ? res : defVal, paramsIdx);
}


static SQInteger script_does_localized_text_exist(HSQUIRRELVM v)
{
  const char *key = NULL;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, 2, &key)));
  sq_pushbool(v, does_localized_text_exist(key));
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

static void load_localization_table_from_file(const char *filename, const char *lang)
{
  load_localization_table_from_csv_V2(filename, lang);
}

namespace bindquirrel
{

void register_dagor_localization_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  ///@module dagor.localize
  exports //
    .SquirrelFuncDeclString(&localize, "pure loc(key: string|null, ...): string|null")
    .SquirrelFuncDeclString(&localize_for_lang, "pure getLocTextForLang(key: string, lang: string|null, ...): string")
    .SquirrelFuncDeclString(process_chinese_string_with_tab, "pure processHypenationsCN(str: string): string")
    .SquirrelFuncDeclString(process_japanese_string_with_tab, "pure processHypenationsJP(str: string): string")
    .SquirrelFuncDeclString(script_does_localized_text_exist, "pure doesLocTextExist(key: string): bool")
    .Func("getCurrentLanguage", get_current_language)
    .Func("getForceLanguage", get_force_language)
    .Func("setLanguageToSettings", set_language_to_settings)
    .Func("loadLocalizationFromFile", load_localization_table_from_file)
    .Func("getLangId", getLangId)
    .SquirrelFuncDeclString(init_localization, "initLocalization(locBlk: instance, [curLang: string]): null")
    /**/;
  module_mgr->addNativeModule("dagor.localize", exports);
}

} // namespace bindquirrel
