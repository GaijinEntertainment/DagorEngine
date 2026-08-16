..
  This is auto generated file. See daBfg/api/das/docs

.. _stdlib_fg_ecs:

=============================
daFrameGraph in Daslang + ECS
=============================

Nodes can be stored in ECS **singleton** and in this case there is special function annotation :ref:`_stdlib_fg_ecs <handle-fg_ecs-fg_ecs_node>` for hot reloading.

Function should take :ref:`NodeHandle <handle-daFgCore-NodeHandle>` or :ref:`NodeHandleVector <handle-builtin-NodeHandleVector>` as first argument and set up it.

Another arguments of function can be any other components of singleton entity and will be passed to es macro as is.

Possible arguments for fg_ecs_node annotation itself:

- ``on_appear`` - call function on appear of entity.
- ``on_event=EventName`` - call function on event.
- ``track=component_name`` - call function on change of component.

**Example**:

.. code-block:: text

    require daFrameGraph.fg_ecs

    [fg_ecs_node(on_appear)]
    def register_some_node(var some_node : NodeHandle&)
      some_node <- root() |> registerNode("some_node") <| @(var registry : Registry)

        // Node requests, for example:
        registry |> requestRenderPass |> color([[auto[] "some_tex"]])

        return <- @ <|

          // Render code, for example:
          query() <| $ [es] (some_shader : PostFxRenderer)
            some_shader |> render()

++++++++++++++++++++
Function Annotations
++++++++++++++++++++

.. _handle-fg_ecs-fg_ecs_node:

.. das:attribute:: fg_ecs_node

|function_annotation-fg_ecs-fg_ecs_node|

+++++++
Classes
+++++++

.. _struct-fg_ecs-FgEcsNodeAnnotation:

.. das:attribute:: FgEcsNodeAnnotation : AstFunctionAnnotation

|class-fg_ecs-FgEcsNodeAnnotation|

.. das:function:: FgEcsNodeAnnotation.apply(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

apply returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`           +
+--------+--------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-fg_ecs-FgEcsNodeAnnotation.apply|

.. das:function:: FgEcsNodeAnnotation.declareReloadCallback(self: FgEcsNodeAnnotation; func: FunctionPtr; parsed: FgEcsNodeAnnotationArgs const; args: AnnotationArgumentList const)

+--------+------------------------------------------------------------------------------------------+
+argument+argument type                                                                             +
+========+==========================================================================================+
+self    + :ref:`fg_ecs::FgEcsNodeAnnotation <struct-fg_ecs-FgEcsNodeAnnotation>`                   +
+--------+------------------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                                   +
+--------+------------------------------------------------------------------------------------------+
+parsed  + :ref:`fg_ecs::FgEcsNodeAnnotationArgs <struct-fg_ecs-FgEcsNodeAnnotationArgs>`  const    +
+--------+------------------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const          +
+--------+------------------------------------------------------------------------------------------+


|method-fg_ecs-FgEcsNodeAnnotation.declareReloadCallback|

.. das:function:: FgEcsNodeAnnotation.declareES(self: FgEcsNodeAnnotation; func: FunctionPtr; parsed: FgEcsNodeAnnotationArgs const; args: AnnotationArgumentList const)

+--------+------------------------------------------------------------------------------------------+
+argument+argument type                                                                             +
+========+==========================================================================================+
+self    + :ref:`fg_ecs::FgEcsNodeAnnotation <struct-fg_ecs-FgEcsNodeAnnotation>`                   +
+--------+------------------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                                   +
+--------+------------------------------------------------------------------------------------------+
+parsed  + :ref:`fg_ecs::FgEcsNodeAnnotationArgs <struct-fg_ecs-FgEcsNodeAnnotationArgs>`  const    +
+--------+------------------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const          +
+--------+------------------------------------------------------------------------------------------+


|method-fg_ecs-FgEcsNodeAnnotation.declareES|

.. das:function:: FgEcsNodeAnnotation.parseArgs(self: FgEcsNodeAnnotation; func: FunctionPtr; args: AnnotationArgumentList const; errors: das_string)

parseArgs returns  :ref:`fg_ecs::FgEcsNodeAnnotationArgs <struct-fg_ecs-FgEcsNodeAnnotationArgs>`

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`fg_ecs::FgEcsNodeAnnotation <struct-fg_ecs-FgEcsNodeAnnotation>`         +
+--------+--------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-fg_ecs-FgEcsNodeAnnotation.parseArgs|


