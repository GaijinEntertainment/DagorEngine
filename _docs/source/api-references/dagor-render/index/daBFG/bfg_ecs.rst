..
  This is auto generated file. See daBfg/api/das/docs

.. _stdlib_bfg_ecs:

==============
Bfg in das+ecs
==============

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
++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-bfg_ecs-bfg_ecs_node:

.. das:attribute:: bfg_ecs_node

|function_annotation-bfg_ecs-bfg_ecs_node|

+++++++
Classes
+++++++

.. _struct-bfg_ecs-BfgEcsNodeAnnotation:

.. das:attribute:: BfgEcsNodeAnnotation : AstFunctionAnnotation

|class-bfg_ecs-BfgEcsNodeAnnotation|

.. das:function:: BfgEcsNodeAnnotation.apply(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

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


|method-bfg_ecs-BfgEcsNodeAnnotation.apply|

.. das:function:: BfgEcsNodeAnnotation.declareReloadCallback(self: BfgEcsNodeAnnotation; func: FunctionPtr; parsed: BfgEcsNodeAnnotationArgs const; args: AnnotationArgumentList const)

+--------+------------------------------------------------------------------------------------------+
+argument+argument type                                                                             +
+========+==========================================================================================+
+self    + :ref:`bfg_ecs::BfgEcsNodeAnnotation <struct-bfg_ecs-BfgEcsNodeAnnotation>`               +
+--------+------------------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                                   +
+--------+------------------------------------------------------------------------------------------+
+parsed  + :ref:`bfg_ecs::BfgEcsNodeAnnotationArgs <struct-bfg_ecs-BfgEcsNodeAnnotationArgs>`  const+
+--------+------------------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const          +
+--------+------------------------------------------------------------------------------------------+


|method-bfg_ecs-BfgEcsNodeAnnotation.declareReloadCallback|

.. das:function:: BfgEcsNodeAnnotation.declareES(self: BfgEcsNodeAnnotation; func: FunctionPtr; parsed: BfgEcsNodeAnnotationArgs const; args: AnnotationArgumentList const)

+--------+------------------------------------------------------------------------------------------+
+argument+argument type                                                                             +
+========+==========================================================================================+
+self    + :ref:`bfg_ecs::BfgEcsNodeAnnotation <struct-bfg_ecs-BfgEcsNodeAnnotation>`               +
+--------+------------------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                                   +
+--------+------------------------------------------------------------------------------------------+
+parsed  + :ref:`bfg_ecs::BfgEcsNodeAnnotationArgs <struct-bfg_ecs-BfgEcsNodeAnnotationArgs>`  const+
+--------+------------------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const          +
+--------+------------------------------------------------------------------------------------------+


|method-bfg_ecs-BfgEcsNodeAnnotation.declareES|

.. das:function:: BfgEcsNodeAnnotation.parseArgs(self: BfgEcsNodeAnnotation; func: FunctionPtr; args: AnnotationArgumentList const; errors: das_string)

parseArgs returns  :ref:`bfg_ecs::BfgEcsNodeAnnotationArgs <struct-bfg_ecs-BfgEcsNodeAnnotationArgs>` 

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`bfg_ecs::BfgEcsNodeAnnotation <struct-bfg_ecs-BfgEcsNodeAnnotation>`     +
+--------+--------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-bfg_ecs-BfgEcsNodeAnnotation.parseArgs|


