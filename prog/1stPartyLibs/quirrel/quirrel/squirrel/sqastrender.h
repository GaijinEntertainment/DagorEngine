#pragma once

#include "sqastio.h"

namespace SQCompilation {

class RenderVisitor : public Visitor {


    const char* treeopToStr(enum TreeOp op) {
        switch (op)
        {
        case TO_NULLC: return " ?? ";
        case TO_ASSIGN: return " = ";
        case TO_OROR: return " || ";
        case TO_ANDAND: return " && ";
        case TO_OR: return " | ";
        case TO_XOR: return " ^ ";
        case TO_AND: return " & ";
        case TO_NE: return " != ";
        case TO_EQ: return " == ";
        case TO_3CMP: return " <=> ";
        case TO_GE: return " >= ";
        case TO_GT: return " > ";
        case TO_LE: return " <= ";
        case TO_LT: return " < ";
        case TO_IN: return " IN ";
        case TO_INSTANCEOF: return " INSTANCEOF ";
        case TO_USHR: return " >>> ";
        case TO_SHR: return " >> ";
        case TO_SHL: return " << ";
        case TO_MUL: return " * ";
        case TO_DIV: return " / ";
        case TO_MOD: return " % ";
        case TO_ADD: return " + ";
        case TO_SUB: return " - "; //-V1037
        case TO_NOT: return " ! ";
        case TO_BNOT: return " ~ ";
        case TO_NEG: return " - "; //-V1037
        case TO_TYPEOF: return "TYPEOF ";
        case TO_RESUME: return "RESUME ";
        case TO_CLONE: return "CLONE ";
        case TO_DELETE: return "DELETE ";
        case TO_INEXPR_ASSIGN: return " := ";
        case TO_NEWSLOT: return " <- ";
        case TO_PLUSEQ: return " += ";
        case TO_MINUSEQ: return " -= ";
        case TO_MULEQ: return " *= ";
        case TO_DIVEQ: return " /= ";
        case TO_MODEQ: return " %= ";
        default: return "<UNKNOWN>";
        }
    }

    void indent(int ind) {
        for (int i = 0; i < ind; ++i) _out->writeInt8(' ');
    }

    void writeInteger(uint64_t v) {
      char buf[128] = { 0 };
      snprintf(buf, sizeof buf, "%llu", (long long unsigned int)v);
      _out->writeString(buf);
    }

    void writeFloat(SQFloat f) {
      char buf[128] = { 0 };
      snprintf(buf, sizeof buf, "%f", f);
      _out->writeString(buf);
    }

    void newLine() {
      _out->writeChar('\n');
    }

public:
    RenderVisitor(OutputStream *output) : _out(output), _indent(0) {}

    OutputStream *_out;
    SQInteger _indent;

    void render(Node *n) {
        n->visit(this);
        newLine();
    }

    virtual void visitUnExpr(UnExpr *expr) {
        if (expr->op() == TO_PAREN) {
            _out->writeChar('(');
        }
        else {
            _out->writeString(treeopToStr(expr->op()));
        }
        expr->argument()->visit(this);
        if (expr->op() == TO_PAREN) {
          _out->writeChar(')');
        }
    }
    virtual void visitBinExpr(BinExpr *expr) {
        expr->_lhs->visit(this);
        _out->writeString(treeopToStr(expr->op()));
        expr->_rhs->visit(this);
    }
    virtual void visitTerExpr(TerExpr *expr) {
        expr->a()->visit(this);
        _out->writeString(" ? ");
        expr->b()->visit(this);
        _out->writeString(" : ");
        expr->c()->visit(this);
    }
    virtual void visitCallExpr(CallExpr *expr) {
        expr->callee()->visit(this);
        _out->writeChar('(');
        for (SQUnsignedInteger i = 0; i < expr->arguments().size(); ++i) {
            if (i) _out->writeString(", ");
            expr->arguments()[i]->visit(this);
        }
        _out->writeChar(')');

    }
    virtual void visitId(Id *id) { _out->writeString(id->id()); }

    virtual void visitGetFieldExpr(GetFieldExpr *expr) {
        expr->receiver()->visit(this);
        if (expr->isNullable()) _out->writeInt8('?');
        _out->writeChar('.');
        _out->writeString(expr->fieldName());
    }
    virtual void visitSetFieldExpr(SetFieldExpr *expr) {
        expr->receiver()->visit(this);
        if (expr->isNullable()) _out->writeChar('?');
        _out->writeChar('.');
        _out->writeString(expr->fieldName());
        _out->writeString(" = ");
        expr->value()->visit(this);
    }
    virtual void visitGetTableExpr(GetTableExpr *expr) {
        expr->receiver()->visit(this);
        if (expr->isNullable()) _out->writeChar('?');
        _out->writeChar('[');
        expr->key()->visit(this);
        _out->writeChar(']');
    }
    virtual void visitSetTableExpr(SetTableExpr *expr) {
        expr->receiver()->visit(this);
        if (expr->isNullable()) _out->writeChar('?');
        _out->writeChar('[');
        expr->key()->visit(this);
        _out->writeChar(']');
        _out->writeString(" = ");
        expr->value()->visit(this);
    }
    virtual void visitBaseExpr(BaseExpr *expr) { _out->writeString("base"); }
    virtual void visitRootExpr(RootExpr *expr) { _out->writeString("::"); }
    virtual void visitLiteralExpr(LiteralExpr *expr) {
        switch (expr->kind())
        {
        case LK_STRING:
          _out->writeChar('"');
          _out->writeString(expr->s());
          _out->writeChar('"');
          break;
        case LK_FLOAT: writeFloat(expr->f()); break;
        case LK_INT: writeInteger(expr->i()); break;
        case LK_BOOL: _out->writeString(expr->b() ? "True" : "False"); break;
        case LK_NULL: _out->writeString("#NULL"); break;
        default: assert(0);
            break;
        };
    }
    virtual void visitIncExpr(IncExpr *expr) {

        const char *op = expr->diff() < 0 ? "--" : "++";

        if (expr->form() == IF_PREFIX) _out->writeString(op);
        expr->argument()->visit(this);
        if (expr->form() == IF_POSTFIX) _out->writeString(op);
    }

    virtual void visitArrayExpr(ArrayExpr *expr) {
        _out->writeChar('[');
        for (SQUnsignedInteger i = 0; i < expr->initialziers().size(); ++i) {
            if (i) _out->writeString(", ");
            expr->initialziers()[i]->visit(this);
        }
        _out->writeChar(']');
    }

    virtual void visitCommaExpr(CommaExpr *expr) {
        for (SQUnsignedInteger i = 0; i < expr->expressions().size(); ++i) {
            if (i) _out->writeString(", ");
            expr->expressions()[i]->visit(this);
        }
    }

    virtual void visitBlock(Block *block) {
        _out->writeString("{\n");
        int cur = _indent;
        _indent += 2;
        ArenaVector<Statement *> &stmts = block->statements();

        for (auto stmt : stmts) {
            indent(_indent);
            stmt->visit(this);
            newLine();
        }

        indent(cur);
        _out->writeChar('}');
        _indent = cur;
    }
    virtual void visitIfStatement(IfStatement *ifstmt) {
        _out->writeString("IF (");
        ifstmt->condition()->visit(this);
        _out->writeString(")\n");
        indent(_indent);
        _out->writeString("THEN ");
        _indent += 2;
        ifstmt->thenBranch()->visit(this);
        if (ifstmt->elseBranch()) {
            newLine();
            indent(_indent - 2);
            _out->writeString("ELSE ");
            ifstmt->elseBranch()->visit(this);
        }
        _indent -= 2;
        newLine();
        indent(_indent);
        _out->writeString("END_IF");

    }
    virtual void visitLoopStatement(LoopStatement *loop) {
        indent(_indent);
        loop->body()->visit(this);
    }
    virtual void visitWhileStatement(WhileStatement *loop) {
      _out->writeString("WHILE (");
        loop->condition()->visit(this);
        _out->writeString(")\n");
        _indent += 2;
        visitLoopStatement(loop);
        newLine();
        _indent -= 2;
        indent(_indent);
        _out->writeString("END_WHILE");

    }
    virtual void visitDoWhileStatement(DoWhileStatement *loop) {
        _out->writeString("DO");

        _indent += 2;
        visitLoopStatement(loop);
        _indent -= 2;

        indent(_indent);
        _out->writeString("WHILE (");
        newLine();
        loop->condition()->visit(this);
        _out->writeChar(')');
    }
    virtual void visitForStatement(ForStatement *loop) {
      _out->writeString("FOR (");

        if (loop->initializer()) loop->initializer()->visit(this);
        _out->writeString("; ");

        if (loop->condition()) loop->condition()->visit(this);
        _out->writeString("; ");

        if (loop->modifier()) loop->modifier()->visit(this);
        _out->writeString(")\n");

        _indent += 2;
        visitLoopStatement(loop);
        _indent -= 2;

        newLine();

        indent(_indent);
        _out->writeString("END_FOR");
        newLine();
    }
    virtual void visitForeachStatement(ForeachStatement *loop) {
      _out->writeString("FOR_EACH ( {");

        if (loop->idx()) {
            loop->idx()->visit(this);
            _out->writeString(", ");
        }

        loop->val()->visit(this);
        _out->writeString("} in ");

        loop->container()->visit(this);
        _out->writeString(")\n");

        _indent += 2;
        visitLoopStatement(loop);
        _indent -= 2;

        newLine();
        indent(_indent);
        _out->writeString("END_FOREACH");
    }
    virtual void visitSwitchStatement(SwitchStatement *swtch) {
      _out->writeString("SWITCH (");
        swtch->expression()->visit(this);
        _out->writeString(")\n");
        int cur = _indent;
        _indent += 2;

        for (auto &c : swtch->cases()) {
            indent(cur);
            _out->writeString("CASE ");
            c.val->visit(this);
            _out->writeString(": ");
            c.stmt->visit(this);
            newLine();
        }

        if (swtch->defaultCase().stmt) {
            indent(cur);
            _out->writeString("DEFAULT: ");
            swtch->defaultCase().stmt->visit(this);
            newLine();
        }

        _indent -= 2;
    }
    virtual void visitTryStatement(TryStatement *tr) {
      _out->writeString("TRY ");
        //_indent += 2;
        tr->tryStatement()->visit(this);
        newLine();
        indent(_indent);
        _out->writeString("CATCH (");
        visitId(tr->exceptionId());
        _out->writeString(") ");
        tr->catchStatement()->visit(this);
        newLine();
        indent(_indent);
        _out->writeString("END_TRY");
    }
    virtual void visitTerminateStatement(TerminateStatement *term) { if (term->argument()) term->argument()->visit(this); }
    virtual void visitReturnStatement(ReturnStatement *ret) {
        _out->writeString("RETURN ");
        visitTerminateStatement(ret);
    }
    virtual void visitYieldStatement(YieldStatement *yld) {
        _out->writeString("YIELD ");
        visitTerminateStatement(yld);
    }
    virtual void visitThrowStatement(ThrowStatement *thr) {
        _out->writeString("THROW ");
        visitTerminateStatement(thr);
    }
    virtual void visitBreakStatement(BreakStatement *jmp) { _out->writeString("BREAK"); }
    virtual void visitContinueStatement(ContinueStatement *jmp) { _out->writeString("CONTINUE"); }
    virtual void visitExprStatement(ExprStatement *estmt) { estmt->expression()->visit(this); }
    virtual void visitEmptyStatement(EmptyStatement *empty) { _out->writeChar(';'); }

    virtual void visitValueDecl(ValueDecl *decl) {
        if (decl->op() == TO_VAR) {
          _out->writeString(((VarDecl *)decl)->isAssignable() ? "local " : "let ");
        }
        _out->writeString(decl->name());
        if (decl->expression()) {
          _out->writeString(" = ");
            decl->expression()->visit(this);
        }
    }
    virtual void visitTableDecl(TableDecl *tbl) {
      _out->writeString("{\n");
        _indent += 2;

        for (auto &m : tbl->members()) {
            indent(_indent);
            if (m.isStatic()) _out->writeString("STATIC ");
            if (m.isDynamicKey()) _out->writeChar('[');
            m.key->visit(this);
            if (m.isDynamicKey()) _out->writeChar(']');
            const char *op = m.isJson() ? " : " : " <- ";
            _out->writeString(op);
            m.value->visit(this);
            newLine();
        }

        _indent -= 2;
        indent(_indent);
        _out->writeChar('}');
    }
    virtual void visitClassDecl(ClassDecl *cls) {
        _out->writeString("CLASS ");
        if (cls->classKey()) cls->classKey()->visit(this);
        if (cls->classBase()) {
            _out->writeString(" EXTENDS ");
            cls->classBase()->visit(this);
            _out->writeChar(' ');
        }
        visitTableDecl(cls);
    }
    virtual void visitFunctionDecl(FunctionDecl *f) {
      _out->writeString("FUNCTION");
        if (f->isLambda()) {
          _out->writeString(" @ ");
        } else if (f->name()) {
            _out->writeChar(' ');
            _out->writeString(f->name());
        }

        _out->writeChar('(');
        for (SQUnsignedInteger i = 0; i < f->parameters().size(); ++i) {
            if (i) _out->writeString(", ");
            visitParamDecl(f->parameters()[i]);
        }

        if (f->isVararg()) {
          _out->writeString("...");
        }
        _out->writeString(") ");

        f->body()->visit(this);
    }
    virtual void visitConstructorDecl(ConstructorDecl *ctr) { visitFunctionDecl(ctr); }
    virtual void visitConstDecl(ConstDecl *cnst) {
        if (cnst->isGlobal()) _out->writeString("G ");
        _out->writeString(cnst->name());
        _out->writeString(" = ");
        cnst->value()->visit(this);
    }
    virtual void visitEnumDecl(EnumDecl *enm) {
        _out->writeString("ENUM ");
        _out->writeString(enm->name());
        newLine();
        _indent += 2;

        for (auto &c : enm->consts()) {
            indent(_indent);
            _out->writeString(c.id);
            _out->writeString(" = ");
            c.val->visit(this);
            newLine();
        }
        _indent -= 2;
        indent(_indent);
        _out->writeString("END_ENUM");
    }

    virtual void visitDeclGroup(DeclGroup *grp) {
        for (SQUnsignedInteger i = 0; i < grp->declarations().size(); ++i) {
            if (i) _out->writeString(", ");
            grp->declarations()[i]->visit(this);
        }
    }

    virtual void visitDestructuringDecl(DestructuringDecl *destruct) {
      _out->writeString("{ ");
        for (SQUnsignedInteger i = 0; i < destruct->declarations().size(); ++i) {
            if (i) _out->writeString(", ");
            destruct->declarations()[i]->visit(this);
        }
        _out->writeString(" } = ");
        destruct->initiExpression()->visit(this);
    }
};

} // namsespace SQCompilation
