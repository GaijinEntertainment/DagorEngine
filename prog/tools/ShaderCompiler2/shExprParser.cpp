// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shExprParser.h"
#include "shaderSemantic.h"
#include "cppStcodeAssembly.h"
#include "cppStcodeUtils.h"
#include "variantAssembly.h"
#include "shErrorReporting.h"
#include "varMap.h"
#include "shcode.h"
#include "semUtils.h"
#include <debug/dag_debug.h>
#include "shLog.h"
#include "assemblyShader.h"
#include <shaders/shFunc.h>
#include "globVar.h"
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>

using namespace ShaderParser;
using namespace ShaderTerminal;


/*********************************
 *
 * class ExpressionParser
 *
 *********************************/
// ctor/dtor
ExpressionParser::ExpressionParser(shc::VariantContext &ctx) : variantCtx{&ctx}, parser{ctx.tgtCtx().sourceParseState().parser} {}

// parse expression - * and / operators or single expression
bool ExpressionParser::parseSubExpression(ShaderTerminal::arithmetic_expr_md &s, ComplexExpression *e, const Context &ctx) const
{
  G_ASSERT(e);

  // parse left operand
  G_ASSERT(s.lhs);
  if (!parseOperand(*s.lhs, shexpr::OPER_LEFT, e, ctx))
    return false;

  if (!s.rhs.empty() && ctx.destIsInteger) // @TODO: remove when int arithmetic is implemented
  {
    report_error(parser, ctx.destTerm, "Integer arithmetic is not supported (in expressions for local int# vars and @i# constants)");
    return false;
  }

  // parse right operands
  for (int i = 0; i < s.rhs.size(); i++)
  {
    // parse operator
    G_ASSERT(s.op[i]);
    shexpr::BinaryOperator opType;
    switch (s.op[i]->num)
    {
      case SHADER_TOKENS::SHTOK_mul: opType = shexpr::OP_MUL; break;
      case SHADER_TOKENS::SHTOK_div: opType = shexpr::OP_DIV; break;
      default: G_ASSERT(0);
    }

    e->setLastBinOp(opType);

    // parse right operand
    G_ASSERT(s.rhs[i]);
    if (!parseOperand(*s.rhs[i], shexpr::OPER_USER, e, ctx))
      return false;
  }

  return e->validate(parser);
}


// parse expression
bool ExpressionParser::parseExpression(ShaderTerminal::arithmetic_expr &s, ComplexExpression *e, const Context &ctx) const
{
  G_ASSERT(e);
  // parse left operand
  G_ASSERT(s.lhs);

  // If this expression just holds one subexpr, ignore this one and don't create unneeded nested ComplexExpressions
  if (s.rhs.empty())
    return parseSubExpression(*s.lhs, e, ctx);
  // Otherwise, validate
  else if (ctx.destIsInteger) // @TODO: remove when int arithmetic is implemented
  {
    report_error(parser, ctx.destTerm, "Integer arithmetic is not supported (in expressions for local int# vars and @i# constants)");
    return false;
  }

  ComplexExpression *newExpr = new ComplexExpression(s.lhs, e->getValueType(), e->getCurrentChannel());
  e->setOperand(shexpr::OPER_LEFT, newExpr);
  if (!parseSubExpression(*s.lhs, newExpr, ctx))
    return false;

  // parse right operands
  for (int i = 0; i < s.rhs.size(); i++)
  {
    // parse operator
    G_ASSERT(s.op[i]);
    shexpr::BinaryOperator opType;
    switch (s.op[i]->num)
    {
      case SHADER_TOKENS::SHTOK_plus: opType = shexpr::OP_ADD; break;
      case SHADER_TOKENS::SHTOK_minus: opType = shexpr::OP_SUB; break;
      default: G_ASSERT(0);
    }

    // parse right operand
    G_ASSERT(s.rhs[i]);
    newExpr = new ComplexExpression(s.rhs[i], e->getValueType(), e->getCurrentChannel());
    e->addOperand(newExpr, opType);
    if (!parseSubExpression(*s.rhs[i], newExpr, ctx))
      return false;
  }

  return e->validate(parser);
}


bool ExpressionParser::parseConstExpression(ShaderTerminal::arithmetic_expr &s, Color4 &ret_value, const Context &ctx) const
{
  if (ctx.destValueType == shexpr::VT_BUFFER || ctx.destValueType == shexpr::VT_TEXTURE)
  {
    report_error(parser, ctx.destTerm, "Internal error: trying to parse arithmetic expression for buffer/texture expected type");
    return false;
  }

  // dest_type is either color4, real or undefined (which means both are ok)

  // if we allow the expression to be of real type, try to parse it as such
  if (ctx.destValueType != shexpr::VT_COLOR4)
  {
    ComplexExpression real_expr(&s, shexpr::VT_REAL);
    if (!parseExpression(s, &real_expr, ctx))
      return false;
    if (real_expr.getValueType() == shexpr::VT_REAL && real_expr.evaluate(ret_value.r, parser))
      return true;
    else if (ctx.destValueType == shexpr::VT_REAL)
      return false;
  }

  // if we expect the expression to be color4, or don't expect it to be of a given type
  // and failed to parse it as real, try to parse as color4
  if (ctx.destValueType != shexpr::VT_REAL)
  {
    ComplexExpression color_expr(&s, shexpr::VT_COLOR4);
    if (!parseExpression(s, &color_expr, ctx))
      return false;
    if (color_expr.getChannels() != 1 && color_expr.getChannels() != 4)
    {
      report_error(parser, ctx.destTerm, "Invalid number of channels (%d) for float4 const expression (must be 1 or 4)",
        color_expr.getChannels());
      return false;
    }
    return color_expr.evaluate(ret_value, parser);
  }

  return false;
}

// parse color mask
shexpr::ColorChannel ExpressionParser::parseColorMask(char channel) const
{
  if (strchr("rRxX", channel))
    return shexpr::CC_R;
  if (strchr("gGyY", channel))
    return shexpr::CC_G;
  if (strchr("bBzZ", channel))
    return shexpr::CC_B;
  if (strchr("aAwW", channel))
    return shexpr::CC_A;
  return shexpr::_CC_UNDEFINED;
}


// parse operand
bool ExpressionParser::parseOperand(ShaderTerminal::arithmetic_operand &s, shexpr::OperandType op_type, ComplexExpression *e,
  const Context &ctx) const
{
  if (!e)
    return false;

  // detect unary operator of this operand
  shexpr::UnaryOperator opType = shexpr::UOP_POSITIVE;
  if (s.unary_op)
  {
    switch (s.unary_op->op->num)
    {
      case SHADER_TOKENS::SHTOK_plus: opType = shexpr::UOP_POSITIVE; break;
      case SHADER_TOKENS::SHTOK_minus: opType = shexpr::UOP_NEGATIVE; break;
      default: G_ASSERT(0);
    }
  }

  if (opType == shexpr::UOP_NEGATIVE && ctx.destIsInteger) // @TODO: remove when int arithmetic is implemented
  {
    report_error(parser, ctx.destTerm, "Integer arithmetic is not supported (in expressions for local int# vars and @i# constants)");
    return false;
  }

  // detect color channel mask
  eastl::unique_ptr<ColorChannelExpression> colorChExpr;
  int minRequiredChannels = 1;
  if (s.cmask)
  {
    int channels = strlen(s.cmask->channel->text);
    if (e->getCurrentChannel() > channels - 1 && channels > 1)
    {
      report_error(parser, s.cmask->channel, "Wrong color mask %s", s.cmask->channel->text);
      return false;
    }

    // Check min channels required for the mask to retrieve valid data
    for (int i = 0; i < channels; i++)
    {
      shexpr::ColorChannel colorChannel = parseColorMask(s.cmask->channel->text[i]);
      if (colorChannel == shexpr::_CC_UNDEFINED)
      {
        report_error(parser, s.cmask->channel, "r,g,b,a or x,y,z,w expected!");
        return false;
      }
      int reqChannels = colorChannel - shexpr::CC_R + 1;
      minRequiredChannels = eastl::max(minRequiredChannels, reqChannels);
    }

    int index = (e->getCurrentChannel() == -1 || channels == 1) ? 0 : e->getCurrentChannel();
    char channel = s.cmask->channel->text[index];
    shexpr::ColorChannel colorChannel = parseColorMask(channel);

    e->setChannels(channels);

    // If the current channel of expression is defined (!= -1), it means that we are
    // inside of a ColorValue expression, and all the channels of a mask are parsed
    // separately (see ExpressionParser::parseColor)
    if (channels == 1 || e->getCurrentChannel() != -1)
      colorChExpr.reset(new SingleColorChannelExpression(s.cmask->channel, colorChannel));
    else
    {
      MultiColorChannelExpression::ChannelMask allChannels(channels);
      for (int index = 0; index < channels; index++)
        allChannels[index] = parseColorMask(s.cmask->channel->text[index]);
      colorChExpr.reset(new MultiColorChannelExpression(s.cmask->channel, allChannels));
    }
  }

  if (s.expr)
  {
    // it's a complex expression
    shexpr::ValueType valType = colorChExpr ? shexpr::VT_COLOR4 : e->getValueType();
    ComplexExpression *newExpr = new ComplexExpression(s.expr, valType, e->getCurrentChannel());
    newExpr->setUnaryOperator(opType);

    if (colorChExpr)
    {
      colorChExpr->setChild(newExpr);
      e->setOperand(op_type, colorChExpr.get());
      colorChExpr.release();
    }
    else
    {
      e->setOperand(op_type, newExpr);
    }

    if (!parseExpression(*s.expr, newExpr, ctx))
      return false;
    if (newExpr->getChannels() < minRequiredChannels)
    {
      report_error(parser, s.cmask->channel,
        "Expression only has %d channels while at least %d channels are required for the '%s' mask", newExpr->getChannels(),
        minRequiredChannels, s.cmask->channel->text);
      return false;
    }
    return true;
  }
  if (s.var_name)
  {
    // it's a variable
    int varNameId = variantCtx->tgtCtx().varNameMap().getVarId(s.var_name->text);

    int varId = variantCtx->localStcodeVars().getVariableId(varNameId);
    const LocalVar *locVar = variantCtx->localStcodeVars().getVariableByName(varNameId);

    if (locVar)
    {
      if (!locVar->isInteger && ctx.destIsInteger) // @TODO: remove when int arithmetic is implemented
      {
        report_error(parser, ctx.destTerm, "Floating point local var %s is used in integer expression", s.var_name->text);
        return false;
      }

      // it's a local variable
      if (locVar->isConst)
      {
        // Sanity check
        G_ASSERT(locVar->valueType == shexpr::VT_REAL || locVar->valueType == shexpr::VT_COLOR4);

        // add this as numeric constant
        if (locVar->valueType == shexpr::VT_COLOR4)
        {
          // add this as color4 constant
          ConstColor4Value *newExpr = new ConstColor4Value(s.var_name, Color4::rgbV(locVar->cv.c, locVar->cv.c.a));

          if (colorChExpr)
          {
            colorChExpr->setChild(newExpr);
            e->setOperand(op_type, colorChExpr.get());
            colorChExpr.release();
          }
          else
          {
            e->setOperand(op_type, newExpr);
          }

          newExpr->setUnaryOperator(opType);
          return true;
        }
        else
        {
          // add this as real constant
          if (colorChExpr)
          {
            report_error(parser, s.var_name, "get component operator must be used with color4() value!");
            return false;
          }
          ConstRealValue *newExpr = new ConstRealValue(s.var_name, locVar->cv.r);
          e->setOperand(op_type, newExpr);

          newExpr->setUnaryOperator(opType);
          return true;
        }
      }
      else
      {
        // add this as local variable
        LVarValueExpression *newExpr = new LVarValueExpression(s.var_name, *locVar);

        if (colorChExpr)
        {
          colorChExpr->setChild(newExpr);
          e->setOperand(op_type, colorChExpr.get());
          colorChExpr.release();
        }
        else
        {
          e->setOperand(op_type, newExpr);
        }

        newExpr->setUnaryOperator(opType);
        return true;
      }
    }

    // check for another variable
    auto [varIndex, vt, isGlobal] = semantic::lookup_state_var(*s.var_name, *variantCtx);

    if (varIndex < 0)
    {
      report_error(parser, s.var_name, "variable not found!");
      return false;
    }

    bool isDynamic = isGlobal;
    shexpr::ValueType varType;

    bool isInt = false;

    if (!isGlobal)
    {
      // detect local type
      const ShaderSemCode::Var &v = variantCtx->parsedSemCode().vars[varIndex];
      if (v.dynamic)
        isDynamic = true;

      switch (v.type)
      {
        case SHVT_REAL: varType = shexpr::VT_REAL; break;
        case SHVT_COLOR4: varType = shexpr::VT_COLOR4; break;
        case SHVT_INT4:
          varType = shexpr::VT_COLOR4;
          isInt = true;
          break;

        case SHVT_INT:
          varType = shexpr::VT_REAL;
          isInt = true;
          break;

        case SHVT_TEXTURE:
          report_error(parser, s.var_name, "variables of type TEXTURE are unsupported here!");
          return false;
          break;
        // @TODO: validate all types
        default: G_ASSERT(0);
      }
    }
    else
    {
      // detect global type
      const ShaderGlobal::Var &v = variantCtx->tgtCtx().globVars().getVar(varIndex);

      switch (v.type)
      {
        case SHVT_REAL: varType = shexpr::VT_REAL; break;
        case SHVT_COLOR4: varType = shexpr::VT_COLOR4; break;
        case SHVT_INT4:
          varType = shexpr::VT_COLOR4;
          isInt = true;
          break;

        case SHVT_INT:
          varType = shexpr::VT_REAL;
          isInt = true;
          break;

        case SHVT_TEXTURE: varType = shexpr::VT_TEXTURE; break;
        case SHVT_BUFFER: varType = shexpr::VT_BUFFER; break;
        default: G_ASSERT(0);
      }
    }

    if (!isInt && ctx.destIsInteger) // @TODO: remove when int arithmetic is implemented
    {
      const char *varTypeText =
        varType == shexpr::VT_TEXTURE ? "Texture" : (varType == shexpr::VT_BUFFER ? "Buffer" : "Floating point");
      report_error(parser, ctx.destTerm, "%s %s var %s is used in integer expression", varTypeText, isGlobal ? "global" : "static",
        s.var_name->text);
      return false;
    }

    if (colorChExpr && (varType != shexpr::VT_COLOR4))
    {
      report_error(parser, s.var_name, "get component operator must be used with color4() value!");
      return false;
    }

    StVarValueExpression *newExpr = new StVarValueExpression(s.var_name, varIndex, varType, isDynamic, isGlobal, isInt);

    if (colorChExpr)
    {
      colorChExpr->setChild(newExpr);
      e->setOperand(op_type, colorChExpr.get());
      colorChExpr.release();
    }
    else
    {
      e->setOperand(op_type, newExpr);
    }

    newExpr->setUnaryOperator(opType);

    return true;
  }
  else if (s.func)
  {
    // it's arithmetic function

    // detect id
    functional::FunctionId func;
    if (!functional::getFuncId(s.func->func_name->text, func))
    {
      report_error(parser, s.func->func_name, "unknown function!");
      return false;
    }

    // check function return type
    shexpr::ValueType retType = functional::getValueType(func);
    bool retIsInteger = functional::getValueTypeIsInteger(func);

    if (colorChExpr && (retType != shexpr::VT_COLOR4))
    {
      report_error(parser, s.var_name, "get component operator must be used with color4() value!");
      return false;
    }

    // @TODO: refactor codebase to remove these cases and reenable, or do some of these functions actually work properly?
    // if (!retIsInteger && ctx.destIsInteger) // @TODO: remove when int arithmetic is implemented
    // {
    //   const char *varTypeText =
    //     retType == shexpr::VT_TEXTURE ? "Texture" : (retType == shexpr::VT_BUFFER ? "Buffer" : "Floating point");
    //   report_error(parser, ctx.destTerm, "%s-returning function %s is used in integer expression", varTypeText,
    //     s.func->func_name->text);
    //   return false;
    // }

    // get args types
    functional::ArgList args(tmpmem);
    functional::prepareArgs(func, args);
    if (s.func->param.size() != args.size())
    {
      report_error(parser, s.func->func_name, "invalid argument count: required %d, but found %d", args.size(), s.func->param.size());
      return false;
    }

    // parse arguments
    eastl::unique_ptr<FunctionExpression> newExpr(new FunctionExpression(s.func->func_name, func, e->getCurrentChannel()));
    for (int i = 0; i < args.size(); i++)
    {
      eastl::unique_ptr<ComplexExpression> arg(new ComplexExpression(s.func->param[i], args[i].vt, e->getCurrentChannel()));

      // Function arguments have to answer to the type requirements of the function, not the destination expression
      // @TODO: support integer numeric function args
      if (!parseExpression(*s.func->param[i], arg.get(), Context{args[i].vt, false, s.func->func_name}))
        return false;

      // @HACK: this is done so as not to break these type of expressions: (v.xy, 1.0/max(1,v.xy))
      // i.e. expressions where functions that take real args take vectors instead inside color values.
      // max is a real (real, real) function, but due to the way color values work now this
      // expression is treated as (v.x, v.y, 1.0/max(1,z.x), 1.0/max(1,z.y)), and it works in existing code.
      //
      // @TODO: however, this should be removed, because doing the same outside a color value does not work like that
      // and this would break if we were to call some function that should act on vectors not coordinate-wise
      bool argHasAllowedType = (e->getCurrentChannel() != -1 && args[i].vt == shexpr::VT_REAL) || arg->canConvert(args[i].vt);
      if (!argHasAllowedType)
      {
        report_error(parser, s.func->func_name, "cannot convert function arg from '%s' to '%s' here",
          Expression::__getName(arg->getValueType()), Expression::__getName(args[i].vt));
        return false;
      }
      if (!arg->collapseNumbers(parser))
        return false;

      newExpr->setOperand(shexpr::OperandType(i), arg.get());
      arg.release();
    }

    // register function operand
    if (colorChExpr)
    {
      colorChExpr->setChild(newExpr.get());
      e->setOperand(op_type, colorChExpr.get());
      colorChExpr.release();
    }
    else
    {
      e->setOperand(op_type, newExpr.get());
    }

    newExpr->setUnaryOperator(opType);
    newExpr.release();

    return true;
  }
  else if (s.real_value)
  {
    // it's a real value
    if (colorChExpr)
    {
      report_error(parser, s.real_value->value, "get component operator must be used with color4() value!");
      return false;
    }

    ConstRealValue *newExpr = new ConstRealValue(s.real_value->value);
    e->setOperand(op_type, newExpr);

    newExpr->setUnaryOperator(opType);

    if (ctx.destIsInteger)
    {
      int val = semutils::int_number(s.real_value);

      // bit cast is "safe" in integer context: validation disallows arithmetic, and it just gets passed to the destination
      newExpr->setValue(*(float *)&val, false, parser);
    }
    else
      newExpr->setValue(semutils::real_number(s.real_value), true, parser);

    return true;
  }
  else if (s.color_value)
  {
    // it's a color4 value
    eastl::unique_ptr<ColorValueExpression> newExpr(new ColorValueExpression(s.color_value->decl));
    if (!parseColor(*s.color_value, newExpr.get(), ctx))
      return false;

    if (colorChExpr)
    {
      colorChExpr->setChild(newExpr.get());
      e->setOperand(op_type, colorChExpr.get());
      colorChExpr.release();
    }
    else
    {
      e->setOperand(op_type, newExpr.get());
    }

    newExpr->setUnaryOperator(opType);
    newExpr.release();

    return true;
  }
  return false;
}


// parse color
bool ExpressionParser::parseColor(ShaderTerminal::arithmetic_color &s, ColorValueExpression *e, const Context &ctx) const
{
  if (!e)
    return false;

  int current_operand = 0;
  for (auto expr_to_parse : {s.expr0, s.expr1, s.expr2, s.expr3})
  {
    if (!expr_to_parse)
    {
      if (current_operand >= 4)
        break;
      e->setOperand(shexpr::OperandType(current_operand++), new ConstRealValue(s.decl, 0));
      continue;
    }

    eastl::unique_ptr<ComplexExpression> expr(new ComplexExpression(expr_to_parse, shexpr::VT_REAL, 0));
    if (!parseExpression(*expr_to_parse, expr.get(), ctx))
      return false;
    int channels = expr->getChannels();
    if (channels > 1)
    {
      for (int j = 0; j < channels; j++)
      {
        expr.reset(new ComplexExpression(expr_to_parse, shexpr::VT_REAL, j));
        if (!parseExpression(*expr_to_parse, expr.get(), ctx))
          return false;
        e->setOperand(shexpr::OperandType(current_operand++), expr.release());
      }
      continue;
    }
    if (expr->getValueType() != shexpr::VT_REAL)
    {
      expr.reset(new ComplexExpression(expr_to_parse, shexpr::VT_COLOR4, 0));
      if (!parseExpression(*expr_to_parse, expr.get(), ctx))
        return false;
    }
    e->setOperand(shexpr::OperandType(current_operand++), expr.release());
  }
  if (current_operand != 4)
  {
    report_error(parser, s.decl, "Wrong color expression with %i channels", current_operand);
    return false;
  }

  return true;
}

// class ExpressionParser
//
