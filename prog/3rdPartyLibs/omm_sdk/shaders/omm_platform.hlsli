/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef PLATFORM_HLSLI
#define PLATFORM_HLSLI

#ifdef __spirv__
#define ALLOW_UAV_CONDITION
#define VK_PUSH_CONSTANT [[vk::push_constant]]
#else // dxil
#define ALLOW_UAV_CONDITION [allow_uav_condition]
#define VK_PUSH_CONSTANT
#endif

//=================================================================================================================================
// BINDINGS
//=================================================================================================================================

#ifndef COMPILER_DXC
#define COMPILER_DXC (1)
#endif

#ifndef COMPILER_FXC
#define COMPILER_FXC (0)
#endif

#if( COMPILER_FXC || COMPILER_DXC )

#define OMM_CONSTANTS_START(name) \
        struct name {
#define OMM_CONSTANT( constantType, constantName ) constantType constantName;
#define OMM_CONSTANTS_END(name, registerIndex)                               \
            };                                                              \
            ConstantBuffer<name>	g_ ## name		: register(b ## registerIndex);

#define OMM_PUSH_CONSTANTS_START(name) struct name {
#define OMM_PUSH_CONSTANT( constantType, constantName ) constantType constantName;
#define OMM_PUSH_CONSTANTS_END(name, registerIndex)                               \
            };                                                              \
            VK_PUSH_CONSTANT ConstantBuffer<name>	g_ ## name		: register(b ## registerIndex);

    #define OMM_INPUT_RESOURCE( resourceType, resourceName, regName, bindingIndex ) \
        resourceType resourceName : register( regName ## bindingIndex );
    #define OMM_OUTPUT_RESOURCE( resourceType, resourceName, regName, bindingIndex ) \
        resourceType resourceName : register( regName ## bindingIndex );
    #define OMM_SAMPLER( resourceType, resourceName, regName, bindingIndex ) \
        resourceType resourceName : register( regName ## bindingIndex );

#define OMM_SUBRESOURCE( resourceType, alias, name ) \
        static resourceType alias = name;
#define OMM_INPUT_SUBRESOURCE( resourceType, alias, name ) \
        static resourceType alias = name;

#define OMM_SUBRESOURCE_LOAD(subResource, offset) subResource.Load(g_GlobalConstants.subResource##Offset + offset)
#define OMM_SUBRESOURCE_LOAD2(subResource, offset) subResource.Load2(g_GlobalConstants.subResource##Offset + offset)
#define OMM_SUBRESOURCE_LOAD3(subResource, offset) subResource.Load3(g_GlobalConstants.subResource##Offset + offset)
#define OMM_SUBRESOURCE_STORE(subResource, offset, value) subResource.Store(g_GlobalConstants.subResource##Offset + offset, value)
#define OMM_SUBRESOURCE_STORE2(subResource, offset, value) subResource.Store2(g_GlobalConstants.subResource##Offset + offset, value)
#define OMM_SUBRESOURCE_STORE3(subResource, offset, value) subResource.Store3(g_GlobalConstants.subResource##Offset + offset, value)
#define OMM_SUBRESOURCE_CAS(subResource, offset, compare_value, value, original_value) subResource.InterlockedCompareExchange(g_GlobalConstants.subResource##Offset + offset, compare_value, value, original_value)
#define OMM_SUBRESOURCE_INTERLOCKEDADD(subResource, offset, value, original_value) subResource.InterlockedAdd(g_GlobalConstants.subResource##Offset + offset, value, original_value)
#define OMM_SUBRESOURCE_INTERLOCKEDMAX(subResource, offset, value, original_value) subResource.InterlockedMax(g_GlobalConstants.subResource##Offset + offset, value, original_value)

#elif( \
defined( OMM_INPUT_RESOURCE ) && \
defined( OMM_OUTPUT_RESOURCE ) && \
defined( OMM_SUBRESOURCE ) &&\
defined( OMM_CONSTANTS_START ) && \
defined( OMM_CONSTANT ) && \
defined( OMM_CONSTANTS_END ) && \
defined( OMM_PUSH_CONSTANTS_START ) && \
defined( OMM_PUSH_CONSTANT ) && \
defined( OMM_PUSH_CONSTANTS_END)
)
    // Custom engine that has already defined all the macros

#else

    #error "Please, define one of COMPILER_FXC / COMPILER_DXC or COMPILER_UNREAL_ENGINE or add custom bindings"
    #error "Please, define the following HLSL keywords to match your platform"

    #define RWTexture2D
    #define numthreads
    #define SV_DispatchThreadId
    #define SV_GroupThreadId
    #define SV_GroupId
    #define cbuffer
    #define SV_GroupIndex
    #define GroupMemoryBarrierWithGroupSync
    #define groupshared
    #define GroupMemoryBarrier
    #define SampleLevel
    #define unorm
    #define InterlockedAdd
    #define InterlockedMax

#endif

#define GPU_ASSERT(expr) \
do  \
{\
	if (!(expr)) \
	{ \
		u_debugBuffer.Store(g_GlobalConstants.AssertBufferOffset + 0, __LINE__); \
	} \
		 \
} while (false)

#endif // include guard