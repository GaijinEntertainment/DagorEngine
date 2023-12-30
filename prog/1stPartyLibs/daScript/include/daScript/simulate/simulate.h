#pragma once

#include "daScript/misc/vectypes.h"
#include "daScript/misc/type_name.h"
#include "daScript/misc/arraytype.h"
#include "daScript/simulate/cast.h"
#include "daScript/simulate/runtime_string.h"
#include "daScript/simulate/debug_info.h"
#include "daScript/simulate/heap.h"

#include "daScript/simulate/simulate_visit_op.h"

namespace das
{
    #define DAS_BIND_FUN(a)                     decltype(&a), a
    #define DAS_BIND_MEMBER_FUN(a)              decltype(&a), &a
    #define DAS_BIND_PROP(BIGTYPE,FIELDNAME)    decltype(&BIGTYPE::FIELDNAME), &BIGTYPE::FIELDNAME
    #define DAS_BIND_FIELD(BIGTYPE,FIELDNAME)   decltype(das::declval<BIGTYPE>().FIELDNAME), offsetof(BIGTYPE,FIELDNAME)

    #define DAS_CALL_METHOD(mname)              DAS_BIND_FUN(mname::invoke)

    #ifndef DAS_ENABLE_SMART_PTR_TRACKING
    #define DAS_ENABLE_SMART_PTR_TRACKING   0
    #endif

    #ifndef DAS_ENABLE_STACK_WALK
    #define DAS_ENABLE_STACK_WALK   1
    #endif

    #ifndef DAS_ENABLE_EXCEPTIONS
    #define DAS_ENABLE_EXCEPTIONS   0
    #endif

    #if DAS_ENABLE_PROFILER
        #define DAS_PROFILE_NODE    profileNode(this);
    #else
        #define DAS_PROFILE_NODE
    #endif

    class Context;
    struct SimNode;
    struct Block;
    struct SimVisitor;

    enum class ContextCategory : uint32_t {
        none =              0
    ,   dead =              (1<<0)
    ,   debug_context =     (1<<1)
    ,   thread_clone =      (1<<2)
    ,   job_clone =         (1<<3)
    ,   opengl =            (1<<4)
    ,   debugger_tick =     (1<<5)
    ,   debugger_attached = (1<<6)
    ,   macro_context     = (1<<7)
    ,   folding_context =   (1<<8)
    ,   audio_context =     (1<<9)
    };

    struct GlobalVariable {
        char *          name;
        VarInfo *       debugInfo;
        SimNode *       init;
        uint64_t        mangledNameHash;
        uint32_t        size;
        uint32_t        offset;
        union {
            struct {
                bool    shared : 1;
            };
            uint32_t    flags;
        };
    };

    struct SimFunction {
        char *      name;
        char *      mangledName;
        SimNode *   code;
        FuncInfo *  debugInfo;
        uint64_t    mangledNameHash;
        void *      aotFunction;
        uint32_t    stackSize;
        union {
            uint32_t    flags;
            struct {
                bool    aot : 1;
                bool    fastcall : 1;
                bool    builtin : 1;
                bool    jit : 1;
                bool    unsafe : 1;
                bool    cmres : 1;
                bool    pinvoke : 1;
            };
        };
        const LineInfo * getLineInfo() const;
    };

    struct SimNode {
        SimNode ( const LineInfo & at ) : debugInfo(at) {}
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code );
        DAS_EVAL_ABI virtual vec4f eval ( Context & ) = 0;
        virtual SimNode * visit ( SimVisitor & vis );
        virtual char *      evalPtr ( Context & context );
        virtual bool        evalBool ( Context & context );
        virtual float       evalFloat ( Context & context );
        virtual double      evalDouble ( Context & context );
        virtual int32_t     evalInt ( Context & context );
        virtual uint32_t    evalUInt ( Context & context );
        virtual int64_t     evalInt64 ( Context & context );
        virtual uint64_t    evalUInt64 ( Context & context );
        LineInfo debugInfo;
        virtual bool rtti_node_isSourceBase() const { return false;  }
        virtual bool rtti_node_isBlock() const { return false; }
        virtual bool rtti_node_isIf() const { return false; }
        virtual bool rtti_node_isInstrument() const { return false; }
        virtual bool rtti_node_isInstrumentFunction() const { return false; }
        virtual bool rtti_node_isJit() const { return false; }
    protected:
        virtual ~SimNode() {}
    };

    struct alignas(16) Prologue {
        union {
            FuncInfo *  info;
            Block *     block;
        };
        union {
            struct {
                const char * fileName;
                LineInfo *   functionLine;
                int32_t      stackSize;
            };
            struct {
                vec4f *     arguments;
                void *      cmres;
                LineInfo *  line;
            };
        };
    };

    struct BlockArguments {
        vec4f *     arguments;
        char *      copyOrMoveResult;
    };

    enum EvalFlags : uint32_t {
        stopForBreak        = 1 << 0
    ,   stopForReturn       = 1 << 1
    ,   stopForContinue     = 1 << 2
    ,   jumpToLabel         = 1 << 3
    ,   yield               = 1 << 4
    };

#define DAS_PROCESS_LOOP_FLAGS(howtocontinue) \
    { if (context.stopFlags) { \
        if (context.stopFlags & EvalFlags::stopForContinue) { \
            context.stopFlags &= ~EvalFlags::stopForContinue; \
            howtocontinue; \
        } else if (context.stopFlags&EvalFlags::jumpToLabel && context.gotoLabel<this->totalLabels) { \
            if ((body=this->list+this->labels[context.gotoLabel])>=this->list) { \
                context.stopFlags &= ~EvalFlags::jumpToLabel; \
                goto loopbegin; \
            } \
        } \
        goto loopend; \
    } }

#define DAS_PROCESS_LOOP1_FLAGS(howtocontinue) \
    { if (context.stopFlags) { \
        if (context.stopFlags & EvalFlags::stopForContinue) { \
            context.stopFlags &= ~EvalFlags::stopForContinue; \
            howtocontinue; \
        } \
        goto loopend; \
    } }

#if DAS_ENABLE_EXCEPTIONS
    class dasException : public std::runtime_error {
    public:
        dasException ( const char * why, const LineInfo & at ) : runtime_error(why), exceptionAt(at) {}
    public:
        LineInfo exceptionAt;
    };
#endif

    struct SimVisitor {
        virtual void preVisit ( SimNode * ) { }
        virtual void cr () {}
        virtual void op ( const char * /* name */, uint32_t /* sz */ = 0, const string & /* TT */ = string() ) {}
        virtual void sp ( uint32_t /* stackTop */,  const char * /* op */ = "#sp" ) { }
        virtual void arg ( int32_t /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( uint32_t /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( const char * /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( vec4f /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( int64_t /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( uint64_t /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( float /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( double /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( bool /* argV */,  const char * /* argN */  ) { }
        virtual void arg ( Func /* fun */,  const char * /* mangledName */, const char * /* argN */ ) { }
        virtual void arg ( Func /* fun */,  uint32_t /* mangledName */, const char * /* argN */ ) { }
        virtual void sub ( SimNode ** nodes, uint32_t count, const char * );
        virtual SimNode * sub ( SimNode * node, const char * /* opN */ = "subexpr" ) { return node->visit(*this); }
        virtual SimNode * visit ( SimNode * node ) { return node; }
    };

    void printSimNode ( TextWriter & ss, Context * context, SimNode * node, bool debugHash=false );
    class Function;
    void printSimFunction ( TextWriter & ss, Context * context, Function * fun, SimNode * node, bool debugHash=false );
    uint64_t getSemanticHash ( SimNode * node, Context * context );

    class DebugAgent : public ptr_ref_count {
    public:
        virtual void onInstall ( DebugAgent * ) {}
        virtual void onUninstall ( DebugAgent * ) {}
        virtual void onCreateContext ( Context * ) {}
        virtual void onDestroyContext ( Context * ) {}
        virtual void onSimulateContext ( Context * ) {}
        virtual void onSingleStep ( Context *, const LineInfo & ) {}
        virtual void onInstrument ( Context *, const LineInfo & ) {}
        virtual void onInstrumentFunction ( Context *, SimFunction *, bool, uint64_t ) {}
        virtual void onBreakpoint ( Context *, const LineInfo &, const char *, const char * ) {}
        virtual void onVariable ( Context *, const char *, const char *, TypeInfo *, void * ) {}
        virtual void onTick () {}
        virtual void onCollect ( Context *, const LineInfo & ) {}
        virtual bool onLog ( Context *, const LineInfo * /*at*/, int /*level*/, const char * /*text*/ ) { return false; }
        virtual void onBreakpointsReset ( const char * /*file*/, int /*breakpointsNum*/ ) {}
        virtual bool isCppOnlyAgent() const { return false; }
        bool isThreadLocal = false;
    };
    typedef smart_ptr<DebugAgent> DebugAgentPtr;

    class StackWalker : public ptr_ref_count {
    public:
        virtual bool canWalkArguments () { return true; }
        virtual bool canWalkVariables () { return true; }
        virtual bool canWalkOutOfScopeVariables() { return true; }
        virtual void onBeforeCall ( Prologue *, char * ) { }
        virtual void onCallAOT ( Prologue *, const char * ) { }
        virtual void onCallAt ( Prologue *, FuncInfo *, LineInfo * ) { }
        virtual void onCall ( Prologue *, FuncInfo * ) { }
        virtual void onAfterPrologue ( Prologue *, char * ) { }
        virtual void onArgument ( FuncInfo *, int, VarInfo *, vec4f ) { }
        virtual void onBeforeVariables ( ) { }
        virtual void onVariable ( FuncInfo *, LocalVariableInfo *, void *, bool ) { }
        virtual bool onAfterCall ( Prologue * ) { return true; }
    };
    typedef smart_ptr<StackWalker> StackWalkerPtr;

    void dapiStackWalk ( StackWalkerPtr walker, Context & context, const LineInfo & at );
    int32_t dapiStackDepth ( Context & context );
    void dumpTrackingLeaks();
    void dapiReportContextState ( Context & ctx, const char * category, const char * name, const TypeInfo * info, void * data );
    void dapiSimulateContext ( Context & ctx );

    typedef shared_ptr<Context> ContextPtr;

    class Context : public ptr_ref_count, public enable_shared_from_this<Context> {
        template <typename TT> friend struct SimNode_GetGlobalR2V;
        friend struct SimNode_GetGlobal;
        template <typename TT> friend struct SimNode_GetSharedR2V;
        friend struct SimNode_GetShared;
        friend struct SimNode_TryCatch;
        friend struct SimNode_FuncConstValue;
        friend class Program;
        friend class Module;
    public:
        Context(uint32_t stackSize = 16*1024, bool ph = false);
        Context(const Context &, uint32_t category_);
        Context(const Context &) = delete;
        Context & operator = (const Context &) = delete;
        virtual ~Context();
        void strip();
        void logMemInfo(TextWriter & tw);

        void makeWorkerFor(const Context & ctx);

        uint32_t getGlobalSize() const {
            return globalsSize;
        }

        uint32_t getSharedSize() const {
            return sharedSize;
        }

        uint64_t getInitSemanticHash();

        __forceinline void * getVariable ( int index ) const {
            if ( uint32_t(index)<uint32_t(totalVariables) ) {
                const auto & gvar = globalVariables[index];
                return (gvar.shared ? shared : globals) + gvar.offset;
            } else {
                return nullptr;
            }
        }

        __forceinline VarInfo * getVariableInfo( int index ) const {
            return (uint32_t(index)<uint32_t(totalVariables)) ? globalVariables[index].debugInfo  : nullptr;;
        }

        __forceinline void simEnd() {
            thisHelper = nullptr;
        }

        __forceinline void restart( ) {
            DAS_ASSERTF(insideContext==0,"can't reset locked context");
            stopFlags = 0;
            exception = nullptr;
            last_exception = nullptr;
        }

        __forceinline void restartHeaps() {
            DAS_ASSERTF(insideContext==0,"can't reset heaps in locked context");
            heap->reset();
            stringHeap->reset();
        }

        __forceinline uint32_t tryRestartAndLock() {
            if (insideContext == 0) {
                restart();
            }
            return lock();
        }

        __forceinline uint32_t lock() {
            return insideContext ++;
        }

        virtual uint32_t unlock() {
            return insideContext --;
        }

        DAS_EVAL_ABI __forceinline vec4f eval ( const SimFunction * fnPtr, vec4f * args = nullptr, void * res = nullptr ) {
            return callWithCopyOnReturn(fnPtr, args, res, 0);
        }

        template <typename TT>
        __forceinline void threadlock_context ( TT && subexpr ) {
            DAS_ASSERTF(contextMutex,"context mutex is not set");
            lock_guard<recursive_mutex> guard(*contextMutex);
            lock();
            subexpr();
            unlock();
        }

        DAS_EVAL_ABI vec4f ___noinline evalWithCatch ( SimFunction * fnPtr, vec4f * args = nullptr, void * res = nullptr );
        DAS_EVAL_ABI vec4f ___noinline evalWithCatch ( SimNode * node );
        bool ___noinline runWithCatch ( const callable<void()> & subexpr );
        DAS_NORETURN_PREFIX void throw_error ( const char * message ) DAS_NORETURN_SUFFIX;
        DAS_NORETURN_PREFIX void throw_error_ex ( DAS_FORMAT_STRING_PREFIX const char * message, ... ) DAS_NORETURN_SUFFIX DAS_FORMAT_PRINT_ATTRIBUTE(2,3);
        DAS_NORETURN_PREFIX void throw_error_at ( const LineInfo & at, DAS_FORMAT_STRING_PREFIX const char * message, ... ) DAS_NORETURN_SUFFIX DAS_FORMAT_PRINT_ATTRIBUTE(3,4);
        DAS_NORETURN_PREFIX void throw_error_at ( const LineInfo * at, DAS_FORMAT_STRING_PREFIX const char * message, ... ) DAS_NORETURN_SUFFIX DAS_FORMAT_PRINT_ATTRIBUTE(3,4);
        DAS_NORETURN_PREFIX void throw_fatal_error ( const char * message, const LineInfo & at ) DAS_NORETURN_SUFFIX;
        DAS_NORETURN_PREFIX void rethrow () DAS_NORETURN_SUFFIX;

        __forceinline SimFunction * getFunction ( int index ) const {
            return (index>=0 && index<totalFunctions) ? functions + index : nullptr;
        }
        __forceinline int32_t getTotalFunctions() const {
            return totalFunctions;
        }
        __forceinline int32_t getTotalVariables() const {
            return totalVariables;
        }

        __forceinline uint32_t globalOffsetByMangledName ( uint64_t mnh ) const {
            auto it = tabGMnLookup->find(mnh);
            DAS_ASSERT(it!=tabGMnLookup->end());
            return it->second;
        }
        __forceinline uint64_t adBySid ( uint64_t sid ) const {
            auto it = tabAdLookup->find(sid);
            DAS_ASSERT(it!=tabAdLookup->end());
            return it->second;
        }
        __forceinline SimFunction * fnByMangledName ( uint64_t mnh ) {
            if ( mnh==0 ) return nullptr;
            auto it = tabMnLookup->find(mnh);
            return it!=tabMnLookup->end() ? it->second : nullptr;
        }

        SimFunction * findFunction ( const char * name ) const;
        SimFunction * findFunction ( const char * name, bool & isUnique ) const;
        vector<SimFunction *> findFunctions ( const char * name ) const;
        int findVariable ( const char * name ) const;
        void stackWalk ( const LineInfo * at, bool showArguments, bool showLocalVariables );
        string getStackWalk ( const LineInfo * at, bool showArguments, bool showLocalVariables, bool showOutOfScope = false, bool stackTopOnly = false );
        void runInitScript ();
        bool runShutdownScript ();

        virtual void to_out ( const LineInfo * at, const char * message );   // output to stdout or equivalent
        virtual void to_err ( const LineInfo * at, const char * message );   // output to stderr or equivalent
        virtual void breakPoint(const LineInfo & info, const char * reason = "breakpoint", const char * text = ""); // what to do in case of breakpoint

        __forceinline vec4f * abiArguments() {
            return abiArg;
        }

        __forceinline vec4f * abiThisBlockArguments() {
            return abiThisBlockArg;
        }

        __forceinline vec4f & abiResult() {
            return result;
        }

        __forceinline char * abiCopyOrMoveResult() {
            return (char *) abiCMRES;
        }

        DAS_EVAL_ABI __forceinline vec4f call(const SimFunction * fn, vec4f * args, LineInfo * line) {
            // PUSH
            char * EP, *SP;
            if (!stack.push(fn->stackSize, EP, SP)) {
                throw_error_at(line, "stack overflow while calling %s",fn->mangledName);
                return v_zero();
            }
            // fill prologue
            auto aa = abiArg;
            abiArg = args;
#if DAS_SANITIZER
            memset(stack.sp(), 0xcd, fn->stackSize);
#endif
#if DAS_ENABLE_STACK_WALK
            Prologue * pp = (Prologue *)stack.sp();
            pp->info = fn->debugInfo;
            pp->arguments = args;
            pp->cmres = nullptr;
            pp->line = line;
#endif
            // CALL
            fn->code->eval(*this);
            stopFlags = 0;
            // POP
            abiArg = aa;
            stack.pop(EP, SP);
            return result;
        }

        DAS_EVAL_ABI __forceinline vec4f callOrFastcall(const SimFunction * fn, vec4f * args, LineInfo * line) {
            if ( fn->fastcall ) {
                auto aa = abiArg;
                abiArg = args;
                result = fn->code->eval(*this);
                stopFlags = 0;
                abiArg = aa;
                return result;
            } else {
                // PUSH
                char * EP, *SP;
                if (!stack.push(fn->stackSize, EP, SP)) {
                    throw_error_at(line, "stack overflow while calling %s",fn->mangledName);
                }
                // fill prologue
                auto aa = abiArg;
                abiArg = args;
#if DAS_SANITIZER
                memset(stack.sp(), 0xcd, fn->stackSize);
#endif
#if DAS_ENABLE_STACK_WALK
                Prologue * pp = (Prologue *)stack.sp();
                pp->info = fn->debugInfo;
                pp->arguments = args;
                pp->cmres = nullptr;
                pp->line = line;
#endif
                // CALL
                fn->code->eval(*this);
                stopFlags = 0;
                // POP
                abiArg = aa;
                stack.pop(EP, SP);
                return result;
            }
        }

        DAS_EVAL_ABI __forceinline vec4f callWithCopyOnReturn(const SimFunction * fn, vec4f * args, void * cmres, LineInfo * line) {
            // PUSH
            char * EP, *SP;
            if (!stack.push(fn->stackSize, EP, SP)) {
                throw_error_at(line, "stack overflow while calling %s",fn->mangledName);
            }
            // fill prologue
            auto aa = abiArg; auto acm = abiCMRES;
            abiArg = args; abiCMRES = cmres;
#if DAS_SANITIZER
            memset(stack.sp(), 0xcd, fn->stackSize);
#endif
#if DAS_ENABLE_STACK_WALK
            Prologue * pp = (Prologue *)stack.sp();
            pp->info = fn->debugInfo;
            pp->arguments = args;
            pp->cmres = cmres;
            pp->line = line;
#endif
            // CALL
            fn->code->eval(*this);
            stopFlags = 0;
            // POP
            abiArg = aa; abiCMRES = acm;
            stack.pop(EP, SP);
            return result;
        }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4701)
#pragma warning(disable:4324)
#endif

        DAS_EVAL_ABI __forceinline vec4f invoke(const Block &block, vec4f * args, void * cmres, LineInfo * line ) {
            char * EP, *SP;
            vec4f * TBA = nullptr;
            char * STB = stack.bottom();
#if DAS_ENABLE_STACK_WALK
            if (!stack.push_invoke(sizeof(Prologue), block.stackOffset, EP, SP)) {
                throw_error_at(line, "stack overflow during invoke");
            }
            Prologue * pp = (Prologue *)stack.ap();
            pp->block = (Block *)(intptr_t(&block) | 1);
            pp->arguments = args;
            pp->cmres = cmres;
            pp->line = line;
#else
            finfo;
            stack.invoke(block.stackOffset, EP, SP);
#endif
            BlockArguments * __restrict ba = nullptr;
            BlockArguments saveArguments;
            if ( block.argumentsOffset || cmres ) {
                ba = (BlockArguments *) ( STB + block.argumentsOffset );
                saveArguments = *ba;
                ba->arguments = args;
                ba->copyOrMoveResult = (char *) cmres;
                TBA = abiThisBlockArg;
                abiThisBlockArg = args;
            }
            vec4f * __restrict saveFunctionArguments = abiArg;
            abiArg = block.functionArguments;
            vec4f block_result = block.body->eval(*this);
            abiArg = saveFunctionArguments;
            if ( ba ) {
                *ba = saveArguments;
                abiThisBlockArg = TBA;
            }
            stack.pop(EP, SP);
            return block_result;
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

        template <typename Fn>
        DAS_EVAL_ABI vec4f invokeEx(const Block &block, vec4f * args, void * cmres, Fn && when, LineInfo * line);

        template <typename Fn>
        DAS_EVAL_ABI vec4f callEx(const SimFunction * fn, vec4f *args, void * cmres, LineInfo * line, Fn && when) {
            // PUSH
            char * EP, *SP;
            if(!stack.push(fn->stackSize,EP,SP)) {
                throw_error_at(line, "stack overflow while calling %s",fn->mangledName);
            }
            // fill prologue
            auto aa = abiArg; auto acm = cmres;
            abiArg = args;  abiCMRES = cmres;
#if DAS_SANITIZER
            memset(stack.sp(), 0xcd, fn->stackSize);
#endif
#if DAS_ENABLE_STACK_WALK
            Prologue * pp           = (Prologue *) stack.sp();
            pp->info                = fn->debugInfo;
            pp->arguments           = args;
            pp->cmres               = cmres;
            pp->line                = line;
#endif
            // CALL
            when(fn->code);
            stopFlags = 0;
            // POP
            abiArg = aa; abiCMRES = acm;
            stack.pop(EP,SP);
            return result;
        }

        __forceinline const char * getException() const {
            return exception;
        }

        void relocateCode( bool pwh = false );
        void announceCreation();
        void collectStringHeap(LineInfo * at, bool validate);
        void collectHeap(LineInfo * at, bool stringHeap, bool validate);
        void reportAnyHeap(LineInfo * at, bool sth, bool rgh, bool rghOnly, bool errorsOnly);
        void instrumentFunction ( SimFunction * , bool isInstrumenting, uint64_t userData, bool threadLocal );
        void instrumentContextNode ( const Block & blk, bool isInstrumenting, Context * context, LineInfo * line );
        void clearInstruments();
        void runVisitor ( SimVisitor * vis ) const;

        uint64_t getSharedMemorySize() const;
        uint64_t getUniqueMemorySize() const;

        void resetProfiler();
        void collectProfileInfo( TextWriter & tout );

        vector<FileInfo *> getAllFiles() const;

        char * intern ( const char * str );
        char * intern ( const char * str, uint32_t len );

        void bpcallback ( const LineInfo & at );
        void instrumentFunctionCallback ( SimFunction * sim, bool entering, uint64_t userData );
        void instrumentFunctionCallbackThreadLocal ( SimFunction * sim, bool entering, uint64_t userData );
        void instrumentCallback ( const LineInfo & at );

        uint64_t getCodeAllocatorId() { return (uint64_t) code.get(); }

#define DAS_SINGLE_STEP(context,at,forceStep) \
    context.singleStep(at,forceStep);

        __forceinline void singleStep ( const LineInfo & at, bool forceStep ) {
            if ( singleStepMode ) {
                if ( hwBpIndex!=-1 ) {
                    char reason[128];
                    snprintf(reason, sizeof(reason), "hardware breakpoint 0x%p", hwBpAddress);
                    breakPoint(at, "exception",reason);
                    hwBpIndex = -1;
                } else if ( forceStep || singleStepAt==nullptr || (singleStepAt->fileInfo!=at.fileInfo || singleStepAt->line!=at.line) ) {
                    singleStepAt = &at;
                    bpcallback(at);
                }
            }
        }

        __forceinline void setSingleStep ( bool step ) { singleStepMode = step; }
        void triggerHwBreakpoint ( void * addr, int index );

        __forceinline bool isGlobalPtr ( char * ptr ) const { return globals<=ptr && ptr<(globals+globalsSize); }
        __forceinline bool isSharedPtr ( char * ptr ) const { return shared<=ptr && ptr<(shared+sharedSize); }

        void addGcRoot ( void * ptr, TypeInfo * type );
        void removeGcRoot ( void * ptr );
        template <typename TT>
        __forceinline void foreach_gc_root ( TT && fn ) {
            for ( auto & gr : gcRoots ) {
                fn(gr.first, gr.second);
            }
        }
    public:
        uint64_t *                      annotationData = nullptr;
        smart_ptr<StringHeapAllocator>  stringHeap;
        smart_ptr<AnyHeapAllocator>     heap;
        bool                            persistent = false;
        char *                          globals = nullptr;
        char *                          shared = nullptr;
        shared_ptr<ConstStringAllocator> constStringHeap;
        shared_ptr<NodeAllocator>       code;
        shared_ptr<DebugInfoAllocator>  debugInfo;
        StackAllocator                  stack;
        uint32_t                        insideContext = 0;
        bool                            ownStack = false;
        bool                            shutdown = false;
        bool                            breakOnException = false;
        bool                            alwaysStackWalkOnException = false;
    public:
        string                          name;
        Bitfield                        category = 0;
    public:
        vec4f *         abiThisBlockArg;
        vec4f *         abiArg;
        void *          abiCMRES;
    public:
        string          exceptionMessage;
        const char *    exception = nullptr;
        const char *    last_exception = nullptr;
        LineInfo        exceptionAt;
        jmp_buf *       throwBuf = nullptr;
    protected:
        GlobalVariable * globalVariables = nullptr;
        bool     globalsOwner = true;
        uint32_t sharedSize = 0;
        bool     sharedOwner = true;
        uint32_t globalsSize = 0;
        uint32_t globalInitStackSize = 0;
        SimFunction * functions = nullptr;
        int totalVariables = 0;
        int totalFunctions = 0;
        SimFunction ** initFunctions = nullptr;
        int totalInitFunctions = 0;
    public:
        bool skipLockChecks = false;
        SimNode * aotInitScript = nullptr;
    protected:
        bool            debugger = false;
        volatile bool   singleStepMode = false;
        void *          hwBpAddress = nullptr;
        int             hwBpIndex = -1;
        const LineInfo * singleStepAt = nullptr;
    public:
        shared_ptr<das_hash_map<uint64_t,SimFunction *>> tabMnLookup;
        shared_ptr<das_hash_map<uint64_t,uint32_t>> tabGMnLookup;
        shared_ptr<das_hash_map<uint64_t,uint64_t>> tabAdLookup;
    public:
        class Program * thisProgram = nullptr;
        class DebugInfoHelper * thisHelper = nullptr;
    public:
        uint32_t stopFlags = 0;
        uint32_t gotoLabel = 0;
        vec4f result;
    public:
        recursive_mutex * contextMutex = nullptr;
    protected:
        das_hash_map<void *, TypeInfo *> gcRoots;
    public:
#if DAS_ENABLE_SMART_PTR_TRACKING
        static vector<smart_ptr<ptr_ref_count>> sptrAllocations;
#endif
    };

    struct DebugAgentInstance {
        DebugAgentPtr   debugAgent;
        ContextPtr      debugAgentContext;
    };

    void tickDebugAgent ( );
    void collectDebugAgentState ( Context & ctx, const LineInfo & at );
    void onBreakpointsReset ( const char * file, int breakpointsNum );
    void tickSpecificDebugAgent ( const char * name );
    void installDebugAgent ( DebugAgentPtr newAgent, const char * category, LineInfoArg * at, Context * context );
    void installThreadLocalDebugAgent ( DebugAgentPtr newAgent, LineInfoArg * at, Context * context );
    void shutdownDebugAgent();
    void forkDebugAgentContext ( Func exFn, Context * context, LineInfoArg * lineinfo );
    bool isInDebugAgentCreation();
    bool hasDebugAgentContext ( const char * category, LineInfoArg * at, Context * context );
    void lockDebugAgent ( const TBlock<void> & blk, Context * context, LineInfoArg * line );
    Context & getDebugAgentContext ( const char * category, LineInfoArg * at, Context * context );
    void onCreateCppDebugAgent ( const char * category, function<void (Context *)> && );
    void onDestroyCppDebugAgent ( const char * category, function<void (Context *)> && );
    void onLogCppDebugAgent ( const char * category, function<bool(Context *, const LineInfo * at, int, const char *)> && lmb );
    void uninstallCppDebugAgent ( const char * category );

    class SharedStackGuard {
    public:
        DAS_THREAD_LOCAL static StackAllocator *lastContextStack;
        SharedStackGuard() = delete;
        SharedStackGuard(const SharedStackGuard &) = delete;
        SharedStackGuard & operator = (const SharedStackGuard &) = delete;
        __forceinline SharedStackGuard(Context & currentContext, StackAllocator & shared_stack) : savedStack(0) {
            savedStack.copy(currentContext.stack);
            currentContext.stack.copy(lastContextStack ? *lastContextStack : shared_stack);
            saveLastContextStack = lastContextStack;
            lastContextStack = &currentContext.stack;
        }
        __forceinline ~SharedStackGuard() {
            lastContextStack->copy(savedStack);
            lastContextStack = saveLastContextStack;
            savedStack.letGo();
        }
    protected:
        StackAllocator savedStack;
        StackAllocator *saveLastContextStack = nullptr;
    };

    struct DataWalker;

    struct Iterator {
        virtual ~Iterator() {}
        virtual bool first ( Context & context, char * value ) = 0;
        virtual bool next  ( Context & context, char * value ) = 0;
        virtual void close ( Context & context, char * value ) = 0;    // can't throw
        virtual void walk ( DataWalker & ) { }
       bool isOpen = false;
    };

    struct PointerDimIterator : Iterator {
        PointerDimIterator  ( char ** d, uint32_t cnt, uint32_t sz )
            : data(d), data_end(d+cnt), size(sz) {}
        virtual bool first(Context &, char * _value) override;
        virtual bool next(Context &, char * _value) override;
        virtual void close(Context & context, char * _value) override;
        char ** data;
        char ** data_end;
        uint32_t size;
    };

    struct Sequence {
        mutable Iterator * iter;
    };

#if DAS_ENABLE_PROFILER

__forceinline void profileNode ( SimNode * node ) {
    if ( auto fi = node->debugInfo.fileInfo ) {
        auto li = node->debugInfo.line;
        auto & pdata = fi->profileData;
        if ( pdata.size() <= li ) pdata.resize ( li + 1 );
        pdata[li] ++;
    }
}

#endif

#define DAS_EVAL_NODE               \
    EVAL_NODE(Ptr,char *);          \
    EVAL_NODE(Int,int32_t);         \
    EVAL_NODE(UInt,uint32_t);       \
    EVAL_NODE(Int64,int64_t);       \
    EVAL_NODE(UInt64,uint64_t);     \
    EVAL_NODE(Float,float);         \
    EVAL_NODE(Double,double);       \
    EVAL_NODE(Bool,bool);

#define DAS_NODE(TYPE,CTYPE)                                         \
    DAS_EVAL_ABI virtual vec4f eval ( das::Context & context ) override {         \
        return das::cast<CTYPE>::from(compute(context));             \
    }                                                                \
    virtual CTYPE eval##TYPE ( das::Context & context ) override {   \
        return compute(context);                                     \
    }

#define DAS_PTR_NODE    DAS_NODE(Ptr,char *)
#define DAS_BOOL_NODE   DAS_NODE(Bool,bool)
#define DAS_INT_NODE    DAS_NODE(Int,int32_t)
#define DAS_FLOAT_NODE  DAS_NODE(Float,float)
#define DAS_DOUBLE_NODE DAS_NODE(Double,double)

    template <typename TT>
    struct EvalTT { static __forceinline TT eval ( Context & context, SimNode * node ) {
        return cast<TT>::to(node->eval(context)); }};
    template <>
    struct EvalTT<int32_t> { static __forceinline int32_t eval ( Context & context, SimNode * node ) {
        return node->evalInt(context); }};
    template <>
    struct EvalTT<uint32_t> { static __forceinline uint32_t eval ( Context & context, SimNode * node ) {
        return node->evalUInt(context); }};
    template <>
    struct EvalTT<int64_t> { static __forceinline int64_t eval ( Context & context, SimNode * node ) {
        return node->evalInt64(context); }};
    template <>
    struct EvalTT<uint64_t> { static __forceinline uint64_t eval ( Context & context, SimNode * node ) {
        return node->evalUInt64(context); }};
    template <>
    struct EvalTT<float> { static __forceinline float eval ( Context & context, SimNode * node ) {
        return node->evalFloat(context); }};
    template <>
    struct EvalTT<double> { static __forceinline double eval ( Context & context, SimNode * node ) {
        return node->evalDouble(context); }};
    template <>
    struct EvalTT<bool> { static __forceinline bool eval ( Context & context, SimNode * node ) {
        return node->evalBool(context); }};
    template <>
    struct EvalTT<char *> { static __forceinline char * eval ( Context & context, SimNode * node ) {
        return node->evalPtr(context); }};

    // FUNCTION CALL
    struct SimNode_CallBase : SimNode {
        SimNode_CallBase ( const LineInfo & at ) : SimNode(at) {}
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override;
        void visitCall ( SimVisitor & vis );
        __forceinline void evalArgs ( Context & context, vec4f * argValues ) {
            for ( int i=0, is=nArguments; i!=is && !context.stopFlags; ++i ) {
                argValues[i] = arguments[i]->eval(context);
            }
        }
        SimNode * visitOp1 ( SimVisitor & vis, const char * op, int typeSize, const char * typeName );
        SimNode * visitOp2 ( SimVisitor & vis, const char * op, int typeSize, const char * typeName );
        SimNode * visitOp3 ( SimVisitor & vis, const char * op, int typeSize, const char * typeName );
#define EVAL_NODE(TYPE,CTYPE)\
        virtual CTYPE eval##TYPE ( Context & context ) override {   \
            return cast<CTYPE>::to(eval(context));                  \
        }
        DAS_EVAL_NODE
#undef  EVAL_NODE
        SimNode ** arguments = nullptr;
        TypeInfo ** types = nullptr;
        SimFunction * fnPtr = nullptr;
        int32_t  nArguments = 0;
        SimNode * cmresEval = nullptr;
        void * aotFunction = nullptr;
        // uint32_t stackTop = 0;
    };

    struct SimNode_Final : SimNode {
        SimNode_Final ( const LineInfo & a ) : SimNode(a) {}
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override;
        void visitFinal ( SimVisitor & vis );
        virtual SimNode * visit ( SimVisitor & vis ) override;
        __forceinline void evalFinal ( Context & context ) {
            if ( totalFinal ) {
                auto SF = context.stopFlags;
                auto RE = context.abiResult();
                context.stopFlags = 0;
                for ( uint32_t i=0, is=totalFinal; i!=is; ++i ) {
                    finalList[i]->eval(context);
                }
                context.stopFlags = SF;
                context.abiResult() = RE;
            }
        }
#if DAS_DEBUGGER
        __forceinline void evalFinalSingleStep ( Context & context ) {
            if ( totalFinal ) {
                auto SF = context.stopFlags;
                auto RE = context.abiResult();
                context.stopFlags = 0;
                for ( uint32_t i=0, is=totalFinal; i!=is; ++i ) {
                    DAS_SINGLE_STEP(context,finalList[i]->debugInfo,false);
                    finalList[i]->eval(context);
                }
                context.stopFlags = SF;
                context.abiResult() = RE;
            }
        }
#endif
        SimNode ** finalList = nullptr;
        uint32_t totalFinal = 0;
    };

    struct SimNode_Block : SimNode_Final {
        SimNode_Block ( const LineInfo & at ) : SimNode_Final(at) {}
        virtual bool rtti_node_isBlock() const override { return true; }
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override;
        void visitBlock ( SimVisitor & vis );
        void visitLabels ( SimVisitor & vis );
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
        SimNode ** list = nullptr;
        uint32_t total = 0;
        uint64_t annotationDataSid = 0;
        uint32_t *  labels = nullptr;
        uint32_t    totalLabels = 0;
    };

    struct SimNodeDebug_Block : SimNode_Block {
        SimNodeDebug_Block ( const LineInfo & at ) : SimNode_Block(at) {}
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    struct SimNode_BlockNF : SimNode_Block {
        SimNode_BlockNF ( const LineInfo & at ) : SimNode_Block(at) {}
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    struct SimNodeDebug_BlockNF : SimNode_BlockNF {
        SimNodeDebug_BlockNF ( const LineInfo & at ) : SimNode_BlockNF(at) {}
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    struct SimNode_BlockWithLabels : SimNode_Block {
        SimNode_BlockWithLabels ( const LineInfo & at ) : SimNode_Block(at) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    struct SimNodeDebug_BlockWithLabels : SimNode_BlockWithLabels {
        SimNodeDebug_BlockWithLabels ( const LineInfo & at ) : SimNode_BlockWithLabels(at) {}
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

    struct SimNode_ForBase : SimNode_Block {
        SimNode_ForBase ( const LineInfo & at ) : SimNode_Block(at) {}
        SimNode * visitFor ( SimVisitor & vis, int total, const char * loopName );
        void allocateFor ( NodeAllocator * code, uint32_t t );
        virtual SimNode * copyNode ( Context & context, NodeAllocator * code ) override;
        SimNode **  sources = nullptr;
        uint32_t *  strides = nullptr;
        uint32_t *  stackTop = nullptr;
        uint32_t    size;
        uint32_t    totalSources;
    };

    struct SimNode_Delete : SimNode {
        SimNode_Delete ( const LineInfo & a, SimNode * s, uint32_t t )
            : SimNode(a), subexpr(s), total(t) {}
        virtual SimNode * visit ( SimVisitor & vis ) override;
        SimNode *   subexpr;
        uint32_t    total;
    };

    struct SimNode_ClosureBlock : SimNode_Block {
        SimNode_ClosureBlock ( const LineInfo & at, bool nr, bool c0, uint64_t ad )
            : SimNode_Block(at), annotationData(ad), flags(0) {
                this->needResult = nr;
                this->code0 = c0;
            }
        virtual SimNode * visit ( SimVisitor & vis ) override;
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
        uint64_t annotationData = 0;
        union {
            uint32_t flags;
            struct {
                bool needResult : 1;
                bool code0 : 1;
            };
        };
    };

    struct SimNodeDebug_ClosureBlock : SimNode_ClosureBlock {
        SimNodeDebug_ClosureBlock ( const LineInfo & at, bool nr, bool c0, uint64_t ad )
            : SimNode_ClosureBlock(at,nr,c0,ad) { }
        DAS_EVAL_ABI virtual vec4f eval ( Context & context ) override;
    };

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4701)
#pragma warning(disable:4324)
#endif
    template <typename Fn>
    DAS_EVAL_ABI vec4f Context::invokeEx(const Block &block, vec4f * args, void * cmres, Fn && when, LineInfo * line ) {
        char * EP, *SP;
        vec4f * TBA = nullptr;
        char * STB = stack.bottom();
#if DAS_ENABLE_STACK_WALK
        if (!stack.push_invoke(sizeof(Prologue), block.stackOffset, EP, SP)) {
            throw_error_at(line, "stack overflow during invokeEx");
        }
        Prologue * pp = (Prologue *)stack.ap();
        pp->block = (Block *)(intptr_t(&block) | 1);
        pp->arguments = args;
        pp->cmres = cmres;
        pp->line = line;
#else
        finfo;
        stack.invoke(block.stackOffset, EP, SP);
#endif
        BlockArguments * ba = nullptr;
        BlockArguments saveArguments;
        if ( block.argumentsOffset || cmres ) {
            ba = (BlockArguments *) ( STB + block.argumentsOffset );
            saveArguments = *ba;
            ba->arguments = args;
            ba->copyOrMoveResult = (char *) cmres;
            TBA = abiThisBlockArg;
            abiThisBlockArg = args;
        }
        vec4f * __restrict saveFunctionArguments = abiArg;
        abiArg = block.functionArguments;
        SimNode_ClosureBlock * cb = (SimNode_ClosureBlock *) block.body;
        when(cb->code0 ? cb->list[0] : block.body);
        abiArg = saveFunctionArguments;
        if ( ba ) {
            *ba = saveArguments;
            abiThisBlockArg = TBA;
        }
        stack.pop(EP, SP);
        return result;
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

#include "daScript/simulate/simulate_visit_op_undef.h"


