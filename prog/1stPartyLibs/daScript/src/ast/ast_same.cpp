#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"

namespace das {

    // ===== Expression::sameAs =====
    // Structural equality over VALUE expressions: true means the two subtrees are the
    // same computation over the same operands - same variables (by Variable pointer),
    // fields, indices, constants (bit-exact), and the same resolved functions. It does
    // NOT by itself mean two evaluations yield the same value; a caller folding
    // `E op E` must additionally require E->noSideEffects. Write-flagged occurrences
    // (lvalues) never compare equal, and any node class not explicitly handled below
    // conservatively compares unequal.

    static bool sameExpr ( const Expression * a, const Expression * b ) {
        if ( a==b ) return a!=nullptr;
        if ( !a || !b ) return false;
        // exact-class gate: __rtti is stamped per concrete class in every constructor,
        // so subclasses (ExprCopy under ExprOp2, ExprSafeAt under ExprAt, ...) can never
        // be conflated with their base or with each other
        if ( !a->__rtti || !b->__rtti || strcmp(a->__rtti,b->__rtti)!=0 ) return false;
        if ( a->rtti_isCopy() || a->rtti_isClone() || a->rtti_isSequence() ) return false;   // statements, not values
        if ( a->rtti_isConstant() ) {
            auto ca = static_cast<const ExprConst *>(a);
            auto cb = static_cast<const ExprConst *>(b);
            if ( ca->baseType != cb->baseType ) return false;
            if ( a->rtti_isStringConstant() ) {
                return static_cast<const ExprConstString *>(a)->text == static_cast<const ExprConstString *>(b)->text;
            }
            if ( ca->baseType==Type::tEnumeration ) {
                auto ea = static_cast<const ExprConstEnumeration *>(a);
                auto eb = static_cast<const ExprConstEnumeration *>(b);
                return ea->enumType==eb->enumType && ea->text==eb->text;
            }
            return memcmp(&ca->value,&cb->value,sizeof(vec4f))==0;   // bit-exact, incl. FP
        } else if ( a->rtti_isVar() ) {
            auto va = static_cast<const ExprVar *>(a);
            auto vb = static_cast<const ExprVar *>(b);
            return va->variable && va->variable==vb->variable
                && !va->write && !vb->write && va->r2v==vb->r2v;
        } else if ( a->rtti_isField() || a->rtti_isSafeField()
                 || a->rtti_isIsVariant() || a->rtti_isAsVariant() || a->rtti_isSafeAsVariant() ) {
            auto fa = static_cast<const ExprField *>(a);
            auto fb = static_cast<const ExprField *>(b);
            return !fa->write && !fb->write && fa->r2v==fb->r2v
                && fa->name==fb->name && sameExpr(fa->value, fb->value);
        } else if ( a->rtti_isSwizzle() ) {
            auto sa = static_cast<const ExprSwizzle *>(a);
            auto sb = static_cast<const ExprSwizzle *>(b);
            return !sa->write && !sb->write && sa->r2v==sb->r2v
                && sa->fields==sb->fields && sameExpr(sa->value, sb->value);
        } else if ( a->rtti_isAt() || a->rtti_isSafeAt() ) {
            auto aa = static_cast<const ExprAt *>(a);
            auto ab = static_cast<const ExprAt *>(b);
            return !aa->write && !ab->write && aa->r2v==ab->r2v
                && sameExpr(aa->subexpr, ab->subexpr) && sameExpr(aa->index, ab->index);
        } else if ( a->rtti_isNullCoalescing() ) {   // before isPtr2Ref: subclass keeps the base rtti
            auto na = static_cast<const ExprNullCoalescing *>(a);
            auto nb = static_cast<const ExprNullCoalescing *>(b);
            return sameExpr(na->subexpr, nb->subexpr) && sameExpr(na->defaultValue, nb->defaultValue);
        } else if ( a->rtti_isR2V() ) {
            return sameExpr(static_cast<const ExprRef2Value *>(a)->subexpr,
                            static_cast<const ExprRef2Value *>(b)->subexpr);
        } else if ( a->rtti_isRef2Ptr() ) {
            return sameExpr(static_cast<const ExprRef2Ptr *>(a)->subexpr,
                            static_cast<const ExprRef2Ptr *>(b)->subexpr);
        } else if ( a->rtti_isPtr2Ref() ) {
            return sameExpr(static_cast<const ExprPtr2Ref *>(a)->subexpr,
                            static_cast<const ExprPtr2Ref *>(b)->subexpr);
        } else if ( a->rtti_isCast() ) {
            auto ca = static_cast<const ExprCast *>(a);
            auto cb = static_cast<const ExprCast *>(b);
            return ca->castFlags==cb->castFlags
                && ca->castType && cb->castType && ca->castType->isSameExactType(*cb->castType)
                && sameExpr(ca->subexpr, cb->subexpr);
        } else if ( a->rtti_isOp1() ) {
            auto oa = static_cast<const ExprOp1 *>(a);
            auto ob = static_cast<const ExprOp1 *>(b);
            return oa->op==ob->op && oa->func==ob->func && sameExpr(oa->subexpr, ob->subexpr);
        } else if ( a->rtti_isOp2() ) {
            auto oa = static_cast<const ExprOp2 *>(a);
            auto ob = static_cast<const ExprOp2 *>(b);
            return oa->op==ob->op && oa->func==ob->func
                && sameExpr(oa->left, ob->left) && sameExpr(oa->right, ob->right);
        } else if ( a->rtti_isOp3() ) {
            auto oa = static_cast<const ExprOp3 *>(a);
            auto ob = static_cast<const ExprOp3 *>(b);
            return oa->op==ob->op && oa->func==ob->func && sameExpr(oa->subexpr, ob->subexpr)
                && sameExpr(oa->left, ob->left) && sameExpr(oa->right, ob->right);
        } else if ( a->rtti_isCall() ) {
            auto ca = static_cast<const ExprCall *>(a);
            auto cb = static_cast<const ExprCall *>(b);
            if ( !ca->func || ca->func!=cb->func ) return false;
            if ( ca->arguments.size()!=cb->arguments.size() ) return false;
            for ( size_t i=0, is=ca->arguments.size(); i!=is; ++i ) {
                if ( !sameExpr(ca->arguments[i], cb->arguments[i]) ) return false;
            }
            return true;
        } else if ( a->rtti_isAddr() ) {
            return static_cast<const ExprAddr *>(a)->func
                && static_cast<const ExprAddr *>(a)->func==static_cast<const ExprAddr *>(b)->func;
        }
        return false;   // unrecognized class - conservatively unequal
    }

    bool Expression::sameAs ( const Expression * other ) const {
        return sameExpr(this, other);
    }

}
