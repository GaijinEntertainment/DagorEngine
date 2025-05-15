..
  This is auto generated file. See daBfg/api/das/docs

.. _stdlib_daBfgCore:

====================
DaBfgCore das module
====================

daBfgCore module contains data structures
and bindings from das to cpp.


++++++++++++
Type aliases
++++++++++++

.. _alias-BindingsMap:

.. das:attribute:: BindingsMap = fixedVectorMap`int`Binding`8

|typedef-daBfgCore-BindingsMap|

++++++++++++
Enumerations
++++++++++++

.. _enum-daBfgCore-NameSpaceNameId:

.. das:attribute:: NameSpaceNameId

+-------+-----+
+Invalid+65535+
+-------+-----+


|enumeration-daBfgCore-NameSpaceNameId|

.. _enum-daBfgCore-NodeNameId:

.. das:attribute:: NodeNameId

+-------+-----+
+Invalid+65535+
+-------+-----+


|enumeration-daBfgCore-NodeNameId|

.. _enum-daBfgCore-ResNameId:

.. das:attribute:: ResNameId

+-------+-----+
+Invalid+65535+
+-------+-----+


|enumeration-daBfgCore-ResNameId|

.. _enum-daBfgCore-History:

.. das:attribute:: History

+---------------------+-+
+No                   +0+
+---------------------+-+
+ClearZeroOnFirstFrame+1+
+---------------------+-+
+DiscardOnFirstFrame  +2+
+---------------------+-+


|enumeration-daBfgCore-History|

.. _enum-daBfgCore-ResourceActivationAction:

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


|enumeration-daBfgCore-ResourceActivationAction|

.. _enum-daBfgCore-MultiplexingMode:

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


|enumeration-daBfgCore-MultiplexingMode|

.. _enum-daBfgCore-SideEffect:

.. das:attribute:: SideEffect

+--------+-+
+None    +0+
+--------+-+
+Internal+1+
+--------+-+
+External+2+
+--------+-+


|enumeration-daBfgCore-SideEffect|

.. _enum-daBfgCore-Access:

.. das:attribute:: Access

+----------+-+
+UNKNOWN   +0+
+----------+-+
+READ_ONLY +1+
+----------+-+
+READ_WRITE+2+
+----------+-+


|enumeration-daBfgCore-Access|

.. _enum-daBfgCore-Usage:

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


|enumeration-daBfgCore-Usage|

.. _enum-daBfgCore-Stage:

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


|enumeration-daBfgCore-Stage|

.. _enum-daBfgCore-ResourceType:

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


|enumeration-daBfgCore-ResourceType|

.. _enum-daBfgCore-AutoResTypeNameId:

.. das:attribute:: AutoResTypeNameId

+-------+-----+
+Invalid+65535+
+-------+-----+


|enumeration-daBfgCore-AutoResTypeNameId|

.. _enum-daBfgCore-VariableRateShadingCombiner:

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


|enumeration-daBfgCore-VariableRateShadingCombiner|

.. _enum-daBfgCore-BindingType:

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


|enumeration-daBfgCore-BindingType|

++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-daBfgCore-TextureResourceDescription:

.. das:attribute:: TextureResourceDescription

TextureResourceDescription fields are

+----------+--------------------------------------------------------------------------------------+
+height    +uint                                                                                  +
+----------+--------------------------------------------------------------------------------------+
+mipLevels +uint                                                                                  +
+----------+--------------------------------------------------------------------------------------+
+activation+ :ref:`daBfgCore::ResourceActivationAction <enum-daBfgCore-ResourceActivationAction>` +
+----------+--------------------------------------------------------------------------------------+
+width     +uint                                                                                  +
+----------+--------------------------------------------------------------------------------------+
+cFlags    +uint                                                                                  +
+----------+--------------------------------------------------------------------------------------+


|structure_annotation-daBfgCore-TextureResourceDescription|

.. _handle-daBfgCore-VolTextureResourceDescription:

.. das:attribute:: VolTextureResourceDescription

|structure_annotation-daBfgCore-VolTextureResourceDescription|

.. _handle-daBfgCore-ArrayTextureResourceDescription:

.. das:attribute:: ArrayTextureResourceDescription

|structure_annotation-daBfgCore-ArrayTextureResourceDescription|

.. _handle-daBfgCore-CubeTextureResourceDescription:

.. das:attribute:: CubeTextureResourceDescription

|structure_annotation-daBfgCore-CubeTextureResourceDescription|

.. _handle-daBfgCore-ArrayCubeTextureResourceDescription:

.. das:attribute:: ArrayCubeTextureResourceDescription

|structure_annotation-daBfgCore-ArrayCubeTextureResourceDescription|

.. _handle-daBfgCore-ResourceData:

.. das:attribute:: ResourceData

ResourceData fields are

+-------+--------------------------------------------------------------+
+resType+ :ref:`daBfgCore::ResourceType <enum-daBfgCore-ResourceType>` +
+-------+--------------------------------------------------------------+
+history+ :ref:`daBfgCore::History <enum-daBfgCore-History>`           +
+-------+--------------------------------------------------------------+


|structure_annotation-daBfgCore-ResourceData|

.. _handle-daBfgCore-AutoResolutionData:

.. das:attribute:: AutoResolutionData

AutoResolutionData fields are

+----------+------------------------------------------------------------------------+
+multiplier+float                                                                   +
+----------+------------------------------------------------------------------------+
+id        + :ref:`daBfgCore::AutoResTypeNameId <enum-daBfgCore-AutoResTypeNameId>` +
+----------+------------------------------------------------------------------------+


|structure_annotation-daBfgCore-AutoResolutionData|

.. _handle-daBfgCore-ShaderBlockLayersInfo:

.. das:attribute:: ShaderBlockLayersInfo

ShaderBlockLayersInfo fields are

+-----------+---+
+sceneLayer +int+
+-----------+---+
+objectLayer+int+
+-----------+---+
+frameLayer +int+
+-----------+---+


|structure_annotation-daBfgCore-ShaderBlockLayersInfo|

.. _handle-daBfgCore-VrsStateRequirements:

.. das:attribute:: VrsStateRequirements

VrsStateRequirements fields are

+----------------+--------------------------------------------------------------------------------------------+
+rateTextureResId+ :ref:`daBfgCore::ResNameId <enum-daBfgCore-ResNameId>`                                     +
+----------------+--------------------------------------------------------------------------------------------+
+pixelCombiner   + :ref:`daBfgCore::VariableRateShadingCombiner <enum-daBfgCore-VariableRateShadingCombiner>` +
+----------------+--------------------------------------------------------------------------------------------+
+rateY           +uint                                                                                        +
+----------------+--------------------------------------------------------------------------------------------+
+vertexCombiner  + :ref:`daBfgCore::VariableRateShadingCombiner <enum-daBfgCore-VariableRateShadingCombiner>` +
+----------------+--------------------------------------------------------------------------------------------+
+rateX           +uint                                                                                        +
+----------------+--------------------------------------------------------------------------------------------+


|structure_annotation-daBfgCore-VrsStateRequirements|

.. _handle-daBfgCore-VirtualSubresourceRef:

.. das:attribute:: VirtualSubresourceRef

VirtualSubresourceRef fields are

+--------+--------------------------------------------------------+
+layer   +uint                                                    +
+--------+--------------------------------------------------------+
+nameId  + :ref:`daBfgCore::ResNameId <enum-daBfgCore-ResNameId>` +
+--------+--------------------------------------------------------+
+mipLevel+uint                                                    +
+--------+--------------------------------------------------------+


|structure_annotation-daBfgCore-VirtualSubresourceRef|

.. _handle-daBfgCore-Binding:

.. das:attribute:: Binding

Binding fields are

+--------+------------------------------------------------------------+
+bindType+ :ref:`daBfgCore::BindingType <enum-daBfgCore-BindingType>` +
+--------+------------------------------------------------------------+
+resource+ :ref:`daBfgCore::ResNameId <enum-daBfgCore-ResNameId>`     +
+--------+------------------------------------------------------------+
+history +bool                                                        +
+--------+------------------------------------------------------------+


|structure_annotation-daBfgCore-Binding|

.. _handle-daBfgCore-ResourceUsage:

.. das:attribute:: ResourceUsage

ResourceUsage fields are

+---------+--------------------------------------------------+
+stage    + :ref:`daBfgCore::Stage <enum-daBfgCore-Stage>`   +
+---------+--------------------------------------------------+
+usageType+ :ref:`daBfgCore::Usage <enum-daBfgCore-Usage>`   +
+---------+--------------------------------------------------+
+access   + :ref:`daBfgCore::Access <enum-daBfgCore-Access>` +
+---------+--------------------------------------------------+


|structure_annotation-daBfgCore-ResourceUsage|

.. _handle-daBfgCore-ResourceRequest:

.. das:attribute:: ResourceRequest

ResourceRequest fields are

+-----------+------------------------------------------------------------------+
+usage      + :ref:`daBfgCore::ResourceUsage <handle-daBfgCore-ResourceUsage>` +
+-----------+------------------------------------------------------------------+
+slotRequest+bool                                                              +
+-----------+------------------------------------------------------------------+
+optional   +bool                                                              +
+-----------+------------------------------------------------------------------+


|structure_annotation-daBfgCore-ResourceRequest|

.. _handle-daBfgCore-BufferResourceDescription:

.. das:attribute:: BufferResourceDescription

BufferResourceDescription fields are

+------------------+--------------------------------------------------------------------------------------+
+viewFormat        +uint                                                                                  +
+------------------+--------------------------------------------------------------------------------------+
+activation        + :ref:`daBfgCore::ResourceActivationAction <enum-daBfgCore-ResourceActivationAction>` +
+------------------+--------------------------------------------------------------------------------------+
+elementCount      +uint                                                                                  +
+------------------+--------------------------------------------------------------------------------------+
+cFlags            +uint                                                                                  +
+------------------+--------------------------------------------------------------------------------------+
+elementSizeInBytes+uint                                                                                  +
+------------------+--------------------------------------------------------------------------------------+


|structure_annotation-daBfgCore-BufferResourceDescription|

.. _handle-daBfgCore-NodeStateRequirements:

.. das:attribute:: NodeStateRequirements

NodeStateRequirements fields are

+---------------------+----------------------------------------------------------------------------------------------+
+supportsWireframe    +bool                                                                                          +
+---------------------+----------------------------------------------------------------------------------------------+
+pipelineStateOverride+ :ref:`builtin::optional`OverrideState <handle-builtin-optional`OverrideState>`               +
+---------------------+----------------------------------------------------------------------------------------------+
+vrsState             + :ref:`builtin::optional`VrsStateRequirements <handle-builtin-optional`VrsStateRequirements>` +
+---------------------+----------------------------------------------------------------------------------------------+


|structure_annotation-daBfgCore-NodeStateRequirements|

.. _handle-daBfgCore-VirtualPassRequirements:

.. das:attribute:: VirtualPassRequirements

VirtualPassRequirements fields are

+----------------+----------------------------------------------------------------------------------------------------------+
+colorAttachments+ :ref:`builtin::fixedVector`VirtualSubresourceRef`8 <handle-builtin-fixedVector`VirtualSubresourceRef`8>` +
+----------------+----------------------------------------------------------------------------------------------------------+
+depthReadOnly   +bool                                                                                                      +
+----------------+----------------------------------------------------------------------------------------------------------+
+depthAttachment + :ref:`daBfgCore::VirtualSubresourceRef <handle-daBfgCore-VirtualSubresourceRef>`                         +
+----------------+----------------------------------------------------------------------------------------------------------+


|structure_annotation-daBfgCore-VirtualPassRequirements|

.. _handle-daBfgCore-NodeData:

.. das:attribute:: NodeData

NodeData fields are

+---------------------------+--------------------------------------------------------------------------------------------------------------------------+
+multiplexingMode           + :ref:`daBfgCore::MultiplexingMode <enum-daBfgCore-MultiplexingMode>`                                                     +
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
+shaderBlockLayers          + :ref:`daBfgCore::ShaderBlockLayersInfo <handle-daBfgCore-ShaderBlockLayersInfo>`                                         +
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
+sideEffect                 + :ref:`daBfgCore::SideEffect <enum-daBfgCore-SideEffect>`                                                                 +
+---------------------------+--------------------------------------------------------------------------------------------------------------------------+


|structure_annotation-daBfgCore-NodeData|

.. _handle-daBfgCore-ResourceProvider:

.. das:attribute:: ResourceProvider

|structure_annotation-daBfgCore-ResourceProvider|

.. _handle-daBfgCore-InternalRegistry:

.. das:attribute:: InternalRegistry

InternalRegistry fields are

+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+resources + :ref:`builtin::idIndexedMapping`ResNameId`ResourceData <handle-builtin-idIndexedMapping`ResNameId`ResourceData>`                                                   +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+nodes     + :ref:`builtin::idIndexedMapping`NodeNameId`NodeData <handle-builtin-idIndexedMapping`NodeNameId`NodeData>`                                                         +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
+knownNames+ :ref:`builtin::idNameMap`NameSpaceNameId`ResNameId`NodeNameId`AutoResTypeNameId <handle-builtin-idNameMap`NameSpaceNameId`ResNameId`NodeNameId`AutoResTypeNameId>` +
+----------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+


|structure_annotation-daBfgCore-InternalRegistry|

.. _handle-daBfgCore-NodeTracker:

.. das:attribute:: NodeTracker

|structure_annotation-daBfgCore-NodeTracker|

.. _handle-daBfgCore-NodeHandle:

.. das:attribute:: NodeHandle

NodeHandle property operators are

+-----+----+
+valid+bool+
+-----+----+


|structure_annotation-daBfgCore-NodeHandle|

+++++++++++++++++++
Top level functions
+++++++++++++++++++

  *  :ref:`registerNode (arg0:daBfgCore::NodeTracker implicit;arg1:daBfgCore::NodeNameId const;arg2:__context const) : void <function-_at_daBfgCore_c__c_registerNode_IH_ls_daBfgCore_c__c_NodeTracker_gr__CE16_ls_daBfgCore_c__c_NodeNameId_gr__C_c>` 
  *  :ref:`get_shader_variable_id (arg0:string const implicit) : int <function-_at_daBfgCore_c__c_get_shader_variable_id_CIs>` 
  *  :ref:`fill_slot (arg0:daBfgCore::NameSpaceNameId const;arg1:string const implicit;arg2:daBfgCore::NameSpaceNameId const;arg3:string const implicit) : void <function-_at_daBfgCore_c__c_fill_slot_CE16_ls_daBfgCore_c__c_NameSpaceNameId_gr__CIs_CE16_ls_daBfgCore_c__c_NameSpaceNameId_gr__CIs>` 
  *  :ref:`resetNode (arg0:daBfgCore::NodeHandle implicit) : void <function-_at_daBfgCore_c__c_resetNode_IH_ls_daBfgCore_c__c_NodeHandle_gr_>` 

.. _function-_at_daBfgCore_c__c_registerNode_IH_ls_daBfgCore_c__c_NodeTracker_gr__CE16_ls_daBfgCore_c__c_NodeNameId_gr__C_c:

.. das:function:: registerNode(arg0: NodeTracker implicit; arg1: NodeNameId const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+arg0    + :ref:`daBfgCore::NodeTracker <handle-daBfgCore-NodeTracker>`  implicit+
+--------+-----------------------------------------------------------------------+
+arg1    + :ref:`daBfgCore::NodeNameId <enum-daBfgCore-NodeNameId>`  const       +
+--------+-----------------------------------------------------------------------+


|function-daBfgCore-registerNode|

.. _function-_at_daBfgCore_c__c_get_shader_variable_id_CIs:

.. das:function:: get_shader_variable_id(arg0: string const implicit)

get_shader_variable_id returns int

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+arg0    +string const implicit+
+--------+---------------------+


|function-daBfgCore-get_shader_variable_id|

.. _function-_at_daBfgCore_c__c_fill_slot_CE16_ls_daBfgCore_c__c_NameSpaceNameId_gr__CIs_CE16_ls_daBfgCore_c__c_NameSpaceNameId_gr__CIs:

.. das:function:: fill_slot(arg0: NameSpaceNameId const; arg1: string const implicit; arg2: NameSpaceNameId const; arg3: string const implicit)

+--------+--------------------------------------------------------------------------+
+argument+argument type                                                             +
+========+==========================================================================+
+arg0    + :ref:`daBfgCore::NameSpaceNameId <enum-daBfgCore-NameSpaceNameId>`  const+
+--------+--------------------------------------------------------------------------+
+arg1    +string const implicit                                                     +
+--------+--------------------------------------------------------------------------+
+arg2    + :ref:`daBfgCore::NameSpaceNameId <enum-daBfgCore-NameSpaceNameId>`  const+
+--------+--------------------------------------------------------------------------+
+arg3    +string const implicit                                                     +
+--------+--------------------------------------------------------------------------+


|function-daBfgCore-fill_slot|

.. _function-_at_daBfgCore_c__c_resetNode_IH_ls_daBfgCore_c__c_NodeHandle_gr_:

.. das:function:: resetNode(arg0: NodeHandle implicit)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+arg0    + :ref:`daBfgCore::NodeHandle <handle-daBfgCore-NodeHandle>`  implicit+
+--------+---------------------------------------------------------------------+


|function-daBfgCore-resetNode|


