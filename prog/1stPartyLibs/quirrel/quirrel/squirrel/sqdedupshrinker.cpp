#include "sqpcheader.h"
#include "sqstate.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqarray.h"

#define SHRINK_HASH_INIT 0x811c9dc5
#define SHRINK_HASH_MUL  0x01193193

static inline uint32_t calc_shrinker_hash(uint32_t * data, int bytes)
{
  uint32_t hash = SHRINK_HASH_INIT;
  for (int i = 0; i < bytes / 4; i++)
    hash = (hash ^ data[i]) * SHRINK_HASH_MUL;
  return (hash >> 19) ^ hash;
}

struct SQDeduplicateShrinker
{
private:
  static constexpr int CACHE_SIZE = 1 << 12; // must be power of 2
  static constexpr int CACHE_ITEMS_LIMIT = 20;
  SQObjectPtrVec arrayCache;
  SQObjectPtrVec tableCache;
  sqvector<void *> visitedTables;
  sqvector<void *> visitedArrays;

public:

  SQDeduplicateShrinker(HSQUIRRELVM vm) : arrayCache(vm->_sharedstate->_alloc_ctx), tableCache(vm->_sharedstate->_alloc_ctx),
    visitedTables(vm->_sharedstate->_alloc_ctx), visitedArrays(vm->_sharedstate->_alloc_ctx)
  {
    arrayCache.resize(CACHE_SIZE);
    tableCache.resize(CACHE_SIZE);
  }

  SQDeduplicateShrinker(const SQDeduplicateShrinker &) = delete;
  SQDeduplicateShrinker(SQDeduplicateShrinker &&) = delete;
  SQDeduplicateShrinker & operator=(const SQDeduplicateShrinker &) = delete;
  SQDeduplicateShrinker & operator=(SQDeduplicateShrinker &&) = delete;


  bool shrink(SQObjectPtr & obj)
  {
    switch (obj._type)
    {
      case OT_TABLE:
      {
        obj._flags |= SQOBJ_FLAG_IMMUTABLE;
        bool simpleTypes = true;
        SQTable *t = _table(obj);
        int itemsCount = t->CountUsed();

        SQTable::_HashNode *nodes = t->_nodes;
        int allocatedNodes = t->AllocatedNodes();
        int cacheIndex = 0;

        if (itemsCount <= CACHE_ITEMS_LIMIT)
        {
          uint32_t hash = calc_shrinker_hash((uint32_t *)nodes, allocatedNodes * sizeof(SQTable::_HashNode));
          cacheIndex = hash & (CACHE_SIZE - 1);
          if (sq_istable(tableCache[cacheIndex]) && t->IsBinaryEqual(_table(tableCache[cacheIndex])))
          {
            obj = tableCache[cacheIndex];
            return true;
          }
        }

        for (void *p : visitedTables)
          if (p == _table(obj))
            return false;

        visitedTables.push_back(_table(obj));

        for (int i = 0; i < allocatedNodes; i++)
        {
          if (sq_type(nodes[i].key) != OT_NULL)
          {
            if (!sq_isstring(nodes[i].key) || (!sq_isnull(nodes[i].val) && !sq_isstring(nodes[i].val) && !sq_isinteger(nodes[i].val) && !sq_isfloat(nodes[i].val) && !sq_isbool(nodes[i].val)))
              simpleTypes = false;

            if (sq_isarray(nodes[i].val) || sq_istable(nodes[i].val))
              shrink(nodes[i].val); // note: classType is not changed
          }
        }

        visitedTables.pop_back();

        if (simpleTypes && itemsCount <= CACHE_ITEMS_LIMIT)
          tableCache[cacheIndex] = obj;
      }
      break;

      case OT_ARRAY:
      {
        bool simpleTypes = true;
        SQArray *array = _array(obj);
        array->ShrinkIfNeeded();
        obj._flags |= SQOBJ_FLAG_IMMUTABLE;
        int cacheIndex = 0;

        if (array->Size() <= CACHE_ITEMS_LIMIT)
        {
          uint32_t hash = calc_shrinker_hash((uint32_t *)&array->_values[0], array->Size() * sizeof(SQObjectPtr));
          cacheIndex = hash & (CACHE_SIZE - 1);
          if (sq_isarray(arrayCache[cacheIndex]) && array->IsBinaryEqual(_array(arrayCache[cacheIndex])))
          {
            obj = arrayCache[cacheIndex];
            return true;
          }
        }

        for (void *p : visitedArrays)
          if (p == _array(obj))
            return false;

        visitedArrays.push_back(_array(obj));

        for (int i = 0; i < int(array->Size()); i++)
        {
          if (!sq_isnull(array->_values[i]) && !sq_isstring(array->_values[i]) && !sq_isinteger(array->_values[i]) && !sq_isfloat(array->_values[i]) && !sq_isbool(array->_values[i]))
            simpleTypes = false;

          if (sq_isarray(array->_values[i]) || sq_istable(array->_values[i]))
            shrink(array->_values[i]);
        }

        visitedArrays.pop_back();

        if (simpleTypes && array->Size() <= CACHE_ITEMS_LIMIT)
          arrayCache[cacheIndex] = obj;
      }

      default:
        break;
    }

    return false;
  }
};

SQRESULT sq_deduplicate_object(HSQUIRRELVM vm, int index)
{
  SQDeduplicateShrinker shrinker(vm);
  shrinker.shrink(stack_get(vm, index));
  return SQ_OK;
}
