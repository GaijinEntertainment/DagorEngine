#pragma once

/*
Copyright (c) 2017, Insomniac Games
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CacheSimInternals.h"
#include <algorithm>

/// Describes structures that go in the output standard DataFileWriter-based files
/// after a cache simulation has been run.
namespace CacheSim
{

  struct SerializedModuleEntry
  {
    uint64_t    m_ImageBase;
    uint64_t    m_ImageSegmentOffset; // Linux only
    uint32_t    m_SizeBytes;
    uint32_t    m_StringOffset;
  };
  static_assert(sizeof(SerializedModuleEntry) == 24, "bump version if you're changing this");

  struct SerializedNode
  {
    uint64_t m_Rip;
    uint32_t m_StackIndex;
    uint32_t m_Stats[kAccessResultCount];
    uint32_t m_Padding;
  };
  static_assert(sizeof(SerializedNode) == 48, "bump version if you're changing this");

  inline double BadnessValue(const uint32_t (&stats)[kAccessResultCount])
  {
    uint64_t misses = stats[CacheSim::kL2DMiss];
    uint32_t instructions = stats[CacheSim::kInstructionsExecuted];
    return double(misses * misses) / instructions;
  }

  struct SerializedSymbol
  {
    uintptr_t   m_Rip;
    uint8_t     m_Pad[4];
    uint32_t    m_FileName;
    uint32_t    m_SymbolName;
    uint32_t    m_ModuleIndex;
    uint32_t    m_LineNumber;
    uint32_t    m_Displacement;
  };
  static_assert(sizeof(SerializedSymbol) == 32, "bump version if you're changing this");

  static constexpr uint32_t kCurrentVersion = 0x2;

  template <typename T>
  const T* serializedOffset(const void* base, uint32_t offset)
  {
    return reinterpret_cast<const T*>(reinterpret_cast<uint64_t>(base) + offset);
  }

  struct SerializedHeader
  {
  public:
    uint32_t    m_Magic;
    uint32_t    m_Version;

    uint32_t    m_ModuleOffset;
    uint32_t    m_ModuleCount;
    uint32_t    m_ModuleStringOffset;
    uint32_t    m_FrameOffset;
    uint32_t    m_FrameCount;
    uint32_t    m_StatsOffset;
    uint32_t    m_StatsCount;

    uint32_t    m_SymbolOffset;       // If these are all 0 it means the file is not yet resolved and it lacks this last section.
    uint32_t    m_SymbolCount;

    uint32_t    m_SymbolTextOffset;

  public:
    uint32_t GetModuleCount() const { return m_ModuleCount; }
    const SerializedModuleEntry* GetModules() const { return serializedOffset<SerializedModuleEntry>(this, m_ModuleOffset); }

    const char* GetModuleName(const SerializedModuleEntry& module) const
    {
      return serializedOffset<char>(this, m_ModuleStringOffset + module.m_StringOffset);
    }

    const uintptr_t* GetStacks() const { return serializedOffset<uintptr_t>(this, m_FrameOffset); }
    uint32_t GetStackCount() const { return m_FrameCount; }

    const SerializedNode* GetStats() const { return serializedOffset<SerializedNode>(this, m_StatsOffset); }
    uint32_t GetStatCount() const { return m_StatsCount; }

    const SerializedSymbol* GetSymbols() const { return serializedOffset<SerializedSymbol>(this, m_SymbolOffset); }
    uint32_t GetSymbolCount() const { return m_SymbolCount; }

    const SerializedSymbol* FindSymbol(const uintptr_t rip) const
    {
      const SerializedSymbol* b = GetSymbols();
      const SerializedSymbol* e = b + GetSymbolCount();

      const SerializedSymbol* s = std::lower_bound(b, e, rip, [](const SerializedSymbol& sym, uintptr_t rip) -> bool
      {
        return sym.m_Rip < rip;
      });

      if (s == e || s->m_Rip != rip)
      {
        return nullptr;
      }

      return s;
    }
  };

}
