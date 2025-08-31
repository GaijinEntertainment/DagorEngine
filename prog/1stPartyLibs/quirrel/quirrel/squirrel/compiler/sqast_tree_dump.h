#pragma once

#include <iostream>
#include "sqast.h"

namespace SQCompilation {

class TreeDumpVisitor : public Visitor {

    void indent(int ind) {
        for (int i = 0; i < ind; ++i) _out << "  ";
    }

public:
    TreeDumpVisitor(std::ostream &output) : _out(output), _indent(0) {}

    std::ostream &_out;
    SQInteger _indent;

    virtual void visitNode(Node *node) override {
        TreeOp op = node->op();
        indent(_indent);
        _out << sq_tree_op_names[op];
        const char *opStr = treeopStr(op);
        if (opStr)
            _out << " (" << opStr << ")";

        if (op == TO_ID) {
            _out << " [" << static_cast<Id *>(node)->name() << "]";
        }
        else if (op == TO_VAR) {
            VarDecl *varDecl = static_cast<VarDecl *>(node);
            _out << " [" << (varDecl->isAssignable() ? "local " : "let ") << varDecl->name() << "]";
        }
        else if (op == TO_GETFIELD || op == TO_SETFIELD) {
            FieldAccessExpr *accessExpr = static_cast<FieldAccessExpr *>(node);
            _out << " [" << (accessExpr->isNullable() ? "?" : "") << accessExpr->fieldName() << "]";
        }
        else if (op == TO_LITERAL) {
            LiteralExpr *literal = static_cast<LiteralExpr *>(node);
            LiteralKind kind = literal->kind();
            _out << " [";
            switch (kind) {
                case LK_STRING: _out << '"' << literal->s() << '"'; break;
                case LK_INT: _out << literal->i(); break;
                case LK_FLOAT: _out << literal->f(); break;
                case LK_BOOL: _out << (literal->b() ? "true" : "false"); break;
                case LK_NULL: _out << "null"; break;
                default: _out << "???"; break;
            }
            _out << "]";
        }
        else if (op == TO_PARAM) {
            ParamDecl *paramDecl = static_cast<ParamDecl *>(node);
            _out << " [" << paramDecl->name() << "]";
        }


        _out << " @(" << node->lineStart() << ":" << node->lineEnd() << " - " << node->columnStart() << ":" << node->columnEnd() << ")";
        _out << std::endl;

        ++_indent;
        node->visitChildren(this);
        --_indent;
    }
};

inline std::ostream& operator << (std::ostream &output, const Node *node) {
  if (node) {
    TreeDumpVisitor v(output);
    const_cast<Node*>(node)->visit(&v);
  }
  else output << "(null)" << std::endl;
  return output;
}

} // namespace SQCompilation
