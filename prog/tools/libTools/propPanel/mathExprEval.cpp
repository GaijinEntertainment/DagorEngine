// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/mathExprEval.h>
#include <math/dag_mathBase.h>  // PI, DEG_TO_RAD, RAD_TO_DEG
#include <math/dag_check_nan.h> // check_finite
#include <math.h>
#include <string.h>

namespace PropPanel
{

namespace
{

// Fixed user-facing messages. Unknown identifiers and undefined characters are instead reported
// by name (see failNotDefined), e.g. "`apple` is not defined".
const char *ERR_INVALID = "Invalid expression";
const char *ERR_DIV_ZERO = "Division by zero";
const char *ERR_COMPLEX = "Complex numbers are not supported";
const char *ERR_UNEXPECTED_END = "Unexpected end of expression";

// Bounds the recursion so pathological input ("((((..." or "----...") cannot overflow the
// stack. Counts parenthesis/function nesting (in parsePrimary) and unary-minus chains.
const int MAX_DEPTH = 64;
const int MAX_IDENT_LEN = 31;

// Characters that appear somewhere in the grammar. A character outside this set, found where a
// value is expected or trailing, is reported as undefined; a recognized one out of place is a
// generic syntax error. ',' is recognized because it separates pow's two arguments.
bool isRecognizedChar(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '+' || c == '-' || c == '*' ||
         c == '/' || c == '^' || c == '(' || c == ')' || c == ',' || c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool isKnownFunc(const char *name)
{
  return strcmp(name, "sqrt") == 0 || strcmp(name, "abs") == 0 || strcmp(name, "sin") == 0 || strcmp(name, "cos") == 0 ||
         strcmp(name, "tan") == 0 || strcmp(name, "atan") == 0;
}

bool isTwoArgFunc(const char *name) { return strcmp(name, "pow") == 0; }

// Single-pass recursive-descent parser that evaluates while it parses (no AST). The first
// error is sticky: once set, every parse function short-circuits and the caller reports it.
//
// Grammar (lowest to highest precedence):
//   expr    := term (('+' | '-') term)*           left-assoc
//   term    := unary (('*' | '/') unary)*         left-assoc
//   unary   := ('+' | '-') unary | power          unary +/- bind looser than '^'
//   power   := primary (('^' | '**') unary)?      right-assoc; RHS is unary so 2^-1 works; '**' aliases '^'
//   primary := number | 'pi' | func '(' args ')' | '(' expr ')'
//   args    := expr (',' expr)?                   one argument, or two for pow(base, exp)
struct Parser
{
  const char *p;
  String error; // empty == no error; the first error set wins
  int depth = 0;

  explicit Parser(const char *s) noexcept : p(s) {}

  bool failed() const { return !error.empty(); }

  void fail(const char *msg)
  {
    if (error.empty())
    {
      error = msg;
    }
  }

  void failNotDefined(const char *token)
  {
    if (error.empty())
    {
      error = "`";
      error += token;
      error += "` is not defined";
    }
  }

  // Report the current character as the cause of a missing-value or trailing error: end of input,
  // a recognized token out of place (generic), or a character the grammar does not define.
  void failHere()
  {
    const char c = *p;
    if (c == '\0')
    {
      fail(ERR_UNEXPECTED_END);
    }
    else if (isRecognizedChar(c))
    {
      fail(ERR_INVALID);
    }
    else
    {
      const char token[2] = {c, '\0'};
      failNotDefined(token);
    }
  }

  void skipWs()
  {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
    {
      ++p;
    }
  }

  float parseExpr()
  {
    float v = parseTerm();
    for (;;)
    {
      if (failed())
      {
        break;
      }
      skipWs();
      const char op = *p;
      if (op != '+' && op != '-')
      {
        break;
      }
      ++p;
      const float rhs = parseTerm();
      v = (op == '+') ? v + rhs : v - rhs;
    }
    return v;
  }

  float parseTerm()
  {
    float v = parseUnary();
    for (;;)
    {
      if (failed())
      {
        break;
      }
      skipWs();
      const char op = *p;
      if (op != '*' && op != '/')
      {
        break;
      }
      ++p;
      const float rhs = parseUnary();
      if (op == '*')
      {
        v = v * rhs;
      }
      else if (rhs == 0.0f)
      {
        fail(ERR_DIV_ZERO);
        break;
      }
      else
      {
        v = v / rhs;
      }
    }
    return v;
  }

  float parseUnary()
  {
    if (failed())
    {
      return 0.0f;
    }
    skipWs();
    if (*p == '-' || *p == '+')
    {
      const char op = *p;
      ++p;
      if (++depth > MAX_DEPTH)
      {
        fail(ERR_INVALID);
        --depth;
        return 0.0f;
      }
      const float v = parseUnary();
      --depth;
      return op == '-' ? -v : v;
    }
    return parsePower();
  }

  float parsePower()
  {
    const float base = parsePrimary();
    skipWs();
    // '**' is an accepted spelling of '^'. It must be matched here, before control returns to
    // parseTerm, which would otherwise read the first '*' as multiplication.
    if (*p == '^' || (*p == '*' && p[1] == '*'))
    {
      p += (*p == '^') ? 1 : 2;
      const float exp = parseUnary();
      return powf(base, exp);
    }
    return base;
  }

  float parsePrimary()
  {
    if (failed())
    {
      return 0.0f;
    }
    if (++depth > MAX_DEPTH)
    {
      fail(ERR_INVALID);
      --depth;
      return 0.0f;
    }
    const float v = parsePrimaryInner();
    --depth;
    return v;
  }

  float parsePrimaryInner()
  {
    skipWs();
    const char c = *p;

    if (c == '(')
    {
      ++p;
      const float v = parseExpr();
      skipWs();
      if (*p != ')')
      {
        fail(*p == '\0' ? ERR_UNEXPECTED_END : ERR_INVALID);
        return 0.0f;
      }
      ++p;
      return v;
    }

    if (c == '.' || (c >= '0' && c <= '9'))
    {
      return parseNumber();
    }

    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
    {
      return parseIdent();
    }

    failHere();
    return 0.0f;
  }

  // Locale-independent: only '.' is a decimal separator, never the C locale's. Parses
  // digits[.digits][(e|E)[+-]digits]. A bare leading '.' or '.e' (no digits) is invalid.
  float parseNumber()
  {
    double mantissa = 0.0;
    bool anyDigit = false;
    while (*p >= '0' && *p <= '9')
    {
      mantissa = mantissa * 10.0 + (*p - '0');
      ++p;
      anyDigit = true;
    }
    if (*p == '.')
    {
      ++p;
      double scale = 0.1;
      while (*p >= '0' && *p <= '9')
      {
        mantissa += (*p - '0') * scale;
        scale *= 0.1;
        ++p;
        anyDigit = true;
      }
    }
    if (!anyDigit)
    {
      fail(ERR_INVALID);
      return 0.0f;
    }

    int exp = 0;
    if (*p == 'e' || *p == 'E')
    {
      const char *save = p;
      ++p;
      int sign = 1;
      if (*p == '+')
      {
        ++p;
      }
      else if (*p == '-')
      {
        sign = -1;
        ++p;
      }
      if (*p >= '0' && *p <= '9')
      {
        while (*p >= '0' && *p <= '9')
        {
          if (exp < 1000000) // saturate to avoid int overflow; pow() yields inf/0 either way
          {
            exp = exp * 10 + (*p - '0');
          }
          ++p;
        }
        exp *= sign;
      }
      else
      {
        p = save; // a bare 'e' is not part of the number -- let the trailing check reject it
      }
    }

    double value = mantissa;
    if (exp != 0)
    {
      value *= pow(10.0, exp);
    }
    return static_cast<float>(value);
  }

  float parseIdent()
  {
    char name[MAX_IDENT_LEN + 1];
    int n = 0;
    while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))
    {
      if (n < MAX_IDENT_LEN)
      {
        name[n] = *p;
      }
      ++n;
      ++p;
    }
    if (n > MAX_IDENT_LEN)
    {
      fail(ERR_INVALID);
      return 0.0f;
    }
    name[n] = '\0';

    if (strcmp(name, "pi") == 0)
    {
      return PI;
    }

    const bool twoArg = isTwoArgFunc(name);
    if (!twoArg && !isKnownFunc(name))
    {
      failNotDefined(name); // unknown identifier, used as a variable or a function
      return 0.0f;
    }

    skipWs();
    if (*p != '(') // a known function must be called with parentheses
    {
      fail(*p == '\0' ? ERR_UNEXPECTED_END : ERR_INVALID);
      return 0.0f;
    }
    ++p;
    const float arg = parseExpr();

    float arg2 = 0.0f;
    if (twoArg) // pow(base, exp): a comma separates the two arguments
    {
      skipWs();
      if (*p != ',')
      {
        fail(*p == '\0' ? ERR_UNEXPECTED_END : ERR_INVALID);
        return 0.0f;
      }
      ++p;
      arg2 = parseExpr();
    }

    skipWs();
    if (*p != ')')
    {
      fail(*p == '\0' ? ERR_UNEXPECTED_END : ERR_INVALID);
      return 0.0f;
    }
    ++p;

    if (failed())
    {
      return 0.0f;
    }
    if (twoArg)
    {
      return powf(arg, arg2); // pow is the only two-arg function; equivalent to '^' and '**'
    }
    return applyFunc(name, arg);
  }

  float applyFunc(const char *name, float x)
  {
    if (strcmp(name, "sqrt") == 0)
    {
      if (x < 0.0f)
      {
        fail(ERR_COMPLEX);
        return 0.0f;
      }
      return sqrtf(x);
    }
    if (strcmp(name, "abs") == 0)
    {
      return fabsf(x);
    }
    if (strcmp(name, "sin") == 0)
    {
      return sinf(x * DEG_TO_RAD);
    }
    if (strcmp(name, "cos") == 0)
    {
      return cosf(x * DEG_TO_RAD);
    }
    if (strcmp(name, "tan") == 0)
    {
      return tanf(x * DEG_TO_RAD);
    }
    if (strcmp(name, "atan") == 0)
    {
      return atanf(x) * RAD_TO_DEG;
    }

    fail(ERR_INVALID);
    return 0.0f;
  }
};

} // namespace

MathExprResult eval_math_expr(const char *expr)
{
  MathExprResult res;

  if (!expr)
  {
    res.error = ERR_UNEXPECTED_END;
    return res;
  }

  Parser parser(expr);
  parser.skipWs();
  if (*parser.p == '\0') // empty / whitespace-only
  {
    res.error = ERR_UNEXPECTED_END;
    return res;
  }

  const float v = parser.parseExpr();

  parser.skipWs();
  if (!parser.failed() && *parser.p != '\0') // unconsumed trailing characters
  {
    parser.failHere();
  }

  if (parser.failed())
  {
    res.error = parser.error;
    return res;
  }

  if (!check_finite(v)) // overflow to inf, or a stray NaN
  {
    res.error = ERR_INVALID;
    return res;
  }

  res.ok = true;
  res.value = v;
  return res;
}

} // namespace PropPanel
