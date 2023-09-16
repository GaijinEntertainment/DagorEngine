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

/// This generic hash table is mostly useful for cases where its hard to derive
/// a unique 32 or 64-bit key for entries. It is much slower than HashTable and
/// HashTable64, but offers support for retaining arbitrary data in the keys as
/// well as stable value pointers which can be a great help in tools such as
/// builders.
///
/// Generic hash tables are always backed by a heap. This is usually OK because
/// they're best suited to tools and not the game runtime.
///
/// The following restrictions apply:
/// - The key type must be POD
/// - The key type must be comparable with operator==
/// - The key type must be hashable by calling HashFunctions::Hash(KeyType kt)
///   - Re-open the namespace and override as needed for your own struct-based key types.
/// - Constructors or destructors are called for value types.

#include "Platform.h"

namespace HashFunctions
{
  template<typename T> uint32_t Hash(T v) {return HashTypeOverload(v); }
  uint32_t HashTypeOverload(uint32_t v) { return v; }
  uint32_t HashTypeOverload(int32_t v) { return v; }
  uint32_t HashTypeOverload(uint64_t v) { return uint32_t(v) ^ uint32_t(v >> 32); }
  uint32_t HashTypeOverload(int64_t v) { return uint32_t(v) ^ uint32_t(uint64_t(v) >> 32); }
}

constexpr unsigned int BLOCK_SIZE = 4 * 1024;

template<typename ElemType> class HashTableAllocator
{
private:
  union ElemTypeBlock
  {
    ElemTypeBlock* m_Next = nullptr;
    ElemType m_Element;
  };

public:
  ElemType* AllocElement()
  {
    if (m_FreeList == nullptr)
    {
      AllocInternal();
    }

    ElemTypeBlock* element = m_FreeList;
    m_FreeList = m_FreeList->m_Next;
    return &element->m_Element;
  }

  void FreeElement(ElemType* element)
  {
    ElemTypeBlock* elemTypeBlock = (ElemTypeBlock*)element;
    elemTypeBlock->m_Next = m_FreeList;
    m_FreeList = elemTypeBlock;
  }

  ~HashTableAllocator()
  {
    while ( m_BlockList )
    {
      void* next = *(void**)((uintptr_t)m_BlockList + BLOCK_SIZE - sizeof(void*));
      VirtualMemoryFree(m_BlockList, BLOCK_SIZE);
      m_BlockList = next;
    }
  }

private: 

  void AllocInternal()
  {
    void* block = VirtualMemoryAlloc(BLOCK_SIZE);
    void** next = (void**)((uintptr_t)block + BLOCK_SIZE - sizeof(void*));
    
    *next = m_BlockList;
    m_BlockList = block;

    ElemTypeBlock* element = (ElemTypeBlock*)block;
    while ( ((uintptr_t)element + sizeof(ElemTypeBlock)) < (uintptr_t)next )
    {
      element->m_Next = m_FreeList;
      m_FreeList = element;
      element++;
    }
  }

  ElemTypeBlock* m_FreeList = nullptr;
  void* m_BlockList = nullptr;
};

template <typename KeyType, typename ValueType>
class GenericHashTable
{
private:
  struct Elem
  {
    uint32_t  m_Hash;
    KeyType   m_Key;
    ValueType m_Value;
    Elem*     m_Next;
  };

  size_t        m_Capacity;     // Always a power of two
  size_t        m_Count;
  Elem**        m_Table;
  HashTableAllocator<Elem> m_Allocator;

public:
  GenericHashTable() { Reset(); }

  void Init()
  {
    Reset();
  }

  void FreeAll()
  {
    size_t capacity = m_Capacity;
    for (size_t i = 0; i < capacity; ++i)
    {
      Elem* chain = m_Table[i];
      while (chain)
      {
        Elem* next = chain->m_Next;
        chain->m_Value.~ValueType();
        chain = next;
      }
    }
    m_Allocator.~HashTableAllocator<Elem>();
    new(&m_Allocator) HashTableAllocator<Elem>;

    m_Count = 0;
    m_Capacity = 0;
    m_Table = nullptr;
  }

  void Destroy()
  {
    FreeAll();
  }

  size_t GetCount() const { return m_Count; }

  size_t GetCapacity() const { return m_Capacity; }

  /// Locate an existing value associated with a key, and return a pointer to it.
  /// The pointer is guaranteed to stay valid as long as the hash table is, or until the key is removed.
  ValueType* Find(const KeyType& key)
  {
    const uint32_t hash = HashFunctions::Hash(key);
    return FindInternal(hash, key);
  }

  /// Insert a new element into the table.
  /// If an existing element with the same key exists, a pointer to it is returned.
  /// The pointer is guaranteed to stay valid as long as the hash table is, or until the key is removed.
  ValueType* Insert(const KeyType& key)
  {
    const uint32_t hash = HashFunctions::Hash(key);
    uint32_t index = hash & (m_Capacity - 1);

    if (ValueType* existing = FindInternal(hash, key))
    {
      return existing;
    }

    if (m_Count * 4 >= m_Capacity * 3)    // Allow 75% fill
    {
      Grow();
      index = hash & (m_Capacity - 1);
    }

    ++m_Count;

    Elem* elem = m_Allocator.AllocElement();
    elem->m_Hash = hash;
    elem->m_Key = key;
    elem->m_Next = m_Table[index];

    m_Table[index] = elem;

    return new (&elem->m_Value) ValueType;
  }

  /// Remove the specified key, and free its value.
  /// Returns true if the key was found.
  bool Remove(const KeyType& key)
  {
    const uint32_t hash = HashFunctions::Hash(key);
    uint32_t index = hash & (m_Capacity - 1);

    Elem** cloc = &m_Table[index];

    for (;;)
    {
      Elem* e = *cloc;
      if (e->m_Hash == hash && e->m_Key == key)
      {
        *cloc = e->m_Next;
        e->m_Value.~ValueType();
        m_Allocator.FreeElement(e);
        return true;
      }
      cloc = &e->m_Next;
    }

    return false;
  }

public:
  template <typename AccessFn>
  class Iterator : private AccessFn
  {
    size_t  m_Index;
    size_t  m_Max;
    Elem**  m_Table;
    Elem*   m_Curr;

    typedef typename AccessFn::ResultType ResultType;

  public:
    Iterator() : m_Index(0), m_Max(0), m_Table(nullptr), m_Curr(nullptr) {}

    explicit Iterator(GenericHashTable* tab, bool is_end)
      : m_Index(0)
      , m_Max(tab->m_Capacity)
      , m_Table(tab->m_Table)
      , m_Curr(nullptr)
    {
      if (!is_end)
        Step();
      else
        m_Index = m_Max;
    }

  private:
    void Step()
    {
      if (Elem* e = m_Curr)
      {
        m_Curr = e->m_Next;
      }

      while (!m_Curr && m_Index < m_Max)
      {
        m_Curr = m_Table[m_Index++];
      }
    }

  public:
    Iterator& operator++()
    {
      Step();
      return *this;
    }

    ResultType operator*() const
    {
      if (!m_Curr)
      {
        DebugBreak(); // Invalid iterator
      }
      return this->Access(m_Curr);
    }

    bool operator==(const Iterator& other) const
    {
      return m_Table == other.m_Table && m_Index == other.m_Index && m_Curr == other.m_Curr;
    }

    bool operator!=(const Iterator& other) const
    {
      return !(*this == other);
    }
  };

  struct KeyAccess
  {
    typedef const KeyType& ResultType;
    ResultType Access(Elem* e) const { return e->m_Key; }
  };

  struct ValueAccess
  {
    typedef ValueType& ResultType;
    ResultType Access(Elem* e) const { return e->m_Value; }
  };

  typedef Iterator<KeyAccess> KeyIterator;
  typedef Iterator<ValueAccess> ValueIterator;

  struct KeyProxy
  {
    KeyProxy(GenericHashTable* t) : m_Table(t) {}
  
    GenericHashTable* m_Table;

    KeyIterator begin() { return KeyIterator(m_Table, false); }
    KeyIterator end() { return KeyIterator(m_Table, true); }
  };

  struct ValueProxy
  {
    ValueProxy(GenericHashTable* t) : m_Table(t) {}
  
    GenericHashTable* m_Table;

    ValueIterator begin() { return ValueIterator(m_Table, false); }
    ValueIterator end() { return ValueIterator(m_Table, true); }
  };

  KeyProxy Keys() { return KeyProxy(this); }
  ValueProxy Values() { return ValueProxy(this); }

private:
  ValueType* FindInternal(uint32_t hash, const KeyType& key)
  {
    if (!m_Table)
    {
      return nullptr;
    }

    const uint32_t index = hash & (m_Capacity - 1);

    Elem* chain = m_Table[index];
    while (chain)
    {
      if (hash == chain->m_Hash && key == chain->m_Key)
      {
        return &chain->m_Value;
      }

      chain = chain->m_Next;
    }

    return nullptr;
  }

  void Reset()
  {
    m_Capacity  = 0;
    m_Count     = 0;
    m_Table     = nullptr;
  }

  void Grow()
  {
    const size_t old_capacity = m_Capacity;
    const size_t new_capacity = old_capacity ? old_capacity * 2 : 64;

    Elem** old_table = m_Table;
    Elem** new_table = (Elem**) VirtualMemoryAlloc(sizeof(Elem*) * new_capacity);
    memset(new_table, 0, sizeof(Elem*) * new_capacity);

    // Rehash
    for (size_t i = 0; i < old_capacity; ++i)
    {
      Elem* chain = old_table[i];

      while (chain)
      {
        uint32_t index = chain->m_Hash & (new_capacity - 1);
        Elem* next = chain->m_Next;
        chain->m_Next = new_table[index];
        new_table[index] = chain;
        chain = next;
      }
    }

    // Commit
    m_Capacity  = new_capacity;
    m_Table     = new_table;

    VirtualMemoryFree(old_table, old_capacity);
  }
};
