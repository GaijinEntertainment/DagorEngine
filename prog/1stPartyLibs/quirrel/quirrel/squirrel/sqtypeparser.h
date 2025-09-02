#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "squtils.h"

struct SQFunctionType
{
    SQObjectPtr functionName;
    SQInteger returnTypeMask;
    SQInteger objectTypeMask;
    sqvector<SQObjectPtr> argNames;
    sqvector<SQInteger> argTypeMask;
    sqvector<SQObjectPtr> defaultValues; // null|string
    SQInteger requiredArgs;
    SQInteger ellipsisArgTypeMask; // 0 if no ellipsis
    bool pure;

    SQFunctionType(SQSharedState *ss) :
        argNames(ss->_alloc_ctx),
        defaultValues(ss->_alloc_ctx),
        argTypeMask(ss->_alloc_ctx)
    {
        returnTypeMask = -1;
        objectTypeMask = -1;
        requiredArgs = 0;
        ellipsisArgTypeMask = 0;
        pure = false;
    }
};

bool sq_parse_function_type_string(SQVM* vm, const SQChar* s, SQFunctionType& res, SQInteger& error_pos, SQObjectPtr& error_string);
SQObjectPtr sq_stringify_function_type(SQVM* vm, const SQFunctionType& ft);
