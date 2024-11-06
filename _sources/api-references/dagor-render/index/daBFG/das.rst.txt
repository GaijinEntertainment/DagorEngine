..
  This is auto generated file. See daBfg/api/das/docs

.. _stdlib_daBfg:

==========
Bfg in das
==========

To use daBfg inside daScript you need first of all compile daBfg library with ``DABFG_ENABLE_DAS_INTERGRATION = yes``.

This will compile das module that you can import with ``require daBfg.bfg_ecs``
or ``require daBfg``, whether you need ecs support or not.

DaScript daBfg methods are very similar to cpp methods, so usage will be the same, but with das syntax instead.

:ref:`daBfg::registerNode <function-_at_daBfg_c__c_registerNode_S_ls_daBfg_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_H_ls_daBfgCore_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>` registers node with provided name and declaration callback.
Returns :ref:`NodeHandle <handle-daBfgCore-NodeHandle>`.

Declaration callback is a das lambda with one argument :ref:`Registry <handle-daBfgCore-Registry>`.
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
Type aliases
++++++++++++

.. _alias-VrsRateTexture:

.. das:attribute:: VrsRateTexture is a variant type

+----+------------------------------------------------------------------------------------+
+some+ :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` +
+----+------------------------------------------------------------------------------------+
+none+void?                                                                               +
+----+------------------------------------------------------------------------------------+


|typedef-daBfg-VrsRateTexture|

.. _alias-VirtualAttachmentResource:

.. das:attribute:: VirtualAttachmentResource is a variant type

+------+--------------------------------------------+
+resUid+ :ref:`daBfg::ResUid <struct-daBfg-ResUid>` +
+------+--------------------------------------------+
+name  +string                                      +
+------+--------------------------------------------+


|typedef-daBfg-VirtualAttachmentResource|

.. _alias-TextureResolution:

.. das:attribute:: TextureResolution is a variant type

+-------+--------------------------------------------------------------------------+
+res    +tuple<x:uint;y:uint>                                                      +
+-------+--------------------------------------------------------------------------+
+autoRes+ :ref:`daBfg::AutoResolutionRequest <struct-daBfg-AutoResolutionRequest>` +
+-------+--------------------------------------------------------------------------+


|typedef-daBfg-TextureResolution|

.. _struct-daBfg-NameSpaceRequest:

.. das:attribute:: NameSpaceRequest



NameSpaceRequest fields are

+-----------+-------------------------------------------------------------------------+
+nameSpaceId+ :ref:`daBfgCore::NameSpaceNameId <enum-daBfgCore-NameSpaceNameId>`      +
+-----------+-------------------------------------------------------------------------+
+nodeId     + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+-----------+-------------------------------------------------------------------------+
+registry   + :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+-----------+-------------------------------------------------------------------------+


|structure-daBfg-NameSpaceRequest|

.. _struct-daBfg-Registry:

.. das:attribute:: Registry

 : NameSpaceRequest

Registry fields are

+-----------+-------------------------------------------------------------------------+
+nameSpaceId+ :ref:`daBfgCore::NameSpaceNameId <enum-daBfgCore-NameSpaceNameId>`      +
+-----------+-------------------------------------------------------------------------+
+nodeId     + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+-----------+-------------------------------------------------------------------------+
+registry   + :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+-----------+-------------------------------------------------------------------------+


|structure-daBfg-Registry|

.. _struct-daBfg-NameSpace:

.. das:attribute:: NameSpace



NameSpace fields are

+-----------+--------------------------------------------------------------------+
+nameSpaceId+ :ref:`daBfgCore::NameSpaceNameId <enum-daBfgCore-NameSpaceNameId>` +
+-----------+--------------------------------------------------------------------+


|structure-daBfg-NameSpace|

.. _struct-daBfg-ResUid:

.. das:attribute:: ResUid



ResUid fields are

+-------+--------------------------------------------------------+
+nameId + :ref:`daBfgCore::ResNameId <enum-daBfgCore-ResNameId>` +
+-------+--------------------------------------------------------+
+history+bool                                                    +
+-------+--------------------------------------------------------+


|structure-daBfg-ResUid|

.. _struct-daBfg-VirtualResourceRequestBase:

.. das:attribute:: VirtualResourceRequestBase



VirtualResourceRequestBase fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualResourceRequestBase|

.. _struct-daBfg-VirtualResourceHandle:

.. das:attribute:: VirtualResourceHandle



VirtualResourceHandle fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualResourceHandle|

.. _struct-daBfg-VirtualResourceCreationSemiRequest:

.. das:attribute:: VirtualResourceCreationSemiRequest

 : VirtualResourceRequestBase

VirtualResourceCreationSemiRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualResourceCreationSemiRequest|

.. _struct-daBfg-VirtualResourceSemiRequest:

.. das:attribute:: VirtualResourceSemiRequest

 : VirtualResourceRequestBase

VirtualResourceSemiRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualResourceSemiRequest|

.. _struct-daBfg-VirtualResourceRequest:

.. das:attribute:: VirtualResourceRequest

 : VirtualResourceRequestBase

VirtualResourceRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualResourceRequest|

.. _struct-daBfg-VirtualTextureHandle:

.. das:attribute:: VirtualTextureHandle

 : VirtualResourceHandle

VirtualTextureHandle fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualTextureHandle|

.. _struct-daBfg-VirtualBufferHandle:

.. das:attribute:: VirtualBufferHandle

 : VirtualResourceHandle

VirtualBufferHandle fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualBufferHandle|

.. _struct-daBfg-VirtualTextureRequest:

.. das:attribute:: VirtualTextureRequest

 : VirtualResourceRequest

VirtualTextureRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualTextureRequest|

.. _struct-daBfg-VirtualBufferRequest:

.. das:attribute:: VirtualBufferRequest

 : VirtualResourceRequest

VirtualBufferRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                              +
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualBufferRequest|

.. _struct-daBfg-StateRequest:

.. das:attribute:: StateRequest



StateRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-StateRequest|

.. _struct-daBfg-VrsRequirements:

.. das:attribute:: VrsRequirements



VrsRequirements fields are

+--------------+--------------------------------------------------------------------------------------------+
+rateX         +uint                                                                                        +
+--------------+--------------------------------------------------------------------------------------------+
+rateY         +uint                                                                                        +
+--------------+--------------------------------------------------------------------------------------------+
+rateTexture   + :ref:`VrsRateTexture <alias-VrsRateTexture>`                                               +
+--------------+--------------------------------------------------------------------------------------------+
+vertexCombiner+ :ref:`daBfgCore::VariableRateShadingCombiner <enum-daBfgCore-VariableRateShadingCombiner>` +
+--------------+--------------------------------------------------------------------------------------------+
+pixelCombiner + :ref:`daBfgCore::VariableRateShadingCombiner <enum-daBfgCore-VariableRateShadingCombiner>` +
+--------------+--------------------------------------------------------------------------------------------+


|structure-daBfg-VrsRequirements|

.. _struct-daBfg-VirtualPassRequest:

.. das:attribute:: VirtualPassRequest



VirtualPassRequest fields are

+--------+-------------------------------------------------------------------------+
+registry+ :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+--------+-------------------------------------------------------------------------+
+nodeId  + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`                +
+--------+-------------------------------------------------------------------------+


|structure-daBfg-VirtualPassRequest|

.. _struct-daBfg-VirtualAttachmentRequest:

.. das:attribute:: VirtualAttachmentRequest



VirtualAttachmentRequest fields are

+--------+--------------------------------------------------------------------+
+resource+ :ref:`VirtualAttachmentResource <alias-VirtualAttachmentResource>` +
+--------+--------------------------------------------------------------------+
+mipLevel+uint                                                                +
+--------+--------------------------------------------------------------------+
+layer   +uint                                                                +
+--------+--------------------------------------------------------------------+


|structure-daBfg-VirtualAttachmentRequest|

.. _struct-daBfg-AutoResolutionRequest:

.. das:attribute:: AutoResolutionRequest



AutoResolutionRequest fields are

+-------------+-------------------------------------------------------------------------+
+autoResTypeId+ :ref:`daBfgCore::AutoResTypeNameId <enum-daBfgCore-AutoResTypeNameId>`  +
+-------------+-------------------------------------------------------------------------+
+multiplier   +float                                                                    +
+-------------+-------------------------------------------------------------------------+
+registry     + :ref:`daBfgCore::InternalRegistry <handle-daBfgCore-InternalRegistry>` ?+
+-------------+-------------------------------------------------------------------------+


|structure-daBfg-AutoResolutionRequest|

.. _struct-daBfg-Texture2dCreateInfo:

.. das:attribute:: Texture2dCreateInfo



Texture2dCreateInfo fields are

+-------------+----------------------------------------------------+
+resolution   + :ref:`TextureResolution <alias-TextureResolution>` +
+-------------+----------------------------------------------------+
+creationFlags+uint                                                +
+-------------+----------------------------------------------------+
+mipLevels    +uint                                                +
+-------------+----------------------------------------------------+


|structure-daBfg-Texture2dCreateInfo|

.. _struct-daBfg-BufferCreateInfo:

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


|structure-daBfg-BufferCreateInfo|

.. _struct-daBfg-NamedSlot:

.. das:attribute:: NamedSlot



NamedSlot fields are

+----+------+
+name+string+
+----+------+


|structure-daBfg-NamedSlot|

+++++++++++++++++++
Top level functions
+++++++++++++++++++

  *  :ref:`root () : daBfg::NameSpace <function-_at_daBfg_c__c_root>` 
  *  :ref:`/ (self:daBfg::NameSpace -const;child_name:string const) : daBfg::NameSpace <function-_at_daBfg_c__c_/_S_ls_daBfg_c__c_NameSpace_gr__Cs>` 
  *  :ref:`fillSlot (self:daBfg::NameSpace -const;slot:daBfg::NamedSlot const;res_name_space:daBfg::NameSpace const;res_name:string const) : void <function-_at_daBfg_c__c_fillSlot_S_ls_daBfg_c__c_NameSpace_gr__CS_ls_daBfg_c__c_NamedSlot_gr__CS_ls_daBfg_c__c_NameSpace_gr__Cs>` 
  *  :ref:`registerNode (self:daBfg::NameSpace -const;name:string const;declaration_callback:lambda\<(var reg:daBfg::Registry -const):lambda\<void\>\> -const) : daBfgCore::NodeHandle <function-_at_daBfg_c__c_registerNode_S_ls_daBfg_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_S_ls_daBfg_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>` 
  *  :ref:`root (self:daBfg::Registry -const) : daBfg::NameSpaceRequest <function-_at_daBfg_c__c_root_S_ls_daBfg_c__c_Registry_gr_>` 
  *  :ref:`/ (self:daBfg::NameSpaceRequest -const;child_name:string const) : daBfg::NameSpaceRequest <function-_at_daBfg_c__c_/_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs>` 

.. _function-_at_daBfg_c__c_root:

.. das:function:: root()

root returns  :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>` 

|function-daBfg-root|

.. _function-_at_daBfg_c__c_/_S_ls_daBfg_c__c_NameSpace_gr__Cs:

.. das:function:: operator /(self: NameSpace; child_name: string const)

/ returns  :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>` 

+----------+--------------------------------------------------+
+argument  +argument type                                     +
+==========+==================================================+
+self      + :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>` +
+----------+--------------------------------------------------+
+child_name+string const                                      +
+----------+--------------------------------------------------+


|function-daBfg-/|

.. _function-_at_daBfg_c__c_fillSlot_S_ls_daBfg_c__c_NameSpace_gr__CS_ls_daBfg_c__c_NamedSlot_gr__CS_ls_daBfg_c__c_NameSpace_gr__Cs:

.. das:function:: fillSlot(self: NameSpace; slot: NamedSlot const; res_name_space: NameSpace const; res_name: string const)

+--------------+--------------------------------------------------------+
+argument      +argument type                                           +
+==============+========================================================+
+self          + :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>`       +
+--------------+--------------------------------------------------------+
+slot          + :ref:`daBfg::NamedSlot <struct-daBfg-NamedSlot>`  const+
+--------------+--------------------------------------------------------+
+res_name_space+ :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>`  const+
+--------------+--------------------------------------------------------+
+res_name      +string const                                            +
+--------------+--------------------------------------------------------+


|function-daBfg-fillSlot|

.. _function-_at_daBfg_c__c_registerNode_S_ls_daBfg_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_S_ls_daBfg_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_:

.. das:function:: registerNode(self: NameSpace; name: string const; declaration_callback: lambda<(var reg:daBfg::Registry -const):lambda<void>>)

registerNode returns  :ref:`daBfgCore::NodeHandle <handle-daBfgCore-NodeHandle>` 

+--------------------+-----------------------------------------------------------------------+
+argument            +argument type                                                          +
+====================+=======================================================================+
+self                + :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>`                      +
+--------------------+-----------------------------------------------------------------------+
+name                +string const                                                           +
+--------------------+-----------------------------------------------------------------------+
+declaration_callback+lambda<(reg: :ref:`daBfg::Registry <struct-daBfg-Registry>` ):lambda<>>+
+--------------------+-----------------------------------------------------------------------+


|function-daBfg-registerNode|

.. _function-_at_daBfg_c__c_root_S_ls_daBfg_c__c_Registry_gr_:

.. das:function:: root(self: Registry)

root returns  :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>` +
+--------+------------------------------------------------+


|function-daBfg-root|

.. _function-_at_daBfg_c__c_/_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: operator /(self: NameSpaceRequest; child_name: string const)

/ returns  :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` 

+----------+----------------------------------------------------------------+
+argument  +argument type                                                   +
+==========+================================================================+
+self      + :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` +
+----------+----------------------------------------------------------------+
+child_name+string const                                                    +
+----------+----------------------------------------------------------------+


|function-daBfg-/|

+++++++++++++++++++++
Registry manipulation
+++++++++++++++++++++

  *  :ref:`orderMeBefore (self:daBfg::Registry -const;name:string const) : daBfg::Registry <function-_at_daBfg_c__c_orderMeBefore_S_ls_daBfg_c__c_Registry_gr__Cs>` 
  *  :ref:`orderMeBefore (self:daBfg::Registry -const;names:array\<string\> const) : daBfg::Registry <function-_at_daBfg_c__c_orderMeBefore_S_ls_daBfg_c__c_Registry_gr__C1_ls_s_gr_A>` 
  *  :ref:`orderMeAfter (self:daBfg::Registry -const;name:string const) : daBfg::Registry <function-_at_daBfg_c__c_orderMeAfter_S_ls_daBfg_c__c_Registry_gr__Cs>` 
  *  :ref:`orderMeAfter (self:daBfg::Registry -const;names:array\<string\> const) : daBfg::Registry <function-_at_daBfg_c__c_orderMeAfter_S_ls_daBfg_c__c_Registry_gr__C1_ls_s_gr_A>` 
  *  :ref:`setPriority (self:daBfg::Registry -const;priority:int const) : daBfg::Registry <function-_at_daBfg_c__c_setPriority_S_ls_daBfg_c__c_Registry_gr__Ci>` 
  *  :ref:`multiplex (self:daBfg::Registry -const;multiplexing_mode:daBfgCore::MultiplexingMode const) : daBfg::Registry <function-_at_daBfg_c__c_multiplex_S_ls_daBfg_c__c_Registry_gr__CE_ls_daBfgCore_c__c_MultiplexingMode_gr_>` 
  *  :ref:`executionHas (self:daBfg::Registry -const;side_effect:daBfgCore::SideEffect const) : daBfg::Registry <function-_at_daBfg_c__c_executionHas_S_ls_daBfg_c__c_Registry_gr__CE8_ls_daBfgCore_c__c_SideEffect_gr_>` 
  *  :ref:`create (self:daBfg::Registry -const;name:string const;history:daBfgCore::History const) : daBfg::VirtualResourceCreationSemiRequest <function-_at_daBfg_c__c_create_S_ls_daBfg_c__c_Registry_gr__Cs_CE8_ls_daBfgCore_c__c_History_gr_>` 
  *  :ref:`getResolution (self:daBfg::NameSpaceRequest -const;type_name:string const;multiplier:float const) : daBfg::AutoResolutionRequest <function-_at_daBfg_c__c_getResolution_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs_Cf>` 
  *  :ref:`read (self:daBfg::NameSpaceRequest -const;name:string const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_read_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs>` 
  *  :ref:`read (self:daBfg::NameSpaceRequest -const;slot:daBfg::NamedSlot const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_read_S_ls_daBfg_c__c_NameSpaceRequest_gr__CS_ls_daBfg_c__c_NamedSlot_gr_>` 
  *  :ref:`history (self:daBfg::NameSpaceRequest -const;name:string -const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_history_S_ls_daBfg_c__c_NameSpaceRequest_gr__s>` 
  *  :ref:`modify (self:daBfg::NameSpaceRequest -const;name:string const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_modify_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs>` 
  *  :ref:`modify (self:daBfg::NameSpaceRequest -const;slot:daBfg::NamedSlot const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_modify_S_ls_daBfg_c__c_NameSpaceRequest_gr__CS_ls_daBfg_c__c_NamedSlot_gr_>` 
  *  :ref:`rename (self:daBfg::NameSpaceRequest -const;from:string const;to:string const;history:daBfgCore::History const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_rename_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs_Cs_CE8_ls_daBfgCore_c__c_History_gr_>` 
  *  :ref:`get (resolution:daBfg::AutoResolutionRequest const) : int2 <function-_at_daBfg_c__c_get_CS_ls_daBfg_c__c_AutoResolutionRequest_gr_>` 
  *  :ref:`requestState (self:daBfg::Registry -const) : daBfg::StateRequest <function-_at_daBfg_c__c_requestState_S_ls_daBfg_c__c_Registry_gr_>` 
  *  :ref:`requestRenderPass (self:daBfg::Registry -const) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_requestRenderPass_S_ls_daBfg_c__c_Registry_gr_>` 

.. _function-_at_daBfg_c__c_orderMeBefore_S_ls_daBfg_c__c_Registry_gr__Cs:

.. das:function:: orderMeBefore(self: Registry; name: string const)

orderMeBefore returns  :ref:`daBfg::Registry <struct-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>` +
+--------+------------------------------------------------+
+name    +string const                                    +
+--------+------------------------------------------------+


|function-daBfg-orderMeBefore|

.. _function-_at_daBfg_c__c_orderMeBefore_S_ls_daBfg_c__c_Registry_gr__C1_ls_s_gr_A:

.. das:function:: orderMeBefore(self: Registry; names: array<string> const)

orderMeBefore returns  :ref:`daBfg::Registry <struct-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>` +
+--------+------------------------------------------------+
+names   +array<string> const                             +
+--------+------------------------------------------------+


|function-daBfg-orderMeBefore|

.. _function-_at_daBfg_c__c_orderMeAfter_S_ls_daBfg_c__c_Registry_gr__Cs:

.. das:function:: orderMeAfter(self: Registry; name: string const)

orderMeAfter returns  :ref:`daBfg::Registry <struct-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>` +
+--------+------------------------------------------------+
+name    +string const                                    +
+--------+------------------------------------------------+


|function-daBfg-orderMeAfter|

.. _function-_at_daBfg_c__c_orderMeAfter_S_ls_daBfg_c__c_Registry_gr__C1_ls_s_gr_A:

.. das:function:: orderMeAfter(self: Registry; names: array<string> const)

orderMeAfter returns  :ref:`daBfg::Registry <struct-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>` +
+--------+------------------------------------------------+
+names   +array<string> const                             +
+--------+------------------------------------------------+


|function-daBfg-orderMeAfter|

.. _function-_at_daBfg_c__c_setPriority_S_ls_daBfg_c__c_Registry_gr__Ci:

.. das:function:: setPriority(self: Registry; priority: int const)

setPriority returns  :ref:`daBfg::Registry <struct-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>` +
+--------+------------------------------------------------+
+priority+int const                                       +
+--------+------------------------------------------------+


|function-daBfg-setPriority|

.. _function-_at_daBfg_c__c_multiplex_S_ls_daBfg_c__c_Registry_gr__CE_ls_daBfgCore_c__c_MultiplexingMode_gr_:

.. das:function:: multiplex(self: Registry; multiplexing_mode: MultiplexingMode const)

multiplex returns  :ref:`daBfg::Registry <struct-daBfg-Registry>` 

+-----------------+----------------------------------------------------------------------------+
+argument         +argument type                                                               +
+=================+============================================================================+
+self             + :ref:`daBfg::Registry <struct-daBfg-Registry>`                             +
+-----------------+----------------------------------------------------------------------------+
+multiplexing_mode+ :ref:`daBfgCore::MultiplexingMode <enum-daBfgCore-MultiplexingMode>`  const+
+-----------------+----------------------------------------------------------------------------+


|function-daBfg-multiplex|

.. _function-_at_daBfg_c__c_executionHas_S_ls_daBfg_c__c_Registry_gr__CE8_ls_daBfgCore_c__c_SideEffect_gr_:

.. das:function:: executionHas(self: Registry; side_effect: SideEffect const)

executionHas returns  :ref:`daBfg::Registry <struct-daBfg-Registry>` 

+-----------+----------------------------------------------------------------+
+argument   +argument type                                                   +
+===========+================================================================+
+self       + :ref:`daBfg::Registry <struct-daBfg-Registry>`                 +
+-----------+----------------------------------------------------------------+
+side_effect+ :ref:`daBfgCore::SideEffect <enum-daBfgCore-SideEffect>`  const+
+-----------+----------------------------------------------------------------+


|function-daBfg-executionHas|

.. _function-_at_daBfg_c__c_create_S_ls_daBfg_c__c_Registry_gr__Cs_CE8_ls_daBfgCore_c__c_History_gr_:

.. das:function:: create(self: Registry; name: string const; history: History const)

create returns  :ref:`daBfg::VirtualResourceCreationSemiRequest <struct-daBfg-VirtualResourceCreationSemiRequest>` 

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>`           +
+--------+----------------------------------------------------------+
+name    +string const                                              +
+--------+----------------------------------------------------------+
+history + :ref:`daBfgCore::History <enum-daBfgCore-History>`  const+
+--------+----------------------------------------------------------+


|function-daBfg-create|

.. _function-_at_daBfg_c__c_getResolution_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs_Cf:

.. das:function:: getResolution(self: NameSpaceRequest; type_name: string const; multiplier: float const)

getResolution returns  :ref:`daBfg::AutoResolutionRequest <struct-daBfg-AutoResolutionRequest>` 

+----------+----------------------------------------------------------------+
+argument  +argument type                                                   +
+==========+================================================================+
+self      + :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` +
+----------+----------------------------------------------------------------+
+type_name +string const                                                    +
+----------+----------------------------------------------------------------+
+multiplier+float const                                                     +
+----------+----------------------------------------------------------------+


|function-daBfg-getResolution|

.. _function-_at_daBfg_c__c_read_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: read(self: NameSpaceRequest; name: string const)

read returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+name    +string const                                                    +
+--------+----------------------------------------------------------------+


|function-daBfg-read|

.. _function-_at_daBfg_c__c_read_S_ls_daBfg_c__c_NameSpaceRequest_gr__CS_ls_daBfg_c__c_NamedSlot_gr_:

.. das:function:: read(self: NameSpaceRequest; slot: NamedSlot const)

read returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+slot    + :ref:`daBfg::NamedSlot <struct-daBfg-NamedSlot>`  const        +
+--------+----------------------------------------------------------------+


|function-daBfg-read|

.. _function-_at_daBfg_c__c_history_S_ls_daBfg_c__c_NameSpaceRequest_gr__s:

.. das:function:: history(self: NameSpaceRequest; name: string)

history returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+name    +string                                                          +
+--------+----------------------------------------------------------------+


|function-daBfg-history|

.. _function-_at_daBfg_c__c_modify_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: modify(self: NameSpaceRequest; name: string const)

modify returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+name    +string const                                                    +
+--------+----------------------------------------------------------------+


|function-daBfg-modify|

.. _function-_at_daBfg_c__c_modify_S_ls_daBfg_c__c_NameSpaceRequest_gr__CS_ls_daBfg_c__c_NamedSlot_gr_:

.. das:function:: modify(self: NameSpaceRequest; slot: NamedSlot const)

modify returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+slot    + :ref:`daBfg::NamedSlot <struct-daBfg-NamedSlot>`  const        +
+--------+----------------------------------------------------------------+


|function-daBfg-modify|

.. _function-_at_daBfg_c__c_rename_S_ls_daBfg_c__c_NameSpaceRequest_gr__Cs_Cs_CE8_ls_daBfgCore_c__c_History_gr_:

.. das:function:: rename(self: NameSpaceRequest; from: string const; to: string const; history: History const)

rename returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <struct-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+from    +string const                                                    +
+--------+----------------------------------------------------------------+
+to      +string const                                                    +
+--------+----------------------------------------------------------------+
+history + :ref:`daBfgCore::History <enum-daBfgCore-History>`  const      +
+--------+----------------------------------------------------------------+


|function-daBfg-rename|

.. _function-_at_daBfg_c__c_get_CS_ls_daBfg_c__c_AutoResolutionRequest_gr_:

.. das:function:: get(resolution: AutoResolutionRequest const)

get returns int2

+----------+--------------------------------------------------------------------------------+
+argument  +argument type                                                                   +
+==========+================================================================================+
+resolution+ :ref:`daBfg::AutoResolutionRequest <struct-daBfg-AutoResolutionRequest>`  const+
+----------+--------------------------------------------------------------------------------+


|function-daBfg-get|

.. _function-_at_daBfg_c__c_requestState_S_ls_daBfg_c__c_Registry_gr_:

.. das:function:: requestState(self: Registry)

requestState returns  :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>` +
+--------+------------------------------------------------+


|function-daBfg-requestState|

.. _function-_at_daBfg_c__c_requestRenderPass_S_ls_daBfg_c__c_Registry_gr_:

.. das:function:: requestRenderPass(self: Registry)

requestRenderPass returns  :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <struct-daBfg-Registry>` +
+--------+------------------------------------------------+


|function-daBfg-requestRenderPass|

++++++++++++++++++++
Request manipulation
++++++++++++++++++++

  *  :ref:`texture (self:daBfg::VirtualResourceCreationSemiRequest -const;info:daBfg::Texture2dCreateInfo const) : daBfg::VirtualTextureRequest <function-_at_daBfg_c__c_texture_S_ls_daBfg_c__c_VirtualResourceCreationSemiRequest_gr__CS_ls_daBfg_c__c_Texture2dCreateInfo_gr_>` 
  *  :ref:`texture (self:daBfg::VirtualResourceSemiRequest -const) : daBfg::VirtualTextureRequest <function-_at_daBfg_c__c_texture_S_ls_daBfg_c__c_VirtualResourceSemiRequest_gr_>` 
  *  :ref:`buffer (self:daBfg::VirtualResourceCreationSemiRequest -const;info:daBfg::BufferCreateInfo const) : daBfg::VirtualBufferRequest <function-_at_daBfg_c__c_buffer_S_ls_daBfg_c__c_VirtualResourceCreationSemiRequest_gr__CS_ls_daBfg_c__c_BufferCreateInfo_gr_>` 
  *  :ref:`buffer (self:daBfg::VirtualResourceSemiRequest -const) : daBfg::VirtualBufferRequest <function-_at_daBfg_c__c_buffer_S_ls_daBfg_c__c_VirtualResourceSemiRequest_gr_>` 
  *  :ref:`blob (self:daBfg::VirtualResourceSemiRequest -const) : daBfg::VirtualResourceRequest <function-_at_daBfg_c__c_blob_S_ls_daBfg_c__c_VirtualResourceSemiRequest_gr_>` 
  *  :ref:`modifyRequest (self:daBfg::VirtualResourceRequest -const;modifier:block\<(var request:daBfgCore::ResourceRequest -const):void\> const) : void <function-_at_daBfg_c__c_modifyRequest_S_ls_daBfg_c__c_VirtualResourceRequest_gr__CN_ls_request_gr_0_ls_H_ls_daBfgCore_c__c_ResourceRequest_gr__gr_1_ls_v_gr__builtin_>` 
  *  :ref:`handle (self:daBfg::VirtualTextureRequest const) : daBfg::VirtualTextureHandle <function-_at_daBfg_c__c_handle_CS_ls_daBfg_c__c_VirtualTextureRequest_gr_>` 
  *  :ref:`handle (self:daBfg::VirtualBufferRequest const) : daBfg::VirtualBufferHandle <function-_at_daBfg_c__c_handle_CS_ls_daBfg_c__c_VirtualBufferRequest_gr_>` 
  *  :ref:`view (handle:daBfg::VirtualTextureHandle const) : DagorResPtr::ManagedTexView <function-_at_daBfg_c__c_view_CS_ls_daBfg_c__c_VirtualTextureHandle_gr_>` 
  *  :ref:`view (handle:daBfg::VirtualBufferHandle const) : DagorResPtr::ManagedBufView <function-_at_daBfg_c__c_view_CS_ls_daBfg_c__c_VirtualBufferHandle_gr_>` 
  *  :ref:`setFrameBlock (self:daBfg::StateRequest -const;name:string const) : daBfg::StateRequest <function-_at_daBfg_c__c_setFrameBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs>` 
  *  :ref:`setSceneBlock (self:daBfg::StateRequest -const;name:string const) : daBfg::StateRequest <function-_at_daBfg_c__c_setSceneBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs>` 
  *  :ref:`setObjectBlock (self:daBfg::StateRequest -const;name:string const) : daBfg::StateRequest <function-_at_daBfg_c__c_setObjectBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs>` 
  *  :ref:`allowWireFrame (self:daBfg::StateRequest -const) : daBfg::StateRequest <function-_at_daBfg_c__c_allowWireFrame_S_ls_daBfg_c__c_StateRequest_gr_>` 
  *  :ref:`allowVrs (self:daBfg::StateRequest -const;vrs:daBfg::VrsRequirements const) : daBfg::StateRequest <function-_at_daBfg_c__c_allowVrs_S_ls_daBfg_c__c_StateRequest_gr__CS_ls_daBfg_c__c_VrsRequirements_gr_>` 
  *  :ref:`enableOverride (self:daBfg::StateRequest -const;das_override:DagorDriver3D::OverrideRenderState const) : daBfg::StateRequest <function-_at_daBfg_c__c_enableOverride_S_ls_daBfg_c__c_StateRequest_gr__CS_ls_DagorDriver3D_c__c_OverrideRenderState_gr_>` 
  *  :ref:`color (self:daBfg::VirtualPassRequest -const;attachments:array\<daBfg::VirtualAttachmentRequest\> const) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C1_ls_S_ls_daBfg_c__c_VirtualAttachmentRequest_gr__gr_A>` 
  *  :ref:`optional (self:auto(TT) -const) : TT <function-_at_daBfg_c__c_optional_Y_ls_TT_gr_.>` 
  *  :ref:`useAs (self:auto(TT) -const;usageType:daBfgCore::Usage const) : TT <function-_at_daBfg_c__c_useAs_Y_ls_TT_gr_._CE8_ls_daBfgCore_c__c_Usage_gr_>` 
  *  :ref:`atStage (self:auto(TT) -const;stage:daBfgCore::Stage const) : TT <function-_at_daBfg_c__c_atStage_Y_ls_TT_gr_._CE8_ls_daBfgCore_c__c_Stage_gr_>` 
  *  :ref:`bindToShaderVar (self:auto(TT) -const;name:string const) : TT <function-_at_daBfg_c__c_bindToShaderVar_Y_ls_TT_gr_._Cs>` 
  *  :ref:`color (self:daBfg::VirtualPassRequest -const;requests:daBfg::VirtualTextureRequest const[]) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C[-1]S_ls_daBfg_c__c_VirtualTextureRequest_gr_>` 
  *  :ref:`color (self:daBfg::VirtualPassRequest -const;names:string const[]) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C[-1]s>` 
  *  :ref:`depthRw (self:daBfg::VirtualPassRequest -const;attachment:auto const) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_depthRw_S_ls_daBfg_c__c_VirtualPassRequest_gr__C.>` 
  *  :ref:`depthRo (self:daBfg::VirtualPassRequest -const;attachment:auto const) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_depthRo_S_ls_daBfg_c__c_VirtualPassRequest_gr__C.>` 

.. _function-_at_daBfg_c__c_texture_S_ls_daBfg_c__c_VirtualResourceCreationSemiRequest_gr__CS_ls_daBfg_c__c_Texture2dCreateInfo_gr_:

.. das:function:: texture(self: VirtualResourceCreationSemiRequest; info: Texture2dCreateInfo const)

texture returns  :ref:`daBfg::VirtualTextureRequest <struct-daBfg-VirtualTextureRequest>` 

+--------+----------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                       +
+========+====================================================================================================+
+self    + :ref:`daBfg::VirtualResourceCreationSemiRequest <struct-daBfg-VirtualResourceCreationSemiRequest>` +
+--------+----------------------------------------------------------------------------------------------------+
+info    + :ref:`daBfg::Texture2dCreateInfo <struct-daBfg-Texture2dCreateInfo>`  const                        +
+--------+----------------------------------------------------------------------------------------------------+


|function-daBfg-texture|

.. _function-_at_daBfg_c__c_texture_S_ls_daBfg_c__c_VirtualResourceSemiRequest_gr_:

.. das:function:: texture(self: VirtualResourceSemiRequest)

texture returns  :ref:`daBfg::VirtualTextureRequest <struct-daBfg-VirtualTextureRequest>` 

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+self    + :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` +
+--------+------------------------------------------------------------------------------------+


|function-daBfg-texture|

.. _function-_at_daBfg_c__c_buffer_S_ls_daBfg_c__c_VirtualResourceCreationSemiRequest_gr__CS_ls_daBfg_c__c_BufferCreateInfo_gr_:

.. das:function:: buffer(self: VirtualResourceCreationSemiRequest; info: BufferCreateInfo const)

buffer returns  :ref:`daBfg::VirtualBufferRequest <struct-daBfg-VirtualBufferRequest>` 

+--------+----------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                       +
+========+====================================================================================================+
+self    + :ref:`daBfg::VirtualResourceCreationSemiRequest <struct-daBfg-VirtualResourceCreationSemiRequest>` +
+--------+----------------------------------------------------------------------------------------------------+
+info    + :ref:`daBfg::BufferCreateInfo <struct-daBfg-BufferCreateInfo>`  const                              +
+--------+----------------------------------------------------------------------------------------------------+


|function-daBfg-buffer|

.. _function-_at_daBfg_c__c_buffer_S_ls_daBfg_c__c_VirtualResourceSemiRequest_gr_:

.. das:function:: buffer(self: VirtualResourceSemiRequest)

buffer returns  :ref:`daBfg::VirtualBufferRequest <struct-daBfg-VirtualBufferRequest>` 

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+self    + :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` +
+--------+------------------------------------------------------------------------------------+


|function-daBfg-buffer|

.. _function-_at_daBfg_c__c_blob_S_ls_daBfg_c__c_VirtualResourceSemiRequest_gr_:

.. das:function:: blob(self: VirtualResourceSemiRequest)

blob returns  :ref:`daBfg::VirtualResourceRequest <struct-daBfg-VirtualResourceRequest>` 

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+self    + :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` +
+--------+------------------------------------------------------------------------------------+


|function-daBfg-blob|

.. _function-_at_daBfg_c__c_modifyRequest_S_ls_daBfg_c__c_VirtualResourceRequest_gr__CN_ls_request_gr_0_ls_H_ls_daBfgCore_c__c_ResourceRequest_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: modifyRequest(self: VirtualResourceRequest; modifier: block<(var request:daBfgCore::ResourceRequest -const):void> const)

+--------+--------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                     +
+========+==================================================================================================+
+self    + :ref:`daBfg::VirtualResourceRequest <struct-daBfg-VirtualResourceRequest>`                       +
+--------+--------------------------------------------------------------------------------------------------+
+modifier+block<(request: :ref:`daBfgCore::ResourceRequest <handle-daBfgCore-ResourceRequest>` ):void> const+
+--------+--------------------------------------------------------------------------------------------------+


|function-daBfg-modifyRequest|

.. _function-_at_daBfg_c__c_handle_CS_ls_daBfg_c__c_VirtualTextureRequest_gr_:

.. das:function:: handle(self: VirtualTextureRequest const)

handle returns  :ref:`daBfg::VirtualTextureHandle <struct-daBfg-VirtualTextureHandle>` 

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`daBfg::VirtualTextureRequest <struct-daBfg-VirtualTextureRequest>`  const+
+--------+--------------------------------------------------------------------------------+


|function-daBfg-handle|

.. _function-_at_daBfg_c__c_handle_CS_ls_daBfg_c__c_VirtualBufferRequest_gr_:

.. das:function:: handle(self: VirtualBufferRequest const)

handle returns  :ref:`daBfg::VirtualBufferHandle <struct-daBfg-VirtualBufferHandle>` 

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+self    + :ref:`daBfg::VirtualBufferRequest <struct-daBfg-VirtualBufferRequest>`  const+
+--------+------------------------------------------------------------------------------+


|function-daBfg-handle|

.. _function-_at_daBfg_c__c_view_CS_ls_daBfg_c__c_VirtualTextureHandle_gr_:

.. das:function:: view(handle: VirtualTextureHandle const)

view returns  :ref:`DagorResPtr::ManagedTexView <handle-DagorResPtr-ManagedTexView>` 

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+handle  + :ref:`daBfg::VirtualTextureHandle <struct-daBfg-VirtualTextureHandle>`  const+
+--------+------------------------------------------------------------------------------+


|function-daBfg-view|

.. _function-_at_daBfg_c__c_view_CS_ls_daBfg_c__c_VirtualBufferHandle_gr_:

.. das:function:: view(handle: VirtualBufferHandle const)

view returns  :ref:`DagorResPtr::ManagedBufView <handle-DagorResPtr-ManagedBufView>` 

+--------+----------------------------------------------------------------------------+
+argument+argument type                                                               +
+========+============================================================================+
+handle  + :ref:`daBfg::VirtualBufferHandle <struct-daBfg-VirtualBufferHandle>`  const+
+--------+----------------------------------------------------------------------------+


|function-daBfg-view|

.. _function-_at_daBfg_c__c_setFrameBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs:

.. das:function:: setFrameBlock(self: StateRequest; name: string const)

setFrameBlock returns  :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` 

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` +
+--------+--------------------------------------------------------+
+name    +string const                                            +
+--------+--------------------------------------------------------+


|function-daBfg-setFrameBlock|

.. _function-_at_daBfg_c__c_setSceneBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs:

.. das:function:: setSceneBlock(self: StateRequest; name: string const)

setSceneBlock returns  :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` 

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` +
+--------+--------------------------------------------------------+
+name    +string const                                            +
+--------+--------------------------------------------------------+


|function-daBfg-setSceneBlock|

.. _function-_at_daBfg_c__c_setObjectBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs:

.. das:function:: setObjectBlock(self: StateRequest; name: string const)

setObjectBlock returns  :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` 

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` +
+--------+--------------------------------------------------------+
+name    +string const                                            +
+--------+--------------------------------------------------------+


|function-daBfg-setObjectBlock|

.. _function-_at_daBfg_c__c_allowWireFrame_S_ls_daBfg_c__c_StateRequest_gr_:

.. das:function:: allowWireFrame(self: StateRequest)

allowWireFrame returns  :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` 

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` +
+--------+--------------------------------------------------------+


|function-daBfg-allowWireFrame|

.. _function-_at_daBfg_c__c_allowVrs_S_ls_daBfg_c__c_StateRequest_gr__CS_ls_daBfg_c__c_VrsRequirements_gr_:

.. das:function:: allowVrs(self: StateRequest; vrs: VrsRequirements const)

allowVrs returns  :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` 

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+self    + :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>`             +
+--------+--------------------------------------------------------------------+
+vrs     + :ref:`daBfg::VrsRequirements <struct-daBfg-VrsRequirements>`  const+
+--------+--------------------------------------------------------------------+


|function-daBfg-allowVrs|

.. _function-_at_daBfg_c__c_enableOverride_S_ls_daBfg_c__c_StateRequest_gr__CS_ls_DagorDriver3D_c__c_OverrideRenderState_gr_:

.. das:function:: enableOverride(self: StateRequest; das_override: OverrideRenderState const)

enableOverride returns  :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` 

+------------+--------------------------------------------------------------------------------------------+
+argument    +argument type                                                                               +
+============+============================================================================================+
+self        + :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>`                                     +
+------------+--------------------------------------------------------------------------------------------+
+das_override+ :ref:`DagorDriver3D::OverrideRenderState <struct-DagorDriver3D-OverrideRenderState>`  const+
+------------+--------------------------------------------------------------------------------------------+


|function-daBfg-enableOverride|

.. _function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C1_ls_S_ls_daBfg_c__c_VirtualAttachmentRequest_gr__gr_A:

.. das:function:: color(self: VirtualPassRequest; attachments: array<daBfg::VirtualAttachmentRequest> const)

color returns  :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` 

+-----------+---------------------------------------------------------------------------------------------+
+argument   +argument type                                                                                +
+===========+=============================================================================================+
+self       + :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>`                          +
+-----------+---------------------------------------------------------------------------------------------+
+attachments+array< :ref:`daBfg::VirtualAttachmentRequest <struct-daBfg-VirtualAttachmentRequest>` > const+
+-----------+---------------------------------------------------------------------------------------------+


|function-daBfg-color|

.. _function-_at_daBfg_c__c_optional_Y_ls_TT_gr_.:

.. das:function:: optional(self: auto(TT))

optional returns TT

+--------+-------------+
+argument+argument type+
+========+=============+
+self    +auto(TT)     +
+--------+-------------+


|function-daBfg-optional|

.. _function-_at_daBfg_c__c_useAs_Y_ls_TT_gr_._CE8_ls_daBfgCore_c__c_Usage_gr_:

.. das:function:: useAs(self: auto(TT); usageType: Usage const)

useAs returns TT

+---------+------------------------------------------------------+
+argument +argument type                                         +
+=========+======================================================+
+self     +auto(TT)                                              +
+---------+------------------------------------------------------+
+usageType+ :ref:`daBfgCore::Usage <enum-daBfgCore-Usage>`  const+
+---------+------------------------------------------------------+


|function-daBfg-useAs|

.. _function-_at_daBfg_c__c_atStage_Y_ls_TT_gr_._CE8_ls_daBfgCore_c__c_Stage_gr_:

.. das:function:: atStage(self: auto(TT); stage: Stage const)

atStage returns TT

+--------+------------------------------------------------------+
+argument+argument type                                         +
+========+======================================================+
+self    +auto(TT)                                              +
+--------+------------------------------------------------------+
+stage   + :ref:`daBfgCore::Stage <enum-daBfgCore-Stage>`  const+
+--------+------------------------------------------------------+


|function-daBfg-atStage|

.. _function-_at_daBfg_c__c_bindToShaderVar_Y_ls_TT_gr_._Cs:

.. das:function:: bindToShaderVar(self: auto(TT); name: string const)

bindToShaderVar returns TT

+--------+-------------+
+argument+argument type+
+========+=============+
+self    +auto(TT)     +
+--------+-------------+
+name    +string const +
+--------+-------------+


|function-daBfg-bindToShaderVar|

.. _function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C[-1]S_ls_daBfg_c__c_VirtualTextureRequest_gr_:

.. das:function:: color(self: VirtualPassRequest; requests: VirtualTextureRequest const[])

color returns  :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` 

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+self    + :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>`                 +
+--------+------------------------------------------------------------------------------------+
+requests+ :ref:`daBfg::VirtualTextureRequest <struct-daBfg-VirtualTextureRequest>`  const[-1]+
+--------+------------------------------------------------------------------------------------+


|function-daBfg-color|

.. _function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C[-1]s:

.. das:function:: color(self: VirtualPassRequest; names: string const[])

color returns  :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` 

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+self    + :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` +
+--------+--------------------------------------------------------------------+
+names   +string const[-1]                                                    +
+--------+--------------------------------------------------------------------+


|function-daBfg-color|

.. _function-_at_daBfg_c__c_depthRw_S_ls_daBfg_c__c_VirtualPassRequest_gr__C.:

.. das:function:: depthRw(self: VirtualPassRequest; attachment: auto const)

depthRw returns  :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` 

+----------+--------------------------------------------------------------------+
+argument  +argument type                                                       +
+==========+====================================================================+
+self      + :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` +
+----------+--------------------------------------------------------------------+
+attachment+auto const                                                          +
+----------+--------------------------------------------------------------------+


|function-daBfg-depthRw|

.. _function-_at_daBfg_c__c_depthRo_S_ls_daBfg_c__c_VirtualPassRequest_gr__C.:

.. das:function:: depthRo(self: VirtualPassRequest; attachment: auto const)

depthRo returns  :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` 

+----------+--------------------------------------------------------------------+
+argument  +argument type                                                       +
+==========+====================================================================+
+self      + :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` +
+----------+--------------------------------------------------------------------+
+attachment+auto const                                                          +
+----------+--------------------------------------------------------------------+


|function-daBfg-depthRo|


