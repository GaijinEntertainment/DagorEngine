To use daBfg inside daScript you need first of all compile daBfg library with ``DABFG_ENABLE_DAS_INTERGRATION = yes``.

This will compile das module that you can import with ``require daBfg``.

DaScript daBfg methods are very similar to cpp methods, so usage will be the same, but with das syntax instead.

:ref:`daBfg::registerNode <function-_at_daBfg_c__c_registerNode_Cs_N_ls_reg_gr_0_ls_H_ls_daBfg_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>` registers node with provided name and declaration callback.
Returns :ref:`daBfg::NodeHandle <handle-daBfg-NodeHandle>`.

Declaration callback is a das lambda with one argument :ref:`daBfg::Registry <handle-daBfg-Registry>`.
It returns execute lambda.

Inside declaration callback you describe node using registry argument.

Cpp declaration code

.. code-block:: cpp

  registry.orderMeAfter("some_node")
  registry.requestRenderPass().color("rt_tex")

Will be in das

.. code-block:: das

  registry |> orderMeAfter("some_node")
  registry |> requestRenderPass |> color([[auto "rt_tex"]])

Example:

.. code-block:: das

  require daBfg

  def register_debug_visualization_node(var handle : NodeHandle& |#)
    handle <- registerNode(debug_visualization_node_name) <| @(var registry : Registry)
      if is_forward_rendering()
        registry |> requestRenderPass |> color([[auto[] "target_after_under_water_fog"]]) |> depthRo("depth_for_transparent_effects")
      else
        registry |> orderMeAfter("transparent_effects_node")
        registry |> orderMeBefore("transparent_scene_late_node")
        registry |> requestRenderPass |> color([[auto[] "opaque_final_target"]]) |> depthRw("depth_for_transparency")

      registry |> requestState() |> setFrameBlock("global_frame")
      return <- @ <|
        worldRenderer_renderDebug()


Nodes can be stored in ecs and in this case there is special function annotation :ref:`bfg_ecs_node <handle-daBfg-bfg_ecs_node>` for hot reloading.

Example:

.. code-block:: das

  [bfg_ecs_node(name="node_name", entity="entity_name", handle="node_handle")]
  def register_node(var handle : NodeHandle& |#)
    handle <- registerNode("node_name") <| @(var registry : Registry)
      ...
      return <- @ <|
        ...

  ...

  var eid = getSingletonEntity("entity_name")
  query(eid) <| $ [es] (var node_handle : NodeHandle&)
    node_handle |> register_node