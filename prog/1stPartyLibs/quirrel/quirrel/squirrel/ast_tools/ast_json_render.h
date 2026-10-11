#pragma once
#include <cstdio>
#include <cstring>
#include <string>
#include <squirrel.h>
#include <compiler/ast.h>
#include <compiler/compilationcontext.h>
#include <compiler/sqtypeparser.h>

namespace SQCompilation {

// Writes the AST of a script as one line of JSON. ast_json.md next to this file states the document
// shape. A field whose value is its default (false, null, the type any, an empty doc) is left out.
class JsonTreeRenderer : public Visitor {
    SQStreamWriteFunc _write;
    void *_ud;
    bool needComma;
    static constexpr size_t BUFFER_SIZE = 8192;
    char *_buffer;
    size_t _bufferSize = 0;

    void flush() {
        if (_bufferSize) {
            _buffer[_bufferSize] = 0;
            _write(_buffer, _ud);
            _bufferSize = 0;
        }
    }

    void put(const char *s) {
        size_t len = strlen(s);
        while (len) {
            size_t available = BUFFER_SIZE - _bufferSize;
            size_t n = len < available ? len : available;
            memcpy(_buffer + _bufferSize, s, n);
            _bufferSize += n;
            s += n;
            len -= n;
            if (_bufferSize == BUFFER_SIZE)
                flush();
        }
    }

    void sep() {
        if (needComma)
            put(",");
        needComma = false;
    }

    void open(const char *bracket) { sep(); put(bracket); }
    void close(const char *bracket) { put(bracket); needComma = true; }

    // Length of the well-formed UTF-8 sequence at p (2..4), or 0.
    static int utf8SequenceLength(const unsigned char *p) {
        int len = (p[0] & 0xE0) == 0xC0 ? 2 : (p[0] & 0xF0) == 0xE0 ? 3 : (p[0] & 0xF8) == 0xF0 ? 4 : 0;
        for (int i = 1; i < len; ++i)
            if ((p[i] & 0xC0) != 0x80)
                return 0;
        if (len == 0)
            return 0;
        unsigned cp = len == 2 ? (p[0] & 0x1Fu) << 6 | (p[1] & 0x3Fu)
                    : len == 3 ? (p[0] & 0x0Fu) << 12 | (p[1] & 0x3Fu) << 6 | (p[2] & 0x3Fu)
                    : (p[0] & 0x07u) << 18 | (p[1] & 0x3Fu) << 12 | (p[2] & 0x3Fu) << 6 | (p[3] & 0x3Fu);
        unsigned minCp = len == 2 ? 0x80u : len == 3 ? 0x800u : 0x10000u;
        if (cp < minCp || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            return 0;
        return len;
    }

    void putEscaped(const char *s, size_t size) {
        char buf[256];
        size_t n = 0;
        put("\"");
        for (const unsigned char *p = (const unsigned char *)s, *e = p + size; p < e; ++p) {
            if (n + 8 > sizeof(buf)) {
                buf[n] = 0;
                put(buf);
                n = 0;
            }
            switch (*p) {
                case '"': buf[n++] = '\\'; buf[n++] = '"'; break;
                case '\\': buf[n++] = '\\'; buf[n++] = '\\'; break;
                case '\n': buf[n++] = '\\'; buf[n++] = 'n'; break;
                case '\r': buf[n++] = '\\'; buf[n++] = 'r'; break;
                case '\t': buf[n++] = '\\'; buf[n++] = 't'; break;
                default: {
                    int len = *p < 0x80 ? 1 : utf8SequenceLength(p);
                    if (len > (int)(e - p))
                        len = 0;
                    if (*p < 0x20 || len == 0)
                        n += snprintf(buf + n, 8, "\\u%04x", (unsigned)*p);
                    else {
                        for (int i = 0; i < len; ++i)
                            buf[n++] = (char)p[i];
                        p += len - 1;
                    }
                    break;
                }
            }
        }
        buf[n] = 0;
        put(buf);
        put("\"");
    }

    void key(const char *k) {
        sep();
        put("\"");
        put(k);
        put("\":");
    }

    void str(const char *s, size_t size) {
        sep();
        putEscaped(s, size);
        needComma = true;
    }

    void str(const char *s) {
        sep();
        if (s)
            putEscaped(s, strlen(s));
        else
            put("null");
        needComma = true;
    }

    void integer(long long v) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%lld", v);
        sep();
        put(buf);
        needComma = true;
    }

    void boolean(bool v) { sep(); put(v ? "true" : "false"); needComma = true; }

    void floating(SQFloat v) {
        char buf[64];
        snprintf(buf, sizeof(buf), sizeof(SQFloat) == sizeof(float) ? "%.9g" : "%.17g", (double)v);
        // the separator of the C locale is what JSON takes, whatever the host set
        for (char *c = buf; *c; ++c)
            if (*c == ',')
                *c = '.';
        // an integral value still reads as a float
        if (!strpbrk(buf, ".e"))
            memcpy(buf + strlen(buf), ".0", 3);
        sep();
        put(buf);
        needComma = true;
    }

    // a default value is left out
    void field(const char *k, const char *s) { if (s) { key(k); str(s); } }
    void field(const char *k, bool v) { if (v) { key(k); boolean(true); } }
    void field(const char *k, long long v) { key(k); integer(v); }

    void typeField(const char *k, unsigned mask) {
        if (mask == ~0u)
            return;
        char buf[256];
        sq_stringify_type_mask(buf, sizeof(buf), mask);
        field(k, buf);
    }

    void writeNode(Node *n) {
        if (n)
            n->visit(this);
        else
            str(nullptr);
    }

    void node(const char *k, Node *n) {
        if (n) {
            key(k);
            n->visit(this);
        }
    }

    template <typename Vec> void nodes(const char *k, const Vec &list) {
        key(k);
        open("[");
        for (auto *n : list)
            writeNode(n);
        close("]");
    }

    void span(const char *k, int32_t l1, int32_t c1, int32_t l2, int32_t c2) {
        char buf[96];
        snprintf(buf, sizeof(buf), "[%d,%d,%d,%d]", (int)l1, (int)c1, (int)l2, (int)c2);
        key(k);
        put(buf);
        needComma = true;
    }

    void begin(const char *type, const Node *n) {
        const SourceSpan s = n->sourceSpan();
        open("{");
        field("type", type);
        span("span", s.start.line, s.start.column, s.end.line, s.end.column);
    }

    void end() { close("}"); }

    void members(const TableExpr *tbl) {
        key("members");
        open("[");
        for (const TableMember &m : tbl->members()) {
            open("{");
            node("key", m.key);
            node("value", m.value);
            field("static", m.isStatic());
            field("dynamicKey", m.isDynamicKey());
            field("json", m.isJson());
            close("}");
        }
        close("]");
        doc(tbl->docObject);
    }

    // @@"" is a docstring too, and it is a default like a missing one
    void doc(const DocObject &d) {
        if (!d.isEmpty() && *d.getDocString())
            field("doc", d.getDocString());
    }

    void literalValue(const LiteralExpr *lit) {
        switch (lit->kind()) {
            case LK_STRING: field("kind", "string"); key("value"); str(lit->s()); break;
            case LK_INT: field("kind", "int"); key("value"); integer((long long)lit->i()); break;
            case LK_BOOL: field("kind", "bool"); key("value"); boolean(lit->b()); break;
            case LK_NULL: field("kind", "null"); key("value"); str(nullptr); break;
            case LK_FLOAT: field("kind", "float"); key("value"); floating(lit->f()); break;
        }
    }

    // the operators treeopStr() does not spell
    static const char *opName(TreeOp op) {
        switch (op) {
        case TO_IN: return "in";
        case TO_INSTANCEOF: return "instanceof";
        case TO_TYPEOF: return "typeof";
        case TO_RESUME: return "resume";
        case TO_AWAIT: return "await";
        case TO_CLONE: return "clone";
        case TO_DELETE: return "delete";
        case TO_INLINE_CONST: return "const";
        case TO_SPREAD: return "...";
        case TO_PAREN: return "()";
        default: {
            const char *s = treeopStr(op);
            return s ? s : "?";
        }
        }
    }

    // The lexer stores a comment without its opening delimiter, one piece per source line, and the
    // last piece of a block comment ends with the '*' of its closing delimiter. A CR before a line
    // break is dropped from the text and the span, so a CRLF file dumps the same as its LF copy.
    void comments(const Comments *list) {
        key("comments");
        open("[");
        std::string text;
        int line = 0, startLine = 0, startColumn = 0;
        bool inBlock = false;
        for (const Comments::LineCommentsList &lineComments : list->commentsList()) {
            ++line;
            for (const CommentData &c : lineComments) {
                bool endsLine = c.size > 0 && c.text[c.size - 1] == '\n';
                bool crlf = endsLine && c.size > 1 && c.text[c.size - 2] == '\r';
                if (!inBlock) {
                    startLine = line;
                    startColumn = (int)c.begin - (c.kind == CK_LINE ? 1 : 2);
                    text.clear();
                }
                text.append(c.text, c.size - (crlf ? 2 : 0));
                if (crlf)
                    text.push_back('\n');
                if (c.kind != CK_LINE && endsLine) {
                    inBlock = true;
                    continue;
                }
                inBlock = false;
                if (c.kind == CK_LINE) {
                    while (!text.empty() && (text.back() == '\n' || text.back() == 0))
                        text.pop_back();
                }
                else if (!text.empty() && text.back() == '*')
                    text.pop_back();
                open("{");
                field("kind", c.kind == CK_LINE ? "line" : "block");
                span("span", startLine, startColumn, line, (int)c.end - (crlf ? 1 : 0));
                key("text");
                str(text.c_str(), text.size());
                close("}");
            }
        }
        close("]");
    }

public:
    JsonTreeRenderer(SQStreamWriteFunc write, void *ud)
        : _write(write), _ud(ud), needComma(false), _buffer(new char[BUFFER_SIZE + 1]) {}
    ~JsonTreeRenderer() { delete[] _buffer; }
    JsonTreeRenderer(const JsonTreeRenderer &) = delete;
    JsonTreeRenderer &operator=(const JsonTreeRenderer &) = delete;

    void render(Node *root, const char *sourceName, const Comments *comments_) {
        open("{");
        field("file", sourceName);
        key("ast");
        writeNode(root);
        if (comments_)
            comments(comments_);
        put("}\n");
        flush();
        needComma = false;
    }

    void visitNode(Node *n) override {
        TreeOp op = n->op();
        switch (op) {
            case TO_ID:
                begin("Id", n);
                field("name", static_cast<Id *>(n)->name());
                break;

            case TO_LITERAL:
                begin("LiteralExpr", n);
                literalValue(static_cast<LiteralExpr *>(n));
                break;

            case TO_NOT: case TO_BNOT: case TO_NEG: case TO_TYPEOF: case TO_RESUME: case TO_AWAIT: case TO_CLONE:
            case TO_PAREN: case TO_DELETE: case TO_SPREAD: case TO_INLINE_CONST:
                begin("UnExpr", n);
                field("op", opName(op));
                node("arg", static_cast<UnExpr *>(n)->argument());
                break;

            case TO_NULLC: case TO_ASSIGN: case TO_OROR: case TO_ANDAND: case TO_OR: case TO_XOR: case TO_AND:
            case TO_NE: case TO_EQ: case TO_3CMP: case TO_GE: case TO_GT: case TO_LE: case TO_LT: case TO_IN:
            case TO_INSTANCEOF: case TO_USHR: case TO_SHR: case TO_SHL: case TO_MUL: case TO_DIV: case TO_MOD:
            case TO_ADD: case TO_SUB: case TO_NEWSLOT: case TO_PLUSEQ: case TO_MINUSEQ: case TO_MULEQ:
            case TO_DIVEQ: case TO_MODEQ: {
                BinExpr *e = static_cast<BinExpr *>(n);
                begin("BinExpr", n);
                field("op", opName(op));
                node("lhs", e->lhs());
                node("rhs", e->rhs());
                break;
            }

            case TO_TERNARY: {
                TerExpr *e = static_cast<TerExpr *>(n);
                begin("TerExpr", n);
                node("cond", e->a());
                node("then", e->b());
                node("else", e->c());
                break;
            }

            case TO_CODE_BLOCK_EXPR:
                begin("CodeBlockExpr", n);
                node("block", static_cast<CodeBlockExpr *>(n)->block());
                break;

            case TO_CALL: {
                CallExpr *e = static_cast<CallExpr *>(n);
                begin("CallExpr", n);
                field("nullable", e->isNullable());
                node("callee", e->callee());
                nodes("args", e->arguments());
                break;
            }

            case TO_GETFIELD: {
                GetFieldExpr *e = static_cast<GetFieldExpr *>(n);
                begin("GetFieldExpr", n);
                field("nullable", e->isNullable());
                field("typeMethod", e->isTypeMethod());
                field("field", e->fieldName());
                node("receiver", e->receiver());
                break;
            }

            case TO_GETSLOT: {
                GetSlotExpr *e = static_cast<GetSlotExpr *>(n);
                begin("GetSlotExpr", n);
                field("nullable", e->isNullable());
                node("receiver", e->receiver());
                node("key", e->key());
                break;
            }

            case TO_INC: {
                IncExpr *e = static_cast<IncExpr *>(n);
                begin("IncExpr", n);
                field("prefix", e->form() == IF_PREFIX);
                field("diff", (long long)e->diff());
                node("arg", e->argument());
                break;
            }

            case TO_ARRAY:
                begin("ArrayExpr", n);
                nodes("elements", static_cast<ArrayExpr *>(n)->initializers());
                break;

            case TO_COMMA:
                begin("CommaExpr", n);
                nodes("exprs", static_cast<CommaExpr *>(n)->expressions());
                break;

            case TO_TABLE:
                begin("TableExpr", n);
                members(static_cast<TableExpr *>(n));
                break;

            case TO_CLASS: {
                ClassExpr *cls = static_cast<ClassExpr *>(n);
                begin("ClassExpr", n);
                node("key", cls->classKey());
                node("base", cls->classBase());
                members(cls);
                break;
            }

            case TO_FUNCTION: {
                FunctionExpr *fn = static_cast<FunctionExpr *>(n);
                begin("FunctionExpr", n);
                field("name", fn->name());
                field("lambda", fn->isLambda());
                field("pure", fn->isPure());
                field("nodiscard", fn->isNodiscard());
                field("async", fn->isAsync());
                field("vararg", fn->isVararg());
                typeField("resultType", fn->getResultTypeMask());
                doc(fn->docObject);
                nodes("params", fn->parameters());
                node("body", fn->body());
                break;
            }

            case TO_PARAM: {
                ParamDecl *p = static_cast<ParamDecl *>(n);
                begin("ParamDecl", n);
                field("name", p->name());
                field("vararg", p->isVararg());
                typeField("valueType", p->getTypeMask());
                node("default", p->defaultValue());
                node("destructuring", p->getDestructuring());
                break;
            }

            case TO_VAR: {
                VarDecl *d = static_cast<VarDecl *>(n);
                begin("VarDecl", n);
                field("name", d->name());
                field("assignable", d->isAssignable());
                field("destructured", d->isDestructured());
                typeField("valueType", d->getTypeMask());
                node("init", d->initializer());
                break;
            }

            case TO_CONST: {
                ConstDecl *d = static_cast<ConstDecl *>(n);
                begin("ConstDecl", n);
                field("name", d->name());
                field("global", d->isGlobal());
                node("value", d->value());
                break;
            }

            case TO_ENUM: {
                EnumDecl *d = static_cast<EnumDecl *>(n);
                begin("EnumDecl", n);
                field("name", d->name());
                field("global", d->isGlobal());
                key("consts");
                open("[");
                for (const EnumConst &c : d->consts()) {
                    open("{");
                    field("name", c.id);
                    node("value", c.val);
                    close("}");
                }
                close("]");
                break;
            }

            case TO_DECL_GROUP:
                begin("DeclGroup", n);
                nodes("decls", static_cast<DeclGroup *>(n)->declarations());
                break;

            case TO_DESTRUCTURE: {
                DestructuringDecl *d = static_cast<DestructuringDecl *>(n);
                begin("DestructuringDecl", n);
                field("kind", d->type() == DT_ARRAY ? "array" : "table");
                nodes("decls", d->declarations());
                node("init", d->initExpression());
                break;
            }

            case TO_BLOCK: {
                Block *b = static_cast<Block *>(n);
                begin("Block", n);
                field("root", b->isRoot());
                field("body", b->isBody());
                nodes("statements", b->statements());
                break;
            }

            case TO_IF: {
                IfStatement *s = static_cast<IfStatement *>(n);
                begin("IfStatement", n);
                node("cond", s->condition());
                node("then", s->thenBranch());
                node("else", s->elseBranch());
                break;
            }

            case TO_WHILE: case TO_DOWHILE: {
                begin(op == TO_WHILE ? "WhileStatement" : "DoWhileStatement", n);
                if (op == TO_WHILE)
                    node("cond", static_cast<WhileStatement *>(n)->condition());
                else
                    node("cond", static_cast<DoWhileStatement *>(n)->condition());
                node("body", static_cast<LoopStatement *>(n)->body());
                break;
            }

            case TO_FOR: {
                ForStatement *s = static_cast<ForStatement *>(n);
                begin("ForStatement", n);
                node("init", s->initializer());
                node("cond", s->condition());
                node("modifier", s->modifier());
                node("body", s->body());
                break;
            }

            case TO_FOREACH: {
                ForeachStatement *s = static_cast<ForeachStatement *>(n);
                begin("ForeachStatement", n);
                node("index", s->idx());
                node("value", s->val());
                node("container", s->container());
                node("body", s->body());
                break;
            }

            case TO_SWITCH: {
                SwitchStatement *s = static_cast<SwitchStatement *>(n);
                begin("SwitchStatement", n);
                node("expr", s->expression());
                key("cases");
                open("[");
                for (const SwitchCase &c : s->cases()) {
                    open("{");
                    node("value", c.val);
                    node("body", c.stmt);
                    close("}");
                }
                close("]");
                node("default", s->defaultCase().stmt);
                break;
            }

            case TO_TRY: {
                TryStatement *s = static_cast<TryStatement *>(n);
                begin("TryStatement", n);
                node("body", s->tryStatement());
                key("catches");
                open("[");
                for (const CatchClause &c : s->catches()) {
                    open("{");
                    node("exceptionType", c.type);
                    node("exception", c.exception);
                    node("body", c.body);
                    close("}");
                }
                close("]");
                break;
            }

            case TO_RETURN: case TO_YIELD: case TO_THROW:
                begin(op == TO_RETURN ? "ReturnStatement" : op == TO_YIELD ? "YieldStatement" : "ThrowStatement", n);
                node("arg", static_cast<TerminateStatement *>(n)->argument());
                break;

            case TO_BREAK: begin("BreakStatement", n); break;
            case TO_CONTINUE: begin("ContinueStatement", n); break;
            case TO_EMPTY: begin("EmptyStatement", n); break;
            case TO_BASE: begin("BaseExpr", n); break;
            case TO_ROOT_TABLE_ACCESS: begin("RootTableAccessExpr", n); break;

            case TO_EXPR_STMT:
                begin("ExprStatement", n);
                node("expr", static_cast<ExprStatement *>(n)->expression());
                break;

            case TO_DIRECTIVE: {
                DirectiveStmt *d = static_cast<DirectiveStmt *>(n);
                begin("DirectiveStmt", n);
                field("name", d->name);
                field("applyToDefault", d->applyToDefault);
                break;
            }

            case TO_IMPORT: {
                ImportStmt *imp = static_cast<ImportStmt *>(n);
                begin("ImportStmt", n);
                field("module", imp->moduleName);
                field("alias", imp->moduleAlias);
                key("slots");
                open("[");
                for (const SQModuleImportSlot &slot : imp->slots) {
                    open("{");
                    field("name", slot.name);
                    field("alias", slot.alias);
                    field("line", (long long)slot.line);
                    field("column", (long long)slot.column);
                    close("}");
                }
                close("]");
                break;
            }

            default:
                // a node kind this renderer does not know yet still reaches a tool under its name
                begin(sq_tree_op_names[op], n);
                break;
        }
        end();
    }
};

} // namespace SQCompilation
