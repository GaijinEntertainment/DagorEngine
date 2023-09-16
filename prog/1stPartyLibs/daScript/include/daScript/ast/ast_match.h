#pragma once

#include "daScript/ast/ast.h"

namespace das {

    // recognize a==0, a!=0, 0==a, 0!=a
    bool matchEquNequZero ( const ExpressionPtr & expr, ExpressionPtr & zeroCond, bool & condIfZero );

    bool isZeroConst ( const ExpressionPtr & expr );
    bool isFloatConst ( const ExpressionPtr & expr, float value );
    bool isIntConst ( const ExpressionPtr & expr, int64_t value );
    bool isUIntConst ( const ExpressionPtr & expr, uint64_t value );
    bool isIntOrUIntConst ( const ExpressionPtr & expr, int64_t value );
    bool isPtrZero ( const ExpressionPtr & expr );
}
