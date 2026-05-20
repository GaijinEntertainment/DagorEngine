#pragma once

#include "daScript/ast/ast.h"

namespace das {

    // recognize a==0, a!=0, 0==a, 0!=a
    bool matchEquNequZero ( ExpressionPtr expr, ExpressionPtr & zeroCond, bool & condIfZero );

    bool isZeroConst ( ExpressionPtr expr );
    bool isFloatConst ( ExpressionPtr expr, float value );
    bool isIntConst ( ExpressionPtr expr, int64_t value );
    bool isUIntConst ( ExpressionPtr expr, uint64_t value );
    bool isIntOrUIntConst ( ExpressionPtr expr, int64_t value );
    bool isPtrZero ( ExpressionPtr expr );
}
