options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

module fuzzer shared private

require daslib/faker
require daslib/constant_expression
require dastest/testing_boost

var private total_fuzz_count : int = 1000

def public fuzz ( blk : block )
    //! run block however many times
    //! ignore panic, so that we can see that runtime crashes
    for i in range(total_fuzz_count)
        try
            invoke(blk)
        recover
            pass

def public fuzz ( fuzz_count:int; blk : block )
    //! run block however many times
    //! ignore panic, so that we can see that runtime crashes
    for i in range(fuzz_count)
        try
            invoke(blk)
        recover
            pass

def public fuzz_debug ( blk : block )
    //! run block however many times
    //! do not ignore panic, so that we can see where the runtime fails
    //! this is here so that `fuzz` can be easily replaced with `fuzz_debug` for the purpose of debugging
    for i in range(total_fuzz_count)
        invoke(blk)

def public fuzz_debug ( fuzz_count:int; blk : block )
    //! run block however many times
    //! do not ignore panic, so that we can see where the runtime fails
    //! this is here so that `fuzz` can be easily replaced with `fuzz_debug` for the purpose of debugging
    for i in range(fuzz_count)
        invoke(blk)

[constant_expression(funcname)]
def public fuzz_numeric_and_vector_op1 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes single numeric or vector argument.
    //! arguments are: int, uint, float, double, string, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4
    invoke ( funcname, t, fake |> random_int )
    invoke ( funcname, t, fake |> random_uint )
    invoke ( funcname, t, fake |> random_float )
    invoke ( funcname, t, fake |> random_double )
    invoke ( funcname, t, fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int4 )
    invoke ( funcname, t, fake |> random_uint2 )
    invoke ( funcname, t, fake |> random_uint3 )
    invoke ( funcname, t, fake |> random_uint4 )
    invoke ( funcname, t, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4 )

[constant_expression(funcname)]
def public fuzz_numeric_and_vector_signed_op1 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes single numeric or vector argument.
    //! arguments are: int, uint, float, double, string, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4
    invoke ( funcname, t, fake |> random_int )
    invoke ( funcname, t, fake |> random_float )
    invoke ( funcname, t, fake |> random_double )
    invoke ( funcname, t, fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int4 )
    invoke ( funcname, t, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4 )

[constant_expression(funcname)]
def public fuzz_numeric_op1 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes single numeric or vector argument.
    //! arguments are: int, uint, float, double
    invoke ( funcname, t, fake |> random_int )
    invoke ( funcname, t, fake |> random_uint )
    invoke ( funcname, t, fake |> random_float )
    invoke ( funcname, t, fake |> random_double )

[constant_expression(funcname)]
def public fuzz_numeric_and_storage_op1 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes single numeric or vector argument.
    //! arguments are: int, uint, int8, uint8, int16, uint16, int64, uint64, float, double
    invoke ( funcname, t, fake |> random_int )
    invoke ( funcname, t, fake |> random_uint )
    invoke ( funcname, t, fake |> random_int8 )
    invoke ( funcname, t, fake |> random_uint8 )
    invoke ( funcname, t, fake |> random_int16 )
    invoke ( funcname, t, fake |> random_uint16 )
    invoke ( funcname, t, fake |> random_int64 )
    invoke ( funcname, t, fake |> random_uint64 )
    invoke ( funcname, t, fake |> random_float )
    invoke ( funcname, t, fake |> random_double )

[constant_expression(funcname)]
def public fuzz_all_ints_op1 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes single numeric or vector argument.
    //! arguments are: int, uint, int64, uint64
    invoke ( funcname, t, fake |> random_int )
    invoke ( funcname, t, fake |> random_uint )
    invoke ( funcname, t, fake |> random_int64 )
    invoke ( funcname, t, fake |> random_uint64 )

[constant_expression(funcname)]
def public fuzz_all_unsigned_ints_op1 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes single numeric or vector argument.
    //! arguments are: uint, uint64
    invoke ( funcname, t, fake |> random_uint )
    invoke ( funcname, t, fake |> random_uint64 )

[constant_expression(funcname)]
def public fuzz_float_double_or_float_vec_op1 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes single numeric or vector argument.
    //! arguments are: float, double, float2, float3, float4
    invoke ( funcname, t, fake |> random_float )
    invoke ( funcname, t, fake |> random_double )
    invoke ( funcname, t, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4 )

[constant_expression(funcname)]
def public fuzz_float_or_float_vec_op1 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes single numeric or vector argument.
    //! arguments are: float, float2, float3, float4
    invoke ( funcname, t, fake |> random_float )
    invoke ( funcname, t, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4 )

[constant_expression(funcname)]
def public fuzz_numeric_and_vector_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes two numeric or vector arguments.
    //! arguments are: int, uint, float, double, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4
    invoke ( funcname, t, fake |> random_int,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_float, fake |> random_float )
    invoke ( funcname, t, fake |> random_double, fake |> random_double )
    invoke ( funcname, t, fake |> random_int2,  fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int3,  fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int4,  fake |> random_int4 )
    invoke ( funcname, t, fake |> random_uint2,  fake |> random_uint2 )
    invoke ( funcname, t, fake |> random_uint3,  fake |> random_uint3 )
    invoke ( funcname, t, fake |> random_uint4,  fake |> random_uint4 )
    invoke ( funcname, t, fake |> random_float2, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4, fake |> random_float4 )

[constant_expression(funcname)]
def public fuzz_numeric_and_vector_op2_no_unint_vec ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes two numeric or vector arguments.
    //! arguments are: int, uint, float, double, int2, int3, int4, float2, float3, float4
    invoke ( funcname, t, fake |> random_int,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_float, fake |> random_float )
    invoke ( funcname, t, fake |> random_double, fake |> random_double )
    invoke ( funcname, t, fake |> random_int2,  fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int3,  fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int4,  fake |> random_int4 )
    invoke ( funcname, t, fake |> random_float2, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4, fake |> random_float4 )


[constant_expression(funcname)]
def public fuzz_numeric_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes two numeric or vector arguments.
    //! arguments are: int, uint, float, double
    invoke ( funcname, t, fake |> random_int,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_float, fake |> random_float )
    invoke ( funcname, t, fake |> random_double, fake |> random_double )

[constant_expression(funcname)]
def public fuzz_compareable_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes two numeric or vector arguments.
    //! arguments are: int, uint, float, double, int64, uint64, string
    invoke ( funcname, t, fake |> random_int,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_int64,  fake |> random_int64 )
    invoke ( funcname, t, fake |> random_uint64, fake |> random_uint64 )
    invoke ( funcname, t, fake |> random_float, fake |> random_float )
    invoke ( funcname, t, fake |> random_double, fake |> random_double )
    invoke ( funcname, t, fake |> any_string, fake |> any_string )

[constant_expression(funcname)]
def public fuzz_eq_neq_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes two numeric or vector arguments.
    //! arguments are: int, uint, int64, uint64, float, double, string, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4
    invoke ( funcname, t, fake |> random_int,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_int64,  fake |> random_int64 )
    invoke ( funcname, t, fake |> random_uint64, fake |> random_uint64 )
    invoke ( funcname, t, fake |> random_float, fake |> random_float )
    invoke ( funcname, t, fake |> random_double, fake |> random_double )
    invoke ( funcname, t, fake |> any_string, fake |> any_string )
    invoke ( funcname, t, fake |> random_int2,  fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int3,  fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int4,  fake |> random_int4 )
    invoke ( funcname, t, fake |> random_uint2,  fake |> random_uint2 )
    invoke ( funcname, t, fake |> random_uint3,  fake |> random_uint3 )
    invoke ( funcname, t, fake |> random_uint4,  fake |> random_uint4 )
    invoke ( funcname, t, fake |> random_float2, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4, fake |> random_float4 )

[constant_expression(funcname)]
def public fuzz_numeric_vec_scal_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes vector and matching scalar on the right
    //! arguments pairs are: int2,int; int3,int; uint2,uint; uint3,uint; uint4,uint; int4,int; float2,float; float3,float; float4,float
    invoke ( funcname, t, fake |> random_int2,  fake |> random_int )
    invoke ( funcname, t, fake |> random_int3,  fake |> random_int )
    invoke ( funcname, t, fake |> random_int4,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint2,  fake |> random_uint )
    invoke ( funcname, t, fake |> random_uint3,  fake |> random_uint )
    invoke ( funcname, t, fake |> random_uint4,  fake |> random_uint )
    invoke ( funcname, t, fake |> random_float2, fake |> random_float )
    invoke ( funcname, t, fake |> random_float3, fake |> random_float )
    invoke ( funcname, t, fake |> random_float4, fake |> random_float )

[constant_expression(funcname)]
def public fuzz_numeric_scal_vec_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes vector and matching scalar on the left
    //! arguments pairs are: int2,int; int3,int; uint2,uint; uint3,uint; uint4,uint; int4,int; float2,float; float3,float; float4,float
    invoke ( funcname, t, fake |> random_int,  fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int,  fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int,  fake |> random_int4 )
    invoke ( funcname, t, fake |> random_uint,  fake |> random_uint2 )
    invoke ( funcname, t, fake |> random_uint,  fake |> random_uint3 )
    invoke ( funcname, t, fake |> random_uint,  fake |> random_uint4 )
    invoke ( funcname, t, fake |> random_float, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float, fake |> random_float4 )

[constant_expression(funcname)]
def public fuzz_int_vector_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes two numeric or vector arguments.
    //! arguments are: int, uint, int2, int3, int4, uint2, uint3, uint4
    invoke ( funcname, t, fake |> random_int,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_int2,  fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int3,  fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int4,  fake |> random_int4 )
    invoke ( funcname, t, fake |> random_uint2,  fake |> random_uint2 )
    invoke ( funcname, t, fake |> random_uint3,  fake |> random_uint3 )
    invoke ( funcname, t, fake |> random_uint4,  fake |> random_uint4 )

[constant_expression(funcname)]
def public fuzz_shift_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes numeric or vector argument, with matching shift type on the right.
    //! arguments are: int, uint, int2, int3, int4, uint2, uint3, uint4
    invoke ( funcname, t, fake |> random_int,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_int2,  fake |> random_int )
    invoke ( funcname, t, fake |> random_int3,  fake |> random_int )
    invoke ( funcname, t, fake |> random_int4,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint2,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint3,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint4,  fake |> random_int )

[constant_expression(funcname)]
def public fuzz_rotate_op2 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes numeric or vector argument, with matching rotate type on the right.
    //! arguments are: int, uint
    invoke ( funcname, t, fake |> random_int,  fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint )

[constant_expression(funcname)]
def public fuzz_numeric_op3 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes three numeric or vector arguments.
    //! arguments are: int, uint, float, double
    invoke ( funcname, t, fake |> random_int, fake |> random_int, fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_float, fake |> random_float, fake |> random_float )
    invoke ( funcname, t, fake |> random_double, fake |> random_double, fake |> random_double )

[constant_expression(funcname)]
def public fuzz_vec_op3 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes three numeric or vector arguments.
    //! arguments are: float2, float3, float4, int2, int3, int4, uint2, uint3, uint4
    invoke ( funcname, t, fake |> random_float2, fake |> random_float2, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3, fake |> random_float3, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4, fake |> random_float4, fake |> random_float4 )
    invoke ( funcname, t, fake |> random_int2, fake |> random_int2, fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int3, fake |> random_int3, fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int4, fake |> random_int4, fake |> random_int4 )
    invoke ( funcname, t, fake |> random_uint2, fake |> random_uint2, fake |> random_uint2 )
    invoke ( funcname, t, fake |> random_uint3, fake |> random_uint3, fake |> random_uint3 )
    invoke ( funcname, t, fake |> random_uint4, fake |> random_uint4, fake |> random_uint4 )

[constant_expression(funcname)]
def public fuzz_vec_mad_op3 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes three numeric or vector arguments.
    //! arguments are: float2, float3, float4, int2, int3, int4, uint2, uint3, uint4 second argument is float, int, uint accordingly
    invoke ( funcname, t, fake |> random_float2, fake |> random_float, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3, fake |> random_float, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4, fake |> random_float, fake |> random_float4 )
    invoke ( funcname, t, fake |> random_int2, fake |> random_int, fake |> random_int2 )
    invoke ( funcname, t, fake |> random_int3, fake |> random_int, fake |> random_int3 )
    invoke ( funcname, t, fake |> random_int4, fake |> random_int, fake |> random_int4 )
    invoke ( funcname, t, fake |> random_uint2, fake |> random_uint, fake |> random_uint2 )
    invoke ( funcname, t, fake |> random_uint3, fake |> random_uint, fake |> random_uint3 )
    invoke ( funcname, t, fake |> random_uint4, fake |> random_uint, fake |> random_uint4 )

[constant_expression(funcname)]
def public fuzz_float_double_or_float_vec_op3 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes three numeric or vector arguments.
    //! arguments are: float, double, float2, float3, float4
    invoke ( funcname, t, fake |> random_float, fake |> random_float, fake |> random_float )
    invoke ( funcname, t, fake |> random_double, fake |> random_double, fake |> random_double )
    invoke ( funcname, t, fake |> random_float2, fake |> random_float2, fake |> random_float2 )
    invoke ( funcname, t, fake |> random_float3, fake |> random_float3, fake |> random_float3 )
    invoke ( funcname, t, fake |> random_float4, fake |> random_float4, fake |> random_float4 )


[constant_expression(funcname)]
def public fuzz_numeric_op4 ( t:T?; var fake:Faker; funcname:string )
    //! fuzzes generic function that takes four numeric or vector arguments.
    //! arguments are: int, uint, float, double
    invoke ( funcname, t, fake |> random_int, fake |> random_int, fake |> random_int, fake |> random_int )
    invoke ( funcname, t, fake |> random_uint, fake |> random_uint, fake |> random_uint, fake |> random_uint )
    invoke ( funcname, t, fake |> random_float, fake |> random_float, fake |> random_float, fake |> random_float )
    invoke ( funcname, t, fake |> random_double, fake |> random_double, fake |> random_double, fake |> random_double )

