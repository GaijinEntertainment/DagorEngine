#include <debug/dag_assert.h>
#include <pcrecpp_nostl.h>
#include "sqRegExp.h"
#include <sqrat.h>

// Regilar Expression for Squirrel based on PCRE (Perl-compatible regular expression library).


namespace bindquirrel
{

RegExp::RegExp() : re("", pcrecpp::UTF8()) {}


RegExp::RegExp(const char *pattern) : re(pattern, pcrecpp::UTF8()) {}


// Check that 'match' string match any 'pattern' substrings.
bool RegExp::match(const char *match)
{
  G_ASSERT(match != NULL);
  if (match == NULL)
    return false;

  return re.PartialMatch(match);
}


// Check that 'match' string match 'pattern' string exactly.
bool RegExp::fullMatch(const char *match)
{
  G_ASSERT(match != NULL);
  if (match == NULL)
    return false;

  return re.FullMatch(match);
}


// Replace all occurrences of 'pattern' in 'text' with 'replacement'.
const char *RegExp::replace(const char *replacement, const char *text)
{
  G_ASSERT(replacement != NULL);
  G_ASSERT(text != NULL);
  if (replacement == NULL || text == NULL)
    return NULL;

  return re.GlobalReplace(replacement, text);
}


void RegExp::multiExtract(const char *extractTemplate, const char *text, const eastl::function<void(const char *)> cb)
{
  if (!cb)
    return;
  pcrecpp::RE2::MultiExtractState state;
  re.MultiExtractInit(&state, text);
  const char *extracted = re.MultiExtract(&state, extractTemplate);
  while (extracted)
  {
    cb(extracted);
    extracted = re.MultiExtract(&state, extractTemplate);
  }
}


SQInteger RegExp::sqMultiExtract(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<RegExp *>(vm))
    return SQ_ERROR;

  Sqrat::Var<RegExp *> self(vm, 1);
  Sqrat::Var<const char *> templateStr(vm, 2);
  Sqrat::Var<const char *> text(vm, 3);

  Sqrat::Array result(vm);
  if (self.value && templateStr.value && text.value)
  {
    self.value->multiExtract(templateStr.value, text.value, [&result](const char *match) { result.Append(match); });
  }

  Sqrat::PushVarR(vm, result);
  return 1;
}

// Return 'pattern' string, for debug.
const char *RegExp::pattern() { return re.pattern(); }

} // namespace bindquirrel
