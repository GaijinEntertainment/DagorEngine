#pragma once
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "squtils.h"

struct SQFunctionType
{
    SQObjectPtr functionName;
    SQUnsignedInteger32 returnTypeMask;
    SQUnsignedInteger32 objectTypeMask;
    sqvector<SQObjectPtr> argNames;
    sqvector<SQUnsignedInteger32> argTypeMask;
    sqvector<SQObjectPtr> defaultValues; // null|string
    SQInteger requiredArgs; // Not including `this`
    SQUnsignedInteger32 ellipsisArgTypeMask; // 0 if no ellipsis
    bool pure;
    bool nodiscard;

    SQFunctionType(SQSharedState *ss) :
        argNames(ss->_alloc_ctx),
        defaultValues(ss->_alloc_ctx),
        argTypeMask(ss->_alloc_ctx)
    {
        returnTypeMask = ~0u;
        objectTypeMask = ~0u;
        requiredArgs = 0;
        ellipsisArgTypeMask = 0;
        pure = false;
        nodiscard = false;
    }
};

bool sq_parse_function_type_string(SQVM* vm, const char* s, SQFunctionType& res, SQInteger& error_pos, SQObjectPtr& error_string);
SQObjectPtr sq_stringify_function_type(SQVM* vm, const SQFunctionType& ft);
bool sq_type_string_to_mask(const char* type_name, SQUnsignedInteger32& mask, const char*& suggestion);
void sq_stringify_type_mask(char* buffer, int buffer_length, SQUnsignedInteger32 mask);
