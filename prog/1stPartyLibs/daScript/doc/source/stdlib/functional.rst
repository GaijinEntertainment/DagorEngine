
.. _stdlib_functional:

==============================
Functional programming library
==============================

.. include:: detail/functional.rst

The functional module implements a collection of high-order functions and patters to expose functional programming patters to Daslang.

All functions and symbols are in "functional" module, use require to get access to it. ::

    require daslib/functional

+++++++++++
Map, reduce
+++++++++++

  *  :ref:`filter (src:iterator\<auto(TT)\> -const;blk:lambda\<(what:TT const -&):bool\> const) : auto <function-_at_functional_c__c_filter_1_ls_Y_ls_TT_gr_._gr_G_CN_ls_what_gr_0_ls_CY_ls_TT_gr_L_gr_1_ls_b_gr__at_>`
  *  :ref:`filter (src:iterator\<auto(TT)\> -const;blk:function\<(what:TT const -&):bool\> const) : auto <function-_at_functional_c__c_filter_1_ls_Y_ls_TT_gr_._gr_G_CN_ls_what_gr_0_ls_CY_ls_TT_gr_L_gr_1_ls_b_gr__at__at_>`
  *  :ref:`map (src:iterator\<auto(TT)\> -const;blk:lambda\<(what:TT const -&):auto(QQ)\> const) : auto <function-_at_functional_c__c_map_1_ls_Y_ls_TT_gr_._gr_G_CN_ls_what_gr_0_ls_CY_ls_TT_gr_L_gr_1_ls_Y_ls_QQ_gr_._gr__at_>`
  *  :ref:`map (src:iterator\<auto(TT)\> -const;blk:function\<(what:TT const -&):auto(QQ)\> const) : auto <function-_at_functional_c__c_map_1_ls_Y_ls_TT_gr_._gr_G_CN_ls_what_gr_0_ls_CY_ls_TT_gr_L_gr_1_ls_Y_ls_QQ_gr_._gr__at__at_>`
  *  :ref:`reduce (it:iterator\<auto(TT)\> const;blk:lambda\<(left:TT const -&;right:TT const -&):TT const -&\> const) : auto <function-_at_functional_c__c_reduce_C1_ls_Y_ls_TT_gr_._gr_G_CN_ls_left;right_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_CY_ls_TT_gr_L_gr__at_>`
  *  :ref:`reduce (it:iterator\<auto(TT)\> const;blk:function\<(left:TT const -&;right:TT const -&):TT const -&\> const) : auto <function-_at_functional_c__c_reduce_C1_ls_Y_ls_TT_gr_._gr_G_CN_ls_left;right_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_CY_ls_TT_gr_L_gr__at__at_>`
  *  :ref:`reduce (it:iterator\<auto(TT)\> const;blk:block\<(left:TT const -&;right:TT const -&):TT const -&\> const) : auto <function-_at_functional_c__c_reduce_C1_ls_Y_ls_TT_gr_._gr_G_CN_ls_left;right_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_CY_ls_TT_gr_L_gr__builtin_>`
  *  :ref:`sum (it:iterator\<auto(TT)\> const) : auto <function-_at_functional_c__c_sum_C1_ls_Y_ls_TT_gr_._gr_G>`
  *  :ref:`any (it:auto const) : auto <function-_at_functional_c__c_any_C.>`
  *  :ref:`all (it:auto const) : auto <function-_at_functional_c__c_all_C.>`
  *  :ref:`cycle (src:iterator\<auto(TT)\> -const) : auto <function-_at_functional_c__c_cycle_1_ls_Y_ls_TT_gr_._gr_G>`
  *  :ref:`islice (src:iterator\<auto(TT)\> -const;start:int const;stop:int const) : auto <function-_at_functional_c__c_islice_1_ls_Y_ls_TT_gr_._gr_G_Ci_Ci>`
  *  :ref:`repeat_ref (value:auto(TT) const;total:int -const) : auto <function-_at_functional_c__c_repeat_ref_CY_ls_TT_gr_._i>`
  *  :ref:`repeat (value:auto(TT) const;count:int -const) : auto <function-_at_functional_c__c_repeat_CY_ls_TT_gr_._i>`
  *  :ref:`not (x:auto const) : auto <function-_at_functional_c__c_not_C.>`
  *  :ref:`echo (x:auto -const;extra:string const) : auto <function-_at_functional_c__c_echo_._Cs>`
  *  :ref:`flatten (it:iterator\<auto(TT)\> -const) : auto <function-_at_functional_c__c_flatten_1_ls_Y_ls_TT_gr_._gr_G>`

.. _function-_at_functional_c__c_filter_1_ls_Y_ls_TT_gr_._gr_G_CN_ls_what_gr_0_ls_CY_ls_TT_gr_L_gr_1_ls_b_gr__at_:

.. das:function:: filter(src: iterator<auto(TT)>; blk: lambda<(what:TT const -&):bool> const)

filter returns auto

+--------+----------------------------------+
+argument+argument type                     +
+========+==================================+
+src     +iterator<auto(TT)>                +
+--------+----------------------------------+
+blk     +lambda<(what:TT const):bool> const+
+--------+----------------------------------+


iterates over `src` and yields only those elements for which `blk` returns true

.. _function-_at_functional_c__c_filter_1_ls_Y_ls_TT_gr_._gr_G_CN_ls_what_gr_0_ls_CY_ls_TT_gr_L_gr_1_ls_b_gr__at__at_:

.. das:function:: filter(src: iterator<auto(TT)>; blk: function<(what:TT const -&):bool> const)

filter returns auto

+--------+------------------------------------+
+argument+argument type                       +
+========+====================================+
+src     +iterator<auto(TT)>                  +
+--------+------------------------------------+
+blk     +function<(what:TT const):bool> const+
+--------+------------------------------------+


iterates over `src` and yields only those elements for which `blk` returns true

.. _function-_at_functional_c__c_map_1_ls_Y_ls_TT_gr_._gr_G_CN_ls_what_gr_0_ls_CY_ls_TT_gr_L_gr_1_ls_Y_ls_QQ_gr_._gr__at_:

.. das:function:: map(src: iterator<auto(TT)>; blk: lambda<(what:TT const -&):auto(QQ)> const)

map returns auto

+--------+--------------------------------------+
+argument+argument type                         +
+========+======================================+
+src     +iterator<auto(TT)>                    +
+--------+--------------------------------------+
+blk     +lambda<(what:TT const):auto(QQ)> const+
+--------+--------------------------------------+


iterates over `src` and yields the result of `blk` for each element

.. _function-_at_functional_c__c_map_1_ls_Y_ls_TT_gr_._gr_G_CN_ls_what_gr_0_ls_CY_ls_TT_gr_L_gr_1_ls_Y_ls_QQ_gr_._gr__at__at_:

.. das:function:: map(src: iterator<auto(TT)>; blk: function<(what:TT const -&):auto(QQ)> const)

map returns auto

+--------+----------------------------------------+
+argument+argument type                           +
+========+========================================+
+src     +iterator<auto(TT)>                      +
+--------+----------------------------------------+
+blk     +function<(what:TT const):auto(QQ)> const+
+--------+----------------------------------------+


iterates over `src` and yields the result of `blk` for each element

.. _function-_at_functional_c__c_reduce_C1_ls_Y_ls_TT_gr_._gr_G_CN_ls_left;right_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_CY_ls_TT_gr_L_gr__at_:

.. das:function:: reduce(it: iterator<auto(TT)> const; blk: lambda<(left:TT const -&;right:TT const -&):TT const -&> const)

reduce returns auto

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+it      +iterator<auto(TT)> const                             +
+--------+-----------------------------------------------------+
+blk     +lambda<(left:TT const;right:TT const):TT const> const+
+--------+-----------------------------------------------------+


iterates over `it` and yields the reduced (combined) result of `blk` for each element
and previous reduction result

.. _function-_at_functional_c__c_reduce_C1_ls_Y_ls_TT_gr_._gr_G_CN_ls_left;right_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_CY_ls_TT_gr_L_gr__at__at_:

.. das:function:: reduce(it: iterator<auto(TT)> const; blk: function<(left:TT const -&;right:TT const -&):TT const -&> const)

reduce returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+it      +iterator<auto(TT)> const                               +
+--------+-------------------------------------------------------+
+blk     +function<(left:TT const;right:TT const):TT const> const+
+--------+-------------------------------------------------------+


iterates over `it` and yields the reduced (combined) result of `blk` for each element
and previous reduction result

.. _function-_at_functional_c__c_reduce_C1_ls_Y_ls_TT_gr_._gr_G_CN_ls_left;right_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_CY_ls_TT_gr_L_gr__builtin_:

.. das:function:: reduce(it: iterator<auto(TT)> const; blk: block<(left:TT const -&;right:TT const -&):TT const -&> const)

reduce returns auto

+--------+----------------------------------------------------+
+argument+argument type                                       +
+========+====================================================+
+it      +iterator<auto(TT)> const                            +
+--------+----------------------------------------------------+
+blk     +block<(left:TT const;right:TT const):TT const> const+
+--------+----------------------------------------------------+


iterates over `it` and yields the reduced (combined) result of `blk` for each element
and previous reduction result

.. _function-_at_functional_c__c_sum_C1_ls_Y_ls_TT_gr_._gr_G:

.. das:function:: sum(it: iterator<auto(TT)> const)

sum returns auto

+--------+------------------------+
+argument+argument type           +
+========+========================+
+it      +iterator<auto(TT)> const+
+--------+------------------------+


iterates over `it` and yields the sum of all elements
same as reduce(it, @(a,b) => a + b)

.. _function-_at_functional_c__c_any_C.:

.. das:function:: any(it: auto const)

any returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+it      +auto const   +
+--------+-------------+


iterates over `it` and yields true if any element is true

.. _function-_at_functional_c__c_all_C.:

.. das:function:: all(it: auto const)

all returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+it      +auto const   +
+--------+-------------+


iterates over `it` and yields true if all elements are true

.. _function-_at_functional_c__c_cycle_1_ls_Y_ls_TT_gr_._gr_G:

.. das:function:: cycle(src: iterator<auto(TT)>)

cycle returns auto

+--------+------------------+
+argument+argument type     +
+========+==================+
+src     +iterator<auto(TT)>+
+--------+------------------+


endlessly iterates over `src`

.. _function-_at_functional_c__c_islice_1_ls_Y_ls_TT_gr_._gr_G_Ci_Ci:

.. das:function:: islice(src: iterator<auto(TT)>; start: int const; stop: int const)

islice returns auto

+--------+------------------+
+argument+argument type     +
+========+==================+
+src     +iterator<auto(TT)>+
+--------+------------------+
+start   +int const         +
+--------+------------------+
+stop    +int const         +
+--------+------------------+


iterates over `src` and yields only the elements in the range [start,stop)

.. _function-_at_functional_c__c_repeat_ref_CY_ls_TT_gr_._i:

.. das:function:: repeat_ref(value: auto(TT) const; total: int)

repeat_ref returns auto

+--------+--------------+
+argument+argument type +
+========+==============+
+value   +auto(TT) const+
+--------+--------------+
+total   +int           +
+--------+--------------+


yields `value` by reference `count` times

.. _function-_at_functional_c__c_repeat_CY_ls_TT_gr_._i:

.. das:function:: repeat(value: auto(TT) const; count: int)

repeat returns auto

+--------+--------------+
+argument+argument type +
+========+==============+
+value   +auto(TT) const+
+--------+--------------+
+count   +int           +
+--------+--------------+


yields `value` `count` times

.. _function-_at_functional_c__c_not_C.:

.. das:function:: not(x: auto const)

not returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +auto const   +
+--------+-------------+


yeilds !x

.. _function-_at_functional_c__c_echo_._Cs:

.. das:function:: echo(x: auto; extra: string const)

echo returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +auto         +
+--------+-------------+
+extra   +string const +
+--------+-------------+


prints contents of the string to the output, with `extra` string appended

.. _function-_at_functional_c__c_flatten_1_ls_Y_ls_TT_gr_._gr_G:

.. das:function:: flatten(it: iterator<auto(TT)>)

flatten returns auto

+--------+------------------+
+argument+argument type     +
+========+==================+
+it      +iterator<auto(TT)>+
+--------+------------------+


iterates over `it`, than iterates over each element of each element of `it` and yields it

+++++++
Queries
+++++++

  *  :ref:`is_equal (a:auto const;b:auto const) : auto <function-_at_functional_c__c_is_equal_C._C.>`
  *  :ref:`is_not_equal (a:auto const;b:auto const) : auto <function-_at_functional_c__c_is_not_equal_C._C.>`

.. _function-_at_functional_c__c_is_equal_C._C.:

.. das:function:: is_equal(a: auto const; b: auto const)

is_equal returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +auto const   +
+--------+-------------+
+b       +auto const   +
+--------+-------------+


yields true if `a` and `b` are equal

.. _function-_at_functional_c__c_is_not_equal_C._C.:

.. das:function:: is_not_equal(a: auto const; b: auto const)

is_not_equal returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +auto const   +
+--------+-------------+
+b       +auto const   +
+--------+-------------+


yields true if `a` and `b` are not equal

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_functional_c__c_sorted_1_ls_._gr_A:

.. das:function:: sorted(arr: array<auto>)

sorted returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+arr     +array<auto>  +
+--------+-------------+


iterates over input and returns it sorted version

.. _function-_at_functional_c__c_sorted_1_ls_Y_ls_TT_gr_._gr_G:

.. das:function:: sorted(it: iterator<auto(TT)>)

sorted returns auto

+--------+------------------+
+argument+argument type     +
+========+==================+
+it      +iterator<auto(TT)>+
+--------+------------------+


iterates over input and returns it sorted version


