We can handle in ecs :ref:`NodeHandleWithSlotsAccess <handle-ResourceSlotCore-NodeHandleWithSlotsAccess>` as we handle :ref:`NodeHandle <handle-DaBfgCore-NodeHandle>`

Let's say we have das code for node:

.. code-block:: das

  require daBfg.bfg_ecs

  [bfg_ecs_node(on_appear)]
  def register_some_node(var some_node_handle : NodeHandle&)

    some_node_handle <- root() |> registerNode("some_node") <| @(slots_state; var registry : Registry)

      registry |> create("some_node_output_tex", History No) |> texture(...)
      registry |> requestRenderPass |> color([[auto[] "some_node_output_tex"]])

      registry |> read("some_node_input_tex") |> texture |> atStage(Stage POST_RASTER) |> bindToShaderVar("some_shader_var")

      return <- @ <|
        query() <| $ [es] (some_renderer : PostFxRenderer)
          some_renderer |> render


We can convert it to read texture from and write back to resource slot `slot_name`:

.. code-block:: das

  require daBfg.resource_slot_ecs

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
Difference with bfg_ecs
+++++++++++++++++++++++

+---------------------------+---------------------------------------------------+
+bfg_ecs                    +resource_slot_ecs                                  +
+===========================+===================================================+
+require daBfg.bfg_ecs      + require daBfg.resource_slot_ecs                   +
+---------------------------+---------------------------------------------------+
+[bfg_ecs_node(on_appear)]  + [resource_slot_ecs(on_appear)]                    +
+---------------------------+---------------------------------------------------+
+NodeHandle                 + NodeHandleWithSlotsAccess                         +
+---------------------------+---------------------------------------------------+
+registerNode("node_name")  + registerAccess("node_name", [[SlotActions ... ]]) +
+---------------------------+---------------------------------------------------+
+@(var registry : Registry) + @(slots_state; var registry : Registry)           +
+---------------------------+---------------------------------------------------+
+"some_node_input_tex"      + `slots_state |> resourceToReadFrom("slot_name")`  +
+---------------------------+---------------------------------------------------+
+"some_node_output_tex"     + `slots_state |> resourceToCreateFor("slot_name")` +
+---------------------------+---------------------------------------------------+

:ref:`SlotActions <struct-resource_slot-SlotActions>` have 3 possible properties:

- create : array<:ref:`Create <struct-resource_slot-Create>`>
- update : array<:ref:`Update <struct-resource_slot-Update>`>
- read : array<:ref:`Read <struct-resource_slot-Read>`>

See also: :ref:`registerAccess <function-_at_resource_slot_c__c_registerAccess_S_ls_daBfg_c__c_NameSpace_gr__Cs_CS_ls_resource_slot_c__c_SlotActions_gr__N_ls_slots_state;reg_gr_0_ls_CH_ls_ResourceSlotCore_c__c_State_gr_;S_ls_daBfg_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>`

