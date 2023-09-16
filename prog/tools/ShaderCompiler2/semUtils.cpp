#include "semUtils.h"
#include <math/dag_color.h>
#include "shExprParser.h"
#include "shsem.h"

using namespace semutils;

// convert from string to integer
int semutils::int_number(const char *s)
{
  G_ASSERT(s);
  G_ASSERT(s[0]);
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    return strtoul(s + 2, NULL, 16);
  int l = (int)strlen(s);
  if (s[l - 1] == 'h' || s[l - 1] == 'H')
    return strtoul(s, NULL, 16);
  return strtoul(s, NULL, 10);
}

// convert from string to real
double semutils::real_number(const char *s)
{
  G_ASSERT(s && s[0]);
  return strtod(s, NULL);
}

// convert from real decl to real
double semutils::real_number(ShaderTerminal::signed_real *s)
{
  G_ASSERT(s);
  double v = real_number(s->value->text);
  if (s->sign)
    if (s->sign->num == SHADER_TOKENS::SHTOK_minus)
      v = -v;
  return v;
}
