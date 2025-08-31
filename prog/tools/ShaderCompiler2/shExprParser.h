// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  support for shader arithemtic expressions & local variables
/************************************************************************/

#include "shExpr.h"
#include <shaders/shExprTypes.h>
#include <EASTL/stack.h>
#include <dag/dag_vector.h>


struct Parser;
struct LocalVar;

namespace shc
{
class VariantContext;
}

namespace ShaderParser
{

/*********************************
 *
 * class ExpressionParser
 *
 *********************************/
class ExpressionParser
{
public:
  struct Context
  {
    shexpr::ValueType destValueType = shexpr::VT_UNDEFINED;
    bool destIsInteger = false;
    Terminal *destTerm = nullptr;
  };

  // ctor/dtor
  explicit ExpressionParser(shc::VariantContext &ctx);
  explicit ExpressionParser(Parser &parser) : variantCtx{nullptr}, parser{parser} {}

  // parse expression
  bool parseExpression(ShaderTerminal::arithmetic_expr &s, ComplexExpression *e, const Context &ctx) const;

  // parse expression - return true, if it's constant number
  bool parseConstExpression(ShaderTerminal::arithmetic_expr &s, Color4 &ret_value, const Context &ctx) const;

private:
  shc::VariantContext *variantCtx = nullptr;
  Parser &parser;

  // parse expression - * and / operators or single expression
  bool parseSubExpression(ShaderTerminal::arithmetic_expr_md &s, ComplexExpression *e, const Context &ctx) const;

  // parse operand
  bool parseOperand(ShaderTerminal::arithmetic_operand &s, shexpr::OperandType op_type, ComplexExpression *e,
    const Context &ctx) const;

  // parse color
  bool parseColor(ShaderTerminal::arithmetic_color &s, ColorValueExpression *e, const Context &ctx) const;

  // parse color mask
  shexpr::ColorChannel parseColorMask(char channel) const;
}; // class ExpressionParser
//
} // namespace ShaderParser
