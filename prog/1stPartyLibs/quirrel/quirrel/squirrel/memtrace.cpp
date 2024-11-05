#include "sqpcheader.h"
#include "memtrace.h"

#include "sqvm.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "squserdata.h"
#include "sqcompiler.h"
#include "sqfuncstate.h"
#include "sqclass.h"
#include <squirrel.h>
#include <sqstdaux.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/sort.h>


namespace sqmemtrace
{
  enum AllocatedByType
  {
    ALLOC_NULL = 1 << 0,
    ALLOC_INTEGER = 1 << 1,
    ALLOC_FLOAT = 1 << 2,
    ALLOC_BOOL = 1 << 3,
    ALLOC_STRING = 1 << 4,
    ALLOC_TABLE = 1 << 5,
    ALLOC_ARRAY = 1 << 6,
    ALLOC_USERDATA = 1 << 7,
    ALLOC_CLOSURE = 1 << 8,
    ALLOC_FUNCPROTO = 1 << 9,
    ALLOC_OUTER = 1 << 10,
    ALLOC_NATIVECLOSURE = 1 << 11,
    ALLOC_GENERATOR = 1 << 12,
    ALLOC_USERPOINTER = 1 << 13,
    ALLOC_THREAD = 1 << 14,
    ALLOC_CLASS = 1 << 15,
    ALLOC_INSTANCE = 1 << 16,
    ALLOC_WEAKREF = 1 << 17,
    ALLOC_UNKNOWN = 1 << 18,

    ALLOC__LAST = 1 << 19,
  };

  struct ObjectSizeHistogram
  {
    eastl::unordered_map<size_t, size_t> sizeHistogram;
    eastl::unordered_map<size_t, size_t> allocTypeHistogram;
    eastl::unordered_set<void *> processedObjects;
    eastl::unordered_map<const char *, size_t> typeCountHistogram;
    eastl::unordered_map<const char *, size_t> typeSizeHistogram;
    eastl::unordered_map<size_t, size_t> tableNodesHistogram;
    eastl::unordered_map<size_t, size_t> tableUsedNodesHistogram;
    eastl::unordered_map<size_t, size_t> stringLengthsHistogram;
    eastl::unordered_map<size_t, size_t> arrayLengthsHistogram;
    eastl::vector<void*> stack;
    size_t totalSize = 0;
    size_t uniqueStrings = 0;
    size_t totalStrings = 0;
    bool recursion = false;
  };

  static bool checkRecursion(const HSQOBJECT& obj, ObjectSizeHistogram& hist)
  {
    void * ptr = (void *)_rawval(obj);
    if (eastl::find(hist.stack.begin(), hist.stack.end(), ptr) != hist.stack.end())
    {
      hist.recursion = true;
      return true;
    }
    return false;
  }

  static void get_obj_size_recursively(HSQUIRRELVM vm, HSQOBJECT &obj, ObjectSizeHistogram &hist, bool in_container, size_t size_limit)
  {
    if (size_limit != 0 && hist.totalSize >= size_limit)
      return;

    SQInteger prevTop = sq_gettop(vm);

    switch(sq_type(obj))
    {
      case OT_NULL:
      {
        hist.typeCountHistogram["null"]++;
        if (in_container)
          break;
        size_t size = sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_NULL;
        hist.typeSizeHistogram["null"] += size;
      }
      break;

      case OT_INTEGER:
      {
        hist.typeCountHistogram["integer"]++;
        if (in_container)
          break;
        size_t size = sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_INTEGER;
        hist.typeSizeHistogram["integer"] += size;
      }
      break;

      case OT_FLOAT:
      {
        hist.typeCountHistogram["float"]++;
        if (in_container)
          break;
        size_t size = sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_FLOAT;
        hist.typeSizeHistogram["float"] += size;
      }
      break;

      case OT_BOOL:
      {
        hist.typeCountHistogram["bool"]++;
        if (in_container)
          break;
        size_t size = sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_BOOL;
        hist.typeSizeHistogram["bool"] += size;
      }
      break;

      case OT_STRING:
      {
        SQString *str = _string(obj);
        size_t size = in_container ? 0 : sizeof(SQObjectPtr);

        if (hist.processedObjects.find((void *)str->_val) == hist.processedObjects.end())
        {
          hist.processedObjects.insert((void *)str->_val);
          size += sizeof(SQString) + str->_len + 1;
          hist.uniqueStrings++;
        }

        if (size)
        {
          hist.totalSize += size;
          hist.sizeHistogram[size]++;
          hist.allocTypeHistogram[size] |= ALLOC_STRING;
          hist.typeSizeHistogram["string"] += size;
        }

        hist.totalStrings++;
        hist.typeCountHistogram["string"]++;
        hist.stringLengthsHistogram[str->_len]++;
      }
      break;

      case OT_TABLE:
      {
        if (checkRecursion(obj, hist))
          break;
        hist.typeCountHistogram["table"]++;
        SQTable *table = _table(obj);
        size_t size = sizeof(SQTable) + (table->AllocatedNodes()) * (sizeof(SQObjectPtr) * 2 + sizeof(void *));
        if (!in_container)
          size += sizeof(SQObjectPtr);

        if (hist.processedObjects.find((void *)table) == hist.processedObjects.end())
          hist.processedObjects.insert((void *)table);
        else
          break;

        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_TABLE;
        hist.typeSizeHistogram["table"] += size;
        hist.tableNodesHistogram[table->AllocatedNodes()]++;
        hist.tableUsedNodesHistogram[table->CountUsed()]++;

        sq_pushobject(vm, obj);
        sq_pushnull(vm);

        hist.stack.push_back((void *)_rawval(obj));
        while (SQ_SUCCEEDED(sq_next(vm, -2))) {
          get_obj_size_recursively(vm, stack_get(vm, -2), hist, true, size_limit);
          get_obj_size_recursively(vm, stack_get(vm, -1), hist, true, size_limit);
          sq_pop(vm, 2);
        }
        hist.stack.pop_back();
        sq_pop(vm, 2);
      }
      break;

      case OT_CLASS:
      {
        if (checkRecursion(obj, hist))
          break;
        SQClass *cls = _class(obj);
        size_t size = sizeof(SQClass) + cls->_members->AllocatedNodes() * (sizeof(SQObjectPtr) * 2 + sizeof(void *));
        if (!in_container)
          size += sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_CLASS;
        hist.typeCountHistogram["class"]++;
        hist.typeSizeHistogram["class"] += size;
        hist.tableNodesHistogram[cls->_members->AllocatedNodes()]++;
        hist.tableUsedNodesHistogram[cls->_members->CountUsed()]++;

        sq_pushobject(vm, obj);
        sq_pushnull(vm);

        hist.stack.push_back((void *)_rawval(obj));
        while (SQ_SUCCEEDED(sq_next(vm, -2))) {
          get_obj_size_recursively(vm, stack_get(vm, -2), hist, true, size_limit);
          get_obj_size_recursively(vm, stack_get(vm, -1), hist, true, size_limit);
          sq_pop(vm, 2);
        }
        hist.stack.pop_back();
        sq_pop(vm, 2);
      }
      break;

      case OT_INSTANCE:
      {
        if (checkRecursion(obj, hist))
          break;
        SQInstance *inst = _instance(obj);
        size_t size = calcinstancesize(inst->_class);
        if (!in_container)
          size += sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_INSTANCE;
        hist.typeCountHistogram["instance"]++;
        hist.typeSizeHistogram["instance"] += size;

        sq_pushobject(vm, obj);
        sq_pushnull(vm);

        hist.stack.push_back((void *)_rawval(obj));
        while (SQ_SUCCEEDED(sq_next(vm, -2))) {
            get_obj_size_recursively(vm, stack_get(vm, -2), hist, true, size_limit);
            get_obj_size_recursively(vm, stack_get(vm, -1), hist, true, size_limit);
            sq_pop(vm, 2);
        }
        hist.stack.pop_back();
        sq_pop(vm, 2);
      }
      break;

      case OT_ARRAY:
      {
        if (checkRecursion(obj, hist))
          break;
        hist.typeCountHistogram["array"]++;
        SQArray *arr = _array(obj);
        size_t size = sizeof(SQArray) + arr->_values.capacity() * sizeof(SQObjectPtr);
        if (!in_container)
          size += sizeof(SQObjectPtr);

        if (hist.processedObjects.find((void *)arr) == hist.processedObjects.end())
          hist.processedObjects.insert((void *)arr);
        else
          break;

        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_ARRAY;
        hist.typeSizeHistogram["array"] += size;
        hist.arrayLengthsHistogram[arr->_values.size()]++;

        hist.stack.push_back((void *)_rawval(obj));
        for (SQInteger i = 0; i < arr->_values.size(); i++)
          get_obj_size_recursively(vm, arr->_values._vals[i], hist, true, size_limit);
        hist.stack.pop_back();
      }
      break;

      case OT_CLOSURE:
      {
        hist.typeCountHistogram["closure"]++;
        SQClosure *clo = _closure(obj);
        SQFunctionProto *func = clo->_function;
        size_t size = sizeof(SQClosure) + _FUNC_SIZE(func->_ninstructions, func->_nliterals, func->_nparameters, func->_nfunctions,
          func->_noutervalues, func->_nlineinfos, func->_nlocalvarinfos, func->_ndefaultparams);

        if (hist.processedObjects.find((void *)func) == hist.processedObjects.end())
        {
          hist.processedObjects.insert((void *)func);
          if (!in_container)
            size += sizeof(SQObjectPtr);
          hist.totalSize += size;
          hist.sizeHistogram[size]++;
          hist.allocTypeHistogram[size] |= ALLOC_CLOSURE;
          hist.typeSizeHistogram["closure"] += size;
        }
      }
      break;

      case OT_FUNCPROTO:
      {
        if (checkRecursion(obj, hist))
          break;
        hist.typeCountHistogram["funcproto"]++;
        SQFunctionProto *func = _funcproto(obj);
        size_t size = _FUNC_SIZE(func->_ninstructions, func->_nliterals, func->_nparameters, func->_nfunctions,
          func->_noutervalues, func->_nlineinfos, func->_nlocalvarinfos, func->_ndefaultparams);

        if (hist.processedObjects.find((void *)func) == hist.processedObjects.end())
        {
          hist.processedObjects.insert((void *)func);
          hist.stack.push_back((void *)_rawval(obj));

          for (SQInteger i = 0; i < func->_nliterals; i++)
            get_obj_size_recursively(vm, func->_literals[i], hist, true, size_limit);

          for (SQInteger i = 0; i < func->_nfunctions; i++)
            get_obj_size_recursively(vm, func->_functions[i], hist, true, size_limit);

          hist.stack.pop_back();
        }
        else
          size = 0;

        if (!in_container)
          size += sizeof(SQObjectPtr);

        if (size)
        {
          hist.totalSize += size;
          hist.sizeHistogram[size]++;
          hist.allocTypeHistogram[size] |= ALLOC_FUNCPROTO;
          hist.typeSizeHistogram["funcproto"] += size;
        }
      }
      break;

      case OT_OUTER:
      {
        SQOuter *outer = _outer(obj);
        size_t size = sizeof(SQOuter);
        if (!in_container)
          size += sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_OUTER;
        hist.typeCountHistogram["outer"]++;
        hist.typeSizeHistogram["outer"] += size;
      }
      break;

      case OT_NATIVECLOSURE:
      {
        hist.typeCountHistogram["nativeclosure"]++;
        SQNativeClosure *clo = _nativeclosure(obj);
        size_t size = sizeof(SQNativeClosure);
        if (!in_container)
          size += sizeof(SQObjectPtr);
        SQFUNCTION func = clo->_function;
        if (hist.processedObjects.find((void *)func) == hist.processedObjects.end())
        {
          hist.processedObjects.insert((void *)func);
          hist.totalSize += size;
          hist.sizeHistogram[size]++;
          hist.allocTypeHistogram[size] |= ALLOC_NATIVECLOSURE;
          hist.typeSizeHistogram["nativeclosure"] += size;
        }
      }
      break;

      case OT_GENERATOR:
      {
        SQGenerator *gen = _generator(obj);
        size_t size = sizeof(SQGenerator);
        if (!in_container)
          size += sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_GENERATOR;
        hist.typeCountHistogram["generator"]++;
        hist.typeSizeHistogram["generator"] += size;
      }
      break;

      case OT_USERDATA:
      {
        SQUserData *ud = _userdata(obj);
        size_t size = sizeof(SQUserData) + ud->_size;
        if (!in_container)
          size += sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_USERDATA;
        hist.typeCountHistogram["userdata"]++;
        hist.typeSizeHistogram["userdata"] += size;
      }
      break;

      case OT_THREAD:
      {
        SQVM *v = _thread(obj);
        size_t size = sizeof(SQVM);
        if (!in_container)
          size += sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_THREAD;
        hist.typeCountHistogram["thread"]++;
        hist.typeSizeHistogram["thread"] += size;
      }
      break;

      default:
      {
        size_t size = sizeof(SQObjectPtr);
        hist.totalSize += size;
        hist.sizeHistogram[size]++;
        hist.allocTypeHistogram[size] |= ALLOC_UNKNOWN;
        hist.typeCountHistogram["unknown"]++;
        hist.typeSizeHistogram["unknown"] += size;
      }
    }

    G_ASSERTF(sq_gettop(vm) == prevTop, "Top = %d -> %d", prevTop, sq_gettop(vm));
  }


  SQInteger get_quirrel_object_size(HSQUIRRELVM vm)
  {
    ObjectSizeHistogram hist;

    HSQOBJECT obj;
    sq_getstackobj(vm, 2, &obj);

    get_obj_size_recursively(vm, obj, hist, false, 0);

    sq_newtable(vm);

    sq_pushstring(vm, "size", -1);
    sq_pushinteger(vm, hist.totalSize);
    sq_rawset(vm, -3);

    sq_pushstring(vm, "size_histogram", -1);
    sq_newtable(vm);
    for (auto && it : hist.sizeHistogram)
    {
      sq_pushinteger(vm, it.first);
      sq_pushinteger(vm, it.second);
      sq_rawset(vm, -3);
    }
    sq_rawset(vm, -3);

    sq_pushstring(vm, "table_nodes_histogram", -1);
    sq_newtable(vm);
    for (auto && it : hist.tableNodesHistogram)
    {
      sq_pushinteger(vm, it.first);
      sq_pushinteger(vm, it.second);
      sq_rawset(vm, -3);
    }
    sq_rawset(vm, -3);

    sq_pushstring(vm, "string_lengths_histogram", -1);
    sq_newtable(vm);
    for (auto && it : hist.stringLengthsHistogram)
    {
      sq_pushinteger(vm, it.first);
      sq_pushinteger(vm, it.second);
      sq_rawset(vm, -3);
    }
    sq_rawset(vm, -3);

    sq_pushstring(vm, "array_lengths_histogram", -1);
    sq_newtable(vm);
    for (auto && it : hist.arrayLengthsHistogram)
    {
      sq_pushinteger(vm, it.first);
      sq_pushinteger(vm, it.second);
      sq_rawset(vm, -3);
    }
    sq_rawset(vm, -3);

    sq_pushstring(vm, "type_count_histogram", -1);
    sq_newtable(vm);
    for (auto && it : hist.typeCountHistogram)
    {
      sq_pushstring(vm, it.first, -1);
      sq_pushinteger(vm, it.second);
      sq_rawset(vm, -3);
    }
    sq_rawset(vm, -3);

    sq_pushstring(vm, "type_size_histogram", -1);
    sq_newtable(vm);
    for (auto && it : hist.typeSizeHistogram)
    {
      sq_pushstring(vm, it.first, -1);
      sq_pushinteger(vm, it.second);
      sq_rawset(vm, -3);
    }
    sq_rawset(vm, -3);

    return 1;
  }


  SQInteger get_quirrel_object_size_as_string(HSQUIRRELVM vm)
  {
    ObjectSizeHistogram hist;

    HSQOBJECT obj;
    sq_getstackobj(vm, 2, &obj);

    get_obj_size_recursively(vm, obj, hist, false, 0);

    eastl::string str;
    if (hist.recursion)
      str.append("WARNING: Recursion detected\n");

    str.append_sprintf("Size = %zu\n", hist.totalSize);
    str.append("Size histogram:\n");

    typedef eastl::pair<size_t, size_t> AllocInfo;
    eastl::vector<AllocInfo> sorted(hist.sizeHistogram.begin(), hist.sizeHistogram.end());
    eastl::vector<AllocInfo> sortedTypes(hist.allocTypeHistogram.begin(), hist.allocTypeHistogram.end());
    eastl::sort(sorted.begin(), sorted.end(), [](const AllocInfo & a, const AllocInfo & b) { return a.first > b.first; });
    eastl::sort(sortedTypes.begin(), sortedTypes.end(), [](const AllocInfo & a, const AllocInfo & b) { return a.first > b.first; });
    for (auto && it : sorted)
    {
      str.append_sprintf("  %zu bytes = %zu items", it.first, it.second);
      if (hist.allocTypeHistogram.find(it.first) != hist.allocTypeHistogram.end())
      {
        size_t mask = hist.allocTypeHistogram[it.first];
        if (mask)
        {
          str.append(" (");
          if (mask & ALLOC_NULL)
            str.append("null, ");
          if (mask & ALLOC_INTEGER)
            str.append("integer, ");
          if (mask & ALLOC_FLOAT)
            str.append("float, ");
          if (mask & ALLOC_BOOL)
            str.append("bool, ");
          if (mask & ALLOC_STRING)
            str.append("string, ");
          if (mask & ALLOC_TABLE)
            str.append("table, ");
          if (mask & ALLOC_ARRAY)
            str.append("array, ");
          if (mask & ALLOC_USERDATA)
            str.append("userdata, ");
          if (mask & ALLOC_CLOSURE)
            str.append("closure, ");
          if (mask & ALLOC_FUNCPROTO)
            str.append("funcproto, ");
          if (mask & ALLOC_OUTER)
            str.append("outer, ");
          if (mask & ALLOC_NATIVECLOSURE)
            str.append("nativeclosure, ");
          if (mask & ALLOC_GENERATOR)
            str.append("generator, ");
          if (mask & ALLOC_USERPOINTER)
            str.append("userpointer, ");
          if (mask & ALLOC_THREAD)
            str.append("thread, ");
          if (mask & ALLOC_CLASS)
            str.append("class, ");
          if (mask & ALLOC_INSTANCE)
            str.append("instance, ");
          if (mask & ALLOC_WEAKREF)
            str.append("weakref, ");
          if (mask & ALLOC_UNKNOWN)
            str.append("unknown, ");
          str.pop_back();
          str.back() = ')';
        }
      }
      str.append("\n");
    }
    str.append("\n");

    str.append("Type count histogram:\n");
    for (auto && it : hist.typeCountHistogram)
      str.append_sprintf("  %s = %zu items\n", it.first, it.second);
    str.append("\n");

    str.append("Type size histogram:\n");
    for (auto && it : hist.typeSizeHistogram)
      str.append_sprintf("  %s = %zu bytes\n", it.first, it.second);
    str.append("\n");

    str.append("Table allocated nodes histogram:\n");
    sorted.assign(hist.tableNodesHistogram.begin(), hist.tableNodesHistogram.end());
    eastl::sort(sorted.begin(), sorted.end(), [](const AllocInfo & a, const AllocInfo & b) { return a.first > b.first; });
    for (auto && it : sorted)
      str.append_sprintf("  %zu nodes = %zu items\n", it.first, it.second);
    str.append("\n");

    str.append("Table used nodes histogram:\n");
    sorted.assign(hist.tableUsedNodesHistogram.begin(), hist.tableUsedNodesHistogram.end());
    eastl::sort(sorted.begin(), sorted.end(), [](const AllocInfo & a, const AllocInfo & b) { return a.first > b.first; });
    for (auto && it : sorted)
      str.append_sprintf("  %zu nodes = %zu items\n", it.first, it.second);
    str.append("\n");

    str.append("String lengths histogram:\n");
    sorted.assign(hist.stringLengthsHistogram.begin(), hist.stringLengthsHistogram.end());
    eastl::sort(sorted.begin(), sorted.end(), [](const AllocInfo & a, const AllocInfo & b) { return a.first > b.first; });
    for (auto && it : sorted)
      str.append_sprintf("  %zu chars = %zu items\n", it.first, it.second);
    str.append("\n");

    str.append("Array lengths histogram:\n");
    sorted.assign(hist.arrayLengthsHistogram.begin(), hist.arrayLengthsHistogram.end());
    eastl::sort(sorted.begin(), sorted.end(), [](const AllocInfo & a, const AllocInfo & b) { return a.first > b.first; });
    for (auto && it : sorted)
      str.append_sprintf("  %zu len = %zu items\n", it.first, it.second);
    str.append("\n");

    str.append_sprintf("Total strings = %zu\n", hist.totalStrings);
    str.append_sprintf("Unique strings = %zu\n", hist.uniqueStrings);

    sq_pushstring(vm, str.c_str(), -1);
    return 1;
  }

  SQInteger is_quirrel_object_larger_than(HSQUIRRELVM vm)
  {
    ObjectSizeHistogram hist;

    HSQOBJECT obj;
    sq_getstackobj(vm, 2, &obj);

    SQInteger sizeLimit;
    sq_getinteger(vm, 3, &sizeLimit);
    if (sizeLimit <= 0)
      return sqstd_throwerrorf(vm, "size limit must be positive, got %d", int(sizeLimit));

    get_obj_size_recursively(vm, obj, hist, false, sizeLimit);

    sq_pushbool(vm, hist.totalSize > sizeLimit);

    return 1;
  }

}


#if MEM_TRACE_ENABLED == 1

#include "vartrace.h"
#include "squtils.h"

#define MEM_TRACE_MAX_VM 8
#define MEM_TRACE_STACK_SIZE 4

namespace sqmemtrace
{

struct ScriptAllocRecord
{
  const char * fileNames[MEM_TRACE_STACK_SIZE] = { 0 };
  int lines[MEM_TRACE_STACK_SIZE] = { 0 };
  int size = 0;
};

typedef struct SQAllocContextT * SQAllocContext;
typedef eastl::unordered_map<const void *, ScriptAllocRecord> ScriptAllocRecordsMap;


static SQAllocContext trace_ctx_to_index[MEM_TRACE_MAX_VM] = { 0 };
static ScriptAllocRecordsMap mem_trace[MEM_TRACE_MAX_VM];



inline ScriptAllocRecordsMap & ctx_to_alloc_records_map(SQAllocContext ctx)
{
  if (!ctx)
    return mem_trace[MEM_TRACE_MAX_VM - 1];

  for (int i = 0; i < MEM_TRACE_MAX_VM - 1; i++)
    if (ctx == trace_ctx_to_index[i])
      return mem_trace[i];

  G_ASSERTF(0, "sq mem trace map not found");
  return mem_trace[MEM_TRACE_MAX_VM - 1];
}

inline int get_memtrace_index(SQAllocContext ctx)
{
  int idx = MEM_TRACE_MAX_VM - 1;
  if (ctx)
    for (int i = 0; i < MEM_TRACE_MAX_VM - 1; i++)
      if (ctx == trace_ctx_to_index[i])
      {
        idx = i;
        break;
      }

  return idx;
}


void add_ctx(SQAllocContext ctx)
{
  debug("sqmemtrace::add_ctx %p", ctx);
  for (int i = 0; i < MEM_TRACE_MAX_VM - 1; i++)
    if (!trace_ctx_to_index[i])
    {
      trace_ctx_to_index[i] = ctx;
      return;
    }

  G_ASSERTF(0, "memtrace: too many VMs, increase MEM_TRACE_MAX_VM");
}


static void dump_sq_allocations_internal(ScriptAllocRecordsMap & alloc_map, int n_top_records)
{
  struct TraceLocation
  {
    const char * fileNames[MEM_TRACE_STACK_SIZE] = { 0 };
    int lines[MEM_TRACE_STACK_SIZE] = { 0 };
  };

  auto hash = [](const TraceLocation& n)
  {
    size_t res = 0;
    for (int i = 0; i < MEM_TRACE_STACK_SIZE; i++)
      res += size_t(n.fileNames[i]) + size_t(n.lines[i]);
    return res;
  };

  auto equal = [](const TraceLocation& a, const TraceLocation& b)
  {
    for (int i = 0; i < MEM_TRACE_STACK_SIZE; i++)
      if (a.fileNames[i] != b.fileNames[i] || a.lines[i] != b.lines[i])
        return false;
    return true;
  };

  struct ScriptAllocRecordWithCount
  {
    int size = 0;
    int count = 0;
  };

  eastl::unordered_map<TraceLocation, ScriptAllocRecordWithCount, decltype(hash), decltype(equal)> accum(1024, hash, equal);
  for (auto && rec : alloc_map)
  {
    TraceLocation loc;
    for (int i = 0; i < MEM_TRACE_STACK_SIZE; i++)
    {
      loc.fileNames[i] = rec.second.fileNames[i];
      loc.lines[i] = rec.second.lines[i];
    }
    auto it = accum.find(loc);
    if (it != accum.end())
    {
      it->second.size += rec.second.size;
      it->second.count++;
    }
    else
    {
      ScriptAllocRecordWithCount m;
      m.size = rec.second.size;
      m.count = 1;
      accum[loc] = m;
    }
  }

  typedef eastl::pair<TraceLocation, ScriptAllocRecordWithCount> AllocInfo;
  eastl::vector<AllocInfo> sorted(accum.begin(), accum.end());
  eastl::sort(sorted.begin(), sorted.end(), [](const AllocInfo & a, const AllocInfo & b) { return a.second.size > b.second.size; });

  debug_("\n==== Begin of quirrel memory allocations ====\n");
  int total = 0;
  unsigned totalBytes = 0;
  int records = 0;
  for (auto && rec : sorted)
  {
    total += rec.second.count;
    totalBytes += rec.second.size;
    debug_("\n%d bytes, %d allocations\n", rec.second.size, rec.second.count);
    for (int i = 0; i < MEM_TRACE_STACK_SIZE && rec.first.fileNames[i]; i++)
      debug_("%s:%d\n", rec.first.fileNames[i], rec.first.lines[i]);

    records++;
    if (n_top_records > 0 && records > n_top_records)
    {
      debug_("\n...");
      break;
    }
  }
  debug_("\ntotal = %d allocations, %u bytes", total, totalBytes);
  debug("\n==== End of quirrel memory allocations ====");
}


void remove_ctx(SQAllocContext ctx)
{
  debug("sqmemtrace::remove_ctx %p", ctx);
  int idx = get_memtrace_index(ctx);

  if (!mem_trace[idx].empty())
  {
    logerr("Detected memory leaks in quirrel VM, see debug for details");
    dump_sq_allocations_internal(mem_trace[idx], 128);
  }

  mem_trace[idx].clear();
  trace_ctx_to_index[idx] = nullptr;
}


void on_alloc(SQAllocContext ctx, HSQUIRRELVM vm, const void * p, size_t size)
{
  ScriptAllocRecordsMap & m = ctx_to_alloc_records_map(ctx);
  ScriptAllocRecord rec;

  rec.fileNames[0] = "unknown";
  rec.lines[0] = 0;
  if (vm && vm->ci)
  {
    SQStackInfos si;
    SQInteger level = 0;

    int count = 0;
    while (SQ_SUCCEEDED(sq_stackinfos(vm, level, &si)))
    {
      if (!si.source)
        break;

      rec.fileNames[count] = si.source;
      rec.lines[count] = int(si.line);
      if (int(si.line) != -1)
      {
        count++;
        if (count >= MEM_TRACE_STACK_SIZE)
          break;
      }
      level++;
    }
  }

  rec.size = int(size);
  m[p] = rec;
}


void on_free(SQAllocContext ctx, const void * p)
{
  ScriptAllocRecordsMap & m = ctx_to_alloc_records_map(ctx);
  auto it = m.find(p);
  if (it != m.end())
    m.erase(it);
}


void reset_statistics_for_current_vm(HSQUIRRELVM vm)
{
  SQAllocContext ctx = vm->_sharedstate->_alloc_ctx;
  int idx = get_memtrace_index(ctx);
  mem_trace[idx].clear();
}


void reset_all()
{
  for (int i = 0; i < MEM_TRACE_MAX_VM; i++)
    mem_trace[i].clear();
}


void dump_statistics_for_current_vm(HSQUIRRELVM vm, int n_top_records)
{
  SQAllocContext ctx = vm->_sharedstate->_alloc_ctx;
  int idx = get_memtrace_index(ctx);
  dump_sq_allocations_internal(mem_trace[idx], n_top_records);
}


void dump_all(int n_top_records)
{
  for (int i = 0; i < MEM_TRACE_MAX_VM; i++)
    if (!mem_trace[i].empty())
    {
      debug("\nMemTrace VM #%d:", i);
      dump_sq_allocations_internal(mem_trace[i], n_top_records);
    }
}

} // namespace sqmemtrace

#else // MEM_TRACE_ENABLED == 0

namespace sqmemtrace
{

void reset_statistics_for_current_vm(HSQUIRRELVM)
{
  logerr("compile exe with SqVarTrace=yes to call sqmemtrace::reset_statistics_for_current_vm");
}

void reset_all()
{
  logerr("compile exe with SqVarTrace=yes to call sqmemtrace::reset_all");
}

void dump_statistics_for_current_vm(HSQUIRRELVM, int)
{
  logerr("compile exe with SqVarTrace=yes to call sqmemtrace::dump_statistics_for_current_vm");
}

void dump_all(int)
{
  logerr("compile exe with SqVarTrace=yes to call sqmemtrace::dump_all");
}

} // namespace sqmemtrace

#endif // MEM_TRACE_ENABLED
