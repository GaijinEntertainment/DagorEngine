
#include <stdarg.h>
#include "sqbinaryast.h"
#include "sqstate.h"
#include "sqvm.h"

class SQASTWritingVisitor : public Visitor {

  OutputStream *stream;

  ArenaVector<const SQChar *> &stringIndexes;

  struct str_hash {
    size_t operator()(const SQChar *s) const {
      size_t r = 0;
      while (*s) {
        r *= 31;
        r += *s;
        ++s;
      }
      return r;
    }
  };

  struct eq_str
  {
    bool operator()(const SQChar *a, const SQChar *b) const
    {
      return std::strcmp(a, b) == 0;
    }
  };

  typedef ArenaUnorederMap<const SQChar *, unsigned, str_hash, eq_str> StringTable;
  StringTable stringTable;

  void writeNodeHeader(const Node *n);
  void writeNull();
  void writeExprVector(const ArenaVector<Expr *> &v);
  void writeNullable(Node *n);
public:

  uint32_t writeString(const SQChar *s);

  SQASTWritingVisitor(OutputStream *strm, Arena *arena, ArenaVector<const SQChar *> &sVec)
    : stream(strm)
    , stringIndexes(sVec)
    , stringTable(StringTable::Allocator(arena)) {

  }

  void visitBlock(Block *block) override;

  void visitIfStatement(IfStatement *ifStmt) override;

  void visitWhileStatement(WhileStatement *whileLoop) override;

  void visitDoWhileStatement(DoWhileStatement *doWhileLoop) override;

  void visitForStatement(ForStatement *forLoop) override;

  void visitForeachStatement(ForeachStatement *foreachLoop) override;

  void visitSwitchStatement(SwitchStatement *swtch) override;

  void visitTryStatement(TryStatement *tryStmt) override;

  void visitEmptyStatement(EmptyStatement *stmt) override;

  void visitJumpStatement(JumpStatement *stmt) override;

  void visitTerminateStatement(TerminateStatement *terminator) override;

  void visitReturnStatement(ReturnStatement *retStmt) override;

  void visitExprStatement(ExprStatement *stmt) override;

  void visitTableDecl(TableDecl *tableDecl) override;

  void visitClassDecl(ClassDecl *klass) override;

  void visitValueDecl(ValueDecl *decl) override;

  void visitVarDecl(VarDecl *var) override;

  void visitDeclGroup(DeclGroup *group) override;

  void visitDestructuringDecl(DestructuringDecl *destruct) override;

  void visitFunctionDecl(FunctionDecl *func) override;

  void visitConstDecl(ConstDecl *decl) override;

  void visitEnumDecl(EnumDecl *enums) override;

  void visitCallExpr(CallExpr *call) override;

  void visitBaseExpr(BaseExpr *base) override;

  void visitRootTableAccessExpr(RootTableAccessExpr *expr) override;

  void visitLiteralExpr(LiteralExpr *lit) override;

  void visitArrayExpr(ArrayExpr *expr) override;

  void visitUnExpr(UnExpr *unary) override;

  void visitGetFieldExpr(GetFieldExpr *expr) override;

  void visitGetSlotExpr(GetSlotExpr *expr) override;

  void visitBinExpr(BinExpr *expr) override;

  void visitTerExpr(TerExpr *expr) override;

  void visitIncExpr(IncExpr *expr) override;

  void visitId(Id *id) override;

  void visitCommaExpr(CommaExpr *expr) override;

  void visitDeclExpr(DeclExpr *expr) override;

  void visitDirectiveStatement(DirectiveStmt *dir) override;
};

void SQASTWritingVisitor::writeNodeHeader(const Node *n) {
  stream->writeInt32(n->op() + OP_DELTA);

  stream->writeInt32(n->lineStart());
  stream->writeInt32(n->columnStart());
  stream->writeInt32(n->lineEnd());
  stream->writeInt32(n->columnEnd());
}

void SQASTWritingVisitor::writeNull() {
  stream->writeUIntptr(0ULL);
}

void SQASTWritingVisitor::writeNullable(Node *n) {
  if (n)
    n->visit(this);
  else
    writeNull();
}

void SQASTWritingVisitor::writeExprVector(const ArenaVector<Expr *> &v) {
  stream->writeUInt64(v.size());

  for (auto e : v) {
    e->visit(this);
  }
}

uint32_t SQASTWritingVisitor::writeString(const SQChar *s) {
  auto it = stringTable.find(s);

  if (it != stringTable.end()) {
    stream->writeUInt32(it->second);
    return it->second;
  }

  unsigned idx = stringIndexes.size();
  stringIndexes.push_back(s);
  stringTable[s] = idx;

  stream->writeUInt32(idx);
  return idx;
}

void SQASTWritingVisitor::visitUnExpr(UnExpr *expr) {
  writeNodeHeader(expr);
  expr->argument()->visit(this);
}

void SQASTWritingVisitor::visitBinExpr(BinExpr *expr) {
  writeNodeHeader(expr);
  expr->lhs()->visit(this);
  expr->rhs()->visit(this);
}

void SQASTWritingVisitor::visitTerExpr(TerExpr *expr) {
  writeNodeHeader(expr);
  expr->a()->visit(this);
  expr->b()->visit(this);
  expr->c()->visit(this);
}

void SQASTWritingVisitor::visitBaseExpr(BaseExpr *expr) {
  writeNodeHeader(expr);
}

void SQASTWritingVisitor::visitRootTableAccessExpr(RootTableAccessExpr *expr) {
  writeNodeHeader(expr);
}

void SQASTWritingVisitor::visitId(Id *id) {
  writeNodeHeader(id);
  writeString(id->id());
}

void SQASTWritingVisitor::visitGetFieldExpr(GetFieldExpr *gf) {
  writeNodeHeader(gf);
  gf->receiver()->visit(this);
  stream->writeInt8(gf->isNullable());
  stream->writeInt8(gf->isTypeMethod());
  writeString(gf->fieldName());
}

void SQASTWritingVisitor::visitGetSlotExpr(GetSlotExpr *ge) {
  writeNodeHeader(ge);
  ge->receiver()->visit(this);
  stream->writeInt8(ge->isNullable());
  ge->key()->visit(this);
}

void SQASTWritingVisitor::visitIncExpr(IncExpr *expr) {
  writeNodeHeader(expr);
  stream->writeInt32(expr->form());
  stream->writeInt32(expr->diff());
  expr->argument()->visit(this);
}

void SQASTWritingVisitor::visitCommaExpr(CommaExpr *expr) {
  writeNodeHeader(expr);
  writeExprVector(expr->expressions());
}

void SQASTWritingVisitor::visitDeclExpr(DeclExpr *expr) {
  writeNodeHeader(expr);
  expr->declaration()->visit(this);
}

void SQASTWritingVisitor::visitArrayExpr(ArrayExpr *expr) {
  writeNodeHeader(expr);
  writeExprVector(expr->initializers());
}

void SQASTWritingVisitor::visitLiteralExpr(LiteralExpr *lit) {
  writeNodeHeader(lit);
  stream->writeInt32(lit->kind());

  switch (lit->kind())
  {
  case LK_STRING: writeString(lit->s()); break;
  case LK_BOOL: stream->writeInt8(lit->b()); break;
  case LK_FLOAT: stream->writeSQFloat(lit->f()); break;
  case LK_INT: stream->writeSQInteger(lit->i()); break;
  case LK_NULL: break;
  default: assert(0);
  }
}

void SQASTWritingVisitor::visitCallExpr(CallExpr *expr) {
  writeNodeHeader(expr);
  expr->callee()->visit(this);
  stream->writeInt8(expr->isNullable());
  writeExprVector(expr->arguments());
}

void SQASTWritingVisitor::visitExprStatement(ExprStatement *estmt) {
  writeNodeHeader(estmt);
  estmt->expression()->visit(this);
}

void SQASTWritingVisitor::visitDirectiveStatement(DirectiveStmt *stmt) {
  writeNodeHeader(stmt);
  stream->writeUInt32(stmt->setFlags);
  stream->writeUInt32(stmt->clearFlags);
  stream->writeInt8(stmt->applyToDefault);
}

void SQASTWritingVisitor::visitEnumDecl(EnumDecl *decl) {
  writeNodeHeader(decl);
  writeString(decl->name());
  stream->writeInt8(decl->isGlobal());
  stream->writeUInt64(decl->consts().size());

  for (auto &ec : decl->consts()) {
    writeString(ec.id);
    ec.val->visit(this);
  }
}

void SQASTWritingVisitor::visitConstDecl(ConstDecl *decl) {
  writeNodeHeader(decl);
  writeString(decl->name());
  decl->value()->visit(this);
  stream->writeInt8(decl->isGlobal());
}

void SQASTWritingVisitor::visitFunctionDecl(FunctionDecl *f) {
  writeNodeHeader(f);

  writeString(f->name());

  stream->writeUInt64(f->parameters().size());

  for (auto &p : f->parameters()) {
    p->visit(this);
  }

  f->body()->visit(this);
  stream->writeInt8(f->isVararg());
  stream->writeInt8(f->isLambda());

  writeString(f->sourceName());
}

void SQASTWritingVisitor::visitDeclGroup(DeclGroup *group) {
  writeNodeHeader(group);

  stream->writeUInt64(group->declarations().size());

  for (auto &d : group->declarations()) {
    d->visit(this);
  }
}

void SQASTWritingVisitor::visitDestructuringDecl(DestructuringDecl *destruct) {
  visitDeclGroup(destruct);
  stream->writeInt8(destruct->type());
  destruct->initExpression()->visit(this);
}

void SQASTWritingVisitor::visitValueDecl(ValueDecl *value) {
  writeNodeHeader(value);
  writeString(value->name());
  writeNullable(value->expression());
}

void SQASTWritingVisitor::visitVarDecl(VarDecl *var) {
  visitValueDecl(var);
  stream->writeInt8(var->isAssignable());
}

void SQASTWritingVisitor::visitTableDecl(TableDecl *table) {
  writeNodeHeader(table);

  stream->writeUInt64(table->members().size());

  for (auto &m : table->members()) {
    m.key->visit(this);
    m.value->visit(this);
    stream->writeUInt32(m.flags);
  }
}

void SQASTWritingVisitor::visitClassDecl(ClassDecl *klass) {
  visitTableDecl(klass);

  writeNullable(klass->classKey());
  writeNullable(klass->classBase());
}

void SQASTWritingVisitor::visitTerminateStatement(TerminateStatement *stmt) {
  writeNodeHeader(stmt);

  writeNullable(stmt->argument());
}

void SQASTWritingVisitor::visitReturnStatement(ReturnStatement *stmt) {
  visitTerminateStatement(stmt);
  stream->writeInt8(stmt->isLambdaReturn());
}

void SQASTWritingVisitor::visitJumpStatement(JumpStatement *stmt) {
  writeNodeHeader(stmt);
}

void SQASTWritingVisitor::visitEmptyStatement(EmptyStatement *stmt) {
  writeNodeHeader(stmt);
}

void SQASTWritingVisitor::visitTryStatement(TryStatement *stmt) {
  writeNodeHeader(stmt);

  stmt->tryStatement()->visit(this);
  stmt->exceptionId()->visit(this);
  stmt->catchStatement()->visit(this);
}

void SQASTWritingVisitor::visitSwitchStatement(SwitchStatement *swtch) {
  writeNodeHeader(swtch);

  swtch->expression()->visit(this);

  stream->writeUInt64(swtch->cases().size());

  for (auto &c : swtch->cases()) {
    c.val->visit(this);
    c.stmt->visit(this);
  }

  writeNullable(swtch->defaultCase().stmt);
}

void SQASTWritingVisitor::visitBlock(Block *b) {
  writeNodeHeader(b);

  stream->writeUInt64(b->statements().size());

  for (auto &s : b->statements()) {
    s->visit(this);
  }
}

void SQASTWritingVisitor::visitIfStatement(IfStatement *stmt) {
  writeNodeHeader(stmt);

  stmt->condition()->visit(this);
  stmt->thenBranch()->visit(this);

  writeNullable(stmt->elseBranch());
}

void SQASTWritingVisitor::visitWhileStatement(WhileStatement *stmt) {
  writeNodeHeader(stmt);

  stmt->condition()->visit(this);
  stmt->body()->visit(this);
}

void SQASTWritingVisitor::visitDoWhileStatement(DoWhileStatement *stmt) {
  writeNodeHeader(stmt);

  stmt->body()->visit(this);
  stmt->condition()->visit(this);
}

void SQASTWritingVisitor::visitForStatement(ForStatement *stmt) {
  writeNodeHeader(stmt);

  writeNullable(stmt->initializer());
  writeNullable(stmt->condition());
  writeNullable(stmt->modifier());

  stmt->body()->visit(this);
}

void SQASTWritingVisitor::visitForeachStatement(ForeachStatement *stmt) {
  writeNodeHeader(stmt);

  writeNullable(stmt->idx());
  writeNullable(stmt->val());

  stmt->container()->visit(this);
  stmt->body()->visit(this);
}

SQASTWriter::SQASTWriter(SQAllocContext alloc_ctx, OutputStream *s)
  : stream(s)
  , arena(alloc_ctx, "AST Writter")
  , stringIndexes(&arena) {

}

void SQASTWriter::writeMagic() {
  const char *m = MAGIC;

  stream->writeUInt8(m[0]);
  stream->writeUInt8(m[1]);
  stream->writeUInt8(m[2]);
  stream->writeUInt8(m[3]);
}

void SQASTWriter::writeOptions() {
  stream->writeInt8(VERSION);
  stream->writeInt8(OP_DELTA);
  stream->writeInt8(sizeof(SQInteger));
  stream->writeInt8(sizeof(SQFloat));
}

size_t SQASTWriter::writeSingleString(const SQChar *s) {
  size_t startOff = stream->pos();

  size_t stringLength = strlen(s);

  stream->writeUInt32(stringLength);

  for (size_t idx = 0; idx < stringLength; ++idx) {
    stream->writeInt8(s[idx]);
  }

  return stream->pos() - startOff;
}

size_t SQASTWriter::writeStringTable() {
  size_t startOff = stream->pos();

  stream->writeUInt32(stringIndexes.size());

  size_t ptr = stream->pos();

  for (auto *s : stringIndexes) {
    writeSingleString(s);
  }

  return stream->pos() - startOff;
}

size_t SQASTWriter::writeAST(RootBlock *root, const SQChar *sourcename) {
  stream->seek(0);

  writeMagic();

  assert(stream->pos() == sizeof(uint32_t));

  writeOptions();

  assert(stream->pos() == 2 * sizeof(uint32_t));

  size_t sourceNameOff = stream->pos();

  SQASTWritingVisitor writingVisitor(stream, &arena, stringIndexes);
  writingVisitor.writeString(sourcename);

  size_t stringTableOffsetOff = stream->pos();
  stream->writeRawUInt64(0);

  root->visit(&writingVisitor);

  size_t stringTableOff = stream->pos();

  stream->seek(stringTableOffsetOff);
  stream->writeRawUInt64(stringTableOff);

  stream->seek(stringTableOff);
  writeStringTable();

  return stream->pos();
}

// -----------------------------------------------------------------------

void SQASTReader::readSingleString() {
  size_t stringLength = stream->readUInt32();

  SQChar *buffer = (SQChar *)astArena->allocate((stringLength + 1) * sizeof(SQChar));

  for (size_t i = 0; i < stringLength; ++i) {
    buffer[i] = stream->readInt8();
  }

  buffer[stringLength] = '\0';

  stringVector.push_back(buffer);
}

void SQASTReader::readStringTable() {
  uint32_t stringTableSize = stream->readUInt32();

  stringVector.resize(stringTableSize);

  for (size_t i = 0; i < stringTableSize; ++i) {
    readSingleString();
  }
}

bool SQASTReader::checkMagic() {
  const char *m = MAGIC;

  uint8_t m0 = stream->readUInt8();
  uint8_t m1 = stream->readUInt8();
  uint8_t m2 = stream->readUInt8();
  uint8_t m3 = stream->readUInt8();

  return m0 == m[0] && m1 == m[1] && m2 == m[2] && m3 == m[3];
}

void SQASTReader::error(const char *fmt, ...) {
  if (raiseError && _ss(vm)->_compilererrorhandler) {
    char buffer[1024] = { 0 };
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, vl);
    va_end(vl);

    _ss(vm)->_compilererrorhandler(vm, SEV_ERROR, buffer, "BinaryAST", -1, -1, nullptr);
  }
  longjmp(_errorjmp, 1);
}

RootBlock *SQASTReader::readAst(const SQChar *&sourcename) {
  if (setjmp(_errorjmp) == 0) {
    stream->seek(0);

    const char *m = MAGIC;

    if (!checkMagic()) {
      error("Not a AST bionary data");
    }

    uint8_t version = stream->readInt8();
    op_delta = stream->readInt8();
    uint8_t sq_int_size = stream->readInt8();
    uint8_t sq_float_size = stream->readInt8();

    uint32_t sourceNameIdx = stream->readUInt32();

    uintptr_t stringTableOffset = stream->readRawUInt64();
    size_t dataOffset = stream->pos();

    stream->seek(stringTableOffset);
    readStringTable();
    stream->seek(dataOffset);

    sourcename = getString(sourceNameIdx);

    RootBlock *root = readRoot();

    assert(stream->pos() == stringTableOffset);

    return root;
  }
  else {
    return nullptr;
  }
}

Statement *SQASTReader::readStatement() {
  Node *n = readNullable();

  if (!n || !n->isStatementOrDeclaration()) {
    error("Expected non-null statement node");
  }

  return (Statement *)n;
}

Statement *SQASTReader::readNullableStatement() {
  Node *n = readNullable();

  if (n && !n->isStatementOrDeclaration()) {
    error("Expected statement node");
  }

  return (Statement *)n;
}

Expr *SQASTReader::readExpression() {
  Node *n = readNullable();

  if (!n || !n->isExpression()) {
    error("Expected non-null expression node");
  }

  return (Expr *)n;
}

Expr *SQASTReader::readNullableExpression() {
  Node *n = readNullable();

  if (n && !n->isExpression()) {
    error("Expected expression node");
  }

  return (Expr *)n;
}

Decl *SQASTReader::readDeclaration() {
  Node *n = readNullable();

  if (!n || !n->isDeclaration()) {
    error("Expected non-null declaration node");
  }

  return (Decl *)n;
}

Decl *SQASTReader::readNullableDeclaration() {
  Node *n = readNullable();

  if (n && !n->isDeclaration()) {
    error("Expected declaration node");
  }

  return (Decl *)n;
}

LiteralExpr *SQASTReader::readNonNullLiteral() {
  Node *n = readNullable();

  if (!n || n->op() != TO_LITERAL) {
    error("Expected non-null literal node");
  }

  return (LiteralExpr *)n;
}

VarDecl *SQASTReader::readNonNullVar() {
  Node *n = readNullable();

  if (!n || n->op() != TO_VAR) {
    error("Expected non-null var node");
  }

  return (VarDecl *)n;
}

RootBlock *SQASTReader::readRoot() {
  int32_t op = stream->readInt32();
  int32_t line = stream->readInt32();
  int32_t column = stream->readInt32();

  RootBlock *root = (RootBlock *)readBlock(true);
  assert(root->isRoot());

  return root;
}

Node *SQASTReader::readNullable() {
  int32_t op = stream->readInt32();
  if (op) {
    int32_t lineS = stream->readInt32();
    int32_t columnS = stream->readInt32();
    int32_t lineE = stream->readInt32();
    int32_t columnE = stream->readInt32();

    Node *n = readNode((enum TreeOp)(op - op_delta));

    n->setLineStartPos(lineS);
    n->setColumnStartPos(columnS);
    n->setLineEndPos(lineE);
    n->setColumnEndPos(columnE);

    return n;
  }
  else {
    return nullptr;
  }
}

Block *SQASTReader::readBlock(bool isRoot) {
  int32_t lineEnd = stream->readInt32();
  size_t size = stream->readUInt64();

  Block *b = isRoot? newNode<RootBlock>(astArena) : newNode<Block>(astArena, false);

  b->statements().resize(size);

  for (size_t i = 0; i < size; ++i) {
    b->addStatement(readStatement());
  }

  return b;
}

IfStatement *SQASTReader::readIfStatement() {
  Expr *cond = readExpression();

  Statement *thenB = readStatement();
  Statement *elseB = readNullableStatement();

  return newNode<IfStatement>(cond, thenB, elseB);
}

WhileStatement *SQASTReader::readWhileStatement() {
  Expr *condition = readExpression();
  Statement *body = readStatement();

  return newNode<WhileStatement>(condition, body);
}

DoWhileStatement *SQASTReader::readDoWhileStatement() {
  Statement *body = readStatement();
  Expr *condition = readExpression();

  return newNode<DoWhileStatement>(body, condition);
}

ForStatement *SQASTReader::readForStatement() {
  Node *init = readNullable();
  Expr *condition = readNullableExpression();
  Expr *modifier = readNullableExpression();

  Statement *body = readNullableStatement();

  return newNode<ForStatement>(init, condition, modifier, body);
}

ForeachStatement *SQASTReader::readForeachStatement() {
  Decl *idx = readNullableDeclaration();
  if (idx && idx->op() != TO_VAR) {
    error("At foreach 'idx' position should be Var node");
  }

  Decl *val = readNullableDeclaration();
  if (val && val->op() != TO_VAR) {
    error("At foreach 'val' position should be Var node");
  }

  Expr *container = readExpression();

  Statement *body = readStatement();

  return newNode<ForeachStatement>((VarDecl *)idx, (VarDecl *)val, container, body);
}

SwitchStatement *SQASTReader::readSwitchStatement() {
  Expr *expr = readExpression();

  SwitchStatement *swtch = newNode<SwitchStatement>(astArena, (Expr *)expr);

  size_t size = stream->readUInt64();

  swtch->cases().resize(size);

  for (size_t i = 0; i < size; ++i) {
    Expr *v = readExpression();
    Statement *s = readStatement();

    swtch->addCases(v, s);
  }

  swtch->addDefault(readNullableStatement());

  return swtch;
}

ReturnStatement *SQASTReader::readReturnStatement() {
  Expr *arg = readNullableExpression();
  bool isLambda = stream->readInt8();

  ReturnStatement *n = newNode<ReturnStatement>(arg);
  if (isLambda)
    n->setIsLambda();
  return n;
}

YieldStatement *SQASTReader::readYieldStatement() {
  Expr *arg = readNullableExpression();

  return newNode<YieldStatement>(arg);
}

ThrowStatement *SQASTReader::readThrowStatement() {
  Expr *arg = readNullableExpression();

  return newNode<ThrowStatement>(arg);
}

TryStatement *SQASTReader::readTryStatement() {
  Statement *tryStmt = readStatement();

  Node *exId = readNullable();
  if (!exId || exId->op() != TO_ID) {
    error("At try-catch 'ex' position expected non-null Id node");
  }

  Statement *catchStmt = readStatement();

  return newNode<TryStatement>(tryStmt, (Id *)exId, catchStmt);
}

BreakStatement *SQASTReader::readBreakStatement() {
  return newNode<BreakStatement>(nullptr);
}

ContinueStatement *SQASTReader::readContinueStatement() {
  return newNode<ContinueStatement>(nullptr);
}

EmptyStatement *SQASTReader::readEmptyStatement() {
  return newNode<EmptyStatement>();
}

ExprStatement *SQASTReader::readExprStatement() {
  Node *expr = readExpression();

  return newNode<ExprStatement>((Expr *)expr);
}

const SQChar *SQASTReader::readString() {
  unsigned idx = stream->readUInt32();

  return getString(idx);
}

Id *SQASTReader::readId() {
  const SQChar *s = readString();

  return newNode<Id>(s);
}

CommaExpr *SQASTReader::readCommaExpr() {
  size_t size = stream->readUInt64();
  CommaExpr *comma = newNode<CommaExpr>(astArena);

  comma->expressions().resize(size);

  for (size_t i = 0; i < size; ++i) {
    comma->addExpression(readExpression());
  }

  return comma;
}

UnExpr *SQASTReader::readUnaryExpr(enum TreeOp op) {
  Expr *arg = readExpression();

  return newNode<UnExpr>(op, arg);
}

BinExpr *SQASTReader::readBinExpr(enum TreeOp op) {
  Expr *lhs = readExpression();
  Expr *rhs = readExpression();

  return newNode<BinExpr>(op, lhs, rhs);
}

TerExpr *SQASTReader::readTernaryExpr() {
  Expr *a = readExpression();
  Expr *b = readExpression();
  Expr *c = readExpression();

  return newNode<TerExpr>(a, b, c);
}

BaseExpr *SQASTReader::readBaseExpr() {
  return newNode<BaseExpr>();
}

RootTableAccessExpr *SQASTReader::readRootTableAccessExpr() {
  return newNode<RootTableAccessExpr>();
}

IncExpr *SQASTReader::readIncExpr() {
  int32_t form = stream->readInt32();
  int32_t diff = stream->readInt32();
  Expr *arg = readExpression();

  return newNode<IncExpr>(arg, diff, (enum IncForm)form);
}

LiteralExpr *SQASTReader::readLiteral() {
  enum LiteralKind kind = (enum LiteralKind)stream->readInt32();

  switch (kind)
  {
  case LK_STRING: return newNode<LiteralExpr>(readString());
  case LK_INT: return newNode<LiteralExpr>(stream->readSQInteger());
  case LK_FLOAT: return newNode<LiteralExpr>(stream->readSQFloat());
  case LK_BOOL: return newNode<LiteralExpr>((bool)stream->readInt8());
  case LK_NULL: return newNode<LiteralExpr>();
  default:
    assert(0);
    return nullptr;
  }
}

DeclExpr *SQASTReader::readDeclExpr() {
  return newNode<DeclExpr>(readDeclaration());
}

ArrayExpr *SQASTReader::readArrayExpr() {
  size_t size = stream->readUInt64();

  ArrayExpr *e = newNode<ArrayExpr>(astArena);

  e->initializers().resize(size);

  for (size_t i = 0; i < size; ++i) {
    e->addValue(readExpression());
  }

  return e;
}

CallExpr *SQASTReader::readCallExpr() {
  Node *callee = readNullable();
  assert(callee && callee->isExpression());
  bool nullable = stream->readInt8();

  CallExpr *call = newNode<CallExpr>(astArena, (Expr *)callee, nullable);

  size_t size = stream->readUInt64();
  call->arguments().resize(size);

  for (size_t i = 0; i < size; ++i) {
    call->addArgument(readExpression());
  }

  return call;
}

GetFieldExpr *SQASTReader::readGetFieldExpr() {
  Expr *receiver = readExpression();

  bool nullable = (bool)stream->readInt8();
  bool isTypeMethod = (bool)stream->readInt8();
  const SQChar *s = readString();

  return newNode<GetFieldExpr>(receiver, s, nullable, isTypeMethod);
}

GetSlotExpr *SQASTReader::readGetSlotExpr() {
  Expr *receiver = readExpression();

  bool nullable = (bool)stream->readInt8();

  Expr *key = readExpression();

  return newNode<GetSlotExpr>(receiver, key, nullable);
}

ValueDecl *SQASTReader::readValueDecl(bool isVar) {
  const SQChar *name = readString();
  Expr *init = readNullableExpression();

  return isVar
    ? (ValueDecl *)newNode<VarDecl>(name, init, stream->readInt8())
    : (ValueDecl *)newNode<ParamDecl>(name, init);
}

ConstDecl *SQASTReader::readConstDecl() {
  const SQChar *s = readString();

  LiteralExpr *v = readNonNullLiteral();

  bool isGlobal = stream->readInt8();

  return newNode<ConstDecl>(s, v, isGlobal);
}

EnumDecl *SQASTReader::readEnumDecl() {
  const SQChar *name = readString();
  bool isGlobal = stream->readInt8();

  EnumDecl *e = newNode<EnumDecl>(astArena, name, isGlobal);

  size_t size = stream->readUInt64();
  e->consts().resize(size);

  for (size_t i = 0; i < size; ++i) {
    const SQChar *n = readString();
    LiteralExpr *v = readNonNullLiteral();
    e->addConst(n, v);
  }

  return e;
}


void SQASTReader::readTableBody(TableDecl *table) {
  size_t size = stream->readUIntptr();
  table->members().resize(size);

  for (size_t i = 0; i < size; ++i) {
    Expr *key = readExpression();
    Expr *value = readExpression();
    unsigned flags = stream->readUInt32();

    table->addMember(key, value, flags);
  }
}

TableDecl *SQASTReader::readTableDecl() {
  TableDecl *t = newNode<TableDecl>(astArena);
  readTableBody(t);

  return t;
}

ClassDecl *SQASTReader::readClassDecl() {
  ClassDecl *klass = newNode<ClassDecl>(astArena, nullptr, nullptr);

  readTableBody(klass);

  Expr *key = readNullableExpression();
  Expr *base = readNullableExpression();

  klass->setClassKey(key);
  klass->setClassBase(base);

  return klass;
}

void SQASTReader::readDeclGroupBody(DeclGroup *g) {
  size_t size = stream->readUIntptr();

  g->declarations().resize(size);

  for (size_t i = 0; i < size; ++i) {
    g->addDeclaration(readNonNullVar());
  }
}

DeclGroup *SQASTReader::readDeclGroup() {
  DeclGroup *g = newNode<DeclGroup>(astArena);

  readDeclGroupBody(g);

  return g;
}

DestructuringDecl *SQASTReader::readDestructuringDecl() {
  DestructuringDecl *g = newNode<DestructuringDecl>(astArena, DT_TABLE);

  readDeclGroupBody(g);

  enum DestructuringType type = (enum DestructuringType)stream->readInt8();

  g->setType(type);
  g->setExpression(readExpression());

  return g;
}

FunctionDecl *SQASTReader::readFunctionDecl(bool isCtor) {
  const SQChar *name = readString();

  FunctionDecl *f = isCtor ? newNode<ConstructorDecl>(astArena, name) : newNode<FunctionDecl>(astArena, name);

  size_t size = stream->readUInt64();
  f->parameters().resize(size);

  for (size_t i = 0; i < size; ++i) {
    Decl *p = readDeclaration();
    if (p->op() != TO_PARAM) {
      error("Expected PARAM node at function parameter position");
    }
    f->parameters().push_back((ParamDecl *)p);
  }

  Statement *nbody = readStatement();
  if (nbody->op() != TO_BLOCK) {
    error("Expected BLOCK node at function body position");
  }
  Block *body = (Block *)nbody;

  body->setIsBody(); //-V522

  f->setBody(body);

  f->setVararg((bool)stream->readInt8());
  f->setLambda((bool)stream->readInt8());

  f->setSourceName(readString());

  if (f->isVararg()) {
    f->parameters().back()->setVararg();
  }

  return f;
}

DirectiveStmt *SQASTReader::readDirectiveStmt() {
  DirectiveStmt *d = newNode<DirectiveStmt>();

  d->setFlags = stream->readUInt32();
  d->clearFlags = stream->readUInt32();
  d->applyToDefault = stream->readInt8();

  return d;
}

Node *SQASTReader::readNode(enum TreeOp op) {
  switch (op)
  {
  case TO_BLOCK: return readBlock(false);
  case TO_IF: return readIfStatement();
  case TO_WHILE: return readWhileStatement();
  case TO_DOWHILE: return readDoWhileStatement();
  case TO_FOR: return readForStatement();
  case TO_FOREACH: return readForeachStatement();
  case TO_SWITCH: return readSwitchStatement();
  case TO_RETURN: return readReturnStatement();
  case TO_YIELD: return readYieldStatement();
  case TO_THROW: return readThrowStatement();
  case TO_TRY: return readTryStatement();
  case TO_BREAK: return readBreakStatement();
  case TO_CONTINUE: return readContinueStatement();
  case TO_EXPR_STMT: return readExprStatement();
  case TO_EMPTY: return readEmptyStatement();
  // case TO_STATEMENT_MARK:
  case TO_ID: return readId();
  case TO_COMMA: return readCommaExpr();
  case TO_NULLC:
  case TO_ASSIGN:
  case TO_OROR:
  case TO_ANDAND:
  case TO_OR:
  case TO_XOR:
  case TO_AND:
  case TO_NE:
  case TO_EQ:
  case TO_3CMP:
  case TO_GE:
  case TO_GT:
  case TO_LE:
  case TO_LT:
  case TO_IN:
  case TO_INSTANCEOF:
  case TO_USHR:
  case TO_SHR:
  case TO_SHL:
  case TO_MUL:
  case TO_DIV:
  case TO_MOD:
  case TO_ADD:
  case TO_SUB:
  case TO_NEWSLOT:
  case TO_PLUSEQ:
  case TO_MINUSEQ:
  case TO_MULEQ:
  case TO_DIVEQ:
  case TO_MODEQ:
    return readBinExpr(op);
  case TO_NOT:
  case TO_BNOT:
  case TO_NEG:
  case TO_TYPEOF:
  case TO_RESUME:
  case TO_CLONE:
  case TO_PAREN:
  case TO_DELETE:
    return readUnaryExpr(op);
  case TO_LITERAL: return readLiteral();
  case TO_BASE: return readBaseExpr();
  case TO_ROOT_TABLE_ACCESS: return readRootTableAccessExpr();
  case TO_INC: return readIncExpr();
  case TO_DECL_EXPR: return readDeclExpr();
  case TO_ARRAYEXPR: return readArrayExpr();
  case TO_GETFIELD: return readGetFieldExpr();
  case TO_GETSLOT: return readGetSlotExpr();
  case TO_CALL: return readCallExpr();
  case TO_TERNARY: return readTernaryExpr();
  // case TO_EXPR_MARK:
  case TO_VAR: return readValueDecl(true);
  case TO_PARAM: return readValueDecl(false);
  case TO_CONST: return readConstDecl();
  case TO_DECL_GROUP: return readDeclGroup();
  case TO_DESTRUCTURE: return readDestructuringDecl();
  case TO_FUNCTION: return readFunctionDecl(false);
  case TO_CONSTRUCTOR: return readFunctionDecl(true);
  case TO_CLASS: return readClassDecl();
  case TO_ENUM: return readEnumDecl();
  case TO_TABLE: return readTableDecl();
  case TO_DIRECTIVE: return readDirectiveStmt();
  default:
    assert("Unknown Tree code");
    return NULL;
  }
}

// --------------------------------------------------------------

typedef union {
  uint64_t v;
  uint8_t b[sizeof(uint64_t)];
} U64Conv;

uint64_t InputStream::readVaruint() {
  uint64_t v = 0;
  uint8_t t = 0;

  uint8_t s = 0;

  do {
    t = readByte();
    v |= uint64_t(t & 0x7F) << s;
    s += 7;
  } while (t & 0x80);

  return v;
}

int64_t InputStream::readVarint() {
  union {
    int64_t i;
    uint64_t u;
  } conv;

  conv.u = readVaruint();

  return conv.i;
}

int8_t InputStream::readInt8() {

  union {
    int8_t i;
    uint8_t u;
  } conv;

  conv.u = readByte();
  return conv.i;
}

int16_t InputStream::readInt16() {
  return (int16_t)readVarint();
}

int32_t InputStream::readInt32() {
  return (int32_t)readVarint();
}

int64_t InputStream::readInt64() {
  return (int64_t)readVarint();
}

intptr_t InputStream::readIntptr() {
  return readInt64();
}

uint8_t InputStream::readUInt8() {
  return readByte();
}

uint16_t InputStream::readUInt16() {
  return (uint16_t)readVaruint();
}

uint32_t InputStream::readUInt32() {
  return (uint32_t)readVaruint();
}

uint64_t InputStream::readUInt64() {
  return readVaruint();
}

uintptr_t InputStream::readUIntptr() {
  return readUInt64();
}

uint64_t InputStream::readRawUInt64() {
  U64Conv conv;

  for (int i = 0; i < sizeof conv.b; ++i) {
    conv.b[i] = readByte();
  }

  return conv.v;
}

// ---------------------------------------------

void OutputStream::writeVaruint(uint64_t v) {

  while (v > 0x7F) {
    writeByte(0x80 | uint8_t(v & 0x7F));
    v >>= 7;
  }

  writeByte(uint8_t(v));
}

void OutputStream::writeVarint(int64_t v) {
  union {
    int64_t i;
    uint64_t u;
  } conv;

  conv.i = v;

  writeVaruint(conv.u);
}

void OutputStream::writeInt8(int8_t v) {
  union {
    uint8_t u;
    int8_t i;
  } conv;

  conv.i = v;

  writeByte(conv.u);
}

void OutputStream::writeInt16(int16_t v) {
  writeVarint(v);
}

void OutputStream::writeInt32(int32_t v) {
  writeVarint(v);
}

void OutputStream::writeInt64(int64_t v) {
  writeVarint(v);
}

void OutputStream::writeIntptr(intptr_t v) {
  writeInt64(v);
}

void OutputStream::writeUInt8(uint8_t v) {
  writeByte(v);
}

void OutputStream::writeUInt16(uint16_t v) {
  writeVaruint(v);
}

void OutputStream::writeUInt32(uint32_t v) {
  writeVaruint(v);
}

void OutputStream::writeUInt64(uint64_t v) {
  writeVaruint(v);
}

void OutputStream::writeUIntptr(uintptr_t v) {
  writeUInt64(v);
}

void OutputStream::writeString(const char *s) {
  while (*s) {
    writeByte(*s++);
  }
}

void OutputStream::writeChar(char c) {
  writeInt8(c);
}

void OutputStream::writeRawUInt64(uint64_t v) {
  U64Conv conv;

  conv.v = v;

  for (int i = 0; i < sizeof conv.b; ++i) {
    writeByte(conv.b[i]);
  }
}

//-------------------------------------------------------

uint8_t StdInputStream::readByte() {
  int v = i.get();
  assert(v != EOF);
  return static_cast<uint8_t>(v);
}

size_t StdInputStream::pos() {
  return static_cast<size_t>(i.tellg());
}

void StdInputStream::seek(size_t p) {
  i.seekg(static_cast<std::streampos>(p));
}

FileInputStream::FileInputStream(const char *fileName) {
  file = fopen(fileName, "rb");
}

FileInputStream::~FileInputStream() {
  fclose(file);
}

uint8_t FileInputStream::readByte() {
  int v = fgetc(file);
  assert(v != EOF);
  return static_cast<uint8_t>(v);
}

size_t FileInputStream::pos() {
  return static_cast<size_t>(ftell(file));
}

void FileInputStream::seek(size_t p) {
  fseek(file, p, SEEK_SET);
}

uint8_t MemoryInputStream::readByte() {
  assert(ptr < size);
  return buffer[ptr++];
}

size_t MemoryInputStream::pos() {
  return ptr;
}

void MemoryInputStream::seek(size_t p) {
  assert(p <= size);
  ptr = p;
}

//-----------------------------------------------------------

void StdOutputStream::writeByte(uint8_t v) {
  o.put(v);
}

size_t StdOutputStream::pos() {
  return static_cast<size_t>(o.tellp());
}

void StdOutputStream::seek(size_t p) {
  o.seekp(static_cast<std::streampos>(p));
}

FileOutputStream::FileOutputStream(const char *fileName) {
  file = fopen(fileName, "wb");
  close = true;
}

FileOutputStream::~FileOutputStream() {
  if (close) {
    fclose(file);
  }
}

void FileOutputStream::writeByte(uint8_t v) {
  fputc(v & 0xFF, file);
}

size_t FileOutputStream::pos() {
  return static_cast<size_t>(ftell(file));
}

void FileOutputStream::seek(size_t p) {
  fseek(file, p, SEEK_SET);
}

MemoryOutputStream::~MemoryOutputStream() {
  if (_buffer)
    free(_buffer);
}

void MemoryOutputStream::resize(size_t n) {
  if (n < _size) return;

  uint8_t *newBuffer = (uint8_t*)malloc(n);
  assert(newBuffer);

  if (_buffer) {
    memcpy(newBuffer, _buffer, _size * sizeof(uint8_t)); //-V575
  }

  free(_buffer);
  _buffer = newBuffer;
  _size = n;
}

void MemoryOutputStream::writeByte(uint8_t v) {
  if (ptr >= _size) {
    resize((_size + 512) << 2);
  }

  _buffer[ptr++] = v;
}

size_t MemoryOutputStream::pos() {
  return ptr;
}

void MemoryOutputStream::seek(size_t p) {
  assert(p <= _size);
  ptr = p;
}