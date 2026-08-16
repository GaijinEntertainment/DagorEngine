#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"

namespace das {

    // ===== shared alias model for the CSE / DSE passes =====
    // A tracked CHAIN is a Field/At/Swizzle access path over a plain (non-ref) variable
    // base: struct/tuple/bitfield fields and array/fixed-array/vector indexing only.
    // Pointer or handled-type links, variant access, and table indexing (which inserts on
    // read) fall outside the model. Soundness rests on the access flags stamped by
    // buildAccessFlags/TrackVariableFlags (ast_unused.cpp), which run at the top of every
    // optimization round (optimizationUnused precedes CSE/DSE in ast_optimize.cpp):
    //  * every write stamps `write` on the full access chain, including the base ExprVar -
    //    statement stores, op-assign, addr-of, move sources, non-const for sources, and
    //    call sites of argument-modifying callees (propagateModifiedArguments) alike;
    //  * every read-through stamps `r2cr`, so `r2v | (r2cr && !write)` identifies an
    //    occurrence that cannot modify the variable.
    // A pass can therefore KILL facts about a base at statements that visibly store
    // through it, and treat any other write-flagged chain root (pointer paths, globals,
    // loop variables, block arguments) as OPAQUE - discarding every memory-derived fact.

    struct AliasChain {
        Variable *              base = nullptr;
        ExprVar *               baseVar = nullptr;
        vector<const char *>    path;       // field names, base-side first; nullptr marks an At/Swizzle link
        vector<Expression *>    indices;    // At index subtrees (not part of the spine)
        bool                    exact = true;       // no At/Swizzle links anywhere in the chain
        bool                    anyWrite = false;   // some link carries a write flag
    };

    // peel Field/At/Swizzle/R2V links down to a variable base. returns false when a link
    // is outside the model; `spine` (when given) collects every node consumed by the walk
    // so a top-down scanner can avoid re-classifying interior links
    inline bool aliasChainOf ( Expression * e, AliasChain & out, das_hash_set<Expression *> * spine = nullptr ) {
        vector<const char *> rpath;         // outermost first, reversed at the end
        vector<Expression *> seen;
        for ( ;; ) {
            seen.push_back(e);
            if ( e->rtti_isR2V() ) {
                e = static_cast<ExprRef2Value *>(e)->subexpr;
            } else if ( e->rtti_isField() ) {   // exact class: safe-field and variant forms excluded
                auto f = static_cast<ExprField *>(e);
                if ( f->annotation ) return false;      // handled-type field or property
                auto & vt = f->value->type;
                if ( !vt || vt->isPointer() ) return false;
                if ( !(vt->baseType==Type::tStructure || vt->baseType==Type::tTuple
                    || vt->baseType==Type::tBitfield) ) return false;
                out.anyWrite |= f->write;
                rpath.push_back(f->name.c_str());
                e = f->value;
            } else if ( e->rtti_isAt() ) {      // exact class: safe-at excluded
                auto a = static_cast<ExprAt *>(e);
                auto & vt = a->subexpr->type;
                if ( !vt ) return false;
                if ( !(vt->baseType==Type::tArray || vt->baseType==Type::tFixedArray
                    || vt->isVectorType()) ) return false;
                out.anyWrite |= a->write;
                out.exact = false;
                out.indices.push_back(a->index);
                rpath.push_back(nullptr);
                e = a->subexpr;
            } else if ( e->rtti_isSwizzle() ) {
                auto s = static_cast<ExprSwizzle *>(e);
                out.anyWrite |= s->write;
                out.exact = false;
                rpath.push_back(nullptr);
                e = s->value;
            } else if ( e->rtti_isVar() ) {
                auto v = static_cast<ExprVar *>(e);
                if ( !v->variable ) return false;
                if ( v->variable->type && v->variable->type->ref ) return false;    // ref local: aliases elsewhere
                out.anyWrite |= v->write;
                out.baseVar = v;
                out.base = v->variable;
                // dagor: das::vector maps to dag::Vector, whose iterator assign casts to a raw
                // pointer, so a reverse_iterator range does not compile. Copy in reverse by hand.
                out.path.clear();
                out.path.reserve(rpath.size());
                for ( size_t i=rpath.size(); i!=0; --i ) out.path.push_back(rpath[i-1]);
                if ( spine ) for ( auto n : seen ) spine->insert(n);
                return true;
            } else {
                return false;
            }
        }
    }

    // base-side prefix length known exactly (stops at the first At/Swizzle link)
    inline size_t aliasChainExactLen ( const vector<const char *> & path ) {
        for ( size_t i=0, is=path.size(); i!=is; ++i ) {
            if ( !path[i] ) return i;
        }
        return path.size();
    }

    // paths agree on their first n links (caller keeps n within both exact prefixes)
    inline bool aliasPathAgree ( const vector<const char *> & a, const vector<const char *> & b, size_t n ) {
        for ( size_t i=0; i!=n; ++i ) {
            if ( strcmp(a[i],b[i])!=0 ) return false;
        }
        return true;
    }
}
