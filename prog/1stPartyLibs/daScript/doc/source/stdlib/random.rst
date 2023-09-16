
.. _stdlib_random:

========================
Random generator library
========================

.. include:: detail/random.rst

The random library implements basic random routines.

All functions and symbols are in "random" module, use require to get access to it. ::

    require random


+++++++++
Constants
+++++++++

.. _global-random-LCG_RAND_MAX:

.. das:attribute:: LCG_RAND_MAX = 32767

maximum possible output of random number generator

.. _global-random-LCG_RAND_MAX_BIG:

.. das:attribute:: LCG_RAND_MAX_BIG = 1073741823

maximum possible output of random_big_int

+++++++++++++++++++++++++
Seed and basic generators
+++++++++++++++++++++++++

  *  :ref:`random_seed (seed:int const) : auto <function-_at_random_c__c_random_seed_Ci>` 
  *  :ref:`random_seed2D (seed:int4& -const;co:int2 const;cf:int const) : auto <function-_at_random_c__c_random_seed2D_&i4_Ci2_Ci>` 
  *  :ref:`random_int (seed:int4& -const) : auto <function-_at_random_c__c_random_int_&i4>` 
  *  :ref:`random_big_int (seed:int4& -const) : auto <function-_at_random_c__c_random_big_int_&i4>` 
  *  :ref:`random_uint (seed:int4& -const) : auto <function-_at_random_c__c_random_uint_&i4>` 
  *  :ref:`random_int4 (seed:int4& -const) : auto <function-_at_random_c__c_random_int4_&i4>` 
  *  :ref:`random_float (seed:int4& -const) : auto <function-_at_random_c__c_random_float_&i4>` 
  *  :ref:`random_float4 (seed:int4& -const) : auto <function-_at_random_c__c_random_float4_&i4>` 

.. _function-_at_random_c__c_random_seed_Ci:

.. das:function:: random_seed(seed: int const)

random_seed returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int const    +
+--------+-------------+


constructs seed vector out of single integer seed

.. _function-_at_random_c__c_random_seed2D_&i4_Ci2_Ci:

.. das:function:: random_seed2D(seed: int4&; co: int2 const; cf: int const)

random_seed2D returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+
+co      +int2 const   +
+--------+-------------+
+cf      +int const    +
+--------+-------------+


constructs seed vector out of 2d screen coordinates and frame counter `cf`

.. _function-_at_random_c__c_random_int_&i4:

.. das:function:: random_int(seed: int4&)

random_int returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random integer 0..32767 (LCG_RAND_MAX)

.. _function-_at_random_c__c_random_big_int_&i4:

.. das:function:: random_big_int(seed: int4&)

random_big_int returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random integer 0..32768*32768-1 (LCG_RAND_MAX_BIG)

.. _function-_at_random_c__c_random_uint_&i4:

.. das:function:: random_uint(seed: int4&)

random_uint returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random integer 0..32768*32768-1 (LCG_RAND_MAX_BIG)

.. _function-_at_random_c__c_random_int4_&i4:

.. das:function:: random_int4(seed: int4&)

random_int4 returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random int4, each component is 0..32767 (LCG_RAND_MAX)

.. _function-_at_random_c__c_random_float_&i4:

.. das:function:: random_float(seed: int4&)

random_float returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random float 0..1

.. _function-_at_random_c__c_random_float4_&i4:

.. das:function:: random_float4(seed: int4&)

random_float4 returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random float4, each component is 0..1

++++++++++++++++
Random iterators
++++++++++++++++

  *  :ref:`each_random_uint (rnd_seed:int const) : iterator\<uint\> <function-_at_random_c__c_each_random_uint_Ci>` 

.. _function-_at_random_c__c_each_random_uint_Ci:

.. das:function:: each_random_uint(rnd_seed: int const)

each_random_uint returns iterator<uint>

+--------+-------------+
+argument+argument type+
+========+=============+
+rnd_seed+int const    +
+--------+-------------+


|function-random-each_random_uint|

++++++++++++++++++++++
Specific distributions
++++++++++++++++++++++

  *  :ref:`random_unit_vector (seed:int4& -const) : auto <function-_at_random_c__c_random_unit_vector_&i4>` 
  *  :ref:`random_in_unit_sphere (seed:int4& -const) : auto <function-_at_random_c__c_random_in_unit_sphere_&i4>` 
  *  :ref:`random_in_unit_disk (seed:int4& -const) : auto <function-_at_random_c__c_random_in_unit_disk_&i4>` 

.. _function-_at_random_c__c_random_unit_vector_&i4:

.. das:function:: random_unit_vector(seed: int4&)

random_unit_vector returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random float3 unit vector (length=1.)

.. _function-_at_random_c__c_random_in_unit_sphere_&i4:

.. das:function:: random_in_unit_sphere(seed: int4&)

random_in_unit_sphere returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random float3 unit vector (length=1) which happens to be inside a sphere R=1

.. _function-_at_random_c__c_random_in_unit_disk_&i4:

.. das:function:: random_in_unit_disk(seed: int4&)

random_in_unit_disk returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +int4&        +
+--------+-------------+


random float3 unit vector (length=1) which happens to be inside a disk R=1, Z=0


