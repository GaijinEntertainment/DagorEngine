
.. _stdlib_math_boost:

======================
Boost package for math
======================

.. include:: detail/math_boost.rst

The math boost module implements collection of helper macros and functions to accompany :ref:`math <stdlib_math>`.

All functions and symbols are in "math_boost" module, use require to get access to it. ::

    require daslib/math_boost


.. _struct-math_boost-AABR:

.. das:attribute:: AABR



AABR fields are

+---+------+
+min+float2+
+---+------+
+max+float2+
+---+------+


axis aligned bounding rectangle

.. _struct-math_boost-AABB:

.. das:attribute:: AABB



AABB fields are

+---+------+
+min+float3+
+---+------+
+max+float3+
+---+------+


axis aligned bounding box

.. _struct-math_boost-Ray:

.. das:attribute:: Ray



Ray fields are

+------+------+
+dir   +float3+
+------+------+
+origin+float3+
+------+------+


ray (direction and origin)

+++++++++++++++++
Angle conversions
+++++++++++++++++

  *  :ref:`degrees (f:float const) : float <function-_at_math_boost_c__c_degrees_Cf>` 
  *  :ref:`radians (f:float const) : float <function-_at_math_boost_c__c_radians_Cf>` 

.. _function-_at_math_boost_c__c_degrees_Cf:

.. das:function:: degrees(f: float const)

degrees returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+f       +float const  +
+--------+-------------+


convert radians to degrees

.. _function-_at_math_boost_c__c_radians_Cf:

.. das:function:: radians(f: float const)

radians returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+f       +float const  +
+--------+-------------+


convert degrees to radians

+++++++++++++
Intersections
+++++++++++++

  *  :ref:`is_intersecting (a:math_boost::AABR const;b:math_boost::AABR const) : bool <function-_at_math_boost_c__c_is_intersecting_CS_ls_math_boost_c__c_AABR_gr__CS_ls_math_boost_c__c_AABR_gr_>` 
  *  :ref:`is_intersecting (a:math_boost::AABB const;b:math_boost::AABB const) : bool <function-_at_math_boost_c__c_is_intersecting_CS_ls_math_boost_c__c_AABB_gr__CS_ls_math_boost_c__c_AABB_gr_>` 
  *  :ref:`is_intersecting (ray:math_boost::Ray const;aabb:math_boost::AABB const;Tmin:float const;Tmax:float const) : bool <function-_at_math_boost_c__c_is_intersecting_CS_ls_math_boost_c__c_Ray_gr__CS_ls_math_boost_c__c_AABB_gr__Cf_Cf>` 

.. _function-_at_math_boost_c__c_is_intersecting_CS_ls_math_boost_c__c_AABR_gr__CS_ls_math_boost_c__c_AABR_gr_:

.. das:function:: is_intersecting(a: AABR const; b: AABR const)

is_intersecting returns bool

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+a       + :ref:`math_boost::AABR <struct-math_boost-AABR>`  const+
+--------+--------------------------------------------------------+
+b       + :ref:`math_boost::AABR <struct-math_boost-AABR>`  const+
+--------+--------------------------------------------------------+


A.LO<=B.HI && A.HI>=B.LO

.. _function-_at_math_boost_c__c_is_intersecting_CS_ls_math_boost_c__c_AABB_gr__CS_ls_math_boost_c__c_AABB_gr_:

.. das:function:: is_intersecting(a: AABB const; b: AABB const)

is_intersecting returns bool

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+a       + :ref:`math_boost::AABB <struct-math_boost-AABB>`  const+
+--------+--------------------------------------------------------+
+b       + :ref:`math_boost::AABB <struct-math_boost-AABB>`  const+
+--------+--------------------------------------------------------+


A.LO<=B.HI && A.HI>=B.LO

.. _function-_at_math_boost_c__c_is_intersecting_CS_ls_math_boost_c__c_Ray_gr__CS_ls_math_boost_c__c_AABB_gr__Cf_Cf:

.. das:function:: is_intersecting(ray: Ray const; aabb: AABB const; Tmin: float const; Tmax: float const)

is_intersecting returns bool

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+ray     + :ref:`math_boost::Ray <struct-math_boost-Ray>`  const  +
+--------+--------------------------------------------------------+
+aabb    + :ref:`math_boost::AABB <struct-math_boost-AABB>`  const+
+--------+--------------------------------------------------------+
+Tmin    +float const                                             +
+--------+--------------------------------------------------------+
+Tmax    +float const                                             +
+--------+--------------------------------------------------------+


A.LO<=B.HI && A.HI>=B.LO

++++++++
Matrices
++++++++

  *  :ref:`look_at_lh (Eye:float3 const;At:float3 const;Up:float3 const) : math::float4x4 <function-_at_math_boost_c__c_look_at_lh_Cf3_Cf3_Cf3>` 
  *  :ref:`look_at_rh (Eye:float3 const;At:float3 const;Up:float3 const) : math::float4x4 <function-_at_math_boost_c__c_look_at_rh_Cf3_Cf3_Cf3>` 
  *  :ref:`perspective_lh (fovy:float const;aspect:float const;zn:float const;zf:float const) : math::float4x4 <function-_at_math_boost_c__c_perspective_lh_Cf_Cf_Cf_Cf>` 
  *  :ref:`perspective_rh (fovy:float const;aspect:float const;zn:float const;zf:float const) : math::float4x4 <function-_at_math_boost_c__c_perspective_rh_Cf_Cf_Cf_Cf>` 
  *  :ref:`perspective_rh_opengl (fovy:float const;aspect:float const;zn:float const;zf:float const) : math::float4x4 <function-_at_math_boost_c__c_perspective_rh_opengl_Cf_Cf_Cf_Cf>` 
  *  :ref:`ortho_rh (left:float const;right:float const;bottom:float const;top:float const;zNear:float const;zFar:float const) : math::float4x4 <function-_at_math_boost_c__c_ortho_rh_Cf_Cf_Cf_Cf_Cf_Cf>` 
  *  :ref:`planar_shadow (Light:float4 const;Plane:float4 const) : math::float4x4 <function-_at_math_boost_c__c_planar_shadow_Cf4_Cf4>` 

.. _function-_at_math_boost_c__c_look_at_lh_Cf3_Cf3_Cf3:

.. das:function:: look_at_lh(Eye: float3 const; At: float3 const; Up: float3 const)

look_at_lh returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+Eye     +float3 const +
+--------+-------------+
+At      +float3 const +
+--------+-------------+
+Up      +float3 const +
+--------+-------------+


left-handed (z forward) look at matrix with origin at `Eye` and target at `At`, and up vector `Up`.

.. _function-_at_math_boost_c__c_look_at_rh_Cf3_Cf3_Cf3:

.. das:function:: look_at_rh(Eye: float3 const; At: float3 const; Up: float3 const)

look_at_rh returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+Eye     +float3 const +
+--------+-------------+
+At      +float3 const +
+--------+-------------+
+Up      +float3 const +
+--------+-------------+


right-handed (z towards viewer) look at matrix with origin at `Eye` and target at `At`, and up vector `Up`.

.. _function-_at_math_boost_c__c_perspective_lh_Cf_Cf_Cf_Cf:

.. das:function:: perspective_lh(fovy: float const; aspect: float const; zn: float const; zf: float const)

perspective_lh returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+fovy    +float const  +
+--------+-------------+
+aspect  +float const  +
+--------+-------------+
+zn      +float const  +
+--------+-------------+
+zf      +float const  +
+--------+-------------+


left-handed (z forward) perspective matrix

.. _function-_at_math_boost_c__c_perspective_rh_Cf_Cf_Cf_Cf:

.. das:function:: perspective_rh(fovy: float const; aspect: float const; zn: float const; zf: float const)

perspective_rh returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+fovy    +float const  +
+--------+-------------+
+aspect  +float const  +
+--------+-------------+
+zn      +float const  +
+--------+-------------+
+zf      +float const  +
+--------+-------------+


right-handed (z toward viewer) perspective matrix

.. _function-_at_math_boost_c__c_perspective_rh_opengl_Cf_Cf_Cf_Cf:

.. das:function:: perspective_rh_opengl(fovy: float const; aspect: float const; zn: float const; zf: float const)

perspective_rh_opengl returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+fovy    +float const  +
+--------+-------------+
+aspect  +float const  +
+--------+-------------+
+zn      +float const  +
+--------+-------------+
+zf      +float const  +
+--------+-------------+


right-handed (z toward viewer) opengl (z in [-1..1]) perspective matrix

.. _function-_at_math_boost_c__c_ortho_rh_Cf_Cf_Cf_Cf_Cf_Cf:

.. das:function:: ortho_rh(left: float const; right: float const; bottom: float const; top: float const; zNear: float const; zFar: float const)

ortho_rh returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+left    +float const  +
+--------+-------------+
+right   +float const  +
+--------+-------------+
+bottom  +float const  +
+--------+-------------+
+top     +float const  +
+--------+-------------+
+zNear   +float const  +
+--------+-------------+
+zFar    +float const  +
+--------+-------------+


right handed (z towards viwer) orthographic (parallel) projection matrix

.. _function-_at_math_boost_c__c_planar_shadow_Cf4_Cf4:

.. das:function:: planar_shadow(Light: float4 const; Plane: float4 const)

planar_shadow returns  :ref:`math::float4x4 <handle-math-float4x4>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+Light   +float4 const +
+--------+-------------+
+Plane   +float4 const +
+--------+-------------+


planar shadow projection matrix, i.e. all light shadows to be projected on a plane

+++++
Plane
+++++

  *  :ref:`plane_dot (Plane:float4 const;Vec:float4 const) : float <function-_at_math_boost_c__c_plane_dot_Cf4_Cf4>` 
  *  :ref:`plane_normalize (Plane:float4 const) : float4 const <function-_at_math_boost_c__c_plane_normalize_Cf4>` 
  *  :ref:`plane_from_point_normal (p:float3 const;n:float3 const) : float4 <function-_at_math_boost_c__c_plane_from_point_normal_Cf3_Cf3>` 

.. _function-_at_math_boost_c__c_plane_dot_Cf4_Cf4:

.. das:function:: plane_dot(Plane: float4 const; Vec: float4 const)

plane_dot returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+Plane   +float4 const +
+--------+-------------+
+Vec     +float4 const +
+--------+-------------+


dot product of `Plane` and 'Vec'

.. _function-_at_math_boost_c__c_plane_normalize_Cf4:

.. das:function:: plane_normalize(Plane: float4 const)

plane_normalize returns float4 const

+--------+-------------+
+argument+argument type+
+========+=============+
+Plane   +float4 const +
+--------+-------------+


normalize `Plane', length xyz will be 1.0 (or 0.0 for no plane)

.. _function-_at_math_boost_c__c_plane_from_point_normal_Cf3_Cf3:

.. das:function:: plane_from_point_normal(p: float3 const; n: float3 const)

plane_from_point_normal returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+p       +float3 const +
+--------+-------------+
+n       +float3 const +
+--------+-------------+


construct plane from point `p` and normal `n`

++++++++++++++++++++++++++
Color packig and unpacking
++++++++++++++++++++++++++

  *  :ref:`RGBA_TO_UCOLOR (x:float const;y:float const;z:float const;w:float const) : uint <function-_at_math_boost_c__c_RGBA_TO_UCOLOR_Cf_Cf_Cf_Cf>` 
  *  :ref:`RGBA_TO_UCOLOR (xyzw:float4 const) : uint <function-_at_math_boost_c__c_RGBA_TO_UCOLOR_Cf4>` 
  *  :ref:`UCOLOR_TO_RGBA (x:uint const) : float4 <function-_at_math_boost_c__c_UCOLOR_TO_RGBA_Cu>` 
  *  :ref:`UCOLOR_TO_RGB (x:uint const) : float3 <function-_at_math_boost_c__c_UCOLOR_TO_RGB_Cu>` 

.. _function-_at_math_boost_c__c_RGBA_TO_UCOLOR_Cf_Cf_Cf_Cf:

.. das:function:: RGBA_TO_UCOLOR(x: float const; y: float const; z: float const; w: float const)

RGBA_TO_UCOLOR returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+
+y       +float const  +
+--------+-------------+
+z       +float const  +
+--------+-------------+
+w       +float const  +
+--------+-------------+


conversion from RGBA to ucolor. x,y,z,w are in [0,1] range

.. _function-_at_math_boost_c__c_RGBA_TO_UCOLOR_Cf4:

.. das:function:: RGBA_TO_UCOLOR(xyzw: float4 const)

RGBA_TO_UCOLOR returns uint

+--------+-------------+
+argument+argument type+
+========+=============+
+xyzw    +float4 const +
+--------+-------------+


conversion from RGBA to ucolor. x,y,z,w are in [0,1] range

.. _function-_at_math_boost_c__c_UCOLOR_TO_RGBA_Cu:

.. das:function:: UCOLOR_TO_RGBA(x: uint const)

UCOLOR_TO_RGBA returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint const   +
+--------+-------------+


conversion from ucolor to RGBA. x components are in [0,255] range

.. _function-_at_math_boost_c__c_UCOLOR_TO_RGB_Cu:

.. das:function:: UCOLOR_TO_RGB(x: uint const)

UCOLOR_TO_RGB returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +uint const   +
+--------+-------------+


conversion from ucolor to RGB. x components are in [0,255] range. result is float3(x,y,z)

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_math_boost_c__c_linear_to_SRGB_Cf:

.. das:function:: linear_to_SRGB(x: float const)

linear_to_SRGB returns float

+--------+-------------+
+argument+argument type+
+========+=============+
+x       +float const  +
+--------+-------------+


convert value from linear space to sRGB curve space

.. _function-_at_math_boost_c__c_linear_to_SRGB_Cf3:

.. das:function:: linear_to_SRGB(c: float3 const)

linear_to_SRGB returns float3

+--------+-------------+
+argument+argument type+
+========+=============+
+c       +float3 const +
+--------+-------------+


convert value from linear space to sRGB curve space

.. _function-_at_math_boost_c__c_linear_to_SRGB_Cf4:

.. das:function:: linear_to_SRGB(c: float4 const)

linear_to_SRGB returns float4

+--------+-------------+
+argument+argument type+
+========+=============+
+c       +float4 const +
+--------+-------------+


convert value from linear space to sRGB curve space


