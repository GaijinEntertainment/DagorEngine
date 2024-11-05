..
  This is auto generated file. See daBfg/api/das/docs

.. _stdlib_ResourceSlotCore:

===========================
ResourceSlotCore das module
===========================

ResourceSlotCore das module gives low-level bindings to cpp.


++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-ResourceSlotCore-NodeHandleWithSlotsAccess:

.. das:attribute:: NodeHandleWithSlotsAccess

NodeHandleWithSlotsAccess property operators are

+-----+----+
+valid+bool+
+-----+----+


|structure_annotation-ResourceSlotCore-NodeHandleWithSlotsAccess|

.. _handle-ResourceSlotCore-ActionList:

.. das:attribute:: ActionList

|structure_annotation-ResourceSlotCore-ActionList|

.. _handle-ResourceSlotCore-State:

.. das:attribute:: State

|structure_annotation-ResourceSlotCore-State|

++++++++++++++++
State operations
++++++++++++++++

  *  :ref:`resourceToReadFrom (state:ResourceSlotCore::State const implicit;slot_name:string const implicit) : string const <function-_at_ResourceSlotCore_c__c_resourceToReadFrom_CIH_ls_ResourceSlotCore_c__c_State_gr__CIs>` 
  *  :ref:`resourceToCreateFor (state:ResourceSlotCore::State const implicit;slot_name:string const implicit) : string const <function-_at_ResourceSlotCore_c__c_resourceToCreateFor_CIH_ls_ResourceSlotCore_c__c_State_gr__CIs>` 

.. _function-_at_ResourceSlotCore_c__c_resourceToReadFrom_CIH_ls_ResourceSlotCore_c__c_State_gr__CIs:

.. das:function:: resourceToReadFrom(state: State const implicit; slot_name: string const implicit)

resourceToReadFrom returns string const

+---------+-------------------------------------------------------------------------------+
+argument +argument type                                                                  +
+=========+===============================================================================+
+state    + :ref:`ResourceSlotCore::State <handle-ResourceSlotCore-State>`  const implicit+
+---------+-------------------------------------------------------------------------------+
+slot_name+string const implicit                                                          +
+---------+-------------------------------------------------------------------------------+


|function-ResourceSlotCore-resourceToReadFrom|

.. _function-_at_ResourceSlotCore_c__c_resourceToCreateFor_CIH_ls_ResourceSlotCore_c__c_State_gr__CIs:

.. das:function:: resourceToCreateFor(state: State const implicit; slot_name: string const implicit)

resourceToCreateFor returns string const

+---------+-------------------------------------------------------------------------------+
+argument +argument type                                                                  +
+=========+===============================================================================+
+state    + :ref:`ResourceSlotCore::State <handle-ResourceSlotCore-State>`  const implicit+
+---------+-------------------------------------------------------------------------------+
+slot_name+string const implicit                                                          +
+---------+-------------------------------------------------------------------------------+


|function-ResourceSlotCore-resourceToCreateFor|

++++++++++++++
Handle methods
++++++++++++++

  *  :ref:`NodeHandleWithSlotsAccess () : ResourceSlotCore::NodeHandleWithSlotsAccess <function-_at_ResourceSlotCore_c__c_NodeHandleWithSlotsAccess>` 
  *  :ref:`reset (handle:ResourceSlotCore::NodeHandleWithSlotsAccess implicit) : void <function-_at_ResourceSlotCore_c__c_reset_IH_ls_ResourceSlotCore_c__c_NodeHandleWithSlotsAccess_gr_>` 

.. _function-_at_ResourceSlotCore_c__c_NodeHandleWithSlotsAccess:

.. das:function:: NodeHandleWithSlotsAccess()

NodeHandleWithSlotsAccess returns  :ref:`ResourceSlotCore::NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>` 

|function-ResourceSlotCore-NodeHandleWithSlotsAccess|

.. _function-_at_ResourceSlotCore_c__c_reset_IH_ls_ResourceSlotCore_c__c_NodeHandleWithSlotsAccess_gr_:

.. das:function:: reset(handle: NodeHandleWithSlotsAccess implicit)

+--------+-----------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                    +
+========+=================================================================================================================+
+handle  + :ref:`ResourceSlotCore::NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>`  implicit+
+--------+-----------------------------------------------------------------------------------------------------------------+


|function-ResourceSlotCore-reset|


