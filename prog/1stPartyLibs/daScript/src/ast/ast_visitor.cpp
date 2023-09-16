#include "daScript/misc/platform.h"

#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_expressions.h"

namespace das {

    struct AstContextVisitor : Visitor {
        AstContextVisitor ( Expression * expr )
            : refExpr(expr) { }
        AstContext astc, astr;
        Expression * refExpr = nullptr;
        int32_t count = 0;
    // expression
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            if ( expr==refExpr ) {
                count ++;
                if ( count==1 ) {
                    astr = astc;
                    astr.valid = true;
                }
            }
        }
    // function
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            astc.func = f;
        }
        virtual FunctionPtr visit ( Function * that ) override {
            // if any of this asserts failed, there is logic error in how we pop
            DAS_ASSERT(astc.loop.size()==0);
            DAS_ASSERT(astc.scopes.size()==0);
            DAS_ASSERT(astc.blocks.size()==0);
            DAS_ASSERT(astc.with.size()==0);
            astc.func.reset();
            return Visitor::visit(that);
        }
    // ExprWith
        virtual void preVisit ( ExprWith * expr ) override {
            Visitor::preVisit(expr);
            astc.with.push_back(expr);
        }
        virtual ExpressionPtr visit ( ExprWith * expr ) override {
            astc.with.pop_back();
            return Visitor::visit(expr);
        }
    // ExprFor
        virtual void preVisit ( ExprFor * expr ) override {
            Visitor::preVisit(expr);
            astc.loop.push_back(expr);
        }
        virtual ExpressionPtr visit ( ExprFor * expr ) override {
            astc.loop.pop_back();
            return Visitor::visit(expr);
        }
    // ExprWhile
        virtual void preVisit ( ExprWhile * expr ) override {
            Visitor::preVisit(expr);
            astc.loop.push_back(expr);
        }
        virtual ExpressionPtr visit ( ExprWhile * expr ) override {
            astc.loop.pop_back();
            return Visitor::visit(expr);
        }
    // ExprBlock
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            if ( block->isClosure ) {
                astc.blocks.push_back(block);
            }
            astc.scopes.push_back(block);
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            astc.scopes.pop_back();
            if ( block->isClosure ) {
                astc.blocks.pop_back();
            }
            return Visitor::visit(block);
        }
    };

    AstContext generateAstContext( const ProgramPtr & prog, Expression * expr ) {
        AstContextVisitor acv(expr);
        prog->visit(acv);
        return acv.count==1 ? acv.astr : AstContext();
    }
}
