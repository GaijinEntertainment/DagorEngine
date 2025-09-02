..
  This is auto generated file. See daFrameGraph/api/das/docs

.. _stdlib_daFrameGraph:

=======================
daFrameGraph in Daslang
=======================

To use daFrameGraph inside Daslang you need first of all compile daFrameGraph
library with ``DAFG_ENABLE_DAS_INTERGRATION = yes``.

This will compile das module that you can import with ``require daFg.fg_ecs``
or ``require daFg``, whether you need ECS support or not.

Daslang daFg methods are very similar to C++ methods, so usage will be the same,
but with Daslang syntax instead.

:ref:`daframegraph::registerNode <function-_at_daFrameGraph_c__c_registerNode_S_ls_daFrameGraph_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_H_ls_daFgCore_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>` registers node with provided name and declaration callback.
Returns :ref:`NodeHandle <handle-daFgCore-NodeHandle>`.

Declaration callback is a Daslang lambda with one argument :ref:`Registry <handle-daFgCore-Registry>`.
It returns execute lambda.

Inside declaration callback you describe node using registry argument.

C++ declaration code

.. code-block:: cpp

    registry.orderMeAfter("some_node")
    registry.requestRenderPass().color("rt_tex")

Will be in Daslang:

.. code-block:: das

    registry |> orderMeAfter("some_node")
    registry |> requestRenderPass |> color([[auto "rt_tex"]])

**Example**:

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

++++++++++++
Type Aliases
++++++++++++

.. _alias-VrsRateTexture:

.. das:attribute:: VrsRateTexture is a variant type

+----+--------------------------------------------------------------------------------------------------+
+some+ :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>` +
+----+--------------------------------------------------------------------------------------------------+
+none+void?                                                                                             +
+----+--------------------------------------------------------------------------------------------------+


|typedef-daframegraph-VrsRateTexture|

.. _alias-VirtualAttachmentResource:

.. das:attribute:: VirtualAttachmentResource is a variant type

+------+----------------------------------------------------------+
+resUid+ :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>` +
+------+----------------------------------------------------------+
+name  +string                                                    +
+------+----------------------------------------------------------+


|typedef-daframegraph-VirtualAttachmentResource|

.. _alias-TextureResolution:

.. das:attribute:: TextureResolution is a variant type

+-------+----------------------------------------------------------------------------------------+
+res    +tuple<x:uint;y:uint>                                                                    +
+-------+----------------------------------------------------------------------------------------+
+autoRes+ :ref:`daframegraph::AutoResolutionRequest <struct-daframegraph-AutoResolutionRequest>` +
+-------+----------------------------------------------------------------------------------------+


|typedef-daframegraph-TextureResolution|

.. _struct-daframegraph-NameSpaceRequest:

.. das:attribute:: NameSpaceRequest



NameSpaceRequest fields are

+-----------+-------------------------------------------------------------------------+
+nameSpaceId+ :ref:`daFgCore::NameSpaceNameId <enum-daFgCore-NameSpaceNameId>`        +
+-----------+-------------------------------------------------------------------------+
+nodeId     + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+-----------+-------------------------------------------------------------------------+
+registry   + :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+-----------+-------------------------------------------------------------------------+


|structure-daframegraph-NameSpaceRequest|

.. _struct-daframegraph-Registry:

.. das:attribute:: Registry

 : NameSpaceRequest

Registry fields are

+-----------+-------------------------------------------------------------------------+
+nameSpaceId+ :ref:`daFgCore::NameSpaceNameId <enum-daFgCore-NameSpaceNameId>`        +
+-----------+-------------------------------------------------------------------------+
+nodeId     + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+-----------+-------------------------------------------------------------------------+
+registry   + :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+-----------+-------------------------------------------------------------------------+


|structure-daframegraph-Registry|

.. _struct-daframegraph-NameSpace:

.. das:attribute:: NameSpace



NameSpace fields are

+-----------+--------------------------------------------------------------------+
+nameSpaceId+ :ref:`daFgCore::NameSpaceNameId <enum-daFgCore-NameSpaceNameId>`   +
+-----------+--------------------------------------------------------------------+


|structure-daframegraph-NameSpace|

.. _struct-daframegraph-ResUid:

.. das:attribute:: ResUid



ResUid fields are

+-------+--------------------------------------------------------+
+nameId + :ref:`daFgCore::ResNameId <enum-daFgCore-ResNameId>`   +
+-------+--------------------------------------------------------+
+history+bool                                                    +
+-------+--------------------------------------------------------+


|structure-daframegraph-ResUid|

.. _struct-daframegraph-VirtualResourceRequestBase:

.. das:attribute:: VirtualResourceRequestBase



VirtualResourceRequestBase fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualResourceRequestBase|

.. _struct-daframegraph-VirtualResourceHandle:

.. das:attribute:: VirtualResourceHandle



VirtualResourceHandle fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualResourceHandle|

.. _struct-daframegraph-VirtualResourceCreationSemiRequest:

.. das:attribute:: VirtualResourceCreationSemiRequest

 : VirtualResourceRequestBase

VirtualResourceCreationSemiRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualResourceCreationSemiRequest|

.. _struct-daframegraph-VirtualResourceSemiRequest:

.. das:attribute:: VirtualResourceSemiRequest

 : VirtualResourceRequestBase

VirtualResourceSemiRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualResourceSemiRequest|

.. _struct-daframegraph-VirtualResourceRequest:

.. das:attribute:: VirtualResourceRequest

 : VirtualResourceRequestBase

VirtualResourceRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualResourceRequest|

.. _struct-daframegraph-VirtualTextureHandle:

.. das:attribute:: VirtualTextureHandle

 : VirtualResourceHandle

VirtualTextureHandle fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualTextureHandle|

.. _struct-daframegraph-VirtualBufferHandle:

.. das:attribute:: VirtualBufferHandle

 : VirtualResourceHandle

VirtualBufferHandle fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualBufferHandle|

.. _struct-daframegraph-VirtualTextureRequest:

.. das:attribute:: VirtualTextureRequest

 : VirtualResourceRequest

VirtualTextureRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualTextureRequest|

.. _struct-daframegraph-VirtualBufferRequest:

.. das:attribute:: VirtualBufferRequest

 : VirtualResourceRequest

VirtualBufferRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daframegraph::ResUid <struct-daframegraph-ResUid>`                +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualBufferRequest|

.. _struct-daframegraph-StateRequest:

.. das:attribute:: StateRequest



StateRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-StateRequest|

.. _struct-daframegraph-VrsRequirements:

.. das:attribute:: VrsRequirements



VrsRequirements fields are

+--------------+--------------------------------------------------------------------------------------------+
+rateX         +uint                                                                                        +
+--------------+--------------------------------------------------------------------------------------------+
+rateY         +uint                                                                                        +
+--------------+--------------------------------------------------------------------------------------------+
+rateTexture   + :ref:`VrsRateTexture <alias-VrsRateTexture>`                                               +
+--------------+--------------------------------------------------------------------------------------------+
+vertexCombiner+ :ref:`daFgCore::VariableRateShadingCombiner <enum-daFgCore-VariableRateShadingCombiner>`   +
+--------------+--------------------------------------------------------------------------------------------+
+pixelCombiner + :ref:`daFgCore::VariableRateShadingCombiner <enum-daFgCore-VariableRateShadingCombiner>`   +
+--------------+--------------------------------------------------------------------------------------------+


|structure-daframegraph-VrsRequirements|

.. _struct-daframegraph-VirtualPassRequest:

.. das:attribute:: VirtualPassRequest



VirtualPassRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`                  +
+--------+-------------------------------------------------------------------------+


|structure-daframegraph-VirtualPassRequest|

.. _struct-daframegraph-VirtualAttachmentRequest:

.. das:attribute:: VirtualAttachmentRequest



VirtualAttachmentRequest fields are

+--------+--------------------------------------------------------------------+
+resource+ :ref:`VirtualAttachmentResource <alias-VirtualAttachmentResource>` +
+--------+--------------------------------------------------------------------+
+mipLevel+uint                                                                +
+--------+--------------------------------------------------------------------+
+layer   +uint                                                                +
+--------+--------------------------------------------------------------------+


|structure-daframegraph-VirtualAttachmentRequest|

.. _struct-daframegraph-AutoResolutionRequest:

.. das:attribute:: AutoResolutionRequest



AutoResolutionRequest fields are

+-------------+-------------------------------------------------------------------------+
+autoResTypeId+ :ref:`daFgCore::AutoResTypeNameId <enum-daFgCore-AutoResTypeNameId>`    +
+-------------+-------------------------------------------------------------------------+
+multiplier   +float                                                                    +
+-------------+-------------------------------------------------------------------------+
+registry     + :ref:`daFgCore::InternalRegistry <handle-daFgCore-InternalRegistry>` ?  +
+-------------+-------------------------------------------------------------------------+


|structure-daframegraph-AutoResolutionRequest|

.. _struct-daframegraph-Texture2dCreateInfo:

.. das:attribute:: Texture2dCreateInfo



Texture2dCreateInfo fields are

+-------------+----------------------------------------------------+
+resolution   + :ref:`TextureResolution <alias-TextureResolution>` +
+-------------+----------------------------------------------------+
+creationFlags+uint                                                +
+-------------+----------------------------------------------------+
+mipLevels    +uint                                                +
+-------------+----------------------------------------------------+


|structure-daframegraph-Texture2dCreateInfo|

.. _struct-daframegraph-BufferCreateInfo:

.. das:attribute:: BufferCreateInfo



BufferCreateInfo fields are

+------------+----+
+elementSize +uint+
+------------+----+
+elementCount+uint+
+------------+----+
+flags       +uint+
+------------+----+
+format      +uint+
+------------+----+


|structure-daframegraph-BufferCreateInfo|

.. _struct-daframegraph-NamedSlot:

.. das:attribute:: NamedSlot



NamedSlot fields are

+----+------+
+name+string+
+----+------+


|structure-daframegraph-NamedSlot|

+++++++++++++++++++
Top Level Functions
+++++++++++++++++++

  *  :ref:`root () : daFrameGraph::NameSpace <function-_at_daFrameGraph_c__c_root>`
  *  :ref:`/ (self:daFrameGraph::NameSpace -const;child_name:string const) : daFrameGraph::NameSpace <function-_at_daFrameGraph_c__c_/_S_ls_daFrameGraph_c__c_NameSpace_gr__Cs>`
  *  :ref:`fillSlot (self:daFrameGraph::NameSpace -const;slot:daFrameGraph::NamedSlot const;res_name_space:daFrameGraph::NameSpace const;res_name:string const) : void <function-_at_daFrameGraph_c__c_fillSlot_S_ls_daFrameGraph_c__c_NameSpace_gr__CS_ls_daFrameGraph_c__c_NamedSlot_gr__CS_ls_daFrameGraph_c__c_NameSpace_gr__Cs>`
  *  :ref:`registerNode (self:daFrameGraph::NameSpace -const;name:string const;declaration_callback:lambda\<(var reg:daFrameGraph::Registry -const):lambda\<void\>\> -const) : daFgCore::NodeHandle <function-_at_daFrameGraph_c__c_registerNode_S_ls_daFrameGraph_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_S_ls_daFrameGraph_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>`
  *  :ref:`root (self:daFrameGraph::Registry -const) : daFrameGraph::NameSpaceRequest <function-_at_daFrameGraph_c__c_root_S_ls_daFrameGraph_c__c_Registry_gr_>`
  *  :ref:`/ (self:daFrameGraph::NameSpaceRequest -const;child_name:string const) : daFrameGraph::NameSpaceRequest <function-_at_daFrameGraph_c__c_/_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs>`

.. _function-_at_daFrameGraph_c__c_root:

.. das:function:: root()

root returns  :ref:`daframegraph::NameSpace <struct-daframegraph-NameSpace>`

|function-daframegraph-root|

.. _function-_at_daFrameGraph_c__c_/_S_ls_daFrameGraph_c__c_NameSpace_gr__Cs:

.. das:function:: operator /(self: NameSpace; child_name: string const)

/ returns  :ref:`daframegraph::NameSpace <struct-daframegraph-NameSpace>`

+----------+----------------------------------------------------------------+
+argument  +argument type                                                   +
+==========+================================================================+
+self      + :ref:`daframegraph::NameSpace <struct-daframegraph-NameSpace>` +
+----------+----------------------------------------------------------------+
+child_name+string const                                                    +
+----------+----------------------------------------------------------------+


|function-daframegraph-/|

.. _function-_at_daFrameGraph_c__c_fillSlot_S_ls_daFrameGraph_c__c_NameSpace_gr__CS_ls_daFrameGraph_c__c_NamedSlot_gr__CS_ls_daFrameGraph_c__c_NameSpace_gr__Cs:

.. das:function:: fillSlot(self: NameSpace; slot: NamedSlot const; res_name_space: NameSpace const; res_name: string const)

+--------------+----------------------------------------------------------------------+
+argument      +argument type                                                         +
+==============+======================================================================+
+self          + :ref:`daframegraph::NameSpace <struct-daframegraph-NameSpace>`       +
+--------------+----------------------------------------------------------------------+
+slot          + :ref:`daframegraph::NamedSlot <struct-daframegraph-NamedSlot>`  const+
+--------------+----------------------------------------------------------------------+
+res_name_space+ :ref:`daframegraph::NameSpace <struct-daframegraph-NameSpace>`  const+
+--------------+----------------------------------------------------------------------+
+res_name      +string const                                                          +
+--------------+----------------------------------------------------------------------+


|function-daframegraph-fillSlot|

.. _function-_at_daFrameGraph_c__c_registerNode_S_ls_daFrameGraph_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_S_ls_daFrameGraph_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_:

.. das:function:: registerNode(self: NameSpace; name: string const; declaration_callback: lambda<(var reg:daFrameGraph::Registry -const):lambda<void>>)

registerNode returns  :ref:`daFgCore::NodeHandle <handle-daFgCore-NodeHandle>`

+--------------------+--------------------------------------------------------------------------------------+
+argument            +argument type                                                                         +
+====================+======================================================================================+
+self                + :ref:`daframegraph::NameSpace <struct-daframegraph-NameSpace>`                       +
+--------------------+--------------------------------------------------------------------------------------+
+name                +string const                                                                          +
+--------------------+--------------------------------------------------------------------------------------+
+declaration_callback+lambda<(reg: :ref:`daframegraph::Registry <struct-daframegraph-Registry>` ):lambda<>> +
+--------------------+--------------------------------------------------------------------------------------+


|function-daframegraph-registerNode|

.. _function-_at_daFrameGraph_c__c_root_S_ls_daFrameGraph_c__c_Registry_gr_:

.. das:function:: root(self: Registry)

root returns  :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>` +
+--------+--------------------------------------------------------------+


|function-daframegraph-root|

.. _function-_at_daFrameGraph_c__c_/_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: operator /(self: NameSpaceRequest; child_name: string const)

/ returns  :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>`

+----------+------------------------------------------------------------------------------+
+argument  +argument type                                                                 +
+==========+==============================================================================+
+self      + :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>` +
+----------+------------------------------------------------------------------------------+
+child_name+string const                                                                  +
+----------+------------------------------------------------------------------------------+


|function-daframegraph-/|

+++++++++++++++++++++
Registry Manipulation
+++++++++++++++++++++

  *  :ref:`orderMeBefore (self:daFrameGraph::Registry -const;name:string const) : daFrameGraph::Registry <function-_at_daFrameGraph_c__c_orderMeBefore_S_ls_daFrameGraph_c__c_Registry_gr__Cs>`
  *  :ref:`orderMeBefore (self:daFrameGraph::Registry -const;names:array\<string\> const) : daFrameGraph::Registry <function-_at_daFrameGraph_c__c_orderMeBefore_S_ls_daFrameGraph_c__c_Registry_gr__C1_ls_s_gr_A>`
  *  :ref:`orderMeAfter (self:daFrameGraph::Registry -const;name:string const) : daFrameGraph::Registry <function-_at_daFrameGraph_c__c_orderMeAfter_S_ls_daFrameGraph_c__c_Registry_gr__Cs>`
  *  :ref:`orderMeAfter (self:daFrameGraph::Registry -const;names:array\<string\> const) : daFrameGraph::Registry <function-_at_daFrameGraph_c__c_orderMeAfter_S_ls_daFrameGraph_c__c_Registry_gr__C1_ls_s_gr_A>`
  *  :ref:`setPriority (self:daFrameGraph::Registry -const;priority:int const) : daFrameGraph::Registry <function-_at_daFrameGraph_c__c_setPriority_S_ls_daFrameGraph_c__c_Registry_gr__Ci>`
  *  :ref:`multiplex (self:daFrameGraph::Registry -const;multiplexing_mode:daFgCore::MultiplexingMode const) : daFrameGraph::Registry <function-_at_daFrameGraph_c__c_multiplex_S_ls_daFrameGraph_c__c_Registry_gr__CE_ls_daFgCore_c__c_MultiplexingMode_gr_>`
  *  :ref:`executionHas (self:daFrameGraph::Registry -const;side_effect:daFgCore::SideEffect const) : daFrameGraph::Registry <function-_at_daFrameGraph_c__c_executionHas_S_ls_daFrameGraph_c__c_Registry_gr__CE8_ls_daFgCore_c__c_SideEffect_gr_>`
  *  :ref:`create (self:daFrameGraph::Registry -const;name:string const;history:daFgCore::History const) : daFrameGraph::VirtualResourceCreationSemiRequest <function-_at_daFrameGraph_c__c_create_S_ls_daFrameGraph_c__c_Registry_gr__Cs_CE8_ls_daFgCore_c__c_History_gr_>`
  *  :ref:`getResolution (self:daFrameGraph::NameSpaceRequest -const;type_name:string const;multiplier:float const) : daFrameGraph::AutoResolutionRequest <function-_at_daFrameGraph_c__c_getResolution_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs_Cf>`
  *  :ref:`read (self:daFrameGraph::NameSpaceRequest -const;name:string const) : daFrameGraph::VirtualResourceSemiRequest <function-_at_daFrameGraph_c__c_read_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs>`
  *  :ref:`read (self:daFrameGraph::NameSpaceRequest -const;slot:daFrameGraph::NamedSlot const) : daFrameGraph::VirtualResourceSemiRequest <function-_at_daFrameGraph_c__c_read_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__CS_ls_daFrameGraph_c__c_NamedSlot_gr_>`
  *  :ref:`history (self:daFrameGraph::NameSpaceRequest -const;name:string -const) : daFrameGraph::VirtualResourceSemiRequest <function-_at_daFrameGraph_c__c_history_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__s>`
  *  :ref:`modify (self:daFrameGraph::NameSpaceRequest -const;name:string const) : daFrameGraph::VirtualResourceSemiRequest <function-_at_daFrameGraph_c__c_modify_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs>`
  *  :ref:`modify (self:daFrameGraph::NameSpaceRequest -const;slot:daFrameGraph::NamedSlot const) : daFrameGraph::VirtualResourceSemiRequest <function-_at_daFrameGraph_c__c_modify_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__CS_ls_daFrameGraph_c__c_NamedSlot_gr_>`
  *  :ref:`rename (self:daFrameGraph::NameSpaceRequest -const;from:string const;to:string const;history:daFgCore::History const) : daFrameGraph::VirtualResourceSemiRequest <function-_at_daFrameGraph_c__c_rename_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs_Cs_CE8_ls_daFgCore_c__c_History_gr_>`
  *  :ref:`get (resolution:daFrameGraph::AutoResolutionRequest const) : int2 <function-_at_daFrameGraph_c__c_get_CS_ls_daFrameGraph_c__c_AutoResolutionRequest_gr_>`
  *  :ref:`requestState (self:daFrameGraph::Registry -const) : daFrameGraph::StateRequest <function-_at_daFrameGraph_c__c_requestState_S_ls_daFrameGraph_c__c_Registry_gr_>`
  *  :ref:`requestRenderPass (self:daFrameGraph::Registry -const) : daFrameGraph::VirtualPassRequest <function-_at_daFrameGraph_c__c_requestRenderPass_S_ls_daFrameGraph_c__c_Registry_gr_>`

.. _function-_at_daFrameGraph_c__c_orderMeBefore_S_ls_daFrameGraph_c__c_Registry_gr__Cs:

.. das:function:: orderMeBefore(self: Registry; name: string const)

orderMeBefore returns  :ref:`daframegraph::Registry <struct-daframegraph-Registry>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>` +
+--------+--------------------------------------------------------------+
+name    +string const                                                  +
+--------+--------------------------------------------------------------+


|function-daframegraph-orderMeBefore|

.. _function-_at_daFrameGraph_c__c_orderMeBefore_S_ls_daFrameGraph_c__c_Registry_gr__C1_ls_s_gr_A:

.. das:function:: orderMeBefore(self: Registry; names: array<string> const)

orderMeBefore returns  :ref:`daframegraph::Registry <struct-daframegraph-Registry>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>` +
+--------+--------------------------------------------------------------+
+names   +array<string> const                                           +
+--------+--------------------------------------------------------------+


|function-daframegraph-orderMeBefore|

.. _function-_at_daFrameGraph_c__c_orderMeAfter_S_ls_daFrameGraph_c__c_Registry_gr__Cs:

.. das:function:: orderMeAfter(self: Registry; name: string const)

orderMeAfter returns  :ref:`daframegraph::Registry <struct-daframegraph-Registry>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>` +
+--------+--------------------------------------------------------------+
+name    +string const                                                  +
+--------+--------------------------------------------------------------+


|function-daframegraph-orderMeAfter|

.. _function-_at_daFrameGraph_c__c_orderMeAfter_S_ls_daFrameGraph_c__c_Registry_gr__C1_ls_s_gr_A:

.. das:function:: orderMeAfter(self: Registry; names: array<string> const)

orderMeAfter returns  :ref:`daframegraph::Registry <struct-daframegraph-Registry>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>` +
+--------+--------------------------------------------------------------+
+names   +array<string> const                                           +
+--------+--------------------------------------------------------------+


|function-daframegraph-orderMeAfter|

.. _function-_at_daFrameGraph_c__c_setPriority_S_ls_daFrameGraph_c__c_Registry_gr__Ci:

.. das:function:: setPriority(self: Registry; priority: int const)

setPriority returns  :ref:`daframegraph::Registry <struct-daframegraph-Registry>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>` +
+--------+--------------------------------------------------------------+
+priority+int const                                                     +
+--------+--------------------------------------------------------------+


|function-daframegraph-setPriority|

.. _function-_at_daFrameGraph_c__c_multiplex_S_ls_daFrameGraph_c__c_Registry_gr__CE_ls_daFgCore_c__c_MultiplexingMode_gr_:

.. das:function:: multiplex(self: Registry; multiplexing_mode: MultiplexingMode const)

multiplex returns  :ref:`daframegraph::Registry <struct-daframegraph-Registry>`

+-----------------+----------------------------------------------------------------------------+
+argument         +argument type                                                               +
+=================+============================================================================+
+self             + :ref:`daframegraph::Registry <struct-daframegraph-Registry>`               +
+-----------------+----------------------------------------------------------------------------+
+multiplexing_mode+ :ref:`daFgCore::MultiplexingMode <enum-daFgCore-MultiplexingMode>`  const  +
+-----------------+----------------------------------------------------------------------------+


|function-daframegraph-multiplex|

.. _function-_at_daFrameGraph_c__c_executionHas_S_ls_daFrameGraph_c__c_Registry_gr__CE8_ls_daFgCore_c__c_SideEffect_gr_:

.. das:function:: executionHas(self: Registry; side_effect: SideEffect const)

executionHas returns  :ref:`daframegraph::Registry <struct-daframegraph-Registry>`

+-----------+----------------------------------------------------------------+
+argument   +argument type                                                   +
+===========+================================================================+
+self       + :ref:`daframegraph::Registry <struct-daframegraph-Registry>`   +
+-----------+----------------------------------------------------------------+
+side_effect+ :ref:`daFgCore::SideEffect <enum-daFgCore-SideEffect>`  const  +
+-----------+----------------------------------------------------------------+


|function-daframegraph-executionHas|

.. _function-_at_daFrameGraph_c__c_create_S_ls_daFrameGraph_c__c_Registry_gr__Cs_CE8_ls_daFgCore_c__c_History_gr_:

.. das:function:: create(self: Registry; name: string const; history: History const)

create returns  :ref:`daframegraph::VirtualResourceCreationSemiRequest <struct-daframegraph-VirtualResourceCreationSemiRequest>`

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>`           +
+--------+------------------------------------------------------------------------+
+name    +string const                                                            +
+--------+------------------------------------------------------------------------+
+history + :ref:`daFgCore::History <enum-daFgCore-History>`  const                +
+--------+------------------------------------------------------------------------+


|function-daframegraph-create|

.. _function-_at_daFrameGraph_c__c_getResolution_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs_Cf:

.. das:function:: getResolution(self: NameSpaceRequest; type_name: string const; multiplier: float const)

getResolution returns  :ref:`daframegraph::AutoResolutionRequest <struct-daframegraph-AutoResolutionRequest>`

+----------+------------------------------------------------------------------------------+
+argument  +argument type                                                                 +
+==========+==============================================================================+
+self      + :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>` +
+----------+------------------------------------------------------------------------------+
+type_name +string const                                                                  +
+----------+------------------------------------------------------------------------------+
+multiplier+float const                                                                   +
+----------+------------------------------------------------------------------------------+


|function-daframegraph-getResolution|

.. _function-_at_daFrameGraph_c__c_read_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: read(self: NameSpaceRequest; name: string const)

read returns  :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>`

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+self    + :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>` +
+--------+------------------------------------------------------------------------------+
+name    +string const                                                                  +
+--------+------------------------------------------------------------------------------+


|function-daframegraph-read|

.. _function-_at_daFrameGraph_c__c_read_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__CS_ls_daFrameGraph_c__c_NamedSlot_gr_:

.. das:function:: read(self: NameSpaceRequest; slot: NamedSlot const)

read returns  :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>`

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+self    + :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>` +
+--------+------------------------------------------------------------------------------+
+slot    + :ref:`daframegraph::NamedSlot <struct-daframegraph-NamedSlot>`  const        +
+--------+------------------------------------------------------------------------------+


|function-daframegraph-read|

.. _function-_at_daFrameGraph_c__c_history_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__s:

.. das:function:: history(self: NameSpaceRequest; name: string)

history returns  :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>`

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+self    + :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>` +
+--------+------------------------------------------------------------------------------+
+name    +string                                                                        +
+--------+------------------------------------------------------------------------------+


|function-daframegraph-history|

.. _function-_at_daFrameGraph_c__c_modify_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: modify(self: NameSpaceRequest; name: string const)

modify returns  :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>`

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+self    + :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>` +
+--------+------------------------------------------------------------------------------+
+name    +string const                                                                  +
+--------+------------------------------------------------------------------------------+


|function-daframegraph-modify|

.. _function-_at_daFrameGraph_c__c_modify_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__CS_ls_daFrameGraph_c__c_NamedSlot_gr_:

.. das:function:: modify(self: NameSpaceRequest; slot: NamedSlot const)

modify returns  :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>`

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+self    + :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>` +
+--------+------------------------------------------------------------------------------+
+slot    + :ref:`daframegraph::NamedSlot <struct-daframegraph-NamedSlot>`  const        +
+--------+------------------------------------------------------------------------------+


|function-daframegraph-modify|

.. _function-_at_daFrameGraph_c__c_rename_S_ls_daFrameGraph_c__c_NameSpaceRequest_gr__Cs_Cs_CE8_ls_daFgCore_c__c_History_gr_:

.. das:function:: rename(self: NameSpaceRequest; from: string const; to: string const; history: History const)

rename returns  :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>`

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+self    + :ref:`daframegraph::NameSpaceRequest <struct-daframegraph-NameSpaceRequest>` +
+--------+------------------------------------------------------------------------------+
+from    +string const                                                                  +
+--------+------------------------------------------------------------------------------+
+to      +string const                                                                  +
+--------+------------------------------------------------------------------------------+
+history + :ref:`daFgCore::History <enum-daFgCore-History>`  const                      +
+--------+------------------------------------------------------------------------------+


|function-daframegraph-rename|

.. _function-_at_daFrameGraph_c__c_get_CS_ls_daFrameGraph_c__c_AutoResolutionRequest_gr_:

.. das:function:: get(resolution: AutoResolutionRequest const)

get returns int2

+----------+----------------------------------------------------------------------------------------------+
+argument  +argument type                                                                                 +
+==========+==============================================================================================+
+resolution+ :ref:`daframegraph::AutoResolutionRequest <struct-daframegraph-AutoResolutionRequest>`  const+
+----------+----------------------------------------------------------------------------------------------+


|function-daframegraph-get|

.. _function-_at_daFrameGraph_c__c_requestState_S_ls_daFrameGraph_c__c_Registry_gr_:

.. das:function:: requestState(self: Registry)

requestState returns  :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>` +
+--------+--------------------------------------------------------------+


|function-daframegraph-requestState|

.. _function-_at_daFrameGraph_c__c_requestRenderPass_S_ls_daFrameGraph_c__c_Registry_gr_:

.. das:function:: requestRenderPass(self: Registry)

requestRenderPass returns  :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`daframegraph::Registry <struct-daframegraph-Registry>` +
+--------+--------------------------------------------------------------+


|function-daframegraph-requestRenderPass|

++++++++++++++++++++
Request Manipulation
++++++++++++++++++++

  *  :ref:`texture (self:daFrameGraph::VirtualResourceCreationSemiRequest -const;info:daFrameGraph::Texture2dCreateInfo const) : daFrameGraph::VirtualTextureRequest <function-_at_daFrameGraph_c__c_texture_S_ls_daFrameGraph_c__c_VirtualResourceCreationSemiRequest_gr__CS_ls_daFrameGraph_c__c_Texture2dCreateInfo_gr_>`
  *  :ref:`texture (self:daFrameGraph::VirtualResourceSemiRequest -const) : daFrameGraph::VirtualTextureRequest <function-_at_daFrameGraph_c__c_texture_S_ls_daFrameGraph_c__c_VirtualResourceSemiRequest_gr_>`
  *  :ref:`buffer (self:daFrameGraph::VirtualResourceCreationSemiRequest -const;info:daFrameGraph::BufferCreateInfo const) : daFrameGraph::VirtualBufferRequest <function-_at_daFrameGraph_c__c_buffer_S_ls_daFrameGraph_c__c_VirtualResourceCreationSemiRequest_gr__CS_ls_daFrameGraph_c__c_BufferCreateInfo_gr_>`
  *  :ref:`buffer (self:daFrameGraph::VirtualResourceSemiRequest -const) : daFrameGraph::VirtualBufferRequest <function-_at_daFrameGraph_c__c_buffer_S_ls_daFrameGraph_c__c_VirtualResourceSemiRequest_gr_>`
  *  :ref:`blob (self:daFrameGraph::VirtualResourceSemiRequest -const) : daFrameGraph::VirtualResourceRequest <function-_at_daFrameGraph_c__c_blob_S_ls_daFrameGraph_c__c_VirtualResourceSemiRequest_gr_>`
  *  :ref:`modifyRequest (self:daFrameGraph::VirtualResourceRequest -const;modifier:block\<(var request:daFgCore::ResourceRequest -const):void\> const) : void <function-_at_daFrameGraph_c__c_modifyRequest_S_ls_daFrameGraph_c__c_VirtualResourceRequest_gr__CN_ls_request_gr_0_ls_H_ls_daFgCore_c__c_ResourceRequest_gr__gr_1_ls_v_gr__builtin_>`
  *  :ref:`handle (self:daFrameGraph::VirtualTextureRequest const) : daFrameGraph::VirtualTextureHandle <function-_at_daFrameGraph_c__c_handle_CS_ls_daFrameGraph_c__c_VirtualTextureRequest_gr_>`
  *  :ref:`handle (self:daFrameGraph::VirtualBufferRequest const) : daFrameGraph::VirtualBufferHandle <function-_at_daFrameGraph_c__c_handle_CS_ls_daFrameGraph_c__c_VirtualBufferRequest_gr_>`
  *  :ref:`view (handle:daFrameGraph::VirtualTextureHandle const) : DagorResPtr::ManagedTexView <function-_at_daFrameGraph_c__c_view_CS_ls_daFrameGraph_c__c_VirtualTextureHandle_gr_>`
  *  :ref:`view (handle:daFrameGraph::VirtualBufferHandle const) : DagorResPtr::ManagedBufView <function-_at_daFrameGraph_c__c_view_CS_ls_daFrameGraph_c__c_VirtualBufferHandle_gr_>`
  *  :ref:`setFrameBlock (self:daFrameGraph::StateRequest -const;name:string const) : daFrameGraph::StateRequest <function-_at_daFrameGraph_c__c_setFrameBlock_S_ls_daFrameGraph_c__c_StateRequest_gr__Cs>`
  *  :ref:`setSceneBlock (self:daFrameGraph::StateRequest -const;name:string const) : daFrameGraph::StateRequest <function-_at_daFrameGraph_c__c_setSceneBlock_S_ls_daFrameGraph_c__c_StateRequest_gr__Cs>`
  *  :ref:`setObjectBlock (self:daFrameGraph::StateRequest -const;name:string const) : daFrameGraph::StateRequest <function-_at_daFrameGraph_c__c_setObjectBlock_S_ls_daFrameGraph_c__c_StateRequest_gr__Cs>`
  *  :ref:`allowWireFrame (self:daFrameGraph::StateRequest -const) : daFrameGraph::StateRequest <function-_at_daFrameGraph_c__c_allowWireFrame_S_ls_daFrameGraph_c__c_StateRequest_gr_>`
  *  :ref:`allowVrs (self:daFrameGraph::StateRequest -const;vrs:daFrameGraph::VrsRequirements const) : daFrameGraph::StateRequest <function-_at_daFrameGraph_c__c_allowVrs_S_ls_daFrameGraph_c__c_StateRequest_gr__CS_ls_daFrameGraph_c__c_VrsRequirements_gr_>`
  *  :ref:`enableOverride (self:daFrameGraph::StateRequest -const;das_override:DagorDriver3D::OverrideRenderState const) : daFrameGraph::StateRequest <function-_at_daFrameGraph_c__c_enableOverride_S_ls_daFrameGraph_c__c_StateRequest_gr__CS_ls_DagorDriver3D_c__c_OverrideRenderState_gr_>`
  *  :ref:`color (self:daFrameGraph::VirtualPassRequest -const;attachments:array\<daFrameGraph::VirtualAttachmentRequest\> const) : daFrameGraph::VirtualPassRequest <function-_at_daFrameGraph_c__c_color_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C1_ls_S_ls_daFrameGraph_c__c_VirtualAttachmentRequest_gr__gr_A>`
  *  :ref:`optional (self:auto(TT) -const) : TT <function-_at_daFrameGraph_c__c_optional_Y_ls_TT_gr_.>`
  *  :ref:`useAs (self:auto(TT) -const;usageType:daFgCore::Usage const) : TT <function-_at_daFrameGraph_c__c_useAs_Y_ls_TT_gr_._CE8_ls_daFgCore_c__c_Usage_gr_>`
  *  :ref:`atStage (self:auto(TT) -const;stage:daFgCore::Stage const) : TT <function-_at_daFrameGraph_c__c_atStage_Y_ls_TT_gr_._CE8_ls_daFgCore_c__c_Stage_gr_>`
  *  :ref:`bindToShaderVar (self:auto(TT) -const;name:string const) : TT <function-_at_daFrameGraph_c__c_bindToShaderVar_Y_ls_TT_gr_._Cs>`
  *  :ref:`color (self:daFrameGraph::VirtualPassRequest -const;requests:daFrameGraph::VirtualTextureRequest const[]) : daFrameGraph::VirtualPassRequest <function-_at_daFrameGraph_c__c_color_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C[-1]S_ls_daFrameGraph_c__c_VirtualTextureRequest_gr_>`
  *  :ref:`color (self:daFrameGraph::VirtualPassRequest -const;names:string const[]) : daFrameGraph::VirtualPassRequest <function-_at_daFrameGraph_c__c_color_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C[-1]s>`
  *  :ref:`depthRw (self:daFrameGraph::VirtualPassRequest -const;attachment:auto const) : daFrameGraph::VirtualPassRequest <function-_at_daFrameGraph_c__c_depthRw_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C.>`
  *  :ref:`depthRo (self:daFrameGraph::VirtualPassRequest -const;attachment:auto const) : daFrameGraph::VirtualPassRequest <function-_at_daFrameGraph_c__c_depthRo_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C.>`

.. _function-_at_daFrameGraph_c__c_texture_S_ls_daFrameGraph_c__c_VirtualResourceCreationSemiRequest_gr__CS_ls_daFrameGraph_c__c_Texture2dCreateInfo_gr_:

.. das:function:: texture(self: VirtualResourceCreationSemiRequest; info: Texture2dCreateInfo const)

texture returns  :ref:`daframegraph::VirtualTextureRequest <struct-daframegraph-VirtualTextureRequest>`

+--------+------------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                     +
+========+==================================================================================================================+
+self    + :ref:`daframegraph::VirtualResourceCreationSemiRequest <struct-daframegraph-VirtualResourceCreationSemiRequest>` +
+--------+------------------------------------------------------------------------------------------------------------------+
+info    + :ref:`daframegraph::Texture2dCreateInfo <struct-daframegraph-Texture2dCreateInfo>`  const                        +
+--------+------------------------------------------------------------------------------------------------------------------+


|function-daframegraph-texture|

.. _function-_at_daFrameGraph_c__c_texture_S_ls_daFrameGraph_c__c_VirtualResourceSemiRequest_gr_:

.. das:function:: texture(self: VirtualResourceSemiRequest)

texture returns  :ref:`daframegraph::VirtualTextureRequest <struct-daframegraph-VirtualTextureRequest>`

+--------+--------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                     +
+========+==================================================================================================+
+self    + :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>` +
+--------+--------------------------------------------------------------------------------------------------+


|function-daframegraph-texture|

.. _function-_at_daFrameGraph_c__c_buffer_S_ls_daFrameGraph_c__c_VirtualResourceCreationSemiRequest_gr__CS_ls_daFrameGraph_c__c_BufferCreateInfo_gr_:

.. das:function:: buffer(self: VirtualResourceCreationSemiRequest; info: BufferCreateInfo const)

buffer returns  :ref:`daframegraph::VirtualBufferRequest <struct-daframegraph-VirtualBufferRequest>`

+--------+-----------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                    +
+========+=================================================================================================================+
+self    + :ref:`daframegraph::VirtualResourceCreationSemiRequest <struct-daframegraph-VirtualResourceCreationSemiRequest>`+
+--------+-----------------------------------------------------------------------------------------------------------------+
+info    + :ref:`daframegraph::BufferCreateInfo <struct-daframegraph-BufferCreateInfo>` const                              +
+--------+-----------------------------------------------------------------------------------------------------------------+


|function-daframegraph-buffer|

.. _function-_at_daFrameGraph_c__c_buffer_S_ls_daFrameGraph_c__c_VirtualResourceSemiRequest_gr_:

.. das:function:: buffer(self: VirtualResourceSemiRequest)

buffer returns  :ref:`daframegraph::VirtualBufferRequest <struct-daframegraph-VirtualBufferRequest>`

+--------+--------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                     +
+========+==================================================================================================+
+self    + :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>` +
+--------+--------------------------------------------------------------------------------------------------+


|function-daframegraph-buffer|

.. _function-_at_daFrameGraph_c__c_blob_S_ls_daFrameGraph_c__c_VirtualResourceSemiRequest_gr_:

.. das:function:: blob(self: VirtualResourceSemiRequest)

blob returns  :ref:`daframegraph::VirtualResourceRequest <struct-daframegraph-VirtualResourceRequest>`

+--------+--------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                     +
+========+==================================================================================================+
+self    + :ref:`daframegraph::VirtualResourceSemiRequest <struct-daframegraph-VirtualResourceSemiRequest>` +
+--------+--------------------------------------------------------------------------------------------------+


|function-daframegraph-blob|

.. _function-_at_daFrameGraph_c__c_modifyRequest_S_ls_daFrameGraph_c__c_VirtualResourceRequest_gr__CN_ls_request_gr_0_ls_H_ls_daFgCore_c__c_ResourceRequest_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: modifyRequest(self: VirtualResourceRequest; modifier: block<(var request:daFgCore::ResourceRequest -const):void> const)

+--------+--------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                     +
+========+==================================================================================================+
+self    + :ref:`daframegraph::VirtualResourceRequest <struct-daframegraph-VirtualResourceRequest>`         +
+--------+--------------------------------------------------------------------------------------------------+
+modifier+block<(request: :ref:`daFgCore::ResourceRequest <handle-daFgCore-ResourceRequest>` ):void> const  +
+--------+--------------------------------------------------------------------------------------------------+


|function-daframegraph-modifyRequest|

.. _function-_at_daFrameGraph_c__c_handle_CS_ls_daFrameGraph_c__c_VirtualTextureRequest_gr_:

.. das:function:: handle(self: VirtualTextureRequest const)

handle returns  :ref:`daframegraph::VirtualTextureHandle <struct-daframegraph-VirtualTextureHandle>`

+--------+----------------------------------------------------------------------------------------------+
+argument+argument type                                                                                 +
+========+==============================================================================================+
+self    + :ref:`daframegraph::VirtualTextureRequest <struct-daframegraph-VirtualTextureRequest>`  const+
+--------+----------------------------------------------------------------------------------------------+


|function-daframegraph-handle|

.. _function-_at_daFrameGraph_c__c_handle_CS_ls_daFrameGraph_c__c_VirtualBufferRequest_gr_:

.. das:function:: handle(self: VirtualBufferRequest const)

handle returns  :ref:`daframegraph::VirtualBufferHandle <struct-daframegraph-VirtualBufferHandle>`

+--------+--------------------------------------------------------------------------------------------+
+argument+argument type                                                                               +
+========+============================================================================================+
+self    + :ref:`daframegraph::VirtualBufferRequest <struct-daframegraph-VirtualBufferRequest>`  const+
+--------+--------------------------------------------------------------------------------------------+


|function-daframegraph-handle|

.. _function-_at_daFrameGraph_c__c_view_CS_ls_daFrameGraph_c__c_VirtualTextureHandle_gr_:

.. das:function:: view(handle: VirtualTextureHandle const)

view returns  :ref:`DagorResPtr::ManagedTexView <handle-DagorResPtr-ManagedTexView>`

+--------+--------------------------------------------------------------------------------------------+
+argument+argument type                                                                               +
+========+============================================================================================+
+handle  + :ref:`daframegraph::VirtualTextureHandle <struct-daframegraph-VirtualTextureHandle>`  const+
+--------+--------------------------------------------------------------------------------------------+


|function-daframegraph-view|

.. _function-_at_daFrameGraph_c__c_view_CS_ls_daFrameGraph_c__c_VirtualBufferHandle_gr_:

.. das:function:: view(handle: VirtualBufferHandle const)

view returns  :ref:`DagorResPtr::ManagedBufView <handle-DagorResPtr-ManagedBufView>`

+--------+------------------------------------------------------------------------------------------+
+argument+argument type                                                                             +
+========+==========================================================================================+
+handle  + :ref:`daframegraph::VirtualBufferHandle <struct-daframegraph-VirtualBufferHandle>`  const+
+--------+------------------------------------------------------------------------------------------+


|function-daframegraph-view|

.. _function-_at_daFrameGraph_c__c_setFrameBlock_S_ls_daFrameGraph_c__c_StateRequest_gr__Cs:

.. das:function:: setFrameBlock(self: StateRequest; name: string const)

setFrameBlock returns  :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+self    + :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>` +
+--------+----------------------------------------------------------------------+
+name    +string const                                                          +
+--------+----------------------------------------------------------------------+


|function-daframegraph-setFrameBlock|

.. _function-_at_daFrameGraph_c__c_setSceneBlock_S_ls_daFrameGraph_c__c_StateRequest_gr__Cs:

.. das:function:: setSceneBlock(self: StateRequest; name: string const)

setSceneBlock returns  :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+self    + :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>` +
+--------+----------------------------------------------------------------------+
+name    +string const                                                          +
+--------+----------------------------------------------------------------------+


|function-daframegraph-setSceneBlock|

.. _function-_at_daFrameGraph_c__c_setObjectBlock_S_ls_daFrameGraph_c__c_StateRequest_gr__Cs:

.. das:function:: setObjectBlock(self: StateRequest; name: string const)

setObjectBlock returns  :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+self    + :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>` +
+--------+----------------------------------------------------------------------+
+name    + string const                                                         +
+--------+----------------------------------------------------------------------+


|function-daframegraph-setObjectBlock|

.. _function-_at_daFrameGraph_c__c_allowWireFrame_S_ls_daFrameGraph_c__c_StateRequest_gr_:

.. das:function:: allowWireFrame(self: StateRequest)

allowWireFrame returns  :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+self    + :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>` +
+--------+----------------------------------------------------------------------+


|function-daframegraph-allowWireFrame|

.. _function-_at_daFrameGraph_c__c_allowVrs_S_ls_daFrameGraph_c__c_StateRequest_gr__CS_ls_daFrameGraph_c__c_VrsRequirements_gr_:

.. das:function:: allowVrs(self: StateRequest; vrs: VrsRequirements const)

allowVrs returns  :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`

+--------+----------------------------------------------------------------------------------+
+argument+argument type                                                                     +
+========+==================================================================================+
+self    + :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`             +
+--------+----------------------------------------------------------------------------------+
+vrs     + :ref:`daframegraph::VrsRequirements <struct-daframegraph-VrsRequirements>`  const+
+--------+----------------------------------------------------------------------------------+


|function-daframegraph-allowVrs|

.. _function-_at_daFrameGraph_c__c_enableOverride_S_ls_daFrameGraph_c__c_StateRequest_gr__CS_ls_DagorDriver3D_c__c_OverrideRenderState_gr_:

.. das:function:: enableOverride(self: StateRequest; das_override: OverrideRenderState const)

enableOverride returns  :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`

+------------+--------------------------------------------------------------------------------------------+
+argument    +argument type                                                                               +
+============+============================================================================================+
+self        + :ref:`daframegraph::StateRequest <struct-daframegraph-StateRequest>`                       +
+------------+--------------------------------------------------------------------------------------------+
+das_override+ :ref:`DagorDriver3D::OverrideRenderState <struct-DagorDriver3D-OverrideRenderState>`  const+
+------------+--------------------------------------------------------------------------------------------+


|function-daframegraph-enableOverride|

.. _function-_at_daFrameGraph_c__c_color_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C1_ls_S_ls_daFrameGraph_c__c_VirtualAttachmentRequest_gr__gr_A:

.. das:function:: color(self: VirtualPassRequest; attachments: array<daFrameGraph::VirtualAttachmentRequest> const)

color returns  :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>`

+-----------+-----------------------------------------------------------------------------------------------------------+
+argument   +argument type                                                                                              +
+===========+===========================================================================================================+
+self       + :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>`                          +
+-----------+-----------------------------------------------------------------------------------------------------------+
+attachments+array< :ref:`daframegraph::VirtualAttachmentRequest <struct-daframegraph-VirtualAttachmentRequest>` > const+
+-----------+-----------------------------------------------------------------------------------------------------------+


|function-daframegraph-color|

.. _function-_at_daFrameGraph_c__c_optional_Y_ls_TT_gr_.:

.. das:function:: optional(self: auto(TT))

optional returns TT

+--------+-------------+
+argument+argument type+
+========+=============+
+self    +auto(TT)     +
+--------+-------------+


|function-daframegraph-optional|

.. _function-_at_daFrameGraph_c__c_useAs_Y_ls_TT_gr_._CE8_ls_daFgCore_c__c_Usage_gr_:

.. das:function:: useAs(self: auto(TT); usageType: Usage const)

useAs returns TT

+---------+------------------------------------------------------+
+argument +argument type                                         +
+=========+======================================================+
+self     + auto(TT)                                             +
+---------+------------------------------------------------------+
+usageType+ :ref:`daFgCore::Usage <enum-daFgCore-Usage>`  const  +
+---------+------------------------------------------------------+


|function-daframegraph-useAs|

.. _function-_at_daFrameGraph_c__c_atStage_Y_ls_TT_gr_._CE8_ls_daFgCore_c__c_Stage_gr_:

.. das:function:: atStage(self: auto(TT); stage: Stage const)

atStage returns TT

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+self    + auto(TT)                                              +
+--------+-------------------------------------------------------+
+stage   + :ref:`daFgCore::Stage <enum-daFgCore-Stage>`  const   +
+--------+-------------------------------------------------------+


|function-daframegraph-atStage|

.. _function-_at_daFrameGraph_c__c_bindToShaderVar_Y_ls_TT_gr_._Cs:

.. das:function:: bindToShaderVar(self: auto(TT); name: string const)

bindToShaderVar returns TT

+--------+-------------+
+argument+argument type+
+========+=============+
+self    +auto(TT)     +
+--------+-------------+
+name    +string const +
+--------+-------------+


|function-daframegraph-bindToShaderVar|

.. _function-_at_daFrameGraph_c__c_color_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C[-1]S_ls_daFrameGraph_c__c_VirtualTextureRequest_gr_:

.. das:function:: color(self: VirtualPassRequest; requests: VirtualTextureRequest const[])

color returns  :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>`

+--------+--------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                     +
+========+==================================================================================================+
+self    + :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>`                 +
+--------+--------------------------------------------------------------------------------------------------+
+requests+ :ref:`daframegraph::VirtualTextureRequest <struct-daframegraph-VirtualTextureRequest>`  const[-1]+
+--------+--------------------------------------------------------------------------------------------------+


|function-daframegraph-color|

.. _function-_at_daFrameGraph_c__c_color_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C[-1]s:

.. das:function:: color(self: VirtualPassRequest; names: string const[])

color returns  :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>`

+--------+----------------------------------------------------------------------------------+
+argument+argument type                                                                     +
+========+==================================================================================+
+self    + :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>` +
+--------+----------------------------------------------------------------------------------+
+names   + string const[-1]                                                                 +
+--------+----------------------------------------------------------------------------------+


|function-daframegraph-color|

.. _function-_at_daFrameGraph_c__c_depthRw_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C.:

.. das:function:: depthRw(self: VirtualPassRequest; attachment: auto const)

depthRw returns  :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>`

+----------+----------------------------------------------------------------------------------+
+argument  +argument type                                                                     +
+==========+==================================================================================+
+self      + :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>` +
+----------+----------------------------------------------------------------------------------+
+attachment+ auto const                                                                       +
+----------+----------------------------------------------------------------------------------+


|function-daframegraph-depthRw|

.. _function-_at_daFrameGraph_c__c_depthRo_S_ls_daFrameGraph_c__c_VirtualPassRequest_gr__C.:

.. das:function:: depthRo(self: VirtualPassRequest; attachment: auto const)

depthRo returns  :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>`

+----------+----------------------------------------------------------------------------------+
+argument  +argument type                                                                     +
+==========+==================================================================================+
+self      + :ref:`daframegraph::VirtualPassRequest <struct-daframegraph-VirtualPassRequest>` +
+----------+----------------------------------------------------------------------------------+
+attachment+ auto const                                                                       +
+----------+----------------------------------------------------------------------------------+


|function-daframegraph-depthRo|


