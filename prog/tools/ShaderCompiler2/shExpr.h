/************************************************************************
  support for shader arithemtic expressions
/************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __SHEXPR_H
#define __SHEXPR_H

#include "shsyn.h"
#include <EASTL/unique_ptr.h>
#include <generic/dag_tab.h>
#include <math/dag_color.h>
#include <shaders/shExprTypes.h>
#include <util/dag_globDef.h>

class LocalVarTable;

namespace ShaderParser
{
//************************************************************************
//* forwards
//************************************************************************
class AssembleShaderEvalCB;

typedef Tab<int> CodeTable;

/*********************************
 *
 * class Expression
 *
 *********************************/
class Expression
{
public:
  // ctor/dtor
  Expression() : unaryOp(shexpr::UOP_POSITIVE), parent(NULL) {}
  virtual ~Expression() {}

  // return expression type
  virtual shexpr::Type getType() const = 0;

  // return value type
  virtual shexpr::ValueType getValueType() const = 0;

  // assembly this expression
  virtual bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, class Register &dest_reg, bool is_integer) = 0;

  // check convertion
  virtual bool canConvert(shexpr::ValueType vt) const = 0;

  // get/set operator
  inline void setUnaryOperator(shexpr::UnaryOperator t) { unaryOp = t; }
  inline shexpr::UnaryOperator getUnaryOperator() const { return unaryOp; }

  // get/set parent
  inline Expression *getParent() const { return parent; }
  inline void SetParent(Expression *p) { parent = p; }

  // return true, if expression has only numeric constants and local variables
  virtual bool isConst() const = 0;

  // return true, if expression - is a dynamic expression
  virtual bool isDynamic() const = 0;

  // evaluate this expression as real
  virtual bool evaluate(real &out_value) = 0;

  // evaluate this expression as color4
  virtual bool evaluate(Color4 &out_value) = 0;

  // evaluate all avalible branches; return false if error occuried
  virtual bool collapseNumbers() = 0;

  // return terminal symbol, if present
  virtual Terminal *getTerminal() const { return NULL; }

  virtual int getChannels() const { return 1; }

  // add register
  Register addReg(AssembleShaderEvalCB &owner);

  //************************************************************************
  //* helper functions
  //************************************************************************
  // return string name
  static const char *__getName(shexpr::ValueType vt);
  static const char *__getName(shexpr::BinaryOperator op);
  static const char *__getName(shexpr::ColorChannel cc);

  // assembly numeric constant
  static void assemblyConstant(CodeTable &code, real v, int dest_reg, bool is_integer);
  static void assemblyConstant(CodeTable &code, const Color4 &v, int dest_reg);

  // dump to debug
  void dump(int level = 0) const;

protected:
  // dump to debug
  virtual void dump_internal(int level, const char *tabs) const = 0;

private:
  Expression *parent;
  shexpr::UnaryOperator unaryOp;
}; // class Expression
//

/*********************************
 *
 * class ComplexExpression
 *
 *********************************/
class ComplexExpression : public Expression
{
public:
  // ctor/dtor
  ComplexExpression(shexpr::ValueType vt, int channel = -1);
  ~ComplexExpression() override;

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_COMPLEX; }

  // return value type
  shexpr::ValueType getValueType() const override;

  // set operand
  void setOperand(shexpr::OperandType op, Expression *child);

  // add user operand
  void addOperand(Expression *child, shexpr::BinaryOperator t);

  // assembly this expression
  bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer) override;

  // check convertion
  bool canConvert(shexpr::ValueType vt) const override;

  // get/set operator
  void setBinOperator(shexpr::OperandType op, shexpr::BinaryOperator t);
  shexpr::BinaryOperator getBinOperator(shexpr::OperandType op) const;

  // return operator for specific expression. return UNDEFINED, if operator not found
  shexpr::BinaryOperator getBinOperator(const Expression *e) const;

  // return operand
  inline Expression *getOperand(shexpr::OperandType t)
  {
    if (t < 0)
      return NULL;
    return operands[t];
  }

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override;

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override;

  // evaluate this expression as real
  bool evaluate(real &out_value) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value) override;

  // set last binary operator (used in setOperand, if operand_type is USER)
  void setLastBinOp(shexpr::BinaryOperator bop) { lastBinOp = bop; }

  // return operand count
  inline int getOperandCount() const { return operands.size(); }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers() override;

  int getCurrentChannel() const { return currentChannel; }
  void setChannels(int count) { channels = eastl::max(channels, count); }
  int getChannels() const override;

protected:
  // resize operands
  void resizeOperands(int count);

  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  Tab<Expression *> operands;
  Tab<shexpr::BinaryOperator> binOp;
  shexpr::ValueType baseValueType;
  shexpr::BinaryOperator lastBinOp;
  const int currentChannel;
  int channels = 1;
}; // class ComplexExpression
//


/*********************************
 *
 * class ColorChannelExpression
 *
 *********************************/
class ColorChannelExpression : public Expression
{
public:
  // ctor/dtor
  ColorChannelExpression(Terminal *s, shexpr::ColorChannel cc);
  ~ColorChannelExpression() override;

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_COLOR_CHANNEL; }

  // return value type
  shexpr::ValueType getValueType() const override;

  // assembly this expression
  bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer) override;

  // check convertion - if fail, report error & return false
  bool canConvert(shexpr::ValueType vt) const override;

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override;

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override;

  // evaluate this expression as real
  bool evaluate(real &out_value) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value) override;

  // get/set color channel
  inline shexpr::ColorChannel getColorChannel() const { return colorChannel; }
  inline void setColorChannel(shexpr::ColorChannel c) { colorChannel = c; }

  // get/set child
  Expression *getChild() const { return child.get(); }
  void setChild(Expression *v) { child.reset(v); }

  // return terminal symbol, if present
  virtual Terminal *getTerminal() const { return t; }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers() override;

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  shexpr::ColorChannel colorChannel;
  eastl::unique_ptr<Expression> child;
  Terminal *t;
}; // class ColorChannelExpression
//


/*********************************
 *
 * class ConstRealValue
 *
 *********************************/
class ConstRealValue : public Expression
{
public:
  // ctor/dtor
  ConstRealValue(Terminal *s, real v = 0);
  ~ConstRealValue() override;

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_CONST_VALUE; }

  // return value type
  inline shexpr::ValueType getValueType() const override { return shexpr::VT_REAL; }

  // assembly this expression
  bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer) override;

  // get associated value
  inline real getReal() const { return value; }

  // set associated value
  void setValue(const real v);

  // get converted value to specified type
  const Color4 &getConvertedColor() const;
  const real getConvertedReal() const;

  // check convertion - if fail, report error & return false
  bool canConvert(shexpr::ValueType vt) const override;

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override { return true; }

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override { return false; }

  // evaluate this expression as real
  bool evaluate(real &out_value) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return t; }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers() override { return true; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  real value;
  Terminal *t;
}; // class ConstRealValue
//


/*********************************
 *
 * class ConstColor4Value
 *
 *********************************/
class ConstColor4Value : public Expression
{
public:
  // ctor/dtor
  ConstColor4Value(Terminal *s, const Color4 &v = Color4(0, 0, 0, 0));
  ~ConstColor4Value() override;

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_CONST_COLOR4; }

  // return value type
  inline shexpr::ValueType getValueType() const override { return shexpr::VT_COLOR4; }

  // assembly this expression
  bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer) override;

  // set associated value
  void setValue(const Color4 &v);

  // get converted value to specified type
  const Color4 &getConvertedColor() const;
  const real getConvertedReal() const;

  // check convertion - if fail, report error & return false
  bool canConvert(shexpr::ValueType vt) const override;

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override { return true; }

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override { return false; }

  // evaluate this expression as real
  bool evaluate(real &out_value) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return t; }
  int getChannels() const override { return 4; }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers() override { return true; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  Color4 value;
  Terminal *t;
}; // class ConstColor4Value
//


/*********************************
 *
 * class ColorValueExpression
 *
 *********************************/
class ColorValueExpression : public ComplexExpression
{
public:
  // ctor/dtor
  ColorValueExpression(Terminal *s);

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_COLOR_VALUE; }

  // return value type
  inline shexpr::ValueType getValueType() const override { return shexpr::VT_COLOR4; }

  // evaluate this expression as real
  bool evaluate(real &out_value) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return t; }

  // assembly this expression
  bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer) override;

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers() override;
  int getChannels() const override { return 4; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  Terminal *t;
}; // class ColorValueExpression
//


/*********************************
 *
 * class LVarValueExpression
 *
 *********************************/
class LVarValueExpression : public Expression
{
public:
  // ctor/dtor
  LVarValueExpression(Terminal *s, int var, LocalVarTable &vars);
  ~LVarValueExpression() override;

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_LVAR_VALUE; }

  // return value type
  shexpr::ValueType getValueType() const override;

  // assembly this expression
  bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer) override;

  // check convertion - if fail, report error & return false
  bool canConvert(shexpr::ValueType vt) const override;

  // return true, if expression has only numeric constants and constant local variables
  bool isConst() const override;

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override;

  // evaluate this expression as real
  bool evaluate(real &out_value) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return t; }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers() override { return true; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  int var;
  Terminal *t;
  LocalVarTable &vars;
}; // class LVarValueExpression
//

/*********************************
 *
 * class StVarValueExpression
 *
 *********************************/
class StVarValueExpression : public Expression
{
public:
  // ctor/dtor
  StVarValueExpression(Terminal *s, int var_id, shexpr::ValueType vt, bool is_dynamic, bool is_global, bool is_int);

  ~StVarValueExpression() override;

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_STVAR_VALUE; }

  // return value type
  inline shexpr::ValueType getValueType() const override { return valueType; }

  // assembly this expression
  bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer) override;

  // check convertion - if fail, report error & return false
  bool canConvert(shexpr::ValueType vt) const override;

  // return true, if expression has only numeric constants and constant local variables
  bool isConst() const override { return false; }

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override { return isDynamicFlag; }

  // evaluate this expression as real
  bool evaluate(real &out_value) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return t; }

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers() override { return true; }

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  int varId;
  shexpr::ValueType valueType;
  bool isDynamicFlag;
  bool isGlobalFlag;
  bool isInteger;
  Terminal *t;
}; // class StVarValueExpression
//

/*********************************
 *
 * class FunctionExpression
 *
 *********************************/
class FunctionExpression : public ComplexExpression
{
public:
  // ctor/dtor
  FunctionExpression(Terminal *s, int function_id);

  // return expression type
  inline shexpr::Type getType() const override { return shexpr::E_FUNCTION; }

  // return value type
  shexpr::ValueType getValueType() const override;

  // evaluate this expression as real
  bool evaluate(real &out_value) override;

  // evaluate this expression as color4
  bool evaluate(Color4 &out_value) override;

  // return terminal symbol, if present
  Terminal *getTerminal() const override { return t; }

  // assembly this expression
  bool assembly(AssembleShaderEvalCB &owner, CodeTable &code, Register &dest_reg, bool is_integer) override;

  // evaluate all avalible branches; return false if error occuried
  bool collapseNumbers() override;

  // check convertion - if fail, report error & return false
  bool canConvert(shexpr::ValueType vt) const override;

  // return true, if expression has only numeric constants and local variables
  bool isConst() const override;

  // return true, if expression - is a dynamic expression
  bool isDynamic() const override;

protected:
  // dump to debug
  void dump_internal(int level, const char *tabs) const override;

private:
  int func;
  Terminal *t;
}; // class FunctionExpression
//
} // namespace ShaderParser

#endif //__SHEXPR_H
