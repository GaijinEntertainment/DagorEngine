..
  This is auto generated file. See daBfg/api/das/docs

.. _stdlib_resource_slot:

====================
Resource slot in das
====================

Storing in :ref:`das+ecs <stdlib_resource_slot_ecs>` is more convenient.

However, we can store :ref:`NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>` outside ecs with `daBfg.resource_slot` module.

Unfortunately, NodeHandleWithSlotsAccess can't be stored in local
das variable, because das doesn't call destructors for local types.
If you really need to store handle outside ecs, you have to store
handle in heap or make binding for storing it on cpp-side.

Also hot-reload will work automatically for nodes inside
:ref:`ecs <stdlib_resource_slot_ecs>`.

.. code-block:: das

  require daBfg.resource_slot

  some_cpp_binding <| $(var handle : NodeHandleWithSlotsAccess &)
    some_struct.handle <- root() |> registerAccess("node_name", [[SlotActions update <- [{auto Update("slot_name", "texture_name", 100)}] ]]) <| @(slots_state; var registry : Registry)

      return <- @ <|
        // some render code


+++++++++
Constants
+++++++++

.. _global-resource_slot-DEFAULT_READ_PRIORITY:

.. das:attribute:: DEFAULT_READ_PRIORITY = 2147483647

|variable-resource_slot-DEFAULT_READ_PRIORITY|

.. _struct-resource_slot-Create:

.. das:attribute:: Create



Create fields are

+--------+------+
+slot    +string+
+--------+------+
+resource+string+
+--------+------+


|structure-resource_slot-Create|

.. _struct-resource_slot-Update:

.. das:attribute:: Update



Update fields are

+--------+------+
+slot    +string+
+--------+------+
+resource+string+
+--------+------+
+priority+int   +
+--------+------+


|structure-resource_slot-Update|

.. _struct-resource_slot-Read:

.. das:attribute:: Read



Read fields are

+--------+------+
+slot    +string+
+--------+------+
+priority+int   +
+--------+------+


|structure-resource_slot-Read|

.. _struct-resource_slot-SlotActions:

.. das:attribute:: SlotActions



SlotActions fields are

+------+-------------------------------------------------------------------+
+create+array< :ref:`resource_slot::Create <struct-resource_slot-Create>` >+
+------+-------------------------------------------------------------------+
+update+array< :ref:`resource_slot::Update <struct-resource_slot-Update>` >+
+------+-------------------------------------------------------------------+
+read  +array< :ref:`resource_slot::Read <struct-resource_slot-Read>` >    +
+------+-------------------------------------------------------------------+


|structure-resource_slot-SlotActions|

+++++++++++++++
Register access
+++++++++++++++

  *  :ref:`registerAccess (self:daBfg::NameSpace -const;name:string const;slot_decl:resource_slot::SlotActions const;declaration_callback:lambda\<(slots_state:ResourceSlotCore::State const;var reg:daBfg::Registry -const):lambda\<void\>\> -const) : ResourceSlotCore::NodeHandleWithSlotsAccess <function-_at_resource_slot_c__c_registerAccess_S_ls_daBfg_c__c_NameSpace_gr__Cs_CS_ls_resource_slot_c__c_SlotActions_gr__N_ls_slots_state;reg_gr_0_ls_CH_ls_ResourceSlotCore_c__c_State_gr_;S_ls_daBfg_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>` 

.. _function-_at_resource_slot_c__c_registerAccess_S_ls_daBfg_c__c_NameSpace_gr__Cs_CS_ls_resource_slot_c__c_SlotActions_gr__N_ls_slots_state;reg_gr_0_ls_CH_ls_ResourceSlotCore_c__c_State_gr_;S_ls_daBfg_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_:

.. das:function:: registerAccess(self: NameSpace; name: string const; slot_decl: SlotActions const; declaration_callback: lambda<(slots_state:ResourceSlotCore::State const;var reg:daBfg::Registry -const):lambda<void>>)

registerAccess returns  :ref:`ResourceSlotCore::NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>` 

+--------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
+argument            +argument type                                                                                                                                             +
+====================+==========================================================================================================================================================+
+self                + :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>`                                                                                                         +
+--------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
+name                +string const                                                                                                                                              +
+--------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
+slot_decl           + :ref:`resource_slot::SlotActions <struct-resource_slot-SlotActions>`  const                                                                              +
+--------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
+declaration_callback+lambda<(slots_state: :ref:`ResourceSlotCore::State <handle-ResourceSlotCore-State>`  const;reg: :ref:`daBfg::Registry <struct-daBfg-Registry>` ):lambda<>>+
+--------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+


|function-resource_slot-registerAccess|


