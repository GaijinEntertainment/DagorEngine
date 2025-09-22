..
  This is auto generated file. See daFrameGraph/api/das/docs

.. _stdlib_resource_slot_ecs:

==============================
Resource Slot in Daslang + ECS
==============================

We can handle in ECS :ref:`NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>` as we handle :ref:`NodeHandle <handle-DaFgCore-NodeHandle>`.

Let's say we have Daslang code for node:

.. code-block:: text

    require daFrameGraph.fg_ecs

    [fg_ecs_node(on_appear)]
    def register_some_node(var some_node_handle : NodeHandle&)

      some_node_handle <- root() |> registerNode("some_node") <| @(slots_state; var registry : Registry)

        registry |> create("some_node_output_tex", History No) |> texture(...)
        registry |> requestRenderPass |> color([[auto[] "some_node_output_tex"]])

        registry |> read("some_node_input_tex") |> texture |> atStage(Stage POST_RASTER) |> bindToShaderVar("some_shader_var")

        return <- @ <|
          query() <| $ [es] (some_renderer : PostFxRenderer)
            some_renderer |> render


We can convert it to read texture from and write back to resource slot ``slot_name``:

.. code-block:: text

    require daFrameGraph.resource_slot_ecs

    [resource_slot_ecs(on_appear)]
    def register_some_node(var some_node_handle : NodeHandleWithSlotsAccess&)

      some_node_handle <- root() |> registerAccess("some_node", [[SlotActions update <- [{Update slot="slot_name", resource="some_node_output_tex", priority=100}] ]]) <| @(slots_state; var registry : Registry)

        registry |> create(slots_state |> resourceToCreateFor("slot_name"), History No) |> texture(...)
        registry |> requestRenderPass |> color([[auto[] slots_state |> resourceToCreateFor("postfx_input_slot")]])

        registry |> read(slots_state |> resourceToReadFrom("slot_name")) |> texture |> atStage(Stage POST_RASTER) |> bindToShaderVar("some_shader_var")

        return <- @ <|
          query() <| $ [es] (some_renderer : PostFxRenderer)
            some_renderer |> render

+++++++++++++++++++++++
Difference with fg_ecs
+++++++++++++++++++++++

+----------------------------+-----------------------------------------------------+
+fg_ecs                      +resource_slot_ecs                                    +
+============================+=====================================================+
+require daFrameGraph.fg_ecs + require daFrameGraph.resource_slot_ecs              +
+----------------------------+-----------------------------------------------------+
+[fg_ecs_node(on_appear)]    + [resource_slot_ecs(on_appear)]                      +
+----------------------------+-----------------------------------------------------+
+NodeHandle                  + NodeHandleWithSlotsAccess                           +
+----------------------------+-----------------------------------------------------+
+registerNode("node_name")   + registerAccess("node_name", [[SlotActions ... ]])   +
+----------------------------+-----------------------------------------------------+
+@(var registry : Registry)  + @(slots_state; var registry : Registry)             +
+----------------------------+-----------------------------------------------------+
+"some_node_input_tex"       + \`slots_state |> resourceToReadFrom("slot_name")\`  +
+----------------------------+-----------------------------------------------------+
+"some_node_output_tex"      + \`slots_state |> resourceToCreateFor("slot_name")\` +
+----------------------------+-----------------------------------------------------+

:ref:`SlotActions <struct-resource_slot-SlotActions>` have 3 possible properties:

- create : array<:ref:`Create <struct-resource_slot-Create>`>
- update : array<:ref:`Update <struct-resource_slot-Update>`>
- read : array<:ref:`Read <struct-resource_slot-Read>`>

.. seealso:: For more information, see :ref:`registerAccess <function-_at_resource_slot_c__c_registerAccess_S_ls_daFrameGraph_c__c_NameSpace_gr__Cs_CS_ls_resource_slot_c__c_SlotActions_gr__N_ls_slots_state;reg_gr_0_ls_CH_ls_ResourceSlotCore_c__c_State_gr_;S_ls_daFrameGraph_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>`.

++++++++++++++++++++
Function Annotations
++++++++++++++++++++

.. _handle-resource_slot_ecs-resource_slot_ecs:

.. das:attribute:: resource_slot_ecs

+++++++
Classes
+++++++

.. _struct-resource_slot_ecs-ResourceSlotEcsAnnotation:

.. das:attribute:: ResourceSlotEcsAnnotation : AstFunctionAnnotation

.. das:function:: ResourceSlotEcsAnnotation.apply(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

``apply`` returns bool.

+--------+-------------------------------------+
+Argument+Argument type                        +
+========+=====================================+
+self    + ast::AstFunctionAnnotation          +
+--------+-------------------------------------+
+func    + FunctionPtr                         +
+--------+-------------------------------------+
+group   + rtti::ModuleGroup                   +
+--------+-------------------------------------+
+args    + rtti::AnnotationArgumentList  const +
+--------+-------------------------------------+
+errors  + :builtin::das_string                +
+--------+-------------------------------------+

.. das:function:: ResourceSlotEcsAnnotation.parseArgs(self: ResourceSlotEcsAnnotation; func: FunctionPtr; args: AnnotationArgumentList const; errors: das_string)

``parseArgs`` returns ``resource_slot_ecs::ResSlotEcsAnnotationArgs``.

+--------+---------------------------------------------------------------------------------------------------------+
+Argument+Argument type                                                                                            +
+========+=========================================================================================================+
+self    + :ref:`resource_slot_ecs::ResourceSlotEcsAnnotation <struct-resource_slot_ecs-ResourceSlotEcsAnnotation>`+
+--------+---------------------------------------------------------------------------------------------------------+
+func    + FunctionPtr                                                                                             +
+--------+---------------------------------------------------------------------------------------------------------+
+args    + rtti::AnnotationArgumentList  const                                                                     +
+--------+---------------------------------------------------------------------------------------------------------+
+errors  + builtin::das_string                                                                                     +
+--------+---------------------------------------------------------------------------------------------------------+

.. das:function:: ResourceSlotEcsAnnotation.declareReloadCallback(self: ResourceSlotEcsAnnotation; func: FunctionPtr; parsed: ResSlotEcsAnnotationArgs const; args: AnnotationArgumentList const)

+--------+---------------------------------------------------------------------------------------------------------+
+Argument+Argument type                                                                                            +
+========+=========================================================================================================+
+self    + :ref:`resource_slot_ecs::ResourceSlotEcsAnnotation <struct-resource_slot_ecs-ResourceSlotEcsAnnotation>`+
+--------+---------------------------------------------------------------------------------------------------------+
+func    + FunctionPtr                                                                                             +
+--------+---------------------------------------------------------------------------------------------------------+
+parsed  + resource_slot_ecs::ResSlotEcsAnnotationArgs  const                                                      +
+--------+---------------------------------------------------------------------------------------------------------+
+args    + rtti::AnnotationArgumentList  const                                                                     +
+--------+---------------------------------------------------------------------------------------------------------+

.. das:function:: ResourceSlotEcsAnnotation.declareES(self: ResourceSlotEcsAnnotation; func: FunctionPtr; parsed: ResSlotEcsAnnotationArgs const; args: AnnotationArgumentList const)

+--------+---------------------------------------------------------------------------------------------------------+
+Argument+Argument type                                                                                            +
+========+=========================================================================================================+
+self    + :ref:`resource_slot_ecs::ResourceSlotEcsAnnotation <struct-resource_slot_ecs-ResourceSlotEcsAnnotation>`+
+--------+---------------------------------------------------------------------------------------------------------+
+func    + FunctionPtr                                                                                             +
+--------+---------------------------------------------------------------------------------------------------------+
+parsed  + resource_slot_ecs::ResSlotEcsAnnotationArgs  const                                                      +
+--------+---------------------------------------------------------------------------------------------------------+
+args    + rtti::AnnotationArgumentList  const                                                                     +
+--------+---------------------------------------------------------------------------------------------------------+