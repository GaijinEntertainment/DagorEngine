// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqrat.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqmodules/sqmodules.h>
#include "sqRegExp.h"
#include "sqUtf8.h"
#include <utf8.h>


namespace bindquirrel
{
static SQInteger utf8_slice(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<Utf8 *>(vm))
    return SQ_ERROR;

  Sqrat::Var<Utf8 *> utf8(vm, 1);
  sq_pushstring(vm, utf8.value->slice(vm), -1);
  return 1;
}

static SQInteger utf8_indexof(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<Utf8 *>(vm))
    return SQ_ERROR;

  Sqrat::Var<Utf8 *> utf8(vm, 1);
  int res = utf8.value->indexof(vm);
  if (res >= 0)
    sq_pushinteger(vm, res);
  else
    sq_pushnull(vm);
  return 1;
}

static SQInteger utf8_int_to_char(HSQUIRRELVM vm)
{
  SQInteger c = 0;
  sq_getinteger(vm, 2, &c);
  char buf[4] = {};
  int n = 0;
  if (c <= 0x7F)
    buf[n++] = char(c);
  else if (c <= 0x10FFFF)
    n = int(utf8::append(uint32_t(c), buf) - buf);
  sq_pushstring(vm, buf, n);
  return 1;
}

static SQInteger utf8_char_to_int(HSQUIRRELVM vm)
{
  static const uint8_t prefix[] = {0, 0xC0, 0xE0, 0xF0};
  static const uint8_t prefixMask[] = {0x80, 0xE0, 0xF0, 0xF8};
  static const uint8_t mask[] = {0x7F, 0x1F, 0x0F, 0x07};
  const char *s = nullptr;
  SQInteger len = 0;
  sq_getstringandsize(vm, 2, &s, &len);
  SQInteger res = 0;
  if (len > 0 && len <= 4 && (uint8_t(s[0]) & prefixMask[len - 1]) == prefix[len - 1])
  {
    res = uint8_t(s[0]) & mask[len - 1];
    for (SQInteger i = 1; i < len; i++)
    {
      if ((uint8_t(s[i]) & 0xC0) != 0x80)
      {
        res = 0;
        break;
      }
      res = (res << 6) | (uint8_t(s[i]) & 0x3F);
    }
  }
  sq_pushinteger(vm, res);
  return 1;
}

void register_reg_exp(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Class<RegExp> regexp2(vm, "regexp2");
  ///@class regexp2=regexp2
  regexp2 //
    .Ctor<const char *>()
    ///@param regexp_string s
    .Func("match", &RegExp::match)
    .Func("fullmatch", &RegExp::fullMatch)
    .Func("replace", &RegExp::replace)
    .SquirrelFuncDeclString(&RegExp::sqMultiExtract, "instance.multiExtract(templateStr: string, text: string): array")
    .Func("pattern", &RegExp::pattern)
    /**/;

  module_mgr->addNativeModule("regexp2", Sqrat::Object(regexp2.GetObject(), vm));
}

void register_utf8(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Class<Utf8> utf8(vm, "utf8");
  ///@class utf8=utf8
  utf8 //
    .Ctor<const char *>()
    ///@param text s
    .Func("_tostring", &Utf8::str)
    .Func("str", &Utf8::str)
    .Func("charCount", &Utf8::charCount)
    .Func("strtr", &Utf8::strtr)
    .Func("toLower", &Utf8::toLower)
    .Func("toUpper", &Utf8::toUpper)
    .Func("capitalize", &Utf8::capitalize)
    .Func("capitalizeWords", &Utf8::capitalizeWords)
    .SquirrelFuncDeclString(utf8_slice, "instance.slice(from: int, [to: int]): string")
    .SquirrelFuncDeclString(utf8_indexof, "instance.indexof(substring: string, [startIndex: int]): int|null")
    .SquirrelFuncDeclString(utf8_int_to_char, "pure intToChar(code: int): string",
      SQ_DOC("UTF-8 bytes of a code point, also of a surrogate (not valid UTF-8); \"\" above 0x10FFFF, code & 0xFF below 0"))
    .SquirrelFuncDeclString(utf8_char_to_int, "pure charToInt(char: string): int",
      SQ_DOC("UTF-8 bits of a 1-4 byte string as a code point, no overlong or range test; 0 for a bad length or byte"))
    /**/;

  module_mgr->addNativeModule("utf8", Sqrat::Object(utf8.GetObject(), vm));
}
} // namespace bindquirrel
