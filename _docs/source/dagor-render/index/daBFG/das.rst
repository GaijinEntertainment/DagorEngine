..
  This is auto generated file. See daBfg/api/das/docs

.. _stdlib_daBfg:

===============
DaScript module
===============

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
    handle <- root() |> registerNode(debug_visualization_node_name) <| @(var registry : Registry)
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
    handle <- root() |> registerNode("node_name") <| @(var registry : Registry)
      ...
      return <- @ <|
        ...

  ...

  var eid = getSingletonEntity("entity_name")
  query(eid) <| $ [es] (var node_handle : NodeHandle&)
    node_handle |> register_node
++++++++++++
Type aliases
++++++++++++

.. _alias-BindingsMap:

.. das:attribute:: BindingsMap = fixedVectorMap`int`Binding`8

|typedef-daBfg-BindingsMap|

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

++++++++++++
Enumerations
++++++++++++

.. _enum-daBfg-NameSpaceNameId:

.. das:attribute:: NameSpaceNameId

+-------+----------+
+Invalid+4294967295+
+-------+----------+


|enumeration-daBfg-NameSpaceNameId|

.. _enum-daBfg-NodeNameId:

.. das:attribute:: NodeNameId

+-------+----------+
+Invalid+4294967295+
+-------+----------+


|enumeration-daBfg-NodeNameId|

.. _enum-daBfg-ResNameId:

.. das:attribute:: ResNameId

+-------+----------+
+Invalid+4294967295+
+-------+----------+


|enumeration-daBfg-ResNameId|

.. _enum-daBfg-History:

.. das:attribute:: History

+---------------------+-+
+No                   +0+
+---------------------+-+
+ClearZeroOnFirstFrame+1+
+---------------------+-+
+DiscardOnFirstFrame  +2+
+---------------------+-+


|enumeration-daBfg-History|

.. _enum-daBfg-ResourceActivationAction:

.. das:attribute:: ResourceActivationAction

+---------------------------+-+
+REWRITE_AS_COPY_DESTINATION+0+
+---------------------------+-+
+REWRITE_AS_UAV             +1+
+---------------------------+-+
+REWRITE_AS_RTV_DSV         +2+
+---------------------------+-+
+CLEAR_F_AS_UAV             +3+
+---------------------------+-+
+CLEAR_I_AS_UAV             +4+
+---------------------------+-+
+CLEAR_AS_RTV_DSV           +5+
+---------------------------+-+
+DISCARD_AS_UAV             +6+
+---------------------------+-+
+DISCARD_AS_RTV_DSV         +7+
+---------------------------+-+


|enumeration-daBfg-ResourceActivationAction|

.. _enum-daBfg-MultiplexingMode:

.. das:attribute:: MultiplexingMode

+-------------+-+
+None         +0+
+-------------+-+
+SuperSampling+1+
+-------------+-+
+SubSampling  +2+
+-------------+-+
+Viewport     +4+
+-------------+-+
+FullMultiplex+7+
+-------------+-+


|enumeration-daBfg-MultiplexingMode|

.. _enum-daBfg-SideEffect:

.. das:attribute:: SideEffect

+--------+-+
+None    +0+
+--------+-+
+Internal+1+
+--------+-+
+External+2+
+--------+-+


|enumeration-daBfg-SideEffect|

.. _enum-daBfg-Access:

.. das:attribute:: Access

+----------+-+
+UNKNOWN   +0+
+----------+-+
+READ_ONLY +1+
+----------+-+
+READ_WRITE+2+
+----------+-+


|enumeration-daBfg-Access|

.. _enum-daBfg-Usage:

.. das:attribute:: Usage

+------------------------------------+--+
+UNKNOWN                             +0 +
+------------------------------------+--+
+COLOR_ATTACHMENT                    +1 +
+------------------------------------+--+
+INPUT_ATTACHMENT                    +2 +
+------------------------------------+--+
+DEPTH_ATTACHMENT                    +3 +
+------------------------------------+--+
+DEPTH_ATTACHMENT_AND_SHADER_RESOURCE+4 +
+------------------------------------+--+
+RESOLVE_ATTACHMENT                  +5 +
+------------------------------------+--+
+SHADER_RESOURCE                     +6 +
+------------------------------------+--+
+CONSTANT_BUFFER                     +7 +
+------------------------------------+--+
+INDEX_BUFFER                        +8 +
+------------------------------------+--+
+VERTEX_BUFFER                       +9 +
+------------------------------------+--+
+COPY                                +10+
+------------------------------------+--+
+BLIT                                +11+
+------------------------------------+--+
+INDIRECTION_BUFFER                  +12+
+------------------------------------+--+
+VRS_RATE_TEXTURE                    +13+
+------------------------------------+--+


|enumeration-daBfg-Usage|

.. _enum-daBfg-Stage:

.. das:attribute:: Stage

+---------------+--+
+UNKNOWN        +0 +
+---------------+--+
+PRE_RASTER     +1 +
+---------------+--+
+POST_RASTER    +2 +
+---------------+--+
+COMPUTE        +4 +
+---------------+--+
+TRANSFER       +8 +
+---------------+--+
+RAYTRACE       +16+
+---------------+--+
+ALL_GRAPHICS   +3 +
+---------------+--+
+ALL_INDIRECTION+21+
+---------------+--+


|enumeration-daBfg-Stage|

.. _enum-daBfg-ResourceType:

.. das:attribute:: ResourceType

+-------+-+
+Invalid+0+
+-------+-+
+Texture+1+
+-------+-+
+Buffer +2+
+-------+-+
+Blob   +3+
+-------+-+


|enumeration-daBfg-ResourceType|

.. _enum-daBfg-AutoResTypeNameId:

.. das:attribute:: AutoResTypeNameId

+-------+----------+
+Invalid+4294967295+
+-------+----------+


|enumeration-daBfg-AutoResTypeNameId|

.. _enum-daBfg-VariableRateShadingCombiner:

.. das:attribute:: VariableRateShadingCombiner

+---------------+-+
+VRS_PASSTHROUGH+0+
+---------------+-+
+VRS_OVERRIDE   +1+
+---------------+-+
+VRS_MIN        +2+
+---------------+-+
+VRS_MAX        +3+
+---------------+-+
+VRS_SUM        +4+
+---------------+-+


|enumeration-daBfg-VariableRateShadingCombiner|

.. _enum-daBfg-BindingType:

.. das:attribute:: BindingType

+----------+-+
+ShaderVar +0+
+----------+-+
+ViewMatrix+1+
+----------+-+
+ProjMatrix+2+
+----------+-+
+Invalid   +3+
+----------+-+


|enumeration-daBfg-BindingType|

.. _struct-daBfg-NameSpace:

.. das:attribute:: NameSpace



NameSpace fields are

+-----------+------------------------------------------------------------+
+nameSpaceId+ :ref:`daBfg::NameSpaceNameId <enum-daBfg-NameSpaceNameId>` +
+-----------+------------------------------------------------------------+


|structure-daBfg-NameSpace|

.. _struct-daBfg-ResUid:

.. das:attribute:: ResUid



ResUid fields are

+-------+------------------------------------------------+
+nameId + :ref:`daBfg::ResNameId <enum-daBfg-ResNameId>` +
+-------+------------------------------------------------+
+history+bool                                            +
+-------+------------------------------------------------+


|structure-daBfg-ResUid|

.. _struct-daBfg-VirtualResourceRequestBase:

.. das:attribute:: VirtualResourceRequestBase



VirtualResourceRequestBase fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+
+nodeId  + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualResourceRequestBase|

.. _struct-daBfg-VirtualResourceHandle:

.. das:attribute:: VirtualResourceHandle



VirtualResourceHandle fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualResourceHandle|

.. _struct-daBfg-VirtualResourceCreationSemiRequest:

.. das:attribute:: VirtualResourceCreationSemiRequest

 : VirtualResourceRequestBase

VirtualResourceCreationSemiRequest fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+
+nodeId  + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualResourceCreationSemiRequest|

.. _struct-daBfg-VirtualResourceSemiRequest:

.. das:attribute:: VirtualResourceSemiRequest

 : VirtualResourceRequestBase

VirtualResourceSemiRequest fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+
+nodeId  + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualResourceSemiRequest|

.. _struct-daBfg-VirtualResourceRequest:

.. das:attribute:: VirtualResourceRequest

 : VirtualResourceRequestBase

VirtualResourceRequest fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+
+nodeId  + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualResourceRequest|

.. _struct-daBfg-VirtualTextureHandle:

.. das:attribute:: VirtualTextureHandle

 : VirtualResourceHandle

VirtualTextureHandle fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualTextureHandle|

.. _struct-daBfg-VirtualBufferHandle:

.. das:attribute:: VirtualBufferHandle

 : VirtualResourceHandle

VirtualBufferHandle fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualBufferHandle|

.. _struct-daBfg-VirtualTextureRequest:

.. das:attribute:: VirtualTextureRequest

 : VirtualResourceRequest

VirtualTextureRequest fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+
+nodeId  + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualTextureRequest|

.. _struct-daBfg-VirtualBufferRequest:

.. das:attribute:: VirtualBufferRequest

 : VirtualResourceRequest

VirtualBufferRequest fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+resUid  + :ref:`daBfg::ResUid <struct-daBfg-ResUid>`                      +
+--------+-----------------------------------------------------------------+
+nodeId  + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+--------+-----------------------------------------------------------------+


|structure-daBfg-VirtualBufferRequest|

.. _struct-daBfg-StateRequest:

.. das:attribute:: StateRequest



StateRequest fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+nodeId  + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+--------+-----------------------------------------------------------------+


|structure-daBfg-StateRequest|

.. _struct-daBfg-VrsRequirements:

.. das:attribute:: VrsRequirements



VrsRequirements fields are

+--------------+------------------------------------------------------------------------------------+
+rateX         +uint                                                                                +
+--------------+------------------------------------------------------------------------------------+
+rateY         +uint                                                                                +
+--------------+------------------------------------------------------------------------------------+
+rateTexture   + :ref:`VrsRateTexture <alias-VrsRateTexture>`                                       +
+--------------+------------------------------------------------------------------------------------+
+vertexCombiner+ :ref:`daBfg::VariableRateShadingCombiner <enum-daBfg-VariableRateShadingCombiner>` +
+--------------+------------------------------------------------------------------------------------+
+pixelCombiner + :ref:`daBfg::VariableRateShadingCombiner <enum-daBfg-VariableRateShadingCombiner>` +
+--------------+------------------------------------------------------------------------------------+


|structure-daBfg-VrsRequirements|

.. _struct-daBfg-VirtualPassRequest:

.. das:attribute:: VirtualPassRequest



VirtualPassRequest fields are

+--------+-----------------------------------------------------------------+
+registry+ :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+--------+-----------------------------------------------------------------+
+nodeId  + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+--------+-----------------------------------------------------------------+


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

+-------------+-----------------------------------------------------------------+
+autoResTypeId+ :ref:`daBfg::AutoResTypeNameId <enum-daBfg-AutoResTypeNameId>`  +
+-------------+-----------------------------------------------------------------+
+multiplier   +float                                                            +
+-------------+-----------------------------------------------------------------+
+registry     + :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+-------------+-----------------------------------------------------------------+


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

++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-daBfg-TextureResourceDescription:

.. das:attribute:: TextureResourceDescription

TextureResourceDescription fields are

+----------+------------------------------------------------------------------------------+
+height    +uint                                                                          +
+----------+------------------------------------------------------------------------------+
+mipLevels +uint                                                                          +
+----------+------------------------------------------------------------------------------+
+activation+ :ref:`daBfg::ResourceActivationAction <enum-daBfg-ResourceActivationAction>` +
+----------+------------------------------------------------------------------------------+
+width     +uint                                                                          +
+----------+------------------------------------------------------------------------------+
+cFlags    +uint                                                                          +
+----------+------------------------------------------------------------------------------+


|structure_annotation-daBfg-TextureResourceDescription|

.. _handle-daBfg-VolTextureResourceDescription:

.. das:attribute:: VolTextureResourceDescription

|structure_annotation-daBfg-VolTextureResourceDescription|

.. _handle-daBfg-ArrayTextureResourceDescription:

.. das:attribute:: ArrayTextureResourceDescription

|structure_annotation-daBfg-ArrayTextureResourceDescription|

.. _handle-daBfg-CubeTextureResourceDescription:

.. das:attribute:: CubeTextureResourceDescription

|structure_annotation-daBfg-CubeTextureResourceDescription|

.. _handle-daBfg-ArrayCubeTextureResourceDescription:

.. das:attribute:: ArrayCubeTextureResourceDescription

|structure_annotation-daBfg-ArrayCubeTextureResourceDescription|

.. _handle-daBfg-ResourceData:

.. das:attribute:: ResourceData

ResourceData fields are

+-------+------------------------------------------------------+
+resType+ :ref:`daBfg::ResourceType <enum-daBfg-ResourceType>` +
+-------+------------------------------------------------------+
+history+ :ref:`daBfg::History <enum-daBfg-History>`           +
+-------+------------------------------------------------------+


|structure_annotation-daBfg-ResourceData|

.. _handle-daBfg-AutoResolutionData:

.. das:attribute:: AutoResolutionData

AutoResolutionData fields are

+----------+----------------------------------------------------------------+
+multiplier+float                                                           +
+----------+----------------------------------------------------------------+
+id        + :ref:`daBfg::AutoResTypeNameId <enum-daBfg-AutoResTypeNameId>` +
+----------+----------------------------------------------------------------+


|structure_annotation-daBfg-AutoResolutionData|

.. _handle-daBfg-ShaderBlockLayersInfo:

.. das:attribute:: ShaderBlockLayersInfo

ShaderBlockLayersInfo fields are

+-----------+---+
+sceneLayer +int+
+-----------+---+
+objectLayer+int+
+-----------+---+
+frameLayer +int+
+-----------+---+


|structure_annotation-daBfg-ShaderBlockLayersInfo|

.. _handle-daBfg-VrsStateRequirements:

.. das:attribute:: VrsStateRequirements

VrsStateRequirements fields are

+----------------+------------------------------------------------------------------------------------+
+rateTextureResId+ :ref:`daBfg::ResNameId <enum-daBfg-ResNameId>`                                     +
+----------------+------------------------------------------------------------------------------------+
+pixelCombiner   + :ref:`daBfg::VariableRateShadingCombiner <enum-daBfg-VariableRateShadingCombiner>` +
+----------------+------------------------------------------------------------------------------------+
+rateY           +uint                                                                                +
+----------------+------------------------------------------------------------------------------------+
+vertexCombiner  + :ref:`daBfg::VariableRateShadingCombiner <enum-daBfg-VariableRateShadingCombiner>` +
+----------------+------------------------------------------------------------------------------------+
+rateX           +uint                                                                                +
+----------------+------------------------------------------------------------------------------------+


|structure_annotation-daBfg-VrsStateRequirements|

.. _handle-daBfg-VirtualSubresourceRef:

.. das:attribute:: VirtualSubresourceRef

VirtualSubresourceRef fields are

+--------+------------------------------------------------+
+layer   +uint                                            +
+--------+------------------------------------------------+
+nameId  + :ref:`daBfg::ResNameId <enum-daBfg-ResNameId>` +
+--------+------------------------------------------------+
+mipLevel+uint                                            +
+--------+------------------------------------------------+


|structure_annotation-daBfg-VirtualSubresourceRef|

.. _handle-daBfg-Binding:

.. das:attribute:: Binding

Binding fields are

+--------+----------------------------------------------------+
+bindType+ :ref:`daBfg::BindingType <enum-daBfg-BindingType>` +
+--------+----------------------------------------------------+
+resource+ :ref:`daBfg::ResNameId <enum-daBfg-ResNameId>`     +
+--------+----------------------------------------------------+
+history +bool                                                +
+--------+----------------------------------------------------+


|structure_annotation-daBfg-Binding|

.. _handle-daBfg-ResourceUsage:

.. das:attribute:: ResourceUsage

ResourceUsage fields are

+---------+------------------------------------------+
+stage    + :ref:`daBfg::Stage <enum-daBfg-Stage>`   +
+---------+------------------------------------------+
+usageType+ :ref:`daBfg::Usage <enum-daBfg-Usage>`   +
+---------+------------------------------------------+
+access   + :ref:`daBfg::Access <enum-daBfg-Access>` +
+---------+------------------------------------------+


|structure_annotation-daBfg-ResourceUsage|

.. _handle-daBfg-ResourceRequest:

.. das:attribute:: ResourceRequest

ResourceRequest fields are

+-----------+----------------------------------------------------------+
+usage      + :ref:`daBfg::ResourceUsage <handle-daBfg-ResourceUsage>` +
+-----------+----------------------------------------------------------+
+slotRequest+bool                                                      +
+-----------+----------------------------------------------------------+
+optional   +bool                                                      +
+-----------+----------------------------------------------------------+


|structure_annotation-daBfg-ResourceRequest|

.. _handle-daBfg-BufferResourceDescription:

.. das:attribute:: BufferResourceDescription

BufferResourceDescription fields are

+------------------+------------------------------------------------------------------------------+
+viewFormat        +uint                                                                          +
+------------------+------------------------------------------------------------------------------+
+activation        + :ref:`daBfg::ResourceActivationAction <enum-daBfg-ResourceActivationAction>` +
+------------------+------------------------------------------------------------------------------+
+elementCount      +uint                                                                          +
+------------------+------------------------------------------------------------------------------+
+cFlags            +uint                                                                          +
+------------------+------------------------------------------------------------------------------+
+elementSizeInBytes+uint                                                                          +
+------------------+------------------------------------------------------------------------------+


|structure_annotation-daBfg-BufferResourceDescription|

.. _handle-daBfg-NodeStateRequirements:

.. das:attribute:: NodeStateRequirements

NodeStateRequirements fields are

+---------------------+----------------------------------------------------------------------------------------------+
+supportsWireframe    +bool                                                                                          +
+---------------------+----------------------------------------------------------------------------------------------+
+pipelineStateOverride+ :ref:`builtin::optional`OverrideState <handle-builtin-optional`OverrideState>`               +
+---------------------+----------------------------------------------------------------------------------------------+
+vrsState             + :ref:`builtin::optional`VrsStateRequirements <handle-builtin-optional`VrsStateRequirements>` +
+---------------------+----------------------------------------------------------------------------------------------+


|structure_annotation-daBfg-NodeStateRequirements|

.. _handle-daBfg-VirtualPassRequirements:

.. das:attribute:: VirtualPassRequirements

VirtualPassRequirements fields are

+----------------+----------------------------------------------------------------------------------------------------------+
+colorAttachments+ :ref:`builtin::fixedVector`VirtualSubresourceRef`8 <handle-builtin-fixedVector`VirtualSubresourceRef`8>` +
+----------------+----------------------------------------------------------------------------------------------------------+
+depthReadOnly   +bool                                                                                                      +
+----------------+----------------------------------------------------------------------------------------------------------+
+depthAttachment + :ref:`daBfg::VirtualSubresourceRef <handle-daBfg-VirtualSubresourceRef>`                                 +
+----------------+----------------------------------------------------------------------------------------------------------+


|structure_annotation-daBfg-VirtualPassRequirements|

.. _handle-daBfg-NodeData:

.. das:attribute:: NodeData

NodeData fields are

+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+multiplexingMode           + :ref:`daBfg::MultiplexingMode <enum-daBfg-MultiplexingMode>`                                                             +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+generation                 +uint                                                                                                                      +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+precedingNodeIds           + :ref:`builtin::fixedVectorSet`NodeNameId`4 <handle-builtin-fixedVectorSet`NodeNameId`4>`                                 +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+modifiedResources          + :ref:`builtin::fixedVectorSet`ResNameId`8 <handle-builtin-fixedVectorSet`ResNameId`8>`                                   +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+followingNodeIds           + :ref:`builtin::fixedVectorSet`NodeNameId`4 <handle-builtin-fixedVectorSet`NodeNameId`4>`                                 +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+resourceRequests           + :ref:`builtin::fixedVectorMap`ResNameId`ResourceRequest`16 <handle-builtin-fixedVectorMap`ResNameId`ResourceRequest`16>` +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+readResources              + :ref:`builtin::fixedVectorSet`ResNameId`8 <handle-builtin-fixedVectorSet`ResNameId`8>`                                   +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+nodeSource                 + :ref:`builtin::das_string <handle-builtin-das_string>`                                                                   +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+shaderBlockLayers          + :ref:`daBfg::ShaderBlockLayersInfo <handle-daBfg-ShaderBlockLayersInfo>`                                                 +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+bindings                   + :ref:`builtin::fixedVectorMap`int`Binding`8 <handle-builtin-fixedVectorMap`int`Binding`8>`                               +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+historyResourceReadRequests+ :ref:`builtin::fixedVectorMap`ResNameId`ResourceRequest`16 <handle-builtin-fixedVectorMap`ResNameId`ResourceRequest`16>` +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+stateRequirements          + :ref:`builtin::optional`NodeStateRequirements <handle-builtin-optional`NodeStateRequirements>`                           +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+createdResources           + :ref:`builtin::fixedVectorSet`ResNameId`8 <handle-builtin-fixedVectorSet`ResNameId`8>`                                   +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+renderingRequirements      + :ref:`builtin::optional`VirtualPassRequirements <handle-builtin-optional`VirtualPassRequirements>`                       +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+renamedResources           + :ref:`builtin::fixedVectorMap`ResNameId`ResNameId`8 <handle-builtin-fixedVectorMap`ResNameId`ResNameId`8>`               +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+priority                   +int                                                                                                                       +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+sideEffect                 + :ref:`daBfg::SideEffect <enum-daBfg-SideEffect>`                                                                         +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+


|structure_annotation-daBfg-NodeData|

.. _handle-daBfg-ResourceProvider:

.. das:attribute:: ResourceProvider

|structure_annotation-daBfg-ResourceProvider|

.. _handle-daBfg-InternalRegistry:

.. das:attribute:: InternalRegistry

InternalRegistry fields are

+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+resources + :ref:`builtin::idIndexedMapping`ResNameId`ResourceData <handle-builtin-idIndexedMapping`ResNameId`ResourceData>`                                                   +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+nodes     + :ref:`builtin::idIndexedMapping`NodeNameId`NodeData <handle-builtin-idIndexedMapping`NodeNameId`NodeData>`                                                         +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+knownNames+ :ref:`builtin::idNameMap`NameSpaceNameId`ResNameId`NodeNameId`AutoResTypeNameId <handle-builtin-idNameMap`NameSpaceNameId`ResNameId`NodeNameId`AutoResTypeNameId>` +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+


|structure_annotation-daBfg-InternalRegistry|

.. _handle-daBfg-NameSpaceRequest:

.. das:attribute:: NameSpaceRequest

NameSpaceRequest fields are

+-----------+-----------------------------------------------------------------+
+registry   + :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+-----------+-----------------------------------------------------------------+
+nameSpaceId+ :ref:`daBfg::NameSpaceNameId <enum-daBfg-NameSpaceNameId>`      +
+-----------+-----------------------------------------------------------------+
+nodeId     + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+-----------+-----------------------------------------------------------------+


|structure_annotation-daBfg-NameSpaceRequest|

.. _handle-daBfg-Registry:

.. das:attribute:: Registry

Registry fields are

+-----------+-----------------------------------------------------------------+
+registry   + :ref:`daBfg::InternalRegistry <handle-daBfg-InternalRegistry>` ?+
+-----------+-----------------------------------------------------------------+
+nameSpaceId+ :ref:`daBfg::NameSpaceNameId <enum-daBfg-NameSpaceNameId>`      +
+-----------+-----------------------------------------------------------------+
+nodeId     + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`                +
+-----------+-----------------------------------------------------------------+


|structure_annotation-daBfg-Registry|

.. _handle-daBfg-NodeTracker:

.. das:attribute:: NodeTracker

|structure_annotation-daBfg-NodeTracker|

.. _handle-daBfg-NodeHandle:

.. das:attribute:: NodeHandle

|structure_annotation-daBfg-NodeHandle|

++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-daBfg-bfg_ecs_node:

.. das:attribute:: bfg_ecs_node

This annotation reloads nodes on the fly after script reloading.
It is designed for functions accepting temp value or ref to :ref:`daBfg::NodeHandle <handle-daBfg-NodeHandle>`, acquired from ecs system.
Annotation accepts three parameters:

+------+----------------------------------------------------------------+
+name  +string name of registred node inside annotated function         +
+------+----------------------------------------------------------------+
+entity+string name of entity in which node is stored                   +
+------+----------------------------------------------------------------+
+handle+string name of component of type :cpp:class:`dabfg::NodeHandle` +
+------+----------------------------------------------------------------+

+++++++++++++++++++
Top level functions
+++++++++++++++++++

  *  :ref:`registerNode (arg0:daBfg::NodeTracker implicit;arg1:daBfg::NodeNameId const) : void <function-_at_daBfg_c__c_registerNode_IH_ls_daBfg_c__c_NodeTracker_gr__CE_ls_daBfg_c__c_NodeNameId_gr_>` 
  *  :ref:`get_shader_variable_id (arg0:string const implicit) : int <function-_at_daBfg_c__c_get_shader_variable_id_CIs>` 
  *  :ref:`resetNode (arg0:daBfg::NodeHandle implicit) : void <function-_at_daBfg_c__c_resetNode_IH_ls_daBfg_c__c_NodeHandle_gr_>` 
  *  :ref:`root () : daBfg::NameSpace <function-_at_daBfg_c__c_root>` 
  *  :ref:`fillSlot (self:daBfg::NameSpace -const;slot:daBfg::NamedSlot const;res_name_space:daBfg::NameSpace const;res_name:string const) : auto <function-_at_daBfg_c__c_fillSlot_S_ls_daBfg_c__c_NameSpace_gr__CS_ls_daBfg_c__c_NamedSlot_gr__CS_ls_daBfg_c__c_NameSpace_gr__Cs>` 
  *  :ref:`registerNode (self:daBfg::NameSpace -const;name:string const;declaration_callback:lambda\<(var reg:daBfg::Registry -const):lambda\<void\>\> -const) : daBfg::NodeHandle <function-_at_daBfg_c__c_registerNode_S_ls_daBfg_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_H_ls_daBfg_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_>` 

.. _function-_at_daBfg_c__c_registerNode_IH_ls_daBfg_c__c_NodeTracker_gr__CE_ls_daBfg_c__c_NodeNameId_gr_:

.. das:function:: registerNode(arg0: NodeTracker implicit; arg1: NodeNameId const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+arg0    + :ref:`daBfg::NodeTracker <handle-daBfg-NodeTracker>`  implicit+
+--------+---------------------------------------------------------------+
+arg1    + :ref:`daBfg::NodeNameId <enum-daBfg-NodeNameId>`  const       +
+--------+---------------------------------------------------------------+


|function-daBfg-registerNode|

.. _function-_at_daBfg_c__c_get_shader_variable_id_CIs:

.. das:function:: get_shader_variable_id(arg0: string const implicit)

get_shader_variable_id returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+arg0    +string const implicit+
+--------+---------------------+


|function-daBfg-get_shader_variable_id|

.. _function-_at_daBfg_c__c_resetNode_IH_ls_daBfg_c__c_NodeHandle_gr_:

.. das:function:: resetNode(arg0: NodeHandle implicit)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+arg0    + :ref:`daBfg::NodeHandle <handle-daBfg-NodeHandle>`  implicit+
+--------+-------------------------------------------------------------+


|function-daBfg-resetNode|

.. _function-_at_daBfg_c__c_root:

.. das:function:: root()

root returns  :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>` 

|function-daBfg-root|

.. _function-_at_daBfg_c__c_fillSlot_S_ls_daBfg_c__c_NameSpace_gr__CS_ls_daBfg_c__c_NamedSlot_gr__CS_ls_daBfg_c__c_NameSpace_gr__Cs:

.. das:function:: fillSlot(self: NameSpace; slot: NamedSlot const; res_name_space: NameSpace const; res_name: string const)

fillSlot returns auto

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

.. _function-_at_daBfg_c__c_registerNode_S_ls_daBfg_c__c_NameSpace_gr__Cs_N_ls_reg_gr_0_ls_H_ls_daBfg_c__c_Registry_gr__gr_1_ls_1_ls_v_gr__at__gr__at_:

.. das:function:: registerNode(self: NameSpace; name: string const; declaration_callback: lambda<(var reg:daBfg::Registry -const):lambda<void>>)

registerNode returns  :ref:`daBfg::NodeHandle <handle-daBfg-NodeHandle>` 

+--------------------+-----------------------------------------------------------------------+
+argument            +argument type                                                          +
+====================+=======================================================================+
+self                + :ref:`daBfg::NameSpace <struct-daBfg-NameSpace>`                      +
+--------------------+-----------------------------------------------------------------------+
+name                +string const                                                           +
+--------------------+-----------------------------------------------------------------------+
+declaration_callback+lambda<(reg: :ref:`daBfg::Registry <handle-daBfg-Registry>` ):lambda<>>+
+--------------------+-----------------------------------------------------------------------+


|function-daBfg-registerNode|

+++++++++++++++++++++
Registry manipulation
+++++++++++++++++++++

  *  :ref:`orderMeBefore (self:daBfg::Registry -const;name:string const) : daBfg::Registry <function-_at_daBfg_c__c_orderMeBefore_H_ls_daBfg_c__c_Registry_gr__Cs>` 
  *  :ref:`orderMeBefore (self:daBfg::Registry -const;names:array\<string\> const) : daBfg::Registry <function-_at_daBfg_c__c_orderMeBefore_H_ls_daBfg_c__c_Registry_gr__C1_ls_s_gr_A>` 
  *  :ref:`orderMeAfter (self:daBfg::Registry -const;name:string const) : daBfg::Registry <function-_at_daBfg_c__c_orderMeAfter_H_ls_daBfg_c__c_Registry_gr__Cs>` 
  *  :ref:`orderMeAfter (self:daBfg::Registry -const;names:array\<string\> const) : daBfg::Registry <function-_at_daBfg_c__c_orderMeAfter_H_ls_daBfg_c__c_Registry_gr__C1_ls_s_gr_A>` 
  *  :ref:`setPriority (self:daBfg::Registry -const;priority:int const) : daBfg::Registry <function-_at_daBfg_c__c_setPriority_H_ls_daBfg_c__c_Registry_gr__Ci>` 
  *  :ref:`multiplex (self:daBfg::Registry -const;multiplexing_mode:daBfg::MultiplexingMode const) : daBfg::Registry <function-_at_daBfg_c__c_multiplex_H_ls_daBfg_c__c_Registry_gr__CE_ls_daBfg_c__c_MultiplexingMode_gr_>` 
  *  :ref:`executionHas (self:daBfg::Registry -const;side_effect:daBfg::SideEffect const) : daBfg::Registry <function-_at_daBfg_c__c_executionHas_H_ls_daBfg_c__c_Registry_gr__CE8_ls_daBfg_c__c_SideEffect_gr_>` 
  *  :ref:`create (self:daBfg::Registry -const;name:string const;history:daBfg::History const) : daBfg::VirtualResourceCreationSemiRequest <function-_at_daBfg_c__c_create_H_ls_daBfg_c__c_Registry_gr__Cs_CE8_ls_daBfg_c__c_History_gr_>` 
  *  :ref:`read (self:daBfg::NameSpaceRequest -const;name:string const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_read_H_ls_daBfg_c__c_NameSpaceRequest_gr__Cs>` 
  *  :ref:`read (self:daBfg::NameSpaceRequest -const;slot:daBfg::NamedSlot const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_read_H_ls_daBfg_c__c_NameSpaceRequest_gr__CS_ls_daBfg_c__c_NamedSlot_gr_>` 
  *  :ref:`history (self:daBfg::NameSpaceRequest -const;name:string -const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_history_H_ls_daBfg_c__c_NameSpaceRequest_gr__s>` 
  *  :ref:`modify (self:daBfg::NameSpaceRequest -const;name:string const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_modify_H_ls_daBfg_c__c_NameSpaceRequest_gr__Cs>` 
  *  :ref:`modify (self:daBfg::NameSpaceRequest -const;slot:daBfg::NamedSlot const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_modify_H_ls_daBfg_c__c_NameSpaceRequest_gr__CS_ls_daBfg_c__c_NamedSlot_gr_>` 
  *  :ref:`rename (self:daBfg::NameSpaceRequest -const;from:string const;to:string const;history:daBfg::History const) : daBfg::VirtualResourceSemiRequest <function-_at_daBfg_c__c_rename_H_ls_daBfg_c__c_NameSpaceRequest_gr__Cs_Cs_CE8_ls_daBfg_c__c_History_gr_>` 
  *  :ref:`requestState (self:daBfg::Registry -const) : daBfg::StateRequest <function-_at_daBfg_c__c_requestState_H_ls_daBfg_c__c_Registry_gr_>` 
  *  :ref:`requestRenderPass (self:daBfg::Registry -const) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_requestRenderPass_H_ls_daBfg_c__c_Registry_gr_>` 

.. _function-_at_daBfg_c__c_orderMeBefore_H_ls_daBfg_c__c_Registry_gr__Cs:

.. das:function:: orderMeBefore(self: Registry; name: string const)

orderMeBefore returns  :ref:`daBfg::Registry <handle-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <handle-daBfg-Registry>` +
+--------+------------------------------------------------+
+name    +string const                                    +
+--------+------------------------------------------------+


|function-daBfg-orderMeBefore|

.. _function-_at_daBfg_c__c_orderMeBefore_H_ls_daBfg_c__c_Registry_gr__C1_ls_s_gr_A:

.. das:function:: orderMeBefore(self: Registry; names: array<string> const)

orderMeBefore returns  :ref:`daBfg::Registry <handle-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <handle-daBfg-Registry>` +
+--------+------------------------------------------------+
+names   +array<string> const                             +
+--------+------------------------------------------------+


|function-daBfg-orderMeBefore|

.. _function-_at_daBfg_c__c_orderMeAfter_H_ls_daBfg_c__c_Registry_gr__Cs:

.. das:function:: orderMeAfter(self: Registry; name: string const)

orderMeAfter returns  :ref:`daBfg::Registry <handle-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <handle-daBfg-Registry>` +
+--------+------------------------------------------------+
+name    +string const                                    +
+--------+------------------------------------------------+


|function-daBfg-orderMeAfter|

.. _function-_at_daBfg_c__c_orderMeAfter_H_ls_daBfg_c__c_Registry_gr__C1_ls_s_gr_A:

.. das:function:: orderMeAfter(self: Registry; names: array<string> const)

orderMeAfter returns  :ref:`daBfg::Registry <handle-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <handle-daBfg-Registry>` +
+--------+------------------------------------------------+
+names   +array<string> const                             +
+--------+------------------------------------------------+


|function-daBfg-orderMeAfter|

.. _function-_at_daBfg_c__c_setPriority_H_ls_daBfg_c__c_Registry_gr__Ci:

.. das:function:: setPriority(self: Registry; priority: int const)

setPriority returns  :ref:`daBfg::Registry <handle-daBfg-Registry>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <handle-daBfg-Registry>` +
+--------+------------------------------------------------+
+priority+int const                                       +
+--------+------------------------------------------------+


|function-daBfg-setPriority|

.. _function-_at_daBfg_c__c_multiplex_H_ls_daBfg_c__c_Registry_gr__CE_ls_daBfg_c__c_MultiplexingMode_gr_:

.. das:function:: multiplex(self: Registry; multiplexing_mode: MultiplexingMode const)

multiplex returns  :ref:`daBfg::Registry <handle-daBfg-Registry>` 

+-----------------+--------------------------------------------------------------------+
+argument         +argument type                                                       +
+=================+====================================================================+
+self             + :ref:`daBfg::Registry <handle-daBfg-Registry>`                     +
+-----------------+--------------------------------------------------------------------+
+multiplexing_mode+ :ref:`daBfg::MultiplexingMode <enum-daBfg-MultiplexingMode>`  const+
+-----------------+--------------------------------------------------------------------+


|function-daBfg-multiplex|

.. _function-_at_daBfg_c__c_executionHas_H_ls_daBfg_c__c_Registry_gr__CE8_ls_daBfg_c__c_SideEffect_gr_:

.. das:function:: executionHas(self: Registry; side_effect: SideEffect const)

executionHas returns  :ref:`daBfg::Registry <handle-daBfg-Registry>` 

+-----------+--------------------------------------------------------+
+argument   +argument type                                           +
+===========+========================================================+
+self       + :ref:`daBfg::Registry <handle-daBfg-Registry>`         +
+-----------+--------------------------------------------------------+
+side_effect+ :ref:`daBfg::SideEffect <enum-daBfg-SideEffect>`  const+
+-----------+--------------------------------------------------------+


|function-daBfg-executionHas|

.. _function-_at_daBfg_c__c_create_H_ls_daBfg_c__c_Registry_gr__Cs_CE8_ls_daBfg_c__c_History_gr_:

.. das:function:: create(self: Registry; name: string const; history: History const)

create returns  :ref:`daBfg::VirtualResourceCreationSemiRequest <struct-daBfg-VirtualResourceCreationSemiRequest>` 

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+self    + :ref:`daBfg::Registry <handle-daBfg-Registry>`   +
+--------+--------------------------------------------------+
+name    +string const                                      +
+--------+--------------------------------------------------+
+history + :ref:`daBfg::History <enum-daBfg-History>`  const+
+--------+--------------------------------------------------+


|function-daBfg-create|

.. _function-_at_daBfg_c__c_read_H_ls_daBfg_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: read(self: NameSpaceRequest; name: string const)

read returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+name    +string const                                                    +
+--------+----------------------------------------------------------------+


|function-daBfg-read|

.. _function-_at_daBfg_c__c_read_H_ls_daBfg_c__c_NameSpaceRequest_gr__CS_ls_daBfg_c__c_NamedSlot_gr_:

.. das:function:: read(self: NameSpaceRequest; slot: NamedSlot const)

read returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+slot    + :ref:`daBfg::NamedSlot <struct-daBfg-NamedSlot>`  const        +
+--------+----------------------------------------------------------------+


|function-daBfg-read|

.. _function-_at_daBfg_c__c_history_H_ls_daBfg_c__c_NameSpaceRequest_gr__s:

.. das:function:: history(self: NameSpaceRequest; name: string)

history returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+name    +string                                                          +
+--------+----------------------------------------------------------------+


|function-daBfg-history|

.. _function-_at_daBfg_c__c_modify_H_ls_daBfg_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: modify(self: NameSpaceRequest; name: string const)

modify returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+name    +string const                                                    +
+--------+----------------------------------------------------------------+


|function-daBfg-modify|

.. _function-_at_daBfg_c__c_modify_H_ls_daBfg_c__c_NameSpaceRequest_gr__CS_ls_daBfg_c__c_NamedSlot_gr_:

.. das:function:: modify(self: NameSpaceRequest; slot: NamedSlot const)

modify returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+slot    + :ref:`daBfg::NamedSlot <struct-daBfg-NamedSlot>`  const        +
+--------+----------------------------------------------------------------+


|function-daBfg-modify|

.. _function-_at_daBfg_c__c_rename_H_ls_daBfg_c__c_NameSpaceRequest_gr__Cs_Cs_CE8_ls_daBfg_c__c_History_gr_:

.. das:function:: rename(self: NameSpaceRequest; from: string const; to: string const; history: History const)

rename returns  :ref:`daBfg::VirtualResourceSemiRequest <struct-daBfg-VirtualResourceSemiRequest>` 

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+self    + :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` +
+--------+----------------------------------------------------------------+
+from    +string const                                                    +
+--------+----------------------------------------------------------------+
+to      +string const                                                    +
+--------+----------------------------------------------------------------+
+history + :ref:`daBfg::History <enum-daBfg-History>`  const              +
+--------+----------------------------------------------------------------+


|function-daBfg-rename|

.. _function-_at_daBfg_c__c_requestState_H_ls_daBfg_c__c_Registry_gr_:

.. das:function:: requestState(self: Registry)

requestState returns  :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <handle-daBfg-Registry>` +
+--------+------------------------------------------------+


|function-daBfg-requestState|

.. _function-_at_daBfg_c__c_requestRenderPass_H_ls_daBfg_c__c_Registry_gr_:

.. das:function:: requestRenderPass(self: Registry)

requestRenderPass returns  :ref:`daBfg::VirtualPassRequest <struct-daBfg-VirtualPassRequest>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`daBfg::Registry <handle-daBfg-Registry>` +
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
  *  :ref:`useAs (self:auto(TT) -const;usageType:daBfg::Usage const) : TT <function-_at_daBfg_c__c_useAs_Y_ls_TT_gr_._CE_ls_daBfg_c__c_Usage_gr_>` 
  *  :ref:`atStage (self:auto(TT) -const;stage:daBfg::Stage const) : TT <function-_at_daBfg_c__c_atStage_Y_ls_TT_gr_._CE_ls_daBfg_c__c_Stage_gr_>` 
  *  :ref:`bindToShaderVar (self:auto(TT) -const;name:string const) : TT <function-_at_daBfg_c__c_bindToShaderVar_Y_ls_TT_gr_._Cs>` 
  *  :ref:`handle (self:daBfg::VirtualTextureRequest const) : daBfg::VirtualTextureHandle <function-_at_daBfg_c__c_handle_CS_ls_daBfg_c__c_VirtualTextureRequest_gr_>` 
  *  :ref:`handle (self:daBfg::VirtualBufferRequest const) : daBfg::VirtualBufferHandle <function-_at_daBfg_c__c_handle_CS_ls_daBfg_c__c_VirtualBufferRequest_gr_>` 
  *  :ref:`view (handle:daBfg::VirtualTextureHandle const) : DagorResPtr::ManagedTexView <function-_at_daBfg_c__c_view_CS_ls_daBfg_c__c_VirtualTextureHandle_gr_>` 
  *  :ref:`view (handle:daBfg::VirtualBufferHandle const) : DagorResPtr::ManagedBufView <function-_at_daBfg_c__c_view_CS_ls_daBfg_c__c_VirtualBufferHandle_gr_>` 
  *  :ref:`setFrameBlock (self:daBfg::StateRequest -const;name:string const) : daBfg::StateRequest <function-_at_daBfg_c__c_setFrameBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs>` 
  *  :ref:`setSceneBlock (self:daBfg::StateRequest -const;name:string const) : daBfg::StateRequest <function-_at_daBfg_c__c_setSceneBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs>` 
  *  :ref:`setObjectBlock (self:daBfg::StateRequest -const;name:string const) : daBfg::StateRequest <function-_at_daBfg_c__c_setObjectBlock_S_ls_daBfg_c__c_StateRequest_gr__Cs>` 
  *  :ref:`allowWireFrame (self:daBfg::StateRequest -const) : daBfg::StateRequest <function-_at_daBfg_c__c_allowWireFrame_S_ls_daBfg_c__c_StateRequest_gr_>` 
  *  :ref:`allowVrs (self:daBfg::StateRequest -const;vrs:daBfg::VrsRequirements const) : daBfg::StateRequest <function-_at_daBfg_c__c_allowVrs_S_ls_daBfg_c__c_StateRequest_gr__CS_ls_daBfg_c__c_VrsRequirements_gr_>` 
  *  :ref:`enableOverride (self:daBfg::StateRequest -const;das_override:DagorDriver3D::OverrideRenderState const) : auto <function-_at_daBfg_c__c_enableOverride_S_ls_daBfg_c__c_StateRequest_gr__CS_ls_DagorDriver3D_c__c_OverrideRenderState_gr_>` 
  *  :ref:`color (self:daBfg::VirtualPassRequest -const;requests:daBfg::VirtualTextureRequest const[]) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C[-1]S_ls_daBfg_c__c_VirtualTextureRequest_gr_>` 
  *  :ref:`color (self:daBfg::VirtualPassRequest -const;names:string const[]) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C[-1]s>` 
  *  :ref:`color (self:daBfg::VirtualPassRequest -const;attachments:array\<daBfg::VirtualAttachmentRequest\> const) : daBfg::VirtualPassRequest <function-_at_daBfg_c__c_color_S_ls_daBfg_c__c_VirtualPassRequest_gr__C1_ls_S_ls_daBfg_c__c_VirtualAttachmentRequest_gr__gr_A>` 
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

.. _function-_at_daBfg_c__c_useAs_Y_ls_TT_gr_._CE_ls_daBfg_c__c_Usage_gr_:

.. das:function:: useAs(self: auto(TT); usageType: Usage const)

useAs returns TT

+---------+----------------------------------------------+
+argument +argument type                                 +
+=========+==============================================+
+self     +auto(TT)                                      +
+---------+----------------------------------------------+
+usageType+ :ref:`daBfg::Usage <enum-daBfg-Usage>`  const+
+---------+----------------------------------------------+


|function-daBfg-useAs|

.. _function-_at_daBfg_c__c_atStage_Y_ls_TT_gr_._CE_ls_daBfg_c__c_Stage_gr_:

.. das:function:: atStage(self: auto(TT); stage: Stage const)

atStage returns TT

+--------+----------------------------------------------+
+argument+argument type                                 +
+========+==============================================+
+self    +auto(TT)                                      +
+--------+----------------------------------------------+
+stage   + :ref:`daBfg::Stage <enum-daBfg-Stage>`  const+
+--------+----------------------------------------------+


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

enableOverride returns auto

+------------+--------------------------------------------------------------------------------------------+
+argument    +argument type                                                                               +
+============+============================================================================================+
+self        + :ref:`daBfg::StateRequest <struct-daBfg-StateRequest>`                                     +
+------------+--------------------------------------------------------------------------------------------+
+das_override+ :ref:`DagorDriver3D::OverrideRenderState <struct-DagorDriver3D-OverrideRenderState>`  const+
+------------+--------------------------------------------------------------------------------------------+


|function-daBfg-enableOverride|

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

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_daBfg_c__c_fill_slot_CE_ls_daBfg_c__c_NameSpaceNameId_gr__CIs_CE_ls_daBfg_c__c_NameSpaceNameId_gr__CIs:

.. das:function:: fill_slot(arg0: NameSpaceNameId const; arg1: string const implicit; arg2: NameSpaceNameId const; arg3: string const implicit)

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+arg0    + :ref:`daBfg::NameSpaceNameId <enum-daBfg-NameSpaceNameId>`  const+
+--------+------------------------------------------------------------------+
+arg1    +string const implicit                                             +
+--------+------------------------------------------------------------------+
+arg2    + :ref:`daBfg::NameSpaceNameId <enum-daBfg-NameSpaceNameId>`  const+
+--------+------------------------------------------------------------------+
+arg3    +string const implicit                                             +
+--------+------------------------------------------------------------------+


|function-daBfg-fill_slot|

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

.. _function-_at_daBfg_c__c_/_H_ls_daBfg_c__c_NameSpaceRequest_gr__Cs:

.. das:function:: operator /(self: NameSpaceRequest; child_name: string const)

/ returns  :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` 

+----------+----------------------------------------------------------------+
+argument  +argument type                                                   +
+==========+================================================================+
+self      + :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` +
+----------+----------------------------------------------------------------+
+child_name+string const                                                    +
+----------+----------------------------------------------------------------+


|function-daBfg-/|

.. _function-_at_daBfg_c__c_getResolution_H_ls_daBfg_c__c_NameSpaceRequest_gr__Cs_Cf:

.. das:function:: getResolution(self: NameSpaceRequest; type_name: string const; multiplier: float const)

getResolution returns  :ref:`daBfg::AutoResolutionRequest <struct-daBfg-AutoResolutionRequest>` 

+----------+----------------------------------------------------------------+
+argument  +argument type                                                   +
+==========+================================================================+
+self      + :ref:`daBfg::NameSpaceRequest <handle-daBfg-NameSpaceRequest>` +
+----------+----------------------------------------------------------------+
+type_name +string const                                                    +
+----------+----------------------------------------------------------------+
+multiplier+float const                                                     +
+----------+----------------------------------------------------------------+


|function-daBfg-getResolution|


