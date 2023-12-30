// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include "shExprParser.h"
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


ExpressionParser *ExpressionParser::self = NULL;


ExpressionParser &ExpressionParser::getStatic()
{
  if (!self)
  {
    self = new ExpressionParser;
  }

  return *self;
}


/*********************************
 *
 * class ExpressionParser
 *
 *********************************/
// ctor/dtor
ExpressionParser::ExpressionParser() : owner(NULL), parser(NULL) {}


ExpressionParser::~ExpressionParser() {}

void ExpressionParser::setVar(LocalVar *var, bool is_integer, ComplexExpression *rootExpr, ShaderTerminal::SHTOK_ident *decl_name)
{
  Tab<int> &code = owner->curpass->get_alt_curstcode(rootExpr->isDynamic());

  Register varReg;
  if (var->valueType == shexpr::VT_REAL)
    varReg = owner->add_reg();
  else
    varReg = owner->add_vec_reg();
  if (!rootExpr->assembly(*owner, code, varReg, is_integer))
  {
    error(String(128, "cannot assembly local variable '%s' expression!", decl_name->text), decl_name);
  }
  var->reg = eastl::move(varReg).release();
}

// parse local variable declaration
LocalVar *ExpressionParser::parseLocalVarDecl(local_var_decl &decl)
{
  G_ASSERT(owner);
  shexpr::ValueType valueType;
  bool is_integer = false;
  switch (decl.type->type->number())
  {
    case SHADER_TOKENS::SHTOK_float: valueType = shexpr::VT_REAL; break;
    case SHADER_TOKENS::SHTOK_float4: valueType = shexpr::VT_COLOR4; break;
    case SHADER_TOKENS::SHTOK_int4:
      valueType = shexpr::VT_COLOR4;
      is_integer = true;
      break;
    default: G_ASSERT(0);
  }

  int varNameId = VarMap::addVarId(decl.name->text);

  // check for variable names
  if (ShaderGlobal::get_var_internal_index(varNameId) >= 0)
  {
    error(String(128, "variable with name '%s' already declared as global variable!", decl.name->text), decl.name);
    return nullptr;
  }

  if (owner->code.find_var(varNameId) >= 0)
  {
    error(String(128, "variable with name '%s' already declared!", decl.name->text), decl.name);
    return nullptr;
  }

  if (owner->code.locVars.getVariableId(varNameId) != -1)
  {
    error(String(128, "local variable with name '%s' already declared!", decl.name->text), decl.name);
    return nullptr;
  }

  // registe new variable
  int varId = owner->code.locVars.addVariable(LocalVar(varNameId, valueType));
  LocalVar *var = owner->code.locVars.getVariableById(varId);
  G_ASSERT(var);

  // initialize variable
  ComplexExpression rootExpr(valueType);
  if (!parseExpression(*decl.expr, &rootExpr))
    return nullptr;

  if (!rootExpr.collapseNumbers())
    return nullptr;

  var->isDynamic = rootExpr.isDynamic();

  // try to get constant value
  Color4 v;
  if (valueType == shexpr::VT_REAL)
  {
    var->isConst = rootExpr.evaluate(v.r);
  }
  else if (valueType == shexpr::VT_COLOR4)
  {
    var->isConst = rootExpr.evaluate(v);
  }
  if (var->isConst)
  {
    var->cv.c.set(v);
    return var;
  }

  // assign as dynamic variable
  if (!rootExpr.canConvert(valueType))
  {
    error(String(128, "local variable '%s' must be initialized with %s expression, not %s (operands = %d, %d)!", decl.name->text,
            Expression::__getName(valueType), Expression::__getName(rootExpr.getValueType()), rootExpr.getOperandCount(),
            rootExpr.getType()),
      decl.name);
  }

  // add value of this variable as dynamic value
  setVar(var, is_integer, &rootExpr, decl.name);
  return var;
}

// parse expression - * and / operators or single expression
bool ExpressionParser::parseSubExpression(ShaderTerminal::arithmetic_expr_md &s, ComplexExpression *e)
{
  G_ASSERT(e);

  // parse left operand
  G_ASSERT(s.lhs);
  if (!parseOperand(*s.lhs, shexpr::OPER_LEFT, e))
    return false;

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
    if (!parseOperand(*s.rhs[i], shexpr::OPER_USER, e))
      return false;
  }

  return true;
}


// parse expression
bool ExpressionParser::parseExpression(ShaderTerminal::arithmetic_expr &s, ComplexExpression *e)
{
  G_ASSERT(e);
  // parse left operand
  G_ASSERT(s.lhs);
  ComplexExpression *newExpr = new ComplexExpression(e->getValueType(), e->getCurrentChannel());
  e->setOperand(shexpr::OPER_LEFT, newExpr);
  if (!parseSubExpression(*s.lhs, newExpr))
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
    newExpr = new ComplexExpression(e->getValueType(), e->getCurrentChannel());
    e->addOperand(newExpr, opType);
    if (!parseSubExpression(*s.rhs[i], newExpr))
      return false;
  }

  return true;
}


bool ExpressionParser::parseConstExpression(ShaderTerminal::arithmetic_expr &s, Color4 &ret_value)
{
  ComplexExpression real_expr(shexpr::VT_REAL);
  if (!parseExpression(s, &real_expr))
    return false;
  if (real_expr.getValueType() == shexpr::VT_REAL && real_expr.evaluate(ret_value.r))
    return true;

  ComplexExpression color_expr(shexpr::VT_COLOR4);
  if (!parseExpression(s, &color_expr))
    return false;
  return color_expr.evaluate(ret_value);
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
bool ExpressionParser::parseOperand(ShaderTerminal::arithmetic_operand &s, shexpr::OperandType op_type, ComplexExpression *e)
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

  // detect color channel mask
  eastl::unique_ptr<ColorChannelExpression> colorChExpr;
  if (s.cmask)
  {
    int channels = strlen(s.cmask->channel->text);
    if (e->getCurrentChannel() > channels - 1 && channels > 1)
    {
      eastl::string err(eastl::string::CtorSprintf{}, "Wrong color mask %s", s.cmask->channel->text);
      error(err.c_str(), s.cmask->channel);
      return false;
    }

    int index = (e->getCurrentChannel() == -1 || channels == 1) ? 0 : e->getCurrentChannel();
    char channel = s.cmask->channel->text[index];
    shexpr::ColorChannel colorChannel = parseColorMask(channel);
    if (colorChannel == shexpr::_CC_UNDEFINED)
    {
      error("r,g,b,a or x,y,z,w expected!", s.cmask->channel);
      return false;
    }

    e->setChannels(channels);

    colorChExpr.reset(new ColorChannelExpression(s.cmask->channel, colorChannel));
  }

  if (s.expr)
  {
    // it's a complex expression
    ComplexExpression *newExpr = new ComplexExpression(e->getValueType(), e->getCurrentChannel());
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

    return parseExpression(*s.expr, newExpr);
  }
  if (s.var_name)
  {
    // it's a variable
    int varNameId = VarMap::getVarId(s.var_name->text);

    int varId = owner->code.locVars.getVariableId(varNameId);
    LocalVar *locVar = owner->code.locVars.getVariableByName(varNameId);
    if (locVar)
    {
      // it's a local variable
      if (locVar->isConst)
      {
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
            error(String(128, "get component operator must be used with color4() value!"), s.var_name);
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
        LVarValueExpression *newExpr = new LVarValueExpression(s.var_name, varId, owner->code.locVars);

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
    int vt;
    bool isGlobal = false;
    int varIndex = owner->get_state_var(*s.var_name, vt, isGlobal);

    if (varIndex < 0)
    {
      error("variable not found!", s.var_name);
      return false;
    }

    bool isDynamic = isGlobal;
    shexpr::ValueType varType;

    bool isInt = false;

    if (!isGlobal)
    {
      // detect local type
      ShaderSemCode::Var &v = owner->code.vars[varIndex];
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
          error("variables of type TEXTURE are unsupported here!", s.var_name);
          return false;
          break;
        default: G_ASSERT(0);
      }
    }
    else
    {
      // detect global type
      ShaderGlobal::Var &v = ShaderGlobal::get_var(varIndex);

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

    if (colorChExpr && (varType != shexpr::VT_COLOR4))
    {
      error("get component operator must be used with color4() value!", s.var_name);
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
      error("unknown function!", s.func->func_name);
      return false;
    }

    // check function return type
    shexpr::ValueType retType = functional::getValueType(func);

    if (colorChExpr && (retType != shexpr::VT_COLOR4))
    {
      error("get component operator must be used with color4() value!", s.var_name);
      return false;
    }

    // get args types
    functional::ArgList args(tmpmem);
    functional::prepareArgs(func, args);
    if (s.func->param.size() != args.size())
    {
      error(String(128, "invalid argument count: required %d, but found %d", args.size(), s.func->param.size()), s.func->func_name);
      return false;
    }

    // parse arguments
    eastl::unique_ptr<FunctionExpression> newExpr(new FunctionExpression(s.func->func_name, func));
    for (int i = 0; i < args.size(); i++)
    {
      eastl::unique_ptr<ComplexExpression> arg(new ComplexExpression(args[i].vt, e->getCurrentChannel()));
      if (!parseExpression(*s.func->param[i], arg.get()))
        return false;
      if (!arg->collapseNumbers())
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
      error("get component operator must be used with color4() value!", s.real_value->value);
      return false;
    }

    ConstRealValue *newExpr = new ConstRealValue(s.real_value->value);
    e->setOperand(op_type, newExpr);

    newExpr->setUnaryOperator(opType);
    newExpr->setValue(semutils::real_number(s.real_value));
    return true;
  }
  else if (s.color_value)
  {
    // it's a color4 value
    eastl::unique_ptr<ColorValueExpression> newExpr(new ColorValueExpression(s.color_value->decl));
    if (!parseColor(*s.color_value, newExpr.get()))
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
bool ExpressionParser::parseColor(ShaderTerminal::arithmetic_color &s, ColorValueExpression *e)
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

    eastl::unique_ptr<ComplexExpression> expr(new ComplexExpression(shexpr::VT_REAL));
    if (!parseExpression(*expr_to_parse, expr.get()))
      return false;
    int channels = expr->getChannels();
    if (channels > 1)
    {
      for (int j = 0; j < channels; j++)
      {
        expr.reset(new ComplexExpression(shexpr::VT_REAL, j));
        if (!parseExpression(*expr_to_parse, expr.get()))
          return false;
        e->setOperand(shexpr::OperandType(current_operand++), expr.release());
      }
      continue;
    }
    if (expr->getValueType() != shexpr::VT_REAL)
    {
      expr.reset(new ComplexExpression(shexpr::VT_COLOR4));
      if (!parseExpression(*expr_to_parse, expr.get()))
        return false;
    }
    e->setOperand(shexpr::OperandType(current_operand++), expr.release());
  }
  if (current_operand != 4)
  {
    eastl::string err(eastl::string::CtorSprintf{}, "Wrong color expression with %i channels", current_operand);
    error(err.c_str(), s.decl);
    return false;
  }

  return true;
}

// show error
void ExpressionParser::error(const char *msg, Terminal *t)
{
  if (parser)
  {
    if (t)
    {
      eastl::string str = msg;
      if (!t->macro_call_stack.empty())
        str.append("\nCall stack:\n");
      for (auto it = t->macro_call_stack.rbegin(); it != t->macro_call_stack.rend(); ++it)
        str.append_sprintf("  %s()\n    %s(%i)\n", it->name, parser->get_lex_parser().get_filename(it->file), it->line);
      parser->get_lex_parser().set_error(t->file_start, t->line_start, t->col_start, str.c_str());
    }
    else
      parser->get_lex_parser().set_error(msg);
  }
  else
  {
    sh_debug(SHLOG_ERROR, "<unknown location> %s", msg);
  }
}

// set owner
void ExpressionParser::setOwner(AssembleShaderEvalCB *init_owner)
{
  owner = init_owner;
  parser = NULL;

  if (owner)
  {
    parser = &owner->parser;
  }
}


// class ExpressionParser
//
