
.. _stdlib_unroll:

==============
Loop unrolling
==============

.. include:: detail/unroll.rst

The unroll module implements loop unrolling infrastructure.

All functions and symbols are in "unroll" module, use require to get access to it. ::

    require daslib/unroll

++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-unroll-UnrollMacro:

.. das:attribute:: UnrollMacro

This macro implements loop unrolling in the form of `unroll` function.
Unroll function expects block with the single for loop in it.
Moveover only range for is supported, and only with the fixed range.
For example:::

    var n : float4[9]
    unroll <|   // contents of the loop will be replaced with 9 image load instructions.
        for i in range(9)
            n[i] = imageLoad(c_bloom_htex, xy + int2(0,i-4))

+++++++++
Unrolling
+++++++++

  *  :ref:`unroll (blk:block\<\> const) : void <function-_at_unroll_c__c_unroll_C_builtin_>` 

.. _function-_at_unroll_c__c_unroll_C_builtin_:

.. das:function:: unroll(blk: block<> const)

+--------+-------------+
+argument+argument type+
+========+=============+
+blk     +block<> const+
+--------+-------------+


Unrolls the for loop (with fixed range)


