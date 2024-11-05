#ifndef _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_DATAWALKERWRAPPER_H_
#define _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_DATAWALKERWRAPPER_H_
#pragma once

#include <daScript/daScript.h>
#include <debug/dag_assert.h>
#include "dasDeque.h"


namespace das
{

  template <template <typename A> class T, class F>
  void typeTemplateSelect(Type type, F && fn)
  {
    switch (type)
    {
      case tBool: fn(T<bool>()); break;
      case tInt64: fn(T<int64_t>()); break;
      case tUInt64: fn(T<uint64_t>()); break;
      case tInt: fn(T<int32_t>()); break;
      case tUInt: fn(T<uint32_t>()); break;
      case tFloat: fn(T<float>()); break;
      case tDouble: fn(T<double>()); break;
      case tString: fn(T<char *>()); break;
      case tInt2: fn(T<int2>()); break;
      case tInt3: fn(T<int3>()); break;
      case tInt4: fn(T<int4>()); break;
      case tUInt2: fn(T<uint2>()); break;
      case tUInt3: fn(T<uint3>()); break;
      case tUInt4: fn(T<uint4>()); break;
      case tFloat2: fn(T<float2>()); break;
      case tFloat3: fn(T<float3>()); break;
      case tFloat4: fn(T<float4>()); break;
      case tRange: fn(T<range>()); break;
      case tURange: fn(T<urange>()); break;
      /*
      tStructure,
      tHandle,
      tEnumeration,
      tPointer,
      tIterator,
      tArray,
      tTable,
      tBlock
        break;*/
      default:
        G_ASSERT(false);
        break;
    }
  }


  template <typename K>
  struct AllowDirectTableSerialization
  {
    static const bool value = false;
  };

  template <>
  struct AllowDirectTableSerialization<char *>
  {
    static const bool value = true;
  };


  inline bool allowDirectTableSerialization(TypeInfo * ti)
  {
    bool result = false;
    typeTemplateSelect<AllowDirectTableSerialization>(ti->firstType->type,
                                                      [&result](auto && allower)
                                                      {
                                                        result = allower.value;
                                                      });
    return result;
  }


  struct ArrayElementTag {};

  template <typename T>
  class DataWalkerWrapper : protected DataWalker
  {
    typedef DataWalkerWrapper<T>  TThisType;
  public:
    template <typename ... Args>
    explicit DataWalkerWrapper(das::Context & context_, Args && ... args)
      : context(context_)
    {
      stack.emplace_back(state, context, das::forward<Args>(args)...);
    }

    void process(vec4f & f, TypeInfo * ti, bool read)
    {
      G_ASSERT(ti);
      state = State::Root;
      fieldName = nullptr;
      indexCounter = 1;
      ptrMap.clear();
      reading = read;
      if (ti->flags & TypeInfo::flag_refType)
      {
          walk(cast<char *>::to(f), ti);
      }
      else
      {
          walk((char *)&f, ti);
      }

      G_ASSERT(stack.size() == 1);
    }

    intptr_t createInstance(TypeInfo * ti)
    {
      G_ASSERT(ti);
      G_ASSERT(ti->type == das::tStructure);
      auto size = das::getTypeSize(ti);
      StructInfo * si = ti->structType;
      G_ASSERT(si);
      auto data = context.allocate(size);
      if ( si->init_mnh!=0 )
      {
        if (auto fn = context.fnByMangledName(si->init_mnh))
          context.callWithCopyOnReturn(fn, nullptr, data, 0);
        else
          G_ASSERT(0);
      }
      else
      {
        memset(data, 0, size);
      }

      return intptr_t(data);
    }

  private:
    enum class State
    {
      Root,
      Field,
      ComplexField,
      Structure,
      TableKey,
      Array,
      Ptr
    };

    typedef T HandlerType;
    struct WalkerState
    {
      template <typename ... Args>
      explicit WalkerState(State state_, Args && ... args)
        : handler(das::forward<Args>(args)...)
        , state(state_)
      {

      }

      State state;
      HandlerType handler;
    };

    State state = State::Root;
    const char * fieldName = nullptr;
    bool skipNextObj = false;
    das::unordered_map<intptr_t, das::pair<intptr_t, TypeInfo*>> ptrMap;
    intptr_t indexCounter = 1;
    das::Context & context;

    das::das_deque<WalkerState> stack;
    void push(State newState)
    {
      if (fieldName)
      {
        stack.emplace_back(newState, ref(stack.back().handler), fieldName);
        fieldName = nullptr;
      }
      else if (state == State::Array)
      {
        stack.emplace_back(newState, ref(stack.back().handler), ArrayElementTag());
      }
      else
      {
        stack.emplace_back(newState, ref(stack.back().handler));
      }
      state = newState;
    }

    void pop()
    {
      if (stack.size() < 2)
      {
        G_ASSERT(stack.size() < 2);
        return;
      }

      stack.pop_back();
      state = stack.back().state;
    }

    HandlerType & handler()
    {
      return stack.back().handler;
    }

    void beforeStructure ( char * ps, StructInfo * si ) override
    {
      if (state != State::Ptr)
      {
        push(State::Structure);
        handler().handleStructureStart();
      }
    }

    void afterStructure ( char * ps, StructInfo * si ) override
    {
      if (state != State::Ptr)
      {
        handler().handleStructureEnd();
        pop();
      }
    }

    void beginField(const char * name)
    {
      G_ASSERT(name);
      G_ASSERT(state == State::Structure || state == State::Ptr || state == State::TableKey);
      state = State::Field;
      fieldName = name;
    }

    void endField()
    {
      if (state == State::ComplexField)
      {
        pop();
      }
      else
      {
        state = stack.back().state;
      }

      fieldName = nullptr;
    }

    void beforeStructureField ( char * ps, StructInfo * si, char * pv, VarInfo * vi, bool last ) override
    {
      beginField(vi->name);
    }

    void afterStructureField ( char * ps, StructInfo * si, char * pv, VarInfo * vi, bool last ) override
    {
      endField();
    }

    template <typename V>
    const char * extractString(V && value)
    {
      G_ASSERT(0);
      return nullptr;
    }

    const char * extractString(char * & value)
    {
      return value ? value : "";
    }

    template <typename V>
    void handleSimpleType(V && value)
    {
      switch (state)
      {
        case State::Field:
          G_ASSERT(fieldName);
          handler().handleField(fieldName, value);
          break;
        case State::Array:
          handler().handleArrayElement(value);
          break;
        case State::Root:
          handler().handleSimpleType(value);
          break;
        case State::TableKey:
          beginField(extractString(value));
          break;
        default: G_ASSERT(false);
      }
    }

    void beforeArrayData ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) override
    {
    }

    void afterArrayData ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) override
    {
    }

    void beforeArrayElement ( char * pa, TypeInfo * ti, char * pe, uint32_t index, bool last ) override {}
    void afterArrayElement ( char * pa, TypeInfo * ti, char * pe, uint32_t index, bool last ) override {}
    void beforeDim ( char * pa, TypeInfo * ti ) override
    {
      push(State::Array);
      handler().handleArrayStart();
    }
    void afterDim ( char * pa, TypeInfo * ti ) override
    {
      handler().handleArrayEnd();
      pop();
    }

    void beforeArray ( Array * pa, TypeInfo * ti ) override
    {
      push(State::Array);
      handler().handleArrayStart();
      if (reading)
      {
        uint32_t capacity = handler().getArraySize();
        uint32_t stride = getTypeSize(ti->firstType);
        array_resize(context, *pa, capacity, stride, true, /*at*/nullptr);
      }
    }

    void afterArray ( Array * pa, TypeInfo * ti ) override
    {
      handler().handleArrayEnd();
      pop();
    }


    void beforeTable ( Table * pa, TypeInfo * ti ) override
    {
      if (!allowDirectTableSerialization(ti))
      {
        push(State::Array);
        handler().handleArrayStart();
      }
      else
      {
        push(State::Structure);
        handler().handleStructureStart();
      }

      if (reading)
      {
        table_clear(context, *pa, /*at*/nullptr);
      }
    }

    void beforeTableKey ( Table * pa, TypeInfo * ti, char * pk, TypeInfo * ki, uint32_t index, bool last ) override
    {
      if (allowDirectTableSerialization(ti))
      {
        push(State::TableKey);
      }
      else
      {
        push(State::Structure);
        beginField("k");
      }
    }

    void afterTableKey ( Table * pa, TypeInfo * ti, char * pk, TypeInfo * ki, uint32_t index, bool last ) override
    {
      if (!allowDirectTableSerialization(ti))
        endField();
    }
    void beforeTableValue ( Table * pa, TypeInfo * ti, char * pv, TypeInfo * kv, uint32_t index, bool last ) override
    {
      if (!allowDirectTableSerialization(ti))
        beginField("v");
    }
    void afterTableValue ( Table * pa, TypeInfo * ti, char * pv, TypeInfo * kv, uint32_t index, bool last ) override
    {
      endField();
      pop();
    }


    template <typename K, bool allowDirect>
    struct TableLoaderImpl
    {
      typedef K KeyType;
      void loadTable(TThisType & walker, Table & table, TypeInfo & ti)
      {
        Context & context = walker.context;
        G_ASSERT(ti.type == tTable);
        G_ASSERT(ti.secondType);
        int valueTypeSize = getTypeSize(ti.secondType);
        TableHash<KeyType> tableHash(&context, valueTypeSize);
        int arraySize = (int)walker.handler().getArraySize();
        // PVS studio went crazy about that simple cycle and claimed that the condition is always false.
        // This is the 8th try to make it happy about that
        //-V:suspiciousCounter:621,654
        for (int suspiciousCounter = 0; suspiciousCounter < arraySize; ++suspiciousCounter)
        {
          walker.push(State::Structure);

          KeyType key = KeyType();
          walker.beginField("k");
          walker.handleSimpleType(key);
          walker.endField();
          auto hfn = hash_function(context, key);
          int index = tableHash.reserve(table, key, hfn);
          G_ASSERT(index >= 0);
          char * pValue = table.data + index * valueTypeSize;
          walker.beginField("v");
          walker.walk(pValue, ti.secondType);
          walker.endField();

          walker.pop();
        }
      }
    };


    template <typename K>
    struct TableLoaderImpl<K, true>
    {
      static void extractKeyString(Context & context, char * & key, const char * keyStr)
      {
        key = context.intern(keyStr);
      }

      typedef K KeyType;
      void loadTable(TThisType & walker, Table & table, TypeInfo & ti)
      {
        Context & context = walker.context;
        G_ASSERT(ti.type == tTable);
        G_ASSERT(ti.secondType);
        int valueTypeSize = getTypeSize(ti.secondType);
        TableHash<KeyType> tableHash(&context, valueTypeSize);
        int arraySize = (int)walker.handler().getArraySize();
        // PVS studio went crazy about that simple cycle and claimed that the condition is always false.
        // This is the 8th try to make it happy about that
        //-V:suspiciousCounter:621,654
        for (int suspiciousCounter = 0; suspiciousCounter < arraySize; ++suspiciousCounter)
        {
          KeyType key = KeyType();
          const char * keyStr = walker.handler().readTableKey();
          extractKeyString(context, key, keyStr);
          walker.push(State::TableKey);
          walker.beginField(keyStr);
          auto hfn = hash_function(context, key);
          int index = tableHash.reserve(table, key, hfn);
          G_ASSERT(index >= 0);
          char * pValue = table.data + index * valueTypeSize;
          walker.walk(pValue, ti.secondType);
          walker.endField();
          walker.pop();
        }
      }
    };


    template <typename K>
    struct TableLoader : public TableLoaderImpl<K, AllowDirectTableSerialization<K>::value>
    {
    };


    void afterTable ( Table * pa, TypeInfo * ti ) override
    {
      if (reading)
      {
        G_ASSERT(ti->firstType);
        typeTemplateSelect<TableLoader>(
            ti->firstType->type,
            [&](auto && loader)
            {
              loader.loadTable(*this, *pa, *ti);
            });
      }
      if (!allowDirectTableSerialization(ti))
        handler().handleArrayEnd();
      else
        handler().handleStructureEnd();
      pop();
    }

    void beforeRef ( char * pa, TypeInfo * ti ) override {}
    void afterRef ( char * pa, TypeInfo * ti ) override {}

    void walk( char * pf, TypeInfo * ti ) override
    {
      if (!skipNextObj)
      {
        DataWalker::walk(pf, ti);
      }

      skipNextObj = false;
    }

    void walk(vec4f f, TypeInfo * ti) override
    {
      if (!skipNextObj)
      {
        DataWalker::walk(f, ti);
      }

      skipNextObj = false;
    }

    void writeSimplePtr(intptr_t ptr)
    {
      switch (state)
      {
        case State::Field:
          G_ASSERT(fieldName);
          handler().writePtr(fieldName, ptr);
          break;
        case State::Array:
          handler().appendPtr(ptr);
          break;
        default: G_ASSERT(false);
      }
    }

    void beforePtr ( char * pa, TypeInfo * ti ) override
    {
      if (reading)
      {
        char ** ptrPtr = (char **) pa;
        das::pair<intptr_t, bool> readed = state == State::Array ?
            handler().readElementPtr() :
            handler().readPtr(fieldName);

        if (readed.first)
        {
          auto & newPtr = ptrMap[readed.first];
          if (newPtr.first)
          {
            G_ASSERT(ti->firstType == newPtr.second->firstType);
          }
          else
          {
            newPtr.first = *ptrPtr ? intptr_t(*ptrPtr) : createInstance(ti->firstType);
            newPtr.second = ti;
          }

          *ptrPtr = (char*)newPtr.first;

          skipNextObj = !readed.second;
        }
        else
        {
          *ptrPtr = nullptr;
        }
      }
      else
      {
        char * ptr = *(char **)pa;
        if (ptr)
        {
          auto & index = ptrMap[intptr_t(ptr)];
          if (index.first)
          {
            G_ASSERT(ti->firstType == index.second);
            skipNextObj = true;
            writeSimplePtr(index.first);
          }
          else
          {
            index = das::make_pair(indexCounter++, ti->firstType);
            push(State::Ptr);
            handler().writePtr(index.first);
          }
        }
        else
        {
          writeSimplePtr(0);
        }
      }
    }

    void afterPtr ( char * pa, TypeInfo * ti ) override
    {
      if (reading)
      {
      }
      else
      {
        if (state == State::Ptr)
        {
          pop();
        }
      }
    }

    void beforeHandle ( char * pa, TypeInfo * ti ) override { G_ASSERT(false); }
    void afterHandle ( char * pa, TypeInfo * ti ) override { G_ASSERT(false); }
  // types
    void Null ( TypeInfo * ti ) override {}
    void Bool ( bool & value) override { handleSimpleType(value); }
    void Int64 ( int64_t & value) override { handleSimpleType(value); }
    void UInt64 ( uint64_t & value) override { handleSimpleType(value); }
    void String ( char * & value) override { handleSimpleType(value); }
    void Double ( double & value) override { handleSimpleType(value); }
    void Float ( float & value) override { handleSimpleType(value); }
    void Int ( int32_t & value) override { handleSimpleType(value); }
    void UInt ( uint32_t & value) override { handleSimpleType(value); }

    template <typename V>
    void handleSimpleType(vec2<V> & value)
    {
      push(State::Structure);
      handler().handleField("x", value.x);
      handler().handleField("y", value.y);
      pop();
    }

    template <typename V>
    void handleSimpleType(vec3<V> & value)
    {
      push(State::Structure);
      handler().handleField("x", value.x);
      handler().handleField("y", value.y);
      handler().handleField("z", value.z);
      pop();
    }

    template <typename V>
    void handleSimpleType(vec4<V> & value)
    {
      push(State::Structure);
      handler().handleField("x", value.x);
      handler().handleField("y", value.y);
      handler().handleField("z", value.z);
      handler().handleField("w", value.w);
      pop();
    }

    void Int2(int2 & value) override { handleSimpleType(value); }
    void Int3 (int3 & value) override { handleSimpleType(value); }
    void Int4 (int4 & value) override { handleSimpleType(value); }
    void UInt2 (uint2 & value) override { handleSimpleType(value); }
    void UInt3 (uint3 & value) override { handleSimpleType(value); }
    void UInt4 (uint4 & value) override { handleSimpleType(value); }
    void Float2 (float2 & value) override { handleSimpleType(value); }
    void Float3 (float3 & value) override { handleSimpleType(value); }
    void Float4 (float4 & value) override { handleSimpleType(value); }

    template <typename V>
    void handleSimpleType(RangeType<V> & value)
    {
      push(State::Structure);
      handler().handleField("from", value.from);
      handler().handleField("to", value.to);
      pop();
    }

    void Range ( range & value ) override { handleSimpleType(value); }
    void URange ( urange & value) override { handleSimpleType(value); }

    void WalkBlock ( Block * ) override { G_ASSERT(false); }
    void WalkEnumeration(int32_t & value, EnumInfo * ei) override
    {
      if (reading)
      {
        value = 0;
        char * strValue = nullptr;
        handleSimpleType(strValue);
        if (!strValue)
        {
          return;
        }

        for (int i = 0; i < ei->count; ++i)
        {
          if (!strcmp(ei->fields[i]->name, strValue))
          {
            value = ei->fields[i]->value;
          }
        }
      }
      else
      {
        for (int i = 0; i < ei->count; ++i)
        {
          if (ei->fields[i]->value == value)
          {
            handleSimpleType(ei->fields[i]->name);
          }
        }
      }
    }
  };

} // namespace das


#endif // _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_DATAWALKERWRAPPER_H_
