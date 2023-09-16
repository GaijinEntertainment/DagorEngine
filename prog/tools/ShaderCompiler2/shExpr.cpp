// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <float.h>
#include "shExpr.h"
#include "shcode.h"
#include "assemblyShader.h"
#include <debug/dag_debug.h>
#include "shExprParser.h"
#include "globVar.h"
#include <shaders/shFunc.h>
#include <generic/dag_tabUtils.h>
#include <shaders/shOpcodeFormat.h>
#include <shaders/shOpcode.h>

using namespace ShaderParser;
using namespace ShaderTerminal;

/*********************************
 *
 * class Expression
 *
 *********************************/
// return string name
const char *Expression::__getName(shexpr::ValueType vt)
{
  switch (vt)
  {
    case shexpr::VT_COLOR4: return "color4"; break;
    case shexpr::VT_REAL: return "real"; break;
    case shexpr::VT_UNDEFINED: return "undefined"; break;
  }
  return "<unknown>";
}

const char *Expression::__getName(shexpr::BinaryOperator op)
{
  switch (op)
  {
    case shexpr::OP_ADD: return "+"; break;
    case shexpr::OP_SUB: return "-"; break;
    case shexpr::OP_MUL: return "*"; break;
    case shexpr::OP_DIV: return "/"; break;
    case shexpr::_OP_UNDEFINED: return "<none>"; break;
  }
  return "<unknown>";
}

const char *Expression::__getName(shexpr::ColorChannel cc)
{
  switch (cc)
  {
    case shexpr::CC_R: return ".r"; break;
    case shexpr::CC_G: return ".g"; break;
    case shexpr::CC_B: return ".b"; break;
    case shexpr::CC_A: return ".a"; break;
    case shexpr::_CC_UNDEFINED: return "<none>"; break;
  }
  return "<unknown>";
}


// add register
Register Expression::addReg(AssembleShaderEvalCB &owner)
{
  shexpr::ValueType vt = getValueType();
  switch (vt)
  {
    case shexpr::VT_REAL: return owner.add_reg();
    case shexpr::VT_COLOR4: return owner.add_vec_reg();
    default:
      owner.error(String(128, "registers in expression of value type '%s' %d, are invalid!", __getName(vt), vt), getTerminal());
      return {};
  }
}


// assembly this expression
bool Expression::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  // assembly unary operator
  if (getUnaryOperator() == shexpr::UOP_NEGATIVE)
  {
    if (getValueType() == shexpr::VT_REAL)
      code.push_back(shaderopcode::makeOp2(SHCOD_INVERSE, int(dest_reg), 1));
    else
      code.push_back(shaderopcode::makeOp2(SHCOD_INVERSE, int(dest_reg), 4));
  }

  return true;
}


// assembly numeric constant
void Expression::assemblyConstant(CodeTable &code, real v, int dest_reg, bool is_integer)
{
  int vx = *(int *)&v;
  if (is_integer)
  {
    code.push_back(shaderopcode::makeOp1(SHCOD_IMM_REAL, dest_reg));
    code.push_back((int)v);
  }
  else if ((vx & 0xFFFF) == 0)
    code.push_back(shaderopcode::makeOp2_8_16(SHCOD_IMM_REAL1, dest_reg, vx >> 16));
  else
  {
    code.push_back(shaderopcode::makeOp1(SHCOD_IMM_REAL, dest_reg));
    code.push_back(vx);
  }
}


void Expression::assemblyConstant(CodeTable &code, const Color4 &v, int dest_reg)
{
  int *col = (int *)&v.r;
  if (col[0] == col[1] && col[0] == col[2] && col[0] == col[3] && (col[0] & 0xFFFF) == 0)
  {
    code.push_back(shaderopcode::makeOp2_8_16(SHCOD_IMM_SVEC1, dest_reg, col[0] >> 16));
    return;
  }
  code.push_back(shaderopcode::makeOp1(SHCOD_IMM_VEC, dest_reg));
  append_items(code, 4, col);
}

// evaluate this expression as real
bool Expression::evaluate(real &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  // apply unary operator
  if (getUnaryOperator() == shexpr::UOP_NEGATIVE)
  {
    out_value = -out_value;
  }

  return true;
}

// evaluate this expression as color4
bool Expression::evaluate(Color4 &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  // apply unary operator
  if (getUnaryOperator() == shexpr::UOP_NEGATIVE)
  {
    out_value = -out_value;
  }

  return true;
}

// dump to debug
void Expression::dump(int level) const
{
  String r;
  for (int i = 0; i < level * 2; i++)
    r += " ";
  dump_internal(level, r);
}

// dump to debug
void Expression::dump_internal(int level, const char *tabs) const
{
  String us("???");
  switch (unaryOp)
  {
    case shexpr::UOP_NEGATIVE: us = "-"; break;
    case shexpr::UOP_POSITIVE: us = "+"; break;
  }

  debug("%sunary_op=%s", tabs, us.str());
}

// class Expression
//


/*********************************
 *
 * class ComplexExpression
 *
 *********************************/
// ctor/dtor
ComplexExpression::ComplexExpression(shexpr::ValueType vt, int channel) :
  binOp(midmem), baseValueType(vt), operands(midmem), currentChannel(channel)
{
  resizeOperands(1);
}


ComplexExpression::~ComplexExpression() { tabutils::deleteAll(operands); }


// assembly this expression
bool ComplexExpression::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  if (operands.size() == 1)
  {
    Register original_dest = dest_reg;
    if (operands[shexpr::OPER_LEFT]->assembly(owner, code, dest_reg, is_integer))
    {
      if (operands[0]->getValueType() == shexpr::VT_REAL && baseValueType == shexpr::VT_COLOR4)
      {
        code.push_back(shaderopcode::makeOp3(SHCOD_MAKE_VEC, int(original_dest), int(dest_reg), int(dest_reg)));
        code.push_back(shaderopcode::makeData2(int(dest_reg), int(dest_reg)));
        dest_reg = original_dest;
      }
      return Expression::assembly(owner, code, dest_reg, is_integer);
    }
    return false;
  }

  // assembly operands
  eastl::vector<Register> opRegs;
  opRegs.reserve(operands.size());
  for (int i = 0; i < operands.size(); i++)
  {
    opRegs.emplace_back(addReg(owner));
    Register original_dest = opRegs[i];
    if (!operands[i]->assembly(owner, code, opRegs[i], is_integer))
      return false;

    if (operands[i]->getValueType() == shexpr::VT_REAL && baseValueType == shexpr::VT_COLOR4)
    {
      code.push_back(shaderopcode::makeOp3(SHCOD_MAKE_VEC, int(original_dest), int(opRegs[i]), int(opRegs[i])));
      code.push_back(shaderopcode::makeData2(int(opRegs[i]), int(opRegs[i])));
      opRegs[i] = original_dest;
    }
  }

  static const int binOperatorMapR[] = {SHCOD_ADD_REAL, SHCOD_SUB_REAL, SHCOD_MUL_REAL, SHCOD_DIV_REAL};

  static const int binOperatorMapC[] = {SHCOD_ADD_VEC, SHCOD_SUB_VEC, SHCOD_MUL_VEC, SHCOD_DIV_VEC};

  // assembly binary operators
  Register srcRegL = opRegs[0];
  int opCode;
  Register destReg;
  for (int i = 1; i < operands.size(); i++)
  {
    G_ASSERT(operands[i]);

    G_ASSERT(binOp[i] != shexpr::_OP_UNDEFINED);
    switch (baseValueType)
    {
      case shexpr::VT_REAL: opCode = binOperatorMapR[binOp[i]]; break;
      case shexpr::VT_COLOR4: opCode = binOperatorMapC[binOp[i]]; break;
      default: G_ASSERT(0);
    }

    if (i == operands.size() - 1)
    {
      destReg = dest_reg;
    }
    else
    {
      destReg = addReg(owner);
    }

    code.push_back(shaderopcode::makeOp3(opCode, int(destReg), int(srcRegL), int(opRegs[i])));
    srcRegL = destReg;
  }

  return Expression::assembly(owner, code, dest_reg, is_integer);
}


// set operand
void ComplexExpression::setOperand(shexpr::OperandType op, Expression *child)
{
  if (op == shexpr::OPER_USER)
  {
    // G_ASSERT(lastBinOp != shexpr::_OP_UNDEFINED);
    addOperand(child, lastBinOp);
    lastBinOp = shexpr::_OP_UNDEFINED;
    return;
  }

  if (op >= operands.size())
    return;

  operands[op] = child;

  if (child)
  {
    child->SetParent(this);
  }
}


// return value type
shexpr::ValueType ComplexExpression::getValueType() const
{
  if (operands.size() == 1 && operands[0] && operands[0]->getValueType() == shexpr::VT_REAL && baseValueType == shexpr::VT_COLOR4)
    return baseValueType;

  for (int i = 0; i < operands.size(); i++)
  {
    if (!operands[i])
      continue;

    if ((operands[i]->getValueType() != baseValueType) && !operands[i]->canConvert(baseValueType))
      return shexpr::VT_UNDEFINED;
  }

  return baseValueType;
}

int ComplexExpression::getChannels() const
{
  int ch = channels;
  for (auto operand : operands)
    ch = eastl::max(ch, operand->getChannels());
  return ch;
}


// check convertion
bool ComplexExpression::canConvert(shexpr::ValueType vt) const
{
  if (operands.size() == 1 && operands[0] && baseValueType == shexpr::VT_COLOR4 &&
      (operands[0]->getValueType() == shexpr::VT_REAL || operands[0]->getValueType() == shexpr::VT_COLOR4))
    return true;

  for (int i = 0; i < operands.size(); i++)
  {
    if (operands[i] && !operands[i]->canConvert(vt))
      return false;
  }

  return true;
}

// return true, if expression has only numeric constants and local variables
bool ComplexExpression::isConst() const
{
  for (int i = 0; i < operands.size(); i++)
  {
    if (operands[i] && !operands[i]->isConst())
      return false;
  }

  return true;
}


// evaluate this expression as real
bool ComplexExpression::evaluate(real &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  G_ASSERT(operands.size());

  if (!operands[shexpr::OPER_LEFT])
    return false;
  if (operands.size() == 1)
  {
    // single operand
    if (!operands[shexpr::OPER_LEFT]->evaluate(out_value))
      return false;
  }
  else
  {
    // many operands
    real value;
    for (int i = 0; i < operands.size(); i++)
    {
      G_ASSERT(operands[i]);
      if (!operands[i]->evaluate(value))
        return false;

      switch (getBinOperator(shexpr::OperandType(i)))
      {
        case shexpr::OP_ADD: out_value += value; break;
        case shexpr::OP_SUB: out_value -= value; break;
        case shexpr::OP_DIV:
        {
          if (value == 0)
          {
            ExpressionParser::getStatic().error("divide by zero error!", NULL);
            return false;
          }
          out_value /= value;
          break;
        }
        case shexpr::OP_MUL: out_value *= value; break;
        case shexpr::_OP_UNDEFINED: out_value = value; break;
        default: G_ASSERT(0);
      }
    }
  }

  return Expression::evaluate(out_value);
}

// evaluate this expression as color4
bool ComplexExpression::evaluate(Color4 &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  G_ASSERT(operands.size());

  if (!operands[shexpr::OPER_LEFT])
    return false;
  if (operands.size() == 1)
  {
    // single operand
    if (!operands[shexpr::OPER_LEFT]->evaluate(out_value))
      return false;
  }
  else
  {
    // many operands
    Color4 value;
    for (int i = 0; i < operands.size(); i++)
    {
      G_ASSERT(operands[i]);
      if (!operands[i]->evaluate(value))
        return false;

      switch (getBinOperator(shexpr::OperandType(i)))
      {
        case shexpr::OP_ADD: out_value += value; break;
        case shexpr::OP_SUB: out_value -= value; break;
        case shexpr::OP_DIV:
        {
          debug("%.4f %.4f %.4f %.4f", out_value.r, out_value.g, out_value.b, out_value.a);
          if (value.r == 0)
          {
            ExpressionParser::getStatic().error("divide by zero error - color4.r = 0!", NULL);
            return false;
          }
          if (value.g == 0)
          {
            ExpressionParser::getStatic().error("divide by zero error - color4.g = 0!", NULL);
            return false;
          }
          if (value.b == 0)
          {
            ExpressionParser::getStatic().error("divide by zero error - color4.b = 0!", NULL);
            return false;
          }
          if (value.a == 0)
          {
            ExpressionParser::getStatic().error("divide by zero error - color4.a = 0!", NULL);
            return false;
          }

          out_value /= value;
          break;
        }
        case shexpr::OP_MUL: out_value *= value; break;
        case shexpr::_OP_UNDEFINED: out_value = value; break;
        default: G_ASSERT(0);
      }
    }
  }

  return Expression::evaluate(out_value);
}


// resize operands
void ComplexExpression::resizeOperands(int count)
{
  tabutils::safeResize(operands, count);
  binOp.resize(count);
  tabutils::setAll(binOp, shexpr::_OP_UNDEFINED);
}


// add user operand
void ComplexExpression::addOperand(Expression *child, shexpr::BinaryOperator t)
{
  operands.push_back(child);
  binOp.push_back(t);

  if (child)
  {
    child->SetParent(this);
  }
}

// get/set operator
void ComplexExpression::setBinOperator(shexpr::OperandType op, shexpr::BinaryOperator t)
{
  if ((op < 0) || (op >= binOp.size()))
    return;
  binOp[op] = t;
}


shexpr::BinaryOperator ComplexExpression::getBinOperator(shexpr::OperandType op) const
{
  if (op < 0 || op >= binOp.size())
    return shexpr::_OP_UNDEFINED;
  return binOp[op];
}


// return operator for specific expression. return UNDEFINED, if operator not found
shexpr::BinaryOperator ComplexExpression::getBinOperator(const Expression *e) const
{
  if (e)
  {
    for (int i = 0; i < operands.size(); i++)
    {
      if (operands[i] == e)
        return binOp[i];
    }
  }
  return shexpr::_OP_UNDEFINED;
}


// dump to debug
void ComplexExpression::dump_internal(int level, const char *tabs) const
{
  debug("%s<ComplexExpression>-%s", tabs, Expression::__getName(baseValueType));
  Expression::dump_internal(level, tabs);

  for (int i = 0; i < operands.size(); i++)
  {
    debug("%sbin_op=%s", tabs, Expression::__getName(binOp[i]));
    operands[i]->dump(level + 1);
  }

  debug("%s</ComplexExpression>", tabs);
}


// evaluate all avalible branches; return false if error occuried
bool ComplexExpression::collapseNumbers()
{
  for (int i = 0; i < operands.size(); i++)
  {
    // if operator is constant, calculate it now
    Expression *op = operands[i];
    if (op->isConst())
    {
      Color4 v;
      if (baseValueType == shexpr::VT_COLOR4)
      {
        if (op->getType() != shexpr::E_CONST_COLOR4)
        {
          // calc color4
          if (op->evaluate(v))
          {
            // replace with constant color4 value
            Terminal *t = op->getTerminal();
            delete op;
            setOperand(shexpr::OperandType(i), new ConstColor4Value(t, v));
          }
        }
      }
      else
      {
        // calc real
        if (op->evaluate(v.r))
        {
          // replace with constant real value
          Terminal *t = op->getTerminal();
          delete op;
          setOperand(shexpr::OperandType(i), new ConstRealValue(t, v.r));
        }
      }
    }
    else
    {
      if (!op->collapseNumbers())
        return false;
    }
  }

  return true;
}

// return true, if expression - is a dynamic expression
bool ComplexExpression::isDynamic() const
{
  for (int i = 0; i < operands.size(); i++)
  {
    if (operands[i]->isDynamic())
      return true;
  }

  return false;
}


// class ComplexExpression
//


/*********************************
 *
 * class ColorChannelExpression
 *
 *********************************/
// ctor/dtor
ColorChannelExpression::ColorChannelExpression(Terminal *s, shexpr::ColorChannel cc) : t(s), colorChannel(cc) {}

ColorChannelExpression::~ColorChannelExpression() {}


// return value type
shexpr::ValueType ColorChannelExpression::getValueType() const
{
  return (colorChannel == shexpr::_CC_UNDEFINED) ? child->getValueType() : shexpr::VT_REAL;
}


// assembly this expression
bool ColorChannelExpression::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  Register srcReg = owner.add_vec_reg();
  if (!child->assembly(owner, code, srcReg, is_integer))
    return false;
  dest_reg.reset(int(srcReg) + colorChannel);
  return true;
}


// check convertion - if fail, report error & return false
bool ColorChannelExpression::canConvert(shexpr::ValueType vt) const
{
  // real to color - accept only if used in multiply or div on the right side
  if (vt == shexpr::VT_COLOR4)
  {
    Expression *p = getParent();
    if (p->getType() == shexpr::E_COMPLEX)
    {
      ComplexExpression *cp = static_cast<ComplexExpression *>(p);
      shexpr::BinaryOperator binOp = cp->getBinOperator(this);
      if ((cp->getOperand(shexpr::OPER_LEFT) != this) && ((binOp == shexpr::OP_MUL) || (binOp == shexpr::OP_DIV)))
      {
        return true;
      }
    }
    return false;
  }

  // check by default
  if (colorChannel == shexpr::_CC_UNDEFINED)
  {
    return child->canConvert(vt);
  }
  else
  {
    return getValueType() == vt;
  }
}

// return true, if expression has only numeric constants and constant local variables
bool ColorChannelExpression::isConst() const { return child->isConst(); }


// return true, if expression - is a dynamic expression
bool ColorChannelExpression::isDynamic() const { return child->isDynamic(); }


// evaluate this expression as real
bool ColorChannelExpression::evaluate(real &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  if (!canConvert(shexpr::VT_REAL))
  {
    ExpressionParser::getStatic().error(String(128, "cannot convert from '%s' to '%s' here", Expression::__getName(getValueType()),
                                          Expression::__getName(shexpr::VT_REAL)),
      t);
    return false;
  }

  if (colorChannel == shexpr::_CC_UNDEFINED)
  {
    return child->evaluate(out_value);
  }
  else
  {
    Color4 v;
    if (!child->evaluate(v))
      return false;
    switch (colorChannel)
    {
      case shexpr::CC_R: out_value = v.r; break;
      case shexpr::CC_G: out_value = v.g; break;
      case shexpr::CC_B: out_value = v.b; break;
      case shexpr::CC_A: out_value = v.a; break;
      default: G_ASSERT(0);
    }
    return true;
  }

  return false;
}


// evaluate this expression as color4
bool ColorChannelExpression::evaluate(Color4 &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  if (!canConvert(shexpr::VT_COLOR4))
  {
    ExpressionParser::getStatic().error(String(128, "cannot convert from '%s' to '%s' here", Expression::__getName(getValueType()),
                                          Expression::__getName(shexpr::VT_COLOR4)),
      t);
    return false;
  }

  if (colorChannel == shexpr::_CC_UNDEFINED)
  {
    return child->evaluate(out_value);
  }

  Color4 v;
  if (!child->evaluate(v))
    return false;

  switch (colorChannel)
  {
    case shexpr::CC_R: out_value = Color4(v.r, v.r, v.r, v.r); break;
    case shexpr::CC_G: out_value = Color4(v.g, v.g, v.g, v.g); break;
    case shexpr::CC_B: out_value = Color4(v.b, v.b, v.b, v.b); break;
    case shexpr::CC_A: out_value = Color4(v.a, v.a, v.a, v.a); break;
    default: G_ASSERT(0);
  }
  return true;
}


// dump to debug
void ColorChannelExpression::dump_internal(int level, const char *tabs) const
{
  debug("%s<ColorChannelExpression>-%s", tabs, Expression::__getName(colorChannel));
  Expression::dump_internal(level, tabs);

  child->dump(level + 1);

  debug("%s</ColorChannelExpression>", tabs);
}


// evaluate all avalible branches; return false if error occuried
bool ColorChannelExpression::collapseNumbers() { return child->collapseNumbers(); }


// class ColorChannelExpression
//


/*********************************
 *
 * class ConstRealValue
 *
 *********************************/
// ctor/dtor
ConstRealValue::ConstRealValue(Terminal *s, real v) : t(s), value(v) {}


ConstRealValue::~ConstRealValue() {}


// assembly this expression
bool ConstRealValue::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  Expression::assemblyConstant(code, value, int(dest_reg), is_integer);
  return true;
}


// set associated value
void ConstRealValue::setValue(const real v) { value = v; }


// get converted value to specified type
const Color4 &ConstRealValue::getConvertedColor() const
{
  static Color4 cTemp(0, 0, 0, 0);
  cTemp.r = value;
  cTemp.g = value;
  cTemp.b = value;
  cTemp.a = value;
  return cTemp;
}


const real ConstRealValue::getConvertedReal() const { return value; }


// check convertion - if fail, report error & return false
bool ConstRealValue::canConvert(shexpr::ValueType vt) const { return true; }

// evaluate this expression as real
bool ConstRealValue::evaluate(real &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  out_value = getConvertedReal();

  return true;
}

// evaluate this expression as color4
bool ConstRealValue::evaluate(Color4 &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  out_value = getConvertedColor();

  return true;
}


// dump to debug
void ConstRealValue::dump_internal(int level, const char *tabs) const
{
  debug("%s<ConstRealValue>", tabs);
  Expression::dump_internal(level, tabs);

  debug("%svalue=%.4f", tabs, value);

  debug("%s</ConstRealValue>", tabs);
}

// class ConstRealValue
//


/*********************************
 *
 * class ConstColor4Value
 *
 *********************************/
// ctor/dtor
ConstColor4Value::ConstColor4Value(Terminal *s, const Color4 &v) : t(s), value(v) {}


ConstColor4Value::~ConstColor4Value() {}


// assembly this expression
bool ConstColor4Value::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  Expression::assemblyConstant(code, value, int(dest_reg));
  return true;
}


// set associated value
void ConstColor4Value::setValue(const Color4 &v) { value = v; }


// get converted value to specified type
const Color4 &ConstColor4Value::getConvertedColor() const { return value; }


const real ConstColor4Value::getConvertedReal() const { return value.r; }


// check convertion - if fail, report error & return false
bool ConstColor4Value::canConvert(shexpr::ValueType vt) const
{
  if (vt == getValueType())
    return true;

  // color to real - never accepted
  return false;
}

// evaluate this expression as real
bool ConstColor4Value::evaluate(real &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  if (!canConvert(shexpr::VT_REAL))
  {
    ExpressionParser::getStatic().error(String(128, "cannot convert from '%s' to '%s' here", Expression::__getName(getValueType()),
                                          Expression::__getName(shexpr::VT_REAL)),
      t);
    return false;
  }

  return false;
}

// evaluate this expression as color4
bool ConstColor4Value::evaluate(Color4 &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  if (!canConvert(shexpr::VT_COLOR4))
  {
    ExpressionParser::getStatic().error(String(128, "cannot convert from '%s' to '%s' here", Expression::__getName(getValueType()),
                                          Expression::__getName(shexpr::VT_COLOR4)),
      t);
    return false;
  }

  out_value = getConvertedColor();

  return true;
}


// dump to debug
void ConstColor4Value::dump_internal(int level, const char *tabs) const
{
  debug("%s<ConstColor4Value>", tabs);
  Expression::dump_internal(level, tabs);

  debug("%svalue=%.4f %.4f %.4f %.4f", tabs, value.r, value.g, value.b, value.a);

  debug("%s</ConstColor4Value>", tabs);
}

// class ConstColor4Value
//


/*********************************
 *
 * class ColorValueExpression
 *
 *********************************/
// ctor/dtor
ColorValueExpression::ColorValueExpression(Terminal *s) : ComplexExpression(shexpr::VT_COLOR4), t(s) { resizeOperands(4); }

// evaluate this expression as real
bool ColorValueExpression::evaluate(real &out_value)
{
  // cannot convert this to real
  return false;
}


// evaluate this expression as color4
bool ColorValueExpression::evaluate(Color4 &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  G_ASSERT(getOperandCount() == 4);
  memset(&out_value, 0, sizeof(out_value));

  // try to evaluate
  G_ASSERT(getOperand(shexpr::OperandType(0)));
  G_ASSERT(getOperand(shexpr::OperandType(1)));
  G_ASSERT(getOperand(shexpr::OperandType(2)));
  G_ASSERT(getOperand(shexpr::OperandType(3)));

  if (!getOperand(shexpr::OperandType(0))->evaluate(out_value.r))
    return false;
  if (!getOperand(shexpr::OperandType(1))->evaluate(out_value.g))
    return false;
  if (!getOperand(shexpr::OperandType(2))->evaluate(out_value.b))
    return false;
  if (!getOperand(shexpr::OperandType(3))->evaluate(out_value.a))
    return false;

  if (check_nan(out_value.r))
  {
    ExpressionParser::getStatic().error("error in argument #1 - value is NAN!", t);
    return false;
  }

  if (check_nan(out_value.g))
  {
    ExpressionParser::getStatic().error("error in argument #2 - value is NAN!", t);
    return false;
  }

  if (check_nan(out_value.b))
  {
    ExpressionParser::getStatic().error("error in argument #3 - value is NAN!", t);
    return false;
  }

  if (check_nan(out_value.a))
  {
    ExpressionParser::getStatic().error("error in argument #4 - value is NAN!", t);
    return false;
  }

  return Expression::evaluate(out_value);
}


// dump to debug
void ColorValueExpression::dump_internal(int level, const char *tabs) const
{
  debug("%s<ColorValueExpression>", tabs);
  ComplexExpression::dump_internal(level, tabs);

  debug("%s</ColorValueExpression>", tabs);
}


// assembly this expression
bool ColorValueExpression::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  // assembly operands
  eastl::vector<Register> opRegs;
  opRegs.reserve(getOperandCount());
  for (int i = 0; i < getOperandCount(); i++)
  {
    opRegs.emplace_back(owner.add_reg());
    if (!getOperand(shexpr::OperandType(i))->assembly(owner, code, opRegs[i], is_integer))
      return false;
  }

  // make vector
  code.push_back(shaderopcode::makeOp3(SHCOD_MAKE_VEC, int(dest_reg), int(opRegs[0]), int(opRegs[1])));
  code.push_back(shaderopcode::makeData2(int(opRegs[2]), int(opRegs[3])));

  return true;
}


// evaluate all avalible branches; return false if error occuried
bool ColorValueExpression::collapseNumbers()
{
  for (int i = 0; i < getOperandCount(); i++)
  {
    // if operator is constant, calculate it now
    Expression *op = getOperand(shexpr::OperandType(i));
    if (op->isConst())
    {
      real v;
      // calc real
      if (op->evaluate(v))
      {
        // replace with constant real value
        Terminal *t = op->getTerminal();
        delete op;
        setOperand(shexpr::OperandType(i), new ConstRealValue(t, v));
      }
    }
    else
    {
      if (!op->collapseNumbers())
        return false;
    }
  }

  return true;
}

// class ColorValueExpression
//


/*********************************
 *
 * class LVarValueExpression
 *
 *********************************/
// ctor/dtor
LVarValueExpression::LVarValueExpression(Terminal *s, int _var, LocalVarTable &_vars) : t(s), var(_var), vars(_vars)
{
  LocalVar *locVar = vars.getVariableById(var);
  G_ASSERT(locVar);
  G_ASSERT(!locVar->isConst);
}


LVarValueExpression::~LVarValueExpression() {}


// assembly this expression
bool LVarValueExpression::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  if (var < 0)
  {
    owner.error("local variable not found!", t);
    return false;
  }

  LocalVar *locVar = vars.getVariableById(var);
  G_ASSERT(locVar);

  // add reference to local variable
  int cod;
  if (locVar->valueType == shexpr::VT_REAL)
    cod = SHCOD_COPY_REAL;
  else if (locVar->valueType == shexpr::VT_COLOR4)
    cod = SHCOD_COPY_VEC;
  else
    G_ASSERT(0);

  if (getUnaryOperator() == shexpr::UOP_NEGATIVE)
    code.push_back(shaderopcode::makeOp2(cod, int(dest_reg), locVar->reg));
  else
    dest_reg.reset(locVar->reg);

  return Expression::assembly(owner, code, dest_reg, is_integer);
}


// check convertion - if fail, report error & return false
bool LVarValueExpression::canConvert(shexpr::ValueType vt) const
{
  LocalVar *locVar = vars.getVariableById(var);
  G_ASSERT(locVar);
  if (locVar->valueType != shexpr::VT_REAL)
    return vt == locVar->valueType;

  if (vt == shexpr::VT_COLOR4)
  {
    Expression *p = getParent();
    if (p->getType() == shexpr::E_COMPLEX)
    {
      ComplexExpression *cp = static_cast<ComplexExpression *>(p);
      shexpr::BinaryOperator binOp = cp->getBinOperator(this);
      if ((cp->getOperand(shexpr::OPER_LEFT) != this) && ((binOp == shexpr::OP_MUL) || (binOp == shexpr::OP_DIV)))
        return true;
    }
    return false;
  }
  return (vt == locVar->valueType);
  //  return locVar->val.e ? locVar->val.e->canConvert(vt) : false;
}


// evaluate this expression as real
bool LVarValueExpression::evaluate(real &out_value)
{
  // only static or dynamic expressions are allowed in this variables
  return false;

  //  G_ASSERT(locVar->val.e);
  //  return locVar->val.e->evaluate(out_value);
}


// evaluate this expression as color4
bool LVarValueExpression::evaluate(Color4 &out_value)
{
  // only static or dynamic expressions are allowed in this variables
  return false;

  //  G_ASSERT(locVar->val.e);
  //  return locVar->val.e->evaluate(out_value);
}

// dump to debug
void LVarValueExpression::dump_internal(int level, const char *tabs) const
{
  debug("%s<LVarValueExpression>", tabs);
  Expression::dump_internal(level, tabs);

  debug("%s</LVarValueExpression>", tabs);
}

// return value type
shexpr::ValueType LVarValueExpression::getValueType() const
{
  LocalVar *locVar = vars.getVariableById(var);
  G_ASSERT(locVar);
  return locVar->valueType;
}


// return true, if expression has only numeric constants and constant local variables
bool LVarValueExpression::isConst() const
{
  LocalVar *locVar = vars.getVariableById(var);
  G_ASSERT(locVar);
  return locVar->isConst;
}

// return true, if expression - is a dynamic expression
bool LVarValueExpression::isDynamic() const
{
  LocalVar *locVar = vars.getVariableById(var);
  G_ASSERT(locVar);
  return locVar->isDynamic;
}

// class LVarValueExpression
//


/*********************************
 *
 * class StVarValueExpression
 *
 *********************************/
// ctor/dtor
StVarValueExpression::StVarValueExpression(Terminal *s, int var_id, shexpr::ValueType vt, bool is_dynamic, bool is_global,
  bool is_int) :
  t(s), varId(var_id), valueType(vt), isDynamicFlag(is_dynamic), isGlobalFlag(is_global), isInteger(is_int)
{
  if (isGlobalFlag)
  {
    G_ASSERT(isDynamicFlag == true);
  }
}


StVarValueExpression::~StVarValueExpression() {}


// assembly this expression
bool StVarValueExpression::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  int cod;
  if (isGlobalFlag)
  {
    // add reference to global variable
    if (valueType == shexpr::VT_REAL)
    {
      if (is_integer)
        cod = SHCOD_GET_GINT;
      else
        cod = isInteger ? SHCOD_GET_GINT_TOREAL : SHCOD_GET_GREAL;
    }
    else if (valueType == shexpr::VT_COLOR4)
      cod = SHCOD_GET_GVEC;
    else if (valueType == shexpr::VT_TEXTURE)
      cod = SHCOD_GET_GTEX;
    else
      G_ASSERT(false);
  }
  else
  {
    // add reference to local variable
    if (valueType == shexpr::VT_REAL)
    {
      if (is_integer)
        cod = SHCOD_GET_INT;
      else
        cod = isInteger ? SHCOD_GET_INT_TOREAL : SHCOD_GET_REAL;
    }
    else if (valueType == shexpr::VT_COLOR4)
      cod = SHCOD_GET_VEC;
    else
      G_ASSERT(false);
  }

  auto [it, inserted] = owner.stVarToReg.emplace(eastl::make_pair(varId, cod), dest_reg);
  if (!inserted)
  {
    dest_reg = it->second;
  }
  else
  {
    code.push_back(shaderopcode::makeOp2(cod, int(dest_reg), varId));
  }

  if (getUnaryOperator() == shexpr::UOP_NEGATIVE)
  {
    if (getValueType() == shexpr::VT_REAL)
      cod = SHCOD_COPY_REAL;
    else if (getValueType() == shexpr::VT_COLOR4)
      cod = SHCOD_COPY_VEC;
    else
      G_ASSERT(0);

    Register copy_reg = addReg(owner);
    code.push_back(shaderopcode::makeOp2(cod, int(copy_reg), int(dest_reg)));
    dest_reg = eastl::move(copy_reg);
  }
  return Expression::assembly(owner, code, dest_reg, is_integer);
}


// check convertion - if fail, report error & return false
bool StVarValueExpression::canConvert(shexpr::ValueType vt) const
{
  if (valueType != shexpr::VT_REAL)
    return vt == valueType;

  // real to color - accept only if used in multiply or div on the right side
  if (vt == shexpr::VT_COLOR4)
  {
    Expression *p = getParent();
    if (p->getType() == shexpr::E_COMPLEX)
    {
      ComplexExpression *cp = static_cast<ComplexExpression *>(p);
      shexpr::BinaryOperator binOp = cp->getBinOperator(this);
      if ((cp->getOperand(shexpr::OPER_LEFT) != this) && ((binOp == shexpr::OP_MUL) || (binOp == shexpr::OP_DIV)))
        return true;
    }
    return false;
  }

  return getValueType() == vt;
}


// evaluate this expression as real
bool StVarValueExpression::evaluate(real &out_value) { return false; }


// evaluate this expression as color4
bool StVarValueExpression::evaluate(Color4 &out_value) { return false; }

// dump to debug
void StVarValueExpression::dump_internal(int level, const char *tabs) const
{
  debug("%s<StVarValueExpression>", tabs);
  Expression::dump_internal(level, tabs);

  debug("%s</StVarValueExpression>", tabs);
}

// class StVarValueExpression
//


/*********************************
 *
 * class FunctionExpression
 *
 *********************************/
// ctor/dtor
FunctionExpression::FunctionExpression(Terminal *s, int function_id) :
  ComplexExpression(functional::getValueType(functional::FunctionId(function_id))), t(s), func(function_id)
{
  resizeOperands(functional::getOpCount(functional::FunctionId(func)));
}

// evaluate this expression as real
bool FunctionExpression::evaluate(real &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  if (!canConvert(shexpr::VT_REAL))
  {
    ExpressionParser::getStatic().error(String(128, "cannot convert from '%s' to '%s' here", Expression::__getName(getValueType()),
                                          Expression::__getName(shexpr::VT_REAL)),
      t);
    return false;
  }

  // check all operands
  int i;
  for (i = 0; i < getOperandCount(); i++)
  {
    G_ASSERT(getOperand(shexpr::OperandType(i)));
  }

  // try to evaluate
  functional::ArgList inArgs(tmpmem);
  functional::prepareArgs(functional::FunctionId(func), inArgs);

  for (i = 0; i < getOperandCount(); i++)
  {
    if (inArgs[i].vt == shexpr::VT_COLOR4)
    {
      if (!getOperand(shexpr::OperandType(i))->evaluate(inArgs[i].val.c4()))
        return false;
    }
    else
    {
      if (!getOperand(shexpr::OperandType(i))->evaluate(inArgs[i].val.r))
        return false;
    }
  }

  Color4 res;
  if (!functional::evaluate(functional::FunctionId(func), res, inArgs))
    return false;
  out_value = res.r;

  return Expression::evaluate(out_value);
}


// evaluate this expression as color4
bool FunctionExpression::evaluate(Color4 &out_value)
{
  // cannot evaluate dynamic expressions
  if (isDynamic())
    return false;

  if (!canConvert(shexpr::VT_COLOR4))
  {
    ExpressionParser::getStatic().error(String(128, "cannot convert from '%s' to '%s' here", Expression::__getName(getValueType()),
                                          Expression::__getName(shexpr::VT_COLOR4)),
      t);
    return false;
  }

  // check all operands
  int i;
  for (i = 0; i < getOperandCount(); i++)
  {
    G_ASSERT(getOperand(shexpr::OperandType(i)));
  }

  // try to evaluate
  functional::ArgList inArgs(tmpmem);
  functional::prepareArgs(functional::FunctionId(func), inArgs);

  for (i = 0; i < getOperandCount(); i++)
  {
    if (inArgs[i].vt == shexpr::VT_COLOR4)
    {
      if (!getOperand(shexpr::OperandType(i))->evaluate(inArgs[i].val.c4()))
        return false;
    }
    else
    {
      if (!getOperand(shexpr::OperandType(i))->evaluate(inArgs[i].val.r))
        return false;
    }
  }

  if (!functional::evaluate(functional::FunctionId(func), out_value, inArgs))
    return false;

  return Expression::evaluate(out_value);
}


// dump to debug
void FunctionExpression::dump_internal(int level, const char *tabs) const
{
  debug("%s<FunctionExpression '%s'>", tabs, functional::getFuncName(functional::FunctionId(func)));
  ComplexExpression::dump_internal(level, tabs);

  debug("%s</FunctionExpression>", tabs);
}


// assembly this expression
bool FunctionExpression::assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer)
{
  // assembly operands
  eastl::vector<Register> opRegs;
  opRegs.reserve(getOperandCount());
  for (int i = 0; i < getOperandCount(); i++)
  {
    opRegs.emplace_back(addReg(owner));
    if (!getOperand(shexpr::OperandType(i))->assembly(owner, code, opRegs[i], is_integer))
      return false;
  }

  // make code
  code.push_back(shaderopcode::makeOp3(SHCOD_CALL_FUNCTION, func, int(dest_reg), opRegs.size()));
  for (int i = 0; i < getOperandCount(); i++)
    code.push_back(int(opRegs[i]));

  return true;
}


// evaluate all avalible branches; return false if error occuried
bool FunctionExpression::collapseNumbers()
{
  functional::ArgList args(tmpmem);
  functional::prepareArgs(functional::FunctionId(func), args);

  for (int i = 0; i < getOperandCount(); i++)
  {
    // if operator is constant, calculate it now
    Expression *op = getOperand(shexpr::OperandType(i));
    if (op->isConst())
    {
      Color4 v;
      if (args[i].vt == shexpr::VT_COLOR4)
      {
        if (op->getType() != shexpr::E_CONST_COLOR4)
        {
          // calc color4
          if (op->evaluate(v))
          {
            // replace with constant color4 value
            Terminal *t = op->getTerminal();
            delete op;
            setOperand(shexpr::OperandType(i), new ConstColor4Value(t, v));
          }
        }
      }
      else
      {
        // calc real
        if (op->evaluate(v.r))
        {
          // replace with constant real value
          Terminal *t = op->getTerminal();
          delete op;
          setOperand(shexpr::OperandType(i), new ConstRealValue(t, v.r));
        }
      }
    }
    else
    {
      if (!op->collapseNumbers())
        return false;
    }
  }
  return true;
}


bool FunctionExpression::isConst() const
{
  if (functional::isAlwaysDynamic(functional::FunctionId(func)))
    return false;
  return ComplexExpression::isConst();
}

bool FunctionExpression::isDynamic() const
{
  if (functional::isAlwaysDynamic(functional::FunctionId(func)))
    return true;
  return ComplexExpression::isDynamic();
}


// check convertion - if fail, report error & return false
bool FunctionExpression::canConvert(shexpr::ValueType vt) const
{
  // check by default
  if (getValueType() == vt)
    return true;

  // real to color - accept only if used in multiply or div on the right side
  if (vt == shexpr::VT_COLOR4)
  {
    Expression *p = getParent();
    if (p->getType() == shexpr::E_COMPLEX)
    {
      ComplexExpression *cp = static_cast<ComplexExpression *>(p);
      shexpr::BinaryOperator binOp = cp->getBinOperator(this);
      if ((cp->getOperand(shexpr::OPER_LEFT) != this) && ((binOp == shexpr::OP_MUL) || (binOp == shexpr::OP_DIV)))
      {
        return true;
      }
    }
  }

  return false;
}

// return value type
shexpr::ValueType FunctionExpression::getValueType() const { return functional::getValueType(functional::FunctionId(func)); }

// class FunctionExpression
//
