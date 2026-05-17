#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    // ---- annotation ----

    void Annotation::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
    }

    // ---- annotation declaration ----

    void AnnotationDeclaration::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
        if ( annotation ) annotation->gc_collect(target, from);
    }

    // ---- base classes ----

    void Expression::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
        if ( type ) type->gc_collect(target, from);
    }

    void Variable::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
        if ( type ) type->gc_collect(target, from);
        if ( init ) init->gc_collect(target, from);
        if ( source ) source->gc_collect(target, from);
        // note: loop_source is a weak pointer to ExprFor::sources[i], not owned
    }

    void Enumeration::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
        for ( auto & val : list ) {
            if ( val.value ) val.value->gc_collect(target, from);
        }
        for ( auto & ann : annotations ) if ( ann ) ann->gc_collect(target, from);
    }

    void Structure::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
        for ( auto & field : fields ) {
            if ( field.type ) field.type->gc_collect(target, from);
            if ( field.init ) field.init->gc_collect(target, from);
        }
        aliases.foreach([&](auto td) {
            if ( td ) td->gc_collect(target, from);
        });
        if ( parent ) parent->gc_collect(target, from);
        for ( auto & ann : annotations ) if ( ann ) ann->gc_collect(target, from);
    }

    void Function::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
        if ( result ) result->gc_collect(target, from);
        for ( auto & arg : arguments ) if ( arg ) arg->gc_collect(target, from);
        if ( body ) body->gc_collect(target, from);
        for ( auto & ann : annotations ) if ( ann ) ann->gc_collect(target, from);
    }

    // ---- leaf expressions (no extra fields beyond Expression) ----

    void ExprReader::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
    }

    void ExprLabel::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
    }

    void ExprBreak::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
    }

    void ExprContinue::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
    }

    void ExprFakeContext::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
    }

    void ExprFakeLineInfo::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
    }

    void ExprConstEnumeration::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
    }

    void ExprConstString::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
    }

    // ---- single subexpr ----

    void ExprGoto::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
    }

    void ExprRef2Value::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
    }

    void ExprRef2Ptr::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
    }

    void ExprPtr2Ref::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
    }

    void ExprYield::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
    }

    void ExprMakeBlock::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( block ) block->gc_collect(target, from);
    }

    void ExprUnsafe::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( body ) body->gc_collect(target, from);
    }

    // ---- ExprAddr: TypeDeclPtr only ----

    void ExprAddr::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( funcType ) funcType->gc_collect(target, from);
    }

    // ---- two subexprs ----

    // ExprNullCoalescing : ExprPtr2Ref
    void ExprNullCoalescing::gc_collect ( gc_root * target, gc_root * from ) {
        ExprPtr2Ref::gc_collect(target, from);
        if ( defaultValue ) defaultValue->gc_collect(target, from);
    }

    void ExprDelete::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( sizeexpr ) sizeexpr->gc_collect(target, from);
    }

    void ExprAt::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( index ) index->gc_collect(target, from);
    }

    // ExprSafeAt : ExprAt
    void ExprSafeAt::gc_collect ( gc_root * target, gc_root * from ) {
        ExprAt::gc_collect(target, from);
    }

    void ExprTryCatch::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( try_block ) try_block->gc_collect(target, from);
        if ( catch_block ) catch_block->gc_collect(target, from);
    }

    void ExprWhile::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( cond ) cond->gc_collect(target, from);
        if ( body ) body->gc_collect(target, from);
    }

    void ExprWith::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( with ) with->gc_collect(target, from);
        if ( body ) body->gc_collect(target, from);
    }

    // ---- ExprTag: subexpr + value ----

    void ExprTag::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( value ) value->gc_collect(target, from);
    }

    // ---- three subexprs ----

    void ExprIfThenElse::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( cond ) cond->gc_collect(target, from);
        if ( if_true ) if_true->gc_collect(target, from);
        if ( if_false ) if_false->gc_collect(target, from);
    }

    void ExprArrayComprehension::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( exprFor ) exprFor->gc_collect(target, from);
        if ( exprWhere ) exprWhere->gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
    }

    // ---- field access: ExprField and descendants ----

    void ExprField::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( value ) value->gc_collect(target, from);
        if ( annotation ) annotation->gc_collect(target, from);
    }

    // ExprIsVariant : ExprField
    void ExprIsVariant::gc_collect ( gc_root * target, gc_root * from ) {
        ExprField::gc_collect(target, from);
    }

    // ExprAsVariant : ExprField
    void ExprAsVariant::gc_collect ( gc_root * target, gc_root * from ) {
        ExprField::gc_collect(target, from);
    }

    // ExprSafeAsVariant : ExprField
    void ExprSafeAsVariant::gc_collect ( gc_root * target, gc_root * from ) {
        ExprField::gc_collect(target, from);
    }

    // ExprSafeField : ExprField
    void ExprSafeField::gc_collect ( gc_root * target, gc_root * from ) {
        ExprField::gc_collect(target, from);
    }

    // ExprSwizzle : Expression (separate from ExprField)
    void ExprSwizzle::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( value ) value->gc_collect(target, from);
    }

    // ---- ExpressionPtr + TypeDeclPtr combos ----

    void ExprReturn::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( returnType ) returnType->gc_collect(target, from);
    }

    void ExprConstPtr::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( ptrType ) ptrType->gc_collect(target, from);
    }

    void ExprConstBitfield::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( bitfieldType ) bitfieldType->gc_collect(target, from);
    }

    void ExprAssume::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( assumeType ) assumeType->gc_collect(target, from);
    }

    void ExprTypeInfo::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( typeexpr ) typeexpr->gc_collect(target, from);
    }

    void ExprIs::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( typeexpr ) typeexpr->gc_collect(target, from);
    }

    void ExprAscend::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( ascType ) ascType->gc_collect(target, from);
    }

    void ExprCast::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( castType ) castType->gc_collect(target, from);
    }

    void ExprTypeDecl::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( typeexpr ) typeexpr->gc_collect(target, from);
    }

    // ---- ExprLooksLikeCall and descendants ----

    void ExprLooksLikeCall::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        for ( auto & arg : arguments ) if ( arg ) arg->gc_collect(target, from);
        if ( aliasSubstitution ) aliasSubstitution->gc_collect(target, from);
    }

    // ExprCallMacro : ExprLooksLikeCall
    void ExprCallMacro::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprCallFunc : ExprLooksLikeCall
    void ExprCallFunc::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprOp : ExprCallFunc
    void ExprOp::gc_collect ( gc_root * target, gc_root * from ) {
        ExprCallFunc::gc_collect(target, from);
    }

    // ExprOp1 : ExprOp
    void ExprOp1::gc_collect ( gc_root * target, gc_root * from ) {
        ExprOp::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
    }

    // ExprOp2 : ExprOp
    void ExprOp2::gc_collect ( gc_root * target, gc_root * from ) {
        ExprOp::gc_collect(target, from);
        if ( left ) left->gc_collect(target, from);
        if ( right ) right->gc_collect(target, from);
    }

    // ExprCopy : ExprOp2
    void ExprCopy::gc_collect ( gc_root * target, gc_root * from ) {
        ExprOp2::gc_collect(target, from);
    }

    // ExprMove : ExprOp2
    void ExprMove::gc_collect ( gc_root * target, gc_root * from ) {
        ExprOp2::gc_collect(target, from);
    }

    // ExprClone : ExprOp2
    void ExprClone::gc_collect ( gc_root * target, gc_root * from ) {
        ExprOp2::gc_collect(target, from);
    }

    // ExprSequence : ExprOp2
    void ExprSequence::gc_collect ( gc_root * target, gc_root * from ) {
        ExprOp2::gc_collect(target, from);
    }

    // ExprOp3 : ExprOp
    void ExprOp3::gc_collect ( gc_root * target, gc_root * from ) {
        ExprOp::gc_collect(target, from);
        if ( subexpr ) subexpr->gc_collect(target, from);
        if ( left ) left->gc_collect(target, from);
        if ( right ) right->gc_collect(target, from);
    }

    // ExprNew : ExprCallFunc (inherits arguments from ExprLooksLikeCall)
    void ExprNew::gc_collect ( gc_root * target, gc_root * from ) {
        ExprCallFunc::gc_collect(target, from);
        if ( typeexpr ) typeexpr->gc_collect(target, from);
    }

    // ExprCall : ExprCallFunc
    void ExprCall::gc_collect ( gc_root * target, gc_root * from ) {
        ExprCallFunc::gc_collect(target, from);
    }

    // ExprInvoke : ExprLikeCall<ExprInvoke> : ExprLooksLikeCall
    void ExprInvoke::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprAssert : ExprLikeCall<ExprAssert> : ExprLooksLikeCall
    void ExprAssert::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprQuote : ExprLikeCall<ExprQuote> : ExprLooksLikeCall
    void ExprQuote::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprStaticAssert : ExprLikeCall<ExprStaticAssert> : ExprLooksLikeCall
    void ExprStaticAssert::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprDebug : ExprLikeCall<ExprDebug> : ExprLooksLikeCall
    void ExprDebug::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprMemZero : ExprLikeCall<ExprMemZero> : ExprLooksLikeCall
    void ExprMemZero::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprErase : ExprLikeCall<ExprErase> : ExprLooksLikeCall
    void ExprErase::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprFind : ExprLikeCall<ExprFind> : ExprLooksLikeCall
    void ExprFind::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprKeyExists : ExprLikeCall<ExprKeyExists> : ExprLooksLikeCall
    void ExprKeyExists::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprSetInsert : ExprLikeCall<ExprSetInsert> : ExprLooksLikeCall
    void ExprSetInsert::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
    }

    // ExprMakeGenerator : ExprLooksLikeCall
    void ExprMakeGenerator::gc_collect ( gc_root * target, gc_root * from ) {
        ExprLooksLikeCall::gc_collect(target, from);
        if ( iterType ) iterType->gc_collect(target, from);
    }

    // ---- ExprBlock (complex) ----

    void ExprBlock::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        for ( auto & e : list ) if ( e ) e->gc_collect(target, from);
        for ( auto & e : finalList ) if ( e ) e->gc_collect(target, from);
        if ( returnType ) returnType->gc_collect(target, from);
        for ( auto & arg : arguments ) if ( arg ) arg->gc_collect(target, from);
        for ( auto & ann : annotations ) if ( ann ) ann->gc_collect(target, from);
    }

    // ---- ExprStringBuilder ----

    void ExprStringBuilder::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        for ( auto & e : elements ) if ( e ) e->gc_collect(target, from);
    }

    // ---- ExprLet ----

    void ExprLet::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        for ( auto & v : variables ) if ( v ) v->gc_collect(target, from);
    }

    // ---- ExprFor ----

    void ExprFor::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( body ) body->gc_collect(target, from);
        for ( auto & e : sources ) if ( e ) e->gc_collect(target, from);
        for ( auto & e : iteratorsTags ) if ( e ) e->gc_collect(target, from);
        for ( auto & v : iteratorVariables ) if ( v ) v->gc_collect(target, from);
    }

    // ---- ExprVar ----

    void ExprVar::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( variable ) variable->gc_collect(target, from);
    }

    // ---- MakeFieldDecl / MakeStruct (gc_node) ----

    void MakeFieldDecl::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
        if ( value ) value->gc_collect(target, from);
        if ( tag ) tag->gc_collect(target, from);
    }

    void MakeStruct::gc_collect ( gc_root * target, gc_root * from ) {
        if ( gc_owner == target ) return;
        if ( from && gc_owner != from ) return;
        if ( !from && gc_owner == nullptr ) return;
        gc_assign(target);
        for ( auto & field : *this ) {
            if ( field ) field->gc_collect(target, from);
        }
    }

    // ---- ExprNamedCall : Expression (NOT ExprLooksLikeCall) ----

    void ExprNamedCall::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        for ( auto & e : nonNamedArguments ) if ( e ) e->gc_collect(target, from);
        if ( arguments ) arguments->gc_collect(target, from);
    }

    // ---- ExprMakeLocal and descendants ----

    void ExprMakeLocal::gc_collect ( gc_root * target, gc_root * from ) {
        Expression::gc_collect(target, from);
        if ( makeType ) makeType->gc_collect(target, from);
    }

    // ExprMakeStruct : ExprMakeLocal
    void ExprMakeStruct::gc_collect ( gc_root * target, gc_root * from ) {
        ExprMakeLocal::gc_collect(target, from);
        if ( block ) block->gc_collect(target, from);
        for ( auto & ms : structs ) {
            if ( ms ) ms->gc_collect(target, from);
        }
    }

    // ExprMakeVariant : ExprMakeLocal
    void ExprMakeVariant::gc_collect ( gc_root * target, gc_root * from ) {
        ExprMakeLocal::gc_collect(target, from);
        for ( auto & field : variants ) {
            if ( field ) field->gc_collect(target, from);
        }
    }

    // ExprMakeArray : ExprMakeLocal
    void ExprMakeArray::gc_collect ( gc_root * target, gc_root * from ) {
        ExprMakeLocal::gc_collect(target, from);
        if ( recordType ) recordType->gc_collect(target, from);
        for ( auto & e : values ) if ( e ) e->gc_collect(target, from);
    }

    // ExprMakeTuple : ExprMakeArray
    void ExprMakeTuple::gc_collect ( gc_root * target, gc_root * from ) {
        ExprMakeArray::gc_collect(target, from);
    }

}
