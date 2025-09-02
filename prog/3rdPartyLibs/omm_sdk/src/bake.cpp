/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "omm.h"
#include "omm_handle.h"
#include "bake_cpu_impl.h"
#include "bake_gpu_impl.h"
#include "debug_impl.h"
#include "texture_impl.h"
#include "serialize_impl.h"
#include "version.h"

#include <array>

using namespace omm;

namespace
{

static const ommLibraryDesc g_ommbakeLibraryDesc =
{
    VERSION_MAJOR,
    VERSION_MINOR,
    VERSION_BUILD
};

} // namespace

OMM_API ommLibraryDesc OMM_CALL ommGetLibraryDesc()
{
    static_assert(VERSION_MAJOR == OMM_VERSION_MAJOR);
    static_assert(VERSION_MINOR == OMM_VERSION_MINOR);
    static_assert(VERSION_BUILD == OMM_VERSION_BUILD);
    return g_ommbakeLibraryDesc;
}

OMM_API ommResult OMM_CALL ommCpuCreateTexture(ommBaker baker, const ommCpuTextureDesc* desc, ommCpuTexture* outTexture)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;

    Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);

    if (desc == 0)
        return impl->GetLog().InvalidArg("texture desc was not set");
    if (GetHandleType(baker) != HandleType::CpuBaker)
        return impl->GetLog().InvalidArg("Baker was not created as the right type");

    StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();

    TextureImpl* implementation = Allocate<TextureImpl>(memoryAllocator, memoryAllocator, impl->GetLog());
    const ommResult result = implementation->Create(*desc);

    if (result == ommResult_SUCCESS)
    {
        *outTexture = CreateHandle<ommCpuTexture, TextureImpl>(implementation);
        return ommResult_SUCCESS;
    }

    Deallocate(memoryAllocator, implementation);
    return result;
}

OMM_API ommResult OMM_CALL ommCpuDestroyTexture(ommBaker baker, ommCpuTexture texture)
{
    if (texture == 0)
        return ommResult_INVALID_ARGUMENT;

    Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);

    if (GetHandleType(baker) != HandleType::CpuBaker)
        return impl->GetLog().InvalidArg("Baker was not created as the right type");

    StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();

    TextureImpl* textureImpl = GetHandleImpl<TextureImpl>(texture);

    Deallocate(memoryAllocator, textureImpl);

    return ommResult_SUCCESS;
}

OMM_API ommResult OMM_CALL ommCpuBake(ommBaker baker, const ommCpuBakeInputDesc* bakeInputDesc, ommCpuBakeResult* bakeResult)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;

    Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);

    if (bakeInputDesc == 0)
        return impl->GetLog().InvalidArg("input desc was not set");
    if (GetHandleType(baker) != HandleType::CpuBaker)
        return impl->GetLog().InvalidArg("Baker was not created as the right type");

    return (*impl).BakeOpacityMicromap(*bakeInputDesc, bakeResult);
}

OMM_API ommResult OMM_CALL ommCpuDestroyBakeResult(ommCpuBakeResult bakeResult)
{
    if (bakeResult == 0)
        return ommResult_INVALID_ARGUMENT;

    const StdAllocator<uint8_t>& memoryAllocator = (*(omm::Cpu::BakeOutputImpl*)bakeResult).GetStdAllocator();
    Deallocate(memoryAllocator, (omm::Cpu::BakeOutputImpl*)bakeResult);

    return ommResult_SUCCESS;
}

OMM_API ommResult OMM_CALL ommCpuGetBakeResultDesc(ommCpuBakeResult bakeResult, const ommCpuBakeResultDesc** desc)
{
    if (bakeResult == 0)
        return ommResult_INVALID_ARGUMENT;

    return (*(omm::Cpu::BakeOutputImpl*)bakeResult).GetBakeResultDesc(desc);
}

OMM_API ommResult OMM_CALL ommCpuSerialize(ommBaker baker, const ommCpuDeserializedDesc& desc, ommCpuSerializedResult* outResult)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;

    if (outResult == nullptr)
        return ommResult_INVALID_ARGUMENT;

    Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);

    if (GetHandleType(baker) != HandleType::CpuBaker)
        return impl->GetLog().InvalidArg("Baker was not created as the right type");

    StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();

    omm::Cpu::SerializeResultImpl* blobImpl = Allocate<omm::Cpu::SerializeResultImpl>(memoryAllocator, memoryAllocator, impl->GetLog());

    ommResult res = blobImpl->Serialize(desc);

    if (res == ommResult_SUCCESS)
    {
        *outResult = CreateHandle<ommCpuSerializedResult, omm::Cpu::SerializeResultImpl>(blobImpl);
    }
    else
    {
        Deallocate(memoryAllocator, blobImpl);
        *outResult = (ommCpuSerializedResult)nullptr;
    }

    return res;
}

OMM_API ommResult ommCpuGetSerializedResultDesc(ommCpuSerializedResult result, const ommCpuBlobDesc** desc)
{
    if (result == 0)
        return ommResult_INVALID_ARGUMENT;

    if (desc == nullptr)
        return ommResult_INVALID_ARGUMENT;

    omm::Cpu::SerializeResultImpl* impl = GetHandleImpl<omm::Cpu::SerializeResultImpl>(result);
    *desc = impl->GetDesc();
    return ommResult_SUCCESS;
}

OMM_API ommResult ommCpuDestroySerializedResult(ommCpuSerializedResult result)
{
    if (result == 0)
        return ommResult_INVALID_ARGUMENT;
    if (!CheckHandle<ommCpuSerializedResult, omm::Cpu::SerializeResultImpl>(result))
        return ommResult_INVALID_ARGUMENT;

    omm::Cpu::SerializeResultImpl* impl = GetHandleImpl<omm::Cpu::SerializeResultImpl>(result);
    const StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();
    Deallocate(memoryAllocator, impl);
    return ommResult_SUCCESS;
}

OMM_API ommResult ommCpuDeserialize(ommBaker baker, const ommCpuBlobDesc& desc, ommCpuDeserializedResult* outResult)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;

    Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);

    if (GetHandleType(baker) != HandleType::CpuBaker)
        return impl->GetLog().InvalidArg("Baker was not created as the right type");

    StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();

    omm::Cpu::DeserializedResultImpl* desImpl = Allocate<omm::Cpu::DeserializedResultImpl>(memoryAllocator, memoryAllocator, impl->GetLog());

    ommResult res = desImpl->Deserialize(desc);

    if (res == ommResult_SUCCESS)
    {
        *outResult = CreateHandle<ommCpuDeserializedResult, omm::Cpu::DeserializedResultImpl>(desImpl);
    }
    else
    {
        Deallocate(memoryAllocator, desImpl);
        *outResult = (ommCpuDeserializedResult)nullptr;
    }

    return res;
}

OMM_API ommResult ommCpuGetDeserializedDesc(ommCpuDeserializedResult result, const ommCpuDeserializedDesc** desc)
{
    if (result == 0)
        return ommResult_INVALID_ARGUMENT;
    if (!CheckHandle<ommCpuDeserializedResult, omm::Cpu::DeserializedResultImpl>(result))
        return ommResult_INVALID_ARGUMENT;
    if (desc == nullptr)
        return ommResult_INVALID_ARGUMENT;

    omm::Cpu::DeserializedResultImpl* desImpl = GetHandleImpl<omm::Cpu::DeserializedResultImpl>(result);

    *desc = desImpl->GetDesc();

    return ommResult_SUCCESS;
}

OMM_API ommResult ommCpuDestroyDeserializedResult(ommCpuDeserializedResult result)
{
    if (result == 0)
        return ommResult_INVALID_ARGUMENT;
    if (!CheckHandle<ommCpuDeserializedResult, omm::Cpu::DeserializedResultImpl>(result))
        return ommResult_INVALID_ARGUMENT;

    omm::Cpu::DeserializedResultImpl* impl = GetHandleImpl<omm::Cpu::DeserializedResultImpl>(result);

    const StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();
    Deallocate(memoryAllocator, impl);
    return ommResult_SUCCESS;
}

OMM_API ommResult OMM_CALL ommGpuGetStaticResourceData(ommGpuResourceType resource, uint8_t* data, size_t* outByteSize)
{
    return Gpu::OmmStaticBuffers::GetStaticResourceData(resource, data, outByteSize);
}

///< The Pipeline determines the 
OMM_API ommResult OMM_CALL ommGpuCreatePipeline(ommBaker baker, const ommGpuPipelineConfigDesc* config, ommGpuPipeline* outPipeline)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;

    Gpu::BakerImpl* bakePtr = GetHandleImpl<Gpu::BakerImpl>(baker);

    if (config == 0)
        return bakePtr->GetLog().InvalidArg("[Invalid Arg] - pipeline config desc must be provided");
    if (GetHandleType(baker) != HandleType::GpuBaker)
        return bakePtr->GetLog().InvalidArg("[Invalid Arg] - invalid baker type");
    return bakePtr->CreatePipeline(*config, outPipeline);
}

OMM_API ommResult OMM_CALL ommGpuGetPipelineDesc(ommGpuPipeline pipeline, const ommGpuPipelineInfoDesc** outPipelineDesc)
{
    if (pipeline == 0)
        return ommResult_INVALID_ARGUMENT;
    Gpu::PipelineImpl* impl = GetHandleImpl<Gpu::PipelineImpl>(pipeline);
    return impl->GetPipelineDesc(outPipelineDesc);
}

OMM_API ommResult OMM_CALL ommGpuDestroyPipeline(ommBaker baker, ommGpuPipeline pipeline)
{
    if (pipeline == 0)
        return ommResult_INVALID_ARGUMENT;
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;
    if (GetHandleType(baker) != HandleType::GpuBaker)
        return ommResult_INVALID_ARGUMENT;
    Gpu::BakerImpl* bakePtr = GetHandleImpl<Gpu::BakerImpl>(baker);
    return bakePtr->DestroyPipeline(pipeline);
}

OMM_API ommResult OMM_CALL ommGpuGetPreDispatchInfo(ommGpuPipeline pipeline, const ommGpuDispatchConfigDesc* config, ommGpuPreDispatchInfo* outPreBuildInfo)
{
    if (pipeline == 0)
        return ommResult_INVALID_ARGUMENT;
    if (config == 0)
        return ommResult_INVALID_ARGUMENT;
    Gpu::PipelineImpl* impl = GetHandleImpl<Gpu::PipelineImpl>(pipeline);
    return impl->GetPreDispatchInfo(*config, outPreBuildInfo);
}

OMM_API ommResult OMM_CALL ommGpuDispatch(ommGpuPipeline pipeline, const ommGpuDispatchConfigDesc* dispatchConfig, const ommGpuDispatchChain** outDispatchDesc)
{
    if (pipeline == 0)
        return ommResult_INVALID_ARGUMENT;
    if (dispatchConfig == 0)
        return ommResult_INVALID_ARGUMENT;
    Gpu::PipelineImpl* impl = GetHandleImpl<Gpu::PipelineImpl>(pipeline);
    return impl->GetDispatchDesc(*dispatchConfig, outDispatchDesc);
}

OMM_API ommResult OMM_CALL ommDebugSaveAsImages(ommBaker baker, const ommCpuBakeInputDesc* bakeInputDesc, const ommCpuBakeResultDesc* res, const ommDebugSaveImagesDesc* desc)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;
    if (bakeInputDesc == 0)
        return ommResult_INVALID_ARGUMENT;
    if (desc == 0)
        return ommResult_INVALID_ARGUMENT;
    if (GetHandleType(baker) == HandleType::CpuBaker)
    {
        Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);
        StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();
        return SaveAsImagesImpl(memoryAllocator, *bakeInputDesc, res, *desc);
    }
    else if (GetHandleType(baker) == HandleType::GpuBaker)
    {
        Gpu::BakerImpl* impl = GetHandleImpl<Gpu::BakerImpl>(baker);
        StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();
        return SaveAsImagesImpl(memoryAllocator, *bakeInputDesc, res, *desc);
    }
    else
        return ommResult_INVALID_ARGUMENT;
}

OMM_API ommResult OMM_CALL ommDebugGetStats(ommBaker baker, const ommCpuBakeResultDesc* res, ommDebugStats* out)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;

    if (GetHandleType(baker) == HandleType::CpuBaker)
    {
        Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);
        StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();
        return GetStatsImpl(memoryAllocator, res, out);
    }
    else if (GetHandleType(baker) == HandleType::GpuBaker)
    {
        Gpu::BakerImpl* impl = GetHandleImpl<Gpu::BakerImpl>(baker);
        StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();
        return GetStatsImpl(memoryAllocator, res, out);
    }
    else
        return ommResult_INVALID_ARGUMENT;
}

OMM_API ommResult OMM_CALL ommDebugSaveBinaryToDisk(ommBaker baker, const ommCpuBlobDesc& data, const char* path)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;
    if (path == 0)
        return ommResult_INVALID_ARGUMENT;
    if (GetHandleType(baker) == HandleType::CpuBaker)
    {
        Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);
        const Logger& log = (*impl).GetLog();
        return SaveBinaryToDiskImpl(log, data, path);
    }
    else if (GetHandleType(baker) == HandleType::GpuBaker)
    {
        Gpu::BakerImpl* impl = GetHandleImpl<Gpu::BakerImpl>(baker);
        const Logger& log = (*impl).GetLog();
        return SaveBinaryToDiskImpl(log, data, path);
    }
    else
        return ommResult_INVALID_ARGUMENT;
}

OMM_API ommResult OMM_CALL ommCreateBaker(const ommBakerCreationDesc* desc, ommBaker* baker)
{
    if (desc == 0)
        return ommResult_INVALID_ARGUMENT;

    StdMemoryAllocatorInterface o = 
    {
        .Allocate = desc->memoryAllocatorInterface.allocate,
        .Reallocate = desc->memoryAllocatorInterface.reallocate,
        .Free = desc->memoryAllocatorInterface.free,
        .UserArg = desc->memoryAllocatorInterface.userArg
    };

    CheckAndSetDefaultAllocator(o);
    StdAllocator<uint8_t> memoryAllocator(o);

    if (desc->type == ommBakerType_CPU)
    {
        Cpu::BakerImpl* implementation = Allocate<Cpu::BakerImpl>(memoryAllocator, memoryAllocator);
        const ommResult result = implementation->Create(*desc);

        if (result == ommResult_SUCCESS)
        {
            *baker = CreateHandle<ommBaker, Cpu::BakerImpl>(implementation);
            return ommResult_SUCCESS;
        }

        Deallocate(memoryAllocator, implementation);
    }
    else if (desc->type == ommBakerType_GPU)
    {
        Gpu::BakerImpl* implementation = Allocate<Gpu::BakerImpl>(memoryAllocator, memoryAllocator);
        const ommResult result = implementation->Create(*desc);

        if (result == ommResult_SUCCESS)
        {
            *baker = CreateHandle<ommBaker, Gpu::BakerImpl>(implementation);
            return ommResult_SUCCESS;
        }

        Deallocate(memoryAllocator, implementation);
    }
    else {
        return ommResult_INVALID_ARGUMENT;
    }
   
    return ommResult_FAILURE;
}

OMM_API ommResult OMM_CALL ommDestroyBaker(ommBaker baker)
{
    if (baker == 0)
        return ommResult_INVALID_ARGUMENT;

    const HandleType type = GetHandleType(baker);
    if (type == HandleType::CpuBaker)
    {
        Cpu::BakerImpl* impl = GetHandleImpl<Cpu::BakerImpl>(baker);
        StdAllocator<uint8_t>& memoryAllocator = (*impl).GetStdAllocator();
        Deallocate(memoryAllocator, impl);
        return ommResult_SUCCESS;
    }
    else  if (type == HandleType::GpuBaker)
    {
        Gpu::BakerImpl* bakePtr = GetHandleImpl<Gpu::BakerImpl>(baker);
        StdAllocator<uint8_t>& memoryAllocator = (*bakePtr).GetStdAllocator();
        Deallocate(memoryAllocator, bakePtr);
        return ommResult_SUCCESS;
    }
    return ommResult_FAILURE;
}