..
  This is auto generated file. See daBfg/api/das/docs

.. _stdlib_daFgCore:

=======================
daFgCore Daslang Module
=======================

daFgCore module contains data structures and bindings from Daslang to C++.


++++++++++++
Type Aliases
++++++++++++

.. _alias-BindingsMap:

.. das:attribute:: BindingsMap = fixedVectorMap`int`Binding`8

|typedef-daFgCore-BindingsMap|

++++++++++++
Enumerations
++++++++++++

.. _enum-daFgCore-NameSpaceNameId:

.. das:attribute:: NameSpaceNameId

+-------+-----+
+Invalid+65535+
+-------+-----+


|enumeration-daFgCore-NameSpaceNameId|

.. _enum-daFgCore-NodeNameId:

.. das:attribute:: NodeNameId

+-------+-----+
+Invalid+65535+
+-------+-----+


|enumeration-daFgCore-NodeNameId|

.. _enum-daFgCore-ResNameId:

.. das:attribute:: ResNameId

+-------+-----+
+Invalid+65535+
+-------+-----+


|enumeration-daFgCore-ResNameId|

.. _enum-daFgCore-History:

.. das:attribute:: History

+---------------------+-+
+No                   +0+
+---------------------+-+
+ClearZeroOnFirstFrame+1+
+---------------------+-+
+DiscardOnFirstFrame  +2+
+---------------------+-+


|enumeration-daFgCore-History|

.. _enum-daFgCore-ResourceActivationAction:

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


|enumeration-daFgCore-ResourceActivationAction|

.. _enum-daFgCore-MultiplexingMode:

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


|enumeration-daFgCore-MultiplexingMode|

.. _enum-daFgCore-SideEffect:

.. das:attribute:: SideEffect

+--------+-+
+None    +0+
+--------+-+
+Internal+1+
+--------+-+
+External+2+
+--------+-+


|enumeration-daFgCore-SideEffect|

.. _enum-daFgCore-Access:

.. das:attribute:: Access

+----------+-+
+UNKNOWN   +0+
+----------+-+
+READ_ONLY +1+
+----------+-+
+READ_WRITE+2+
+----------+-+


|enumeration-daFgCore-Access|

.. _enum-daFgCore-Usage:

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


|enumeration-daFgCore-Usage|

.. _enum-daFgCore-Stage:

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


|enumeration-daFgCore-Stage|

.. _enum-daFgCore-ResourceType:

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


|enumeration-daFgCore-ResourceType|

.. _enum-daFgCore-AutoResTypeNameId:

.. das:attribute:: AutoResTypeNameId

+-------+-----+
+Invalid+65535+
+-------+-----+


|enumeration-daFgCore-AutoResTypeNameId|

.. _enum-daFgCore-VariableRateShadingCombiner:

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


|enumeration-daFgCore-VariableRateShadingCombiner|

.. _enum-daFgCore-BindingType:

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


|enumeration-daFgCore-BindingType|

++++++++++++++++++
Handled Structures
++++++++++++++++++

.. _handle-daFgCore-TextureResourceDescription:

.. das:attribute:: TextureResourceDescription

TextureResourceDescription fields are

+----------+--------------------------------------------------------------------------------------+
+height    +uint                                                                                  +
+----------+--------------------------------------------------------------------------------------+
+mipLevels +uint                                                                                  +
+----------+--------------------------------------------------------------------------------------+
+activation+ :ref:`daFgCore::ResourceActivationAction <enum-daFgCore-ResourceActivationAction>`   +
+----------+--------------------------------------------------------------------------------------+
+width     +uint                                                                                  +
+----------+--------------------------------------------------------------------------------------+
+cFlags    +uint                                                                                  +
+----------+--------------------------------------------------------------------------------------+


|structure_annotation-daFgCore-TextureResourceDescription|

.. _handle-daFgCore-VolTextureResourceDescription:

.. das:attribute:: VolTextureResourceDescription

|structure_annotation-daFgCore-VolTextureResourceDescription|

.. _handle-daFgCore-ArrayTextureResourceDescription:

.. das:attribute:: ArrayTextureResourceDescription

|structure_annotation-daFgCore-ArrayTextureResourceDescription|

.. _handle-daFgCore-CubeTextureResourceDescription:

.. das:attribute:: CubeTextureResourceDescription

|structure_annotation-daFgCore-CubeTextureResourceDescription|

.. _handle-daFgCore-ArrayCubeTextureResourceDescription:

.. das:attribute:: ArrayCubeTextureResourceDescription

|structure_annotation-daFgCore-ArrayCubeTextureResourceDescription|

.. _handle-daFgCore-ResourceData:

.. das:attribute:: ResourceData

ResourceData fields are

+-------+------------------------------------------------------------+
+resType+ :ref:`daFgCore::ResourceType <enum-daFgCore-ResourceType>` +
+-------+------------------------------------------------------------+
+history+ :ref:`daFgCore::History <enum-daFgCore-History>`           +
+-------+------------------------------------------------------------+


|structure_annotation-daFgCore-ResourceData|

.. _handle-daFgCore-AutoResolutionData:

.. das:attribute:: AutoResolutionData

AutoResolutionData fields are

+----------+----------------------------------------------------------------------+
+multiplier+float                                                                 +
+----------+----------------------------------------------------------------------+
+id        + :ref:`daFgCore::AutoResTypeNameId <enum-daFgCore-AutoResTypeNameId>` +
+----------+----------------------------------------------------------------------+


|structure_annotation-daFgCore-AutoResolutionData|

.. _handle-daFgCore-ShaderBlockLayersInfo:

.. das:attribute:: ShaderBlockLayersInfo

ShaderBlockLayersInfo fields are

+-----------+---+
+sceneLayer +int+
+-----------+---+
+objectLayer+int+
+-----------+---+
+frameLayer +int+
+-----------+---+


|structure_annotation-daFgCore-ShaderBlockLayersInfo|

.. _handle-daFgCore-VrsStateRequirements:

.. das:attribute:: VrsStateRequirements

VrsStateRequirements fields are

+----------------+------------------------------------------------------------------------------------------+
+rateTextureResId+ :ref:`daFgCore::ResNameId <enum-daFgCore-ResNameId>`                                     +
+----------------+------------------------------------------------------------------------------------------+
+pixelCombiner   + :ref:`daFgCore::VariableRateShadingCombiner <enum-daFgCore-VariableRateShadingCombiner>` +
+----------------+------------------------------------------------------------------------------------------+
+rateY           +uint                                                                                      +
+----------------+------------------------------------------------------------------------------------------+
+vertexCombiner  + :ref:`daFgCore::VariableRateShadingCombiner <enum-daFgCore-VariableRateShadingCombiner>` +
+----------------+------------------------------------------------------------------------------------------+
+rateX           +uint                                                                                      +
+----------------+------------------------------------------------------------------------------------------+


|structure_annotation-daFgCore-VrsStateRequirements|

.. _handle-daFgCore-VirtualSubresourceRef:

.. das:attribute:: VirtualSubresourceRef

VirtualSubresourceRef fields are

+--------+------------------------------------------------------+
+layer   +uint                                                  +
+--------+------------------------------------------------------+
+nameId  + :ref:`daFgCore::ResNameId <enum-daFgCore-ResNameId>` +
+--------+------------------------------------------------------+
+mipLevel+uint                                                  +
+--------+------------------------------------------------------+


|structure_annotation-daFgCore-VirtualSubresourceRef|

.. _handle-daFgCore-Binding:

.. das:attribute:: Binding

Binding fields are

+--------+----------------------------------------------------------+
+bindType+ :ref:`daFgCore::BindingType <enum-daFgCore-BindingType>` +
+--------+----------------------------------------------------------+
+resource+ :ref:`daFgCore::ResNameId <enum-daFgCore-ResNameId>`     +
+--------+----------------------------------------------------------+
+history +bool                                                      +
+--------+----------------------------------------------------------+


|structure_annotation-daFgCore-Binding|

.. _handle-daFgCore-ResourceUsage:

.. das:attribute:: ResourceUsage

ResourceUsage fields are

+---------+------------------------------------------------+
+stage    + :ref:`daFgCore::Stage <enum-daFgCore-Stage>`   +
+---------+------------------------------------------------+
+usageType+ :ref:`daFgCore::Usage <enum-daFgCore-Usage>`   +
+---------+------------------------------------------------+
+access   + :ref:`daFgCore::Access <enum-daFgCore-Access>` +
+---------+------------------------------------------------+


|structure_annotation-daFgCore-ResourceUsage|

.. _handle-daFgCore-ResourceRequest:

.. das:attribute:: ResourceRequest

ResourceRequest fields are

+-----------+----------------------------------------------------------------+
+usage      + :ref:`daFgCore::ResourceUsage <handle-daFgCore-ResourceUsage>` +
+-----------+----------------------------------------------------------------+
+slotRequest+bool                                                            +
+-----------+----------------------------------------------------------------+
+optional   +bool                                                            +
+-----------+----------------------------------------------------------------+


|structure_annotation-daFgCore-ResourceRequest|

.. _handle-daFgCore-BufferResourceDescription:

.. das:attribute:: BufferResourceDescription

BufferResourceDescription fields are

+------------------+------------------------------------------------------------------------------------+
+viewFormat        +uint                                                                                +
+------------------+------------------------------------------------------------------------------------+
+activation        + :ref:`daFgCore::ResourceActivationAction <enum-daFgCore-ResourceActivationAction>` +
+------------------+------------------------------------------------------------------------------------+
+elementCount      +uint                                                                                +
+------------------+------------------------------------------------------------------------------------+
+cFlags            +uint                                                                                +
+------------------+------------------------------------------------------------------------------------+
+elementSizeInBytes+uint                                                                                +
+------------------+------------------------------------------------------------------------------------+


|structure_annotation-daFgCore-BufferResourceDescription|

.. _handle-daFgCore-NodeStateRequirements:

.. das:attribute:: NodeStateRequirements

NodeStateRequirements fields are

+---------------------+----------------------------------------------------------------------------------------------+
+supportsWireframe    +bool                                                                                          +
+---------------------+----------------------------------------------------------------------------------------------+
+pipelineStateOverride+ :ref:`builtin::optional`OverrideState <handle-builtin-optional`OverrideState>`               +
+---------------------+----------------------------------------------------------------------------------------------+
+vrsState             + :ref:`builtin::optional`VrsStateRequirements <handle-builtin-optional`VrsStateRequirements>` +
+---------------------+----------------------------------------------------------------------------------------------+


|structure_annotation-daFgCore-NodeStateRequirements|

.. _handle-daFgCore-VirtualPassRequirements:

.. das:attribute:: VirtualPassRequirements

VirtualPassRequirements fields are

+----------------+----------------------------------------------------------------------------------------------------------+
+colorAttachments+ :ref:`builtin::fixedVector`VirtualSubresourceRef`8 <handle-builtin-fixedVector`VirtualSubresourceRef`8>` +
+----------------+----------------------------------------------------------------------------------------------------------+
+depthReadOnly   +bool                                                                                                      +
+----------------+----------------------------------------------------------------------------------------------------------+
+depthAttachment + :ref:`daFgCore::VirtualSubresourceRef <handle-daFgCore-VirtualSubresourceRef>`                           +
+----------------+----------------------------------------------------------------------------------------------------------+


|structure_annotation-daFgCore-VirtualPassRequirements|

.. _handle-daFgCore-NodeData:

.. das:attribute:: NodeData

NodeData fields are
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+multiplexingMode           + :ref:`daFgCore::MultiplexingMode <enum-daFgCore-MultiplexingMode>`                                                       +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+generation                 +uint16                                                                                                                    +
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
+shaderBlockLayers          + :ref:`daFgCore::ShaderBlockLayersInfo <handle-daFgCore-ShaderBlockLayersInfo>`                                           +
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
+sideEffect                 + :ref:`daFgCore::SideEffect <enum-daFgCore-SideEffect>`                                                                   +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+


|structure_annotation-daFgCore-NodeData|

.. _handle-daFgCore-ResourceProvider:

.. das:attribute:: ResourceProvider

|structure_annotation-daFgCore-ResourceProvider|

.. _handle-daFgCore-InternalRegistry:

.. das:attribute:: InternalRegistry

InternalRegistry fields are

+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+resources + :ref:`builtin::idIndexedMapping`ResNameId`ResourceData <handle-builtin-idIndexedMapping`ResNameId`ResourceData>`                                                   +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+nodes     + :ref:`builtin::idIndexedMapping`NodeNameId`NodeData <handle-builtin-idIndexedMapping`NodeNameId`NodeData>`                                                         +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+knownNames+ :ref:`builtin::idNameMap`NameSpaceNameId`ResNameId`NodeNameId`AutoResTypeNameId <handle-builtin-idNameMap`NameSpaceNameId`ResNameId`NodeNameId`AutoResTypeNameId>` +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+


|structure_annotation-daFgCore-InternalRegistry|

.. _handle-daFgCore-NodeTracker:

.. das:attribute:: NodeTracker

|structure_annotation-daFgCore-NodeTracker|

.. _handle-daFgCore-NodeHandle:

.. das:attribute:: NodeHandle

NodeHandle property operators are

+-----+----+
+valid+bool+
+-----+----+


|structure_annotation-daFgCore-NodeHandle|

+++++++++++++++++++
Top Level Functions
+++++++++++++++++++

  *  :ref:`registerNode (arg0:daFgCore::NodeTracker implicit;arg1:daFgCore::NodeNameId const;arg2:__context const) : void <function-_at_daFgCore_c__c_registerNode_IH_ls_daFgCore_c__c_NodeTracker_gr__CE16_ls_daFgCore_c__c_NodeNameId_gr__C_c>`
  *  :ref:`get_shader_variable_id (arg0:string const implicit) : int <function-_at_daFgCore_c__c_get_shader_variable_id_CIs>`
  *  :ref:`fill_slot (arg0:daFgCore::NameSpaceNameId const;arg1:string const implicit;arg2:daFgCore::NameSpaceNameId const;arg3:string const implicit) : void <function-_at_daFgCore_c__c_fill_slot_CE16_ls_daFgCore_c__c_NameSpaceNameId_gr__CIs_CE16_ls_daFgCore_c__c_NameSpaceNameId_gr__CIs>`
  *  :ref:`resetNode (arg0:daFgCore::NodeHandle implicit) : void <function-_at_daFgCore_c__c_resetNode_IH_ls_daFgCore_c__c_NodeHandle_gr_>`

.. _function-_at_daFgCore_c__c_registerNode_IH_ls_daFgCore_c__c_NodeTracker_gr__CE16_ls_daFgCore_c__c_NodeNameId_gr__C_c:

.. das:function:: registerNode(arg0: NodeTracker implicit; arg1: NodeNameId const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+arg0    + :ref:`daFgCore::NodeTracker <handle-daFgCore-NodeTracker>`  implicit  +
+--------+-----------------------------------------------------------------------+
+arg1    + :ref:`daFgCore::NodeNameId <enum-daFgCore-NodeNameId>`  const         +
+--------+-----------------------------------------------------------------------+


|function-daFgCore-registerNode|

.. _function-_at_daFgCore_c__c_get_shader_variable_id_CIs:

.. das:function:: get_shader_variable_id(arg0: string const implicit)

get_shader_variable_id returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+arg0    +string const implicit+
+--------+---------------------+


|function-daFgCore-get_shader_variable_id|

.. _function-_at_daFgCore_c__c_fill_slot_CE16_ls_daFgCore_c__c_NameSpaceNameId_gr__CIs_CE16_ls_daFgCore_c__c_NameSpaceNameId_gr__CIs:

.. das:function:: fill_slot(arg0: NameSpaceNameId const; arg1: string const implicit; arg2: NameSpaceNameId const; arg3: string const implicit)

+--------+--------------------------------------------------------------------------+
+argument+argument type                                                             +
+========+==========================================================================+
+arg0    + :ref:`daFgCore::NameSpaceNameId <enum-daFgCore-NameSpaceNameId>`  const  +
+--------+--------------------------------------------------------------------------+
+arg1    +string const implicit                                                     +
+--------+--------------------------------------------------------------------------+
+arg2    + :ref:`daFgCore::NameSpaceNameId <enum-daFgCore-NameSpaceNameId>`  const  +
+--------+--------------------------------------------------------------------------+
+arg3    +string const implicit                                                     +
+--------+--------------------------------------------------------------------------+


|function-daFgCore-fill_slot|

.. _function-_at_daFgCore_c__c_resetNode_IH_ls_daFgCore_c__c_NodeHandle_gr_:

.. das:function:: resetNode(arg0: NodeHandle implicit)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+arg0    + :ref:`daFgCore::NodeHandle <handle-daFgCore-NodeHandle>`  implicit  +
+--------+---------------------------------------------------------------------+


|function-daFgCore-resetNode|


