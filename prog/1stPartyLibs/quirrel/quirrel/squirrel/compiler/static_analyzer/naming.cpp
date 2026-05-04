#include <assert.h>
#include "naming.h"
#include "ast_helpers.h"


namespace SQCompilation
{

bool looksLikeElementCount(const Expr *e) {
  const char *checkee = nullptr;

  if (e->op() == TO_ID)
    checkee = e->asId()->name();
  else if (e->op() == TO_GETFIELD) {
    checkee = e->asGetField()->fieldName();
  }
  else if (e->op() == TO_GETSLOT) {
    return looksLikeElementCount(deparenStatic(e->asGetSlot()->key()));
  }
  else if (e->op() == TO_CALL) {
    return looksLikeElementCount(deparenStatic(e->asCallExpr()->callee()));
  }

  if (!checkee)
    return false;

  static const char * markers[] = { "len", "Len", "length", "Length", "count", "Count", "cnt", "Cnt", "size", "Size", "Num", "Number" };

  for (const char *m : markers) {
    const char *p = strstr(checkee, m);
    if (!p)
      continue;

    if (p == checkee || p[-1] == '_' || (sq_islower(p[-1]) && sq_isupper(p[0]))) {
      char next = p[strlen(m)];
      if (!next || next == '_' || sq_isupper(next))
        return true;
    }
  }

  return false;
}


bool isUpperCaseIdentifier(const Expr *e) {
  const char *id = nullptr;

  if (e->op() == TO_GETFIELD) {
    id = e->asGetField()->fieldName();
  }

  if (e->op() == TO_ID) {
    id = e->asId()->name();
  }
  else if (e->op() == TO_LITERAL && e->asLiteral()->kind() == LK_STRING) {
    id = e->asLiteral()->s();
  }

  if (!id)
    return false;

  while (*id) {
    if (*id >= 'a' && *id <= 'z')
      return false;
    ++id;
  }

  return true;
}

}
