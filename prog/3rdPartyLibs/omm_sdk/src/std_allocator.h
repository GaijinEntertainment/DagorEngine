/*
Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#if _WIN32
#include <malloc.h>

inline void* AlignedMalloc(void* UserArg, size_t size, size_t alignment)
{
    return _aligned_malloc(size, alignment);
}

inline void* AlignedRealloc(void* UserArg, void* memory, size_t size, size_t alignment)
{
    return _aligned_realloc(memory, size, alignment);
}

inline void AlignedFree(void* UserArg, void* memory)
{
    _aligned_free(memory);
}

#elif __linux__
#include <cstdlib>
#include <alloca.h>
#define _alloca alloca

inline uint8_t* AlignMemory(uint8_t* memory, size_t alignment)
{
    return (uint8_t*)((size_t(memory) + alignment - 1) & ~(alignment - 1));
}

inline void* AlignedMalloc(void* UserArg, size_t size, size_t alignment)
{
    uint8_t* memory = (uint8_t*)malloc(size + sizeof(uint8_t*) + alignment - 1);

    if (memory == nullptr)
        return nullptr;

    uint8_t* alignedMemory = AlignMemory(memory + sizeof(uint8_t*), alignment);
    uint8_t** memoryHeader = (uint8_t**)alignedMemory - 1;
    *memoryHeader = memory;

    return alignedMemory;
}

inline void* AlignedRealloc(void* UserArg, void* memory, size_t size, size_t alignment)
{
    if (memory == nullptr)
        return AlignedMalloc(UserArg, size, alignment);

    uint8_t** memoryHeader = (uint8_t**)memory - 1;
    uint8_t* oldMemory = *memoryHeader;
    uint8_t* newMemory = (uint8_t*)realloc(oldMemory, size + sizeof(uint8_t*) + alignment - 1);

    if (newMemory == nullptr)
        return nullptr;

    if (newMemory == oldMemory)
        return memory;

    uint8_t* alignedMemory = AlignMemory(newMemory + sizeof(uint8_t*), alignment);
    memoryHeader = (uint8_t**)alignedMemory - 1;
    *memoryHeader = newMemory;

    return alignedMemory;
}

inline void AlignedFree(void* UserArg, void* memory)
{
    if (memory == nullptr)
        return;

    uint8_t** memoryHeader = (uint8_t**)memory - 1;
    uint8_t* oldMemory = *memoryHeader;
    free(oldMemory);
}

#endif

struct StdMemoryAllocatorInterface
{
    void* (*Allocate)(void* userArg, size_t size, size_t alignment) = nullptr;
    void* (*Reallocate)(void* userArg, void* memory, size_t size, size_t alignment) = nullptr;
    void (*Free)(void* userArg, void* memory) = nullptr;
    void* UserArg = nullptr;

    bool operator==(const StdMemoryAllocatorInterface& o) const {
        return 
            Allocate == o.Allocate &&
            Reallocate == o.Reallocate &&
            Free == o.Free &&
            UserArg == o.UserArg;
    }
};

inline void CheckAndSetDefaultAllocator(StdMemoryAllocatorInterface& memoryAllocatorInterface)
{
    if (memoryAllocatorInterface.Allocate != nullptr)
        return;

    memoryAllocatorInterface.Allocate = AlignedMalloc;
    memoryAllocatorInterface.Reallocate = AlignedRealloc;
    memoryAllocatorInterface.Free = AlignedFree;
}

template<typename T>
struct StdAllocator
{
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::false_type is_always_equal;
    typedef std::false_type has_no_allocator_construct;

    StdAllocator(const StdMemoryAllocatorInterface& memoryAllocatorInterface) : m_Interface(memoryAllocatorInterface)
    { CheckAndSetDefaultAllocator(m_Interface); }

    template<class U>
    StdAllocator(const StdAllocator<U>& allocator) : m_Interface(allocator.GetInterface())
    {}

    StdAllocator<T>& operator= (const StdAllocator<T>& allocator)
    {
        m_Interface = allocator.GetInterface();
        return *this;
    }

    T* allocate(size_t n, size_t align = alignof(T)) noexcept
    {
        return (T*)m_Interface.Allocate(m_Interface.UserArg, n * sizeof(T), align);
    }

    void deallocate(T* memory, size_t) noexcept
    { m_Interface.Free(m_Interface.UserArg, memory); }

    T* reallocate(T* memory, size_t n, size_t align = alignof(T)) noexcept
    { return (T*)m_Interface.Reallocate(m_Interface.UserArg, memory, n * sizeof(T), align); }

    const StdMemoryAllocatorInterface& GetInterface() const
    { return m_Interface; }

    template<typename U>
    using other = StdAllocator<U>;

private:
    StdMemoryAllocatorInterface m_Interface = {};
};

template<typename T>
bool operator== (const StdAllocator<T>& left, const StdAllocator<T>& right)
{
    return left.GetInterface() == right.GetInterface();
}

template<typename T>
bool operator!= (const StdAllocator<T>& left, const StdAllocator<T>& right)
{
    return !operator==(left, right);
}

template<typename T>
inline T GetAlignedSize(const T& x, uint32_t alignment)
{
    return ((x + alignment - 1) / alignment) * alignment;
}

template<typename T>
inline T* Align(T* x, size_t alignment)
{
    return (T*)(((size_t)x + alignment - 1) / alignment * alignment);
}

template <typename T, uint32_t N>
constexpr uint32_t GetCountOf(T const (&)[N])
{
    return N;
}

template <typename T>
constexpr uint32_t GetCountOf(const std::vector<T>& v)
{
    return (uint32_t)v.size();
}

template <typename T, uint32_t N>
constexpr uint32_t GetCountOf(const std::array<T, N>& v)
{
    return (uint32_t)v.size();
}

template<typename T, typename U>
constexpr uint32_t GetOffsetOf(U T::*member)
{
    return (uint32_t)((const char*)&((T*)nullptr->*member) - (const char*)nullptr);
}

template<typename T, typename... Args>
constexpr void Construct(T* objects, size_t number, Args&&... args)
{
    for (size_t i = 0; i < number; i++)
        new (objects + i) T(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
inline T* Allocate(const StdAllocator<uint8_t>& allocator, Args&&... args)
{
    const auto& lowLevelAllocator = allocator.GetInterface();
    T* object = (T*)lowLevelAllocator.Allocate(lowLevelAllocator.UserArg, sizeof(T), alignof(T));

    new (object) T(std::forward<Args>(args)...);
    return object;
}

template<typename T, typename... Args>
inline T* AllocateArray(StdAllocator<uint8_t>& allocator, size_t arraySize, Args&&... args)
{
    const auto& lowLevelAllocator = allocator.GetInterface();
    T* array = (T*)lowLevelAllocator.Allocate(lowLevelAllocator.UserArg, arraySize * sizeof(T), alignof(T));

    for (size_t i = 0; i < arraySize; i++)
        new (array + i) T(std::forward<Args>(args)...);

    return array;
}

template<typename T>
inline void Deallocate(const StdAllocator<uint8_t>& allocator, T* object)
{
    if (object == nullptr)
        return;

    object->~T();

    const auto& lowLevelAllocator = allocator.GetInterface();
    lowLevelAllocator.Free(lowLevelAllocator.UserArg, object);
}

template<typename T>
inline void DeallocateArray(StdAllocator<uint8_t>& allocator, T* array, size_t arraySize)
{
    if (array == nullptr)
        return;

    for (size_t i = 0; i < arraySize; i++)
        (array + i)->~T();

    const auto& lowLevelAllocator = allocator.GetInterface();
    lowLevelAllocator.Free(lowLevelAllocator.UserArg, array);
}

//==============================================================================================================================

template<typename T>
using Vector = std::vector<T, StdAllocator<T>>;

template<typename U, typename T>
using UnorderedMap = std::unordered_map<U, T, std::hash<U>, std::equal_to<U>, StdAllocator<std::pair<U, T>>>;

using String = std::basic_string<char, std::char_traits<char>, StdAllocator<char>>;

//==============================================================================================================================

constexpr size_t STACK_ALLOC_MAX_SIZE = 65536;

template<typename T>
constexpr size_t CountStackAllocationSize(size_t arraySize)
{
    return arraySize * sizeof(T) + alignof(T);
}

#define ALLOCATE_SCRATCH(device, T, arraySize) \
    (CountStackAllocationSize<T>(arraySize) <= STACK_ALLOC_MAX_SIZE) ? \
    Align<T>(((arraySize) ? (T*)_alloca(CountStackAllocationSize<T>(arraySize)) : nullptr), alignof(T)) : \
    AllocateArray<T>((device).GetStdAllocator(), arraySize);

#define FREE_SCRATCH(device, array, arraySize) \
    if (array != nullptr && CountStackAllocationSize<decltype(array[0])>(arraySize) > STACK_ALLOC_MAX_SIZE) \
        DeallocateArray((device).GetStdAllocator(), array, arraySize);

#define STACK_ALLOC(T, arraySize) \
    Align<T>(((arraySize) ? (T*)_alloca(CountStackAllocationSize<T>(arraySize)) : nullptr), alignof(T))
