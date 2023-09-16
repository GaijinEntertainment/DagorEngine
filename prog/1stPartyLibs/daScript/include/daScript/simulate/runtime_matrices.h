#pragma once

#include "daScript/misc/type_name.h"
#include "daScript/ast/ast_typefactory.h"

namespace das {

    class Context;

    template <typename VecT, int rowC>
    struct Matrix {
        using VecType = VecT;
        enum { Rows = rowC };
        VecT    m[rowC];
    };

    typedef Matrix<float4,4> float4x4;
    typedef Matrix<float3,4> float3x4;
    typedef Matrix<float3,3> float3x3;

    template <typename VecT, int rowC>
    struct typeName<Matrix<VecT,rowC>> {
        static string name() {
            return string("Matrix<") + typeName<VecT>::name() + "," + to_string(rowC) + ">";
        }
    };
}

// use MAKE_TYPE_FACTORY out of namespaces. Some compilers are not happy otherwise

MAKE_EXTERNAL_TYPE_FACTORY(float4x4, das::float4x4)
MAKE_EXTERNAL_TYPE_FACTORY(float3x4, das::float3x4)
MAKE_EXTERNAL_TYPE_FACTORY(float3x3, das::float3x3)

