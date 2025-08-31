/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include <omm.h>
#include <shared/math.h>

#include "std_allocator.h"
#include "std_containers.h"

namespace omm
{

using uint = uint32_t;

unsigned constexpr const_hash(char const* input) {
    return *input ?
        static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) :
        5381;
}

enum class HLSLResourceType
{
    Buffer,
    ByteAddressBuffer,
    RWByteAddressBuffer,
    Texture2D,
};

template<class T = void>
struct Buffer
{
    static constexpr HLSLResourceType Type = HLSLResourceType::Buffer;
};

struct ByteAddressBuffer
{
    static constexpr HLSLResourceType Type = HLSLResourceType::ByteAddressBuffer;
};

struct RWByteAddressBuffer
{
    static constexpr HLSLResourceType Type = HLSLResourceType::RWByteAddressBuffer;
};

template<class T>
struct Texture2D
{
    static constexpr HLSLResourceType Type = HLSLResourceType::Texture2D;
};

struct ResourceBinding
{
    HLSLResourceType    type;
    const char*         name;
    uint32_t            nameHash;
    uint32_t            registerIndex;
};

struct SubResourceBinding
{
    const char*     name;
    uint32_t        nameHash;
    const char*     resourceName;
    uint32_t        resourceNameHash;
};

struct ShaderBindings
{
protected:
    void BindResource(const ResourceBinding& resource);
    void BindSubResource(const SubResourceBinding& subresource);
    void FinaliseBindings();

    ShaderBindings(const StdAllocator<uint8_t>& stdAllocator)
        : ranges(stdAllocator)
        , resourcesVec(stdAllocator)
        , subResourcesVec(stdAllocator)
        , resources(stdAllocator)
        , subResources(stdAllocator)
    {
    }
public:
    ResourceBinding GetSubResourceBinding(uint32_t subResourceNameHash) const;
    ResourceBinding GetSubResourceBinding(const char* subResourceName) const;
    bool HasSubResourceBinding(const char* subResourceName) const;
    bool HasSubResourceBinding(uint32_t subResourceNameHash) const;

    ResourceBinding GetResourceBinding(uint32_t subResourceNameHash) const;
    ResourceBinding GetResourceBinding(const char* resourceName) const;
    bool HasResourceBinding(const char* resourceName) const;
    bool HasResourceBinding(uint32_t resourceNameHash) const;

    const ommGpuDescriptorRangeDesc* GetRanges() const;
    uint32_t GetNumRanges() const;

    const ResourceBinding* GetResources() const;
    uint32_t GetNumResources() const;

    const SubResourceBinding* GetSubResources() const;
    uint32_t GetNumSubResources() const;

private:
   vector<ommGpuDescriptorRangeDesc>   ranges;
   vector<ResourceBinding>            resourcesVec;
   vector<SubResourceBinding>         subResourcesVec;
   map<uint32_t, ResourceBinding>     resources;
   map<uint32_t, SubResourceBinding>  subResources;
};

#define OMM_INPUT_RESOURCE(type, name, resisterType, registerIndex) BindResource({type::Type, #name, const_hash(#name), registerIndex });
#define OMM_OUTPUT_RESOURCE(type, name, resisterType, registerIndex) BindResource({type::Type, #name, const_hash(#name), registerIndex });
#define OMM_SUBRESOURCE(type, name, resource) BindSubResource({#name, const_hash(#name), #resource, const_hash(#resource) });

#define BEGIN_DECLARE_SHADER_BINDINGS(name) \
struct name : public ShaderBindings \
{ \
    name(const StdAllocator<uint8_t>& stdAllocator):ShaderBindings(stdAllocator) \
    {

#define END_DECLARE_SHADER_BINDINGS \
    FinaliseBindings();\
    } \
}; \

#include "shader_registry.h"

#undef OMM_INPUT_RESOURCE
#undef OMM_OUTPUT_RESOURCE
#undef OMM_OUTPUT_SUBRESOURCE
#undef OMM_DECLARE_LOCAL_CONSTANT_BUFFER
#undef OMM_CONSTANTS_START
#undef OMM_CONSTANT
#undef OMM_CONSTANTS_END


}