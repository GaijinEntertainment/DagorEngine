#pragma once

#include <assert.h>
#include "squirrel.h"
#include "squtils.h"
#include "arena.h"
#include "sqobject.h"
#include "sourceloc.h"

// NOTE: There are some checkers that rely on the order of this list so re-arrange it carefully

#define TREE_OPS \
    DEF_TREE_OP(BLOCK), \
    DEF_TREE_OP(IF), \
    DEF_TREE_OP(WHILE), \
    DEF_TREE_OP(DOWHILE), \
    DEF_TREE_OP(FOR), \
    DEF_TREE_OP(FOREACH), \
    DEF_TREE_OP(SWITCH), \
    DEF_TREE_OP(RETURN), \
    DEF_TREE_OP(YIELD), \
    DEF_TREE_OP(THROW), \
    DEF_TREE_OP(TRY), \
    DEF_TREE_OP(BREAK), \
    DEF_TREE_OP(CONTINUE), \
    DEF_TREE_OP(EXPR_STMT), \
    DEF_TREE_OP(EMPTY), \
    DEF_TREE_OP(DIRECTIVE), \
    DEF_TREE_OP(IMPORT), \
    \
    DEF_TREE_OP(STATEMENT_MARK), \
    \
    DEF_TREE_OP(ID), \
    DEF_TREE_OP(COMMA), \
    DEF_TREE_OP(NULLC), \
    DEF_TREE_OP(ASSIGN), \
    DEF_TREE_OP(OROR), \
    DEF_TREE_OP(ANDAND), \
    DEF_TREE_OP(OR), \
    DEF_TREE_OP(XOR), \
    DEF_TREE_OP(AND), \
    DEF_TREE_OP(NE), \
    DEF_TREE_OP(EQ), \
    DEF_TREE_OP(3CMP), \
    DEF_TREE_OP(GE), \
    DEF_TREE_OP(GT), \
    DEF_TREE_OP(LE), \
    DEF_TREE_OP(LT), \
    DEF_TREE_OP(IN), \
    DEF_TREE_OP(INSTANCEOF), \
    DEF_TREE_OP(USHR), \
    DEF_TREE_OP(SHR), \
    DEF_TREE_OP(SHL), \
    DEF_TREE_OP(MUL), \
    DEF_TREE_OP(DIV), \
    DEF_TREE_OP(MOD), \
    DEF_TREE_OP(ADD), \
    DEF_TREE_OP(SUB), \
    DEF_TREE_OP(NEWSLOT), \
    DEF_TREE_OP(PLUSEQ), \
    DEF_TREE_OP(MINUSEQ), \
    DEF_TREE_OP(MULEQ), \
    DEF_TREE_OP(DIVEQ), \
    DEF_TREE_OP(MODEQ), \
    DEF_TREE_OP(NOT), \
    DEF_TREE_OP(BNOT), \
    DEF_TREE_OP(NEG), \
    DEF_TREE_OP(TYPEOF), \
    DEF_TREE_OP(STATIC_MEMO), \
    DEF_TREE_OP(INLINE_CONST), \
    DEF_TREE_OP(RESUME), \
    DEF_TREE_OP(CLONE), \
    DEF_TREE_OP(PAREN), \
    DEF_TREE_OP(CODE_BLOCK_EXPR), \
    DEF_TREE_OP(DELETE), \
    DEF_TREE_OP(LITERAL), \
    DEF_TREE_OP(BASE), \
    DEF_TREE_OP(ROOT_TABLE_ACCESS), \
    DEF_TREE_OP(INC), \
    DEF_TREE_OP(ARRAY), \
    DEF_TREE_OP(TABLE), \
    DEF_TREE_OP(CLASS), \
    DEF_TREE_OP(FUNCTION), \
    DEF_TREE_OP(GETFIELD), \
    DEF_TREE_OP(SETFIELD), \
    DEF_TREE_OP(GETSLOT), \
    DEF_TREE_OP(SETSLOT), \
    DEF_TREE_OP(CALL), \
    DEF_TREE_OP(TERNARY), \
    \
    DEF_TREE_OP(EXPR_MARK), \
    \
    DEF_TREE_OP(VAR), \
    DEF_TREE_OP(PARAM), \
    DEF_TREE_OP(CONST), \
    DEF_TREE_OP(DECL_GROUP), \
    DEF_TREE_OP(DESTRUCTURE), \
    DEF_TREE_OP(ENUM), \
    DEF_TREE_OP(DECLARATION_MARK), \

enum TreeOp {
#define DEF_TREE_OP(arg) TO_##arg
    TREE_OPS
#undef DEF_TREE_OP

};

namespace SQCompilation {

extern const char* sq_tree_op_names[];


class Visitor;
class Transformer;

class Expr;
class Statement;
class Decl;
class DestructuringDecl;

class Id;
class GetFieldExpr;
class GetSlotExpr;


class Node : public ArenaObj {
protected:
    Node(enum TreeOp op, SourceSpan span) : _op(op), _span(span) {}

public:
    ~Node() {}

    enum TreeOp op() const { return _op; }

    template<typename V>
    void visit(V *visitor);

    template<typename T>
    Node *transform(T *transformer);

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    bool isDeclaration() const { return TO_EXPR_MARK < op() && op() < TO_DECLARATION_MARK; }
    bool isStatement() const { return _op < TO_STATEMENT_MARK; }
    bool isExpression() const { return TO_STATEMENT_MARK < _op && _op < TO_EXPR_MARK; }
    bool isStatementOrDeclaration() const { return isStatement() || isDeclaration(); }

    inline Expr *asExpression() const { assert(isExpression()); return (Expr *)(this); }
    inline Statement *asStatement() const { assert(isStatement() || isDeclaration()); return (Statement *)(this); }
    inline Decl *asDeclaration() const { assert(isDeclaration()); return (Decl *)(this); }

    Id *asId() { assert(_op == TO_ID); return (Id*)this; }
    const Id *asId() const { assert(_op == TO_ID); return (const Id*)this; }
    GetFieldExpr *asGetField() const { assert(_op == TO_GETFIELD); return (GetFieldExpr*)this; }
    GetSlotExpr *asGetSlot() const { assert(_op == TO_GETSLOT); return (GetSlotExpr*)this; }
    DestructuringDecl *asDestructuringDecl() const { assert(_op == TO_DESTRUCTURE); return (DestructuringDecl*)this; }

    SourceSpan sourceSpan() const { return _span; }

    // Only for incrementally-built nodes
    void setSpanEnd(SourceLoc end) { _span.end = end; }

    // Convenience accessors
    int lineStart() const { return _span.start.line; }
    int lineEnd() const { return _span.end.line; }
    int columnStart() const { return _span.start.column; }
    int columnEnd() const { return _span.end.column; }
    int textWidth() const { return _span.textWidth(); }

private:
    enum TreeOp _op;
    SourceSpan _span;
};


struct DocObject {
    DocObject() : docString(nullptr) {}
    void setDocString(const char *doc_string) { docString = doc_string; }
    const char *getDocString() const { return docString; }
    bool isEmpty() const { return docString == nullptr; }
    // TODO: set/get DocTable (use SQObjectPtr ?)
private:
    const char *docString;
};


class AccessExpr;
class LiteralExpr;
class BinExpr;
class CallExpr;
class TableExpr;
class ClassExpr;
class FunctionExpr;

class Expr : public Node {
protected:
    Expr(enum TreeOp op, SourceSpan span) : Node(op, span), _typeMask(~0u), _typeInferred(false) {}
    unsigned _typeMask;
    bool _typeInferred;

public:
    unsigned getTypeMask() const { return _typeMask; }
    void setTypeMask(unsigned m) { _typeMask = m; }
    bool isTypeInferred() const { return _typeInferred; }
    void setTypeInferred() { _typeInferred = true; }

    bool isAccessExpr() const { return TO_GETFIELD <= op() && op() <= TO_SETSLOT; }
    AccessExpr *asAccessExpr() const { assert(isAccessExpr()); return (AccessExpr*)this; }
    LiteralExpr *asLiteral() const { assert(op() == TO_LITERAL); return (LiteralExpr *)this; }
    BinExpr *asBinExpr() const { assert(TO_NULLC <= op() && op() <= TO_MODEQ); return (BinExpr *)this; }
    CallExpr *asCallExpr() const { assert(op() == TO_CALL); return (CallExpr *)this; }
    TableExpr *asTableExpr() const { assert(op() == TO_TABLE || op() == TO_CLASS); return (TableExpr *)this; }
    ClassExpr *asClassExpr() const { assert(op() == TO_CLASS); return (ClassExpr *)this; }
    FunctionExpr *asFunctionExpr() const { assert(op() == TO_FUNCTION); return (FunctionExpr *)this; }
};


class Id : public Expr {
public:
    Id(SourceSpan span, const char *name) : Expr(TO_ID, span), _name(name) {}

    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}

    const char *name() const { return _name; }

private:
    const char *_name;
};

class UnExpr : public Expr {
public:
    UnExpr(enum TreeOp op, SourceLoc opStart, Expr *arg)
        : Expr(op, {opStart, arg->sourceSpan().end}), _arg(arg) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *argument() const { return _arg; }

private:
    Expr *_arg;
};

class BinExpr : public Expr {
public:
    BinExpr(enum TreeOp op, Expr *lhs, Expr *rhs)
        : Expr(op, SourceSpan::merge(lhs->sourceSpan(), rhs->sourceSpan())), _lhs(lhs), _rhs(rhs) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *lhs() const { return _lhs; }
    Expr *rhs() const { return _rhs; }

private:
    Expr *_lhs;
    Expr *_rhs;
};

class TerExpr : public Expr {
public:
    TerExpr(Expr *a, Expr *b, Expr *c)
        : Expr(TO_TERNARY, SourceSpan::merge(a->sourceSpan(), c->sourceSpan())), _a(a), _b(b), _c(c) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *a() const { return _a; }
    Expr *b() const { return _b; }
    Expr *c() const { return _c; }

private:
    Expr *_a;
    Expr *_b;
    Expr *_c;
};

class FieldAccessExpr;

class AccessExpr : public Expr {
protected:
    AccessExpr(enum TreeOp op, SourceSpan span, Expr *receiver, bool nullable)
        : Expr(op, span), _receiver(receiver), _nullable(nullable) {}
public:

    bool isFieldAccessExpr() const { return op() == TO_GETFIELD || op() == TO_SETFIELD; }
    FieldAccessExpr *asFieldAccessExpr() const { assert(isFieldAccessExpr()); return (FieldAccessExpr *)this; }

    bool isNullable() const { return _nullable; }
    Expr *receiver() const { return _receiver; }

protected:
    Expr *_receiver;
    bool _nullable;
};

class FieldAccessExpr : public AccessExpr {
protected:
    FieldAccessExpr(enum TreeOp op, SourceSpan span, Expr *receiver, const char *field, bool nullable)
        : AccessExpr(op, span, receiver, nullable), _fieldName(field) {}

public:

    bool canBeLiteral(bool typeMethod) const { return receiver()->op() != TO_BASE && !isNullable() && !typeMethod; }
    const char *fieldName() const { return _fieldName; }

private:
    const char *_fieldName;

};

class GetFieldExpr : public FieldAccessExpr {
    bool _isTypeMethod;
public:
    // Constructor computes span from receiver start to end of field name
    GetFieldExpr(Expr *receiver, const char *field, bool nullable, bool is_type_method, SourceLoc end)
        : FieldAccessExpr(TO_GETFIELD, {receiver->sourceSpan().start, end}, receiver, field, nullable)
        , _isTypeMethod(is_type_method) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    bool isTypeMethod() const { return _isTypeMethod; }
};


class SetFieldExpr : public FieldAccessExpr {
public:
    // Constructor computes span from receiver start to value end
    SetFieldExpr(Expr *receiver, const char *field, Expr *value, bool nullable)
        : FieldAccessExpr(TO_SETFIELD, {receiver->sourceSpan().start, value->sourceSpan().end}, receiver, field, nullable)
        , _value(value) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *value() const { return _value; }

protected:
    Expr *_value;
};


class SlotAccessExpr : public AccessExpr {
protected:
    SlotAccessExpr(enum TreeOp op, SourceSpan span, Expr *receiver, Expr *key, bool nullable)
        : AccessExpr(op, span, receiver, nullable), _key(key) {}
public:

    Expr *key() const { return _key; }
protected:
    Expr *_key;
};

class GetSlotExpr : public SlotAccessExpr {
public:
    GetSlotExpr(Expr *receiver, Expr *key, bool nullable, SourceLoc end)
        : SlotAccessExpr(TO_GETSLOT, {receiver->sourceSpan().start, end}, receiver, key, nullable) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);
};

class SetSlotExpr : public SlotAccessExpr {
public:
    SetSlotExpr(Expr *receiver, Expr *key, Expr *val, bool nullable)
        : SlotAccessExpr(TO_SETSLOT, {receiver->sourceSpan().start, val->sourceSpan().end}, receiver, key, nullable)
        , _val(val) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *value() const { return _val; }
private:
    Expr *_val;
};

class BaseExpr : public Expr {
public:
    BaseExpr(SourceSpan span) : Expr(TO_BASE, span) {}

    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}
};

class RootTableAccessExpr : public Expr {
public:
    RootTableAccessExpr(SourceSpan span) : Expr(TO_ROOT_TABLE_ACCESS, span) {}

    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}
};

enum LiteralKind {
    LK_STRING,
    LK_INT,
    LK_FLOAT,
    LK_BOOL,
    LK_NULL
};

class LiteralExpr : public Expr {
public:
    LiteralExpr(SourceSpan span) : Expr(TO_LITERAL, span), _kind(LK_NULL) { _v.raw = 0; }
    LiteralExpr(SourceSpan span, const char *s) : Expr(TO_LITERAL, span), _kind(LK_STRING) { _v.s = s; }
    LiteralExpr(SourceSpan span, SQFloat f) : Expr(TO_LITERAL, span), _kind(LK_FLOAT) { _v.f = f; }
    LiteralExpr(SourceSpan span, SQInteger i) : Expr(TO_LITERAL, span), _kind(LK_INT) { _v.i = i; }
    LiteralExpr(SourceSpan span, bool b) : Expr(TO_LITERAL, span), _kind(LK_BOOL) { _v.b = b; }

    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}

    enum LiteralKind kind() const { return _kind;  }

    SQFloat f() const { assert(_kind == LK_FLOAT); return _v.f; }
    SQInteger i() const { assert(_kind == LK_INT); return _v.i; }
    bool b() const { assert(_kind == LK_BOOL); return _v.b; }
    const char *s() const { assert(_kind == LK_STRING); return _v.s; }
    void *null() const { assert(_kind == LK_NULL); return nullptr; }
    SQUnsignedInteger raw() const { return _v.raw; }

private:
    enum LiteralKind _kind;
    union {
        const char *s;
        SQInteger i;
        SQFloat f;
        bool b;
        SQUnsignedInteger raw;
    } _v;

};

enum IncForm {
    IF_PREFIX,
    IF_POSTFIX
};

class IncExpr : public Expr {
public:
    // Constructor computes span based on form
    // For prefix: opLoc is the start of the operator, span is {opLoc, arg->span().end}
    // For postfix: opLoc is the end of the operator, span is {arg->span().start, opLoc}
    IncExpr(Expr *arg, int diff, enum IncForm form, SourceLoc opLoc)
        : Expr(TO_INC, form == IF_PREFIX
            ? SourceSpan{opLoc, arg->sourceSpan().end}
            : SourceSpan{arg->sourceSpan().start, opLoc})
        , _arg(arg), _diff(diff), _form(form) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    enum IncForm form() const { return _form; }
    int diff() const { return _diff; }
    Expr *argument() const { return _arg; }

private:
    Expr *_arg;
    int _diff;
    enum IncForm _form;
};

class CallExpr : public Expr {
public:
    // For incremental building - call setSpanEnd() after adding arguments
    CallExpr(Arena *arena, Expr *callee, bool nullable)
        : Expr(TO_CALL, {callee->sourceSpan().start, SourceLoc::invalid()})
        , _callee(callee), _args(arena), _nullable(nullable) {}

    // For cases where end is known at construction time
    CallExpr(Arena *arena, Expr *callee, bool nullable, SourceLoc end)
        : Expr(TO_CALL, {callee->sourceSpan().start, end})
        , _callee(callee), _args(arena), _nullable(nullable) {}

    void addArgument(Expr *arg) { _args.push_back(arg); }

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    bool isNullable() const { return _nullable; }
    Expr *callee() const { return _callee; }
    const ArenaVector<Expr *> &arguments() const { return _args; }
    ArenaVector<Expr *> &arguments() { return _args; }

private:
    Expr *_callee;
    ArenaVector<Expr *> _args;
    bool _nullable;
};

class ArrayExpr : public Expr {
public:
    // Incremental building - call setSpanEnd() after adding values
    ArrayExpr(Arena *arena, SourceLoc start)
        : Expr(TO_ARRAY, {start, SourceLoc::invalid()}), _inits(arena) {}

    void addValue(Expr *v) { _inits.push_back(v); }

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    const ArenaVector<Expr *> &initializers() const { return _inits; }
    ArenaVector<Expr *> &initializers() { return _inits; }

private:
    ArenaVector<Expr *> _inits;
};

class CommaExpr : public Expr {
public:
    CommaExpr(Arena *arena, ArenaVector<Expr *> exprs)
        : Expr(TO_COMMA, computeSpan(exprs)), _exprs(std::move(exprs)) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    const ArenaVector<Expr *> &expressions() const { return _exprs; }
    ArenaVector<Expr *> &expressions() { return _exprs; }

private:
    static SourceSpan computeSpan(const ArenaVector<Expr *> &exprs) {
        if (exprs.empty()) return SourceSpan::invalid();
        return SourceSpan::merge(exprs[0]->sourceSpan(), exprs.back()->sourceSpan());
    }

    ArenaVector<Expr *> _exprs;
};

class Block;

class Statement : public Node {
protected:
    Statement(enum TreeOp op, SourceSpan span) : Node(op, span) {}

public:
    inline Block *asBlock() const { assert(op() == TO_BLOCK); return (Block *)(this); }
};

class DirectiveStmt : public Statement {
public:
    DirectiveStmt(SourceSpan span) : Statement(TO_DIRECTIVE, span) {}

    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}

    uint32_t setFlags = 0, clearFlags = 0;
    bool applyToDefault = false;
};


class ImportStmt : public Statement {
public:
    // Incremental building - call setSpanEnd() after parsing
    ImportStmt(Arena *arena_, SourceLoc start, const char *module_name, const char *module_alias)
        : Statement(TO_IMPORT, {start, SourceLoc::invalid()})
        , arena(arena_), moduleName(module_name), moduleAlias(module_alias), slots(arena_) {}

    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}

public:
    Arena *arena;
    const char *moduleName;
    const char *moduleAlias;
    int nameCol = 0;
    int aliasCol = 0;
    ArenaVector<SQModuleImportSlot> slots;
};


class ParamDecl;
class VarDecl;

class Decl : public Statement {
protected:
    Decl(enum TreeOp op, SourceSpan span) : Statement(op, span), typeMask(~0u) {}
    unsigned typeMask;
public:

    ParamDecl *asParam() const { assert(op() == TO_PARAM); return (ParamDecl *)(this); }
    VarDecl *asVarDecl() const { assert(op() == TO_VAR); return (VarDecl *)(this); }
    void setTypeMask(unsigned typeMask_) { typeMask = typeMask_; }
    unsigned getTypeMask() const { return typeMask; }
};

class ValueDecl : public Decl {
protected:
    ValueDecl(enum TreeOp op, SourceSpan span, const char *name, Expr *expr)
        : Decl(op, span), _name(name), _expr(expr) {}
public:
    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *expression() const { return _expr; }
    const char *name() const { return _name; }

private:
    const char *_name;
    Expr *_expr;
};

class DestructuringDecl;

class ParamDecl : public ValueDecl {
    bool _isVararg;
    DestructuringDecl *_destructuring;
public:
    ParamDecl(SourceSpan nameSpan, const char *name, Expr *defaultVal)
        : ValueDecl(TO_PARAM,
            {nameSpan.start, defaultVal ? defaultVal->sourceSpan().end : nameSpan.end},
            name, defaultVal)
        , _isVararg(false)
        , _destructuring(nullptr) {}

    bool hasDefaultValue() const { return expression() != NULL; }
    Expr *defaultValue() const { return expression(); }

    void setVararg() { _isVararg = true; };
    bool isVararg() const { return _isVararg; }

    void setDestructuring(DestructuringDecl *destructuring) { _destructuring = destructuring; }
    DestructuringDecl *getDestructuring() const { return _destructuring; }
};

class VarDecl : public ValueDecl {
    Id *_nameId;
public:
    VarDecl(SourceLoc start, Id *nameId, Expr *init, bool assignable, bool destructured = false)
        : ValueDecl(TO_VAR,
            {start, init ? init->sourceSpan().end : nameId->sourceSpan().end},
            nameId->name(), init)
        , _nameId(nameId), _assignable(assignable), _destructured(destructured) {}

    Expr *initializer() const { return expression(); }

    // Get the Id node (for diagnostics - points to identifier)
    Id *nameId() const { return _nameId; }

    bool isAssignable() const { return _assignable; }
    bool isDestructured() const { return _destructured; }

private:
    bool _assignable;
    bool _destructured;
};

enum TableMemberFlags : unsigned {
  TMF_STATIC = (1U << 0),
  TMF_DYNAMIC_KEY = (1U << 1),
  TMF_JSON = (1U << 2)
};

struct TableMember {
    Expr *key;
    Expr *value;
    unsigned flags;

    bool isStatic() const { return (flags & TMF_STATIC) != 0; }
    bool isDynamicKey() const { return (flags & TMF_DYNAMIC_KEY) != 0; }
    bool isJson() const { return (flags & TMF_JSON) != 0; }
};

class TableExpr : public Expr {
public:
    // Incremental building - call setSpanEnd() after adding members
    TableExpr(Arena *arena, SourceLoc start)
        : Expr(TO_TABLE, {start, SourceLoc::invalid()}), _members(arena) {}

    void addMember(Expr *key, Expr *value, unsigned keys = 0) { _members.push_back({ key, value, keys }); }

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    ArenaVector<TableMember> &members() { return _members; }
    const ArenaVector<TableMember> &members() const { return _members; }

    DocObject docObject;

protected:
    TableExpr(Arena *arena, enum TreeOp op, SourceLoc start)
        : Expr(op, {start, SourceLoc::invalid()}), _members(arena) {}

private:
    ArenaVector<TableMember> _members;
};

class ClassExpr : public TableExpr {
public:
    // Incremental building - span starts at class keyword, call setSpanEnd() after body
    ClassExpr(Arena *arena, SourceLoc classKeywordStart, Expr *key, Expr *base)
        : TableExpr(arena, TO_CLASS, classKeywordStart), _key(key), _base(base) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *classBase() const { return _base; }
    Expr* classKey() const { return _key; }

    void setClassBase(Expr *b) { _base = b; }
    void setClassKey(Expr *k) { _key = k; }

    FunctionExpr *findConstructor() const;

private:
    Expr *_key;
    Expr *_base;

};

class Block;

class FunctionExpr : public Expr {
    Id *_nameId;       // Name identifier (for diagnostics)
protected:
    // Incremental building - call setSpanEnd() after body is set
    FunctionExpr(enum TreeOp op, Arena *arena, SourceLoc start, const char *name, Id *nameId = nullptr)
        : Expr(op, {start, SourceLoc::invalid()}), _arena(arena), _parameters(arena), _name(name), _nameId(nameId),
        _vararg(false), _body(NULL), _lambda(false), _pure(false), _nodiscard(false), _sourcename(NULL), _hoistingLevel(0), _resultTypeMask(~0u) {}

public:
    FunctionExpr(Arena *arena, SourceLoc start, Id *nameId)
        : Expr(TO_FUNCTION, {start, SourceLoc::invalid()}), _arena(arena), _parameters(arena), _name(nameId->name()), _nameId(nameId),
        _vararg(false), _body(NULL), _lambda(false), _pure(false), _nodiscard(false), _sourcename(NULL), _hoistingLevel(0), _resultTypeMask(~0u) {}

    FunctionExpr(Arena *arena, SourceLoc start, const char *name)
        : Expr(TO_FUNCTION, {start, SourceLoc::invalid()}), _arena(arena), _parameters(arena), _name(name), _nameId(nullptr),
        _vararg(false), _body(NULL), _lambda(false), _pure(false), _nodiscard(false), _sourcename(NULL), _hoistingLevel(0), _resultTypeMask(~0u) {}

    void addParameter(SourceSpan nameSpan, const char *name, Expr *defaultVal = NULL) {
        _parameters.push_back(new (_arena) ParamDecl(nameSpan, name, defaultVal));
    }

    ArenaVector<ParamDecl *> &parameters() { return _parameters; }
    const ArenaVector<ParamDecl *> &parameters() const { return _parameters; }

    void setVararg(bool v) { _vararg = v; }
    void setBody(Block *body);

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    void setName(const char *newName) { _name = newName; }
    const char *name() const { return _name; }
    Id *nameId() const { return _nameId; }
    bool isVararg() const { return _vararg; }
    Block *body() const { return _body; }

    void setSourceName(const char *sn) { _sourcename = sn; }
    const char *sourceName() const { return _sourcename; }

    bool isLambda() const { return _lambda; }
    void setLambda(bool v) { _lambda = v; }

    void setPure(bool v) { _pure = v; }
    bool isPure() const { return _pure; }

    void setNodiscard(bool v) { _nodiscard = v; }
    bool isNodiscard() const { return _nodiscard; }

    int hoistingLevel() const { return _hoistingLevel; }
    void hoistBy(int level) { _hoistingLevel += level; }

    unsigned getResultTypeMask() const { return _resultTypeMask; }
    void setResultTypeMask(unsigned resultTypeMask) { _resultTypeMask = resultTypeMask; }

    DocObject docObject;

private:
    Arena *_arena;
    const char *_name;
    ArenaVector<ParamDecl *> _parameters;
    Block * _body;
    unsigned _resultTypeMask;
    bool _vararg;
    bool _lambda;
    bool _pure;
    bool _nodiscard;

    const char *_sourcename;
    int _hoistingLevel;
};

struct EnumConst {
    const char *id;
    LiteralExpr *val;
};

class EnumDecl : public Decl {
public:
    // Incremental building - call setSpanEnd() after constants are added
    EnumDecl(Arena *arena, SourceLoc start, const char *id, bool global)
        : Decl(TO_ENUM, {start, SourceLoc::invalid()}), _id(id), _consts(arena), _global(global) {}

    void addConst(const char *id, LiteralExpr *val) { _consts.push_back({ id, val }); }

    ArenaVector<EnumConst> &consts() { return _consts; }
    const ArenaVector<EnumConst> &consts() const { return _consts; }

    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}

    const char *name() const { return _id; }
    bool isGlobal() const { return _global; }

private:
    ArenaVector<EnumConst> _consts;
    const char *_id;
    bool _global;
};

class ConstDecl : public Decl {
public:
    ConstDecl(SourceLoc start, const char *id, Expr *value, bool global)
        : Decl(TO_CONST, {start, value->sourceSpan().end}), _id(id), _value(value), _global(global) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    const char *name() const { return _id; }
    Expr *value() const { return _value; }
    bool isGlobal() const { return _global; }

private:
    const char *_id;
    Expr *_value;
    bool _global;
};

class DeclGroup : public Decl {
protected:
    // Incremental building - span start is keyword, end is set when last decl is added
    DeclGroup(Arena *arena, enum TreeOp op, SourceLoc start)
        : Decl(op, {start, SourceLoc::invalid()}), _decls(arena) {}
public:
    DeclGroup(Arena *arena, SourceLoc start)
        : Decl(TO_DECL_GROUP, {start, SourceLoc::invalid()}), _decls(arena) {}

    void addDeclaration(VarDecl *d) {
      _decls.push_back(d);
      // Update span end to include the new declaration
      setSpanEnd(d->sourceSpan().end);
    }

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    ArenaVector<VarDecl *> &declarations() { return _decls; }
    const ArenaVector<VarDecl *> &declarations() const { return _decls; }

private:
    ArenaVector<VarDecl *> _decls;
};

enum DestructuringType {
    DT_TABLE,
    DT_ARRAY
};

class DestructuringDecl : public DeclGroup {
public:
    DestructuringDecl(Arena *arena, SourceLoc start, enum DestructuringType dt)
        : DeclGroup(arena, TO_DESTRUCTURE, start), _dt_type(dt), _expr(NULL) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    void setExpression(Expr *expr) {
        _expr = expr;
        setSpanEnd(_expr->sourceSpan().end);
    }
    Expr *initExpression() const { return _expr; }

    void setType(enum DestructuringType t) { _dt_type = t; }
    enum DestructuringType type() const { return _dt_type; }

private:
    Expr *_expr;
    enum DestructuringType _dt_type;
};

class Block : public Statement {
public:
    // Incremental building - call setSpanEnd() after statements are added
    Block(Arena *arena, SourceLoc start, bool is_root = false)
        : Statement(TO_BLOCK, {start, SourceLoc::invalid()})
        , _statements(arena), _is_root(is_root), _is_body(false), _is_expr_block(false) {}

    void addStatement(Statement *stmt) { assert(stmt); _statements.push_back(stmt); }

    ArenaVector<Statement *> &statements() { return _statements; }
    const ArenaVector<Statement *> &statements() const { return _statements; }

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    bool isRoot() const { return _is_root; }

    bool isBody() const { return _is_body; }
    void setIsBody() { _is_body = true; }

    bool isExprBlock() { return _is_expr_block; }
    void setIsExprBlock() { _is_expr_block = true; }

private:
    ArenaVector<Statement *> _statements;
    bool _is_root;
    bool _is_body;
    bool _is_expr_block;
};

class RootBlock : public Block {
public:
    RootBlock(Arena *arena, SourceLoc start) : Block(arena, start, true) {}
};

class CodeBlockExpr : public Expr {
public:
    CodeBlockExpr(Block *block): Expr(TO_CODE_BLOCK_EXPR, block->sourceSpan()), _block(block) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Block *block() const { return _block; }

private:
    Block *_block;
};

class IfStatement : public Statement {
public:
    IfStatement(SourceLoc start, Expr *cond, Statement *thenB, Statement *elseB)
        : Statement(TO_IF, {start, (elseB ? elseB : thenB)->sourceSpan().end})
        , _cond(cond), _thenB(thenB), _elseB(elseB) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *condition() const { return _cond; }
    Statement *thenBranch() const { return _thenB; }
    Statement *elseBranch() const { return _elseB; }

private:
    Expr *_cond;
    Statement *_thenB;
    Statement *_elseB;
};

class LoopStatement : public Statement {
protected:
    // Constructor computes span from keyword start to body end
    LoopStatement(enum TreeOp op, SourceLoc start, Statement *body)
        : Statement(op, {start, body->sourceSpan().end}), _body(body) {}

    // For DoWhile which needs a different span
    LoopStatement(enum TreeOp op, SourceSpan span, Statement *body)
        : Statement(op, span), _body(body) {}
public:
    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Statement *body() const { return _body; }

private:
    Statement *_body;
};

class WhileStatement : public LoopStatement {
public:
    WhileStatement(SourceLoc start, Expr *cond, Statement *body)
        : LoopStatement(TO_WHILE, start, body), _cond(cond) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *condition() const { return _cond;  }

private:
    Expr *_cond;
};

class DoWhileStatement : public LoopStatement {
public:
    DoWhileStatement(SourceLoc start, Statement *body, Expr *cond, SourceLoc end)
        : LoopStatement(TO_DOWHILE, {start, end}, body), _cond(cond) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *condition() const { return _cond; }

private:
    Expr *_cond;
};

class ForStatement : public LoopStatement {
public:
    ForStatement(SourceLoc start, Node *init, Expr *cond, Expr *mod, Statement *body)
        : LoopStatement(TO_FOR, start, body), _init(init), _cond(cond), _mod(mod) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Node *initializer() const { return _init; }
    Expr *condition() const { return _cond; }
    Expr *modifier() const { return _mod; }


private:
    Node *_init;
    Expr *_cond;
    Expr *_mod;
};

class ForeachStatement : public LoopStatement {
public:
    ForeachStatement(SourceLoc start, VarDecl *idx, VarDecl *val, Expr *container, Statement *body)
        : LoopStatement(TO_FOREACH, start, body), _idx(idx), _val(val), _container(container) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *container() const { return _container; }
    VarDecl *idx() const { return _idx; }
    VarDecl *val() const { return _val; }

private:
    VarDecl *_idx;
    VarDecl *_val;
    Expr *_container;
};

struct SwitchCase {
    Expr *val;
    Statement *stmt;
};

class SwitchStatement : public Statement {
public:
    // Incremental building - call setSpanEnd() after cases are added
    SwitchStatement(SourceLoc start, Arena *arena, Expr *expr)
        : Statement(TO_SWITCH, {start, SourceLoc::invalid()}), _expr(expr), _cases(arena), _defaultCase() {}

    void addCases(Expr *val, Statement *stmt) { _cases.push_back({ val, stmt }); }

    void addDefault(Statement *stmt) {
        assert(_defaultCase.stmt == NULL);
        _defaultCase.stmt = stmt;
    }

    ArenaVector<SwitchCase> &cases() { return _cases; }
    const ArenaVector<SwitchCase> &cases() const { return _cases; }

    Expr *expression() const { return _expr; }
    const SwitchCase &defaultCase() const { return _defaultCase; }

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

private:
    Expr *_expr;
    ArenaVector<SwitchCase> _cases;
    SwitchCase _defaultCase;
};

class TryStatement : public Statement {
public:
    TryStatement(SourceLoc start, Statement *t, Id *exc, Statement *c)
        : Statement(TO_TRY, {start, c->sourceSpan().end}), _tryStmt(t), _exception(exc), _catchStmt(c) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Statement *tryStatement() const { return _tryStmt; }
    Id *exceptionId() const { return _exception; }
    Statement *catchStatement() const { return _catchStmt; }

private:
    Statement *_tryStmt;
    Id *_exception;
    Statement *_catchStmt;
};

class TerminateStatement : public Statement {
protected:
    TerminateStatement(enum TreeOp op, SourceSpan keywordSpan, Expr *arg)
        : Statement(op, arg ? SourceSpan{keywordSpan.start, arg->sourceSpan().end} : keywordSpan)
        , _arg(arg) {}
public:
    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *argument() const { return _arg; }

private:
    Expr *_arg;
};

class ReturnStatement : public TerminateStatement {
public:
    ReturnStatement(SourceSpan keywordSpan, Expr *arg)
        : TerminateStatement(TO_RETURN, keywordSpan, arg), _isLambda(false) {}


    void setIsLambda() { _isLambda = true; }
    bool isLambdaReturn() const { return _isLambda; }

private:
    bool _isLambda;
};

class YieldStatement : public TerminateStatement {
public:
    YieldStatement(SourceSpan keywordSpan, Expr *arg) : TerminateStatement(TO_YIELD, keywordSpan, arg) {}
};

class ThrowStatement : public TerminateStatement {
public:
    ThrowStatement(SourceSpan keywordSpan, Expr *arg) : TerminateStatement(TO_THROW, keywordSpan, arg) { assert(arg); }
};

class JumpStatement : public Statement {
protected:
    JumpStatement(enum TreeOp op, SourceSpan span) : Statement(op, span) {}
public:
    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}
};

class BreakStatement : public JumpStatement {
public:
    BreakStatement(SourceSpan span, Statement *breakTarget) : JumpStatement(TO_BREAK, span), _target(breakTarget) {}

private:
    Statement *_target; // Currently not used
};

class ContinueStatement : public JumpStatement {
public:
    ContinueStatement(SourceSpan span, LoopStatement *target) : JumpStatement(TO_CONTINUE, span), _target(target) {}

private:
    LoopStatement *_target; // Currently not used
};

class ExprStatement : public Statement {
public:
    ExprStatement(Expr *expr) : Statement(TO_EXPR_STMT, expr->sourceSpan()), _expr(expr) {}

    void visitChildren(Visitor *visitor);
    void transformChildren(Transformer *transformer);

    Expr *expression() const { return _expr; }

private:
    Expr *_expr;
};

class EmptyStatement : public Statement {
public:
    EmptyStatement(SourceSpan span) : Statement(TO_EMPTY, span) {}

    void visitChildren(Visitor *visitor) {}
    void transformChildren(Transformer *transformer) {}
};

const char* treeopStr(enum TreeOp op);

class Visitor {
protected:
    Visitor() {}
public:
    virtual ~Visitor() {}

    virtual void visitNode(Node *node) { node->visitChildren(this); }

    virtual void visitExpr(Expr *expr) { visitNode(expr); }
    virtual void visitUnExpr(UnExpr *expr) { visitExpr(expr); }
    virtual void visitCodeBlockExpr(CodeBlockExpr *expr) { visitExpr(expr); }
    virtual void visitBinExpr(BinExpr *expr) { visitExpr(expr); }
    virtual void visitTerExpr(TerExpr *expr) { visitExpr(expr); }
    virtual void visitCallExpr(CallExpr *expr) { visitExpr(expr); }
    virtual void visitId(Id *id) { visitExpr(id); }
    virtual void visitAccessExpr(AccessExpr *expr) { visitExpr(expr); }
    virtual void visitGetFieldExpr(GetFieldExpr *expr) { visitAccessExpr(expr); }
    virtual void visitSetFieldExpr(SetFieldExpr *expr) { visitAccessExpr(expr); }
    virtual void visitGetSlotExpr(GetSlotExpr *expr) { visitAccessExpr(expr); }
    virtual void visitSetSlotExpr(SetSlotExpr *expr) { visitAccessExpr(expr); }
    virtual void visitBaseExpr(BaseExpr *expr) { visitExpr(expr); }
    virtual void visitRootTableAccessExpr(RootTableAccessExpr *expr) { visitExpr(expr); }
    virtual void visitLiteralExpr(LiteralExpr *expr) { visitExpr(expr); }
    virtual void visitIncExpr(IncExpr *expr) { visitExpr(expr); }
    virtual void visitArrayExpr(ArrayExpr *expr) { visitExpr(expr); }
    virtual void visitTableExpr(TableExpr *tbl) { visitExpr(tbl); }
    virtual void visitClassExpr(ClassExpr *cls) { visitTableExpr(cls); }
    virtual void visitFunctionExpr(FunctionExpr *f) { visitExpr(f); }
    virtual void visitCommaExpr(CommaExpr *expr) { visitExpr(expr); }

    virtual void visitStmt(Statement *stmt) { visitNode(stmt); }
    virtual void visitBlock(Block *block) { visitStmt(block); }
    virtual void visitIfStatement(IfStatement *ifstmt) { visitStmt(ifstmt); }
    virtual void visitLoopStatement(LoopStatement *loop) { visitStmt(loop); }
    virtual void visitWhileStatement(WhileStatement *loop) { visitLoopStatement(loop); }
    virtual void visitDoWhileStatement(DoWhileStatement *loop) { visitLoopStatement(loop); }
    virtual void visitForStatement(ForStatement *loop) { visitLoopStatement(loop); }
    virtual void visitForeachStatement(ForeachStatement *loop) { visitLoopStatement(loop); }
    virtual void visitSwitchStatement(SwitchStatement *swtch) { visitStmt(swtch); }
    virtual void visitTryStatement(TryStatement *tr) { visitStmt(tr); }
    virtual void visitTerminateStatement(TerminateStatement *term) { visitStmt(term); }
    virtual void visitReturnStatement(ReturnStatement *ret) { visitTerminateStatement(ret); }
    virtual void visitYieldStatement(YieldStatement *yld) { visitTerminateStatement(yld); }
    virtual void visitThrowStatement(ThrowStatement *thr) { visitTerminateStatement(thr); }
    virtual void visitJumpStatement(JumpStatement *jmp) { visitStmt(jmp); }
    virtual void visitBreakStatement(BreakStatement *jmp) { visitJumpStatement(jmp); }
    virtual void visitContinueStatement(ContinueStatement *jmp) { visitJumpStatement(jmp); }
    virtual void visitExprStatement(ExprStatement *estmt) { visitStmt(estmt); }
    virtual void visitEmptyStatement(EmptyStatement *empty) { visitStmt(empty); }

    virtual void visitDecl(Decl *decl) { visitStmt(decl); }
    virtual void visitValueDecl(ValueDecl *decl) { visitDecl(decl); }
    virtual void visitVarDecl(VarDecl *decl) { visitValueDecl(decl); }
    virtual void visitParamDecl(ParamDecl *decl) { visitValueDecl(decl); }
    virtual void visitConstDecl(ConstDecl *cnst) { visitDecl(cnst); }
    virtual void visitEnumDecl(EnumDecl *enm) { visitDecl(enm); }
    virtual void visitDeclGroup(DeclGroup *grp) { visitDecl(grp); }
    virtual void visitDestructuringDecl(DestructuringDecl  *destruct) { visitDecl(destruct); }
    virtual void visitDirectiveStatement(DirectiveStmt *dir) { visitStmt(dir); }
    virtual void visitImportStatement(ImportStmt *import) { visitStmt(import); }
};

class Transformer {
protected:
  Transformer() {}
public:
  virtual ~Transformer() {}

  virtual Node* transformNode(Node *node) { node->transformChildren(this); return node; }

  virtual Node *transformExpr(Expr *expr) { return transformNode(expr); }
  virtual Node *transformUnExpr(UnExpr *expr) { return transformExpr(expr); }
  virtual Node *transformCodeBlockExpr(CodeBlockExpr *expr) { return transformExpr(expr); }
  virtual Node *transformBinExpr(BinExpr *expr) { return transformExpr(expr); }
  virtual Node *transformTerExpr(TerExpr *expr) { return transformExpr(expr); }
  virtual Node *transformCallExpr(CallExpr *expr) { return transformExpr(expr); }
  virtual Node *transformId(Id *id) { return transformExpr(id); }
  virtual Node *transformGetFieldExpr(GetFieldExpr *expr) { return transformExpr(expr); }
  virtual Node *transformSetFieldExpr(SetFieldExpr *expr) { return transformExpr(expr); }
  virtual Node *transformGetSlotExpr(GetSlotExpr *expr) { return transformExpr(expr); }
  virtual Node *transformSetSlotExpr(SetSlotExpr *expr) { return transformExpr(expr); }
  virtual Node *transformBaseExpr(BaseExpr *expr) { return transformExpr(expr); }
  virtual Node *transformRootTableAccessExpr(RootTableAccessExpr *expr) { return transformExpr(expr); }
  virtual Node *transformLiteralExpr(LiteralExpr *expr) { return transformExpr(expr); }
  virtual Node *transformIncExpr(IncExpr *expr) { return transformExpr(expr); }
  virtual Node *transformArrayExpr(ArrayExpr *expr) { return transformExpr(expr); }
  virtual Node *transformTableExpr(TableExpr *tbl) { return transformExpr(tbl); }
  virtual Node *transformClassExpr(ClassExpr *cls) { return transformTableExpr(cls); }
  virtual Node *transformFunctionExpr(FunctionExpr *f) { return transformExpr(f); }
  virtual Node *transformCommaExpr(CommaExpr *expr) { return transformExpr(expr); }

  virtual Node *transformStmt(Statement *stmt) { return transformNode(stmt); }
  virtual Node *transformBlock(Block *block) { return transformStmt(block); }
  virtual Node *transformIfStatement(IfStatement *ifstmt) { return transformStmt(ifstmt); }
  virtual Node *transformLoopStatement(LoopStatement *loop) { return transformStmt(loop); }
  virtual Node *transformWhileStatement(WhileStatement *loop) { return transformLoopStatement(loop); }
  virtual Node *transformDoWhileStatement(DoWhileStatement *loop) { return transformLoopStatement(loop); }
  virtual Node *transformForStatement(ForStatement *loop) { return transformLoopStatement(loop); }
  virtual Node *transformForeachStatement(ForeachStatement *loop) { return transformLoopStatement(loop); }
  virtual Node *transformSwitchStatement(SwitchStatement *swtch) { return transformStmt(swtch); }
  virtual Node *transformTryStatement(TryStatement *tr) { return transformStmt(tr); }
  virtual Node *transformTerminateStatement(TerminateStatement *term) { return transformStmt(term); }
  virtual Node *transformReturnStatement(ReturnStatement *ret) { return transformTerminateStatement(ret); }
  virtual Node *transformYieldStatement(YieldStatement *yld) { return transformTerminateStatement(yld); }
  virtual Node *transformThrowStatement(ThrowStatement *thr) { return transformTerminateStatement(thr); }
  virtual Node *transformJumpStatement(JumpStatement *jmp) { return transformStmt(jmp); }
  virtual Node *transformBreakStatement(BreakStatement *jmp) { return transformJumpStatement(jmp); }
  virtual Node *transformContinueStatement(ContinueStatement *jmp) { return transformJumpStatement(jmp); }
  virtual Node *transformExprStatement(ExprStatement *estmt) { return transformStmt(estmt); }
  virtual Node *transformEmptyStatement(EmptyStatement *empty) { return transformStmt(empty); }

  virtual Node *transformDecl(Decl *decl) { return transformStmt(decl); }
  virtual Node *transformValueDecl(ValueDecl *decl) { return transformDecl(decl); }
  virtual Node *transformVarDecl(VarDecl *decl) { return transformValueDecl(decl); }
  virtual Node *transformParamDecl(ParamDecl *decl) { return transformValueDecl(decl); }
  virtual Node *transformConstDecl(ConstDecl *cnst) { return transformDecl(cnst); }
  virtual Node *transformEnumDecl(EnumDecl *enm) { return transformDecl(enm); }
  virtual Node *transformDeclGroup(DeclGroup *grp) { return transformDecl(grp); }
  virtual Node *transformDestructuringDecl(DestructuringDecl  *destruct) { return transformDecl(destruct); }
};

template<typename V>
void Node::visit(V *visitor) {
    switch (op())
    {
    case TO_BLOCK:      visitor->visitBlock(static_cast<Block *>(this)); return;
    case TO_IF:         visitor->visitIfStatement(static_cast<IfStatement *>(this)); return;
    case TO_WHILE:      visitor->visitWhileStatement(static_cast<WhileStatement *>(this)); return;
    case TO_DOWHILE:    visitor->visitDoWhileStatement(static_cast<DoWhileStatement *>(this)); return;
    case TO_FOR:        visitor->visitForStatement(static_cast<ForStatement *>(this)); return;
    case TO_FOREACH:    visitor->visitForeachStatement(static_cast<ForeachStatement *>(this)); return;
    case TO_SWITCH:     visitor->visitSwitchStatement(static_cast<SwitchStatement *>(this)); return;
    case TO_RETURN:     visitor->visitReturnStatement(static_cast<ReturnStatement *>(this)); return;
    case TO_YIELD:      visitor->visitYieldStatement(static_cast<YieldStatement *>(this)); return;
    case TO_THROW:      visitor->visitThrowStatement(static_cast<ThrowStatement *>(this)); return;
    case TO_TRY:        visitor->visitTryStatement(static_cast<TryStatement *>(this)); return;
    case TO_BREAK:      visitor->visitBreakStatement(static_cast<BreakStatement *>(this)); return;
    case TO_CONTINUE:   visitor->visitContinueStatement(static_cast<ContinueStatement *>(this)); return;
    case TO_EXPR_STMT:  visitor->visitExprStatement(static_cast<ExprStatement *>(this)); return;
    case TO_EMPTY:      visitor->visitEmptyStatement(static_cast<EmptyStatement *>(this)); return;
        //case TO_STATEMENT_MARK:
    case TO_ID:         visitor->visitId(static_cast<Id *>(this)); return;
    case TO_COMMA:      visitor->visitCommaExpr(static_cast<CommaExpr *>(this)); return;
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
        visitor->visitBinExpr(static_cast<BinExpr *>(this)); return;
    case TO_NOT:
    case TO_BNOT:
    case TO_NEG:
    case TO_TYPEOF:
    case TO_RESUME:
    case TO_CLONE:
    case TO_PAREN:
    case TO_DELETE:
    case TO_STATIC_MEMO:
    case TO_INLINE_CONST:
        visitor->visitUnExpr(static_cast<UnExpr *>(this)); return;
    case TO_CODE_BLOCK_EXPR:
        visitor->visitCodeBlockExpr(static_cast<CodeBlockExpr *>(this)); return;
    case TO_LITERAL:
        visitor->visitLiteralExpr(static_cast<LiteralExpr *>(this)); return;
    case TO_BASE:
        visitor->visitBaseExpr(static_cast<BaseExpr *>(this)); return;
    case TO_ROOT_TABLE_ACCESS:
        visitor->visitRootTableAccessExpr(static_cast<RootTableAccessExpr *>(this)); return;
    case TO_INC:
        visitor->visitIncExpr(static_cast<IncExpr *>(this)); return;
    case TO_ARRAY:
        visitor->visitArrayExpr(static_cast<ArrayExpr *>(this)); return;
    case TO_TABLE:
        visitor->visitTableExpr(static_cast<TableExpr *>(this)); return;
    case TO_CLASS:
        visitor->visitClassExpr(static_cast<ClassExpr *>(this)); return;
    case TO_FUNCTION:
        visitor->visitFunctionExpr(static_cast<FunctionExpr *>(this)); return;
    case TO_GETFIELD:
        visitor->visitGetFieldExpr(static_cast<GetFieldExpr *>(this)); return;
    case TO_SETFIELD:
        visitor->visitSetFieldExpr(static_cast<SetFieldExpr *>(this)); return;
    case TO_GETSLOT:
        visitor->visitGetSlotExpr(static_cast<GetSlotExpr *>(this)); return;
    case TO_SETSLOT:
        visitor->visitSetSlotExpr(static_cast<SetSlotExpr *>(this)); return;
    case TO_CALL:
        visitor->visitCallExpr(static_cast<CallExpr *>(this)); return;
    case TO_TERNARY:
        visitor->visitTerExpr(static_cast<TerExpr *>(this)); return;
        //case TO_EXPR_MARK:
    case TO_VAR:
        visitor->visitVarDecl(static_cast<VarDecl *>(this)); return;
    case TO_PARAM:
        visitor->visitParamDecl(static_cast<ParamDecl *>(this)); return;
    case TO_CONST:
        visitor->visitConstDecl(static_cast<ConstDecl *>(this)); return;
    case TO_DECL_GROUP:
        visitor->visitDeclGroup(static_cast<DeclGroup *>(this)); return;
    case TO_DESTRUCTURE:
        visitor->visitDestructuringDecl(static_cast<DestructuringDecl  *>(this)); return;
    case TO_ENUM:
        visitor->visitEnumDecl(static_cast<EnumDecl *>(this)); return;
    case TO_DIRECTIVE:
        visitor->visitDirectiveStatement(static_cast<DirectiveStmt *>(this)); return;
    case TO_IMPORT:
        visitor->visitImportStatement(static_cast<ImportStmt *>(this)); return;
    default:
        break;
    }
}

template<typename T>
Node *Node::transform(T *transformer) {
  switch (op())
  {
  case TO_BLOCK:      return transformer->transformBlock(static_cast<Block *>(this));
  case TO_IF:         return transformer->transformIfStatement(static_cast<IfStatement *>(this));
  case TO_WHILE:      return transformer->transformWhileStatement(static_cast<WhileStatement *>(this));
  case TO_DOWHILE:    return transformer->transformDoWhileStatement(static_cast<DoWhileStatement *>(this));
  case TO_FOR:        return transformer->transformForStatement(static_cast<ForStatement *>(this));
  case TO_FOREACH:    return transformer->transformForeachStatement(static_cast<ForeachStatement *>(this));
  case TO_SWITCH:     return transformer->transformSwitchStatement(static_cast<SwitchStatement *>(this));
  case TO_RETURN:     return transformer->transformReturnStatement(static_cast<ReturnStatement *>(this));
  case TO_YIELD:      return transformer->transformYieldStatement(static_cast<YieldStatement *>(this));
  case TO_THROW:      return transformer->transformThrowStatement(static_cast<ThrowStatement *>(this));
  case TO_TRY:        return transformer->transformTryStatement(static_cast<TryStatement *>(this));
  case TO_BREAK:      return transformer->transformBreakStatement(static_cast<BreakStatement *>(this));
  case TO_CONTINUE:   return transformer->transformContinueStatement(static_cast<ContinueStatement *>(this));
  case TO_EXPR_STMT:  return transformer->transformExprStatement(static_cast<ExprStatement *>(this));
  case TO_EMPTY:      return transformer->transformEmptyStatement(static_cast<EmptyStatement *>(this));
    //case TO_STATEMENT_MARK:
  case TO_ID:         return transformer->transformId(static_cast<Id *>(this));
  case TO_COMMA:      return transformer->transformCommaExpr(static_cast<CommaExpr *>(this));
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
    return transformer->transformBinExpr(static_cast<BinExpr *>(this));
  case TO_NOT:
  case TO_BNOT:
  case TO_NEG:
  case TO_TYPEOF:
  case TO_RESUME:
  case TO_CLONE:
  case TO_PAREN:
  case TO_DELETE:
  case TO_STATIC_MEMO:
  case TO_INLINE_CONST:
    return transformer->transformUnExpr(static_cast<UnExpr *>(this));
  case TO_CODE_BLOCK_EXPR:
    return transformer->transformCodeBlockExpr(static_cast<CodeBlockExpr *>(this));
  case TO_LITERAL:
    return transformer->transformLiteralExpr(static_cast<LiteralExpr *>(this));
  case TO_BASE:
    return transformer->transformBaseExpr(static_cast<BaseExpr *>(this));
  case TO_ROOT_TABLE_ACCESS:
    return transformer->transformRootTableAccessExpr(static_cast<RootTableAccessExpr *>(this));
  case TO_INC:
    return transformer->transformIncExpr(static_cast<IncExpr *>(this));
  case TO_ARRAY:
    return transformer->transformArrayExpr(static_cast<ArrayExpr *>(this));
  case TO_TABLE:
    return transformer->transformTableExpr(static_cast<TableExpr *>(this));
  case TO_CLASS:
    return transformer->transformClassExpr(static_cast<ClassExpr *>(this));
  case TO_FUNCTION:
    return transformer->transformFunctionExpr(static_cast<FunctionExpr *>(this));
  case TO_GETFIELD:
    return transformer->transformGetFieldExpr(static_cast<GetFieldExpr *>(this));
  case TO_SETFIELD:
    return transformer->transformSetFieldExpr(static_cast<SetFieldExpr *>(this));
  case TO_GETSLOT:
    return transformer->transformGetSlotExpr(static_cast<GetSlotExpr *>(this));
  case TO_SETSLOT:
    return transformer->transformSetSlotExpr(static_cast<SetSlotExpr *>(this));
  case TO_CALL:
    return transformer->transformCallExpr(static_cast<CallExpr *>(this));
  case TO_TERNARY:
    return transformer->transformTerExpr(static_cast<TerExpr *>(this));
    //case TO_EXPR_MARK:
  case TO_VAR:
    return transformer->transformVarDecl(static_cast<VarDecl *>(this));
  case TO_PARAM:
    return transformer->transformParamDecl(static_cast<ParamDecl *>(this));
  case TO_CONST:
    return transformer->transformConstDecl(static_cast<ConstDecl *>(this));
  case TO_DECL_GROUP:
    return transformer->transformDeclGroup(static_cast<DeclGroup *>(this));
  case TO_DESTRUCTURE:
    return transformer->transformDestructuringDecl(static_cast<DestructuringDecl  *>(this));
  case TO_ENUM:
    return transformer->transformEnumDecl(static_cast<EnumDecl *>(this));
  case TO_DIRECTIVE:
    return this; //-V1037
  case TO_IMPORT:
    return this; //-V1037
  default:
    assert(0 && "Unknown tree type");
    return this;
  }
}

} // namespace SQCompilation
