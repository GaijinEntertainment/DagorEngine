/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include <shared/assert.h>

#include "shader_bindings.h"

namespace omm
{

void ShaderBindings::BindResource(const ResourceBinding& resource) {
    auto res = resources.insert(std::make_pair(resource.nameHash, resource)).second;
    OMM_ASSERT(res);
    resourcesVec.push_back(resource);
}

void ShaderBindings::BindSubResource(const SubResourceBinding& subresource) {
    auto res = subResources.insert(std::make_pair(subresource.nameHash, subresource)).second;
    OMM_ASSERT(res);
    subResourcesVec.push_back(subresource);
}

void ShaderBindings::FinaliseBindings()
{
    ommGpuDescriptorRangeDesc textureRead        = { ommGpuDescriptorType_TextureRead,       0xFFFFFFFF, 0 };
    ommGpuDescriptorRangeDesc bufferRead         = { ommGpuDescriptorType_BufferRead,        0xFFFFFFFF, 0 };
    ommGpuDescriptorRangeDesc rawBufferRead      = { ommGpuDescriptorType_RawBufferRead,     0xFFFFFFFF, 0 };
    ommGpuDescriptorRangeDesc rawBufferWrite     = { ommGpuDescriptorType_RawBufferWrite,    0xFFFFFFFF, 0 };

    for (auto& [_, binding] : resources)
    {
        if (binding.type == HLSLResourceType::Texture2D)
        {
            textureRead.baseRegisterIndex = std::min(textureRead.baseRegisterIndex, binding.registerIndex);
            textureRead.descriptorNum++;
        }
        else if (binding.type == HLSLResourceType::Buffer)
        {
            bufferRead.baseRegisterIndex = std::min(bufferRead.baseRegisterIndex, binding.registerIndex);
            bufferRead.descriptorNum++;
        }
        else if (binding.type == HLSLResourceType::ByteAddressBuffer)
        {
            rawBufferRead.baseRegisterIndex = std::min(rawBufferRead.baseRegisterIndex, binding.registerIndex);
            rawBufferRead.descriptorNum++;
        }
        else if (binding.type == HLSLResourceType::RWByteAddressBuffer)
        {
            rawBufferWrite.baseRegisterIndex = std::min(rawBufferWrite.baseRegisterIndex, binding.registerIndex);
            rawBufferWrite.descriptorNum++;
        }
    }

    if (textureRead.descriptorNum != 0)
        ranges.push_back(textureRead);
    if (bufferRead.descriptorNum != 0)
        ranges.push_back(bufferRead);
    if (rawBufferRead.descriptorNum != 0)
        ranges.push_back(rawBufferRead);
    if (rawBufferWrite.descriptorNum != 0)
        ranges.push_back(rawBufferWrite);
}

ResourceBinding ShaderBindings::GetSubResourceBinding(const char* subResourceName) const
{
    return GetSubResourceBinding(const_hash(subResourceName));
}

ResourceBinding ShaderBindings::GetSubResourceBinding(uint32_t subResourceNameHash) const
{
    auto subIt = subResources.find(subResourceNameHash);
    OMM_ASSERT(subResources.end() != subIt && "Resource not found!");

    auto resIt = resources.find(subIt->second.resourceNameHash);
    return resIt->second;
}

bool ShaderBindings::HasSubResourceBinding(const char* subResourceName) const
{
    return HasSubResourceBinding(const_hash(subResourceName));
}

bool ShaderBindings::HasSubResourceBinding(uint32_t subResourceNameHash) const
{
    auto it = subResources.find(subResourceNameHash);
    return subResources.end() != it;
}

ResourceBinding ShaderBindings::GetResourceBinding(const char* resourceName) const
{
    return GetResourceBinding(const_hash(resourceName));
}

ResourceBinding ShaderBindings::GetResourceBinding(uint32_t resourceNameHash) const
{
    auto it = resources.find(resourceNameHash);
    OMM_ASSERT(resources.end() != it && "Resource not found!");

    return it->second;
}

bool ShaderBindings::HasResourceBinding(const char* resourceName) const
{
    return HasResourceBinding(const_hash(resourceName));
}

bool  ShaderBindings::HasResourceBinding(uint32_t resourceNameHash) const
{
    auto it = resources.find(resourceNameHash);
    return resources.end() != it;
}

const ommGpuDescriptorRangeDesc* ShaderBindings::GetRanges() const
{
    return ranges.data();
}

uint32_t ShaderBindings::GetNumRanges() const
{
    return (uint32_t)ranges.size();
}

const ResourceBinding* ShaderBindings::GetResources() const
{
    return resourcesVec.data();
}

uint32_t ShaderBindings::GetNumResources() const {
    return (uint32_t)resourcesVec.size();
}

const SubResourceBinding* ShaderBindings::GetSubResources() const {
    return subResourcesVec.data();
}

uint32_t ShaderBindings::GetNumSubResources() const {
    return (uint32_t)subResourcesVec.size();
}

}