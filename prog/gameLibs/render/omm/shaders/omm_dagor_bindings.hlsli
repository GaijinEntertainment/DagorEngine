#define COMPILER_DXC (0)
#define COMPILER_FXC (0)

#ifdef OMM_DAGOR_DECLARE_CBUFFER_TYPES
#define OMM_CONSTANTS_START(name) struct name {
#define OMM_CONSTANT(constantType, constantName) constantType constantName;
#define OMM_CONSTANTS_END(name, registerIndex) };

#define OMM_PUSH_CONSTANTS_START(name) struct name {
#define OMM_PUSH_CONSTANT(constantType, constantName) constantType constantName;
#define OMM_PUSH_CONSTANTS_END(name, registerIndex) };
#else
#define OMM_CONSTANTS_START(name)
#define OMM_CONSTANT(constantType, constantName)
#define OMM_CONSTANTS_END(name, registerIndex)

#define OMM_PUSH_CONSTANTS_START(name)
#define OMM_PUSH_CONSTANT(constantType, constantName)
#define OMM_PUSH_CONSTANTS_END(name, registerIndex)
#endif

#define OMM_INPUT_RESOURCE(resourceType, resourceName, regName, bindingIndex)
#define OMM_OUTPUT_RESOURCE(resourceType, resourceName, regName, bindingIndex)
#define OMM_SAMPLER(resourceType, resourceName, regName, bindingIndex)
#define OMM_DECLARE_GLOBAL_SAMPLERS
#define OMM_GLOBAL_SAMPLER(samplerIndex) s_sampler0

#define OMM_SUBRESOURCE(resourceType, alias, name) static resourceType alias = name;
#define OMM_INPUT_SUBRESOURCE(resourceType, alias, name) static resourceType alias = name;

#define OMM_SUBRESOURCE_LOAD(subResource, offset) subResource.Load(g_GlobalConstants.subResource ## Offset + offset)
#define OMM_SUBRESOURCE_LOAD2(subResource, offset) subResource.Load2(g_GlobalConstants.subResource ## Offset + offset)
#define OMM_SUBRESOURCE_LOAD3(subResource, offset) subResource.Load3(g_GlobalConstants.subResource ## Offset + offset)
#define OMM_SUBRESOURCE_LOAD4(subResource, offset) subResource.Load4(g_GlobalConstants.subResource ## Offset + offset)
#define OMM_SUBRESOURCE_STORE(subResource, offset, value) subResource.Store(g_GlobalConstants.subResource ## Offset + offset, value)
#define OMM_SUBRESOURCE_STORE2(subResource, offset, value) subResource.Store2(g_GlobalConstants.subResource ## Offset + offset, value)
#define OMM_SUBRESOURCE_STORE3(subResource, offset, value) subResource.Store3(g_GlobalConstants.subResource ## Offset + offset, value)
#define OMM_SUBRESOURCE_CAS(subResource, offset, compare_value, value, original_value) \
  subResource.InterlockedCompareExchange(g_GlobalConstants.subResource ## Offset + offset, compare_value, value, original_value)
#define OMM_SUBRESOURCE_INTERLOCKEDADD(subResource, offset, value, original_value) \
  subResource.InterlockedAdd(g_GlobalConstants.subResource ## Offset + offset, value, original_value)
#define OMM_SUBRESOURCE_INTERLOCKEDMAX(subResource, offset, value, original_value) \
  subResource.InterlockedMax(g_GlobalConstants.subResource ## Offset + offset, value, original_value)
