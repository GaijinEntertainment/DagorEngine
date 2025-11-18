..
  This is auto generated file. See daFrameGraph/api/das/docs

.. _stdlib_resource_slot:

========================
Resource Slot in Daslang
========================

Storing in :ref:`Daslang + ECS<stdlib_resource_slot_ecs>` is more convenient.

However, we can store :ref:`NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>` outside ECS with ``daFrameGraph.resource_slot`` module.

Unfortunately, ``NodeHandleWithSlotsAccess`` can't be stored in local
Daslang variable, because das doesn't call destructors for local types.
If you really need to store handle outside ECS, you have to store
handle in heap or make binding for storing it on cpp-side.

Also hot-reload will work automatically for nodes inside
:ref:`ECS <stdlib_resource_slot_ecs>`.

.. code-block:: text

    require daFrameGraph.resource_slot

    some_cpp_binding <| $(var handle : NodeHandleWithSlotsAccess &)
      some_struct.handle <- root() |> registerAccess("node_name", [[SlotActions update <- [{auto Update("slot_name", "texture_name", 100)}] ]]) <| @(slots_state; var registry : Registry)

        return <- @ <|
          // some render code

+++++++++
Constants
+++++++++

.. _global-resource_slot-DEFAULT_READ_PRIORITY:

.. das:attribute:: DEFAULT_READ_PRIORITY = 2147483647

.. _struct-resource_slot-Create:

.. das:attribute:: Create

Create fields are

+--------+------+
+slot    +string+
+--------+------+
+resource+string+
+--------+------+

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

.. _struct-resource_slot-Read:

.. das:attribute:: Read

Read fields are

+--------+------+
+slot    +string+
+--------+------+
+priority+int   +
+--------+------+

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

+++++++++++++++
Register Access
+++++++++++++++

 * :ref:`registerAccess (self:daFrameGraph::NameSpace -const;name:string const;slot_decl:resource_slot::SlotActions const;declaration_callback:lambda\<(slots_state:ResourceSlotCore::State const;var reg:daFrameGraph::Registry -const):lambda\<void\>\> -const) : ResourceSlotCore::NodeHandleWithSlotsAccess <function-_at_resource_slot_c__c_registerAccess_S_ls_daFrameGraph_c__c_NameSpace_gr__Cs_CS_ls_resource_slot_c__c_SlotActions_gr__N_ls_slots_state;reg_gr_0_ls_CH_ls_ResourceSlotCore_c__c_State_gr_;S_ls_daFrameGraph_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>`

.. _function-_at_resource_slot_c__c_registerAccess_S_ls_daFrameGraph_c__c_NameSpace_gr__Cs_CS_ls_resource_slot_c__c_SlotActions_gr__N_ls_slots_state;reg_gr_0_ls_CH_ls_ResourceSlotCore_c__c_State_gr_;S_ls_daFrameGraph_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_:

.. das:function:: registerAccess(self: NameSpace; name: string const; slot_decl: SlotActions const; declaration_callback: lambda<(slots_state:ResourceSlotCore::State const;var reg:daFrameGraph::Registry -const):lambda<void>>)

``registerAccess`` returns  :ref:`ResourceSlotCore::NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>`.

+--------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+Argument            +Argument type                                                                                                                                                           +
+====================+========================================================================================================================================================================+
+self                + :ref:`daFrameGraph::NameSpace <struct-daFrameGraph-NameSpace>`                                                                                                         +
+--------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+name                +string const                                                                                                                                                            +
+--------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+slot_decl           + :ref:`resource_slot::SlotActions <struct-resource_slot-SlotActions>`  const                                                                                            +
+--------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+declaration_callback+lambda<(slots_state: :ref:`ResourceSlotCore::State <handle-ResourceSlotCore-State>`  const;reg: :ref:`daFrameGraph::Registry <struct-daFrameGraph-Registry>` ):lambda<>>+
+--------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

