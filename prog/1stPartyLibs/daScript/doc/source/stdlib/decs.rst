
.. _stdlib_decs:

=====================================
DECS, Daslang entity component system
=====================================

.. include:: detail/decs.rst

The DECS module implements low level functionality of Daslang entity component system.

All functions and symbols are in "decs" module, use require to get access to it. ::

    require daslib/decs

Under normal circumstances, the module is not used without the boost package::

    require daslib/desc_boost


++++++++++++
Type aliases
++++++++++++

.. _alias-ComponentHash:

.. das:attribute:: ComponentHash = uint64

Hash value of the ECS component type

.. _alias-TypeHash:

.. das:attribute:: TypeHash = uint64

Hash value of the individual type

.. _alias-DeferEval:

.. das:attribute:: DeferEval = lambda<(var act:DeferAction):void>

Lambda which holds deferred action. Typically creation of destruction of an entity.

.. _alias-ComponentMap:

.. das:attribute:: ComponentMap = array<ComponentValue>

Table of component values for individual entity.

.. _alias-PassFunction:

.. das:attribute:: PassFunction = function<void>

One of the callbacks which form individual pass.

.. _struct-decs-CTypeInfo:

.. das:attribute:: CTypeInfo



CTypeInfo fields are

+-----------+-----------------------------------------------------------------------------------------------------------+
+basicType  + :ref:`rtti::Type <enum-rtti-Type>`                                                                        +
+-----------+-----------------------------------------------------------------------------------------------------------+
+mangledName+string                                                                                                     +
+-----------+-----------------------------------------------------------------------------------------------------------+
+fullName   +string                                                                                                     +
+-----------+-----------------------------------------------------------------------------------------------------------+
+hash       + :ref:`TypeHash <alias-TypeHash>`                                                                          +
+-----------+-----------------------------------------------------------------------------------------------------------+
+size       +uint                                                                                                       +
+-----------+-----------------------------------------------------------------------------------------------------------+
+eraser     +function<(arr:array<uint8>):void>                                                                          +
+-----------+-----------------------------------------------------------------------------------------------------------+
+clonner    +function<(dst:array<uint8>;src:array<uint8> const):void>                                                   +
+-----------+-----------------------------------------------------------------------------------------------------------+
+serializer +function<(arch: :ref:`archive::Archive <struct-archive-Archive>` ;arr:array<uint8>;name:string const):void>+
+-----------+-----------------------------------------------------------------------------------------------------------+
+dumper     +function<(elem:void? const):string>                                                                        +
+-----------+-----------------------------------------------------------------------------------------------------------+
+mkTypeInfo +function<>                                                                                                 +
+-----------+-----------------------------------------------------------------------------------------------------------+
+gc         +function<(src:array<uint8>):lambda<>>                                                                      +
+-----------+-----------------------------------------------------------------------------------------------------------+


Type information for the individual component subtype.
Consists of type name and collection of type-specific routines to control type values during its lifetime, serialization, etc.

.. _struct-decs-Component:

.. das:attribute:: Component



Component fields are

+--------+------------------------------------------------+
+name    +string                                          +
+--------+------------------------------------------------+
+hash    + :ref:`ComponentHash <alias-ComponentHash>`     +
+--------+------------------------------------------------+
+stride  +int                                             +
+--------+------------------------------------------------+
+data    +array<uint8>                                    +
+--------+------------------------------------------------+
+info    + :ref:`decs::CTypeInfo <struct-decs-CTypeInfo>` +
+--------+------------------------------------------------+
+gc_dummy+lambda<>                                        +
+--------+------------------------------------------------+


Single ECS component. Contains component name, data, and data layout.

.. _struct-decs-EntityId:

.. das:attribute:: EntityId



EntityId fields are

+----------+----+
+id        +uint+
+----------+----+
+generation+int +
+----------+----+


Unique identifier of the entity. Consists of id (index in the data array) and generation.

.. _struct-decs-Archetype:

.. das:attribute:: Archetype



Archetype fields are

+----------+-------------------------------------------------------+
+hash      + :ref:`ComponentHash <alias-ComponentHash>`            +
+----------+-------------------------------------------------------+
+components+array< :ref:`decs::Component <struct-decs-Component>` >+
+----------+-------------------------------------------------------+
+size      +int                                                    +
+----------+-------------------------------------------------------+
+eidIndex  +int                                                    +
+----------+-------------------------------------------------------+


ECS archetype. Archetype is unique combination of components.

.. _struct-decs-ComponentValue:

.. das:attribute:: ComponentValue



ComponentValue fields are

+----+------------------------------------------------+
+name+string                                          +
+----+------------------------------------------------+
+info+ :ref:`decs::CTypeInfo <struct-decs-CTypeInfo>` +
+----+------------------------------------------------+
+data+float4[4]                                       +
+----+------------------------------------------------+


Value of the component during creation or transformation.

.. _struct-decs-EcsRequestPos:

.. das:attribute:: EcsRequestPos



EcsRequestPos fields are

+----+------+
+file+string+
+----+------+
+line+uint  +
+----+------+


Location of the ECS request in the code (source file and line number).

.. _struct-decs-EcsRequest:

.. das:attribute:: EcsRequest



EcsRequest fields are

+----------+--------------------------------------------------------+
+hash      + :ref:`ComponentHash <alias-ComponentHash>`             +
+----------+--------------------------------------------------------+
+req       +array<string>                                           +
+----------+--------------------------------------------------------+
+reqn      +array<string>                                           +
+----------+--------------------------------------------------------+
+archetypes+array<int>                                              +
+----------+--------------------------------------------------------+
+at        + :ref:`decs::EcsRequestPos <struct-decs-EcsRequestPos>` +
+----------+--------------------------------------------------------+


Individual ESC requests. Contains list of required components, list of components which are required to be absent.
Caches list of archetypes, which match the request.

.. _struct-decs-DecsState:

.. das:attribute:: DecsState



DecsState fields are

+------------------+---------------------------------------------------------------------------------------------+
+archetypeLookup   +table< :ref:`ComponentHash <alias-ComponentHash>` ;int>                                      +
+------------------+---------------------------------------------------------------------------------------------+
+allArchetypes     +array< :ref:`decs::Archetype <struct-decs-Archetype>` >                                      +
+------------------+---------------------------------------------------------------------------------------------+
+entityFreeList    +array< :ref:`decs::EntityId <struct-decs-EntityId>` >                                        +
+------------------+---------------------------------------------------------------------------------------------+
+entityLookup      +array<tuple<generation:int;archetype: :ref:`ComponentHash <alias-ComponentHash>` ;index:int>>+
+------------------+---------------------------------------------------------------------------------------------+
+componentTypeCheck+table<string; :ref:`decs::CTypeInfo <struct-decs-CTypeInfo>` >                               +
+------------------+---------------------------------------------------------------------------------------------+
+ecsQueries        +array< :ref:`decs::EcsRequest <struct-decs-EcsRequest>` >                                    +
+------------------+---------------------------------------------------------------------------------------------+
+queryLookup       +table< :ref:`ComponentHash <alias-ComponentHash>` ;int>                                      +
+------------------+---------------------------------------------------------------------------------------------+


Entire state of the ECS system.
Conntains archtypes, entities and entity free-list, entity lokup table, all archetypes and archetype lookups, etc.

.. _struct-decs-DecsPass:

.. das:attribute:: DecsPass



DecsPass fields are

+-----+-------------------------------------------------+
+name +string                                           +
+-----+-------------------------------------------------+
+calls+array< :ref:`PassFunction <alias-PassFunction>` >+
+-----+-------------------------------------------------+


Individual pass of the update of the ECS system.
Contains pass name and list of all pass calblacks.

+++++++++++++++++++++
Comparison and access
+++++++++++++++++++++

  *  :ref:`== (a:decs::EntityId const implicit;b:decs::EntityId const implicit) : bool <function-_at_decs_c__c__eq__eq__CIS_ls_decs_c__c_EntityId_gr__CIS_ls_decs_c__c_EntityId_gr_>` 
  *  :ref:`\!= (a:decs::EntityId const implicit;b:decs::EntityId const implicit) : bool <function-_at_decs_c__c__ex__eq__CIS_ls_decs_c__c_EntityId_gr__CIS_ls_decs_c__c_EntityId_gr_>` 
  *  :ref:`. (cmp:array\<decs::ComponentValue\> -const;name:string const) : decs::ComponentValue& <function-_at_decs_c__c_._Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs>` 

.. _function-_at_decs_c__c__eq__eq__CIS_ls_decs_c__c_EntityId_gr__CIS_ls_decs_c__c_EntityId_gr_:

.. das:function:: operator ==(a: EntityId const implicit; b: EntityId const implicit)

== returns bool

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+a       + :ref:`decs::EntityId <struct-decs-EntityId>`  const implicit+
+--------+-------------------------------------------------------------+
+b       + :ref:`decs::EntityId <struct-decs-EntityId>`  const implicit+
+--------+-------------------------------------------------------------+


Equality operator for entity IDs.

.. _function-_at_decs_c__c__ex__eq__CIS_ls_decs_c__c_EntityId_gr__CIS_ls_decs_c__c_EntityId_gr_:

.. das:function:: operator !=(a: EntityId const implicit; b: EntityId const implicit)

!= returns bool

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+a       + :ref:`decs::EntityId <struct-decs-EntityId>`  const implicit+
+--------+-------------------------------------------------------------+
+b       + :ref:`decs::EntityId <struct-decs-EntityId>`  const implicit+
+--------+-------------------------------------------------------------+


Inequality operator for entity IDs.

.. _function-_at_decs_c__c_._Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs:

.. das:function:: operator .(cmp: ComponentMap; name: string const)

. returns  :ref:`decs::ComponentValue <struct-decs-ComponentValue>` &

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+cmp     + :ref:`ComponentMap <alias-ComponentMap>` +
+--------+------------------------------------------+
+name    +string const                              +
+--------+------------------------------------------+


Access to component value by name. For example::

    create_entity <| @ ( eid, cmp )
        cmp.pos := float3(i)    // same as cmp |> set("pos",float3(i))

++++++++++++++++++++++
Access (get/set/clone)
++++++++++++++++++++++

  *  :ref:`clone (cv:decs::ComponentValue -const;val:decs::EntityId const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__CS_ls_decs_c__c_EntityId_gr_>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:bool const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cb>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:range const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cr>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:urange const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cz>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:range64 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cr64>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:urange64 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cz64>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:string const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cs>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:int const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:int8 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci8>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:int16 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci16>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:int64 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci64>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:int2 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci2>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:int3 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci3>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:int4 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci4>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:uint const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:uint8 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu8>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:uint16 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu16>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:uint64 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu64>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:uint2 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu2>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:uint3 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu3>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:uint4 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu4>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:float const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cf>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:float2 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cf2>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:float3 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cf3>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:float4 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cf4>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:math::float3x3 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__CH_ls_math_c__c_float3x3_gr_>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:math::float3x4 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__CH_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:math::float4x4 const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__CH_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`clone (cv:decs::ComponentValue -const;val:double const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cd>` 
  *  :ref:`clone (dst:decs::Component -const;src:decs::Component const) : void <function-_at_decs_c__c_clone_S_ls_decs_c__c_Component_gr__CS_ls_decs_c__c_Component_gr_>` 
  *  :ref:`has (arch:decs::Archetype const;name:string const) : bool <function-_at_decs_c__c_has_CS_ls_decs_c__c_Archetype_gr__Cs>` 
  *  :ref:`has (cmp:array\<decs::ComponentValue\> -const;name:string const) : bool <function-_at_decs_c__c_has_Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs>` 
  *  :ref:`remove (cmp:array\<decs::ComponentValue\> -const;name:string const) : void <function-_at_decs_c__c_remove_Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs>` 
  *  :ref:`set (cv:decs::ComponentValue -const;val:auto const) : auto <function-_at_decs_c__c_set_S_ls_decs_c__c_ComponentValue_gr__C.>` 
  *  :ref:`get (arch:decs::Archetype const;name:string const;value:auto(TT) const) : auto <function-_at_decs_c__c_get_CS_ls_decs_c__c_Archetype_gr__Cs_CY_ls_TT_gr_.>` 
  *  :ref:`get (cmp:array\<decs::ComponentValue\> -const;name:string const;value:auto(TT) -const) : auto <function-_at_decs_c__c_get_Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs_Y_ls_TT_gr_.>` 
  *  :ref:`set (cmp:array\<decs::ComponentValue\> -const;name:string const;value:auto(TT) const) : auto <function-_at_decs_c__c_set_Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs_CY_ls_TT_gr_.>` 

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__CS_ls_decs_c__c_EntityId_gr_:

.. das:function:: clone(cv: ComponentValue; val: EntityId const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     + :ref:`decs::EntityId <struct-decs-EntityId>`  const      +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cb:

.. das:function:: clone(cv: ComponentValue; val: bool const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +bool const                                                +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cr:

.. das:function:: clone(cv: ComponentValue; val: range const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +range const                                               +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cz:

.. das:function:: clone(cv: ComponentValue; val: urange const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +urange const                                              +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cr64:

.. das:function:: clone(cv: ComponentValue; val: range64 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +range64 const                                             +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cz64:

.. das:function:: clone(cv: ComponentValue; val: urange64 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +urange64 const                                            +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cs:

.. das:function:: clone(cv: ComponentValue; val: string const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +string const                                              +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci:

.. das:function:: clone(cv: ComponentValue; val: int const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +int const                                                 +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci8:

.. das:function:: clone(cv: ComponentValue; val: int8 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +int8 const                                                +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci16:

.. das:function:: clone(cv: ComponentValue; val: int16 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +int16 const                                               +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci64:

.. das:function:: clone(cv: ComponentValue; val: int64 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +int64 const                                               +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci2:

.. das:function:: clone(cv: ComponentValue; val: int2 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +int2 const                                                +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci3:

.. das:function:: clone(cv: ComponentValue; val: int3 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +int3 const                                                +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Ci4:

.. das:function:: clone(cv: ComponentValue; val: int4 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +int4 const                                                +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu:

.. das:function:: clone(cv: ComponentValue; val: uint const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +uint const                                                +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu8:

.. das:function:: clone(cv: ComponentValue; val: uint8 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +uint8 const                                               +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu16:

.. das:function:: clone(cv: ComponentValue; val: uint16 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +uint16 const                                              +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu64:

.. das:function:: clone(cv: ComponentValue; val: uint64 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +uint64 const                                              +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu2:

.. das:function:: clone(cv: ComponentValue; val: uint2 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +uint2 const                                               +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu3:

.. das:function:: clone(cv: ComponentValue; val: uint3 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +uint3 const                                               +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cu4:

.. das:function:: clone(cv: ComponentValue; val: uint4 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +uint4 const                                               +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cf:

.. das:function:: clone(cv: ComponentValue; val: float const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +float const                                               +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cf2:

.. das:function:: clone(cv: ComponentValue; val: float2 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +float2 const                                              +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cf3:

.. das:function:: clone(cv: ComponentValue; val: float3 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +float3 const                                              +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cf4:

.. das:function:: clone(cv: ComponentValue; val: float4 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +float4 const                                              +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__CH_ls_math_c__c_float3x3_gr_:

.. das:function:: clone(cv: ComponentValue; val: float3x3 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     + :ref:`math::float3x3 <handle-math-float3x3>`  const      +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__CH_ls_math_c__c_float3x4_gr_:

.. das:function:: clone(cv: ComponentValue; val: float3x4 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     + :ref:`math::float3x4 <handle-math-float3x4>`  const      +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__CH_ls_math_c__c_float4x4_gr_:

.. das:function:: clone(cv: ComponentValue; val: float4x4 const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     + :ref:`math::float4x4 <handle-math-float4x4>`  const      +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_ComponentValue_gr__Cd:

.. das:function:: clone(cv: ComponentValue; val: double const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +double const                                              +
+--------+----------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_clone_S_ls_decs_c__c_Component_gr__CS_ls_decs_c__c_Component_gr_:

.. das:function:: clone(dst: Component; src: Component const)

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+dst     + :ref:`decs::Component <struct-decs-Component>`       +
+--------+------------------------------------------------------+
+src     + :ref:`decs::Component <struct-decs-Component>`  const+
+--------+------------------------------------------------------+


Clones component value.

.. _function-_at_decs_c__c_has_CS_ls_decs_c__c_Archetype_gr__Cs:

.. das:function:: has(arch: Archetype const; name: string const)

has returns bool

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+arch    + :ref:`decs::Archetype <struct-decs-Archetype>`  const+
+--------+------------------------------------------------------+
+name    +string const                                          +
+--------+------------------------------------------------------+


Returns true if object has specified subobjec.

.. _function-_at_decs_c__c_has_Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs:

.. das:function:: has(cmp: ComponentMap; name: string const)

has returns bool

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+cmp     + :ref:`ComponentMap <alias-ComponentMap>` +
+--------+------------------------------------------+
+name    +string const                              +
+--------+------------------------------------------+


Returns true if object has specified subobjec.

.. _function-_at_decs_c__c_remove_Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs:

.. das:function:: remove(cmp: ComponentMap; name: string const)

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+cmp     + :ref:`ComponentMap <alias-ComponentMap>` +
+--------+------------------------------------------+
+name    +string const                              +
+--------+------------------------------------------+


Removes speicified value from the component map.

.. _function-_at_decs_c__c_set_S_ls_decs_c__c_ComponentValue_gr__C.:

.. das:function:: set(cv: ComponentValue; val: auto const)

set returns auto

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+cv      + :ref:`decs::ComponentValue <struct-decs-ComponentValue>` +
+--------+----------------------------------------------------------+
+val     +auto const                                                +
+--------+----------------------------------------------------------+


Set component value specified by name and type.
If value already exists, it is overwritten. If already existing value type is not the same - panic.
overwrite
insert new one

.. _function-_at_decs_c__c_get_CS_ls_decs_c__c_Archetype_gr__Cs_CY_ls_TT_gr_.:

.. das:function:: get(arch: Archetype const; name: string const; value: auto(TT) const)

get returns auto

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+arch    + :ref:`decs::Archetype <struct-decs-Archetype>`  const+
+--------+------------------------------------------------------+
+name    +string const                                          +
+--------+------------------------------------------------------+
+value   +auto(TT) const                                        +
+--------+------------------------------------------------------+


Gets component value specified by name and type.
Will panic if name matches but type does not.

.. _function-_at_decs_c__c_get_Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs_Y_ls_TT_gr_.:

.. das:function:: get(cmp: ComponentMap; name: string const; value: auto(TT))

get returns auto

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+cmp     + :ref:`ComponentMap <alias-ComponentMap>` +
+--------+------------------------------------------+
+name    +string const                              +
+--------+------------------------------------------+
+value   +auto(TT)                                  +
+--------+------------------------------------------+


Gets component value specified by name and type.
Will panic if name matches but type does not.

.. _function-_at_decs_c__c_set_Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_Cs_CY_ls_TT_gr_.:

.. das:function:: set(cmp: ComponentMap; name: string const; value: auto(TT) const)

set returns auto

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+cmp     + :ref:`ComponentMap <alias-ComponentMap>` +
+--------+------------------------------------------+
+name    +string const                              +
+--------+------------------------------------------+
+value   +auto(TT) const                            +
+--------+------------------------------------------+


Set component value specified by name and type.
If value already exists, it is overwritten. If already existing value type is not the same - panic.
overwrite
insert new one

+++++++++++++++++++++++
Deubg and serialization
+++++++++++++++++++++++

  *  :ref:`describe (info:decs::CTypeInfo const) : string <function-_at_decs_c__c_describe_CS_ls_decs_c__c_CTypeInfo_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;src:decs::Component -const) : void <function-_at_decs_c__c_serialize_S_ls_archive_c__c_Archive_gr__S_ls_decs_c__c_Component_gr_>` 
  *  :ref:`finalize (cmp:decs::Component -const) : void <function-_at_decs_c__c_finalize_S_ls_decs_c__c_Component_gr_>` 
  *  :ref:`debug_dump () : void <function-_at_decs_c__c_debug_dump>` 

.. _function-_at_decs_c__c_describe_CS_ls_decs_c__c_CTypeInfo_gr_:

.. das:function:: describe(info: CTypeInfo const)

describe returns string

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+info    + :ref:`decs::CTypeInfo <struct-decs-CTypeInfo>`  const+
+--------+------------------------------------------------------+


Returns textual description of the type.

.. _function-_at_decs_c__c_serialize_S_ls_archive_c__c_Archive_gr__S_ls_decs_c__c_Component_gr_:

.. das:function:: serialize(arch: Archive; src: Component)

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+src     + :ref:`decs::Component <struct-decs-Component>`   +
+--------+--------------------------------------------------+


Serializes component value.

.. _function-_at_decs_c__c_finalize_S_ls_decs_c__c_Component_gr_:

.. das:function:: finalize(cmp: Component)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+cmp     + :ref:`decs::Component <struct-decs-Component>` +
+--------+------------------------------------------------+


Deletes component.

.. _function-_at_decs_c__c_debug_dump:

.. das:function:: debug_dump()

Prints out state of the ECS system.
debug(arch)

++++++
Stages
++++++

  *  :ref:`register_decs_stage_call (name:string const;pcall:function\<void\> const) : void <function-_at_decs_c__c_register_decs_stage_call_Cs_CY_ls_PassFunction_gr_1_ls_v_gr__at__at_>` 
  *  :ref:`decs_stage (name:string const) : void <function-_at_decs_c__c_decs_stage_Cs>` 
  *  :ref:`commit () : void <function-_at_decs_c__c_commit>` 

.. _function-_at_decs_c__c_register_decs_stage_call_Cs_CY_ls_PassFunction_gr_1_ls_v_gr__at__at_:

.. das:function:: register_decs_stage_call(name: string const; pcall: PassFunction)

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+name    +string const                              +
+--------+------------------------------------------+
+pcall   + :ref:`PassFunction <alias-PassFunction>` +
+--------+------------------------------------------+


Registration of a single pass callback. This is a low-level function, used by decs_boost macros.
insert new one

.. _function-_at_decs_c__c_decs_stage_Cs:

.. das:function:: decs_stage(name: string const)

+--------+-------------+
+argument+argument type+
+========+=============+
+name    +string const +
+--------+-------------+


Invokes specific ECS pass.
`commit` is called before and after the invocation.

.. _function-_at_decs_c__c_commit:

.. das:function:: commit()

Finishes all deferred actions.

++++++++++++++++
Deferred actions
++++++++++++++++

  *  :ref:`update_entity (entityid:decs::EntityId const implicit;blk:lambda\<(eid:decs::EntityId const;var cmp:array\<decs::ComponentValue\> -const):void\> -const) : void <function-_at_decs_c__c_update_entity_CIS_ls_decs_c__c_EntityId_gr__N_ls_eid;cmp_gr_0_ls_CS_ls_decs_c__c_EntityId_gr_;Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_gr_1_ls_v_gr__at_>` 
  *  :ref:`create_entity (blk:lambda\<(eid:decs::EntityId const;var cmp:array\<decs::ComponentValue\> -const):void\> -const) : decs::EntityId <function-_at_decs_c__c_create_entity_N_ls_eid;cmp_gr_0_ls_CS_ls_decs_c__c_EntityId_gr_;Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_gr_1_ls_v_gr__at_>` 
  *  :ref:`delete_entity (entityid:decs::EntityId const implicit) : void <function-_at_decs_c__c_delete_entity_CIS_ls_decs_c__c_EntityId_gr_>` 

.. _function-_at_decs_c__c_update_entity_CIS_ls_decs_c__c_EntityId_gr__N_ls_eid;cmp_gr_0_ls_CS_ls_decs_c__c_EntityId_gr_;Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_gr_1_ls_v_gr__at_:

.. das:function:: update_entity(entityid: EntityId const implicit; blk: lambda<(eid:EntityId const;var cmp:array<ComponentValue>):void>)

+--------+----------------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                         +
+========+======================================================================================================================+
+entityid+ :ref:`decs::EntityId <struct-decs-EntityId>`  const implicit                                                         +
+--------+----------------------------------------------------------------------------------------------------------------------+
+blk     +lambda<(eid: :ref:`decs::EntityId <struct-decs-EntityId>`  const;cmp: :ref:`ComponentMap <alias-ComponentMap>` ):void>+
+--------+----------------------------------------------------------------------------------------------------------------------+


Creates deferred action to update entity specified by id.

.. _function-_at_decs_c__c_create_entity_N_ls_eid;cmp_gr_0_ls_CS_ls_decs_c__c_EntityId_gr_;Y_ls_ComponentMap_gr_1_ls_S_ls_decs_c__c_ComponentValue_gr__gr_A_gr_1_ls_v_gr__at_:

.. das:function:: create_entity(blk: lambda<(eid:EntityId const;var cmp:array<ComponentValue>):void>)

create_entity returns  :ref:`decs::EntityId <struct-decs-EntityId>` 

+--------+----------------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                         +
+========+======================================================================================================================+
+blk     +lambda<(eid: :ref:`decs::EntityId <struct-decs-EntityId>`  const;cmp: :ref:`ComponentMap <alias-ComponentMap>` ):void>+
+--------+----------------------------------------------------------------------------------------------------------------------+


Creates deferred action to create entity.

.. _function-_at_decs_c__c_delete_entity_CIS_ls_decs_c__c_EntityId_gr_:

.. das:function:: delete_entity(entityid: EntityId const implicit)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+entityid+ :ref:`decs::EntityId <struct-decs-EntityId>`  const implicit+
+--------+-------------------------------------------------------------+


Creates deferred action to delete entity specified by id.

++++++++++++
GC and reset
++++++++++++

  *  :ref:`restart () : void <function-_at_decs_c__c_restart>` 
  *  :ref:`before_gc () : void <function-_at_decs_c__c_before_gc>` 
  *  :ref:`after_gc () : void <function-_at_decs_c__c_after_gc>` 

.. _function-_at_decs_c__c_restart:

.. das:function:: restart()

Restarts ECS by erasing all deferred actions and entire state.

.. _function-_at_decs_c__c_before_gc:

.. das:function:: before_gc()

Low level callback to be called before the garbage collection.
This is a low-level function typically used by `live`.

.. _function-_at_decs_c__c_after_gc:

.. das:function:: after_gc()

Low level callback to be called after the garbage collection.
This is a low-level function typically used by `live`.

+++++++++
Iteration
+++++++++

  *  :ref:`for_each_archetype (erq:decs::EcsRequest -const;blk:block\<(arch:decs::Archetype const):void\> const) : void <function-_at_decs_c__c_for_each_archetype_S_ls_decs_c__c_EcsRequest_gr__CN_ls_arch_gr_0_ls_CS_ls_decs_c__c_Archetype_gr__gr_1_ls_v_gr__builtin_>` 
  *  :ref:`for_eid_archetype (eid:decs::EntityId const implicit;hash:uint64 const;erq:function\<decs::EcsRequest\> -const;blk:block\<(arch:decs::Archetype const;index:int const):void\> const) : bool const <function-_at_decs_c__c_for_eid_archetype_CIS_ls_decs_c__c_EntityId_gr__CY_ls_ComponentHash_gr_u64_1_ls_S_ls_decs_c__c_EcsRequest_gr__gr__at__at__CN_ls_arch;index_gr_0_ls_CS_ls_decs_c__c_Archetype_gr_;Ci_gr_1_ls_v_gr__builtin_>` 
  *  :ref:`for_each_archetype (hash:uint64 const;erq:function\<decs::EcsRequest\> -const;blk:block\<(arch:decs::Archetype const):void\> const) : void <function-_at_decs_c__c_for_each_archetype_CY_ls_ComponentHash_gr_u64_1_ls_S_ls_decs_c__c_EcsRequest_gr__gr__at__at__CN_ls_arch_gr_0_ls_CS_ls_decs_c__c_Archetype_gr__gr_1_ls_v_gr__builtin_>` 
  *  :ref:`for_each_archetype_find (hash:uint64 const;erq:function\<decs::EcsRequest\> -const;blk:block\<(arch:decs::Archetype const):bool\> const) : bool const <function-_at_decs_c__c_for_each_archetype_find_CY_ls_ComponentHash_gr_u64_1_ls_S_ls_decs_c__c_EcsRequest_gr__gr__at__at__CN_ls_arch_gr_0_ls_CS_ls_decs_c__c_Archetype_gr__gr_1_ls_b_gr__builtin_>` 
  *  :ref:`decs_array (atype:auto(TT) const;src:array\<uint8\> const;capacity:int const) : auto <function-_at_decs_c__c_decs_array_CY_ls_TT_gr_._C1_ls_u8_gr_A_Ci>` 
  *  :ref:`get_ro (arch:decs::Archetype const;name:string const;value:auto(TT) const[]) : array\<TT[-2] -const -& -#\> const <function-_at_decs_c__c_get_ro_CS_ls_decs_c__c_Archetype_gr__Cs_C_lb_-1_rb_Y_ls_TT_gr_.>` 
  *  :ref:`get_ro (arch:decs::Archetype const;name:string const;value:auto(TT) const) : array\<TT -const -& -#\> const <function-_at_decs_c__c_get_ro_CS_ls_decs_c__c_Archetype_gr__Cs_CY_ls_TT_gr_._%_ls__ex_()_gr_>` 
  *  :ref:`get_default_ro (arch:decs::Archetype const;name:string const;value:auto(TT) const) : iterator\<TT const&\> <function-_at_decs_c__c_get_default_ro_CS_ls_decs_c__c_Archetype_gr__Cs_CY_ls_TT_gr_.>` 
  *  :ref:`get_optional (arch:decs::Archetype const;name:string const;value:auto(TT)? const) : iterator\<TT -const -& -#?\> <function-_at_decs_c__c_get_optional_CS_ls_decs_c__c_Archetype_gr__Cs_C1_ls_Y_ls_TT_gr_._gr__qm_>` 

.. _function-_at_decs_c__c_for_each_archetype_S_ls_decs_c__c_EcsRequest_gr__CN_ls_arch_gr_0_ls_CS_ls_decs_c__c_Archetype_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: for_each_archetype(erq: EcsRequest; blk: block<(arch:Archetype const):void> const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+erq     + :ref:`decs::EcsRequest <struct-decs-EcsRequest>`                              +
+--------+-------------------------------------------------------------------------------+
+blk     +block<(arch: :ref:`decs::Archetype <struct-decs-Archetype>`  const):void> const+
+--------+-------------------------------------------------------------------------------+


Invokes block for each entity of each archetype that can be processed by the request.
Request is returned by a specified function.

.. _function-_at_decs_c__c_for_eid_archetype_CIS_ls_decs_c__c_EntityId_gr__CY_ls_ComponentHash_gr_u64_1_ls_S_ls_decs_c__c_EcsRequest_gr__gr__at__at__CN_ls_arch;index_gr_0_ls_CS_ls_decs_c__c_Archetype_gr_;Ci_gr_1_ls_v_gr__builtin_:

.. das:function:: for_eid_archetype(eid: EntityId const implicit; hash: ComponentHash; erq: function<EcsRequest>; blk: block<(arch:Archetype const;index:int const):void> const)

for_eid_archetype returns bool const

+--------+-----------------------------------------------------------------------------------------------+
+argument+argument type                                                                                  +
+========+===============================================================================================+
+eid     + :ref:`decs::EntityId <struct-decs-EntityId>`  const implicit                                  +
+--------+-----------------------------------------------------------------------------------------------+
+hash    + :ref:`ComponentHash <alias-ComponentHash>`                                                    +
+--------+-----------------------------------------------------------------------------------------------+
+erq     +function<>                                                                                     +
+--------+-----------------------------------------------------------------------------------------------+
+blk     +block<(arch: :ref:`decs::Archetype <struct-decs-Archetype>`  const;index:int const):void> const+
+--------+-----------------------------------------------------------------------------------------------+


Invokes block for the specific entity id, given request.
Request is returned by a specified function.

.. _function-_at_decs_c__c_for_each_archetype_CY_ls_ComponentHash_gr_u64_1_ls_S_ls_decs_c__c_EcsRequest_gr__gr__at__at__CN_ls_arch_gr_0_ls_CS_ls_decs_c__c_Archetype_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: for_each_archetype(hash: ComponentHash; erq: function<EcsRequest>; blk: block<(arch:Archetype const):void> const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+hash    + :ref:`ComponentHash <alias-ComponentHash>`                                    +
+--------+-------------------------------------------------------------------------------+
+erq     +function<>                                                                     +
+--------+-------------------------------------------------------------------------------+
+blk     +block<(arch: :ref:`decs::Archetype <struct-decs-Archetype>`  const):void> const+
+--------+-------------------------------------------------------------------------------+


Invokes block for each entity of each archetype that can be processed by the request.
Request is returned by a specified function.

.. _function-_at_decs_c__c_for_each_archetype_find_CY_ls_ComponentHash_gr_u64_1_ls_S_ls_decs_c__c_EcsRequest_gr__gr__at__at__CN_ls_arch_gr_0_ls_CS_ls_decs_c__c_Archetype_gr__gr_1_ls_b_gr__builtin_:

.. das:function:: for_each_archetype_find(hash: ComponentHash; erq: function<EcsRequest>; blk: block<(arch:Archetype const):bool> const)

for_each_archetype_find returns bool const

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+hash    + :ref:`ComponentHash <alias-ComponentHash>`                                    +
+--------+-------------------------------------------------------------------------------+
+erq     +function<>                                                                     +
+--------+-------------------------------------------------------------------------------+
+blk     +block<(arch: :ref:`decs::Archetype <struct-decs-Archetype>`  const):bool> const+
+--------+-------------------------------------------------------------------------------+


Invokes block for each entity of each archetype that can be processed by the request.
Request is returned by a specified function.
If block returns true, iteration is stopped.
[template(atype)]

.. _function-_at_decs_c__c_decs_array_CY_ls_TT_gr_._C1_ls_u8_gr_A_Ci:

.. das:function:: decs_array(atype: auto(TT) const; src: array<uint8> const; capacity: int const)

decs_array returns auto

+--------+------------------+
+argument+argument type     +
+========+==================+
+atype   +auto(TT) const    +
+--------+------------------+
+src     +array<uint8> const+
+--------+------------------+
+capacity+int const         +
+--------+------------------+


Low level function returns temporary array of component given specific type of component.

.. _function-_at_decs_c__c_get_ro_CS_ls_decs_c__c_Archetype_gr__Cs_C_lb_-1_rb_Y_ls_TT_gr_.:

.. das:function:: get_ro(arch: Archetype const; name: string const; value: auto(TT) const[])

get_ro returns array<TT[-2]> const

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+arch    + :ref:`decs::Archetype <struct-decs-Archetype>`  const+
+--------+------------------------------------------------------+
+name    +string const                                          +
+--------+------------------------------------------------------+
+value   +auto(TT) const[-1]                                    +
+--------+------------------------------------------------------+


Returns const temporary array of component given specific name and type of component for regular components.

.. _function-_at_decs_c__c_get_ro_CS_ls_decs_c__c_Archetype_gr__Cs_CY_ls_TT_gr_._%_ls__ex_()_gr_:

.. das:function:: get_ro(arch: Archetype const; name: string const; value: auto(TT) const)

get_ro returns array<TT> const

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+arch    + :ref:`decs::Archetype <struct-decs-Archetype>`  const+
+--------+------------------------------------------------------+
+name    +string const                                          +
+--------+------------------------------------------------------+
+value   +auto(TT) const                                        +
+--------+------------------------------------------------------+


Returns const temporary array of component given specific name and type of component for regular components.

.. _function-_at_decs_c__c_get_default_ro_CS_ls_decs_c__c_Archetype_gr__Cs_CY_ls_TT_gr_.:

.. das:function:: get_default_ro(arch: Archetype const; name: string const; value: auto(TT) const)

get_default_ro returns iterator<TT const&>

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+arch    + :ref:`decs::Archetype <struct-decs-Archetype>`  const+
+--------+------------------------------------------------------+
+name    +string const                                          +
+--------+------------------------------------------------------+
+value   +auto(TT) const                                        +
+--------+------------------------------------------------------+


Returns const iterator of component given specific name and type of component.
If component is not found - iterator will kepp returning the specified value.

.. _function-_at_decs_c__c_get_optional_CS_ls_decs_c__c_Archetype_gr__Cs_C1_ls_Y_ls_TT_gr_._gr__qm_:

.. das:function:: get_optional(arch: Archetype const; name: string const; value: auto(TT)? const)

get_optional returns iterator<TT?>

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+arch    + :ref:`decs::Archetype <struct-decs-Archetype>`  const+
+--------+------------------------------------------------------+
+name    +string const                                          +
+--------+------------------------------------------------------+
+value   +auto(TT)? const                                       +
+--------+------------------------------------------------------+


Returns const iterator of component given specific name and type of component.
If component is not found - iterator will kepp returning default value for the component type.

+++++++
Request
+++++++

  *  :ref:`EcsRequestPos (at:rtti::LineInfo const) : decs::EcsRequestPos <function-_at_decs_c__c_EcsRequestPos_CH_ls_rtti_c__c_LineInfo_gr_>` 
  *  :ref:`verify_request (erq:decs::EcsRequest -const) : tuple\<ok:bool;error:string\> <function-_at_decs_c__c_verify_request_S_ls_decs_c__c_EcsRequest_gr_>` 
  *  :ref:`compile_request (erq:decs::EcsRequest -const) : void <function-_at_decs_c__c_compile_request_S_ls_decs_c__c_EcsRequest_gr_>` 
  *  :ref:`lookup_request (erq:decs::EcsRequest -const) : int <function-_at_decs_c__c_lookup_request_S_ls_decs_c__c_EcsRequest_gr_>` 

.. _function-_at_decs_c__c_EcsRequestPos_CH_ls_rtti_c__c_LineInfo_gr_:

.. das:function:: EcsRequestPos(at: LineInfo const)

EcsRequestPos returns  :ref:`decs::EcsRequestPos <struct-decs-EcsRequestPos>` 

+--------+----------------------------------------------------+
+argument+argument type                                       +
+========+====================================================+
+at      + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const+
+--------+----------------------------------------------------+


Constructs EcsRequestPos from rtti::LineInfo.

.. _function-_at_decs_c__c_verify_request_S_ls_decs_c__c_EcsRequest_gr_:

.. das:function:: verify_request(erq: EcsRequest)

verify_request returns tuple<ok:bool;error:string>

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+erq     + :ref:`decs::EcsRequest <struct-decs-EcsRequest>` +
+--------+--------------------------------------------------+


Verifies ESC request. Returns pair of boolean (true for OK) and error message.
this queries just about everything
assuming require_not is typically shorter

.. _function-_at_decs_c__c_compile_request_S_ls_decs_c__c_EcsRequest_gr_:

.. das:function:: compile_request(erq: EcsRequest)

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+erq     + :ref:`decs::EcsRequest <struct-decs-EcsRequest>` +
+--------+--------------------------------------------------+


Compiles ESC request, by creating request hash.

.. _function-_at_decs_c__c_lookup_request_S_ls_decs_c__c_EcsRequest_gr_:

.. das:function:: lookup_request(erq: EcsRequest)

lookup_request returns int

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+erq     + :ref:`decs::EcsRequest <struct-decs-EcsRequest>` +
+--------+--------------------------------------------------+


Looks up ESC request in the request cache.


