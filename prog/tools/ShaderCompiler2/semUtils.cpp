// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "semUtils.h"
#include <math/dag_color.h>
#include "shExprParser.h"
#include "shsem.h"

#include <cerrno>

using namespace semutils;

static __forceinline bool try_ustr_to_int(const char *s, size_t slen, int radix, int min_range, int max_range, int &out)
{
  G_ASSERT(s && slen > 0);

  char *endptr;
  errno = 0;
  const unsigned long l = strtoul(s, &endptr, radix);

  // Error detection for strtoul:
  // 1: String was not parsed at all
  // 2: String was not parsed to the end
  // 3: Invalid input: man(3) states that return val will be 0, and errno will be EINVAL
  // 4: Overflow: man(3) states that return val will be ULONG_MAX, and errno will be ERANGE
  if (endptr == s || *endptr != '\0' || (l == 0 && errno == EINVAL) || (l == ULONG_MAX && errno == ERANGE))
    return false;

  if (l < min_range || l > max_range)
    return false;

  out = (int)l;
  return true;
}

// convert from string to integer
bool semutils::try_int_number(const char *s, int &out)
{
  if (!s || !s[0])
    return false;
  int l = (int)strlen(s);
  int radix;
  if ((s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) || (s[l - 1] == 'h' || s[l - 1] == 'H'))
    radix = 16;
  else
    radix = 10;

  return try_ustr_to_int(s, l, radix, 0, INT_MAX, out);
}

// convert from string to real
bool semutils::try_real_number(const char *s, double &out)
{
  if (!s || !s[0])
    return false;

  char *endptr;
  errno = 0;
  const double d = strtod(s, &endptr);

  // Error detection for strtod:
  // 1: String was not parsed at all
  // 2: String was not parsed to the end
  // 3: Invalid input: man(3) states that return val will be 0.0, and errno will be EINVAL
  // 4: Overflow: man(3) states that return val will be +-HUGE_VAL, and errno will be ERANGE
  // 5: Underflow: man(3) states that return val will be 0.0, and errno will be ERANGE
  if (endptr == s || *endptr != '\0' || (d == 0.0 && errno == EINVAL) || ((abs(d) == HUGE_VAL || d == 0.0) && errno == ERANGE))
    return false;

  out = d;
  return true;
}

int semutils::int_number(const char *s)
{
  int num = 0;
  G_VERIFYF(try_int_number(s, num), "Failed to parse to int: %s", s);
  return num;
}

double semutils::real_number(const char *s)
{
  double d = 0.0;
  G_VERIFYF(try_real_number(s, d), "Failed to parse to double: %s", s);
  return d;
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

// convert from real decl to int (given that the literal denotes an int)
int semutils::int_number(ShaderTerminal::signed_real *s)
{
  G_ASSERT(s);
  int v = int_number(s->value->text);
  if (s->sign)
    if (s->sign->num == SHADER_TOKENS::SHTOK_minus)
      v = -v;
  return v;
}