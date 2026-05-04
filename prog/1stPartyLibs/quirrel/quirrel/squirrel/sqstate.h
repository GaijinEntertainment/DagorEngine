/*  see copyright notice in squirrel.h */
#ifndef _SQSTATE_H_
#define _SQSTATE_H_

#include "squtils.h"
#include "sqobject.h"
struct SQString;
struct SQTable;

struct SQStringTable
{
    SQStringTable(SQSharedState*ss);
    ~SQStringTable();
    SQString *Add(const char *,SQInteger len);
    void Remove(SQString *);
private:
    void Resize(SQInteger size);
    void AllocNodes(SQInteger size);
    SQString **_strings;
    SQUnsignedInteger _numofslots;
    SQUnsignedInteger _slotused;
    SQSharedState *_sharedstate;
};

struct RefTable {
    struct RefNode {
        SQObjectPtr obj;
        SQUnsignedInteger refs;
        struct RefNode *next;
    };
    RefTable(SQAllocContext ctx);
    ~RefTable();
    void AddRef(SQObject &obj);
    SQBool Release(SQObject &obj);
    SQUnsignedInteger GetRefCount(SQObject &obj);
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
#endif
    void Finalize();
private:
    RefNode *Get(SQObject &obj,SQHash &mainpos,RefNode **prev,bool add);
    RefNode *Add(SQHash mainpos,SQObject &obj);
    void Resize(SQUnsignedInteger size);
    void AllocNodes(SQUnsignedInteger size);
    SQUnsignedInteger _numofslots;
    SQUnsignedInteger _slotused;
    RefNode *_nodes;
    RefNode *_freelist;
    RefNode **_buckets;
    SQAllocContext _alloc_ctx;
};

#define ADD_STRING(ss,str,len) ss->_stringtable->Add(str,len)
#define REMOVE_STRING(ss,bstr) ss->_stringtable->Remove(bstr)

struct SQObjectPtr;

struct SQSharedState
{
    SQSharedState(SQAllocContext allocctx);
    ~SQSharedState();
    void Init();
public:
    bool checkCompilationOption(SQUnsignedInteger co) const {
      return (compilationOptions & co) != 0;
    }

    void enableCompilationOption(SQUnsignedInteger co) {
      compilationOptions |= co;
    }

    void disableCompilationOption(SQUnsignedInteger co) {
      compilationOptions &= ~co;
    }
    char* GetScratchPad(SQInteger size);
    SQInteger GetMetaMethodIdxByName(const SQObjectPtr &name);
#ifndef NO_GARBAGE_COLLECTOR
    SQInteger CollectGarbage(SQVM *vm);
    void RunMark(SQVM *vm,SQCollectable **tchain);
    SQInteger ResurrectUnreachable(SQVM *vm);
    static void MarkObject(SQObjectPtr &o,SQCollectable **chain);
#endif
    SQAllocContext _alloc_ctx;
    SQObjectPtrVec *_metamethodnames;
    SQObjectPtr _metamethodsmap;
    SQObjectPtrVec *_systemstrings;
    SQObjectPtrVec *_types;
    SQStringTable *_stringtable;
    RefTable _refs_table;
    SQObjectPtr _registry;
    SQObjectPtr _consts;
    SQObjectPtr _constructorstr;
#ifndef NO_GARBAGE_COLLECTOR
    SQCollectable *_gc_chain;
#endif
    SQObjectPtr _root_vm;

    // Built-in type classes
    SQObjectPtr _null_class;
    SQObjectPtr _integer_class;
    SQObjectPtr _float_class;
    SQObjectPtr _bool_class;
    SQObjectPtr _string_class;
    SQObjectPtr _array_class;
    SQObjectPtr _table_class;
    SQObjectPtr _function_class;
    SQObjectPtr _generator_class;
    SQObjectPtr _thread_class;
    SQObjectPtr _class_class;
    SQObjectPtr _instance_class;
    SQObjectPtr _weakref_class;
    SQObjectPtr _userdata_class;

    static const SQRegFunction _table_default_type_methods_funcz[];
    static const SQRegFunction _array_default_type_methods_funcz[];
    static const SQRegFunction _string_default_type_methods_funcz[];
    static const SQRegFunction _integer_default_type_methods_funcz[];
    static const SQRegFunction _float_default_type_methods_funcz[];
    static const SQRegFunction _bool_default_type_methods_funcz[];
    static const SQRegFunction _null_default_type_methods_funcz[];
    static const SQRegFunction _generator_default_type_methods_funcz[];
    static const SQRegFunction _closure_default_type_methods_funcz[];
    static const SQRegFunction _thread_default_type_methods_funcz[];
    static const SQRegFunction _class_default_type_methods_funcz[];
    static const SQRegFunction _instance_default_type_methods_funcz[];
    static const SQRegFunction _weakref_default_type_methods_funcz[];
    static const SQRegFunction _userdata_default_type_methods_funcz[];

    SQCOMPILERERROR _compilererrorhandler;
    SQPRINTFUNCTION _printfunc;
    SQPRINTFUNCTION _errorfunc;
    SQ_COMPILER_DIAG_CB _compilerdiaghandler;

    bool _notifyallexceptions;
    bool _lineInfoInExpressions;
    bool _varTraceEnabled;
    SQUnsignedInteger compilationOptions;
    SQUnsignedInteger defaultLangFeatures;
    SQUserPointer _foreignptr;
    SQRELEASEHOOK _releasehook;

    SQObjectPtr doc_objects;
    int doc_object_index;
    SQUnsignedInteger32 rand_seed;
    SQUnsignedInteger32 watchdog_last_alive_time_msec;
    SQUnsignedInteger32 watchdog_threshold_msec;
private:
    char *_scratchpad;
    SQInteger _scratchpadsize;
};

#define _sp(s) (_sharedstate->GetScratchPad(s))
#define _spval (_sharedstate->GetScratchPad(-1))

bool CompileTypemask(SQIntVec &res,const char *typemask);


#endif //_SQSTATE_H_
