//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>

namespace PropPanel
{

// Result of evaluating a user-typed math expression.
struct MathExprResult
{
  bool ok = false;
  float value = 0.0f;
  String error; // empty when ok; otherwise a short user-facing message
};

// Safe evaluator for the simple math expressions a user can type into a numeric field.
// Supports + - * / and ^ (also spelled **) with () grouping and unary +/-, the constant pi,
// and the functions sqrt/sin/cos/tan/atan/abs and pow(base, exp) (trig arguments and the atan
// result are in degrees).
//
// It is a plain recursive-descent parser: the input is never executed as code, and the
// function is pure and locale-independent (only '.' is accepted as a decimal separator).
// On failure, error holds a short user-facing message: "Division by zero", "Complex numbers
// are not supported", "Unexpected end of expression", "`<name>` is not defined" (an unknown
// identifier or character), or "Invalid expression". Out-of-range results are NOT an error
// here; clamping to a field's min/max is the caller's responsibility.
MathExprResult eval_math_expr(const char *expr);

} // namespace PropPanel
