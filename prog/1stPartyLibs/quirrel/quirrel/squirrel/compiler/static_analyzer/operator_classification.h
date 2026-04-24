#pragma once

#include "compiler/ast.h"

namespace SQCompilation
{

inline bool isSuspiciousNeighborOfNullCoalescing(TreeOp op) {
  return (op == TO_3CMP || op == TO_ANDAND || op == TO_OROR || op == TO_IN || /*op == TO_NOTIN ||*/ op == TO_EQ || op == TO_NE || op == TO_LE ||
    op == TO_LT || op == TO_GT || op == TO_GE /*|| op == TO_NOT*/ || op == TO_BNOT || op == TO_AND || op == TO_OR ||
    op == TO_XOR || op == TO_DIV || op == TO_MOD || op == TO_INSTANCEOF || /*op == TO_QMARK ||*/ /*op == TO_NEG ||*/
    op == TO_ADD || op == TO_MUL || op == TO_SHL || op == TO_SHR || op == TO_USHR);
}

inline bool isSuspiciousTernaryConditionOp(TreeOp op) {
  return op == TO_ADD || op == TO_SUB || op == TO_MUL || op == TO_DIV || op == TO_MOD ||
    op == TO_AND || op == TO_OR || op == TO_SHL || op == TO_SHR || op == TO_USHR || op == TO_3CMP || op == TO_NULLC;
}

inline bool isSuspiciousSameOperandsBinaryOp(TreeOp op) {
  return op == TO_EQ || op == TO_LE || op == TO_LT || op == TO_GE || op == TO_GT || op == TO_NE ||
    op == TO_ANDAND || op == TO_OROR || op == TO_SUB || op == TO_3CMP || op == TO_DIV || op == TO_MOD ||
    op == TO_OR || op == TO_AND || op == TO_XOR || op == TO_SHL || op == TO_SHR || op == TO_USHR;
}

inline bool isBlockTerminatorStatement(TreeOp op) {
  return op == TO_RETURN || op == TO_THROW || op == TO_BREAK || op == TO_CONTINUE;
}

inline bool isBooleanBinaryResultOperator(TreeOp op) {
  return op == TO_NE || op == TO_EQ || (TO_GE <= op && op <= TO_IN);
}

inline bool isBooleanResultOperator(TreeOp op) {
  return isBooleanBinaryResultOperator(op) || op == TO_NOT;
}

inline bool isArithOperator(TreeOp op) {
  return op == TO_OROR || op == TO_ANDAND
    || (TO_3CMP <= op && op <= TO_LT)
    || (TO_USHR <= op && op <= TO_SUB)
    || (TO_PLUSEQ <= op && op <= TO_MODEQ)
    || op == TO_BNOT || op == TO_NEG || op == TO_INC;
}

inline bool isDivOperator(TreeOp op) {
  return op == TO_DIV || op == TO_MOD || op == TO_DIVEQ || op == TO_MODEQ;
}

inline bool isPureArithOperator(TreeOp op) {
  return (TO_USHR <= op && op <= TO_SUB) || (TO_PLUSEQ <= op && op <= TO_MODEQ);
}

inline bool isRelationOperator(TreeOp op) {
  return TO_3CMP <= op && op <= TO_LT;
}

inline bool isBoolRelationOperator(TreeOp op) {
  return TO_GE <= op && op <= TO_LT;
}

inline bool isBitwiseOperator(TreeOp op) {
  return op == TO_OR || op == TO_AND || op == TO_XOR;
}

inline bool isBoolCompareOperator(TreeOp op) {
  return op == TO_NE || op == TO_EQ || isBoolRelationOperator(op);
}

inline bool isCompareOperator(TreeOp op) {
  return TO_NE <= op && op <= TO_LT;
}

inline bool isShiftOperator(TreeOp op) {
  return op == TO_SHL || op == TO_SHR || op == TO_USHR;
}

inline bool isHigherShiftPriority(TreeOp op) {
  return TO_MUL <= op && op <= TO_SUB;
}

inline bool isAssignOp(TreeOp op) {
  return op == TO_ASSIGN || (TO_PLUSEQ <= op && op <= TO_MODEQ);
}

}
