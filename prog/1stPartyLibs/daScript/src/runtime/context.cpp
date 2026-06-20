#include "daScript/simulate/simulate.h"
#include "daScript/ast/ast.h"
#include "daScript/misc/fpe.h"

#include <stdarg.h>

namespace das
{
    // Context
    std::recursive_mutex g_DebugAgentMutex;
    das_safe_map<string, DebugAgentInstance>   g_DebugAgents;
    static DAS_THREAD_LOCAL(bool) g_isInDebugAgentCreation;
    extern atomic<int> g_envTotal;

    template <typename TT>
    void on_debug_agent_mutex ( const TT & lmbd ) {
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        lmbd ();
    }

    template <typename TT>
    void for_each_debug_agent ( const TT & lmbd ) {
        if ( g_envTotal > 0 && *daScriptEnvironment::g_threadLocalDebugAgent && (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent ) {
            lmbd ( (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent );
        }
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        for ( auto & it : g_DebugAgents ) {
            if ( !it.second.debugAgent ) continue;
            lmbd ( it.second.debugAgent );
        }
    }

    void dapiReportContextState ( Context & ctx, const char * category, const char * name, const TypeInfo * info, void * data ) {
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onVariable(&ctx,category,name,(TypeInfo*)info,data);
        });
    }

    void dapiSimulateContext ( Context & ctx ) {
        for_each_debug_agent([&]( const DebugAgentPtr & pAgent ){
            pAgent->onSimulateContext(&ctx);
        });
    }

    void dapiUserCommand ( const char * command ) {
        if ( !command ) command = "";
        bool any = false;
        for_each_debug_agent([&]( const DebugAgentPtr & pAgent ){
            if ( !any ) any = pAgent->onUserCommand(command);
        });
    }

    void dapiOnBeforeGC ( Context & ctx ) {
        for_each_debug_agent([&]( const DebugAgentPtr & pAgent ){
            pAgent->onBeforeGC(&ctx);
        });
    }

    void dapiOnAfterGC ( Context & ctx ) {
        for_each_debug_agent([&]( const DebugAgentPtr & pAgent ){
            pAgent->onAfterGC(&ctx);
        });
    }

    Context::Context(uint32_t stackSize, bool ph) : stack(stackSize) {
        ref_count_magic = TRACK_PTR_CONTEXT;
        code = make_shared<NodeAllocator>();
        constStringHeap = make_shared<ConstStringAllocator>();
        debugInfo = make_shared<DebugInfoAllocator>();
        ownStack = (stackSize != 0);
        persistent = ph;
    }

    void Context::setup(int totalVars, uint32_t globalStringHeapSize, CodeOfPolicies policies, AnnotationArgumentList options) {
        verySafeContext = options.getBoolOption("very_safe_context",policies.very_safe_context);
        breakOnException |= policies.debugger;
        gcEnabled = options.getBoolOption("gc", false);
        gcLogTime = options.getBoolOption("log_gc_time", policies.log_gc_time);
        persistent = options.getBoolOption("persistent_heap", policies.persistent_heap);
        if ( persistent ) {
            heap = make_unique<PersistentHeapAllocator>();
            stringHeap = make_unique<PersistentStringAllocator>();
        } else {
            heap = make_unique<LinearHeapAllocator>();
            stringHeap = make_unique<LinearStringAllocator>();
        }
        heap->setInitialSize ( options.getIntOption("heap_size_hint", policies.heap_size_hint) );
        heap->setLimit ( options.getUInt64OptionEx("heap_size_limit", "max_heap_allocated", policies.max_heap_allocated) );
        stringHeap->setInitialSize ( options.getIntOption("string_heap_size_hint", policies.string_heap_size_hint) );
        stringHeap->setLimit ( options.getUInt64OptionEx("string_heap_size_limit", "max_string_heap_allocated", policies.max_string_heap_allocated) );
        bool track = options.getBoolOption("track_allocations", policies.track_allocations);
        heap->setTrackAllocations(track);
        stringHeap->setTrackAllocations(track);
        constStringHeap = make_shared<ConstStringAllocator>();
        totalVariables = totalVars;
        if ( globalStringHeapSize ) {
            constStringHeap->setInitialSize(globalStringHeapSize);
        }
        globalVariables = (GlobalVariable *) code->allocate( uint32_t(totalVars*sizeof(GlobalVariable)) );
        globalsSize = 0;
        sharedSize = 0;
    }

    void Context::strip() {
        stringHeap.reset();
        heap.reset();
        stack.strip();
        if ( globals && globalsOwner ) {
            das_aligned_free16(globals);
            globals = nullptr;
        }
        if ( shared && sharedOwner ) {
            das_aligned_free16(shared);
            shared = nullptr;
        }
    }

    void Context::logMemInfo(TextWriter & tw) {
        uint64_t bytesTotal = 0, bytesUsed = 0;
    // context
        tw << "Context: 0x" << HEX << intptr_t(this) << DEC << " '" << name << "' use_count = " << use_count() << "\n";
        bytesTotal = bytesUsed = sizeof(Context);
    // stringHeap
        if ( stringHeap ) {
            tw << "\tstringHeap: " << stringHeap->bytesAllocated() << " of " << stringHeap->totalAlignedMemoryAllocated()
                << ", depth = " << stringHeap->depth() << "\n";
            bytesTotal += stringHeap->totalAlignedMemoryAllocated();
            bytesUsed += stringHeap->bytesAllocated();
        }
    // constStringHeap
        if ( constStringHeap ) {
            tw << "\tconstStringHeap: " << constStringHeap->bytesAllocated() << " of " << constStringHeap->totalAlignedMemoryAllocated()
                << ", depth = " << constStringHeap->depth() << "\n";
            bytesTotal += constStringHeap->totalAlignedMemoryAllocated();
            bytesUsed += constStringHeap->bytesAllocated();
        }
    // heap
        if ( heap ) {
            tw << "\theap: " << heap->bytesAllocated() << " of " << heap->totalAlignedMemoryAllocated()
                << ", depth = " << heap->depth() << "\n";
            bytesTotal += heap->totalAlignedMemoryAllocated();
            bytesUsed += heap->bytesAllocated();
        }
    // code
        if ( code ) {
            tw << "\tcode: " << code->bytesAllocated() << " of " << code->totalAlignedMemoryAllocated()
                << ", depth = " << code->depth() << "\n";
            tw << "\t\ttableMN[" << tabMnLookup->size() << "]\n";
            tw << "\t\ttableGMN[" << tabGMnLookup->size() << "]\n";
            tw << "\t\ttableAd[" << tabAdLookup->size() << "]\n";
            int aotf = 0;
            for ( int i=0, is=totalFunctions; i!=is; ++i ) {
                if ( functions[i].aotFunction ) aotf++;
            }
            if ( aotf>0 ) {
                int64_t saot = int64_t(sizeof(SimNode_CallBase));
                tw << "\tAOT " << aotf << " of " << totalFunctions << ", " << aotf << " x sizeof(SimNode_Aot = " << saot << ") = " << aotf*saot << "\n";
            }
            bytesTotal += code->totalAlignedMemoryAllocated();
            bytesUsed += code->bytesAllocated();
        }
    // debugInfo
        if ( debugInfo ) {
            tw << "\tdebugInfo: " << debugInfo->bytesAllocated() << " of " << debugInfo->totalAlignedMemoryAllocated()
                << ", depth = " << debugInfo->depth() << "\n";
            bytesTotal += debugInfo->totalAlignedMemoryAllocated();
            bytesUsed += debugInfo->bytesAllocated();
        }
    // stack
        if ( stack.bottom() ) {
            tw << "\tstack: " << stack.size() << "\n";
            bytesTotal += stack.size();
            bytesUsed += stack.size();
        }
    // functions
        //tw << "\functions table: " << totalFunctions*sizeof(SimFunction) << "\n";
        //bytesTotal += totalFunctions*sizeof(SimFunction);
        //bytesUsed += totalFunctions*sizeof(SimFunction);
    // globals
        //tw << "\tglobals table: " << totalVariables*sizeof(GlobalVariable) << "\n";
        //bytesTotal += totalVariables*sizeof(GlobalVariable);
        //bytesUsed += totalVariables*sizeof(GlobalVariable);
    // globals data
        if ( globals ) {
            tw << "\tglobal data: " << globalsSize << "\n";
            bytesTotal += globalsSize;
            bytesUsed += globalsSize;
        }
    // shared
        if ( shared ) {
            tw << "\tshared data: " << sharedSize << "\n";
            bytesTotal += sharedSize;
            bytesUsed += sharedSize;
        }
    // total
        tw << "-----------------------------------------------------------------\n";
        tw << "  total " << bytesTotal << ", wasted "
            << ((bytesTotal - bytesUsed)*100/bytesTotal) << "% ("
            << (bytesTotal - bytesUsed) << ")\n";
    }

    void Context::makeWorkerFor(const Context & ctx)
    {
        if (code == ctx.code)
            return;

        code = ctx.code;
        constStringHeap = ctx.constStringHeap;
        debugInfo = ctx.debugInfo;
        thisProgram = ctx.thisProgram;
        thisHelper = ctx.thisHelper;
        category.value = ctx.category.value;

        // globals (on condition that all context globals are read-only)
        globals = ctx.globals;
        globalsOwner = false;
        annotationData = ctx.annotationData;
        globalsSize = ctx.globalsSize;
        globalInitStackSize = ctx.globalInitStackSize;
        globalVariables = ctx.globalVariables;
        totalVariables = ctx.totalVariables;

        // shared
        sharedSize = ctx.sharedSize;
        shared = ctx.shared;
        sharedOwner = false;
        // functions
        functions = ctx.functions;
        totalFunctions = ctx.totalFunctions;

        // mangled name table
        tabMnLookup = ctx.tabMnLookup;
        tabGMnLookup = ctx.tabGMnLookup;
        tabAdLookup = ctx.tabAdLookup;
    }

    void Context::freeGlobalsAndShared() {
        if ( globals && globalsOwner ) {
            das_aligned_free16(globals);
            globals = nullptr;
        }
        if ( shared && sharedOwner ) {
            das_aligned_free16(shared);
            shared = nullptr;
        }
    }

    void Context::allocateGlobalsAndShared() {
        freeGlobalsAndShared();
        globals = globalsSize ? (char *) das_aligned_alloc16(globalsSize) : nullptr;
        shared = (sharedOwner && sharedSize) ? (char *) das_aligned_alloc16(sharedSize) : nullptr;
        if ( shared ) memset(shared, 0, sharedSize);
        globalsOwner = true;
        sharedOwner = true;
    }
    uint64_t Context::getSharedMemorySize() const {
        uint64_t mem = 0;
        mem += code ? code->totalAlignedMemoryAllocated() : 0;
        mem += constStringHeap ? constStringHeap->totalAlignedMemoryAllocated() : 0;
        mem += debugInfo ? debugInfo->totalAlignedMemoryAllocated() : 0;
        mem += sharedSize;
        return mem;
    }

    uint64_t Context::getUniqueMemorySize() const {
        uint64_t mem = 0;
        mem += globalsSize;
        mem += stack.size();
        mem += heap ? heap->totalAlignedMemoryAllocated() : 0;
        mem += stringHeap ? stringHeap->totalAlignedMemoryAllocated() : 0;
        return mem;
    }


    Context::Context(const Context & ctx, uint32_t category_)
        : Context(ctx, CopyOptions{category_, 0}) {
    }

    Context::Context(const Context & ctx, const CopyOptions & opts)
        : stack(opts.stackSize ? opts.stackSize : ctx.stack.size()) {
        ref_count_magic = TRACK_PTR_CONTEXT;
        verySafeContext = ctx.verySafeContext;
        persistent = ctx.persistent;
        gcEnabled = ctx.gcEnabled;
        code = ctx.code;
        constStringHeap = ctx.constStringHeap;
        debugInfo = ctx.debugInfo;
        thisProgram = ctx.thisProgram;
        thisHelper = ctx.thisHelper;
        name = "clone of " + ctx.name;
        category.value = opts.category;
        ownStack = (ctx.stack.size() != 0);
        if ( persistent ) {
            heap = make_unique<PersistentHeapAllocator>();
            stringHeap = make_unique<PersistentStringAllocator>();
        } else {
            heap = make_unique<LinearHeapAllocator>();
            stringHeap = make_unique<LinearStringAllocator>();
        }
        // heap
        heap->setInitialSize(ctx.heap->getInitialSize());
        heap->setLimit(ctx.heap->getLimit());
        heap->setTrackAllocations(ctx.heap->isTrackingAllocations());
        stringHeap->setInitialSize(ctx.stringHeap->getInitialSize());
        stringHeap->setIntern(ctx.stringHeap->isIntern());
        stringHeap->setLimit(ctx.stringHeap->getLimit());
        stringHeap->setTrackAllocations(ctx.stringHeap->isTrackingAllocations());
        // globals
        annotationData = ctx.annotationData;
        globalsSize = ctx.globalsSize;
        globalInitStackSize = ctx.globalInitStackSize;
        globalVariables = ctx.globalVariables;
        totalVariables = ctx.totalVariables;
        if ( ctx.globals ) {
            globals = (char *) (globalsSize ? das_aligned_alloc16(globalsSize) : nullptr);
        }
        // shared
        sharedSize = ctx.sharedSize;
        shared = ctx.shared;
        sharedOwner = false;
        // functoins
        functions = ctx.functions;
        totalFunctions = ctx.totalFunctions;
        initFunctions = ctx.initFunctions;
        totalInitFunctions = ctx.totalInitFunctions;
        // mangled name table
        tabMnLookup = ctx.tabMnLookup;
        tabGMnLookup = ctx.tabGMnLookup;
        tabAdLookup = ctx.tabAdLookup;
        // jit init script
        jitInitScript = ctx.jitInitScript;
        // threadlock_context
        if ( ctx.contextMutex ) contextMutex = new recursive_mutex;
        // register
        announceCreation();
        // now, make it good to go
        restart();
        if ( !failed ) {
            if ( stack.size() > globalInitStackSize ) {
                failed |= !runWithCatch([&]() {
                    runInitScript();
                });
            } else {
                auto ssz = max ( int(stack.size()), 16384 ) + globalInitStackSize;
                StackAllocator init_stack(ssz);
                SharedStackGuard init_guard(*this, init_stack);
                failed |= !runWithCatch([&]() {
                    runInitScript();
                });
            }
            if ( failed ) {
                to_err(&exceptionAt, getException());
                clearException();
            }
        }
        restart();
    }

    void Context::addGcRoot ( void * ptr, TypeInfo * type ) {
        gcRoots[ptr] = type;
    }

    void Context::removeGcRoot ( void * ptr ) {
        gcRoots.erase(ptr);
    }

    Context::~Context() {
        on_debug_agent_mutex([&](){
            // unregister
            category.value |= uint32_t(ContextCategory::dead);
            // register
            for_each_debug_agent([&](const DebugAgentPtr & pAgent){
                pAgent->onDestroyContext(this);
            });
        });
        if ( !failed ) {
            // shutdown
            runShutdownScript();
        }
        // and free memory
        if ( globals && globalsOwner ) {
            das_aligned_free16(globals);
        }
        if ( shared && sharedOwner ) {
            das_aligned_free16(shared);
        }
        // and lock
        if ( contextMutex ) {
            delete contextMutex;
            contextMutex = nullptr;
        }
        for (auto &obj : deleteUponFinish) {
            delete obj;
        }
    }

    struct SimNodeRelocator : SimVisitor {
        shared_ptr<NodeAllocator>   newCode;
        Context * context = nullptr;
        int totalNodes = 0;
        virtual SimNode * visit ( SimNode * node ) override {
            totalNodes ++;
            return node->copyNode(*context, newCode.get());
        }
    };

    void Context::relocateCode( bool pwh ) {
        SimNodeRelocator rel;
        rel.context = this;
        rel.newCode = make_shared<NodeAllocator>();
        rel.newCode->customGrow = [&](uint64_t ) { return uint64_t(4000); };   // because SimNode_Aot is 80 bytes
        uint32_t codeSize = uint32_t(code->bytesAllocated());
        if ( code->prefixWithHeader && !pwh ) {
            // printf("[REL] %i adjusting\n", code->totalNodesAllocated);
            codeSize -= code->totalNodesAllocated * uint32_t(sizeof(NodePrefix));
        } else {
            // printf("[REL] %i not adjusting\n", code->totalNodesAllocated);
        }
        rel.newCode->prefixWithHeader = pwh;
        rel.newCode->setInitialSize(codeSize);
        SimFunction * oldFunctions = functions;
        if ( totalFunctions ) {
            SimFunction * newFunctions = (SimFunction *) rel.newCode->allocate(totalFunctions*sizeof(SimFunction));
            memcpy ( newFunctions, functions, totalFunctions*sizeof(SimFunction));
            for ( int i=0, is=totalFunctions; i!=is; ++i ) {
                newFunctions[i].name = rel.newCode->allocateName(functions[i].name);
                newFunctions[i].mangledName = rel.newCode->allocateName(functions[i].mangledName);
            }
            functions = newFunctions;
        }
        if ( totalVariables ) {
            GlobalVariable * newVariables = (GlobalVariable *) rel.newCode->allocate(totalVariables*sizeof(GlobalVariable));
            memcpy ( newVariables, globalVariables, totalVariables*sizeof(GlobalVariable));
            for ( int i=0, is=totalVariables; i!=is; ++i ) {
                newVariables[i].name = rel.newCode->allocateName(globalVariables[i].name);
            }
            globalVariables = newVariables;
        }
        // relocate mangle-name lookup
        for ( auto & kv : *tabMnLookup ) {
            auto fn = kv.second;
            if ( fn!=nullptr ) {
                if ( fn>=oldFunctions && fn<(oldFunctions+totalFunctions) ) {
                    ptrdiff_t index = fn - oldFunctions;
                    kv.second = functions + index;
                    DAS_ASSERT(fn->mangledNameHash == kv.second->mangledNameHash);
                    DAS_ASSERT(kv.second>=functions && kv.second<(functions+totalFunctions));
                    // printf("%3i - MNH 0x%8x: %s [move %p -> %p]\n", i, fn->mangledNameHash, fn->name, fn, kv.second );
                }
            }
        }
        // relocate variables
        if ( totalVariables ) {
            for ( int j=0, js=totalVariables; j!=js; ++j ) {
                auto & var = globalVariables[j];
                if ( var.init) {
                    var.init = var.init->visit(rel);
                }
            }
        }
        // relocate functions
        for ( int i=0, is=totalFunctions; i!=is; ++i ) {
            auto & fn = functions[i];
            fn.code = fn.code->visit(rel);
        }
        // swap the code
        rel.newCode->totalNodesAllocated = rel.totalNodes;
        // we need small repro of this happening. disabling the assert until i find one
        // DAS_ASSERTF(rel.newCode->depth()<=1,"after code relocation all code should be on one page");
        code = rel.newCode;
    }

    void Context::announceCreation() {
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onCreateContext(this);
        });
    }

    char * Context::intern( const char * str ) {
        if ( !str ) return nullptr;
        uint32_t len = uint32_t(strlen(str));
        return intern(str, len);
    }

    char * Context::intern ( const char * str, uint32_t len ) {
        if ( !str || !len ) return nullptr;
        char * ist = constStringHeap->intern(str,len);
        if ( !ist ) ist = stringHeap->intern(str,len);
        return ist ? ist : stringHeap->impl_allocateString(this,str,len);
    }

    class SharedDataWalker : public DataWalker {
    public:
        virtual void beforeArray ( Array * pa, TypeInfo * ) override {
            pa->shared = true;
        }
        virtual void beforeTable ( Table * pa, TypeInfo * ) override {
            pa->shared = true;
        }
    };

    void Context::runInitScript ( ) {
        DAS_ASSERTF(insideContext==0,"can't run init script on the locked context");
        char * EP, *SP;
        if(!stack.push(globalInitStackSize,EP,SP)) {
            throw_error("stack overflow in the initialization script");
            return;
        }
        vec4f args[2] = {
            cast<void *>::from(this),
            cast<bool>::from(sharedOwner)   // only init shared if we are the owner
        };
        abiArg = args;
        abiCMRES = nullptr;
        if (globals) memset(globals, 0, globalsSize);
        if ( aotInitScript ) {
            aotInitScript->eval(*this);
        } else if ( jitInitScript ) {
            jitInitScript(this);
        } else {
#if DAS_ENABLE_STACK_WALK
            FuncInfo finfo;
            memset(&finfo, 0, sizeof(finfo));
            finfo.name = (char *) "Context::runInitScript";
            // TODO: init arguments?
#endif
            for ( int i=0, is=totalVariables; i!=is && !stopFlags; ++i ) {
                auto & pv = globalVariables[i];
                if ( pv.init ) {
                    if ( sharedOwner || !pv.shared ) {
#if DAS_ENABLE_STACK_WALK
                        finfo.stackSize = globalInitStackSize;
                        Prologue * pp = (Prologue *)stack.sp();
                        pp->info = &finfo;
                        pp->arguments = nullptr;    // TODO: args
                        pp->cmres = nullptr;
                        pp->line = &pv.init->debugInfo;
#endif
                        pv.init->eval(*this);
#if DAS_ENABLE_STACK_WALK
                        pp->info = nullptr;
#endif
                    }
                }
            }
        }
        abiArg = nullptr;
        stack.pop(EP,SP);
        // run init functions
        for ( int j=0, js=totalInitFunctions; j!=js && !stopFlags; ++j ) {
            auto & pf = initFunctions[j];
            callOrFastcall(pf, nullptr, 0);
        }
        // now, share the data
        if ( sharedOwner && shared ) {
            SharedDataWalker sdw;
            for ( int i=0, is=totalVariables; i!=is; ++i ) {
                auto & pv = globalVariables[i];
                if ( pv.init && pv.shared ) {
                    sdw.walk(shared + pv.offset, pv.debugInfo);
                }
            }
        }
    }

    bool Context::runShutdownScript ( ) {
        DAS_ASSERTF(insideContext==0,"can't run init script on the locked context");
        if ( shutdown ) return false;
        shutdown = true;
        auto ssz = 16384 + globalInitStackSize;
        StackAllocator init_stack(ssz);
        SharedStackGuard guard(*this, init_stack);
        return runWithCatch([&](){
            vector<SimFunction *> lateShutdown;
            for ( int j=0, js=totalFunctions; j!=js && !stopFlags; ++j ) {
                auto & pf = functions[j];
                DAS_ASSERTF(pf.debugInfo, "Missing debug info for %s", pf.name);
                if ( pf.debugInfo->flags & FuncInfo::flag_shutdown ) {
                    if ( pf.debugInfo->flags & FuncInfo::flag_late_shutdown ) {
                        lateShutdown.push_back(&pf);
                    } else {
                        callOrFastcall(&pf, nullptr, 0);
                    }
                }
            }
            if ( !stopFlags && !lateShutdown.empty() ) {
                for ( auto pf : lateShutdown ) {
                    callOrFastcall(pf, nullptr, 0);
                    if ( stopFlags ) break;
                }
            }
        });
    }

    vector<SimFunction *> Context::findFunctions ( const char * fnname ) const {
        vector<SimFunction *> res;
        for ( auto & kv : *tabMnLookup ) {
            auto fn = kv.second;
            if ( fn!=nullptr && strcmp(fn->name, fnname)==0 ) {
                res.push_back(fn);
            }
        }
        return res;
    }

    SimFunction * Context::findFunction ( const char * fnname ) const {
        for ( auto & kv : *tabMnLookup ) {
            auto fn = kv.second;
            if ( fn!=nullptr && strcmp(fn->name, fnname)==0 ) {
                return fn;
            }
        }
        return nullptr;
    }

    SimFunction * Context::findFunction ( const char * fnname, bool & isUnique ) const {
        int candidates = 0;
        SimFunction * found = nullptr;
        for ( auto & kv : *tabMnLookup ) {
            auto fn = kv.second;
            if ( fn!=nullptr && strcmp(fn->name, fnname)==0 ) {
                found = fn;
                candidates++;
            }
        }
        isUnique = candidates == 1;
        return found;
    }

    int Context::findVariable ( const char * fnname ) const {
        for ( int vni=0, vnis=totalVariables; vni!=vnis; ++vni ) {
            if ( strcmp(globalVariables[vni].name, fnname)==0 ) {
                return vni;
            }
        }
        return -1;
    }

    void Context::stackWalk( const LineInfo * at, bool showArguments, bool showLocalVariables ) {
        auto str = getStackWalk(at, showArguments, showLocalVariables);
        to_out(at, str.c_str());
    }

    class StackWalkerTextWriter : public StackWalker {
    public:
        StackWalkerTextWriter ( TextWriter & tw, Context * ctx ) : ssw(tw), context(ctx) {}
        virtual bool canWalkArguments () override {
            return showArguments;
        }
        virtual bool canWalkVariables () override {
            return showLocalVariables;
        }
        virtual bool canWalkOutOfScopeVariables() override {
            return showOutOfScope;
        }
        virtual void onCallAOT ( Prologue *, const char * fileName ) override {
            ssw << fileName << ", AOT";
        }
        virtual void onCallJIT ( Prologue *, const char * fileName ) override {
            ssw << fileName << ", JIT";
        }
        virtual void onCallAt ( Prologue *, FuncInfo * info, LineInfo * at ) override {
            ssw << info->name << " from " << at->describe();
        }
        virtual void onCall ( Prologue *, FuncInfo * info ) override {
            ssw << info->name;
        }
        virtual void onAfterPrologue ( Prologue * pp, char * SP ) override {
            ssw << "(sp=" << (context->stack.top() - SP)
                << ",sptr=0x" << HEX  << intptr_t(SP) << DEC;
            if ( pp->cmres ) {
                ssw << ",cmres=0x" << HEX << intptr_t(pp->cmres) << DEC;
            }
            ssw << ")\n";
        }
        virtual void onArgument ( FuncInfo * info, int i, VarInfo * field, vec4f arg ) override {
            ssw << "\t" << info->fields[i]->name
                << ": " << debug_type(field)
                << " = \t" << debug_value(arg, field, PrintFlags::stackwalker) << "\n";
        }
        virtual void onBeforeVariables ( ) override {
            ssw << "--> local variables\n";
        }
        virtual void onVariable ( FuncInfo *, LocalVariableInfo * lv, void * addr, bool inScope ) override {
            ssw << "\t" << lv->name
                << ": " << debug_type(lv);
            string location;
            if ( !inScope ) {
            } else if ( lv->cmres ) {
                location = "CMRES";
            } else if ( lv->isRefValue( ) ) {
                location = "ref *(sp + " + to_string(lv->stackTop) + ")";
            } else {
                location = "sp + " + to_string(lv->stackTop);
            }
            if ( addr ) {
                ssw << " = \t" << debug_value(addr, lv, PrintFlags::stackwalker)
                    << " at " << location
                    << " 0x" << HEX << intptr_t(addr) << DEC
                    << "\n";
            } else {
                if ( !inScope ) {
                    ssw << "\t// variable out of scope\n";
                } else {
                    ssw << "\t// variable was optimized out\n";
                }
            }
        }
        virtual bool onAfterCall ( Prologue * ) override {
            return !stackTopOnly;
        }
        virtual void onCorruptStack (Prologue * ) override {
            ssw << "!!! stack corrupted, aborting stack walk !!!\n";
        }
    public:
        bool showArguments = true;
        bool showLocalVariables = true;
        bool showOutOfScope = true;
        bool stackTopOnly = false;
    protected:
        TextWriter & ssw;
        Context * context;
    };

    string Context::getStackWalk ( const LineInfo * at, bool showArguments, bool showLocalVariables, bool showOutOfScope, bool stackTopOnly ) {
        FPE_DISABLE;
        TextWriter ssw;
    #if DAS_ENABLE_STACK_WALK
        ssw << "\n";
        if ( at ) {
            ssw << "from " << at->describe() << "\n";
        }
        char * sp = stack.ap();
        ssw << "CALL STACK (sp=" << (stack.top() - stack.ap())
            << ",sptr=0x" << HEX  << intptr_t(sp) << DEC << "):\n";
        StackWalkerTextWriter walker ( ssw, this );
        walker.showArguments = showArguments;
        walker.showLocalVariables =  showLocalVariables;
        walker.showOutOfScope = showOutOfScope;
        walker.stackTopOnly = stackTopOnly;
        dapiStackWalk ( &walker, *this, at ? *at : LineInfo() );
        ssw << "\n";
    #else
        ssw << "\nCALL STACK TRACKING DISABLED:\n\n";
    #endif
        return ssw.str();
    }

    void tickSpecificDebugAgent ( const char * name ) {
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        auto it = g_DebugAgents.find(name);
        if ( it != g_DebugAgents.end() ) {
            it->second.debugAgent->onTick();
        }
    }

    void tickDebugAgent ( ) {
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onTick();
        });
    }

    void collectDebugAgentState ( Context & ctx, const LineInfo & at ) {
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onCollect( &ctx, at );
        });
    }

    void onBreakpointsReset ( const char * file, int breakpointsNum ) {
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onBreakpointsReset( file, breakpointsNum );
        });
    }

    class CppOnlyDebugAgent : public DebugAgent {
    public:
        virtual void onCreateContext ( Context * ctx ) override { if ( onCreateContextOp ) onCreateContextOp(ctx); }
        virtual void onDestroyContext ( Context * ctx ) override { if ( onDestroyContextOp ) onDestroyContextOp(ctx); }
        virtual bool onLog ( Context * context, const LineInfo * at, int level, const char * text ) override { return onLogOp ? onLogOp(context, at, level, text) : false; }
        virtual bool isCppOnlyAgent() const override { return true; }
    public:
        function<void(Context*)> onCreateContextOp;
        function<void(Context*)> onDestroyContextOp;
        function<bool(Context *, const LineInfo *, int, const char *)> onLogOp;
    };

    template <typename TT>
    void onCppDebugAgent ( const char * category, TT && lmb ) {
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        auto it = g_DebugAgents.find(category);
        CppOnlyDebugAgent * agent = nullptr;
        if ( it != g_DebugAgents.end() ) {
            DAS_VERIFY(it->second.debugAgent->isCppOnlyAgent());
            agent = (CppOnlyDebugAgent *) it->second.debugAgent.get();
        } else {
            auto da = make_smart<CppOnlyDebugAgent>();
            agent = (CppOnlyDebugAgent *) da.get();
            g_DebugAgents[category] = {
                nullptr,
                da
            };
        }
        lmb(agent);
    }

    void onCreateCppDebugAgent ( const char * category, function<void (Context *)> && lmb ) {
        onCppDebugAgent(category, [&](CppOnlyDebugAgent * agent){
            agent->onCreateContextOp = das::move(lmb);
        });
    }

    void onDestroyCppDebugAgent ( const char * category, function<void (Context *)> && lmb ) {
        onCppDebugAgent(category, [&](CppOnlyDebugAgent * agent){
            agent->onDestroyContextOp = das::move(lmb);
        });
    }

    void onLogCppDebugAgent ( const char * category, function<bool(Context *, const LineInfo *, int, const char *)> && lmb ) {
        onCppDebugAgent(category, [&](CppOnlyDebugAgent * agent){
            agent->onLogOp = das::move(lmb);
        });
    }

    void uninstallCppDebugAgent ( const char * category ) {
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        auto it = g_DebugAgents.find(category);
        if ( it != g_DebugAgents.end() ) {
            DebugAgentInstance inst = das::move(it->second);
            g_DebugAgents.erase(it);
            inst.debugAgent.reset();  // release agent before context
        }
    }

    void deleteDebugAgent ( const char * category, LineInfoArg * at, Context * context ) {
        if ( !category ) context->throw_error_at(at, "need to specify category");
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        auto it = g_DebugAgents.find(category);
        if ( it != g_DebugAgents.end() ) {
            DebugAgent * oldAgentPtr = it->second.debugAgent.get();
            for ( auto & ap : g_DebugAgents ) {
                ap.second.debugAgent->onUninstall(oldAgentPtr);
            }
            DebugAgentInstance inst = das::move(it->second);
            g_DebugAgents.erase(it);
            inst.debugAgent.reset();  // release agent before context
        }
    }



    // Standalone-AOT contexts are constructed as member objects (see
    // daslib/aot_standalone.das::writeStandaloneCtor), not via make_shared,
    // so their weak_from_this() is empty and shared_from_this() throws
    // bad_weak_ptr. The agent adapter only needs the context's raw pointer
    // for dispatch (passed at make_debug_agent time); debugAgentContext is
    // only consulted by getDebugAgentContext, which already returns an
    // error for null. So fall back to a null shared_ptr in that case
    // instead of aborting the process.
    static inline shared_ptr<Context> debugAgentContextOrNull ( Context * context ) {
        auto wp = context->weak_from_this();
        return wp.expired() ? nullptr : wp.lock();
    }

    void installThreadLocalDebugAgent ( DebugAgentPtr newAgent, LineInfoArg * at, Context * context ) {
        if ( *daScriptEnvironment::g_threadLocalDebugAgent && (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent ) {
            context->throw_error_at(at, "thread local debug agent already installed");
        }
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        (*daScriptEnvironment::g_threadLocalDebugAgent) = new DebugAgentInstance{
            debugAgentContextOrNull(context),
            newAgent
        };
        DebugAgent * newAgentPtr = newAgent.get();
        for_each_debug_agent([newAgentPtr](DebugAgentPtr agent){
            agent->onInstall(newAgentPtr);
        });
    }

    void installDebugAgent ( DebugAgentPtr newAgent, const char * category, LineInfoArg * at, Context * context ) {
        if ( !category ) context->throw_error_at(at, "need to specify category");
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        auto it = g_DebugAgents.find(category);
        if ( it != g_DebugAgents.end() ) {
            DebugAgent * oldAgentPtr = it->second.debugAgent.get();
            for_each_debug_agent([&](const DebugAgentPtr & pAgent){
                pAgent->onUninstall(oldAgentPtr);
            });
        }
        g_DebugAgents[category] = {
            debugAgentContextOrNull(context),
            newAgent
        };
        DebugAgent * newAgentPtr = newAgent.get();
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onInstall(newAgentPtr);
        });
    }

    Context & getDebugAgentContext ( const char * category, LineInfoArg * at, Context * context ) {
        if ( !category ) context->throw_error_at(at, "need to specify category");
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        auto it = g_DebugAgents.find(category);
        if ( it == g_DebugAgents.end() ) context->throw_error_at(at, "can't get debug agent '%s'", category);
        if ( !it->second.debugAgentContext ) context->throw_error_at(at, "debug agent '%s' is a CPP-only agent", category);
        return *it->second.debugAgentContext;
    }

    bool hasDebugAgentContext ( const char * category, LineInfoArg * at, Context * context ) {
        if ( !category ) context->throw_error_at(at, "need to specify category");
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        auto it = g_DebugAgents.find(category);
        return it != g_DebugAgents.end();
    }

    void lockDebugAgent ( const TBlock<void> & blk, Context * context, LineInfoArg * line ) {
        std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
        context->invoke(blk, nullptr, nullptr, line);
    }
}

das::Context* get_clone_context( das::Context * ctx, uint32_t category );//link time resolved dependencies

namespace das
{

    void forkDebugAgentContext ( Func exFn, Context * context, LineInfoArg * lineinfo ) {
        *g_isInDebugAgentCreation = true;
        shared_ptr<Context> forkContext;
        bool realPersistent = context->persistent;
        context->persistent = true;
        forkContext.reset(get_clone_context(context, uint32_t(ContextCategory::debug_context)));
        context->persistent = realPersistent;
        context->sharedPtrContext = true;
        *g_isInDebugAgentCreation = false;
        vec4f args[1];
        args[0] = cast<Context *>::from(context);
        SimFunction * fun = exFn.PTR;
        forkContext->callOrFastcall(fun, args, lineinfo);
    }

    bool isInDebugAgentCreation() {
        return *g_isInDebugAgentCreation;
    }

    void shutdownDebugAgent() {
        bool hasThreadLocal = *daScriptEnvironment::g_threadLocalDebugAgent && (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent;
        DebugAgent * threadLocalDebugAgent = hasThreadLocal ? (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent.get() : nullptr;
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            if ( hasThreadLocal ) {
                threadLocalDebugAgent->onUninstall(pAgent.get());
            }
            for ( auto & ap : g_DebugAgents ) {
                if ( ap.second.debugAgent ) {
                    ap.second.debugAgent->onUninstall(pAgent.get());
                }
            }
        });
        das_safe_map<string,DebugAgentInstance> agents;
        {
            std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
            swap(agents, g_DebugAgents);
            delete (*daScriptEnvironment::g_threadLocalDebugAgent);
            (*daScriptEnvironment::g_threadLocalDebugAgent) = {};
        }
        // release agents before contexts to avoid use-after-free
        // (agent objects live on the context heap)
        for ( auto & ap : agents ) {
            ap.second.debugAgent.reset();
        }
    }

    void shutdownThreadLocalDebugAgent() {
        bool hasThreadLocal = *daScriptEnvironment::g_threadLocalDebugAgent && (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent;
        if ( hasThreadLocal ) {
            DebugAgent * threadLocalDebugAgent = (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent.get();
            for_each_debug_agent([&](const DebugAgentPtr & pAgent){
                pAgent->onUninstall(threadLocalDebugAgent);
            });
            {
                std::lock_guard<std::recursive_mutex> guard(g_DebugAgentMutex);
                delete (*daScriptEnvironment::g_threadLocalDebugAgent);
                (*daScriptEnvironment::g_threadLocalDebugAgent) = {};
            }
        }
    }

    void Context::triggerHwBreakpoint ( void * addr, int index ) {
        singleStepMode = true;
        hwBpAddress = addr;
        hwBpIndex = index;
    }

    void Context::breakPoint(const LineInfo & at, const char * reason, const char * text) {
        if ( debugger ) {
            bool any = false;
            for_each_debug_agent([&](const DebugAgentPtr & pAgent){
                pAgent->onBreakpoint(this, at, reason, text);
                any = true;
            });
            if ( any ) return;
        }
        os_debug_break();
    }

    static DAS_THREAD_LOCAL(bool) g_inLogger;

    void Context::to_out ( const LineInfo * at, int level, const char * message ) {
        if (message) {
            if ( !*g_inLogger ) {
                *g_inLogger = true;
                bool any = false;
                for_each_debug_agent([&](const DebugAgentPtr & pAgent){
                    any |= pAgent->onLog(this, at, level, message);
                });
                *g_inLogger = false;
                if ( any ) return;
            }
            const char * prefix = getLogMarker(level);
            das_to_stdout_level_prefix_text(level, prefix, message);
        }
    }

    void Context::throw_error_at ( const LineInfo * at, DAS_FORMAT_STRING_PREFIX const char * message, ... ) {
        const int PRINT_BUFFER_SIZE = 8192;
        char buffer[PRINT_BUFFER_SIZE];
        va_list args;
        va_start (args, message);
        vsnprintf (buffer,PRINT_BUFFER_SIZE,message, args);
        va_end (args);
        throw_fatal_error(buffer, at ? *at : LineInfo());
    }

    void Context::throw_error_at ( const LineInfo & at, DAS_FORMAT_STRING_PREFIX const char * message, ... ) {
        const int PRINT_BUFFER_SIZE = 8192;
        char buffer[PRINT_BUFFER_SIZE];
        va_list args;
        va_start (args, message);
        vsnprintf (buffer,PRINT_BUFFER_SIZE,message, args);
        va_end (args);
        throw_fatal_error(buffer, at);
    }

    void Context::throw_out_of_memory ( bool isStringHeap, uint64_t size, const LineInfo * at ) {
        if ( isStringHeap ) {
            throw_error_at(at, "out of string heap memory, requested %llu bytes, used %llu / limit %llu", (unsigned long long) size, (unsigned long long) stringHeap->bytesAllocated(), (unsigned long long) stringHeap->getLimit());
        } else {
            throw_error_at(at, "out of heap memory, requested %llu bytes, used %llu / limit %llu", (unsigned long long) size, (unsigned long long) heap->bytesAllocated(), (unsigned long long) heap->getLimit());
        }
    }

    void Context::throw_error_ex ( DAS_FORMAT_STRING_PREFIX const char * message, ... ) {
        const int PRINT_BUFFER_SIZE = 8192;
        char buffer[PRINT_BUFFER_SIZE];
        va_list args;
        va_start (args, message);
        vsnprintf (buffer,PRINT_BUFFER_SIZE,message, args);
        va_end (args);
        throw_fatal_error(buffer, LineInfo());
    }

    void Context::throw_error ( const char * message ) {
        throw_fatal_error(message, LineInfo());
    }

    struct FileInfoCollector : SimVisitor {
        virtual void preVisit ( SimNode * node ) override {
            SimVisitor::preVisit(node);
            if ( auto fi = node->debugInfo.fileInfo ) {
                allFiles.insert(fi);
            }
        }
        das_hash_set<FileInfo *>    allFiles;
    };

    vector<FileInfo *> Context::getAllFiles() const {
        vector<FileInfo *> allFiles;
        FileInfoCollector collector;
        runVisitor(&collector);
        for ( auto & it : collector.allFiles ) {
            allFiles.push_back(it);
        }
        sort ( allFiles.begin(), allFiles.end(), [&]( FileInfo * a, FileInfo * b ){
            return a->name > b->name;
        });
        return allFiles;
    }

    void Context::resetProfiler() {
#if DAS_ENABLE_PROFILER
        auto allFiles = getAllFiles();
        for ( auto fi : allFiles ) {
            fi->profileData.clear();
        }
#endif
    }

    void Context::collectProfileInfo( TextWriter & tout ) {
#if DAS_ENABLE_PROFILER
        uint64_t totalGoo = 0;
        auto allFiles = getAllFiles();
        for ( auto info : allFiles ) {
            for ( auto counter : info->profileData ) {
                totalGoo += counter;
            }
        }
        tout << "\nPROFILING RESULTS:\n";
        for ( auto fi : allFiles ) {
            tout << fi->name << "\n";
            bool newLine = true;
            int  line = 0;
            char txt[2];
            txt[1] = 0;
            int col = 0;
            for ( uint32_t i=0, is=fi->sourceLength; i!=is; ++i ) {
                if ( newLine ) {
                    line ++;
                    col = 0;
                    newLine = false;
                    char total[20];
                    if ( fi->profileData.size()>size_t(line) && fi->profileData[line] ) {
                        uint64_t samples = fi->profileData[line];
                        auto result = fmt::format_to(total, FMT_STRING("{:6.2f}"), samples*100.0/totalGoo); *result = 0;
                        tout << total;
                    } else {
                        tout << "      ";
                    }
                }
                txt[0] = fi->source[i];
                if (txt[0] == '\n') {
                    newLine = true;
                }
                if (txt[0] == '\t') {
                    if (col % 4 == 0) {
                        tout << "    ";
                        col += 4;
                    } else {
                        while (col % 4) {
                            tout << " ";
                            col++;
                        }
                    }
                } else {
                    tout << txt;
                    col++;
                }
            }
        }

#else
        tout << "\nPROFILER IS DISABLED\n";
#endif
    }

    string getLinesAroundCode ( const char* st, int ROW, int TAB ) {
        TextWriter text;
        int col=0, row=1;
        auto it = st;
        while ( *it ) {
            auto CH = *it++;
            if ( CH=='\t' ) {
                int tcol = (col + TAB) & ~(TAB-1);
                while ( col < tcol ) {
                    if ( row>=ROW-3 && row<=ROW+3 ) text << " ";
                    col ++;
                }
                continue;
            } else if ( CH=='\n' ) {
                row++;
                col=0;
                if ( row>=ROW-3 && row<=ROW+3 ) {
                    text << ((row==ROW) ? "\n->  " : "\n    ");
                }
            } else {
                if ( row>=ROW-3 && row<=ROW+3 ) text << CH;
            }
            col ++;
        }
        return text.str();
    }

    void Context::instrumentCallback ( const LineInfo & at ) {
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onInstrument(this, at);
        });
    }

    void Context::instrumentFunctionCallbackThreadLocal ( SimFunction * sim, bool entering, uint64_t userData ) {
        if ( *daScriptEnvironment::g_threadLocalDebugAgent && (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent ) {
            (*daScriptEnvironment::g_threadLocalDebugAgent)->debugAgent->onInstrumentFunction(this, sim, entering, userData);
        }
    }

    void Context::instrumentFunctionCallback ( SimFunction * sim, bool entering, uint64_t userData ) {
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onInstrumentFunction(this, sim, entering, userData);
        });
    }

    void Context::bpcallback( const LineInfo & at ) {
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onSingleStep(this, at);
        });
    }

    void Context::runVisitor ( SimVisitor * vis ) const {
        for ( int gvi=0, gvis=totalVariables; gvi!=gvis; ++gvi ) {
            const auto & gv = globalVariables[gvi];
            if ( gv.init ) gv.init->visit(*vis);
        }
        for ( int fni=0, fnis=totalFunctions; fni!=fnis; ++fni ) {
            const auto & fn = functions[fni];
            if ( fn.code ) fn.code->visit(*vis);
        }
    }

    void Context::onAllocateString ( void * ptr, uint64_t size, bool tempString, const LineInfo & at ) {
        if ( g_envTotal == 0 ) return;
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onAllocateString(this, ptr, size, tempString, at);
        });
    }

    void Context::onFreeString ( void * ptr, bool tempString, const LineInfo & at ) {
        if ( g_envTotal == 0 ) return;
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onFreeString(this, ptr, tempString, at);
        });
    }

    void Context::onAllocate ( void * ptr, uint64_t size, const LineInfo & at ) {
        if ( g_envTotal == 0 ) return;
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onAllocate(this, ptr, size, at);
        });
    }

    void Context::onReallocate ( void * ptr, uint64_t size, void * newPtr, uint64_t newSize, const LineInfo & at ) {
        if ( g_envTotal == 0 ) return;
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onReallocate(this, ptr, size, newPtr, newSize, at);
        });
    }

    void Context::onFree ( void * ptr, const LineInfo & at ) {
        if ( g_envTotal == 0 ) return;
        for_each_debug_agent([&](const DebugAgentPtr & pAgent){
            pAgent->onFree(this, ptr, at);
        });
    }

}
