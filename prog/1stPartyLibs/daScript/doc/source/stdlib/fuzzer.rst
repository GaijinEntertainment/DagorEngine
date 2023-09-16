
.. _stdlib_fuzzer:

======
Fuzzer
======

.. include:: detail/fuzzer.rst

The FUZZER module implements facilities for the fuzz testing.

The idea behind the fuzz testing is to feed random data to the testing function and see if it crashes.
`panic` is considered a valid behavior, and in fact ignored.
Fuzz tests work really well in combination with the sanitizers (asan, ubsan, etc).

All functions and symbols are in "fuzzer" module, use require to get access to it. ::

    require daslib/fuzzer

++++++++++++
Fuzzer tests
++++++++++++

  *  :ref:`fuzz (blk:block\<\> const) : void <function-_at_fuzzer_c__c_fuzz_C_builtin_>` 
  *  :ref:`fuzz (fuzz_count:int const;blk:block\<\> const) : void <function-_at_fuzzer_c__c_fuzz_Ci_C_builtin_>` 
  *  :ref:`fuzz_debug (blk:block\<\> const) : void <function-_at_fuzzer_c__c_fuzz_debug_C_builtin_>` 
  *  :ref:`fuzz_debug (fuzz_count:int const;blk:block\<\> const) : void <function-_at_fuzzer_c__c_fuzz_debug_Ci_C_builtin_>` 
  *  :ref:`fuzz_numeric_and_vector_op1 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_and_vector_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_and_vector_signed_op1 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_and_vector_signed_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_op1 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_and_storage_op1 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_and_storage_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_all_ints_op1 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_all_ints_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_all_unsigned_ints_op1 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_all_unsigned_ints_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_float_double_or_float_vec_op1 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_float_double_or_float_vec_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_float_or_float_vec_op1 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_float_or_float_vec_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_and_vector_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_and_vector_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_and_vector_op2_no_unint_vec (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_and_vector_op2_no_unint_vec_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_compareable_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_compareable_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_eq_neq_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_eq_neq_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_vec_scal_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_vec_scal_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_scal_vec_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_scal_vec_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_int_vector_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_int_vector_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_shift_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_shift_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_rotate_op2 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_rotate_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_op3 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_op3_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_vec_op3 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_vec_op3_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_vec_mad_op3 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_vec_mad_op3_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_float_double_or_float_vec_op3 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_float_double_or_float_vec_op3_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 
  *  :ref:`fuzz_numeric_op4 (t:testing::T? const;fake:faker::Faker -const;funcname:string const) : void <function-_at_fuzzer_c__c_fuzz_numeric_op4_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs>` 

.. _function-_at_fuzzer_c__c_fuzz_C_builtin_:

.. das:function:: fuzz(blk: block<> const)

+--------+-------------+
+argument+argument type+
+========+=============+
+blk     +block<> const+
+--------+-------------+


run block however many times
ignore panic, so that we can see that runtime crashes

.. _function-_at_fuzzer_c__c_fuzz_Ci_C_builtin_:

.. das:function:: fuzz(fuzz_count: int const; blk: block<> const)

+----------+-------------+
+argument  +argument type+
+==========+=============+
+fuzz_count+int const    +
+----------+-------------+
+blk       +block<> const+
+----------+-------------+


run block however many times
ignore panic, so that we can see that runtime crashes

.. _function-_at_fuzzer_c__c_fuzz_debug_C_builtin_:

.. das:function:: fuzz_debug(blk: block<> const)

+--------+-------------+
+argument+argument type+
+========+=============+
+blk     +block<> const+
+--------+-------------+


run block however many times
do not ignore panic, so that we can see where the runtime fails
this is here so that `fuzz` can be easily replaced with `fuzz_debug` for the purpose of debugging

.. _function-_at_fuzzer_c__c_fuzz_debug_Ci_C_builtin_:

.. das:function:: fuzz_debug(fuzz_count: int const; blk: block<> const)

+----------+-------------+
+argument  +argument type+
+==========+=============+
+fuzz_count+int const    +
+----------+-------------+
+blk       +block<> const+
+----------+-------------+


run block however many times
do not ignore panic, so that we can see where the runtime fails
this is here so that `fuzz` can be easily replaced with `fuzz_debug` for the purpose of debugging

.. _function-_at_fuzzer_c__c_fuzz_numeric_and_vector_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_and_vector_op1(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes single numeric or vector argument.
arguments are: int, uint, float, double, string, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4

.. _function-_at_fuzzer_c__c_fuzz_numeric_and_vector_signed_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_and_vector_signed_op1(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes single numeric or vector argument.
arguments are: int, uint, float, double, string, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4

.. _function-_at_fuzzer_c__c_fuzz_numeric_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_op1(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes single numeric or vector argument.
arguments are: int, uint, float, double

.. _function-_at_fuzzer_c__c_fuzz_numeric_and_storage_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_and_storage_op1(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes single numeric or vector argument.
arguments are: int, uint, int8, uint8, int16, uint16, int64, uint64, float, double

.. _function-_at_fuzzer_c__c_fuzz_all_ints_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_all_ints_op1(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes single numeric or vector argument.
arguments are: int, uint, int64, uint64

.. _function-_at_fuzzer_c__c_fuzz_all_unsigned_ints_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_all_unsigned_ints_op1(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes single numeric or vector argument.
arguments are: uint, uint64

.. _function-_at_fuzzer_c__c_fuzz_float_double_or_float_vec_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_float_double_or_float_vec_op1(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes single numeric or vector argument.
arguments are: float, double, float2, float3, float4

.. _function-_at_fuzzer_c__c_fuzz_float_or_float_vec_op1_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_float_or_float_vec_op1(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes single numeric or vector argument.
arguments are: float, float2, float3, float4

.. _function-_at_fuzzer_c__c_fuzz_numeric_and_vector_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_and_vector_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes two numeric or vector arguments.
arguments are: int, uint, float, double, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4

.. _function-_at_fuzzer_c__c_fuzz_numeric_and_vector_op2_no_unint_vec_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_and_vector_op2_no_unint_vec(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes two numeric or vector arguments.
arguments are: int, uint, float, double, int2, int3, int4, float2, float3, float4

.. _function-_at_fuzzer_c__c_fuzz_numeric_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes two numeric or vector arguments.
arguments are: int, uint, float, double

.. _function-_at_fuzzer_c__c_fuzz_compareable_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_compareable_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes two numeric or vector arguments.
arguments are: int, uint, float, double, int64, uint64, string

.. _function-_at_fuzzer_c__c_fuzz_eq_neq_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_eq_neq_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes two numeric or vector arguments.
arguments are: int, uint, int64, uint64, float, double, string, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4

.. _function-_at_fuzzer_c__c_fuzz_numeric_vec_scal_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_vec_scal_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes vector and matching scalar on the right
arguments pairs are: int2,int; int3,int; uint2,uint; uint3,uint; uint4,uint; int4,int; float2,float; float3,float; float4,float

.. _function-_at_fuzzer_c__c_fuzz_numeric_scal_vec_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_scal_vec_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes vector and matching scalar on the left
arguments pairs are: int2,int; int3,int; uint2,uint; uint3,uint; uint4,uint; int4,int; float2,float; float3,float; float4,float

.. _function-_at_fuzzer_c__c_fuzz_int_vector_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_int_vector_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes two numeric or vector arguments.
arguments are: int, uint, int2, int3, int4, uint2, uint3, uint4

.. _function-_at_fuzzer_c__c_fuzz_shift_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_shift_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes numeric or vector argument, with matching shift type on the right.
arguments are: int, uint, int2, int3, int4, uint2, uint3, uint4

.. _function-_at_fuzzer_c__c_fuzz_rotate_op2_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_rotate_op2(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes numeric or vector argument, with matching rotate type on the right.
arguments are: int, uint

.. _function-_at_fuzzer_c__c_fuzz_numeric_op3_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_op3(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes three numeric or vector arguments.
arguments are: int, uint, float, double

.. _function-_at_fuzzer_c__c_fuzz_vec_op3_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_vec_op3(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes three numeric or vector arguments.
arguments are: float2, float3, float4, int2, int3, int4, uint2, uint3, uint4

.. _function-_at_fuzzer_c__c_fuzz_vec_mad_op3_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_vec_mad_op3(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes three numeric or vector arguments.
arguments are: float2, float3, float4, int2, int3, int4, uint2, uint3, uint4 second argument is float, int, uint accordingly

.. _function-_at_fuzzer_c__c_fuzz_float_double_or_float_vec_op3_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_float_double_or_float_vec_op3(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes three numeric or vector arguments.
arguments are: float, double, float2, float3, float4

.. _function-_at_fuzzer_c__c_fuzz_numeric_op4_C1_ls_S_ls_testing_c__c_T_gr__gr_?_S_ls_faker_c__c_Faker_gr__Cs:

.. das:function:: fuzz_numeric_op4(t: testing::T? const; fake: Faker; funcname: string const)

+--------+---------------------------------------------+
+argument+argument type                                +
+========+=============================================+
+t       + :ref:`testing::T <struct-testing-T>` ? const+
+--------+---------------------------------------------+
+fake    + :ref:`faker::Faker <struct-faker-Faker>`    +
+--------+---------------------------------------------+
+funcname+string const                                 +
+--------+---------------------------------------------+


fuzzes generic function that takes four numeric or vector arguments.
arguments are: int, uint, float, double


