#ifndef _SQ_BINARY_AST_H_
#define _SQ_BINARY_AST_H_ 1

#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <setjmp.h>

#include "sqastio.h"
#include "squirrel.h"
#include "sqast.h"
#include "arena.h"

#define OP_DELTA 1
#define VERSION 1

#define MAGIC "QAST"

using namespace SQCompilation;

class SQASTWriter {

  OutputStream *stream;
  Arena arena;
  ArenaVector<const SQChar *> stringIndexes;

  void writeMagic();
  void writeOptions();
  size_t writeStringTable();
  size_t writeSingleString(const SQChar *s);

public:

  SQASTWriter(SQAllocContext alloc_ctx, OutputStream *s);

  size_t writeAST(RootBlock *root, const SQChar *sourceName);
};

struct SQVM;

class SQASTReader {

  Arena *astArena;
  InputStream *stream;

  jmp_buf _errorjmp; //-V730_NOINIT

  SQVM *vm;
  bool raiseError;

  template<typename N, typename ... Args>
  N *newNode(Args... args) {
    return new (astArena) N(args...);
  }

  int32_t op_delta;

  Arena arena;
  ArenaVector<const SQChar *> stringVector;

  void readSingleString();
  void readStringTable();

  const SQChar *getString(unsigned idx) {
    assert(idx < stringVector.size());
    return stringVector[idx];
  }

  bool checkMagic();

  const SQChar *readString();

  Node *readNullable();

  void readTableBody(TableDecl *table);
  void readDeclGroupBody(DeclGroup *g);

  Node *readNode(enum TreeOp op);

  Block *readBlock(bool isRoot);

  IfStatement *readIfStatement();
  WhileStatement *readWhileStatement();
  DoWhileStatement *readDoWhileStatement();
  ForStatement *readForStatement();
  ForeachStatement *readForeachStatement();
  SwitchStatement *readSwitchStatement();

  ReturnStatement *readReturnStatement();
  YieldStatement *readYieldStatement();
  ThrowStatement *readThrowStatement();

  TryStatement *readTryStatement();

  BreakStatement *readBreakStatement();
  ContinueStatement *readContinueStatement();

  ExprStatement *readExprStatement();

  EmptyStatement *readEmptyStatement();

  Id *readId();
  CommaExpr *readCommaExpr();

  UnExpr *readUnaryExpr(enum TreeOp op);
  BinExpr *readBinExpr(enum TreeOp op);
  TerExpr *readTernaryExpr();

  LiteralExpr *readLiteral();

  RootExpr *readRootExpr();
  BaseExpr *readBaseExpr();

  IncExpr *readIncExpr();

  DeclExpr *readDeclExpr();

  ArrayExpr *readArrayExpr();

  CallExpr *readCallExpr();

  GetFieldExpr *readGetFieldExpr();
  GetTableExpr *readGetTableExpr();

  ValueDecl *readValueDecl(bool);
  ConstDecl *readConstDecl();

  EnumDecl *readEnumDecl();
  ClassDecl *readClassDecl();
  FunctionDecl *readFunctionDecl(bool);

  DeclGroup *readDeclGroup();
  DestructuringDecl *readDestructuringDecl();

  TableDecl *readTableDecl();

  DirectiveStmt *readDirectiveStmt();

  RootBlock *readRoot();

  Statement *readStatement();
  Statement *readNullableStatement();

  Expr *readExpression();
  Expr *readNullableExpression();

  Decl *readDeclaration();
  Decl *readNullableDeclaration();

  LiteralExpr *readNonNullLiteral();
  VarDecl *readNonNullVar();

  void error(const char *fmt, ...);
public:

  SQASTReader(SQAllocContext alloc_ctx, SQVM *v, Arena *astA, InputStream *s, bool raiseerr)
    : arena(alloc_ctx, "AST Read")
    , vm(v)
    , stringVector(&arena)
    , astArena(astA)
    , stream(s)
    , op_delta(0)
    , raiseError(raiseerr) {
  }

  RootBlock *readAst(const SQChar *&sourceName);
};

#endif // _SQ_BINARY_AST_H_
