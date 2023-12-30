
.. _stdlib_math:

============
Math library
============

.. include:: detail/math.rst

Floating point math in general is not bit-precise. Compiler can optimize permutations, replace divisions with multiplications, and some of functions are not bit-exact.
If you need precise math use double precision type.
All functions and symbols are in "math" module, use require to get access to it. ::

    require math


+++++++++
Constants
+++++++++

.. _global-math-PI:

.. das:attribute:: PI = 3.14159f

|variable-math-PI|

.. _global-math-FLT_EPSILON:

.. das:attribute:: FLT_EPSILON = 1.19209e-07f

|variable-math-FLT_EPSILON|

.. _global-math-DBL_EPSILON:

.. das:attribute:: DBL_EPSILON = 2.22045e-16lf

|variable-math-DBL_EPSILON|

++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-math-float4x4:

.. das:attribute:: float4x4

float4x4 fields are

+-+------+
+z+float4+
+-+------+
+w+float4+
+-+------+
+y+float4+
+-+------+
+x+float4+
+-+------+


|structure_annotation-math-float4x4|

.. _handle-math-float3x4:

.. das:attribute:: float3x4

float3x4 fields are

+-+------+
+z+float3+
+-+------+
+w+float3+
+-+------+
+y+float3+
+-+------+
+x+float3+
+-+------+


|structure_annotation-math-float3x4|

.. _handle-math-float3x3:

.. das:attribute:: float3x3

float3x3 fields are

+-+------+
+z+float3+
+-+------+
+y+float3+
+-+------+
+x+float3+
+-+------+


|structure_annotation-math-float3x3|

++++++++++++++++++++++++++++++++++++++++++
all numerics (uint*, int*, float*, double)
++++++++++++++++++++++++++++++++++++++++++

  *  :ref:`min (x:int const;y:int const) : int <function-_at_math_c__c_min_Ci_Ci>` 
  *  :ref:`max (x:int const;y:int const) : int <function-_at_math_c__c_max_Ci_Ci>` 
  *  :ref:`min (x:int2 const;y:int2 const) : int2 <function-_at_math_c__c_min_Ci2_Ci2>` 
  *  :ref:`max (x:int2 const;y:int2 const) : int2 <function-_at_math_c__c_max_Ci2_Ci2>` 
  *  :ref:`min (x:int3 const;y:int3 const) : int3 <function-_at_math_c__c_min_Ci3_Ci3>` 
  *  :ref:`max (x:int3 const;y:int3 const) : int3 <function-_at_math_c__c_max_Ci3_Ci3>` 
  *  :ref:`min (x:int4 const;y:int4 const) : int4 <function-_at_math_c__c_min_Ci4_Ci4>` 
  *  :ref:`max (x:int4 const;y:int4 const) : int4 <function-_at_math_c__c_max_Ci4_Ci4>` 
  *  :ref:`min (x:uint const;y:uint const) : uint <function-_at_math_c__c_min_Cu_Cu>` 
  *  :ref:`max (x:uint const;y:uint const) : uint <function-_at_math_c__c_max_Cu_Cu>` 
  *  :ref:`min (x:uint2 const;y:uint2 const) : uint2 <function-_at_math_c__c_min_Cu2_Cu2>` 
  *  :ref:`max (x:uint2 const;y:uint2 const) : uint2 <function-_at_math_c__c_max_Cu2_Cu2>` 
  *  :ref:`min (x:uint3 const;y:uint3 const) : uint3 <function-_at_math_c__c_min_Cu3_Cu3>` 
  *  :ref:`max (x:uint3 const;y:uint3 const) : uint3 <function-_at_math_c__c_max_Cu3_Cu3>` 
  *  :ref:`min (x:uint4 const;y:uint4 const) : uint4 <function-_at_math_c__c_min_Cu4_Cu4>` 
  *  :ref:`max (x:uint4 const;y:uint4 const) : uint4 <function-_at_math_c__c_max_Cu4_Cu4>` 
  *  :ref:`min (x:float const;y:float const) : float <function-_at_math_c__c_min_Cf_Cf>` 
  *  :ref:`max (x:float const;y:float const) : float <function-_at_math_c__c_max_Cf_Cf>` 
  *  :ref:`min (x:float2 const;y:float2 const) : float2 <function-_at_math_c__c_min_Cf2_Cf2>` 
  *  :ref:`max (x:float2 const;y:float2 const) : float2 <function-_at_math_c__c_max_Cf2_Cf2>` 
  *  :ref:`min (x:float3 const;y:float3 const) : float3 <function-_at_math_c__c_min_Cf3_Cf3>` 
  *  :ref:`max (x:float3 const;y:float3 const) : float3 <function-_at_math_c__c_max_Cf3_Cf3>` 
  *  :ref:`min (x:float4 const;y:float4 const) : float4 <function-_at_math_c__c_min_Cf4_Cf4>` 
  *  :ref:`max (x:float4 const;y:float4 const) : float4 <function-_at_math_c__c_max_Cf4_Cf4>` 
  *  :ref:`min (x:double const;y:double const) : double <function-_at_math_c__c_min_Cd_Cd>` 
  *  :ref:`max (x:double const;y:double const) : double <function-_at_math_c__c_max_Cd_Cd>` 
  *  :ref:`min (x:int64 const;y:int64 const) : int64 <function-_at_math_c__c_min_Ci64_Ci64>` 
  *  :ref:`max (x:int64 const;y:int64 const) : int64 <function-_at_math_c__c_max_Ci64_Ci64>` 
  *  :ref:`min (x:uint64 const;y:uint64 const) : uint64 <function-_at_math_c__c_min_Cu64_Cu64>` 
  *  :ref:`max (x:uint64 const;y:uint64 const) : uint64 <function-_at_math_c__c_max_Cu64_Cu64>` 

.. _function-_at_math_c__c_min_Ci_Ci:

.. das:function:: min(x: int const; y: int const)

min returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int const    +
+--------+-------------+
+y       +int const    +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Ci_Ci:

.. das:function:: max(x: int const; y: int const)

max returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int const    +
+--------+-------------+
+y       +int const    +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Ci2_Ci2:

.. das:function:: min(x: int2 const; y: int2 const)

min returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int2 const   +
+--------+-------------+
+y       +int2 const   +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Ci2_Ci2:

.. das:function:: max(x: int2 const; y: int2 const)

max returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int2 const   +
+--------+-------------+
+y       +int2 const   +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Ci3_Ci3:

.. das:function:: min(x: int3 const; y: int3 const)

min returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int3 const   +
+--------+-------------+
+y       +int3 const   +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Ci3_Ci3:

.. das:function:: max(x: int3 const; y: int3 const)

max returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int3 const   +
+--------+-------------+
+y       +int3 const   +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Ci4_Ci4:

.. das:function:: min(x: int4 const; y: int4 const)

min returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int4 const   +
+--------+-------------+
+y       +int4 const   +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Ci4_Ci4:

.. das:function:: max(x: int4 const; y: int4 const)

max returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int4 const   +
+--------+-------------+
+y       +int4 const   +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cu_Cu:

.. das:function:: min(x: uint const; y: uint const)

min returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint const   +
+--------+-------------+
+y       +uint const   +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cu_Cu:

.. das:function:: max(x: uint const; y: uint const)

max returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint const   +
+--------+-------------+
+y       +uint const   +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cu2_Cu2:

.. das:function:: min(x: uint2 const; y: uint2 const)

min returns uint2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint2 const  +
+--------+-------------+
+y       +uint2 const  +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cu2_Cu2:

.. das:function:: max(x: uint2 const; y: uint2 const)

max returns uint2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint2 const  +
+--------+-------------+
+y       +uint2 const  +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cu3_Cu3:

.. das:function:: min(x: uint3 const; y: uint3 const)

min returns uint3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint3 const  +
+--------+-------------+
+y       +uint3 const  +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cu3_Cu3:

.. das:function:: max(x: uint3 const; y: uint3 const)

max returns uint3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint3 const  +
+--------+-------------+
+y       +uint3 const  +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cu4_Cu4:

.. das:function:: min(x: uint4 const; y: uint4 const)

min returns uint4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint4 const  +
+--------+-------------+
+y       +uint4 const  +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cu4_Cu4:

.. das:function:: max(x: uint4 const; y: uint4 const)

max returns uint4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint4 const  +
+--------+-------------+
+y       +uint4 const  +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cf_Cf:

.. das:function:: min(x: float const; y: float const)

min returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+
+y       +float const  +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cf_Cf:

.. das:function:: max(x: float const; y: float const)

max returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+
+y       +float const  +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cf2_Cf2:

.. das:function:: min(x: float2 const; y: float2 const)

min returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+
+y       +float2 const +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cf2_Cf2:

.. das:function:: max(x: float2 const; y: float2 const)

max returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+
+y       +float2 const +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cf3_Cf3:

.. das:function:: min(x: float3 const; y: float3 const)

min returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cf3_Cf3:

.. das:function:: max(x: float3 const; y: float3 const)

max returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cf4_Cf4:

.. das:function:: min(x: float4 const; y: float4 const)

min returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+
+y       +float4 const +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cf4_Cf4:

.. das:function:: max(x: float4 const; y: float4 const)

max returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+
+y       +float4 const +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cd_Cd:

.. das:function:: min(x: double const; y: double const)

min returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+
+y       +double const +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cd_Cd:

.. das:function:: max(x: double const; y: double const)

max returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+
+y       +double const +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Ci64_Ci64:

.. das:function:: min(x: int64 const; y: int64 const)

min returns int64

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int64 const  +
+--------+-------------+
+y       +int64 const  +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Ci64_Ci64:

.. das:function:: max(x: int64 const; y: int64 const)

max returns int64

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int64 const  +
+--------+-------------+
+y       +int64 const  +
+--------+-------------+


|function-math-max|

.. _function-_at_math_c__c_min_Cu64_Cu64:

.. das:function:: min(x: uint64 const; y: uint64 const)

min returns uint64

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint64 const +
+--------+-------------+
+y       +uint64 const +
+--------+-------------+


|function-math-min|

.. _function-_at_math_c__c_max_Cu64_Cu64:

.. das:function:: max(x: uint64 const; y: uint64 const)

max returns uint64

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint64 const +
+--------+-------------+
+y       +uint64 const +
+--------+-------------+


|function-math-max|

+++++++++++++++++
float* and double
+++++++++++++++++

  *  :ref:`sin (x:float const) : float <function-_at_math_c__c_sin_Cf>` 
  *  :ref:`cos (x:float const) : float <function-_at_math_c__c_cos_Cf>` 
  *  :ref:`tan (x:float const) : float <function-_at_math_c__c_tan_Cf>` 
  *  :ref:`atan (x:float const) : float <function-_at_math_c__c_atan_Cf>` 
  *  :ref:`asin (x:float const) : float <function-_at_math_c__c_asin_Cf>` 
  *  :ref:`acos (x:float const) : float <function-_at_math_c__c_acos_Cf>` 
  *  :ref:`atan2 (y:float const;x:float const) : float <function-_at_math_c__c_atan2_Cf_Cf>` 
  *  :ref:`sin (x:float2 const) : float2 <function-_at_math_c__c_sin_Cf2>` 
  *  :ref:`cos (x:float2 const) : float2 <function-_at_math_c__c_cos_Cf2>` 
  *  :ref:`tan (x:float2 const) : float2 <function-_at_math_c__c_tan_Cf2>` 
  *  :ref:`atan (x:float2 const) : float2 <function-_at_math_c__c_atan_Cf2>` 
  *  :ref:`asin (x:float2 const) : float2 <function-_at_math_c__c_asin_Cf2>` 
  *  :ref:`acos (x:float2 const) : float2 <function-_at_math_c__c_acos_Cf2>` 
  *  :ref:`atan2 (y:float2 const;x:float2 const) : float2 <function-_at_math_c__c_atan2_Cf2_Cf2>` 
  *  :ref:`sin (x:float3 const) : float3 <function-_at_math_c__c_sin_Cf3>` 
  *  :ref:`cos (x:float3 const) : float3 <function-_at_math_c__c_cos_Cf3>` 
  *  :ref:`tan (x:float3 const) : float3 <function-_at_math_c__c_tan_Cf3>` 
  *  :ref:`atan (x:float3 const) : float3 <function-_at_math_c__c_atan_Cf3>` 
  *  :ref:`asin (x:float3 const) : float3 <function-_at_math_c__c_asin_Cf3>` 
  *  :ref:`acos (x:float3 const) : float3 <function-_at_math_c__c_acos_Cf3>` 
  *  :ref:`atan2 (y:float3 const;x:float3 const) : float3 <function-_at_math_c__c_atan2_Cf3_Cf3>` 
  *  :ref:`sin (x:float4 const) : float4 <function-_at_math_c__c_sin_Cf4>` 
  *  :ref:`cos (x:float4 const) : float4 <function-_at_math_c__c_cos_Cf4>` 
  *  :ref:`tan (x:float4 const) : float4 <function-_at_math_c__c_tan_Cf4>` 
  *  :ref:`atan (x:float4 const) : float4 <function-_at_math_c__c_atan_Cf4>` 
  *  :ref:`asin (x:float4 const) : float4 <function-_at_math_c__c_asin_Cf4>` 
  *  :ref:`acos (x:float4 const) : float4 <function-_at_math_c__c_acos_Cf4>` 
  *  :ref:`atan2 (y:float4 const;x:float4 const) : float4 <function-_at_math_c__c_atan2_Cf4_Cf4>` 
  *  :ref:`exp (x:float const) : float <function-_at_math_c__c_exp_Cf>` 
  *  :ref:`log (x:float const) : float <function-_at_math_c__c_log_Cf>` 
  *  :ref:`exp2 (x:float const) : float <function-_at_math_c__c_exp2_Cf>` 
  *  :ref:`log2 (x:float const) : float <function-_at_math_c__c_log2_Cf>` 
  *  :ref:`rcp (x:float const) : float <function-_at_math_c__c_rcp_Cf>` 
  *  :ref:`pow (x:float const;y:float const) : float <function-_at_math_c__c_pow_Cf_Cf>` 
  *  :ref:`exp (x:float2 const) : float2 <function-_at_math_c__c_exp_Cf2>` 
  *  :ref:`log (x:float2 const) : float2 <function-_at_math_c__c_log_Cf2>` 
  *  :ref:`exp2 (x:float2 const) : float2 <function-_at_math_c__c_exp2_Cf2>` 
  *  :ref:`log2 (x:float2 const) : float2 <function-_at_math_c__c_log2_Cf2>` 
  *  :ref:`rcp (x:float2 const) : float2 <function-_at_math_c__c_rcp_Cf2>` 
  *  :ref:`pow (x:float2 const;y:float2 const) : float2 <function-_at_math_c__c_pow_Cf2_Cf2>` 
  *  :ref:`exp (x:float3 const) : float3 <function-_at_math_c__c_exp_Cf3>` 
  *  :ref:`log (x:float3 const) : float3 <function-_at_math_c__c_log_Cf3>` 
  *  :ref:`exp2 (x:float3 const) : float3 <function-_at_math_c__c_exp2_Cf3>` 
  *  :ref:`log2 (x:float3 const) : float3 <function-_at_math_c__c_log2_Cf3>` 
  *  :ref:`rcp (x:float3 const) : float3 <function-_at_math_c__c_rcp_Cf3>` 
  *  :ref:`pow (x:float3 const;y:float3 const) : float3 <function-_at_math_c__c_pow_Cf3_Cf3>` 
  *  :ref:`exp (x:float4 const) : float4 <function-_at_math_c__c_exp_Cf4>` 
  *  :ref:`log (x:float4 const) : float4 <function-_at_math_c__c_log_Cf4>` 
  *  :ref:`exp2 (x:float4 const) : float4 <function-_at_math_c__c_exp2_Cf4>` 
  *  :ref:`log2 (x:float4 const) : float4 <function-_at_math_c__c_log2_Cf4>` 
  *  :ref:`rcp (x:float4 const) : float4 <function-_at_math_c__c_rcp_Cf4>` 
  *  :ref:`pow (x:float4 const;y:float4 const) : float4 <function-_at_math_c__c_pow_Cf4_Cf4>` 
  *  :ref:`floor (x:float const) : float <function-_at_math_c__c_floor_Cf>` 
  *  :ref:`ceil (x:float const) : float <function-_at_math_c__c_ceil_Cf>` 
  *  :ref:`sqrt (x:float const) : float <function-_at_math_c__c_sqrt_Cf>` 
  *  :ref:`saturate (x:float const) : float <function-_at_math_c__c_saturate_Cf>` 
  *  :ref:`floor (x:float2 const) : float2 <function-_at_math_c__c_floor_Cf2>` 
  *  :ref:`ceil (x:float2 const) : float2 <function-_at_math_c__c_ceil_Cf2>` 
  *  :ref:`sqrt (x:float2 const) : float2 <function-_at_math_c__c_sqrt_Cf2>` 
  *  :ref:`saturate (x:float2 const) : float2 <function-_at_math_c__c_saturate_Cf2>` 
  *  :ref:`floor (x:float3 const) : float3 <function-_at_math_c__c_floor_Cf3>` 
  *  :ref:`ceil (x:float3 const) : float3 <function-_at_math_c__c_ceil_Cf3>` 
  *  :ref:`sqrt (x:float3 const) : float3 <function-_at_math_c__c_sqrt_Cf3>` 
  *  :ref:`saturate (x:float3 const) : float3 <function-_at_math_c__c_saturate_Cf3>` 
  *  :ref:`floor (x:float4 const) : float4 <function-_at_math_c__c_floor_Cf4>` 
  *  :ref:`ceil (x:float4 const) : float4 <function-_at_math_c__c_ceil_Cf4>` 
  *  :ref:`sqrt (x:float4 const) : float4 <function-_at_math_c__c_sqrt_Cf4>` 
  *  :ref:`saturate (x:float4 const) : float4 <function-_at_math_c__c_saturate_Cf4>` 
  *  :ref:`abs (x:int const) : int <function-_at_math_c__c_abs_Ci>` 
  *  :ref:`sign (x:int const) : int <function-_at_math_c__c_sign_Ci>` 
  *  :ref:`abs (x:int2 const) : int2 <function-_at_math_c__c_abs_Ci2>` 
  *  :ref:`sign (x:int2 const) : int2 <function-_at_math_c__c_sign_Ci2>` 
  *  :ref:`abs (x:int3 const) : int3 <function-_at_math_c__c_abs_Ci3>` 
  *  :ref:`sign (x:int3 const) : int3 <function-_at_math_c__c_sign_Ci3>` 
  *  :ref:`abs (x:int4 const) : int4 <function-_at_math_c__c_abs_Ci4>` 
  *  :ref:`sign (x:int4 const) : int4 <function-_at_math_c__c_sign_Ci4>` 
  *  :ref:`abs (x:uint const) : uint <function-_at_math_c__c_abs_Cu>` 
  *  :ref:`sign (x:uint const) : uint <function-_at_math_c__c_sign_Cu>` 
  *  :ref:`abs (x:uint2 const) : uint2 <function-_at_math_c__c_abs_Cu2>` 
  *  :ref:`sign (x:uint2 const) : uint2 <function-_at_math_c__c_sign_Cu2>` 
  *  :ref:`abs (x:uint3 const) : uint3 <function-_at_math_c__c_abs_Cu3>` 
  *  :ref:`sign (x:uint3 const) : uint3 <function-_at_math_c__c_sign_Cu3>` 
  *  :ref:`abs (x:uint4 const) : uint4 <function-_at_math_c__c_abs_Cu4>` 
  *  :ref:`sign (x:uint4 const) : uint4 <function-_at_math_c__c_sign_Cu4>` 
  *  :ref:`abs (x:float const) : float <function-_at_math_c__c_abs_Cf>` 
  *  :ref:`sign (x:float const) : float <function-_at_math_c__c_sign_Cf>` 
  *  :ref:`abs (x:float2 const) : float2 <function-_at_math_c__c_abs_Cf2>` 
  *  :ref:`sign (x:float2 const) : float2 <function-_at_math_c__c_sign_Cf2>` 
  *  :ref:`abs (x:float3 const) : float3 <function-_at_math_c__c_abs_Cf3>` 
  *  :ref:`sign (x:float3 const) : float3 <function-_at_math_c__c_sign_Cf3>` 
  *  :ref:`abs (x:float4 const) : float4 <function-_at_math_c__c_abs_Cf4>` 
  *  :ref:`sign (x:float4 const) : float4 <function-_at_math_c__c_sign_Cf4>` 
  *  :ref:`abs (x:double const) : double <function-_at_math_c__c_abs_Cd>` 
  *  :ref:`sign (x:double const) : double <function-_at_math_c__c_sign_Cd>` 
  *  :ref:`abs (x:int64 const) : int64 <function-_at_math_c__c_abs_Ci64>` 
  *  :ref:`sign (x:int64 const) : int64 <function-_at_math_c__c_sign_Ci64>` 
  *  :ref:`abs (x:uint64 const) : uint64 <function-_at_math_c__c_abs_Cu64>` 
  *  :ref:`sign (x:uint64 const) : uint64 <function-_at_math_c__c_sign_Cu64>` 
  *  :ref:`is_nan (x:float const) : bool <function-_at_math_c__c_is_nan_Cf>` 
  *  :ref:`is_finite (x:float const) : bool <function-_at_math_c__c_is_finite_Cf>` 
  *  :ref:`is_nan (x:double const) : bool <function-_at_math_c__c_is_nan_Cd>` 
  *  :ref:`is_finite (x:double const) : bool <function-_at_math_c__c_is_finite_Cd>` 
  *  :ref:`sqrt (x:double const) : double <function-_at_math_c__c_sqrt_Cd>` 
  *  :ref:`exp (x:double const) : double <function-_at_math_c__c_exp_Cd>` 
  *  :ref:`rcp (x:double const) : double <function-_at_math_c__c_rcp_Cd>` 
  *  :ref:`log (x:double const) : double <function-_at_math_c__c_log_Cd>` 
  *  :ref:`pow (x:double const;y:double const) : double <function-_at_math_c__c_pow_Cd_Cd>` 
  *  :ref:`exp2 (x:double const) : double <function-_at_math_c__c_exp2_Cd>` 
  *  :ref:`log2 (x:double const) : double <function-_at_math_c__c_log2_Cd>` 
  *  :ref:`sin (x:double const) : double <function-_at_math_c__c_sin_Cd>` 
  *  :ref:`cos (x:double const) : double <function-_at_math_c__c_cos_Cd>` 
  *  :ref:`asin (x:double const) : double <function-_at_math_c__c_asin_Cd>` 
  *  :ref:`acos (x:double const) : double <function-_at_math_c__c_acos_Cd>` 
  *  :ref:`tan (x:double const) : double <function-_at_math_c__c_tan_Cd>` 
  *  :ref:`atan (x:double const) : double <function-_at_math_c__c_atan_Cd>` 
  *  :ref:`atan2 (y:double const;x:double const) : double <function-_at_math_c__c_atan2_Cd_Cd>` 
  *  :ref:`sincos (x:float const;s:float& implicit;c:float& implicit) : void <function-_at_math_c__c_sincos_Cf_&If_&If>` 
  *  :ref:`sincos (x:double const;s:double& implicit;c:double& implicit) : void <function-_at_math_c__c_sincos_Cd_&Id_&Id>` 

.. _function-_at_math_c__c_sin_Cf:

.. das:function:: sin(x: float const)

sin returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-sin|

.. _function-_at_math_c__c_cos_Cf:

.. das:function:: cos(x: float const)

cos returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-cos|

.. _function-_at_math_c__c_tan_Cf:

.. das:function:: tan(x: float const)

tan returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-tan|

.. _function-_at_math_c__c_atan_Cf:

.. das:function:: atan(x: float const)

atan returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-atan|

.. _function-_at_math_c__c_asin_Cf:

.. das:function:: asin(x: float const)

asin returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-asin|

.. _function-_at_math_c__c_acos_Cf:

.. das:function:: acos(x: float const)

acos returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-acos|

.. _function-_at_math_c__c_atan2_Cf_Cf:

.. das:function:: atan2(y: float const; x: float const)

atan2 returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +float const  +
+--------+-------------+
+x       +float const  +
+--------+-------------+


|function-math-atan2|

.. _function-_at_math_c__c_sin_Cf2:

.. das:function:: sin(x: float2 const)

sin returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-sin|

.. _function-_at_math_c__c_cos_Cf2:

.. das:function:: cos(x: float2 const)

cos returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-cos|

.. _function-_at_math_c__c_tan_Cf2:

.. das:function:: tan(x: float2 const)

tan returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-tan|

.. _function-_at_math_c__c_atan_Cf2:

.. das:function:: atan(x: float2 const)

atan returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-atan|

.. _function-_at_math_c__c_asin_Cf2:

.. das:function:: asin(x: float2 const)

asin returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-asin|

.. _function-_at_math_c__c_acos_Cf2:

.. das:function:: acos(x: float2 const)

acos returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-acos|

.. _function-_at_math_c__c_atan2_Cf2_Cf2:

.. das:function:: atan2(y: float2 const; x: float2 const)

atan2 returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +float2 const +
+--------+-------------+
+x       +float2 const +
+--------+-------------+


|function-math-atan2|

.. _function-_at_math_c__c_sin_Cf3:

.. das:function:: sin(x: float3 const)

sin returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-sin|

.. _function-_at_math_c__c_cos_Cf3:

.. das:function:: cos(x: float3 const)

cos returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-cos|

.. _function-_at_math_c__c_tan_Cf3:

.. das:function:: tan(x: float3 const)

tan returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-tan|

.. _function-_at_math_c__c_atan_Cf3:

.. das:function:: atan(x: float3 const)

atan returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-atan|

.. _function-_at_math_c__c_asin_Cf3:

.. das:function:: asin(x: float3 const)

asin returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-asin|

.. _function-_at_math_c__c_acos_Cf3:

.. das:function:: acos(x: float3 const)

acos returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-acos|

.. _function-_at_math_c__c_atan2_Cf3_Cf3:

.. das:function:: atan2(y: float3 const; x: float3 const)

atan2 returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +float3 const +
+--------+-------------+
+x       +float3 const +
+--------+-------------+


|function-math-atan2|

.. _function-_at_math_c__c_sin_Cf4:

.. das:function:: sin(x: float4 const)

sin returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-sin|

.. _function-_at_math_c__c_cos_Cf4:

.. das:function:: cos(x: float4 const)

cos returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-cos|

.. _function-_at_math_c__c_tan_Cf4:

.. das:function:: tan(x: float4 const)

tan returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-tan|

.. _function-_at_math_c__c_atan_Cf4:

.. das:function:: atan(x: float4 const)

atan returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-atan|

.. _function-_at_math_c__c_asin_Cf4:

.. das:function:: asin(x: float4 const)

asin returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-asin|

.. _function-_at_math_c__c_acos_Cf4:

.. das:function:: acos(x: float4 const)

acos returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-acos|

.. _function-_at_math_c__c_atan2_Cf4_Cf4:

.. das:function:: atan2(y: float4 const; x: float4 const)

atan2 returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +float4 const +
+--------+-------------+
+x       +float4 const +
+--------+-------------+


|function-math-atan2|

.. _function-_at_math_c__c_exp_Cf:

.. das:function:: exp(x: float const)

exp returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-exp|

.. _function-_at_math_c__c_log_Cf:

.. das:function:: log(x: float const)

log returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-log|

.. _function-_at_math_c__c_exp2_Cf:

.. das:function:: exp2(x: float const)

exp2 returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-exp2|

.. _function-_at_math_c__c_log2_Cf:

.. das:function:: log2(x: float const)

log2 returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-log2|

.. _function-_at_math_c__c_rcp_Cf:

.. das:function:: rcp(x: float const)

rcp returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-rcp|

.. _function-_at_math_c__c_pow_Cf_Cf:

.. das:function:: pow(x: float const; y: float const)

pow returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+
+y       +float const  +
+--------+-------------+


|function-math-pow|

.. _function-_at_math_c__c_exp_Cf2:

.. das:function:: exp(x: float2 const)

exp returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-exp|

.. _function-_at_math_c__c_log_Cf2:

.. das:function:: log(x: float2 const)

log returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-log|

.. _function-_at_math_c__c_exp2_Cf2:

.. das:function:: exp2(x: float2 const)

exp2 returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-exp2|

.. _function-_at_math_c__c_log2_Cf2:

.. das:function:: log2(x: float2 const)

log2 returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-log2|

.. _function-_at_math_c__c_rcp_Cf2:

.. das:function:: rcp(x: float2 const)

rcp returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-rcp|

.. _function-_at_math_c__c_pow_Cf2_Cf2:

.. das:function:: pow(x: float2 const; y: float2 const)

pow returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+
+y       +float2 const +
+--------+-------------+


|function-math-pow|

.. _function-_at_math_c__c_exp_Cf3:

.. das:function:: exp(x: float3 const)

exp returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-exp|

.. _function-_at_math_c__c_log_Cf3:

.. das:function:: log(x: float3 const)

log returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-log|

.. _function-_at_math_c__c_exp2_Cf3:

.. das:function:: exp2(x: float3 const)

exp2 returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-exp2|

.. _function-_at_math_c__c_log2_Cf3:

.. das:function:: log2(x: float3 const)

log2 returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-log2|

.. _function-_at_math_c__c_rcp_Cf3:

.. das:function:: rcp(x: float3 const)

rcp returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-rcp|

.. _function-_at_math_c__c_pow_Cf3_Cf3:

.. das:function:: pow(x: float3 const; y: float3 const)

pow returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-pow|

.. _function-_at_math_c__c_exp_Cf4:

.. das:function:: exp(x: float4 const)

exp returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-exp|

.. _function-_at_math_c__c_log_Cf4:

.. das:function:: log(x: float4 const)

log returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-log|

.. _function-_at_math_c__c_exp2_Cf4:

.. das:function:: exp2(x: float4 const)

exp2 returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-exp2|

.. _function-_at_math_c__c_log2_Cf4:

.. das:function:: log2(x: float4 const)

log2 returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-log2|

.. _function-_at_math_c__c_rcp_Cf4:

.. das:function:: rcp(x: float4 const)

rcp returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-rcp|

.. _function-_at_math_c__c_pow_Cf4_Cf4:

.. das:function:: pow(x: float4 const; y: float4 const)

pow returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+
+y       +float4 const +
+--------+-------------+


|function-math-pow|

.. _function-_at_math_c__c_floor_Cf:

.. das:function:: floor(x: float const)

floor returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-floor|

.. _function-_at_math_c__c_ceil_Cf:

.. das:function:: ceil(x: float const)

ceil returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-ceil|

.. _function-_at_math_c__c_sqrt_Cf:

.. das:function:: sqrt(x: float const)

sqrt returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-sqrt|

.. _function-_at_math_c__c_saturate_Cf:

.. das:function:: saturate(x: float const)

saturate returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-saturate|

.. _function-_at_math_c__c_floor_Cf2:

.. das:function:: floor(x: float2 const)

floor returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-floor|

.. _function-_at_math_c__c_ceil_Cf2:

.. das:function:: ceil(x: float2 const)

ceil returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-ceil|

.. _function-_at_math_c__c_sqrt_Cf2:

.. das:function:: sqrt(x: float2 const)

sqrt returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-sqrt|

.. _function-_at_math_c__c_saturate_Cf2:

.. das:function:: saturate(x: float2 const)

saturate returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-saturate|

.. _function-_at_math_c__c_floor_Cf3:

.. das:function:: floor(x: float3 const)

floor returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-floor|

.. _function-_at_math_c__c_ceil_Cf3:

.. das:function:: ceil(x: float3 const)

ceil returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-ceil|

.. _function-_at_math_c__c_sqrt_Cf3:

.. das:function:: sqrt(x: float3 const)

sqrt returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-sqrt|

.. _function-_at_math_c__c_saturate_Cf3:

.. das:function:: saturate(x: float3 const)

saturate returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-saturate|

.. _function-_at_math_c__c_floor_Cf4:

.. das:function:: floor(x: float4 const)

floor returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-floor|

.. _function-_at_math_c__c_ceil_Cf4:

.. das:function:: ceil(x: float4 const)

ceil returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-ceil|

.. _function-_at_math_c__c_sqrt_Cf4:

.. das:function:: sqrt(x: float4 const)

sqrt returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-sqrt|

.. _function-_at_math_c__c_saturate_Cf4:

.. das:function:: saturate(x: float4 const)

saturate returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-saturate|

.. _function-_at_math_c__c_abs_Ci:

.. das:function:: abs(x: int const)

abs returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int const    +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Ci:

.. das:function:: sign(x: int const)

sign returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int const    +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Ci2:

.. das:function:: abs(x: int2 const)

abs returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int2 const   +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Ci2:

.. das:function:: sign(x: int2 const)

sign returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int2 const   +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Ci3:

.. das:function:: abs(x: int3 const)

abs returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int3 const   +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Ci3:

.. das:function:: sign(x: int3 const)

sign returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int3 const   +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Ci4:

.. das:function:: abs(x: int4 const)

abs returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int4 const   +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Ci4:

.. das:function:: sign(x: int4 const)

sign returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int4 const   +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cu:

.. das:function:: abs(x: uint const)

abs returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint const   +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cu:

.. das:function:: sign(x: uint const)

sign returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint const   +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cu2:

.. das:function:: abs(x: uint2 const)

abs returns uint2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint2 const  +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cu2:

.. das:function:: sign(x: uint2 const)

sign returns uint2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint2 const  +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cu3:

.. das:function:: abs(x: uint3 const)

abs returns uint3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint3 const  +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cu3:

.. das:function:: sign(x: uint3 const)

sign returns uint3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint3 const  +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cu4:

.. das:function:: abs(x: uint4 const)

abs returns uint4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint4 const  +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cu4:

.. das:function:: sign(x: uint4 const)

sign returns uint4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint4 const  +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cf:

.. das:function:: abs(x: float const)

abs returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cf:

.. das:function:: sign(x: float const)

sign returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cf2:

.. das:function:: abs(x: float2 const)

abs returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cf2:

.. das:function:: sign(x: float2 const)

sign returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cf3:

.. das:function:: abs(x: float3 const)

abs returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cf3:

.. das:function:: sign(x: float3 const)

sign returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cf4:

.. das:function:: abs(x: float4 const)

abs returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cf4:

.. das:function:: sign(x: float4 const)

sign returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cd:

.. das:function:: abs(x: double const)

abs returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cd:

.. das:function:: sign(x: double const)

sign returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Ci64:

.. das:function:: abs(x: int64 const)

abs returns int64

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int64 const  +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Ci64:

.. das:function:: sign(x: int64 const)

sign returns int64

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +int64 const  +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_abs_Cu64:

.. das:function:: abs(x: uint64 const)

abs returns uint64

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint64 const +
+--------+-------------+


|function-math-abs|

.. _function-_at_math_c__c_sign_Cu64:

.. das:function:: sign(x: uint64 const)

sign returns uint64

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint64 const +
+--------+-------------+


|function-math-sign|

.. _function-_at_math_c__c_is_nan_Cf:

.. das:function:: is_nan(x: float const)

is_nan returns bool

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-is_nan|

.. _function-_at_math_c__c_is_finite_Cf:

.. das:function:: is_finite(x: float const)

is_finite returns bool

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-is_finite|

.. _function-_at_math_c__c_is_nan_Cd:

.. das:function:: is_nan(x: double const)

is_nan returns bool

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-is_nan|

.. _function-_at_math_c__c_is_finite_Cd:

.. das:function:: is_finite(x: double const)

is_finite returns bool

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-is_finite|

.. _function-_at_math_c__c_sqrt_Cd:

.. das:function:: sqrt(x: double const)

sqrt returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-sqrt|

.. _function-_at_math_c__c_exp_Cd:

.. das:function:: exp(x: double const)

exp returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-exp|

.. _function-_at_math_c__c_rcp_Cd:

.. das:function:: rcp(x: double const)

rcp returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-rcp|

.. _function-_at_math_c__c_log_Cd:

.. das:function:: log(x: double const)

log returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-log|

.. _function-_at_math_c__c_pow_Cd_Cd:

.. das:function:: pow(x: double const; y: double const)

pow returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+
+y       +double const +
+--------+-------------+


|function-math-pow|

.. _function-_at_math_c__c_exp2_Cd:

.. das:function:: exp2(x: double const)

exp2 returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-exp2|

.. _function-_at_math_c__c_log2_Cd:

.. das:function:: log2(x: double const)

log2 returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-log2|

.. _function-_at_math_c__c_sin_Cd:

.. das:function:: sin(x: double const)

sin returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-sin|

.. _function-_at_math_c__c_cos_Cd:

.. das:function:: cos(x: double const)

cos returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-cos|

.. _function-_at_math_c__c_asin_Cd:

.. das:function:: asin(x: double const)

asin returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-asin|

.. _function-_at_math_c__c_acos_Cd:

.. das:function:: acos(x: double const)

acos returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-acos|

.. _function-_at_math_c__c_tan_Cd:

.. das:function:: tan(x: double const)

tan returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-tan|

.. _function-_at_math_c__c_atan_Cd:

.. das:function:: atan(x: double const)

atan returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-atan|

.. _function-_at_math_c__c_atan2_Cd_Cd:

.. das:function:: atan2(y: double const; x: double const)

atan2 returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +double const +
+--------+-------------+
+x       +double const +
+--------+-------------+


|function-math-atan2|

.. _function-_at_math_c__c_sincos_Cf_&If_&If:

.. das:function:: sincos(x: float const; s: float& implicit; c: float& implicit)

+--------+---------------+
+argument+argument type  +
+========+===============+
+x       +float const    +
+--------+---------------+
+s       +float& implicit+
+--------+---------------+
+c       +float& implicit+
+--------+---------------+


|function-math-sincos|

.. _function-_at_math_c__c_sincos_Cd_&Id_&Id:

.. das:function:: sincos(x: double const; s: double& implicit; c: double& implicit)

+--------+----------------+
+argument+argument type   +
+========+================+
+x       +double const    +
+--------+----------------+
+s       +double& implicit+
+--------+----------------+
+c       +double& implicit+
+--------+----------------+


|function-math-sincos|

+++++++++++
float* only
+++++++++++

  *  :ref:`atan_est (x:float const) : float <function-_at_math_c__c_atan_est_Cf>` 
  *  :ref:`atan2_est (y:float const;x:float const) : float <function-_at_math_c__c_atan2_est_Cf_Cf>` 
  *  :ref:`atan_est (x:float2 const) : float2 <function-_at_math_c__c_atan_est_Cf2>` 
  *  :ref:`atan2_est (y:float2 const;x:float2 const) : float2 <function-_at_math_c__c_atan2_est_Cf2_Cf2>` 
  *  :ref:`atan_est (x:float3 const) : float3 <function-_at_math_c__c_atan_est_Cf3>` 
  *  :ref:`atan2_est (y:float3 const;x:float3 const) : float3 <function-_at_math_c__c_atan2_est_Cf3_Cf3>` 
  *  :ref:`atan_est (x:float4 const) : float4 <function-_at_math_c__c_atan_est_Cf4>` 
  *  :ref:`atan2_est (y:float4 const;x:float4 const) : float4 <function-_at_math_c__c_atan2_est_Cf4_Cf4>` 
  *  :ref:`rcp_est (x:float const) : float <function-_at_math_c__c_rcp_est_Cf>` 
  *  :ref:`rcp_est (x:float2 const) : float2 <function-_at_math_c__c_rcp_est_Cf2>` 
  *  :ref:`rcp_est (x:float3 const) : float3 <function-_at_math_c__c_rcp_est_Cf3>` 
  *  :ref:`rcp_est (x:float4 const) : float4 <function-_at_math_c__c_rcp_est_Cf4>` 
  *  :ref:`fract (x:float const) : float <function-_at_math_c__c_fract_Cf>` 
  *  :ref:`rsqrt (x:float const) : float <function-_at_math_c__c_rsqrt_Cf>` 
  *  :ref:`rsqrt_est (x:float const) : float <function-_at_math_c__c_rsqrt_est_Cf>` 
  *  :ref:`fract (x:float2 const) : float2 <function-_at_math_c__c_fract_Cf2>` 
  *  :ref:`rsqrt (x:float2 const) : float2 <function-_at_math_c__c_rsqrt_Cf2>` 
  *  :ref:`rsqrt_est (x:float2 const) : float2 <function-_at_math_c__c_rsqrt_est_Cf2>` 
  *  :ref:`fract (x:float3 const) : float3 <function-_at_math_c__c_fract_Cf3>` 
  *  :ref:`rsqrt (x:float3 const) : float3 <function-_at_math_c__c_rsqrt_Cf3>` 
  *  :ref:`rsqrt_est (x:float3 const) : float3 <function-_at_math_c__c_rsqrt_est_Cf3>` 
  *  :ref:`fract (x:float4 const) : float4 <function-_at_math_c__c_fract_Cf4>` 
  *  :ref:`rsqrt (x:float4 const) : float4 <function-_at_math_c__c_rsqrt_Cf4>` 
  *  :ref:`rsqrt_est (x:float4 const) : float4 <function-_at_math_c__c_rsqrt_est_Cf4>` 
  *  :ref:`floori (x:float const) : int <function-_at_math_c__c_floori_Cf>` 
  *  :ref:`ceili (x:float const) : int <function-_at_math_c__c_ceili_Cf>` 
  *  :ref:`roundi (x:float const) : int <function-_at_math_c__c_roundi_Cf>` 
  *  :ref:`trunci (x:float const) : int <function-_at_math_c__c_trunci_Cf>` 
  *  :ref:`floori (x:double const) : int <function-_at_math_c__c_floori_Cd>` 
  *  :ref:`ceili (x:double const) : int <function-_at_math_c__c_ceili_Cd>` 
  *  :ref:`roundi (x:double const) : int <function-_at_math_c__c_roundi_Cd>` 
  *  :ref:`trunci (x:double const) : int <function-_at_math_c__c_trunci_Cd>` 
  *  :ref:`floori (x:float2 const) : int2 <function-_at_math_c__c_floori_Cf2>` 
  *  :ref:`ceili (x:float2 const) : int2 <function-_at_math_c__c_ceili_Cf2>` 
  *  :ref:`roundi (x:float2 const) : int2 <function-_at_math_c__c_roundi_Cf2>` 
  *  :ref:`trunci (x:float2 const) : int2 <function-_at_math_c__c_trunci_Cf2>` 
  *  :ref:`floori (x:float3 const) : int3 <function-_at_math_c__c_floori_Cf3>` 
  *  :ref:`ceili (x:float3 const) : int3 <function-_at_math_c__c_ceili_Cf3>` 
  *  :ref:`roundi (x:float3 const) : int3 <function-_at_math_c__c_roundi_Cf3>` 
  *  :ref:`trunci (x:float3 const) : int3 <function-_at_math_c__c_trunci_Cf3>` 
  *  :ref:`floori (x:float4 const) : int4 <function-_at_math_c__c_floori_Cf4>` 
  *  :ref:`ceili (x:float4 const) : int4 <function-_at_math_c__c_ceili_Cf4>` 
  *  :ref:`roundi (x:float4 const) : int4 <function-_at_math_c__c_roundi_Cf4>` 
  *  :ref:`trunci (x:float4 const) : int4 <function-_at_math_c__c_trunci_Cf4>` 
  *  :ref:`- (x:math::float4x4 const implicit) : math::float4x4 <function-_at_math_c__c_-_CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`- (x:math::float3x4 const implicit) : math::float3x4 <function-_at_math_c__c_-_CIH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`- (x:math::float3x3 const implicit) : math::float3x3 <function-_at_math_c__c_-_CIH_ls_math_c__c_float3x3_gr_>` 

.. _function-_at_math_c__c_atan_est_Cf:

.. das:function:: atan_est(x: float const)

atan_est returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-atan_est|

.. _function-_at_math_c__c_atan2_est_Cf_Cf:

.. das:function:: atan2_est(y: float const; x: float const)

atan2_est returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +float const  +
+--------+-------------+
+x       +float const  +
+--------+-------------+


|function-math-atan2_est|

.. _function-_at_math_c__c_atan_est_Cf2:

.. das:function:: atan_est(x: float2 const)

atan_est returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-atan_est|

.. _function-_at_math_c__c_atan2_est_Cf2_Cf2:

.. das:function:: atan2_est(y: float2 const; x: float2 const)

atan2_est returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +float2 const +
+--------+-------------+
+x       +float2 const +
+--------+-------------+


|function-math-atan2_est|

.. _function-_at_math_c__c_atan_est_Cf3:

.. das:function:: atan_est(x: float3 const)

atan_est returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-atan_est|

.. _function-_at_math_c__c_atan2_est_Cf3_Cf3:

.. das:function:: atan2_est(y: float3 const; x: float3 const)

atan2_est returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +float3 const +
+--------+-------------+
+x       +float3 const +
+--------+-------------+


|function-math-atan2_est|

.. _function-_at_math_c__c_atan_est_Cf4:

.. das:function:: atan_est(x: float4 const)

atan_est returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-atan_est|

.. _function-_at_math_c__c_atan2_est_Cf4_Cf4:

.. das:function:: atan2_est(y: float4 const; x: float4 const)

atan2_est returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+y       +float4 const +
+--------+-------------+
+x       +float4 const +
+--------+-------------+


|function-math-atan2_est|

.. _function-_at_math_c__c_rcp_est_Cf:

.. das:function:: rcp_est(x: float const)

rcp_est returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-rcp_est|

.. _function-_at_math_c__c_rcp_est_Cf2:

.. das:function:: rcp_est(x: float2 const)

rcp_est returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-rcp_est|

.. _function-_at_math_c__c_rcp_est_Cf3:

.. das:function:: rcp_est(x: float3 const)

rcp_est returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-rcp_est|

.. _function-_at_math_c__c_rcp_est_Cf4:

.. das:function:: rcp_est(x: float4 const)

rcp_est returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-rcp_est|

.. _function-_at_math_c__c_fract_Cf:

.. das:function:: fract(x: float const)

fract returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-fract|

.. _function-_at_math_c__c_rsqrt_Cf:

.. das:function:: rsqrt(x: float const)

rsqrt returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-rsqrt|

.. _function-_at_math_c__c_rsqrt_est_Cf:

.. das:function:: rsqrt_est(x: float const)

rsqrt_est returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-rsqrt_est|

.. _function-_at_math_c__c_fract_Cf2:

.. das:function:: fract(x: float2 const)

fract returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-fract|

.. _function-_at_math_c__c_rsqrt_Cf2:

.. das:function:: rsqrt(x: float2 const)

rsqrt returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-rsqrt|

.. _function-_at_math_c__c_rsqrt_est_Cf2:

.. das:function:: rsqrt_est(x: float2 const)

rsqrt_est returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-rsqrt_est|

.. _function-_at_math_c__c_fract_Cf3:

.. das:function:: fract(x: float3 const)

fract returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-fract|

.. _function-_at_math_c__c_rsqrt_Cf3:

.. das:function:: rsqrt(x: float3 const)

rsqrt returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-rsqrt|

.. _function-_at_math_c__c_rsqrt_est_Cf3:

.. das:function:: rsqrt_est(x: float3 const)

rsqrt_est returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-rsqrt_est|

.. _function-_at_math_c__c_fract_Cf4:

.. das:function:: fract(x: float4 const)

fract returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-fract|

.. _function-_at_math_c__c_rsqrt_Cf4:

.. das:function:: rsqrt(x: float4 const)

rsqrt returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-rsqrt|

.. _function-_at_math_c__c_rsqrt_est_Cf4:

.. das:function:: rsqrt_est(x: float4 const)

rsqrt_est returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-rsqrt_est|

.. _function-_at_math_c__c_floori_Cf:

.. das:function:: floori(x: float const)

floori returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-floori|

.. _function-_at_math_c__c_ceili_Cf:

.. das:function:: ceili(x: float const)

ceili returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-ceili|

.. _function-_at_math_c__c_roundi_Cf:

.. das:function:: roundi(x: float const)

roundi returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-roundi|

.. _function-_at_math_c__c_trunci_Cf:

.. das:function:: trunci(x: float const)

trunci returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


|function-math-trunci|

.. _function-_at_math_c__c_floori_Cd:

.. das:function:: floori(x: double const)

floori returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-floori|

.. _function-_at_math_c__c_ceili_Cd:

.. das:function:: ceili(x: double const)

ceili returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-ceili|

.. _function-_at_math_c__c_roundi_Cd:

.. das:function:: roundi(x: double const)

roundi returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-roundi|

.. _function-_at_math_c__c_trunci_Cd:

.. das:function:: trunci(x: double const)

trunci returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +double const +
+--------+-------------+


|function-math-trunci|

.. _function-_at_math_c__c_floori_Cf2:

.. das:function:: floori(x: float2 const)

floori returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-floori|

.. _function-_at_math_c__c_ceili_Cf2:

.. das:function:: ceili(x: float2 const)

ceili returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-ceili|

.. _function-_at_math_c__c_roundi_Cf2:

.. das:function:: roundi(x: float2 const)

roundi returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-roundi|

.. _function-_at_math_c__c_trunci_Cf2:

.. das:function:: trunci(x: float2 const)

trunci returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-trunci|

.. _function-_at_math_c__c_floori_Cf3:

.. das:function:: floori(x: float3 const)

floori returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-floori|

.. _function-_at_math_c__c_ceili_Cf3:

.. das:function:: ceili(x: float3 const)

ceili returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-ceili|

.. _function-_at_math_c__c_roundi_Cf3:

.. das:function:: roundi(x: float3 const)

roundi returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-roundi|

.. _function-_at_math_c__c_trunci_Cf3:

.. das:function:: trunci(x: float3 const)

trunci returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-trunci|

.. _function-_at_math_c__c_floori_Cf4:

.. das:function:: floori(x: float4 const)

floori returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-floori|

.. _function-_at_math_c__c_ceili_Cf4:

.. das:function:: ceili(x: float4 const)

ceili returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-ceili|

.. _function-_at_math_c__c_roundi_Cf4:

.. das:function:: roundi(x: float4 const)

roundi returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-roundi|

.. _function-_at_math_c__c_trunci_Cf4:

.. das:function:: trunci(x: float4 const)

trunci returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-trunci|

.. _function-_at_math_c__c_-_CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: operator -(x: float4x4 const implicit)

- returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math--|

.. _function-_at_math_c__c_-_CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: operator -(x: float3x4 const implicit)

- returns  :ref:`math::float3x4 <handle-math-float3x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math--|

.. _function-_at_math_c__c_-_CIH_ls_math_c__c_float3x3_gr_:

.. das:function:: operator -(x: float3x3 const implicit)

- returns  :ref:`math::float3x3 <handle-math-float3x3>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math--|

+++++++++++
float3 only
+++++++++++

  *  :ref:`cross (x:float3 const;y:float3 const) : float3 <function-_at_math_c__c_cross_Cf3_Cf3>` 
  *  :ref:`distance (x:float2 const;y:float2 const) : float <function-_at_math_c__c_distance_Cf2_Cf2>` 
  *  :ref:`distance_sq (x:float2 const;y:float2 const) : float <function-_at_math_c__c_distance_sq_Cf2_Cf2>` 
  *  :ref:`inv_distance (x:float2 const;y:float2 const) : float <function-_at_math_c__c_inv_distance_Cf2_Cf2>` 
  *  :ref:`inv_distance_sq (x:float2 const;y:float2 const) : float <function-_at_math_c__c_inv_distance_sq_Cf2_Cf2>` 
  *  :ref:`distance (x:float3 const;y:float3 const) : float <function-_at_math_c__c_distance_Cf3_Cf3>` 
  *  :ref:`distance_sq (x:float3 const;y:float3 const) : float <function-_at_math_c__c_distance_sq_Cf3_Cf3>` 
  *  :ref:`inv_distance (x:float3 const;y:float3 const) : float <function-_at_math_c__c_inv_distance_Cf3_Cf3>` 
  *  :ref:`inv_distance_sq (x:float3 const;y:float3 const) : float <function-_at_math_c__c_inv_distance_sq_Cf3_Cf3>` 
  *  :ref:`distance (x:float4 const;y:float4 const) : float <function-_at_math_c__c_distance_Cf4_Cf4>` 
  *  :ref:`distance_sq (x:float4 const;y:float4 const) : float <function-_at_math_c__c_distance_sq_Cf4_Cf4>` 
  *  :ref:`inv_distance (x:float4 const;y:float4 const) : float <function-_at_math_c__c_inv_distance_Cf4_Cf4>` 
  *  :ref:`inv_distance_sq (x:float4 const;y:float4 const) : float <function-_at_math_c__c_inv_distance_sq_Cf4_Cf4>` 
  *  :ref:`reflect (v:float3 const;n:float3 const) : float3 <function-_at_math_c__c_reflect_Cf3_Cf3>` 
  *  :ref:`reflect (v:float2 const;n:float2 const) : float2 <function-_at_math_c__c_reflect_Cf2_Cf2>` 
  *  :ref:`refract (v:float3 const;n:float3 const;nint:float const) : float3 <function-_at_math_c__c_refract_Cf3_Cf3_Cf>` 
  *  :ref:`refract (v:float2 const;n:float2 const;nint:float const) : float2 <function-_at_math_c__c_refract_Cf2_Cf2_Cf>` 

.. _function-_at_math_c__c_cross_Cf3_Cf3:

.. das:function:: cross(x: float3 const; y: float3 const)

cross returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-cross|

.. _function-_at_math_c__c_distance_Cf2_Cf2:

.. das:function:: distance(x: float2 const; y: float2 const)

distance returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+
+y       +float2 const +
+--------+-------------+


|function-math-distance|

.. _function-_at_math_c__c_distance_sq_Cf2_Cf2:

.. das:function:: distance_sq(x: float2 const; y: float2 const)

distance_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+
+y       +float2 const +
+--------+-------------+


|function-math-distance_sq|

.. _function-_at_math_c__c_inv_distance_Cf2_Cf2:

.. das:function:: inv_distance(x: float2 const; y: float2 const)

inv_distance returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+
+y       +float2 const +
+--------+-------------+


|function-math-inv_distance|

.. _function-_at_math_c__c_inv_distance_sq_Cf2_Cf2:

.. das:function:: inv_distance_sq(x: float2 const; y: float2 const)

inv_distance_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+
+y       +float2 const +
+--------+-------------+


|function-math-inv_distance_sq|

.. _function-_at_math_c__c_distance_Cf3_Cf3:

.. das:function:: distance(x: float3 const; y: float3 const)

distance returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-distance|

.. _function-_at_math_c__c_distance_sq_Cf3_Cf3:

.. das:function:: distance_sq(x: float3 const; y: float3 const)

distance_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-distance_sq|

.. _function-_at_math_c__c_inv_distance_Cf3_Cf3:

.. das:function:: inv_distance(x: float3 const; y: float3 const)

inv_distance returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-inv_distance|

.. _function-_at_math_c__c_inv_distance_sq_Cf3_Cf3:

.. das:function:: inv_distance_sq(x: float3 const; y: float3 const)

inv_distance_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-inv_distance_sq|

.. _function-_at_math_c__c_distance_Cf4_Cf4:

.. das:function:: distance(x: float4 const; y: float4 const)

distance returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+
+y       +float4 const +
+--------+-------------+


|function-math-distance|

.. _function-_at_math_c__c_distance_sq_Cf4_Cf4:

.. das:function:: distance_sq(x: float4 const; y: float4 const)

distance_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+
+y       +float4 const +
+--------+-------------+


|function-math-distance_sq|

.. _function-_at_math_c__c_inv_distance_Cf4_Cf4:

.. das:function:: inv_distance(x: float4 const; y: float4 const)

inv_distance returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+
+y       +float4 const +
+--------+-------------+


|function-math-inv_distance|

.. _function-_at_math_c__c_inv_distance_sq_Cf4_Cf4:

.. das:function:: inv_distance_sq(x: float4 const; y: float4 const)

inv_distance_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+
+y       +float4 const +
+--------+-------------+


|function-math-inv_distance_sq|

.. _function-_at_math_c__c_reflect_Cf3_Cf3:

.. das:function:: reflect(v: float3 const; n: float3 const)

reflect returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+v       +float3 const +
+--------+-------------+
+n       +float3 const +
+--------+-------------+


returns vector representing reflection of vector v from normal n same as ::

    def reflect(v, n: float3)
        return v - 2. * dot(v, n) * n

.. _function-_at_math_c__c_reflect_Cf2_Cf2:

.. das:function:: reflect(v: float2 const; n: float2 const)

reflect returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+v       +float2 const +
+--------+-------------+
+n       +float2 const +
+--------+-------------+


returns vector representing reflection of vector v from normal n same as ::

    def reflect(v, n: float3)
        return v - 2. * dot(v, n) * n

.. _function-_at_math_c__c_refract_Cf3_Cf3_Cf:

.. das:function:: refract(v: float3 const; n: float3 const; nint: float const)

refract returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+v       +float3 const +
+--------+-------------+
+n       +float3 const +
+--------+-------------+
+nint    +float const  +
+--------+-------------+


returns vector representing refractoin of vector v from normal n same as ::

    def refract(v, n: float3; nint: float; outRefracted: float3&)
        let dt = dot(v, n)
        let discr = 1. - nint * nint * (1. - dt * dt)
        if discr > 0.
            outRefracted = nint * (v - n * dt) - n * sqrt(discr)
            return true
        return false

.. _function-_at_math_c__c_refract_Cf2_Cf2_Cf:

.. das:function:: refract(v: float2 const; n: float2 const; nint: float const)

refract returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+v       +float2 const +
+--------+-------------+
+n       +float2 const +
+--------+-------------+
+nint    +float const  +
+--------+-------------+


returns vector representing refractoin of vector v from normal n same as ::

    def refract(v, n: float3; nint: float; outRefracted: float3&)
        let dt = dot(v, n)
        let discr = 1. - nint * nint * (1. - dt * dt)
        if discr > 0.
            outRefracted = nint * (v - n * dt) - n * sqrt(discr)
            return true
        return false

++++++++++++++++++++++
float2, float3, float4
++++++++++++++++++++++

  *  :ref:`dot (x:float2 const;y:float2 const) : float <function-_at_math_c__c_dot_Cf2_Cf2>` 
  *  :ref:`dot (x:float3 const;y:float3 const) : float <function-_at_math_c__c_dot_Cf3_Cf3>` 
  *  :ref:`dot (x:float4 const;y:float4 const) : float <function-_at_math_c__c_dot_Cf4_Cf4>` 
  *  :ref:`fast_normalize (x:float2 const) : float2 <function-_at_math_c__c_fast_normalize_Cf2>` 
  *  :ref:`fast_normalize (x:float3 const) : float3 <function-_at_math_c__c_fast_normalize_Cf3>` 
  *  :ref:`fast_normalize (x:float4 const) : float4 <function-_at_math_c__c_fast_normalize_Cf4>` 
  *  :ref:`normalize (x:float2 const) : float2 <function-_at_math_c__c_normalize_Cf2>` 
  *  :ref:`normalize (x:float3 const) : float3 <function-_at_math_c__c_normalize_Cf3>` 
  *  :ref:`normalize (x:float4 const) : float4 <function-_at_math_c__c_normalize_Cf4>` 
  *  :ref:`length (x:float2 const) : float <function-_at_math_c__c_length_Cf2>` 
  *  :ref:`length (x:float3 const) : float <function-_at_math_c__c_length_Cf3>` 
  *  :ref:`length (x:float4 const) : float <function-_at_math_c__c_length_Cf4>` 
  *  :ref:`inv_length (x:float2 const) : float <function-_at_math_c__c_inv_length_Cf2>` 
  *  :ref:`inv_length (x:float3 const) : float <function-_at_math_c__c_inv_length_Cf3>` 
  *  :ref:`inv_length (x:float4 const) : float <function-_at_math_c__c_inv_length_Cf4>` 
  *  :ref:`inv_length_sq (x:float2 const) : float <function-_at_math_c__c_inv_length_sq_Cf2>` 
  *  :ref:`inv_length_sq (x:float3 const) : float <function-_at_math_c__c_inv_length_sq_Cf3>` 
  *  :ref:`inv_length_sq (x:float4 const) : float <function-_at_math_c__c_inv_length_sq_Cf4>` 
  *  :ref:`length_sq (x:float2 const) : float <function-_at_math_c__c_length_sq_Cf2>` 
  *  :ref:`length_sq (x:float3 const) : float <function-_at_math_c__c_length_sq_Cf3>` 
  *  :ref:`length_sq (x:float4 const) : float <function-_at_math_c__c_length_sq_Cf4>` 

.. _function-_at_math_c__c_dot_Cf2_Cf2:

.. das:function:: dot(x: float2 const; y: float2 const)

dot returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+
+y       +float2 const +
+--------+-------------+


|function-math-dot|

.. _function-_at_math_c__c_dot_Cf3_Cf3:

.. das:function:: dot(x: float3 const; y: float3 const)

dot returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+
+y       +float3 const +
+--------+-------------+


|function-math-dot|

.. _function-_at_math_c__c_dot_Cf4_Cf4:

.. das:function:: dot(x: float4 const; y: float4 const)

dot returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+
+y       +float4 const +
+--------+-------------+


|function-math-dot|

.. _function-_at_math_c__c_fast_normalize_Cf2:

.. das:function:: fast_normalize(x: float2 const)

fast_normalize returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-fast_normalize|

.. _function-_at_math_c__c_fast_normalize_Cf3:

.. das:function:: fast_normalize(x: float3 const)

fast_normalize returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-fast_normalize|

.. _function-_at_math_c__c_fast_normalize_Cf4:

.. das:function:: fast_normalize(x: float4 const)

fast_normalize returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-fast_normalize|

.. _function-_at_math_c__c_normalize_Cf2:

.. das:function:: normalize(x: float2 const)

normalize returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-normalize|

.. _function-_at_math_c__c_normalize_Cf3:

.. das:function:: normalize(x: float3 const)

normalize returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-normalize|

.. _function-_at_math_c__c_normalize_Cf4:

.. das:function:: normalize(x: float4 const)

normalize returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-normalize|

.. _function-_at_math_c__c_length_Cf2:

.. das:function:: length(x: float2 const)

length returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-length|

.. _function-_at_math_c__c_length_Cf3:

.. das:function:: length(x: float3 const)

length returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-length|

.. _function-_at_math_c__c_length_Cf4:

.. das:function:: length(x: float4 const)

length returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-length|

.. _function-_at_math_c__c_inv_length_Cf2:

.. das:function:: inv_length(x: float2 const)

inv_length returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-inv_length|

.. _function-_at_math_c__c_inv_length_Cf3:

.. das:function:: inv_length(x: float3 const)

inv_length returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-inv_length|

.. _function-_at_math_c__c_inv_length_Cf4:

.. das:function:: inv_length(x: float4 const)

inv_length returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-inv_length|

.. _function-_at_math_c__c_inv_length_sq_Cf2:

.. das:function:: inv_length_sq(x: float2 const)

inv_length_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-inv_length_sq|

.. _function-_at_math_c__c_inv_length_sq_Cf3:

.. das:function:: inv_length_sq(x: float3 const)

inv_length_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-inv_length_sq|

.. _function-_at_math_c__c_inv_length_sq_Cf4:

.. das:function:: inv_length_sq(x: float4 const)

inv_length_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-inv_length_sq|

.. _function-_at_math_c__c_length_sq_Cf2:

.. das:function:: length_sq(x: float2 const)

length_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float2 const +
+--------+-------------+


|function-math-length_sq|

.. _function-_at_math_c__c_length_sq_Cf3:

.. das:function:: length_sq(x: float3 const)

length_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float3 const +
+--------+-------------+


|function-math-length_sq|

.. _function-_at_math_c__c_length_sq_Cf4:

.. das:function:: length_sq(x: float4 const)

length_sq returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-length_sq|

+++++++++++++++
Noise functions
+++++++++++++++

  *  :ref:`uint32_hash (seed:uint const) : uint <function-_at_math_c__c_uint32_hash_Cu>` 
  *  :ref:`uint_noise_1D (position:int const;seed:uint const) : uint <function-_at_math_c__c_uint_noise_1D_Ci_Cu>` 
  *  :ref:`uint_noise_2D (position:int2 const;seed:uint const) : uint <function-_at_math_c__c_uint_noise_2D_Ci2_Cu>` 
  *  :ref:`uint_noise_3D (position:int3 const;seed:uint const) : uint <function-_at_math_c__c_uint_noise_3D_Ci3_Cu>` 

.. _function-_at_math_c__c_uint32_hash_Cu:

.. das:function:: uint32_hash(seed: uint const)

uint32_hash returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+seed    +uint const   +
+--------+-------------+


|function-math-uint32_hash|

.. _function-_at_math_c__c_uint_noise_1D_Ci_Cu:

.. das:function:: uint_noise_1D(position: int const; seed: uint const)

uint_noise_1D returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+position+int const    +
+--------+-------------+
+seed    +uint const   +
+--------+-------------+


|function-math-uint_noise_1D|

.. _function-_at_math_c__c_uint_noise_2D_Ci2_Cu:

.. das:function:: uint_noise_2D(position: int2 const; seed: uint const)

uint_noise_2D returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+position+int2 const   +
+--------+-------------+
+seed    +uint const   +
+--------+-------------+


|function-math-uint_noise_2D|

.. _function-_at_math_c__c_uint_noise_3D_Ci3_Cu:

.. das:function:: uint_noise_3D(position: int3 const; seed: uint const)

uint_noise_3D returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+position+int3 const   +
+--------+-------------+
+seed    +uint const   +
+--------+-------------+


|function-math-uint_noise_3D|

++++++++++++++
lerp/mad/clamp
++++++++++++++

  *  :ref:`mad (a:float const;b:float const;c:float const) : float <function-_at_math_c__c_mad_Cf_Cf_Cf>` 
  *  :ref:`lerp (a:float const;b:float const;t:float const) : float <function-_at_math_c__c_lerp_Cf_Cf_Cf>` 
  *  :ref:`mad (a:float2 const;b:float2 const;c:float2 const) : float2 <function-_at_math_c__c_mad_Cf2_Cf2_Cf2>` 
  *  :ref:`lerp (a:float2 const;b:float2 const;t:float2 const) : float2 <function-_at_math_c__c_lerp_Cf2_Cf2_Cf2>` 
  *  :ref:`mad (a:float3 const;b:float3 const;c:float3 const) : float3 <function-_at_math_c__c_mad_Cf3_Cf3_Cf3>` 
  *  :ref:`lerp (a:float3 const;b:float3 const;t:float3 const) : float3 <function-_at_math_c__c_lerp_Cf3_Cf3_Cf3>` 
  *  :ref:`mad (a:float4 const;b:float4 const;c:float4 const) : float4 <function-_at_math_c__c_mad_Cf4_Cf4_Cf4>` 
  *  :ref:`lerp (a:float4 const;b:float4 const;t:float4 const) : float4 <function-_at_math_c__c_lerp_Cf4_Cf4_Cf4>` 
  *  :ref:`mad (a:float2 const;b:float const;c:float2 const) : float2 <function-_at_math_c__c_mad_Cf2_Cf_Cf2>` 
  *  :ref:`mad (a:float3 const;b:float const;c:float3 const) : float3 <function-_at_math_c__c_mad_Cf3_Cf_Cf3>` 
  *  :ref:`mad (a:float4 const;b:float const;c:float4 const) : float4 <function-_at_math_c__c_mad_Cf4_Cf_Cf4>` 
  *  :ref:`mad (a:int const;b:int const;c:int const) : int <function-_at_math_c__c_mad_Ci_Ci_Ci>` 
  *  :ref:`mad (a:int2 const;b:int2 const;c:int2 const) : int2 <function-_at_math_c__c_mad_Ci2_Ci2_Ci2>` 
  *  :ref:`mad (a:int3 const;b:int3 const;c:int3 const) : int3 <function-_at_math_c__c_mad_Ci3_Ci3_Ci3>` 
  *  :ref:`mad (a:int4 const;b:int4 const;c:int4 const) : int4 <function-_at_math_c__c_mad_Ci4_Ci4_Ci4>` 
  *  :ref:`mad (a:int2 const;b:int const;c:int2 const) : int2 <function-_at_math_c__c_mad_Ci2_Ci_Ci2>` 
  *  :ref:`mad (a:int3 const;b:int const;c:int3 const) : int3 <function-_at_math_c__c_mad_Ci3_Ci_Ci3>` 
  *  :ref:`mad (a:int4 const;b:int const;c:int4 const) : int4 <function-_at_math_c__c_mad_Ci4_Ci_Ci4>` 
  *  :ref:`mad (a:uint const;b:uint const;c:uint const) : uint <function-_at_math_c__c_mad_Cu_Cu_Cu>` 
  *  :ref:`mad (a:uint2 const;b:uint2 const;c:uint2 const) : uint2 <function-_at_math_c__c_mad_Cu2_Cu2_Cu2>` 
  *  :ref:`mad (a:uint3 const;b:uint3 const;c:uint3 const) : uint3 <function-_at_math_c__c_mad_Cu3_Cu3_Cu3>` 
  *  :ref:`mad (a:uint4 const;b:uint4 const;c:uint4 const) : uint4 <function-_at_math_c__c_mad_Cu4_Cu4_Cu4>` 
  *  :ref:`mad (a:uint2 const;b:uint const;c:uint2 const) : uint2 <function-_at_math_c__c_mad_Cu2_Cu_Cu2>` 
  *  :ref:`mad (a:uint3 const;b:uint const;c:uint3 const) : uint3 <function-_at_math_c__c_mad_Cu3_Cu_Cu3>` 
  *  :ref:`mad (a:uint4 const;b:uint const;c:uint4 const) : uint4 <function-_at_math_c__c_mad_Cu4_Cu_Cu4>` 
  *  :ref:`mad (a:double const;b:double const;c:double const) : double <function-_at_math_c__c_mad_Cd_Cd_Cd>` 
  *  :ref:`lerp (a:double const;b:double const;t:double const) : double <function-_at_math_c__c_lerp_Cd_Cd_Cd>` 
  *  :ref:`clamp (t:int const;a:int const;b:int const) : int <function-_at_math_c__c_clamp_Ci_Ci_Ci>` 
  *  :ref:`clamp (t:int2 const;a:int2 const;b:int2 const) : int2 <function-_at_math_c__c_clamp_Ci2_Ci2_Ci2>` 
  *  :ref:`clamp (t:int3 const;a:int3 const;b:int3 const) : int3 <function-_at_math_c__c_clamp_Ci3_Ci3_Ci3>` 
  *  :ref:`clamp (t:int4 const;a:int4 const;b:int4 const) : int4 <function-_at_math_c__c_clamp_Ci4_Ci4_Ci4>` 
  *  :ref:`clamp (t:uint const;a:uint const;b:uint const) : uint <function-_at_math_c__c_clamp_Cu_Cu_Cu>` 
  *  :ref:`clamp (t:uint2 const;a:uint2 const;b:uint2 const) : uint2 <function-_at_math_c__c_clamp_Cu2_Cu2_Cu2>` 
  *  :ref:`clamp (t:uint3 const;a:uint3 const;b:uint3 const) : uint3 <function-_at_math_c__c_clamp_Cu3_Cu3_Cu3>` 
  *  :ref:`clamp (t:uint4 const;a:uint4 const;b:uint4 const) : uint4 <function-_at_math_c__c_clamp_Cu4_Cu4_Cu4>` 
  *  :ref:`clamp (t:float const;a:float const;b:float const) : float <function-_at_math_c__c_clamp_Cf_Cf_Cf>` 
  *  :ref:`clamp (t:float2 const;a:float2 const;b:float2 const) : float2 <function-_at_math_c__c_clamp_Cf2_Cf2_Cf2>` 
  *  :ref:`clamp (t:float3 const;a:float3 const;b:float3 const) : float3 <function-_at_math_c__c_clamp_Cf3_Cf3_Cf3>` 
  *  :ref:`clamp (t:float4 const;a:float4 const;b:float4 const) : float4 <function-_at_math_c__c_clamp_Cf4_Cf4_Cf4>` 
  *  :ref:`clamp (t:double const;a:double const;b:double const) : double <function-_at_math_c__c_clamp_Cd_Cd_Cd>` 
  *  :ref:`clamp (t:int64 const;a:int64 const;b:int64 const) : int64 <function-_at_math_c__c_clamp_Ci64_Ci64_Ci64>` 
  *  :ref:`clamp (t:uint64 const;a:uint64 const;b:uint64 const) : uint64 <function-_at_math_c__c_clamp_Cu64_Cu64_Cu64>` 
  *  :ref:`lerp (a:float2 const;b:float2 const;t:float const) : float2 <function-_at_math_c__c_lerp_Cf2_Cf2_Cf>` 
  *  :ref:`lerp (a:float3 const;b:float3 const;t:float const) : float3 <function-_at_math_c__c_lerp_Cf3_Cf3_Cf>` 
  *  :ref:`lerp (a:float4 const;b:float4 const;t:float const) : float4 <function-_at_math_c__c_lerp_Cf4_Cf4_Cf>` 

.. _function-_at_math_c__c_mad_Cf_Cf_Cf:

.. das:function:: mad(a: float const; b: float const; c: float const)

mad returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float const  +
+--------+-------------+
+b       +float const  +
+--------+-------------+
+c       +float const  +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_lerp_Cf_Cf_Cf:

.. das:function:: lerp(a: float const; b: float const; t: float const)

lerp returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float const  +
+--------+-------------+
+b       +float const  +
+--------+-------------+
+t       +float const  +
+--------+-------------+


|function-math-lerp|

.. _function-_at_math_c__c_mad_Cf2_Cf2_Cf2:

.. das:function:: mad(a: float2 const; b: float2 const; c: float2 const)

mad returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float2 const +
+--------+-------------+
+b       +float2 const +
+--------+-------------+
+c       +float2 const +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_lerp_Cf2_Cf2_Cf2:

.. das:function:: lerp(a: float2 const; b: float2 const; t: float2 const)

lerp returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float2 const +
+--------+-------------+
+b       +float2 const +
+--------+-------------+
+t       +float2 const +
+--------+-------------+


|function-math-lerp|

.. _function-_at_math_c__c_mad_Cf3_Cf3_Cf3:

.. das:function:: mad(a: float3 const; b: float3 const; c: float3 const)

mad returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float3 const +
+--------+-------------+
+b       +float3 const +
+--------+-------------+
+c       +float3 const +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_lerp_Cf3_Cf3_Cf3:

.. das:function:: lerp(a: float3 const; b: float3 const; t: float3 const)

lerp returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float3 const +
+--------+-------------+
+b       +float3 const +
+--------+-------------+
+t       +float3 const +
+--------+-------------+


|function-math-lerp|

.. _function-_at_math_c__c_mad_Cf4_Cf4_Cf4:

.. das:function:: mad(a: float4 const; b: float4 const; c: float4 const)

mad returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float4 const +
+--------+-------------+
+b       +float4 const +
+--------+-------------+
+c       +float4 const +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_lerp_Cf4_Cf4_Cf4:

.. das:function:: lerp(a: float4 const; b: float4 const; t: float4 const)

lerp returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float4 const +
+--------+-------------+
+b       +float4 const +
+--------+-------------+
+t       +float4 const +
+--------+-------------+


|function-math-lerp|

.. _function-_at_math_c__c_mad_Cf2_Cf_Cf2:

.. das:function:: mad(a: float2 const; b: float const; c: float2 const)

mad returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float2 const +
+--------+-------------+
+b       +float const  +
+--------+-------------+
+c       +float2 const +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cf3_Cf_Cf3:

.. das:function:: mad(a: float3 const; b: float const; c: float3 const)

mad returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float3 const +
+--------+-------------+
+b       +float const  +
+--------+-------------+
+c       +float3 const +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cf4_Cf_Cf4:

.. das:function:: mad(a: float4 const; b: float const; c: float4 const)

mad returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float4 const +
+--------+-------------+
+b       +float const  +
+--------+-------------+
+c       +float4 const +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Ci_Ci_Ci:

.. das:function:: mad(a: int const; b: int const; c: int const)

mad returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +int const    +
+--------+-------------+
+b       +int const    +
+--------+-------------+
+c       +int const    +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Ci2_Ci2_Ci2:

.. das:function:: mad(a: int2 const; b: int2 const; c: int2 const)

mad returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +int2 const   +
+--------+-------------+
+b       +int2 const   +
+--------+-------------+
+c       +int2 const   +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Ci3_Ci3_Ci3:

.. das:function:: mad(a: int3 const; b: int3 const; c: int3 const)

mad returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +int3 const   +
+--------+-------------+
+b       +int3 const   +
+--------+-------------+
+c       +int3 const   +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Ci4_Ci4_Ci4:

.. das:function:: mad(a: int4 const; b: int4 const; c: int4 const)

mad returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +int4 const   +
+--------+-------------+
+b       +int4 const   +
+--------+-------------+
+c       +int4 const   +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Ci2_Ci_Ci2:

.. das:function:: mad(a: int2 const; b: int const; c: int2 const)

mad returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +int2 const   +
+--------+-------------+
+b       +int const    +
+--------+-------------+
+c       +int2 const   +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Ci3_Ci_Ci3:

.. das:function:: mad(a: int3 const; b: int const; c: int3 const)

mad returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +int3 const   +
+--------+-------------+
+b       +int const    +
+--------+-------------+
+c       +int3 const   +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Ci4_Ci_Ci4:

.. das:function:: mad(a: int4 const; b: int const; c: int4 const)

mad returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +int4 const   +
+--------+-------------+
+b       +int const    +
+--------+-------------+
+c       +int4 const   +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cu_Cu_Cu:

.. das:function:: mad(a: uint const; b: uint const; c: uint const)

mad returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +uint const   +
+--------+-------------+
+b       +uint const   +
+--------+-------------+
+c       +uint const   +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cu2_Cu2_Cu2:

.. das:function:: mad(a: uint2 const; b: uint2 const; c: uint2 const)

mad returns uint2

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +uint2 const  +
+--------+-------------+
+b       +uint2 const  +
+--------+-------------+
+c       +uint2 const  +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cu3_Cu3_Cu3:

.. das:function:: mad(a: uint3 const; b: uint3 const; c: uint3 const)

mad returns uint3

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +uint3 const  +
+--------+-------------+
+b       +uint3 const  +
+--------+-------------+
+c       +uint3 const  +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cu4_Cu4_Cu4:

.. das:function:: mad(a: uint4 const; b: uint4 const; c: uint4 const)

mad returns uint4

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +uint4 const  +
+--------+-------------+
+b       +uint4 const  +
+--------+-------------+
+c       +uint4 const  +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cu2_Cu_Cu2:

.. das:function:: mad(a: uint2 const; b: uint const; c: uint2 const)

mad returns uint2

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +uint2 const  +
+--------+-------------+
+b       +uint const   +
+--------+-------------+
+c       +uint2 const  +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cu3_Cu_Cu3:

.. das:function:: mad(a: uint3 const; b: uint const; c: uint3 const)

mad returns uint3

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +uint3 const  +
+--------+-------------+
+b       +uint const   +
+--------+-------------+
+c       +uint3 const  +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cu4_Cu_Cu4:

.. das:function:: mad(a: uint4 const; b: uint const; c: uint4 const)

mad returns uint4

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +uint4 const  +
+--------+-------------+
+b       +uint const   +
+--------+-------------+
+c       +uint4 const  +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_mad_Cd_Cd_Cd:

.. das:function:: mad(a: double const; b: double const; c: double const)

mad returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +double const +
+--------+-------------+
+b       +double const +
+--------+-------------+
+c       +double const +
+--------+-------------+


|function-math-mad|

.. _function-_at_math_c__c_lerp_Cd_Cd_Cd:

.. das:function:: lerp(a: double const; b: double const; t: double const)

lerp returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +double const +
+--------+-------------+
+b       +double const +
+--------+-------------+
+t       +double const +
+--------+-------------+


|function-math-lerp|

.. _function-_at_math_c__c_clamp_Ci_Ci_Ci:

.. das:function:: clamp(t: int const; a: int const; b: int const)

clamp returns int

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +int const    +
+--------+-------------+
+a       +int const    +
+--------+-------------+
+b       +int const    +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Ci2_Ci2_Ci2:

.. das:function:: clamp(t: int2 const; a: int2 const; b: int2 const)

clamp returns int2

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +int2 const   +
+--------+-------------+
+a       +int2 const   +
+--------+-------------+
+b       +int2 const   +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Ci3_Ci3_Ci3:

.. das:function:: clamp(t: int3 const; a: int3 const; b: int3 const)

clamp returns int3

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +int3 const   +
+--------+-------------+
+a       +int3 const   +
+--------+-------------+
+b       +int3 const   +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Ci4_Ci4_Ci4:

.. das:function:: clamp(t: int4 const; a: int4 const; b: int4 const)

clamp returns int4

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +int4 const   +
+--------+-------------+
+a       +int4 const   +
+--------+-------------+
+b       +int4 const   +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cu_Cu_Cu:

.. das:function:: clamp(t: uint const; a: uint const; b: uint const)

clamp returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +uint const   +
+--------+-------------+
+a       +uint const   +
+--------+-------------+
+b       +uint const   +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cu2_Cu2_Cu2:

.. das:function:: clamp(t: uint2 const; a: uint2 const; b: uint2 const)

clamp returns uint2

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +uint2 const  +
+--------+-------------+
+a       +uint2 const  +
+--------+-------------+
+b       +uint2 const  +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cu3_Cu3_Cu3:

.. das:function:: clamp(t: uint3 const; a: uint3 const; b: uint3 const)

clamp returns uint3

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +uint3 const  +
+--------+-------------+
+a       +uint3 const  +
+--------+-------------+
+b       +uint3 const  +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cu4_Cu4_Cu4:

.. das:function:: clamp(t: uint4 const; a: uint4 const; b: uint4 const)

clamp returns uint4

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +uint4 const  +
+--------+-------------+
+a       +uint4 const  +
+--------+-------------+
+b       +uint4 const  +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cf_Cf_Cf:

.. das:function:: clamp(t: float const; a: float const; b: float const)

clamp returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +float const  +
+--------+-------------+
+a       +float const  +
+--------+-------------+
+b       +float const  +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cf2_Cf2_Cf2:

.. das:function:: clamp(t: float2 const; a: float2 const; b: float2 const)

clamp returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +float2 const +
+--------+-------------+
+a       +float2 const +
+--------+-------------+
+b       +float2 const +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cf3_Cf3_Cf3:

.. das:function:: clamp(t: float3 const; a: float3 const; b: float3 const)

clamp returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +float3 const +
+--------+-------------+
+a       +float3 const +
+--------+-------------+
+b       +float3 const +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cf4_Cf4_Cf4:

.. das:function:: clamp(t: float4 const; a: float4 const; b: float4 const)

clamp returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +float4 const +
+--------+-------------+
+a       +float4 const +
+--------+-------------+
+b       +float4 const +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cd_Cd_Cd:

.. das:function:: clamp(t: double const; a: double const; b: double const)

clamp returns double

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +double const +
+--------+-------------+
+a       +double const +
+--------+-------------+
+b       +double const +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Ci64_Ci64_Ci64:

.. das:function:: clamp(t: int64 const; a: int64 const; b: int64 const)

clamp returns int64

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +int64 const  +
+--------+-------------+
+a       +int64 const  +
+--------+-------------+
+b       +int64 const  +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_clamp_Cu64_Cu64_Cu64:

.. das:function:: clamp(t: uint64 const; a: uint64 const; b: uint64 const)

clamp returns uint64

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +uint64 const +
+--------+-------------+
+a       +uint64 const +
+--------+-------------+
+b       +uint64 const +
+--------+-------------+


|function-math-clamp|

.. _function-_at_math_c__c_lerp_Cf2_Cf2_Cf:

.. das:function:: lerp(a: float2 const; b: float2 const; t: float const)

lerp returns float2

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float2 const +
+--------+-------------+
+b       +float2 const +
+--------+-------------+
+t       +float const  +
+--------+-------------+


|function-math-lerp|

.. _function-_at_math_c__c_lerp_Cf3_Cf3_Cf:

.. das:function:: lerp(a: float3 const; b: float3 const; t: float const)

lerp returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float3 const +
+--------+-------------+
+b       +float3 const +
+--------+-------------+
+t       +float const  +
+--------+-------------+


|function-math-lerp|

.. _function-_at_math_c__c_lerp_Cf4_Cf4_Cf:

.. das:function:: lerp(a: float4 const; b: float4 const; t: float const)

lerp returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+a       +float4 const +
+--------+-------------+
+b       +float4 const +
+--------+-------------+
+t       +float const  +
+--------+-------------+


|function-math-lerp|

+++++++++++++++++
Matrix operations
+++++++++++++++++

  *  :ref:`* (x:math::float4x4 const implicit;y:math::float4x4 const implicit) : math::float4x4 <function-_at_math_c__c_*_CIH_ls_math_c__c_float4x4_gr__CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`== (x:math::float4x4 const implicit;y:math::float4x4 const implicit) : bool <function-_at_math_c__c__eq__eq__CIH_ls_math_c__c_float4x4_gr__CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`\!= (x:math::float4x4 const implicit;y:math::float4x4 const implicit) : bool <function-_at_math_c__c__ex__eq__CIH_ls_math_c__c_float4x4_gr__CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`* (x:math::float3x4 const implicit;y:math::float3x4 const implicit) : math::float3x4 <function-_at_math_c__c_*_CIH_ls_math_c__c_float3x4_gr__CIH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`* (x:math::float3x4 const implicit;y:float3 const) : float3 <function-_at_math_c__c_*_CIH_ls_math_c__c_float3x4_gr__Cf3>` 
  *  :ref:`* (x:math::float4x4 const implicit;y:float4 const) : float4 <function-_at_math_c__c_*_CIH_ls_math_c__c_float4x4_gr__Cf4>` 
  *  :ref:`== (x:math::float3x4 const implicit;y:math::float3x4 const implicit) : bool <function-_at_math_c__c__eq__eq__CIH_ls_math_c__c_float3x4_gr__CIH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`\!= (x:math::float3x4 const implicit;y:math::float3x4 const implicit) : bool <function-_at_math_c__c__ex__eq__CIH_ls_math_c__c_float3x4_gr__CIH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`* (x:math::float3x3 const implicit;y:math::float3x3 const implicit) : math::float3x3 <function-_at_math_c__c_*_CIH_ls_math_c__c_float3x3_gr__CIH_ls_math_c__c_float3x3_gr_>` 
  *  :ref:`* (x:math::float3x3 const implicit;y:float3 const) : float3 <function-_at_math_c__c_*_CIH_ls_math_c__c_float3x3_gr__Cf3>` 
  *  :ref:`== (x:math::float3x3 const implicit;y:math::float3x3 const implicit) : bool <function-_at_math_c__c__eq__eq__CIH_ls_math_c__c_float3x3_gr__CIH_ls_math_c__c_float3x3_gr_>` 
  *  :ref:`\!= (x:math::float3x3 const implicit;y:math::float3x3 const implicit) : bool <function-_at_math_c__c__ex__eq__CIH_ls_math_c__c_float3x3_gr__CIH_ls_math_c__c_float3x3_gr_>` 

.. _function-_at_math_c__c_*_CIH_ls_math_c__c_float4x4_gr__CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: operator *(x: float4x4 const implicit; y: float4x4 const implicit)

* returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-*|

.. _function-_at_math_c__c__eq__eq__CIH_ls_math_c__c_float4x4_gr__CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: operator ==(x: float4x4 const implicit; y: float4x4 const implicit)

== returns bool

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-==|

.. _function-_at_math_c__c__ex__eq__CIH_ls_math_c__c_float4x4_gr__CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: operator !=(x: float4x4 const implicit; y: float4x4 const implicit)

!= returns bool

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-!=|

.. _function-_at_math_c__c_*_CIH_ls_math_c__c_float3x4_gr__CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: operator *(x: float3x4 const implicit; y: float3x4 const implicit)

* returns  :ref:`math::float3x4 <handle-math-float3x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-*|

.. _function-_at_math_c__c_*_CIH_ls_math_c__c_float3x4_gr__Cf3:

.. das:function:: operator *(x: float3x4 const implicit; y: float3 const)

* returns float3

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       +float3 const                                                 +
+--------+-------------------------------------------------------------+


|function-math-*|

.. _function-_at_math_c__c_*_CIH_ls_math_c__c_float4x4_gr__Cf4:

.. das:function:: operator *(x: float4x4 const implicit; y: float4 const)

* returns float4

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       +float4 const                                                 +
+--------+-------------------------------------------------------------+


|function-math-*|

.. _function-_at_math_c__c__eq__eq__CIH_ls_math_c__c_float3x4_gr__CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: operator ==(x: float3x4 const implicit; y: float3x4 const implicit)

== returns bool

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-==|

.. _function-_at_math_c__c__ex__eq__CIH_ls_math_c__c_float3x4_gr__CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: operator !=(x: float3x4 const implicit; y: float3x4 const implicit)

!= returns bool

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-!=|

.. _function-_at_math_c__c_*_CIH_ls_math_c__c_float3x3_gr__CIH_ls_math_c__c_float3x3_gr_:

.. das:function:: operator *(x: float3x3 const implicit; y: float3x3 const implicit)

* returns  :ref:`math::float3x3 <handle-math-float3x3>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-*|

.. _function-_at_math_c__c_*_CIH_ls_math_c__c_float3x3_gr__Cf3:

.. das:function:: operator *(x: float3x3 const implicit; y: float3 const)

* returns float3

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+
+y       +float3 const                                                 +
+--------+-------------------------------------------------------------+


|function-math-*|

.. _function-_at_math_c__c__eq__eq__CIH_ls_math_c__c_float3x3_gr__CIH_ls_math_c__c_float3x3_gr_:

.. das:function:: operator ==(x: float3x3 const implicit; y: float3x3 const implicit)

== returns bool

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-==|

.. _function-_at_math_c__c__ex__eq__CIH_ls_math_c__c_float3x3_gr__CIH_ls_math_c__c_float3x3_gr_:

.. das:function:: operator !=(x: float3x3 const implicit; y: float3x3 const implicit)

!= returns bool

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+
+y       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-!=|

+++++++++++++++++++
Matrix initializers
+++++++++++++++++++

  *  :ref:`float3x3 () : math::float3x3 <function-_at_math_c__c_float3x3>` 
  *  :ref:`float3x4 () : math::float3x4 <function-_at_math_c__c_float3x4>` 
  *  :ref:`float4x4 () : math::float4x4 <function-_at_math_c__c_float4x4>` 
  *  :ref:`float4x4 (arg0:math::float3x4 const implicit) : math::float4x4 <function-_at_math_c__c_float4x4_CIH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`identity4x4 () : math::float4x4 <function-_at_math_c__c_identity4x4>` 
  *  :ref:`float3x4 (arg0:math::float4x4 const implicit) : math::float3x4 <function-_at_math_c__c_float3x4_CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`identity3x4 () : math::float3x4 <function-_at_math_c__c_identity3x4>` 
  *  :ref:`float3x3 (arg0:math::float4x4 const implicit) : math::float3x3 <function-_at_math_c__c_float3x3_CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`float3x3 (arg0:math::float3x4 const implicit) : math::float3x3 <function-_at_math_c__c_float3x3_CIH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`identity3x3 () : math::float3x3 <function-_at_math_c__c_identity3x3>` 

.. _function-_at_math_c__c_float3x3:

.. das:function:: float3x3()

float3x3 returns  :ref:`math::float3x3 <handle-math-float3x3>` 

|function-math-float3x3|

.. _function-_at_math_c__c_float3x4:

.. das:function:: float3x4()

float3x4 returns  :ref:`math::float3x4 <handle-math-float3x4>` 

|function-math-float3x4|

.. _function-_at_math_c__c_float4x4:

.. das:function:: float4x4()

float4x4 returns  :ref:`math::float4x4 <handle-math-float4x4>` 

|function-math-float4x4|

.. _function-_at_math_c__c_float4x4_CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: float4x4(arg0: float3x4 const implicit)

float4x4 returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+arg0    + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-float4x4|

.. _function-_at_math_c__c_identity4x4:

.. das:function:: identity4x4()

identity4x4 returns  :ref:`math::float4x4 <handle-math-float4x4>` 

|function-math-identity4x4|

.. _function-_at_math_c__c_float3x4_CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: float3x4(arg0: float4x4 const implicit)

float3x4 returns  :ref:`math::float3x4 <handle-math-float3x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+arg0    + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-float3x4|

.. _function-_at_math_c__c_identity3x4:

.. das:function:: identity3x4()

identity3x4 returns  :ref:`math::float3x4 <handle-math-float3x4>` 

|function-math-identity3x4|

.. _function-_at_math_c__c_float3x3_CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: float3x3(arg0: float4x4 const implicit)

float3x3 returns  :ref:`math::float3x3 <handle-math-float3x3>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+arg0    + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-float3x3|

.. _function-_at_math_c__c_float3x3_CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: float3x3(arg0: float3x4 const implicit)

float3x3 returns  :ref:`math::float3x3 <handle-math-float3x3>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+arg0    + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-float3x3|

.. _function-_at_math_c__c_identity3x3:

.. das:function:: identity3x3()

identity3x3 returns  :ref:`math::float3x3 <handle-math-float3x3>` 

|function-math-identity3x3|

+++++++++++++++++++
Matrix manipulation
+++++++++++++++++++

  *  :ref:`identity (x:math::float4x4 implicit) : void <function-_at_math_c__c_identity_IH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`translation (xyz:float3 const) : math::float4x4 <function-_at_math_c__c_translation_Cf3>` 
  *  :ref:`transpose (x:math::float4x4 const implicit) : math::float4x4 <function-_at_math_c__c_transpose_CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`persp_forward (wk:float const;hk:float const;zn:float const;zf:float const) : math::float4x4 <function-_at_math_c__c_persp_forward_Cf_Cf_Cf_Cf>` 
  *  :ref:`persp_reverse (wk:float const;hk:float const;zn:float const;zf:float const) : math::float4x4 <function-_at_math_c__c_persp_reverse_Cf_Cf_Cf_Cf>` 
  *  :ref:`look_at (eye:float3 const;at:float3 const;up:float3 const) : math::float4x4 <function-_at_math_c__c_look_at_Cf3_Cf3_Cf3>` 
  *  :ref:`compose (pos:float3 const;rot:float4 const;scale:float3 const) : math::float4x4 <function-_at_math_c__c_compose_Cf3_Cf4_Cf3>` 
  *  :ref:`decompose (mat:math::float4x4 const implicit;pos:float3& implicit;rot:float4& implicit;scale:float3& implicit) : void <function-_at_math_c__c_decompose_CIH_ls_math_c__c_float4x4_gr__&If3_&If4_&If3>` 
  *  :ref:`identity (x:math::float3x4 implicit) : void <function-_at_math_c__c_identity_IH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`inverse (x:math::float3x4 const implicit) : math::float3x4 <function-_at_math_c__c_inverse_CIH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`inverse (m:math::float4x4 const implicit) : math::float4x4 <function-_at_math_c__c_inverse_CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`orthonormal_inverse (m:math::float3x3 const implicit) : math::float3x3 <function-_at_math_c__c_orthonormal_inverse_CIH_ls_math_c__c_float3x3_gr_>` 
  *  :ref:`orthonormal_inverse (m:math::float3x4 const implicit) : math::float3x4 <function-_at_math_c__c_orthonormal_inverse_CIH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`rotate (x:math::float3x4 const implicit;y:float3 const) : float3 <function-_at_math_c__c_rotate_CIH_ls_math_c__c_float3x4_gr__Cf3>` 
  *  :ref:`identity (x:math::float3x3 implicit) : void <function-_at_math_c__c_identity_IH_ls_math_c__c_float3x3_gr_>` 

.. _function-_at_math_c__c_identity_IH_ls_math_c__c_float4x4_gr_:

.. das:function:: identity(x: float4x4 implicit)

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+x       + :ref:`math::float4x4 <handle-math-float4x4>`  implicit+
+--------+-------------------------------------------------------+


|function-math-identity|

.. _function-_at_math_c__c_translation_Cf3:

.. das:function:: translation(xyz: float3 const)

translation returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+xyz     +float3 const +
+--------+-------------+


produces a translation by xyz

 +---+---+---+---+
 + 1 | 0 | 0 | 0 |
 +---+---+---+---+
 + 0 | 1 | 0 | 0 |
 +---+---+---+---+
 + 0 | 0 | 1 | 0 |
 +---+---+---+---+
 + x | y | z | 1 |
 +---+---+---+---+


.. _function-_at_math_c__c_transpose_CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: transpose(x: float4x4 const implicit)

transpose returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-transpose|

.. _function-_at_math_c__c_persp_forward_Cf_Cf_Cf_Cf:

.. das:function:: persp_forward(wk: float const; hk: float const; zn: float const; zf: float const)

persp_forward returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+wk      +float const  +
+--------+-------------+
+hk      +float const  +
+--------+-------------+
+zn      +float const  +
+--------+-------------+
+zf      +float const  +
+--------+-------------+


|function-math-persp_forward|

.. _function-_at_math_c__c_persp_reverse_Cf_Cf_Cf_Cf:

.. das:function:: persp_reverse(wk: float const; hk: float const; zn: float const; zf: float const)

persp_reverse returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+wk      +float const  +
+--------+-------------+
+hk      +float const  +
+--------+-------------+
+zn      +float const  +
+--------+-------------+
+zf      +float const  +
+--------+-------------+


|function-math-persp_reverse|

.. _function-_at_math_c__c_look_at_Cf3_Cf3_Cf3:

.. das:function:: look_at(eye: float3 const; at: float3 const; up: float3 const)

look_at returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+eye     +float3 const +
+--------+-------------+
+at      +float3 const +
+--------+-------------+
+up      +float3 const +
+--------+-------------+


|function-math-look_at|

.. _function-_at_math_c__c_compose_Cf3_Cf4_Cf3:

.. das:function:: compose(pos: float3 const; rot: float4 const; scale: float3 const)

compose returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+pos     +float3 const +
+--------+-------------+
+rot     +float4 const +
+--------+-------------+
+scale   +float3 const +
+--------+-------------+


|function-math-compose|

.. _function-_at_math_c__c_decompose_CIH_ls_math_c__c_float4x4_gr__&If3_&If4_&If3:

.. das:function:: decompose(mat: float4x4 const implicit; pos: float3& implicit; rot: float4& implicit; scale: float3& implicit)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+mat     + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+
+pos     +float3& implicit                                             +
+--------+-------------------------------------------------------------+
+rot     +float4& implicit                                             +
+--------+-------------------------------------------------------------+
+scale   +float3& implicit                                             +
+--------+-------------------------------------------------------------+


|function-math-decompose|

.. _function-_at_math_c__c_identity_IH_ls_math_c__c_float3x4_gr_:

.. das:function:: identity(x: float3x4 implicit)

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  implicit+
+--------+-------------------------------------------------------+


|function-math-identity|

.. _function-_at_math_c__c_inverse_CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: inverse(x: float3x4 const implicit)

inverse returns  :ref:`math::float3x4 <handle-math-float3x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-inverse|

.. _function-_at_math_c__c_inverse_CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: inverse(m: float4x4 const implicit)

inverse returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+m       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-inverse|

.. _function-_at_math_c__c_orthonormal_inverse_CIH_ls_math_c__c_float3x3_gr_:

.. das:function:: orthonormal_inverse(m: float3x3 const implicit)

orthonormal_inverse returns  :ref:`math::float3x3 <handle-math-float3x3>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+m       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-orthonormal_inverse|

.. _function-_at_math_c__c_orthonormal_inverse_CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: orthonormal_inverse(m: float3x4 const implicit)

orthonormal_inverse returns  :ref:`math::float3x4 <handle-math-float3x4>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+m       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-orthonormal_inverse|

.. _function-_at_math_c__c_rotate_CIH_ls_math_c__c_float3x4_gr__Cf3:

.. das:function:: rotate(x: float3x4 const implicit; y: float3 const)

rotate returns float3

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+
+y       +float3 const                                                 +
+--------+-------------------------------------------------------------+


|function-math-rotate|

.. _function-_at_math_c__c_identity_IH_ls_math_c__c_float3x3_gr_:

.. das:function:: identity(x: float3x3 implicit)

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+x       + :ref:`math::float3x3 <handle-math-float3x3>`  implicit+
+--------+-------------------------------------------------------+


|function-math-identity|

+++++++++++++++++++++
Quaternion operations
+++++++++++++++++++++

  *  :ref:`quat_from_unit_arc (v0:float3 const;v1:float3 const) : float4 <function-_at_math_c__c_quat_from_unit_arc_Cf3_Cf3>` 
  *  :ref:`quat_from_unit_vec_ang (v:float3 const;ang:float const) : float4 <function-_at_math_c__c_quat_from_unit_vec_ang_Cf3_Cf>` 
  *  :ref:`un_quat (m:math::float4x4 const implicit) : float4 <function-_at_math_c__c_un_quat_CIH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`quat_mul (q1:float4 const;q2:float4 const) : float4 <function-_at_math_c__c_quat_mul_Cf4_Cf4>` 
  *  :ref:`quat_mul_vec (q:float4 const;v:float3 const) : float3 <function-_at_math_c__c_quat_mul_vec_Cf4_Cf3>` 
  *  :ref:`quat_conjugate (q:float4 const) : float4 <function-_at_math_c__c_quat_conjugate_Cf4>` 

.. _function-_at_math_c__c_quat_from_unit_arc_Cf3_Cf3:

.. das:function:: quat_from_unit_arc(v0: float3 const; v1: float3 const)

quat_from_unit_arc returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+v0      +float3 const +
+--------+-------------+
+v1      +float3 const +
+--------+-------------+


|function-math-quat_from_unit_arc|

.. _function-_at_math_c__c_quat_from_unit_vec_ang_Cf3_Cf:

.. das:function:: quat_from_unit_vec_ang(v: float3 const; ang: float const)

quat_from_unit_vec_ang returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+v       +float3 const +
+--------+-------------+
+ang     +float const  +
+--------+-------------+


|function-math-quat_from_unit_vec_ang|

.. _function-_at_math_c__c_un_quat_CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: un_quat(m: float4x4 const implicit)

un_quat returns float4

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+m       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-un_quat|

.. _function-_at_math_c__c_quat_mul_Cf4_Cf4:

.. das:function:: quat_mul(q1: float4 const; q2: float4 const)

quat_mul returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+q1      +float4 const +
+--------+-------------+
+q2      +float4 const +
+--------+-------------+


|function-math-quat_mul|

.. _function-_at_math_c__c_quat_mul_vec_Cf4_Cf3:

.. das:function:: quat_mul_vec(q: float4 const; v: float3 const)

quat_mul_vec returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+q       +float4 const +
+--------+-------------+
+v       +float3 const +
+--------+-------------+


|function-math-quat_mul_vec|

.. _function-_at_math_c__c_quat_conjugate_Cf4:

.. das:function:: quat_conjugate(q: float4 const)

quat_conjugate returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+q       +float4 const +
+--------+-------------+


|function-math-quat_conjugate|

+++++++++++++++++++++
Packing and unpacking
+++++++++++++++++++++

  *  :ref:`pack_float_to_byte (x:float4 const) : uint <function-_at_math_c__c_pack_float_to_byte_Cf4>` 
  *  :ref:`unpack_byte_to_float (x:uint const) : float4 <function-_at_math_c__c_unpack_byte_to_float_Cu>` 

.. _function-_at_math_c__c_pack_float_to_byte_Cf4:

.. das:function:: pack_float_to_byte(x: float4 const)

pack_float_to_byte returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float4 const +
+--------+-------------+


|function-math-pack_float_to_byte|

.. _function-_at_math_c__c_unpack_byte_to_float_Cu:

.. das:function:: unpack_byte_to_float(x: uint const)

unpack_byte_to_float returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint const   +
+--------+-------------+


|function-math-unpack_byte_to_float|

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_math_c__c_[]_I_eq_H_ls_math_c__c_float4x4_gr__Ci_C_c_C_l:

.. das:function:: operator [](m: float4x4 implicit ==const; i: int const)

[] returns float4&

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+m       + :ref:`math::float4x4 <handle-math-float4x4>`  implicit!+
+--------+--------------------------------------------------------+
+i       +int const                                               +
+--------+--------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_CI_eq_H_ls_math_c__c_float4x4_gr__Ci_C_c_C_l:

.. das:function:: operator [](m: float4x4 const implicit ==const; i: int const)

[] returns float4 const&

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+m       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit!+
+--------+--------------------------------------------------------------+
+i       +int const                                                     +
+--------+--------------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_I_eq_H_ls_math_c__c_float4x4_gr__Cu_C_c_C_l:

.. das:function:: operator [](m: float4x4 implicit ==const; i: uint const)

[] returns float4&

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+m       + :ref:`math::float4x4 <handle-math-float4x4>`  implicit!+
+--------+--------------------------------------------------------+
+i       +uint const                                              +
+--------+--------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_CI_eq_H_ls_math_c__c_float4x4_gr__Cu_C_c_C_l:

.. das:function:: operator [](m: float4x4 const implicit ==const; i: uint const)

[] returns float4 const&

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+m       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit!+
+--------+--------------------------------------------------------------+
+i       +uint const                                                    +
+--------+--------------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_determinant_CIH_ls_math_c__c_float4x4_gr_:

.. das:function:: determinant(x: float4x4 const implicit)

determinant returns float

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float4x4 <handle-math-float4x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-determinant|

.. _function-_at_math_c__c_determinant_CIH_ls_math_c__c_float3x4_gr_:

.. das:function:: determinant(x: float3x4 const implicit)

determinant returns float

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-determinant|

.. _function-_at_math_c__c_[]_I_eq_H_ls_math_c__c_float3x4_gr__Ci_C_c_C_l:

.. das:function:: operator [](m: float3x4 implicit ==const; i: int const)

[] returns float3&

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+m       + :ref:`math::float3x4 <handle-math-float3x4>`  implicit!+
+--------+--------------------------------------------------------+
+i       +int const                                               +
+--------+--------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_CI_eq_H_ls_math_c__c_float3x4_gr__Ci_C_c_C_l:

.. das:function:: operator [](m: float3x4 const implicit ==const; i: int const)

[] returns float3 const&

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+m       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit!+
+--------+--------------------------------------------------------------+
+i       +int const                                                     +
+--------+--------------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_I_eq_H_ls_math_c__c_float3x4_gr__Cu_C_c_C_l:

.. das:function:: operator [](m: float3x4 implicit ==const; i: uint const)

[] returns float3&

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+m       + :ref:`math::float3x4 <handle-math-float3x4>`  implicit!+
+--------+--------------------------------------------------------+
+i       +uint const                                              +
+--------+--------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_CI_eq_H_ls_math_c__c_float3x4_gr__Cu_C_c_C_l:

.. das:function:: operator [](m: float3x4 const implicit ==const; i: uint const)

[] returns float3 const&

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+m       + :ref:`math::float3x4 <handle-math-float3x4>`  const implicit!+
+--------+--------------------------------------------------------------+
+i       +uint const                                                    +
+--------+--------------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_quat_from_euler_Cf3:

.. das:function:: quat_from_euler(angles: float3 const)

quat_from_euler returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+angles  +float3 const +
+--------+-------------+


|function-math-quat_from_euler|

.. _function-_at_math_c__c_quat_from_euler_Cf_Cf_Cf:

.. das:function:: quat_from_euler(x: float const; y: float const; z: float const)

quat_from_euler returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+
+y       +float const  +
+--------+-------------+
+z       +float const  +
+--------+-------------+


|function-math-quat_from_euler|

.. _function-_at_math_c__c_euler_from_un_quat_Cf4:

.. das:function:: euler_from_un_quat(angles: float4 const)

euler_from_un_quat returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+angles  +float4 const +
+--------+-------------+


|function-math-euler_from_un_quat|

.. _function-_at_math_c__c_determinant_CIH_ls_math_c__c_float3x3_gr_:

.. das:function:: determinant(x: float3x3 const implicit)

determinant returns float

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+x       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit+
+--------+-------------------------------------------------------------+


|function-math-determinant|

.. _function-_at_math_c__c_[]_I_eq_H_ls_math_c__c_float3x3_gr__Ci_C_c_C_l:

.. das:function:: operator [](m: float3x3 implicit ==const; i: int const)

[] returns float3&

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+m       + :ref:`math::float3x3 <handle-math-float3x3>`  implicit!+
+--------+--------------------------------------------------------+
+i       +int const                                               +
+--------+--------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_CI_eq_H_ls_math_c__c_float3x3_gr__Ci_C_c_C_l:

.. das:function:: operator [](m: float3x3 const implicit ==const; i: int const)

[] returns float3 const&

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+m       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit!+
+--------+--------------------------------------------------------------+
+i       +int const                                                     +
+--------+--------------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_I_eq_H_ls_math_c__c_float3x3_gr__Cu_C_c_C_l:

.. das:function:: operator [](m: float3x3 implicit ==const; i: uint const)

[] returns float3&

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+m       + :ref:`math::float3x3 <handle-math-float3x3>`  implicit!+
+--------+--------------------------------------------------------+
+i       +uint const                                              +
+--------+--------------------------------------------------------+


|function-math-[]|

.. _function-_at_math_c__c_[]_CI_eq_H_ls_math_c__c_float3x3_gr__Cu_C_c_C_l:

.. das:function:: operator [](m: float3x3 const implicit ==const; i: uint const)

[] returns float3 const&

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+m       + :ref:`math::float3x3 <handle-math-float3x3>`  const implicit!+
+--------+--------------------------------------------------------------+
+i       +uint const                                                    +
+--------+--------------------------------------------------------------+


|function-math-[]|


