
.. _stdlib_archive:

=============================
General prupose serialization
=============================

.. include:: detail/archive.rst

The archive module implements general purpose serialization infrastructure.

All functions and symbols are in "archive" module, use require to get access to it. ::

    require daslib/archive

To correctly support serialization of the specific type, you need to define and implement `serialize` method for it.
For example this is how DECS implements component serialization: ::

    def public serialize ( var arch:Archive; var src:Component )
        arch |> serialize(src.name)
        arch |> serialize(src.hash)
        arch |> serialize(src.stride)
        arch |> serialize(src.info)
        invoke(src.info.serializer, arch, src.data)


.. _struct-archive-Archive:

.. das:attribute:: Archive



Archive fields are

+-------+---------------------------------------------------------+
+version+uint                                                     +
+-------+---------------------------------------------------------+
+reading+bool                                                     +
+-------+---------------------------------------------------------+
+stream + :ref:`archive::Serializer <struct-archive-Serializer>` ?+
+-------+---------------------------------------------------------+


Archive is a combination of serialization stream, and state (version, and reading status).

+++++++
Classes
+++++++

.. _struct-archive-Serializer:

.. das:attribute:: Serializer

Base class for serializers.

it defines as follows


.. das:function:: Serializer.write(self: Serializer; bytes: void? const implicit; size: int const)

write returns bool

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`archive::Serializer <struct-archive-Serializer>` +
+--------+--------------------------------------------------------+
+bytes   +void? const implicit                                    +
+--------+--------------------------------------------------------+
+size    +int const                                               +
+--------+--------------------------------------------------------+


Write binary data to stream.

.. das:function:: Serializer.read(self: Serializer; bytes: void? const implicit; size: int const)

read returns bool

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`archive::Serializer <struct-archive-Serializer>` +
+--------+--------------------------------------------------------+
+bytes   +void? const implicit                                    +
+--------+--------------------------------------------------------+
+size    +int const                                               +
+--------+--------------------------------------------------------+


Read binary data from stream.

.. das:function:: Serializer.error(self: Serializer; code: string const)

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`archive::Serializer <struct-archive-Serializer>` +
+--------+--------------------------------------------------------+
+code    +string const                                            +
+--------+--------------------------------------------------------+


Report error to the archive

.. das:function:: Serializer.OK(self: Serializer)

OK returns bool

Return status of the archive

.. _struct-archive-MemSerializer:

.. das:attribute:: MemSerializer : Serializer

This serializer stores data in memory (in the array<uint8>)

it defines as follows

  | data       : array<uint8>
  | readOffset : int
  | lastError  : string

.. das:function:: MemSerializer.write(self: Serializer; bytes: void? const implicit; size: int const)

write returns bool

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`archive::Serializer <struct-archive-Serializer>` +
+--------+--------------------------------------------------------+
+bytes   +void? const implicit                                    +
+--------+--------------------------------------------------------+
+size    +int const                                               +
+--------+--------------------------------------------------------+


Appends bytes at the end of the data.

.. das:function:: MemSerializer.read(self: Serializer; bytes: void? const implicit; size: int const)

read returns bool

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`archive::Serializer <struct-archive-Serializer>` +
+--------+--------------------------------------------------------+
+bytes   +void? const implicit                                    +
+--------+--------------------------------------------------------+
+size    +int const                                               +
+--------+--------------------------------------------------------+


Reads bytes from data, advances the reading position.

.. das:function:: MemSerializer.error(self: Serializer; code: string const)

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`archive::Serializer <struct-archive-Serializer>` +
+--------+--------------------------------------------------------+
+code    +string const                                            +
+--------+--------------------------------------------------------+


Sets the last error code.

.. das:function:: MemSerializer.OK(self: Serializer)

OK returns bool

Implements 'OK' method, which returns true if the serializer is in a valid state.

.. das:function:: MemSerializer.extractData(self: MemSerializer)

extractData returns array<uint8>

Extract the data from the serializer.

.. das:function:: MemSerializer.getCopyOfData(self: MemSerializer)

getCopyOfData returns array<uint8>

Returns copy of the data from the seiralizer.

.. das:function:: MemSerializer.getLastError(self: MemSerializer)

getLastError returns string

Returns last serialization error.

+++++++++++++
Serialization
+++++++++++++

  *  :ref:`serialize (arch:archive::Archive -const;value:math::float3x3 -const) : void <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__H_ls_math_c__c_float3x3_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:math::float3x4 -const) : void <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__H_ls_math_c__c_float3x4_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:math::float4x4 -const) : void <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__H_ls_math_c__c_float4x4_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:string& -const) : void <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&s>` 
  *  :ref:`serialize_raw (arch:archive::Archive -const;value:auto(TT)& -const) : auto <function-_at_archive_c__c_serialize_raw_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_.>` 
  *  :ref:`read_raw (arch:archive::Archive -const;value:auto(TT)& -const) : auto <function-_at_archive_c__c_read_raw_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_.>` 
  *  :ref:`write_raw (arch:archive::Archive -const;value:auto(TT)& -const) : auto <function-_at_archive_c__c_write_raw_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_.>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:auto(TT)& -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_((IsAnyEnumMacro_c_expect_any_enum(value_eq_true)||IsAnyWorkhorseNonPtrMacro_c_expect_any_workhorse_raw(value_eq_true))||IsValueHandle_c_expect_value_handle(value_eq_true))_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:auto(TT)& -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_IsAnyFunctionNonPtrMacro_c_expect_any_function(value_eq_true)_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:auto(TT)& -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_IsAnyStructMacro_c_expect_any_struct(value_eq_true)_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:auto(TT)& -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_IsAnyTupleNonPtrMacro_c_expect_any_tuple(value_eq_true)_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:auto(TT)& -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_IsAnyVariantNonPtrMacro_c_expect_any_variant(value_eq_true)_gr_>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:auto(TT)[] -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__[-1]Y_ls_TT_gr_.>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:array\<auto(TT)\> -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__1_ls_Y_ls_TT_gr_._gr_A>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:table\<auto(KT);auto(VT)\> -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__1_ls_Y_ls_KT_gr_._gr_2_ls_Y_ls_VT_gr_._gr_T>` 
  *  :ref:`serialize (arch:archive::Archive -const;value:auto(TT)? -const) : auto <function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__1_ls_Y_ls_TT_gr_._gr_?>` 

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__H_ls_math_c__c_float3x3_gr_:

.. das:function:: serialize(arch: Archive; value: float3x3)

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   + :ref:`math::float3x3 <handle-math-float3x3>`     +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__H_ls_math_c__c_float3x4_gr_:

.. das:function:: serialize(arch: Archive; value: float3x4)

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   + :ref:`math::float3x4 <handle-math-float3x4>`     +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__H_ls_math_c__c_float4x4_gr_:

.. das:function:: serialize(arch: Archive; value: float4x4)

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   + :ref:`math::float4x4 <handle-math-float4x4>`     +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&s:

.. das:function:: serialize(arch: Archive; value: string&)

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +string&                                           +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_raw_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_.:

.. das:function:: serialize_raw(arch: Archive; value: auto(TT)&)

serialize_raw returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)&                                         +
+--------+--------------------------------------------------+


Serialize raw data (straight up bytes for raw pod)

.. _function-_at_archive_c__c_read_raw_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_.:

.. das:function:: read_raw(arch: Archive; value: auto(TT)&)

read_raw returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)&                                         +
+--------+--------------------------------------------------+


Read raw data (straight up bytes for raw pod)

.. _function-_at_archive_c__c_write_raw_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_.:

.. das:function:: write_raw(arch: Archive; value: auto(TT)&)

write_raw returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)&                                         +
+--------+--------------------------------------------------+


Write raw data (straight up bytes for raw pod)

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_((IsAnyEnumMacro_c_expect_any_enum(value_eq_true)||IsAnyWorkhorseNonPtrMacro_c_expect_any_workhorse_raw(value_eq_true))||IsValueHandle_c_expect_value_handle(value_eq_true))_gr_:

.. das:function:: serialize(arch: Archive; value: auto(TT)&)

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)&                                         +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_IsAnyFunctionNonPtrMacro_c_expect_any_function(value_eq_true)_gr_:

.. das:function:: serialize(arch: Archive; value: auto(TT)&)

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)&                                         +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_IsAnyStructMacro_c_expect_any_struct(value_eq_true)_gr_:

.. das:function:: serialize(arch: Archive; value: auto(TT)&)

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)&                                         +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_IsAnyTupleNonPtrMacro_c_expect_any_tuple(value_eq_true)_gr_:

.. das:function:: serialize(arch: Archive; value: auto(TT)&)

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)&                                         +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__&Y_ls_TT_gr_._%_ls_IsAnyVariantNonPtrMacro_c_expect_any_variant(value_eq_true)_gr_:

.. das:function:: serialize(arch: Archive; value: auto(TT)&)

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)&                                         +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__[-1]Y_ls_TT_gr_.:

.. das:function:: serialize(arch: Archive; value: auto(TT)[])

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)[-1]                                      +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__1_ls_Y_ls_TT_gr_._gr_A:

.. das:function:: serialize(arch: Archive; value: array<auto(TT)>)

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +array<auto(TT)>                                   +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__1_ls_Y_ls_KT_gr_._gr_2_ls_Y_ls_VT_gr_._gr_T:

.. das:function:: serialize(arch: Archive; value: table<auto(KT);auto(VT)>)

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +table<auto(KT);auto(VT)>                          +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

.. _function-_at_archive_c__c_serialize_S_ls_archive_c__c_Archive_gr__1_ls_Y_ls_TT_gr_._gr_?:

.. das:function:: serialize(arch: Archive; value: auto(TT)?)

serialize returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+arch    + :ref:`archive::Archive <struct-archive-Archive>` +
+--------+--------------------------------------------------+
+value   +auto(TT)?                                         +
+--------+--------------------------------------------------+


Serializes structured data, based on the `value` type.

++++++++++++++
Memory archive
++++++++++++++

  *  :ref:`mem_archive_save (t:auto& -const) : auto <function-_at_archive_c__c_mem_archive_save_&.>` 
  *  :ref:`mem_archive_load (data:array\<uint8\> -const;t:auto& -const;canfail:bool const) : bool <function-_at_archive_c__c_mem_archive_load_1_ls_u8_gr_A_&._Cb>` 

.. _function-_at_archive_c__c_mem_archive_save_&.:

.. das:function:: mem_archive_save(t: auto&)

mem_archive_save returns auto

+--------+-------------+
+argument+argument type+
+========+=============+
+t       +auto&        +
+--------+-------------+


Saves the object to a memory archive. Result is array<uint8> with the serialized data.

.. _function-_at_archive_c__c_mem_archive_load_1_ls_u8_gr_A_&._Cb:

.. das:function:: mem_archive_load(data: array<uint8>; t: auto&; canfail: bool const)

mem_archive_load returns bool

+--------+-------------+
+argument+argument type+
+========+=============+
+data    +array<uint8> +
+--------+-------------+
+t       +auto&        +
+--------+-------------+
+canfail +bool const   +
+--------+-------------+


Loads the object from a memory archive. `data` is the array<uint8> with the serialized data, returned from `mem_archive_save`.


