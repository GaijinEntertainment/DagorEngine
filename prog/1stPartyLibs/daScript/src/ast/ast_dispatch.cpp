#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

// Per-class virtual shim for SerializeVisitor-style dispatch:
// static-type-aware single call to the matching Visitor::preVisit(ExprXxx*) overload,
// with no child walk and no post-visit. Abstract intermediate classes and templates
// without a matching preVisit overload fall through to preVisitExpression.

namespace das
{

    void Expression::dispatch ( Visitor & vis ) {
        vis.preVisitExpression(this);
    }

    // concrete Expression subclasses -> preVisit(ExprXxx*)
    void ExprReader::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprLabel::dispatch             ( Visitor & vis ) { vis.preVisit(this); }
    void ExprGoto::dispatch              ( Visitor & vis ) { vis.preVisit(this); }
    void ExprRef2Value::dispatch         ( Visitor & vis ) { vis.preVisit(this); }
    void ExprRef2Ptr::dispatch           ( Visitor & vis ) { vis.preVisit(this); }
    void ExprPtr2Ref::dispatch           ( Visitor & vis ) { vis.preVisit(this); }
    void ExprAddr::dispatch              ( Visitor & vis ) { vis.preVisit(this); }
    void ExprNullCoalescing::dispatch    ( Visitor & vis ) { vis.preVisit(this); }
    void ExprDelete::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprAt::dispatch                ( Visitor & vis ) { vis.preVisit(this); }
    void ExprSafeAt::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprBlock::dispatch             ( Visitor & vis ) { vis.preVisit(this); }
    void ExprVar::dispatch               ( Visitor & vis ) { vis.preVisit(this); }
    void ExprTag::dispatch               ( Visitor & vis ) { vis.preVisit(this); }
    void ExprField::dispatch             ( Visitor & vis ) { vis.preVisit(this); }
    void ExprSafeAsVariant::dispatch     ( Visitor & vis ) { vis.preVisit(this); }
    void ExprSwizzle::dispatch           ( Visitor & vis ) { vis.preVisit(this); }
    void ExprSafeField::dispatch         ( Visitor & vis ) { vis.preVisit(this); }
    void ExprLooksLikeCall::dispatch     ( Visitor & vis ) { vis.preVisit(this); }
    void ExprCallMacro::dispatch         ( Visitor & vis ) { vis.preVisit(this); }
    void ExprOp1::dispatch               ( Visitor & vis ) { vis.preVisit(this); }
    void ExprOp2::dispatch               ( Visitor & vis ) { vis.preVisit(this); }
    void ExprCopy::dispatch              ( Visitor & vis ) { vis.preVisit(this); }
    void ExprMove::dispatch              ( Visitor & vis ) { vis.preVisit(this); }
    void ExprClone::dispatch             ( Visitor & vis ) { vis.preVisit(this); }
    void ExprOp3::dispatch               ( Visitor & vis ) { vis.preVisit(this); }
    void ExprTryCatch::dispatch          ( Visitor & vis ) { vis.preVisit(this); }
    void ExprReturn::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprConstPtr::dispatch          ( Visitor & vis ) { vis.preVisit(this); }
    void ExprConstEnumeration::dispatch  ( Visitor & vis ) { vis.preVisit(this); }
    void ExprConstBitfield::dispatch     ( Visitor & vis ) { vis.preVisit(this); }
    void ExprConstString::dispatch       ( Visitor & vis ) { vis.preVisit(this); }
    void ExprStringBuilder::dispatch     ( Visitor & vis ) { vis.preVisit(this); }
    void ExprLet::dispatch               ( Visitor & vis ) { vis.preVisit(this); }
    void ExprFor::dispatch               ( Visitor & vis ) { vis.preVisit(this); }
    void ExprUnsafe::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprWhile::dispatch             ( Visitor & vis ) { vis.preVisit(this); }
    void ExprWith::dispatch              ( Visitor & vis ) { vis.preVisit(this); }
    void ExprAssume::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprMakeBlock::dispatch         ( Visitor & vis ) { vis.preVisit(this); }
    void ExprMakeGenerator::dispatch     ( Visitor & vis ) { vis.preVisit(this); }
    void ExprYield::dispatch             ( Visitor & vis ) { vis.preVisit(this); }
    void ExprInvoke::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprAssert::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprQuote::dispatch             ( Visitor & vis ) { vis.preVisit(this); }
    void ExprTypeInfo::dispatch          ( Visitor & vis ) { vis.preVisit(this); }
    void ExprIs::dispatch                ( Visitor & vis ) { vis.preVisit(this); }
    void ExprAscend::dispatch            ( Visitor & vis ) { vis.preVisit(this); }
    void ExprCast::dispatch              ( Visitor & vis ) { vis.preVisit(this); }
    void ExprNew::dispatch               ( Visitor & vis ) { vis.preVisit(this); }
    void ExprCall::dispatch              ( Visitor & vis ) { vis.preVisit(this); }
    void ExprIfThenElse::dispatch        ( Visitor & vis ) { vis.preVisit(this); }
    void ExprNamedCall::dispatch         ( Visitor & vis ) { vis.preVisit(this); }
    void ExprMakeStruct::dispatch        ( Visitor & vis ) { vis.preVisit(this); }
    void ExprMakeVariant::dispatch       ( Visitor & vis ) { vis.preVisit(this); }
    void ExprMakeArray::dispatch         ( Visitor & vis ) { vis.preVisit(this); }
    void ExprMakeTuple::dispatch         ( Visitor & vis ) { vis.preVisit(this); }
    void ExprArrayComprehension::dispatch( Visitor & vis ) { vis.preVisit(this); }
    void ExprTypeDecl::dispatch          ( Visitor & vis ) { vis.preVisit(this); }

    // abstract intermediate classes -> route to nearest concrete preVisit overload
    void ExprOp::dispatch                ( Visitor & vis ) { vis.preVisit(static_cast<ExprLooksLikeCall*>(this)); }
    void ExprCallFunc::dispatch          ( Visitor & vis ) { vis.preVisit(static_cast<ExprLooksLikeCall*>(this)); }
    void ExprMakeLocal::dispatch         ( Visitor & vis ) { vis.preVisitExpression(this); }

    void ExprConst::dispatch             ( Visitor & vis ) { vis.preVisit(this); }

    // ExprTableKeysOrValues / ExprArrayCallWithSizeOrIndex dispatch live in
    // ast_visitor.h alongside their visit() templates so every TU that
    // instantiates a specialization picks up the definition.

}
