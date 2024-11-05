
.. _stdlib_algorithm:

=======================
Miscelanious algorithms
=======================

.. include:: detail/algorithm.rst

The ALGORITHM module exposes collection of miscellaneous array manipulation algorithms.

All functions and symbols are in "algorithm" module, use require to get access to it. ::

    require daslib/algorithm


++++++
Search
++++++

  *  :ref:`lower_bound (a:array\<auto(TT)\> const;f:int const;l:int const;val:TT const -&) : auto <function-_at_algorithm_c__c_lower_bound_C1_ls_Y_ls_TT_gr_._gr_A_Ci_Ci_CY_ls_TT_gr_L>` 
  *  :ref:`lower_bound (a:array\<auto(TT)\> const;val:TT const -&) : auto <function-_at_algorithm_c__c_lower_bound_C1_ls_Y_ls_TT_gr_._gr_A_CY_ls_TT_gr_L>` 
  *  :ref:`lower_bound (a:array\<auto(TT)\> const;f:int const;l:int const;value:TT const -&;less:block\<(a:TT const -&;b:TT const -&):bool\> const) : auto <function-_at_algorithm_c__c_lower_bound_C1_ls_Y_ls_TT_gr_._gr_A_Ci_Ci_CY_ls_TT_gr_L_CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin_>` 
  *  :ref:`lower_bound (a:array\<auto(TT)\> const;value:TT const -&;less:block\<(a:TT const -&;b:TT const -&):bool\> const) : auto <function-_at_algorithm_c__c_lower_bound_C1_ls_Y_ls_TT_gr_._gr_A_CY_ls_TT_gr_L_CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin_>` 
  *  :ref:`binary_search (a:array\<auto(TT)\> const;val:TT const -&) : auto <function-_at_algorithm_c__c_binary_search_C1_ls_Y_ls_TT_gr_._gr_A_CY_ls_TT_gr_L>` 
  *  :ref:`binary_search (a:array\<auto(TT)\> const;f:int const;last:int const;val:TT const -&) : auto <function-_at_algorithm_c__c_binary_search_C1_ls_Y_ls_TT_gr_._gr_A_Ci_Ci_CY_ls_TT_gr_L>` 
  *  :ref:`binary_search (a:array\<auto(TT)\> const;val:TT const -&;less:block\<(a:TT const -&;b:TT const -&):bool\> const) : auto <function-_at_algorithm_c__c_binary_search_C1_ls_Y_ls_TT_gr_._gr_A_CY_ls_TT_gr_L_CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin_>` 
  *  :ref:`binary_search (a:array\<auto(TT)\> const;f:int const;last:int const;val:TT const -&;less:block\<(a:TT const -&;b:TT const -&):bool\> const) : auto <function-_at_algorithm_c__c_binary_search_C1_ls_Y_ls_TT_gr_._gr_A_Ci_Ci_CY_ls_TT_gr_L_CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin_>` 
  *  :ref:`lower_bound (a:auto const;f:int const;l:int const;val:auto const) : auto <function-_at_algorithm_c__c_lower_bound_C._Ci_Ci_C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 
  *  :ref:`lower_bound (a:auto const;val:auto const) : auto <function-_at_algorithm_c__c_lower_bound_C._C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 
  *  :ref:`lower_bound (a:auto const;f:int const;l:int const;val:auto(TT) const;less:block\<(a:TT const -&;b:TT const -&):bool\> const) : auto <function-_at_algorithm_c__c_lower_bound_C._Ci_Ci_CY_ls_TT_gr_._CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin__%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 
  *  :ref:`lower_bound (a:auto const;val:auto(TT) const;less:block\<(a:TT const -&;b:TT const -&):bool\> const) : auto <function-_at_algorithm_c__c_lower_bound_C._CY_ls_TT_gr_._CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin__%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 
  *  :ref:`binary_search (a:auto const;val:auto const) : auto <function-_at_algorithm_c__c_binary_search_C._C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 
  *  :ref:`binary_search (a:auto const;f:int const;last:int const;val:auto const) : auto <function-_at_algorithm_c__c_binary_search_C._Ci_Ci_C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 
  *  :ref:`binary_search (a:auto const;val:auto(TT) const;less:block\<(a:TT const -&;b:TT const -&):bool\> const) : auto <function-_at_algorithm_c__c_binary_search_C._CY_ls_TT_gr_._CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin__%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 
  *  :ref:`binary_search (a:auto const;f:int const;last:int const;val:auto(TT) const;less:block\<(a:TT const -&;b:TT const -&):bool\> const) : auto <function-_at_algorithm_c__c_binary_search_C._Ci_Ci_CY_ls_TT_gr_._CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin__%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 

.. _function-_at_algorithm_c__c_lower_bound_C1_ls_Y_ls_TT_gr_._gr_A_Ci_Ci_CY_ls_TT_gr_L:

.. das:function:: lower_bound(a: array<auto(TT)> const; f: int const; l: int const; val: TT const)

lower_bound returns auto

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+a       +array<auto(TT)> const+
+--------+---------------------+
+f       +int const            +
+--------+---------------------+
+l       +int const            +
+--------+---------------------+
+val     +TT const             +
+--------+---------------------+


Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.

.. _function-_at_algorithm_c__c_lower_bound_C1_ls_Y_ls_TT_gr_._gr_A_CY_ls_TT_gr_L:

.. das:function:: lower_bound(a: array<auto(TT)> const; val: TT const)

lower_bound returns auto

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+a       +array<auto(TT)> const+
+--------+---------------------+
+val     +TT const             +
+--------+---------------------+


Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.

.. _function-_at_algorithm_c__c_lower_bound_C1_ls_Y_ls_TT_gr_._gr_A_Ci_Ci_CY_ls_TT_gr_L_CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin_:

.. das:function:: lower_bound(a: array<auto(TT)> const; f: int const; l: int const; value: TT const; less: block<(a:TT const;b:TT const):bool> const)

lower_bound returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+a       +array<auto(TT)> const                    +
+--------+-----------------------------------------+
+f       +int const                                +
+--------+-----------------------------------------+
+l       +int const                                +
+--------+-----------------------------------------+
+value   +TT const                                 +
+--------+-----------------------------------------+
+less    +block<(a:TT const;b:TT const):bool> const+
+--------+-----------------------------------------+


Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.

.. _function-_at_algorithm_c__c_lower_bound_C1_ls_Y_ls_TT_gr_._gr_A_CY_ls_TT_gr_L_CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin_:

.. das:function:: lower_bound(a: array<auto(TT)> const; value: TT const; less: block<(a:TT const;b:TT const):bool> const)

lower_bound returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+a       +array<auto(TT)> const                    +
+--------+-----------------------------------------+
+value   +TT const                                 +
+--------+-----------------------------------------+
+less    +block<(a:TT const;b:TT const):bool> const+
+--------+-----------------------------------------+


Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.

.. _function-_at_algorithm_c__c_binary_search_C1_ls_Y_ls_TT_gr_._gr_A_CY_ls_TT_gr_L:

.. das:function:: binary_search(a: array<auto(TT)> const; val: TT const)

binary_search returns auto

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+a       +array<auto(TT)> const+
+--------+---------------------+
+val     +TT const             +
+--------+---------------------+


now for all the other types

.. _function-_at_algorithm_c__c_binary_search_C1_ls_Y_ls_TT_gr_._gr_A_Ci_Ci_CY_ls_TT_gr_L:

.. das:function:: binary_search(a: array<auto(TT)> const; f: int const; last: int const; val: TT const)

binary_search returns auto

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+a       +array<auto(TT)> const+
+--------+---------------------+
+f       +int const            +
+--------+---------------------+
+last    +int const            +
+--------+---------------------+
+val     +TT const             +
+--------+---------------------+


now for all the other types

.. _function-_at_algorithm_c__c_binary_search_C1_ls_Y_ls_TT_gr_._gr_A_CY_ls_TT_gr_L_CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin_:

.. das:function:: binary_search(a: array<auto(TT)> const; val: TT const; less: block<(a:TT const;b:TT const):bool> const)

binary_search returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+a       +array<auto(TT)> const                    +
+--------+-----------------------------------------+
+val     +TT const                                 +
+--------+-----------------------------------------+
+less    +block<(a:TT const;b:TT const):bool> const+
+--------+-----------------------------------------+


now for all the other types

.. _function-_at_algorithm_c__c_binary_search_C1_ls_Y_ls_TT_gr_._gr_A_Ci_Ci_CY_ls_TT_gr_L_CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin_:

.. das:function:: binary_search(a: array<auto(TT)> const; f: int const; last: int const; val: TT const; less: block<(a:TT const;b:TT const):bool> const)

binary_search returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+a       +array<auto(TT)> const                    +
+--------+-----------------------------------------+
+f       +int const                                +
+--------+-----------------------------------------+
+last    +int const                                +
+--------+-----------------------------------------+
+val     +TT const                                 +
+--------+-----------------------------------------+
+less    +block<(a:TT const;b:TT const):bool> const+
+--------+-----------------------------------------+


now for all the other types

.. _function-_at_algorithm_c__c_lower_bound_C._Ci_Ci_C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: lower_bound(a: auto const; f: int const; l: int const; val: auto const)

lower_bound returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +auto const   +
+--------+-------------+
+f       +int const    +
+--------+-------------+
+l       +int const    +
+--------+-------------+
+val     +auto const   +
+--------+-------------+


Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.

.. _function-_at_algorithm_c__c_lower_bound_C._C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: lower_bound(a: auto const; val: auto const)

lower_bound returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +auto const   +
+--------+-------------+
+val     +auto const   +
+--------+-------------+


Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.

.. _function-_at_algorithm_c__c_lower_bound_C._Ci_Ci_CY_ls_TT_gr_._CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin__%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: lower_bound(a: auto const; f: int const; l: int const; val: auto(TT) const; less: block<(a:TT const;b:TT const):bool> const)

lower_bound returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+a       +auto const                               +
+--------+-----------------------------------------+
+f       +int const                                +
+--------+-----------------------------------------+
+l       +int const                                +
+--------+-----------------------------------------+
+val     +auto(TT) const                           +
+--------+-----------------------------------------+
+less    +block<(a:TT const;b:TT const):bool> const+
+--------+-----------------------------------------+


Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.

.. _function-_at_algorithm_c__c_lower_bound_C._CY_ls_TT_gr_._CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin__%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: lower_bound(a: auto const; val: auto(TT) const; less: block<(a:TT const;b:TT const):bool> const)

lower_bound returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+a       +auto const                               +
+--------+-----------------------------------------+
+val     +auto(TT) const                           +
+--------+-----------------------------------------+
+less    +block<(a:TT const;b:TT const):bool> const+
+--------+-----------------------------------------+


Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.

.. _function-_at_algorithm_c__c_binary_search_C._C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: binary_search(a: auto const; val: auto const)

binary_search returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +auto const   +
+--------+-------------+
+val     +auto const   +
+--------+-------------+


now for all the other types

.. _function-_at_algorithm_c__c_binary_search_C._Ci_Ci_C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: binary_search(a: auto const; f: int const; last: int const; val: auto const)

binary_search returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +auto const   +
+--------+-------------+
+f       +int const    +
+--------+-------------+
+last    +int const    +
+--------+-------------+
+val     +auto const   +
+--------+-------------+


now for all the other types

.. _function-_at_algorithm_c__c_binary_search_C._CY_ls_TT_gr_._CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin__%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: binary_search(a: auto const; val: auto(TT) const; less: block<(a:TT const;b:TT const):bool> const)

binary_search returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+a       +auto const                               +
+--------+-----------------------------------------+
+val     +auto(TT) const                           +
+--------+-----------------------------------------+
+less    +block<(a:TT const;b:TT const):bool> const+
+--------+-----------------------------------------+


now for all the other types

.. _function-_at_algorithm_c__c_binary_search_C._Ci_Ci_CY_ls_TT_gr_._CN_ls_a;b_gr_0_ls_CY_ls_TT_gr_L;CY_ls_TT_gr_L_gr_1_ls_b_gr__builtin__%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: binary_search(a: auto const; f: int const; last: int const; val: auto(TT) const; less: block<(a:TT const;b:TT const):bool> const)

binary_search returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+a       +auto const                               +
+--------+-----------------------------------------+
+f       +int const                                +
+--------+-----------------------------------------+
+last    +int const                                +
+--------+-----------------------------------------+
+val     +auto(TT) const                           +
+--------+-----------------------------------------+
+less    +block<(a:TT const;b:TT const):bool> const+
+--------+-----------------------------------------+


now for all the other types

++++++++++++++++++
Array manipulation
++++++++++++++++++

  *  :ref:`unique (a:array\<auto(TT)\> -const) : auto <function-_at_algorithm_c__c_unique_1_ls_Y_ls_TT_gr_._gr_A>` 
  *  :ref:`sort_unique (a:array\<auto(TT)\> -const) : auto <function-_at_algorithm_c__c_sort_unique_1_ls_Y_ls_TT_gr_._gr_A>` 
  *  :ref:`reverse (a:array\<auto\> -const) : auto <function-_at_algorithm_c__c_reverse_1_ls_._gr_A>` 
  *  :ref:`combine (a:array\<auto(TT)\> const;b:array\<auto(TT)\> const) : auto <function-_at_algorithm_c__c_combine_C1_ls_Y_ls_TT_gr_._gr_A_C1_ls_Y_ls_TT_gr_._gr_A>` 
  *  :ref:`reverse (a:auto -const) : auto <function-_at_algorithm_c__c_reverse_._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 
  *  :ref:`combine (a:auto const;b:auto const) : auto <function-_at_algorithm_c__c_combine_C._C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_>` 

.. _function-_at_algorithm_c__c_unique_1_ls_Y_ls_TT_gr_._gr_A:

.. das:function:: unique(a: array<auto(TT)>)

unique returns auto

+--------+---------------+
+argument+argument type  +
+========+===============+
+a       +array<auto(TT)>+
+--------+---------------+


Returns array of the elements of a with duplicates removed.

.. _function-_at_algorithm_c__c_sort_unique_1_ls_Y_ls_TT_gr_._gr_A:

.. das:function:: sort_unique(a: array<auto(TT)>)

sort_unique returns auto

+--------+---------------+
+argument+argument type  +
+========+===============+
+a       +array<auto(TT)>+
+--------+---------------+


Returns array of the elements of a, sorted and with duplicates removed.
The elements of a are sorted in ascending order.
The resulted array has only unqiue elements.

.. _function-_at_algorithm_c__c_reverse_1_ls_._gr_A:

.. das:function:: reverse(a: array<auto>)

reverse returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +array<auto>  +
+--------+-------------+


Returns array of the elements of a in reverse order.

.. _function-_at_algorithm_c__c_combine_C1_ls_Y_ls_TT_gr_._gr_A_C1_ls_Y_ls_TT_gr_._gr_A:

.. das:function:: combine(a: array<auto(TT)> const; b: array<auto(TT)> const)

combine returns auto

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+a       +array<auto(TT)> const+
+--------+---------------------+
+b       +array<auto(TT)> const+
+--------+---------------------+


Returns array of the elements of a and then b.

.. _function-_at_algorithm_c__c_reverse_._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: reverse(a: auto)

reverse returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +auto         +
+--------+-------------+


Returns array of the elements of a in reverse order.

.. _function-_at_algorithm_c__c_combine_C._C._%_ls_IsAnyArrayMacro_c_expect_any_array(a_eq_true)_gr_:

.. das:function:: combine(a: auto const; b: auto const)

combine returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +auto const   +
+--------+-------------+
+b       +auto const   +
+--------+-------------+


Returns array of the elements of a and then b.

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_algorithm_c__c_erase_all_._C._%_ls_IsAnyArrayMacro_c_expect_any_array(arr_eq_true)_gr_:

.. das:function:: erase_all(arr: auto; value: auto const)

erase_all returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+arr     +auto         +
+--------+-------------+
+value   +auto const   +
+--------+-------------+


Erase all elements equal to value from arr

.. _function-_at_algorithm_c__c_topological_sort_C1_ls_Y_ls_Node_gr_._gr_A:

.. das:function:: topological_sort(nodes: array<auto(Node)> const)

topological_sort returns auto

+--------+-----------------------+
+argument+argument type          +
+========+=======================+
+nodes   +array<auto(Node)> const+
+--------+-----------------------+


Topological sort of a graph.
Each node has an id, and set (table with no values) of dependencies.
Dependency `before` represents a link from a node, which should appear in the sorted list before the node.
Returns a sorted list of nodes.

.. _function-_at_algorithm_c__c_intersection_C1_ls_Y_ls_TT_gr_._gr_2_ls_v_gr_T_C1_ls_Y_ls_TT_gr_._gr_2_ls_v_gr_T:

.. das:function:: intersection(a: table<auto(TT);void> const; b: table<auto(TT);void> const)

intersection returns table<TT;void>

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+a       +table<auto(TT);void> const+
+--------+--------------------------+
+b       +table<auto(TT);void> const+
+--------+--------------------------+


Returns the intersection of two sets

.. _function-_at_algorithm_c__c_union_C1_ls_Y_ls_TT_gr_._gr_2_ls_v_gr_T_C1_ls_Y_ls_TT_gr_._gr_2_ls_v_gr_T:

.. das:function:: union(a: table<auto(TT);void> const; b: table<auto(TT);void> const)

union returns table<TT;void>

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+a       +table<auto(TT);void> const+
+--------+--------------------------+
+b       +table<auto(TT);void> const+
+--------+--------------------------+


Returns the union of two sets

.. _function-_at_algorithm_c__c_difference_C1_ls_Y_ls_TT_gr_._gr_2_ls_v_gr_T_C1_ls_Y_ls_TT_gr_._gr_2_ls_v_gr_T:

.. das:function:: difference(a: table<auto(TT);void> const; b: table<auto(TT);void> const)

difference returns table<TT;void>

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+a       +table<auto(TT);void> const+
+--------+--------------------------+
+b       +table<auto(TT);void> const+
+--------+--------------------------+


Returns the difference of two sets

.. _function-_at_algorithm_c__c_identical_C1_ls_Y_ls_TT_gr_._gr_2_ls_v_gr_T_C1_ls_Y_ls_TT_gr_._gr_2_ls_v_gr_T:

.. das:function:: identical(a: table<auto(TT);void> const; b: table<auto(TT);void> const)

identical returns bool

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+a       +table<auto(TT);void> const+
+--------+--------------------------+
+b       +table<auto(TT);void> const+
+--------+--------------------------+


Returns true if the two sets are identical


