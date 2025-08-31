To use daFG inside daslang you need first of all compile daFG library with ``DAFG_ENABLE_DAS_INTERGRATION = yes``.

This will compile das module that you can import with ``require daFrameGraph.fg_ecs``
or ``require daFrameGraph``, whether you need ecs support or not.

Daslang daFG methods are very similar to cpp methods, so usage will be the same, but with das syntax instead.

:ref:`daFG::registerNode <function-_at_DAFG_c__c_registerNode_S_ls_DAFG_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_H_ls_daFgCore_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>` registers node with provided name and declaration callback.
Returns :ref:`NodeHandle <handle-daFgCore-NodeHandle>`.

Declaration callback is a das lambda with one argument :ref:`Registry <handle-daFgCore-Registry>`.
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

    require daFrameGraph

    def register_debug_visualization_node(var handle : NodeHandle& |#)
      handle <- root() |> registerNode(debug_visualization_node_name) <| @(var registry : Registry)
        if is_forward_rendering()
          registry |> requestRenderPass |> color([[auto[] "target_after_under_water_fog"]]) |> depthRo("depth_for_transparent_effects")
        else
          registry |> orderMeAfter("tracers_node")
          registry |> orderMeBefore("transparent_scene_late_node")
          registry |> requestRenderPass |> color([[auto[] "opaque_final_target"]]) |> depthRw("depth_for_transparency")

        registry |> requestState() |> setFrameBlock("global_frame")
        return <- @ <|
          worldRenderer_renderDebug()
