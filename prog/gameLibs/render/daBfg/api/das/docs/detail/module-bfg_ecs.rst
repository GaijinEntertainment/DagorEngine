Nodes can be stored in ecs **singleton** and in this case there is special function annotation :ref:`bfg_ecs_node <handle-bfg_ecs-bfg_ecs_node>` for hot reloading.

Function should take :ref:`NodeHandle <handle-daBfgCore-NodeHandle>` or :ref:`NodeHandleVector <handle-builtin-NodeHandleVector>` as first argument and set up it.

Another arguments of function can be any other components of singleton entity and will be passed to es macro as is.

Possible arguments for bfg_ecs_node annotation itself:

- ``on_appear`` - call function on appear of entity.
- ``on_event=EventName`` - call function on event.
- ``track=component_name`` - call function on change of component.

Example:

.. code-block:: das

  require daBfg.bfg_ecs

  [bfg_ecs_node(on_appear)]
  def register_some_node(var some_node : NodeHandle&)
    some_node <- root() |> registerNode("some_node") <| @(var registry : Registry)

      // Node requests, for example:
      registry |> requestRenderPass |> color([[auto[] "some_tex"]])

      return <- @ <|

        // Render code, for example:
        query() <| $ [es] (some_shader : PostFxRenderer)
          some_shader |> render()