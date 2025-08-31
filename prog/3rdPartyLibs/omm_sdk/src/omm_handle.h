/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "omm.h"

#pragma once

namespace omm
{
    enum class HandleType : uint8_t
    {
        Reserved, // keep zero reserved
        GpuBaker,
        Pipeline,
        CpuBaker,
        Texture,
        BakeResult,
        SerializeResult,
        DeserializeResult
    };

    template<class THandlePtr, class T>
    inline bool CheckHandle(THandlePtr handle) {
        return static_cast<HandleType>((uintptr_t)handle & 0x7ull) == T::kHandleType;
    }

    template<class THandlePtr>
    inline HandleType GetHandleType(THandlePtr handle) {
        return static_cast<HandleType>((uintptr_t)handle & 0x7ull);
    }

    template<class T, class THandlePtr>
    inline T* GetHandleImpl(THandlePtr handle) {
        return reinterpret_cast<T*>((uintptr_t)handle & 0xFFFFFFFFFFFFFFF8ull);
    }

    template<class T, class THandlePtr>
    inline T* GetCheckedHandleImpl(THandlePtr handle) {
        T* ptr = reinterpret_cast<T*>((uintptr_t)handle & 0xFFFFFFFFFFFFFFF8ull);
        HandleType type = (HandleType)((uintptr_t)handle & 0x7ull);
        return T::kHandleType == type ? ptr : nullptr;
    }

    template<class THandlePtr, class T>
    inline THandlePtr CreateHandle(T* impl) {
        return (THandlePtr)((uintptr_t)(impl) | (uintptr_t)T::kHandleType);
    }

} // namespace