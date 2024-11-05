// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  support for shader arithemtic expressions & local variables
/************************************************************************/

#include "shExpr.h"
#include <shaders/shExprTypes.h>
#include <EASTL/stack.h>
#include <dag/dag_vector.h>


namespace ShaderTerminal
{
class ShaderSyntaxParser;
};
struct LocalVar;


namespace ShaderParser
{
//************************************************************************
//* forwards
//************************************************************************
class AssembleShaderEvalCB;

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
  ExpressionParser();
  ~ExpressionParser();

  static ExpressionParser &getStatic();

  // set owner
  void setOwner(AssembleShaderEvalCB *init_owner);
  void setParser(ShaderTerminal::ShaderSyntaxParser *_parser) { parser = _parser; };

  // parse local variable declaration
  LocalVar *parseLocalVarDecl(ShaderTerminal::local_var_decl &decl, bool ignoreColorDimensionMismatch = false);

  // parse expression
  bool parseExpression(ShaderTerminal::arithmetic_expr &s, ComplexExpression *e, const Context &ctx);

  // parse expression - return true, if it's constant number
  bool parseConstExpression(ShaderTerminal::arithmetic_expr &s, Color4 &ret_value, const Context &ctx);

  // show error
  void error(const char *msg, Symbol *t);

private:
  AssembleShaderEvalCB *owner;
  ShaderTerminal::ShaderSyntaxParser *parser;

  // parse expression - * and / operators or single expression
  bool parseSubExpression(ShaderTerminal::arithmetic_expr_md &s, ComplexExpression *e, const Context &ctx);

  // parse operand
  bool parseOperand(ShaderTerminal::arithmetic_operand &s, shexpr::OperandType op_type, ComplexExpression *e, const Context &ctx);

  // parse color
  bool parseColor(ShaderTerminal::arithmetic_color &s, ColorValueExpression *e, const Context &ctx);

  // parse color mask
  shexpr::ColorChannel parseColorMask(char channel) const;

  void setVar(LocalVar *var, bool is_integer, ComplexExpression *rootExpr, ShaderTerminal::SHTOK_ident *decl_name);

  static ExpressionParser *self;
}; // class ExpressionParser
//
} // namespace ShaderParser
