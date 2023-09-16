#include "daScript/misc/platform.h"

#include "daScript/misc/performance_time.h"
#include "daScript/simulate/aot_builtin_jobque.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"

#include "daScript/misc/job_que.h"
#include "module_builtin_rtti.h"

MAKE_TYPE_FACTORY(JobStatus, JobStatus)
MAKE_TYPE_FACTORY(Channel, Channel)
MAKE_TYPE_FACTORY(LockBox, LockBox)

MAKE_TYPE_FACTORY(Atomic32, AtomicTT<int32_t>)
MAKE_TYPE_FACTORY(Atomic64, AtomicTT<int64_t>)

namespace das {

    LockBox::~LockBox() {
        lock_guard<mutex> guard(mCompleteMutex);
        box.clear();
    }

    void LockBox::set ( void * data, TypeInfo * ti, Context * context ) {
        lock_guard<mutex> guard(mCompleteMutex);
        box = Feature(data,ti,context);
        mCond.notify_all();
    }

    void LockBox::get ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        lock_guard<mutex> guard(mCompleteMutex);
        das_invoke<void>::invoke<void *>(context, at, blk, box.data);
    }

    void LockBox::update ( const TBlock<void *,void *> & blk, TypeInfo * ti, Context * context, LineInfoArg * at ) {
        lock_guard<mutex> guard(mCompleteMutex);
        void * oldData = box.data;
        box.data = das_invoke<void *>::invoke<void *>(context, at, blk, box.data);
        if ( oldData != box.data ) {
            box.from = context->shared_from_this();
            box.type = ti;
        }
    }

    Channel::~Channel() {
        lock_guard<mutex> guard(mCompleteMutex);
        pipe = {};
        tail.clear();
        DAS_ASSERT(mRef==0);
    }

    void Channel::push ( void * data, TypeInfo * ti, Context * context ) {
        lock_guard<mutex> guard(mCompleteMutex);
        pipe.emplace_back(data, ti, context!=owner ? context : nullptr);
        mCond.notify_all();  // notify_one??
    }

    void Channel::pop ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        while ( true ) {
            unique_lock<mutex> uguard(mCompleteMutex);
            if ( !mCond.wait_for(uguard, chrono::milliseconds(mSleepMs), [&]() {
                bool continue_waiting = (mRemaining>0) && pipe.empty();
                return !continue_waiting;
            }) ) {
                this_thread::yield();
            } else {
                break;
            }
        }
        lock_guard<mutex> guard(mCompleteMutex);
        if ( pipe.empty() ) {
            tail.clear();
        } else {
            tail = das::move(pipe.front());
            pipe.pop_front();
        }
        das_invoke<void>::invoke<void *>(context, at, blk, tail.data);
    }

    bool Channel::isEmpty() const {
        lock_guard<mutex> guard(mCompleteMutex);
        return pipe.empty();
    }

    int32_t JobStatus::size() const {
        lock_guard<mutex> guard(mCompleteMutex);
        return (int32_t) mRemaining;
    }

    int32_t Channel::total() const {
        lock_guard<mutex> guard(mCompleteMutex);
        return (int32_t) pipe.size();
    }

    int JobStatus::append(int size) {
        lock_guard<mutex> guard(mCompleteMutex);
        mRemaining += size;
        return mRemaining;
    }

    void channelGather ( Channel * ch, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelGather: channel is null");
        ch->gather([&](void * data, TypeInfo *, Context *) {
            das_invoke<void>::invoke<void *>(context, at, blk, data);
        });
    }

    void channelGatherEx ( Channel * ch, const TBlock<void,void *,const TypeInfo *, Context &> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelGather: channel is null");
        ch->gatherEx(context, [&](void * data, TypeInfo * tinfo, Context * ctx) {
            das_invoke<void>::invoke<void *,const TypeInfo *,Context &>(context, at, blk, data, tinfo, *ctx);
        });
    }

    void channelGatherAndForward ( Channel * ch, Channel * toCh, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelGather: channel is null");
        ch->gather_and_forward(toCh, [&](void * data, TypeInfo *, Context *) {
            das_invoke<void>::invoke<void *>(context, at, blk, data);
        });
    }

    void channelPeek ( Channel * ch, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelPeek: channel is null");
        ch->for_each_item([&](void * data, TypeInfo *, Context *) {
            das_invoke<void>::invoke<void *>(context, at, blk, data);
        });
    }

    void channelVerify ( Channel * ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelVerify: channel is null");
        ch->for_each_item([&](void * data, TypeInfo * ti, Context * ctx) {
            Context * vctx = ctx ? ctx : ch->getOwner();
            auto size = ti->firstType->size;
            size = (size + 15) & ~15;
            printf("verify %p of size %i (%i)\n", data, int(ti->firstType->size), int(size));
            if ( !vctx->heap->isValidPtr((char *) data, size) ) {
                os_debug_break();
                context->throw_error_at(at, "channelVerify: channel contains non-owned pointer");
            }
        });
    }

    vec4f channelPush ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto ch = cast<Channel *>::to(args[0]);
        if ( !ch ) context.throw_error_at(call->debugInfo, "channelPush: channel is null");
        void * data = cast<void *>::to(args[1]);
        TypeInfo * ti = call->types[1];
        ch->push(data, ti, &context);
        return v_zero();
    }

    void channelPop ( Channel * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelPop: channel is null");
        ch->pop(blk,context,at);
    }

    int jobAppend ( JobStatus * ch, int size, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "jobAppend: job is null");
        return ch->append(size);
    }

    void withChannel ( const TBlock<void,Channel *> & blk, Context * context, LineInfoArg * at ) {
        Channel ch(context);
        ch.addRef();
        das_invoke<void>::invoke<Channel *>(context, at, blk, &ch);
        if ( ch.releaseRef() ) {
            context->throw_error_at(at, "channel beeing deleted while being used");
        }
    }

    void withChannelEx ( int32_t count, const TBlock<void,Channel *> & blk, Context * context, LineInfoArg * at ) {
        Channel ch(context,count);
        ch.addRef();
        das_invoke<void>::invoke<Channel *>(context, at, blk, &ch);
        if ( ch.releaseRef() ) {
            context->throw_error_at(at, "channel beeing deleted while being used");
        }
    }

    Channel * channelCreate( Context * context, LineInfoArg * ) {
        Channel * ch = new Channel(context);
        ch->addRef();
        return ch;
    }

    void channelRemove( Channel * & ch, Context * context, LineInfoArg * at ) {
        if (!ch->isValid()) context->throw_error_at(at, "channel is invalid (already deleted?)");
        if (ch->releaseRef()) context->throw_error_at(at, "channel beeing deleted while being used");
        delete ch;
        ch = nullptr;
    }

    struct ChannelAnnotation : ManagedStructureAnnotation<Channel,false> {
        ChannelAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Channel", ml) {
            addProperty<DAS_BIND_MANAGED_PROP(isEmpty)>("isEmpty");
            addProperty<DAS_BIND_MANAGED_PROP(isReady)>("isReady");
            addProperty<DAS_BIND_MANAGED_PROP(size)>("size");
            addProperty<DAS_BIND_MANAGED_PROP(total)>("total");
        }
        virtual int32_t getGcFlags(das_set<Structure *> &, das_set<Annotation *> &) const override {
            return TypeDecl::gcFlag_heap | TypeDecl::gcFlag_stringHeap;
        }
        virtual void walk(DataWalker & walker, void * data) override {
            BasicStructureAnnotation::walk(walker, data);
            Channel * ch = (Channel *) data;
            if ( !ch->isValid() ) {
                walker.invalidData();
                return;
            }
            ch->for_each_item([&](void * data, TypeInfo * ti, Context *) {
                walker.walk((char *)&data, ti);
            });
        }
    };

    struct JobStatusAnnotation : ManagedStructureAnnotation<JobStatus,false> {
        JobStatusAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("JobStatus", ml) {
            addProperty<DAS_BIND_MANAGED_PROP(isReady)>("isReady");
            addProperty<DAS_BIND_MANAGED_PROP(isValid)>("isValid");
        }
    };

    mutex              g_jobQueMutex;
    shared_ptr<JobQue> g_jobQue;

    LockBox * lockBoxCreate( Context *, LineInfoArg * ) {
        LockBox * ch = new LockBox();
        ch->addRef();
        return ch;
    }

    void lockBoxRemove( LockBox * & ch, Context * context, LineInfoArg * at ) {
        if (!ch->isValid()) context->throw_error_at(at, "lock box is invalid (already deleted?)");
        if (ch->releaseRef()) context->throw_error_at(at, "lock box beeing deleted while being used");
        delete ch;
        ch = nullptr;
    }

    void withLockBox ( const TBlock<void,LockBox *> & blk, Context * context, LineInfoArg * at ) {
        LockBox ch;
        ch.addRef();
        das_invoke<void>::invoke<LockBox *>(context, at, blk, &ch);
        if ( ch.releaseRef() ) {
            context->throw_error_at(at, "lock box beeing deleted while being used");
        }
    }

    vec4f lockBoxSet ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto ch = cast<LockBox *>::to(args[0]);
        if ( !ch ) context.throw_error_at(call->debugInfo, "lockBoxSet: box is null");
        void * data = cast<void *>::to(args[1]);
        TypeInfo * ti = call->types[1];
        ch->set(data, ti, &context);
        return v_zero();
    }

    void lockBoxUpdate ( LockBox * ch, TypeInfo * ti, const TBlock<void *,void*> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "lockBoxUpdate: box is null");
        ch->update(blk,ti,context,at);
    }

    void lockBoxGet ( LockBox * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "lockBoxGet: box is null");
        ch->get(blk,context,at);
    }

    struct LockBoxAnnotation : ManagedStructureAnnotation<LockBox,false> {
        LockBoxAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LockBox", ml) {
        }
        virtual int32_t getGcFlags(das_set<Structure *> &, das_set<Annotation *> &) const override {
            return TypeDecl::gcFlag_heap | TypeDecl::gcFlag_stringHeap;
        }
        virtual void walk(DataWalker & walker, void * data) override {
            BasicStructureAnnotation::walk(walker, data);
            LockBox * ch = (LockBox *) data;
            if ( !ch->isValid() ) {
                walker.invalidData();
                return;
            }
            ch->peek([&](void * data, TypeInfo * ti, Context *) {
                walker.walk((char *)&data, ti);
            });
        }
    };


    template <typename TT>
    struct AtomicAnnotation : ManagedStructureAnnotation<AtomicTT<TT>,false> {
        AtomicAnnotation(const char * ttname, ModuleLibrary & ml) : ManagedStructureAnnotation<AtomicTT<TT>,false> (ttname, ml) {
        }
        virtual void walk(DataWalker & walker, void * data) override {
            BasicStructureAnnotation::walk(walker, data);
            auto ch = (AtomicTT<TT> *) data;
            if ( !ch->isValid() ) {
                walker.invalidData();
                return;
            }
            TypeInfo info;
            memset(&info, 0, sizeof(TypeInfo));
            info.type = sizeof(TT)==4 ? Type::tInt : Type::tInt64;
            vec4f vd = cast<TT>::from(ch->get());
            walker.walk(vd, &info);
        }
    };
}

das::Context* get_clone_context( das::Context * ctx, uint32_t category );//link time resolved dependencies

namespace das {

    void new_job_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo ) {
        if ( !g_jobQue ) context->throw_error_at(lineinfo, "need to be in 'with_job_que' block");
        shared_ptr<Context> forkContext;
        forkContext.reset(get_clone_context(context, uint32_t(ContextCategory::job_clone)));
        auto ptr = forkContext->heap->allocate(lambdaSize + 16);
        forkContext->heap->mark_comment(ptr, "new [[ ]] in new_job");
        memset ( ptr, 0, lambdaSize + 16 );
        ptr += 16;
        das_invoke_function<void>::invoke(forkContext.get(), lineinfo, fn, ptr, lambda.capture);
        das_delete<Lambda>::clear(context, lambda);
        auto bound = daScriptEnvironment::bound;
        g_jobQue->push([=]() mutable {
            daScriptEnvironment::bound = bound;
            Lambda flambda(ptr);
            das_invoke_lambda<void>::invoke(forkContext.get(), lineinfo, flambda);
            das_delete<Lambda>::clear(forkContext.get(), flambda);
        }, 0, JobPriority::Default);
    }

    static atomic<int32_t> g_jobQueAvailable{0};
    static atomic<int32_t> g_jobQueTotalThreads{0};

    bool is_job_que_shutting_down () {
        return g_jobQueAvailable == 0;
    }

    void new_thread_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo ) {
        shared_ptr<Context> forkContext;
        forkContext.reset(get_clone_context(context, uint32_t(ContextCategory::thread_clone)));
        auto ptr = forkContext->heap->allocate(lambdaSize + 16);
        forkContext->heap->mark_comment(ptr, "new [[ ]] in new_thread");
        memset ( ptr, 0, lambdaSize + 16 );
        ptr += 16;
        das_invoke_function<void>::invoke(forkContext.get(), lineinfo, fn, ptr, lambda.capture);
        das_delete<Lambda>::clear(context, lambda);
        g_jobQueTotalThreads ++;
        auto bound = daScriptEnvironment::bound;
        thread([=]() mutable {
            daScriptEnvironment::bound = bound;
            Lambda flambda(ptr);
            das_invoke_lambda<void>::invoke(forkContext.get(), lineinfo, flambda);
            das_delete<Lambda>::clear(forkContext.get(), flambda);
            g_jobQueTotalThreads --;
        }).detach();
    }

    void withJobQue ( const TBlock<void> & block, Context * context, LineInfoArg * lineInfo ) {
        if ( !g_jobQue ) {
            lock_guard<mutex> guard(g_jobQueMutex);
            g_jobQue = make_shared<JobQue>();
        }
        {
            shared_ptr<JobQue> jq = g_jobQue;
            context->invoke(block, nullptr, nullptr, lineInfo);
        }
        {
            lock_guard<mutex> guard(g_jobQueMutex);
            if ( g_jobQue.use_count()==1 ) g_jobQue.reset();
        }
    }

    void jobStatusAddRef ( JobStatus * status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "jobStatusAddRef: status is null");
        status->addRef();
    }

    void jobStatusReleaseRef ( JobStatus * & status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "jobStatusReleaseRef: status is null");
        status->releaseRef();
        status = nullptr;
    }

    void withJobStatus ( int32_t total, const TBlock<void,JobStatus *> & block, Context * context, LineInfoArg * lineInfo ) {
        JobStatus status(total);
        status.addRef();
        vec4f args[1];
        args[0] = cast<JobStatus *>::from(&status);
        context->invoke(block,args,nullptr,lineInfo);
        if ( int ref = status.releaseRef() ) {
            context->throw_error_at(lineInfo, "job status beeing deleted while being used (ref %i)", ref);
        }
    }

    JobStatus * jobStatusCreate( Context *, LineInfoArg * ) {
        JobStatus * ch = new JobStatus();
        ch->addRef();
        return ch;
    }

    void jobStatusRemove( JobStatus * & ch, Context * context, LineInfoArg * at ) {
        if (!ch->isValid()) context->throw_error_at(at, "job status is invalid (already deleted?)");
        if (ch->releaseRef()) context->throw_error_at(at, "job status beeing deleted while being used");
        delete ch;
        ch = nullptr;
    }

    void waitForJob ( JobStatus * status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "waitForJob: status is null");
        status->Wait();
    }

    void notifyJob ( JobStatus * status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "notifyJob: status is null");
        status->Notify();
    }

    void notifyAndReleaseJob ( JobStatus * & status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "notifyAndReleaseJob: status is null");
        status->NotifyAndRelease();
        status = nullptr;
    }

    int getTotalHwJobs( Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in 'with_job_que' block");
        return g_jobQue->getTotalHwJobs();
    }

    int getTotalHwThreads () {
        return thread::hardware_concurrency();
    }

    class Module_JobQue : public Module {
    public:
        Module_JobQue() : Module("jobque") {
            DAS_PROFILE_SECTION("Module_JobQue");
            g_jobQueAvailable++;
            // libs
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            // types
            addAnnotation(make_smart<JobStatusAnnotation>(lib));
            auto cha = make_smart<ChannelAnnotation>(lib);
            cha->from("JobStatus");
            addAnnotation(cha);
            auto lbx = make_smart<LockBoxAnnotation>(lib);
            lbx->from("JobStatus");
            addAnnotation(lbx);
            auto a32 = make_smart<AtomicAnnotation<int32_t>>("Atomic32",lib);
            a32->from("JobStatus");
            addAnnotation(a32);
            auto a64 = make_smart<AtomicAnnotation<int64_t>>("Atomic64",lib);
            a64->from("JobStatus");
            addAnnotation(a64);
            // atomic 32
            addExtern<DAS_BIND_FUN(atomicCreate<int32_t>)>(*this, lib, "atomic32_create",
                SideEffects::modifyExternal, "atomicCreate<int32_t>")
                    ->args({ "context","line" });
            addExtern<DAS_BIND_FUN(atomicRemove<int32_t>)>(*this, lib, "atomic32_remove",
                SideEffects::modifyArgumentAndExternal, "atomicRemove<int32_t>")
                    ->args({ "atomic", "context","line" })->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(withAtomic<int32_t>)>(*this, lib,  "with_atomic32",
                SideEffects::invoke, "withAtomic<int32_t>")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(atomicSet<int32_t>)>(*this, lib, "set",
                SideEffects::modifyArgumentAndExternal, "atomicSet<int32_t>")
                    ->args({ "atomic", "value", "context","line" });
            addExtern<DAS_BIND_FUN(atomicGet<int32_t>)>(*this, lib, "get",
                SideEffects::modifyArgumentAndExternal, "atomicGet<int32_t>")
                    ->args({ "atomic", "context","line" });
            addExtern<DAS_BIND_FUN(atomicInc<int32_t>)>(*this, lib, "inc",
                SideEffects::modifyArgumentAndExternal, "atomicInc<int32_t>")
                    ->args({ "atomic", "context","line" });
            addExtern<DAS_BIND_FUN(atomicDec<int32_t>)>(*this, lib, "dec",
                SideEffects::modifyArgumentAndExternal, "atomicDec<int32_t>")
                    ->args({ "atomic", "context","line" });
            // atomic 64
            addExtern<DAS_BIND_FUN(atomicCreate<int64_t>)>(*this, lib, "atomic64_create",
                SideEffects::modifyExternal, "atomicCreate<int64_t>")
                    ->args({ "context","line" });
            addExtern<DAS_BIND_FUN(atomicRemove<int64_t>)>(*this, lib, "atomic64_remove",
                SideEffects::modifyArgumentAndExternal, "atomicRemove<int64_t>")
                    ->args({ "atomic", "context","line" })->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(withAtomic<int64_t>)>(*this, lib,  "with_atomic64",
                SideEffects::invoke, "withAtomic<int64_t>")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(atomicSet<int64_t>)>(*this, lib, "set",
                SideEffects::modifyArgumentAndExternal, "atomicSet<int64_t>")
                    ->args({ "atomic", "value", "context","line" });
            addExtern<DAS_BIND_FUN(atomicGet<int64_t>)>(*this, lib, "get",
                SideEffects::modifyArgumentAndExternal, "atomicGet<int64_t>")
                    ->args({ "atomic", "context","line" });
            addExtern<DAS_BIND_FUN(atomicInc<int64_t>)>(*this, lib, "inc",
                SideEffects::modifyArgumentAndExternal, "atomicInc<int64_t>")
                    ->args({ "atomic", "context","line" });
            addExtern<DAS_BIND_FUN(atomicDec<int64_t>)>(*this, lib, "dec",
                SideEffects::modifyArgumentAndExternal, "atomicDec<int64_t>")
                    ->args({ "atomic", "context","line" });
            // lock box
            addExtern<DAS_BIND_FUN(lockBoxCreate)>(*this, lib, "lock_box_create",
                SideEffects::invoke, "lockBoxCreate")
                    ->args({ "context","line" });
            addExtern<DAS_BIND_FUN(lockBoxRemove)>(*this, lib, "lock_box_remove",
                SideEffects::invoke, "lockBoxRemove")
                    ->args({ "box", "context","line" })->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(withLockBox)>(*this, lib,  "with_lock_box",
                SideEffects::invoke, "withLockBox")
                    ->args({"block","context","line"});
            addInterop<lockBoxSet,void,LockBox *,vec4f>(*this, lib,  "_builtin_lockbox_set",
                SideEffects::modifyArgumentAndExternal, "lockBoxSet")
                    ->args({"box","data"});
            addExtern<DAS_BIND_FUN(lockBoxGet)>(*this, lib,  "_builtin_lockbox_get",
                SideEffects::modifyArgumentAndExternal, "lockBoxGet")
                    ->args({"box","block","context","line"});
            addExtern<DAS_BIND_FUN(lockBoxUpdate)>(*this, lib,  "_builtin_lockbox_update",
                SideEffects::modifyArgumentAndExternal, "lockBoxUpdate")
                    ->args({"box","type_info","block","context","line"});
            // channel
            addInterop<channelPush,void,Channel *,vec4f>(*this, lib,  "_builtin_channel_push",
                SideEffects::modifyArgumentAndExternal, "channelPush")
                    ->args({"channel","data"});
            addExtern<DAS_BIND_FUN(channelPop)>(*this, lib,  "_builtin_channel_pop",
                SideEffects::modifyArgumentAndExternal, "channelPop")
                    ->args({"channel","block","context","line"});
            addExtern<DAS_BIND_FUN(channelGather)>(*this, lib,  "_builtin_channel_gather",
                SideEffects::modifyArgumentAndExternal, "channelGather")
                    ->args({"channel","block","context","line"});
            addExtern<DAS_BIND_FUN(channelGatherEx)>(*this, lib,  "_builtin_channel_gather_ex",
                SideEffects::modifyArgumentAndExternal, "channelGatherEx")
                    ->args({"channel","block","context","line"});
            addExtern<DAS_BIND_FUN(channelGatherAndForward)>(*this, lib,  "_builtin_channel_gather_and_forward",
                SideEffects::modifyArgumentAndExternal, "channelGatherAndForward")
                    ->args({"channel","toChannel","block","context","line"});
            addExtern<DAS_BIND_FUN(channelPeek)>(*this, lib,  "_builtin_channel_peek",
                SideEffects::modifyArgumentAndExternal, "channelPeek")
                    ->args({"channel","block","context","line"});
            addExtern<DAS_BIND_FUN(channelVerify)>(*this, lib,  "_builtin_channel_verify",
                SideEffects::modifyArgumentAndExternal, "channelVerify")
                    ->args({"channel","context","line"});
            addExtern<DAS_BIND_FUN(jobAppend)>(*this, lib, "append",
                SideEffects::modifyArgument, "jobAppend")
                    ->args({"channel","size","context","line"});
            addExtern<DAS_BIND_FUN(withChannel)>(*this, lib,  "with_channel",
                SideEffects::invoke, "withChannel")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(withChannelEx)>(*this, lib,  "with_channel",
                SideEffects::invoke, "withChannelEx")
                    ->args({"count","block","context","line"});
            addExtern<DAS_BIND_FUN(channelCreate)>(*this, lib, "channel_create",
                SideEffects::invoke, "channelCreate")
                    ->args({ "context","line" })->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(channelRemove)>(*this, lib, "channel_remove",
                SideEffects::invoke, "channelRemove")
                    ->args({ "channel", "context","line" })->unsafeOperation = true;
            // job status
            addExtern<DAS_BIND_FUN(withJobStatus)>(*this, lib,  "with_job_status",
                SideEffects::modifyExternal, "withJobStatus")
                    ->args({"total","block","context","line"});
            addExtern<DAS_BIND_FUN(jobStatusAddRef)>(*this, lib, "add_ref",
                SideEffects::modifyArgumentAndAccessExternal, "jobStatusAddRef")
                    ->args({"status","context","line"});
            addExtern<DAS_BIND_FUN(jobStatusReleaseRef)>(*this, lib, "release",
                SideEffects::modifyArgumentAndAccessExternal, "jobStatusReleaseRef")
                    ->args({"status","context","line"});
            addExtern<DAS_BIND_FUN(waitForJob)>(*this, lib,  "join",
                SideEffects::modifyExternal, "waitForJob")
                    ->args({"job","context","line"});
            addExtern<DAS_BIND_FUN(notifyJob)>(*this, lib,  "notify",
                SideEffects::modifyExternal, "notifyJob")
                    ->args({"job","context","line"});
            addExtern<DAS_BIND_FUN(notifyAndReleaseJob)>(*this, lib,  "notify_and_release",
                SideEffects::modifyExternal, "notifyAndReleaseJob")
                    ->args({"job","context","line"});
            addExtern<DAS_BIND_FUN(jobStatusCreate)>(*this, lib, "job_status_create",
                SideEffects::invoke, "jobStatusCreate")
                    ->args({ "context","line" });
            addExtern<DAS_BIND_FUN(jobStatusRemove)>(*this, lib, "job_status_remove",
                SideEffects::invoke, "jobStatusRemove")
                    ->args({ "jobStatus", "context","line" })->unsafeOperation = true;
            // fork \ invoke \ etc
            addExtern<DAS_BIND_FUN(new_job_invoke)>(*this, lib,  "new_job_invoke",
                SideEffects::modifyExternal, "new_job_invoke")
                    ->args({"lambda","function","lambdaSize","context","line"});
            addExtern<DAS_BIND_FUN(withJobQue)>(*this, lib,  "with_job_que",
                SideEffects::modifyExternal, "withJobQue")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(getTotalHwJobs)>(*this, lib,  "get_total_hw_jobs",
                SideEffects::accessExternal, "getTotalHwJobs")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(getTotalHwThreads)>(*this, lib,  "get_total_hw_threads",
                SideEffects::accessExternal, "getTotalHwThreads");
            addExtern<DAS_BIND_FUN(new_thread_invoke)>(*this, lib,  "new_thread_invoke",
                SideEffects::modifyExternal, "new_thread_invoke")
                    ->args({"lambda","function","lambdaSize","context","line"});
            addExtern<DAS_BIND_FUN(is_job_que_shutting_down)>(*this, lib,  "is_job_que_shutting_down",
                SideEffects::modifyExternal, "is_job_que_shutting_down");
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_jobque.h\"\n";
            return ModuleAotType::cpp;
        }
        virtual ~Module_JobQue() {
            g_jobQueAvailable--;
            if ( g_jobQueAvailable == 0 ) {
                while ( g_jobQueTotalThreads ) {
                    builtin_sleep(0);
                }
                lock_guard<mutex> guard(g_jobQueMutex);
                g_jobQue.reset();
            }
        }
    protected:

        // bool needShutdown = false;
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_JobQue,das);

