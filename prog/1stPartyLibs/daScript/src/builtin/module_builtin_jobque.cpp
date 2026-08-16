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
MAKE_TYPE_FACTORY(Stream, Stream)
MAKE_TYPE_FACTORY(SeqBox, SeqBox)

MAKE_TYPE_FACTORY(Atomic32, AtomicTT<int32_t>)
MAKE_TYPE_FACTORY(Atomic64, AtomicTT<int64_t>)

namespace das {

    template <typename TT>
    struct AddReleaseGuard {
        AddReleaseGuard ( TT * tt, Context * c, LineInfoArg * a ) : t(tt), at(a), context(c) {
            t->addRef(a);
        }
        ~AddReleaseGuard () {
            if ( int ref = t->releaseRef(at) ) {
                context->throw_error_at(at, "synch primitive deleted while being used (ref=%i)", ref);
            }
        }
        TT * t;
        LineInfoArg * at;
        Context * context;
    };

    LockBox::~LockBox() {
        lock_guard<mutex> guard(mCompleteMutex);
        box.clear();
    }

    void LockBox::set ( void * data, TypeInfo * ti, Context * context ) {
        lock_guard<mutex> guard(mCompleteMutex);
        box = Feature(data,ti,context);
        box.fOwner = this;
        box.fOwnerTrackId = mTrackId;
        mCond.notify_all();
    }

    void LockBox::get ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        lock_guard<mutex> guard(mCompleteMutex);
        das_invoke<void>::invoke<void *>(context, at, blk, box.data);
    }

    void LockBox::fill ( void * data, TypeInfo * ti, Context * context ) {
        lock_guard<mutex> guard(mCompleteMutex);
        box = Feature(data,ti,context);
        box.fOwner = this;
        box.fOwnerTrackId = mTrackId;
        mRemaining = 1;
        mCond.notify_all();
    }

    void LockBox::grab ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        lock_guard<mutex> guard(mCompleteMutex);
        das_invoke<void>::invoke<void *>(context, at, blk, box.data);
        box.clear();
        mRemaining = 0;
        mCond.notify_all();
    }

    void LockBox::update ( const TBlock<void *,void *> & blk, TypeInfo * ti, Context * context, LineInfoArg * at ) {
        lock_guard<mutex> guard(mCompleteMutex);
        void * oldData = box.data;
        box.data = das_invoke<void *>::invoke<void *>(context, at, blk, box.data);
        if ( oldData != box.data ) {
            box.setFrom(context);
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
        pipe.back().fOwner = this;
        pipe.back().fOwnerTrackId = mTrackId;
        mCond.notify_all();  // notify_one??
    }

    void Channel::pushBatch ( void ** data, int count, TypeInfo * ti, Context * context ) {
        lock_guard<mutex> guard(mCompleteMutex);
        auto pushCtx = context!=owner ? context : nullptr;
        for ( int i=0; i!=count; ++i ) {
            pipe.emplace_back(data[i], ti, pushCtx);
            pipe.back().fOwner = this;
            pipe.back().fOwnerTrackId = mTrackId;
        }
        mCond.notify_all();  // notify_one??
    }

    void Channel::pop ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        while ( true ) {
            unique_lock<mutex> uguard(mCompleteMutex);
            if ( !mCond.wait_for(uguard, std::chrono::milliseconds(mSleepMs), [&]() {
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

    bool Channel::tryPop ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        lock_guard<mutex> guard(mCompleteMutex);
        if ( pipe.empty() ) {
            return false;
        }
        tail = das::move(pipe.front());
        pipe.pop_front();
        das_invoke<void>::invoke<void *>(context, at, blk, tail.data);
        return true;
    }

    bool Channel::popWithTimeout ( int timeoutMs, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at ) {
        unique_lock<mutex> uguard(mCompleteMutex);
        if ( !mCond.wait_for(uguard, std::chrono::milliseconds(timeoutMs), [&]() {
            return !pipe.empty() || mRemaining <= 0;
        }) ) {
            return false;
        }
        if ( pipe.empty() ) {
            return false;
        }
        tail = das::move(pipe.front());
        pipe.pop_front();
        das_invoke<void>::invoke<void *>(context, at, blk, tail.data);
        return true;
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

    vec4f channelPushBatch ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto ch = cast<Channel *>::to(args[0]);
        if ( !ch ) context.throw_error_at(call->debugInfo, "channelPushBatch: channel is null");
        TypeInfo * ti = call->types[1];
        if (ti->type != Type::tArray){
            context.throw_error_at(call->debugInfo, "channelPushBatch: type must be array");
        }
        if (!ti->firstType || ti->firstType->type != Type::tPointer){
            context.throw_error_at(call->debugInfo, "channelPushBatch: array must be of pointer type");
        }
        Array * data = cast<Array *>::to(args[1]);
        ch->pushBatch((void**)data->data, data->size, ti->firstType, &context);
        return v_zero();
    }

    void channelPop ( Channel * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelPop: channel is null");
        ch->pop(blk,context,at);
    }

    bool channelTryPop ( Channel * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelTryPop: channel is null");
        return ch->tryPop(blk,context,at);
    }

    bool channelPopWithTimeout ( Channel * ch, int32_t timeoutMs, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelPopWithTimeout: channel is null");
        return ch->popWithTimeout(timeoutMs,blk,context,at);
    }

    int jobAppend ( JobStatus * ch, int size, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "jobAppend: job is null");
        return ch->append(size);
    }
    void withChannel ( const TBlock<void,Channel *> & blk, Context * context, LineInfoArg * at ) {
        Channel ch(context);
        AddReleaseGuard<Channel> guard(&ch, context, at);
        das_invoke<void>::invoke<Channel *>(context, at, blk, &ch);
    }

    void withChannelEx ( int32_t count, const TBlock<void,Channel *> & blk, Context * context, LineInfoArg * at ) {
        Channel ch(context,count);
        AddReleaseGuard<Channel> guard(&ch, context, at);
        das_invoke<void>::invoke<Channel *>(context, at, blk, &ch);
    }

    Channel * channelCreate( Context * context, LineInfoArg * at ) {
        Channel * ch = new Channel(context);
        if ( at ) ch->mCreatedAt = at->describe();
        ch->addRef(at);
        return ch;
    }

    void channelRemove( Channel * & ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "channelRemove: channel is null");
        if (!ch->isValid()) context->throw_error_at(at, "channel is invalid (already deleted?)");
        if (ch->releaseRef(at)) context->throw_error_at(at, "channel being deleted while being used");
        delete ch;
        ch = nullptr;
    }

    Stream::~Stream() {
        lock_guard<mutex> guard(mCompleteMutex);
        pipe.clear();
        DAS_ASSERT(mRef==0);
    }

    void Stream::push ( const uint8_t * data, uint64_t size ) {
        lock_guard<mutex> guard(mCompleteMutex);
        pipe.emplace_back();
        auto & v = pipe.back();
        v.resize(size);
        if ( size ) memcpy(v.data(), data, size);
        mCond.notify_all();
    }

    void Stream::pushBatch ( const uint8_t * const * data, const uint64_t * sizes, int64_t count ) {
        lock_guard<mutex> guard(mCompleteMutex);
        for ( int64_t i=0; i<count; ++i ) {
            pipe.emplace_back();
            auto & v = pipe.back();
            v.resize(sizes[i]);
            if ( sizes[i] ) memcpy(v.data(), data[i], sizes[i]);
        }
        mCond.notify_all();
    }

    static void invoke_stream_block ( const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk,
                                      const uint8_t * data, uint64_t size,
                                      Context * context, LineInfoArg * at ) {
        Array arr;
        array_mark_locked(arr, (void *)data, size);
        vec4f args[1];
        args[0] = cast<Array *>::from(&arr);
        context->invoke(blk, args, nullptr, at);
    }

    void Stream::pop ( const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at ) {
        while ( true ) {
            unique_lock<mutex> uguard(mCompleteMutex);
            if ( !mCond.wait_for(uguard, std::chrono::milliseconds(mSleepMs), [&]() {
                bool continue_waiting = (mRemaining>0) && pipe.empty();
                return !continue_waiting;
            }) ) {
                this_thread::yield();
            } else {
                break;
            }
        }
        vector<uint8_t> item;
        {
            lock_guard<mutex> guard(mCompleteMutex);
            if ( !pipe.empty() ) {
                item = das::move(pipe.front());
                pipe.pop_front();
            }
        }
        invoke_stream_block(blk, item.data(), item.size(), context, at);
    }

    bool Stream::tryPop ( const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at ) {
        vector<uint8_t> item;
        {
            lock_guard<mutex> guard(mCompleteMutex);
            if ( pipe.empty() ) return false;
            item = das::move(pipe.front());
            pipe.pop_front();
        }
        invoke_stream_block(blk, item.data(), item.size(), context, at);
        return true;
    }

    bool Stream::popWithTimeout ( int timeoutMs, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at ) {
        vector<uint8_t> item;
        {
            unique_lock<mutex> uguard(mCompleteMutex);
            if ( !mCond.wait_for(uguard, std::chrono::milliseconds(timeoutMs), [&]() {
                return !pipe.empty() || mRemaining <= 0;
            }) ) {
                return false;
            }
            if ( pipe.empty() ) return false;
            item = das::move(pipe.front());
            pipe.pop_front();
        }
        invoke_stream_block(blk, item.data(), item.size(), context, at);
        return true;
    }

    bool Stream::isEmpty() const {
        lock_guard<mutex> guard(mCompleteMutex);
        return pipe.empty();
    }

    int Stream::total() const {
        lock_guard<mutex> guard(mCompleteMutex);
        return (int) pipe.size();
    }

    Stream * streamCreate ( Context *, LineInfoArg * at ) {
        Stream * ch = new Stream();
        if ( at ) ch->mCreatedAt = at->describe();
        ch->addRef(at);
        return ch;
    }

    void streamRemove ( Stream * & ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "streamRemove: stream is null");
        if (!ch->isValid()) context->throw_error_at(at, "stream is invalid (already deleted?)");
        if (ch->releaseRef(at)) context->throw_error_at(at, "stream being deleted while being used");
        delete ch;
        ch = nullptr;
    }

    void withStream ( const TBlock<void, Stream *> & blk, Context * context, LineInfoArg * at ) {
        Stream ch;
        AddReleaseGuard<Stream> guard(&ch, context, at);
        das_invoke<void>::invoke<Stream *>(context, at, blk, &ch);
    }

    void withStreamEx ( int32_t count, const TBlock<void, Stream *> & blk, Context * context, LineInfoArg * at ) {
        Stream ch(count);
        AddReleaseGuard<Stream> guard(&ch, context, at);
        das_invoke<void>::invoke<Stream *>(context, at, blk, &ch);
    }

    void streamPush ( Stream * ch, const TArray<uint8_t> & data, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "streamPush: stream is null");
        ch->push((const uint8_t *)data.data, data.size);
    }

    void streamPushBatch ( Stream * ch, const TArray<TArray<uint8_t>> & data, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "streamPushBatch: stream is null");
        if ( data.size == 0 ) return;
        vector<const uint8_t *> ptrs;
        vector<uint64_t> sizes;
        ptrs.reserve(data.size);
        sizes.reserve(data.size);
        auto items = (const TArray<uint8_t> *)data.data;
        for ( uint64_t i=0; i!=data.size; ++i ) {
            ptrs.push_back((const uint8_t *)items[i].data);
            sizes.push_back(items[i].size);
        }
        ch->pushBatch(ptrs.data(), sizes.data(), int64_t(data.size));
    }

    void streamPop ( Stream * ch, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "streamPop: stream is null");
        ch->pop(blk, context, at);
    }

    bool streamTryPop ( Stream * ch, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "streamTryPop: stream is null");
        return ch->tryPop(blk, context, at);
    }

    bool streamPopWithTimeout ( Stream * ch, int32_t timeoutMs, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "streamPopWithTimeout: stream is null");
        return ch->popWithTimeout(timeoutMs, blk, context, at);
    }

    void streamGather ( Stream * ch, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "streamGather: stream is null");
        ch->gather([&](Array * arr) {
            vec4f args[1];
            args[0] = cast<Array *>::from(arr);
            context->invoke(blk, args, nullptr, at);
        });
    }

    void streamPeek ( Stream * ch, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "streamPeek: stream is null");
        ch->for_each_item([&](Array * arr) {
            vec4f args[1];
            args[0] = cast<Array *>::from(arr);
            context->invoke(blk, args, nullptr, at);
        });
    }

    struct StreamAnnotation : ManagedStructureAnnotation<Stream,false> {
        StreamAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Stream", ml) {
            addProperty<DAS_BIND_MANAGED_PROP(isEmpty)>("isEmpty");
            addProperty<DAS_BIND_MANAGED_PROP(isReady)>("isReady");
            addProperty<DAS_BIND_MANAGED_PROP(size)>("size");
            addProperty<DAS_BIND_MANAGED_PROP(total)>("total");
        }
        virtual int32_t getGcFlags(das_set<Structure *> &, das_set<Annotation *> &) const override {
            return TypeDecl::gcFlag_heap;
        }
    };

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
            bool gResolve = daScriptEnvironment::getBound()->g_resolve_annotations;
            daScriptEnvironment::getBound()->g_resolve_annotations = false;
            BasicStructureAnnotation::walk(walker, data);
            Channel * ch = (Channel *) data;
            if ( !ch->isValid() ) {
                walker.invalidData();
            } else {
                ch->for_each_item([&](void * data, TypeInfo * ti, Context *) {
                    walker.walk((char *)&data, ti);
                });
            }
            daScriptEnvironment::getBound()->g_resolve_annotations = gResolve;
        }
    };

    struct JobStatusAnnotation : ManagedStructureAnnotation<JobStatus,false> {
        JobStatusAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("JobStatus", ml) {
            addProperty<DAS_BIND_MANAGED_PROP(isReady)>("isReady");
            addProperty<DAS_BIND_MANAGED_PROP(isValid)>("isValid");
        }
    };

    SeqBox * seqBoxCreate( Context *, LineInfoArg * at ) {
        SeqBox * box = new SeqBox();
        if ( at ) box->mCreatedAt = at->describe();
        box->addRef(at);
        return box;
    }

    // Release, and whoever drops the last reference deletes. Deliberately not the
    // create/remove-with-rc==1 shape the other primitives use: a status box is shared with the
    // audio thread, and requiring the owner to remove it last is exactly what forced callers to
    // block on join() first — the wait this box exists to remove.
    // The last drop deletes on whichever thread releases last, possibly a realtime one; that is
    // a teardown-only cost (a tracker unlink under mutex plus one free) and an accepted trade.
    void seqBoxRelease( SeqBox * & box, Context * context, LineInfoArg * at ) {
        if ( !box ) context->throw_error_at(at, "seqBoxRelease: box is null");
        if (!box->isValid()) context->throw_error_at(at, "seq box is invalid (already deleted?)");
        if ( box->releaseRef(at)==0 ) delete box;
        box = nullptr;
    }

    void withSeqBox ( const TBlock<void,SeqBox *> & blk, Context * context, LineInfoArg * at ) {
        SeqBox box;
        AddReleaseGuard<SeqBox> guard(&box, context, at);
        das_invoke<void>::invoke<SeqBox *>(context, at, blk, &box);
    }

    // Size comes from the das side (typeinfo sizeof), so the box never needs the value's TypeInfo —
    // it is a byte buffer with a version, and the size is all it can validate.
    bool seqBoxPublish ( SeqBox * box, void * data, int32_t size, Context * context, LineInfoArg * at ) {
        if ( !box ) context->throw_error_at(at, "seqBoxPublish: box is null");
        if ( size <= 0 || uint32_t(size) > uint32_t(SeqBox::PAYLOAD_BYTES) )
            context->throw_error_at(at, "seqBoxPublish: value does not fit the box payload");
        return box->publish(data, uint32_t(size));
    }

    bool seqBoxRead ( SeqBox * box, void * out, int32_t size, Context * context, LineInfoArg * at ) {
        if ( !box ) context->throw_error_at(at, "seqBoxRead: box is null");
        if ( size <= 0 || uint32_t(size) > uint32_t(SeqBox::PAYLOAD_BYTES) )
            context->throw_error_at(at, "seqBoxRead: value does not fit the box payload");
        return box->read(out, uint32_t(size));
    }

    void seqBoxClear ( SeqBox * box, Context * context, LineInfoArg * at ) {
        if ( !box ) context->throw_error_at(at, "seqBoxClear: box is null");
        box->clear();
    }

    struct SeqBoxAnnotation : ManagedStructureAnnotation<SeqBox,false> {
        SeqBoxAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("SeqBox", ml) {
            addProperty<DAS_BIND_MANAGED_PROP(hasValue)>("hasValue");
            addProperty<DAS_BIND_MANAGED_PROP(isValid)>("isValid");
        }
    };

    mutex              g_jobQueMutex;
    shared_ptr<JobQue> g_jobQue;
    shared_ptr<JobQue> g_persistentJobQue;  // extra ref held by create_job_que so a nested with_job_que's use_count==1 teardown can't reset a persistent queue
    // Bound on the with_job_que scope-exit drain. Deliberately inside the interactive zone: work
    // still outstanding when the scope ends is a bug in the caller, and waiting longer to say so
    // only makes it look like a hang. A block whose jobs legitimately run longer should hold the
    // queue open (create_job_que) rather than lean on this.
    int g_jobQueDrainTimeoutMs = 3000;

    LockBox * lockBoxCreate( Context *, LineInfoArg * at ) {
        LockBox * ch = new LockBox();
        if ( at ) ch->mCreatedAt = at->describe();
        ch->addRef(at);
        return ch;
    }

    void lockBoxRemove( LockBox * & ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "lockBoxRemove: lock box is null");
        if (!ch->isValid()) context->throw_error_at(at, "lock box is invalid (already deleted?)");
        if (ch->releaseRef(at)) context->throw_error_at(at, "lock box being deleted while being used");
        delete ch;
        ch = nullptr;
    }

    void withLockBox ( const TBlock<void,LockBox *> & blk, Context * context, LineInfoArg * at ) {
        LockBox ch;
        AddReleaseGuard<LockBox> guard(&ch, context, at);
        das_invoke<void>::invoke<LockBox *>(context, at, blk, &ch);
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

    vec4f lockBoxFill ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto ch = cast<LockBox *>::to(args[0]);
        if ( !ch ) context.throw_error_at(call->debugInfo, "lockBoxFill: box is null");
        void * data = cast<void *>::to(args[1]);
        TypeInfo * ti = call->types[1];
        ch->fill(data, ti, &context);
        return v_zero();
    }

    void lockBoxGrab ( LockBox * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "lockBoxGrab: box is null");
        ch->grab(blk,context,at);
    }

    struct LockBoxAnnotation : ManagedStructureAnnotation<LockBox,false> {
        LockBoxAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LockBox", ml) {
            addProperty<DAS_BIND_MANAGED_PROP(isReady)>("isReady");
        }
        virtual int32_t getGcFlags(das_set<Structure *> &, das_set<Annotation *> &) const override {
            return TypeDecl::gcFlag_heap | TypeDecl::gcFlag_stringHeap;
        }
        virtual void walk(DataWalker & walker, void * data) override {
            bool gResolve = daScriptEnvironment::getBound()->g_resolve_annotations;
            daScriptEnvironment::getBound()->g_resolve_annotations = false;
            BasicStructureAnnotation::walk(walker, data);
            LockBox * ch = (LockBox *) data;
            if ( !ch->isValid() ) {
                walker.invalidData();
            } else {
                ch->peek([&](void * data, TypeInfo * ti, Context *) {
                    walker.walk((char *)&data, ti);
                });
            }
            daScriptEnvironment::getBound()->g_resolve_annotations = gResolve;
        }
    };


    template <typename TT>
    struct AtomicAnnotation : ManagedStructureAnnotation<AtomicTT<TT>,false> {
        AtomicAnnotation(const char * ttname, ModuleLibrary & ml) : ManagedStructureAnnotation<AtomicTT<TT>,false> (ttname, ml) {
        }
        virtual void walk(DataWalker & walker, void * data) override {
            BasicStructureAnnotation::walk(walker, data);
            auto ch = (AtomicTT<TT> *) data;
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
    void set_jobque_fork_pool ( bool keep, bool skipInit, Context * context, LineInfoArg * ) {
        // Pool the per-job fork contexts on this (dispatching) context instead of cloning/destroying
        // one per new_job, and optionally clone them skip-init. Only safe when the dispatched jobs are
        // pure data processing (no globals, no Features referencing the fork) — e.g. parallel_for matmul.
        context->keepForkContexts.store(keep, std::memory_order_relaxed);
        context->forkSkipInitScript.store(skipInit, std::memory_order_relaxed);
    }

    void set_jobque_fork_skip_heap_reset ( bool skip, Context * context, LineInfoArg * ) {
        // Skip restartHeaps() when reusing a pooled fork (see Context::acquireForkContext). Only safe
        // when the dispatched jobs are pure compute that never leaks onto the fork heap — e.g.
        // parallel_for matmul, whose only fork-heap alloc (the lambda capture) is freed LIFO per job.
        context->forkSkipHeapReset.store(skip, std::memory_order_relaxed);
    }

    void set_jobque_worker_spin ( int32_t usec, Context * context, LineInfoArg * at ) {
        // Idle workers spin-poll the fifo for `usec` before parking on the condvar (0 = park
        // immediately, the default). See JobQue::setWorkerSpin for the why. Opt in for fork/join-
        // heavy compute (LLM decode); leave off where idle CPU matters (wasm, audio, low-core).
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->setWorkerSpin(usec);
    }

    // Batched fork-job dispatch (opt-in, per DISPATCHING thread): new_job keeps the finished job
    // closures locally and join() publishes the whole batch under ONE fifo lock + one notify —
    // the woken worker wake-propagates to the rest (JobQue::job), so the fan-out is log-depth.
    // Removes the per-job push/wake interleaving that stalls a fork/join dispatch loop: every
    // notify_one to a parked worker costs the dispatcher an OS thread-wake (~3-4.5us) serially, and
    // with spinning workers the per-push lock acquisition contends with every consumer's pop.
    // Only safe for pure fork/join use — every new_job on this thread must be joined via its
    // JobStatus (join is the flush point); a job whose results are consumed BEFORE join would stall.
    static thread_local bool g_batchForkJobs = false;
    static thread_local vector<Job> g_pendingForkJobs;
    static thread_local bool g_threadTeamMode = true;

    void set_jobque_batch_dispatch ( bool batch, Context *, LineInfoArg * ) {
        g_batchForkJobs = batch;
    }

    static void flushPendingForkJobs () {
        if ( !g_pendingForkJobs.empty() && g_jobQue ) {
            g_jobQue->pushBatch(das::move(g_pendingForkJobs), 0, JobPriority::Default);
            g_pendingForkJobs.clear();  // moved-from — make the reuse explicit
        }
    }

    void flush_jobque_batch ( Context *, LineInfoArg * ) {
        // Publish this thread's pending batched fork jobs NOW instead of at join — pair with
        // jobque_try_run_one when the dispatching thread participates in the work.
        flushPendingForkJobs();
    }

    bool jobque_try_run_one ( Context * context, LineInfoArg * at ) {
        // Dispatcher-side work stealing: flush any pending batch, then pop ONE queued job and run
        // it on the calling thread (the same cloned-closure job a worker would run — full capture
        // semantics). parallel_for loops this until its wait group is ready, so the dispatching
        // core computes chunks instead of sleeping through the fork. False = fifo empty.
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        flushPendingForkJobs();
        return g_jobQue->tryRunOneJob();
    }

    void set_jobque_join_spin ( int32_t level, Context *, LineInfoArg * ) {
        // JobStatus::Wait (the fork/join `join`) polls the remaining-counter for level*1024*128
        // relax-rounds before parking on the condvar (0 = park immediately, the default) — the
        // ggml hybrid-poll shape and denomination (their --poll 0..100, 50 = their default). With
        // parallel_for executing chunks on the calling thread, the join wait is just the workers'
        // tail — the poll removes the last OS park/wake of the fork/join path. Opt in for
        // fork/join-heavy compute (LLM decode/prefill); leave off where idle CPU matters.
        JobStatus::sJoinSpin.store(level, std::memory_order_relaxed);
    }

    // A panic inside a job body used to unwind straight out of the worker's thread function into
    // std::terminate -> abort, which is a fast-fail: no unhandled-exception filter runs, so the
    // message and location were lost and the process died with no diagnostic at all. Catch it on
    // the worker, report it the way the host reports a main-thread panic, then let it terminate as
    // before -- a panic stays fatal, it just stops being silent.
    __forceinline void invoke_job_lambda ( Context * forkContext, LineInfoArg * lineinfo, Lambda & flambda ) {
        bool ok = forkContext->runWithCatch([&]() {
            das_invoke_lambda<void>::invoke(forkContext, lineinfo, flambda);
        });
        if ( !ok ) {
            // A panic stays fatal — it just stops being silent. DAS_FATAL_ERROR logs with a flush,
            // prints the stack, and exits, instead of unwinding into terminate/abort where the
            // message is lost. Whether one bad job should instead fail only that job is a policy
            // question, deliberately not answered here.
            auto what = forkContext->getException();
            DAS_FATAL_ERROR("JOB EXCEPTION: %s at %s\n",
                what ? what : "unknown",
                forkContext->exceptionAt.describe().c_str());
        }
    }

    void new_job_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo ) {
        if ( !g_jobQue ) context->throw_error_at(lineinfo, "need to be in a 'with_job_que' block, or call create_job_que() first");
        if ( context->keepForkContexts.load(std::memory_order_relaxed) ) {
            // pooled path: borrow a reusable fork, return it to the pool when the job finishes
            Context * forkContext = context->acquireForkContext(uint32_t(ContextCategory::job_clone));
            auto ptr = forkContext->allocate(lambdaSize + 16, lineinfo);
            forkContext->heap->mark_comment(ptr, "new [[ ]] in new_job");
            memset ( ptr, 0, lambdaSize + 16 );
            ptr += 16;
            das_invoke_function<void>::invoke(forkContext, lineinfo, fn, ptr, lambda.capture);
            das_delete<Lambda>::clear(context, lambda);
            auto bound = daScriptEnvironment::getBound();
            Context * parent = context;
            Job jobFn = [=]() mutable {
                daScriptEnvironment::setBound(bound);
                Lambda flambda(ptr);
                invoke_job_lambda(forkContext, lineinfo, flambda);
                das_delete<Lambda>::clear(forkContext, flambda);
                parent->releaseForkContext(forkContext);
            };
            if ( g_batchForkJobs ) g_pendingForkJobs.emplace_back(das::move(jobFn));
            else g_jobQue->push(das::move(jobFn), 0, JobPriority::Default);
            return;
        }
        shared_ptr<Context> forkContext;
        forkContext.reset(get_clone_context(context, uint32_t(ContextCategory::job_clone)));
        forkContext->sharedPtrContext = true;
        auto ptr = forkContext->allocate(lambdaSize + 16,lineinfo);
        forkContext->heap->mark_comment(ptr, "new [[ ]] in new_job");
        memset ( ptr, 0, lambdaSize + 16 );
        ptr += 16;
        das_invoke_function<void>::invoke(forkContext.get(), lineinfo, fn, ptr, lambda.capture);
        das_delete<Lambda>::clear(context, lambda);
        auto bound = daScriptEnvironment::getBound();
        Job jobFn = [=]() mutable {
            daScriptEnvironment::setBound(bound);
            Lambda flambda(ptr);
            invoke_job_lambda(forkContext.get(), lineinfo, flambda);
            das_delete<Lambda>::clear(forkContext.get(), flambda);
        };
        if ( g_batchForkJobs ) g_pendingForkJobs.emplace_back(das::move(jobFn));
        else g_jobQue->push(das::move(jobFn), 0, JobPriority::Default);
    }

    void set_jobque_team_mode ( bool on, Context * context, LineInfoArg * at ) {
        // Sticky team dispatch (see JobQue::setTeamMode): workers poll a published team op
        // alongside the fifo; team_parallel_for then costs one seq bump instead of a per-chunk
        // fifo push + wait group. Hybrid poll/park (the ggml --poll shape): workers spin for the
        // set_jobque_worker_spin window then park, and a publish that finds parked workers pays
        // the wake — pair with a worker spin window for back-to-back dispatch bursts. Opt in for
        // fork/join-heavy compute (LLM decode).
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->setTeamMode(on);
    }

    bool get_jobque_team_mode ( Context *, LineInfoArg * ) {
        return g_jobQue && g_jobQue->getTeamMode();
    }

    void set_jobque_thread_team_mode ( bool on, Context *, LineInfoArg * ) {
        // Per-caller override: an inference thread can run team_parallel_* inline while other
        // callers keep publishing to the shared worker team. New OS threads default to enabled.
        g_threadTeamMode = on;
    }

    bool get_jobque_thread_team_mode ( Context *, LineInfoArg * ) {
        return g_threadTeamMode;
    }

    void set_jobque_worker_limit ( int32_t n, Context * context, LineInfoArg * at ) {
        // Worker-pool limit (JobQue::setWorkerLimit): workers with index >= n go dormant —
        // they leave the spin loop and the publish wake gate until the limit rises. A perf
        // dial for inline-dominated workloads (tiny-model LLM decode); never a correctness
        // knob — any limit completes every dispatch (chunks self-serve, the caller drains).
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->setWorkerLimit(n);
    }

    int32_t get_jobque_worker_limit ( Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        return g_jobQue->getWorkerLimit();
    }

    void set_jobque_team_rank_gate ( bool on, Context * context, LineInfoArg * at ) {
        // Per-op worker participation gate (JobQue::setTeamRankGate): a team op admits only as
        // many workers as it published chunks; higher ranks skip the rendezvous and keep
        // spinning, one relaxed load away from being re-admitted by the next big op. A perf
        // dial — any admit set completes every dispatch (chunks self-serve, the caller drains).
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->setTeamRankGate(on);
    }

    bool get_jobque_team_rank_gate ( Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        return g_jobQue->getTeamRankGate();
    }

    // team-prof programmatic rail (the DAS_TEAM_PROF aggregates): probes reset, run a
    // dispatch storm, then read the phase averages — no queue destruction needed.
    void set_jobque_team_prof ( bool on, Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->setTeamProf(on);
    }

    void reset_jobque_team_prof ( Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->resetTeamProf();
    }

    float4 get_jobque_team_prof ( Context * context, LineInfoArg * at ) {
        // avg ns/op: {publish, caller-serve, join-tail, total}
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        auto s = g_jobQue->getTeamProf();
        float n = s.ops ? float(s.ops) : 1.0f;
        return float4{float(s.publishT) / n, float(s.serveT) / n, float(s.tailT) / n, float(s.totalT) / n};
    }

    int4 get_jobque_team_prof_counts ( Context * context, LineInfoArg * at ) {
        // {ops, total chunks, caller-served chunks, solo ops (no worker claimed)}
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        auto s = g_jobQue->getTeamProf();
        auto clampi = [](uint64_t v) -> int32_t { return v > 0x7fffffffull ? 0x7fffffff : int32_t(v); };
        return int4{clampi(s.ops), clampi(s.chunks), clampi(s.callerChunks), clampi(s.solo)};
    }

    float2 get_jobque_team_prof_react ( Context * context, LineInfoArg * at ) {
        // avg ns after publish: {first worker claim, last worker claim}
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        auto s = g_jobQue->getTeamProf();
        return float2{s.reactN ? float(s.reactT) / float(s.reactN) : 0.0f,
                      s.lastN ? float(s.lastT) / float(s.lastN) : 0.0f};
    }

    // per-lane event tracer (JobQue::traceStart/traceStop/traceSave): every lane records
    // publish/chunk/stage-wait/wake/fifo-job events on one global clock; save writes
    // chrome://tracing JSON for Perfetto — one row per lane, the gaps read directly off the
    // timeline.
    void jobque_trace_start ( int32_t eventsPerLane, Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->traceStart(eventsPerLane);
    }

    void jobque_trace_stop ( Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->traceStop();
    }

    bool jobque_trace_save ( const char * path, Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        return g_jobQue->traceSave(path ? path : "");
    }

    void jobque_trace_tag ( int32_t tag, Context *, LineInfoArg * ) {
        // op tag for subsequent trace publishes (viewer color channel); no-op without a que
        if ( g_jobQue ) g_jobQue->traceSetTag(tag);
    }

    void jobque_trace_category ( int32_t id, const char * name, uint32_t color, Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        g_jobQue->traceCategory(id, name ? name : "", color);
    }

    int32_t jobque_trace_marker_name ( const char * name, Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        return g_jobQue->traceMarkerName(name ? name : "");
    }

    void jobque_trace_marker ( int32_t id, int32_t arg, Context *, LineInfoArg * ) {
        // instant "unit" event (frame/token boundary); no-op without a que or with tracing off
        if ( g_jobQue ) g_jobQue->traceMarker(id, arg);
    }

    void jobque_set_thread_priority ( int32_t level, Context *, LineInfoArg * ) {
        // OS priority of the CALLING thread, JobPriority scale -2 (Minimum) .. 2 (Maximum).
        // The dispatch caller is load-bearing in team mode — only it publishes chains, so a
        // descheduled caller idles every lane; JobQue creation runs its thread at High (+1),
        // this exposes the remaining notch (and the rest of the scale) to scripts.
        int32_t l = level < -2 ? -2 : (level > 2 ? 2 : level);
        SetCurrentThreadPriority((JobPriority)l);
    }

    void team_parallel_for_invoke ( int32_t rangeBegin, int32_t rangeEnd, int32_t numChunks, Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo ) {
        if ( !g_jobQue ) context->throw_error_at(lineinfo, "need to be in a 'with_job_que' block, or call create_job_que() first");
        int total = rangeEnd - rangeBegin;
        if ( total <= 0 || numChunks <= 0 ) {
            das_delete<Lambda>::clear(context, lambda);
            return;
        }
        int actualChunks = numChunks < total ? numChunks : total;
        int nW = g_jobQue->getNumWorkers();
        if ( actualChunks == 1 || nW == 0 || !g_jobQue->getTeamMode() || !g_threadTeamMode ) {
            das_invoke_lambda<void>::invoke(context, lineinfo, lambda, rangeBegin, rangeEnd);
            das_delete<Lambda>::clear(context, lambda);
            return;
        }
        int chunkSz = total / actualChunks;
        int rem = total % actualChunks;
        // one lambda clone per WORKER (the fifo path clones per chunk); the caller runs the original
        vector<Context *> forkCtx(nW);
        vector<char *> clonePtr(nW);
        vector<shared_ptr<Context>> ownedCtx;   // non-pooled path: keep clones alive till the join
        bool pooled = context->keepForkContexts.load(std::memory_order_relaxed);
        if ( !pooled ) ownedCtx.resize(nW);
        for ( int w = 0; w != nW; ++w ) {
            Context * fc;
            if ( pooled ) {
                fc = context->acquireForkContext(uint32_t(ContextCategory::job_clone));
            } else {
                ownedCtx[w].reset(get_clone_context(context, uint32_t(ContextCategory::job_clone)));
                ownedCtx[w]->sharedPtrContext = true;
                fc = ownedCtx[w].get();
            }
            auto ptr = fc->allocate(lambdaSize + 16, lineinfo);
            fc->heap->mark_comment(ptr, "new [[ ]] in team_parallel_for");
            memset(ptr, 0, lambdaSize + 16);
            ptr += 16;
            das_invoke_function<void>::invoke(fc, lineinfo, fn, ptr, lambda.capture);
            forkCtx[w] = fc;
            clonePtr[w] = ptr;
        }
        auto bound = daScriptEnvironment::getBound();
        JobChunk work = [&](int chunkIdx, int slot) {
            int rb = rangeBegin + chunkIdx * chunkSz + (chunkIdx < rem ? chunkIdx : rem);
            int re = rb + chunkSz + (chunkIdx < rem ? 1 : 0);
            if ( slot == nW ) {
                das_invoke_lambda<void>::invoke(context, lineinfo, lambda, rb, re);
            } else {
                daScriptEnvironment::setBound(bound);
                Lambda flambda(clonePtr[slot]);
                das_invoke_lambda<void>::invoke(forkCtx[slot], lineinfo, flambda, rb, re);
            }
        };
        g_jobQue->teamParallelFor(actualChunks, work);
        for ( int w = 0; w != nW; ++w ) {
            Lambda flambda(clonePtr[w]);
            das_delete<Lambda>::clear(forkCtx[w], flambda);
            if ( pooled ) context->releaseForkContext(forkCtx[w]);
        }
        das_delete<Lambda>::clear(context, lambda);
    }

    void team_parallel_for_indexed_invoke ( int32_t rangeBegin, int32_t rangeEnd, int32_t numChunks, Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo ) {
        // team_parallel_for_invoke with the worker slot exposed: the lambda is invoked as
        // (slot, job_begin, job_end). JobQue::teamParallelFor already hands each chunk its
        // claiming worker's slot (caller == getNumWorkers()); the non-indexed binding drops it.
        // No two in-flight invocations share a slot, so slot-indexed scratch needs no locks;
        // slot < getNumWorkers()+1 (degenerate/inline paths run everything as slot 0).
        if ( !g_jobQue ) context->throw_error_at(lineinfo, "need to be in a 'with_job_que' block, or call create_job_que() first");
        int total = rangeEnd - rangeBegin;
        if ( total <= 0 || numChunks <= 0 ) {
            das_delete<Lambda>::clear(context, lambda);
            return;
        }
        int actualChunks = numChunks < total ? numChunks : total;
        int nW = g_jobQue->getNumWorkers();
        if ( actualChunks == 1 || nW == 0 || !g_jobQue->getTeamMode() || !g_threadTeamMode ) {
            das_invoke_lambda<void>::invoke(context, lineinfo, lambda, 0, rangeBegin, rangeEnd);
            das_delete<Lambda>::clear(context, lambda);
            return;
        }
        int chunkSz = total / actualChunks;
        int rem = total % actualChunks;
        // one lambda clone per WORKER (the fifo path clones per chunk); the caller runs the original
        vector<Context *> forkCtx(nW);
        vector<char *> clonePtr(nW);
        vector<shared_ptr<Context>> ownedCtx;   // non-pooled path: keep clones alive till the join
        bool pooled = context->keepForkContexts.load(std::memory_order_relaxed);
        if ( !pooled ) ownedCtx.resize(nW);
        for ( int w = 0; w != nW; ++w ) {
            Context * fc;
            if ( pooled ) {
                fc = context->acquireForkContext(uint32_t(ContextCategory::job_clone));
            } else {
                ownedCtx[w].reset(get_clone_context(context, uint32_t(ContextCategory::job_clone)));
                ownedCtx[w]->sharedPtrContext = true;
                fc = ownedCtx[w].get();
            }
            auto ptr = fc->allocate(lambdaSize + 16, lineinfo);
            fc->heap->mark_comment(ptr, "new [[ ]] in team_parallel_for_indexed");
            memset(ptr, 0, lambdaSize + 16);
            ptr += 16;
            das_invoke_function<void>::invoke(fc, lineinfo, fn, ptr, lambda.capture);
            forkCtx[w] = fc;
            clonePtr[w] = ptr;
        }
        auto bound = daScriptEnvironment::getBound();
        JobChunk work = [&](int chunkIdx, int slot) {
            int rb = rangeBegin + chunkIdx * chunkSz + (chunkIdx < rem ? chunkIdx : rem);
            int re = rb + chunkSz + (chunkIdx < rem ? 1 : 0);
            if ( slot == nW ) {
                das_invoke_lambda<void>::invoke(context, lineinfo, lambda, slot, rb, re);
            } else {
                daScriptEnvironment::setBound(bound);
                Lambda flambda(clonePtr[slot]);
                das_invoke_lambda<void>::invoke(forkCtx[slot], lineinfo, flambda, slot, rb, re);
            }
        };
        g_jobQue->teamParallelFor(actualChunks, work);
        for ( int w = 0; w != nW; ++w ) {
            Lambda flambda(clonePtr[w]);
            das_delete<Lambda>::clear(forkCtx[w], flambda);
            if ( pooled ) context->releaseForkContext(forkCtx[w]);
        }
        das_delete<Lambda>::clear(context, lambda);
    }

    void team_parallel_stages_invoke ( const TArray<int3> & stages, Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo ) {
        // Multi-stage team dispatch (see JobQue::teamParallelForStages): one rendezvous, one join,
        // internal barriers between stages. Each stage s of the array is int3(range_begin,
        // range_end, num_chunks); the lambda is invoked as lambda(stage, job_begin, job_end) with
        // the same chunk split as team_parallel_for. One lambda clone per worker for the WHOLE
        // chain (the single-op path clones per op).
        if ( !g_jobQue ) context->throw_error_at(lineinfo, "need to be in a 'with_job_que' block, or call create_job_que() first");
        int numStages = int(stages.size);
        if ( numStages <= 0 ) {
            das_delete<Lambda>::clear(context, lambda);
            return;
        }
        if ( numStages > JobQue::MAX_TEAM_STAGES ) context->throw_error_at(lineinfo, "team_parallel_stages: at most %i stages", JobQue::MAX_TEAM_STAGES);
        auto stageDef = (const int3 *)stages.data;
        int actualChunks[JobQue::MAX_TEAM_STAGES];
        int maxChunks = 0;
        for ( int s = 0; s != numStages; ++s ) {
            int total = stageDef[s].y - stageDef[s].x;
            int want = stageDef[s].z;
            actualChunks[s] = total <= 0 ? 0 : (want < 1 ? 1 : (want < total ? want : total));
            maxChunks = actualChunks[s] > maxChunks ? actualChunks[s] : maxChunks;
        }
        int nW = g_jobQue->getNumWorkers();
        if ( maxChunks <= 1 || nW == 0 || !g_jobQue->getTeamMode() || !g_threadTeamMode ) {
            // trivial/off: stages in order, one inline invoke per non-empty stage
            for ( int s = 0; s != numStages; ++s ) {
                if ( actualChunks[s] > 0 ) {
                    das_invoke_lambda<void>::invoke(context, lineinfo, lambda, s, stageDef[s].x, stageDef[s].y);
                }
            }
            das_delete<Lambda>::clear(context, lambda);
            return;
        }
        // one lambda clone per WORKER, shared across all stages
        vector<Context *> forkCtx(nW);
        vector<char *> clonePtr(nW);
        vector<shared_ptr<Context>> ownedCtx;   // non-pooled path: keep clones alive till the join
        bool pooled = context->keepForkContexts.load(std::memory_order_relaxed);
        if ( !pooled ) ownedCtx.resize(nW);
        for ( int w = 0; w != nW; ++w ) {
            Context * fc;
            if ( pooled ) {
                fc = context->acquireForkContext(uint32_t(ContextCategory::job_clone));
            } else {
                ownedCtx[w].reset(get_clone_context(context, uint32_t(ContextCategory::job_clone)));
                ownedCtx[w]->sharedPtrContext = true;
                fc = ownedCtx[w].get();
            }
            auto ptr = fc->allocate(lambdaSize + 16, lineinfo);
            fc->heap->mark_comment(ptr, "new [[ ]] in team_parallel_stages");
            memset(ptr, 0, lambdaSize + 16);
            ptr += 16;
            das_invoke_function<void>::invoke(fc, lineinfo, fn, ptr, lambda.capture);
            forkCtx[w] = fc;
            clonePtr[w] = ptr;
        }
        auto bound = daScriptEnvironment::getBound();
        JobChunk works[JobQue::MAX_TEAM_STAGES];
        JobQue::TeamStage teamStages[JobQue::MAX_TEAM_STAGES];
        for ( int s = 0; s != numStages; ++s ) {
            int rangeBegin = stageDef[s].x;
            int total = stageDef[s].y - stageDef[s].x;
            int nCh = actualChunks[s];
            int chunkSz = nCh > 0 ? total / nCh : 0;
            int rem = nCh > 0 ? total % nCh : 0;
            works[s] = [&, s, rangeBegin, chunkSz, rem](int chunkIdx, int slot) {
                int rb = rangeBegin + chunkIdx * chunkSz + (chunkIdx < rem ? chunkIdx : rem);
                int re = rb + chunkSz + (chunkIdx < rem ? 1 : 0);
                if ( slot == nW ) {
                    das_invoke_lambda<void>::invoke(context, lineinfo, lambda, s, rb, re);
                } else {
                    daScriptEnvironment::setBound(bound);
                    Lambda flambda(clonePtr[slot]);
                    das_invoke_lambda<void>::invoke(forkCtx[slot], lineinfo, flambda, s, rb, re);
                }
            };
            teamStages[s].work = &works[s];
            teamStages[s].numChunks = nCh;
        }
        g_jobQue->teamParallelForStages(teamStages, numStages);
        for ( int w = 0; w != nW; ++w ) {
            Lambda flambda(clonePtr[w]);
            das_delete<Lambda>::clear(forkCtx[w], flambda);
            if ( pooled ) context->releaseForkContext(forkCtx[w]);
        }
        das_delete<Lambda>::clear(context, lambda);
    }

    static atomic<int32_t> g_jobQueAvailable{0};
    static atomic<int32_t> g_jobQueTotalThreads{0};

    bool is_job_que_shutting_down () {
        return g_jobQueAvailable == 0;
    }

    bool is_job_que_available () {
        // Non-throwing "does a queue exist" — the que-gated externs (get_total_hw_jobs etc.)
        // throw instead. Lets dispatch shapers answer "1 lane, run inline" on a que-less process.
        return g_jobQue != nullptr;
    }

    uint64_t count_jobque_leaks () {
        // Number of live JobStatus/Channel/LockBox/SeqBox/Stream + Feature objects globally.
        // Tests compare before/after a teardown to assert no net leak (dastest's own
        // infrastructure may hold some, so an absolute count is not meaningful).
        return JobStatus::CountJobQueLeaks();
    }

    void new_thread_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo ) {
        shared_ptr<Context> forkContext;
        forkContext.reset(get_clone_context(context, uint32_t(ContextCategory::thread_clone)));
        forkContext->sharedPtrContext = true;
        auto ptr = forkContext->allocate(lambdaSize + 16,lineinfo);
        forkContext->heap->mark_comment(ptr, "new [[ ]] in new_thread");
        memset ( ptr, 0, lambdaSize + 16 );
        ptr += 16;
        das_invoke_function<void>::invoke(forkContext.get(), lineinfo, fn, ptr, lambda.capture);
        das_delete<Lambda>::clear(context, lambda);
        g_jobQueTotalThreads ++;
        auto bound = daScriptEnvironment::getBound();
        thread([=]() mutable {
            daScriptEnvironment::setBound(bound);
            Lambda flambda(ptr);
            // Same hole as new_job_invoke: the thread is detached, so an escaping panic unwinds into
            // std::terminate -> abort with the message lost. A detached thread has no join point to
            // report at, so it reports here and dies here.
            invoke_job_lambda(forkContext.get(), lineinfo, flambda);
            das_delete<Lambda>::clear(forkContext.get(), flambda);
            g_jobQueTotalThreads --;
            shutdownThreadLocalDebugAgent();
        }).detach();
    }

    extern condition_variable debugger_stopped;
    extern atomic<bool>       debugger_started;
    extern atomic<bool>       stopped;
    extern mutex              debugger_mutex;
    extern atomic<bool>       stop_requested;

    static void stop_debugger() {
        g_jobQueTotalThreads --;
        {
            lock_guard guard{debugger_mutex};
            stopped.store(true);
        }
        debugger_stopped.notify_all();
    }

    void new_debugger_thread ( const Block & lambda, Context * context, LineInfoArg * lineinfo ) {
        g_jobQueTotalThreads ++;
        debugger_started.store(true);
        shared_ptr<Context> forkContext;
        forkContext.reset(get_clone_context(context, uint32_t(ContextCategory::thread_clone)));
        forkContext->sharedPtrContext = true;
        auto bound = daScriptEnvironment::getBound();
        thread([=]() mutable {
            daScriptEnvironment::setBound(bound);
            das_invoke<void>::invoke(forkContext.get(), lineinfo, lambda);
            forkContext.reset();
            stop_debugger();
            stop_requested = false;
            shutdownThreadLocalDebugAgent();
        }).detach();
    }

    void withJobQue ( const TBlock<void> & block, Context * context, LineInfoArg * lineInfo ) {
        {
            lock_guard<mutex> guard(g_jobQueMutex);
            if ( !g_jobQue ) g_jobQue = make_shared<JobQue>();  // check under the lock: create_job_que can also init g_jobQue concurrently
        }
        {
            shared_ptr<JobQue> jq = g_jobQue;
            context->invoke(block, nullptr, nullptr, lineInfo);
        }
        // Batch dispatch is a thread-local, block-scoped contract: flush any stragglers and reset the
        // flag so it cannot leak into unrelated jobque users later on this thread (e.g. the next test
        // in a dastest process, whose consume-before-join pattern would stall on unflushed jobs).
        flushPendingForkJobs();
        g_batchForkJobs = false;
        // Drain before teardown. ~JobQue only join()s -- it flags shutdown and waits for the
        // THREADS, and a worker tests that flag before the fifo, so anything still queued was
        // silently discarded. A job pushed without a channel to wait on simply never ran.
        // Failing to drain is an error, not something to swallow.
        bool drained = true;
        {
            // Copy under the lock -- create_job_que / destroy_job_que and other with_job_que
            // scopes assign g_jobQue, and a shared_ptr copy races an assignment to the same
            // object. Drain OUTSIDE the lock: it can wait up to the timeout, and code reachable
            // from a job may take this mutex.
            shared_ptr<JobQue> jq;
            {
                lock_guard<mutex> guard(g_jobQueMutex);
                jq = g_jobQue;
            }
            if ( jq ) drained = jq->drain(g_jobQueDrainTimeoutMs);
        }
        {
            lock_guard<mutex> guard(g_jobQueMutex);
            if ( g_jobQue.use_count()==1 ) g_jobQue.reset();
        }
        if ( !drained ) {
            context->throw_error_at(lineInfo,
                "with_job_que: jobs did not complete within %d ms; queue torn down with work outstanding",
                g_jobQueDrainTimeoutMs);
        }
    }

    // Persistent job-que lifecycle. Unlike with_job_que (block-scoped), the pool created here lives
    // until destroy_job_que — needed when the frame loop is driven externally (e.g. emscripten's
    // async set_main_loop), where a with_job_que block cannot span the per-frame new_job dispatches.
    void createJobQue ( Context *, LineInfoArg * ) {
        lock_guard<mutex> guard(g_jobQueMutex);
        if ( !g_jobQue ) g_jobQue = make_shared<JobQue>();
        g_persistentJobQue = g_jobQue;  // pin: keeps use_count >= 2 so with_job_que won't reset it
    }

    void destroyJobQue ( Context *, LineInfoArg * ) {
        flushPendingForkJobs();     // batch-dispatch stragglers (see withJobQue) — flush before the drain below
        g_batchForkJobs = false;
        lock_guard<mutex> guard(g_jobQueMutex);
        g_persistentJobQue.reset();
        if ( g_jobQue.use_count()==1 ) g_jobQue.reset();  // ~JobQue drains/joins the pool
    }

    void jobStatusAddRef ( JobStatus * status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "jobStatusAddRef: status is null");
        status->addRef(at);
    }

    void jobStatusReleaseRef ( JobStatus * & status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "jobStatusReleaseRef: status is null");
        status->releaseRef(at);
        status = nullptr;
    }

    void withJobStatus ( int32_t total, const TBlock<void,JobStatus *> & block, Context * context, LineInfoArg * lineInfo ) {
        JobStatus status(total);
        AddReleaseGuard<JobStatus> guard(&status, context, lineInfo);
        vec4f args[1];
        args[0] = cast<JobStatus *>::from(&status);
        context->invoke(block,args,nullptr,lineInfo);
    }

    JobStatus * jobStatusCreate( Context *, LineInfoArg * at ) {
        JobStatus * ch = new JobStatus();
        if ( at ) ch->mCreatedAt = at->describe();
        ch->addRef(at);
        return ch;
    }

    void jobStatusRemove( JobStatus * & ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "jobStatusRemove: job status is null");
        if (!ch->isValid()) context->throw_error_at(at, "job status is invalid (already deleted?)");
        if (ch->releaseRef(at)) context->throw_error_at(at, "job status being deleted while being used");
        delete ch;
        ch = nullptr;
    }

    void waitForJob ( JobStatus * status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "waitForJob: status is null");
        flushPendingForkJobs();     // batched dispatch publishes at the join point
        status->Wait();
    }

    void notifyJob ( JobStatus * status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "notifyJob: status is null");
        if ( !status->Notify() ) context->throw_error_at(at, "notifyJob: nothing to notify");
    }

    void notifyAndReleaseJob ( JobStatus * & status, Context * context, LineInfoArg * at ) {
        if ( !status ) context->throw_error_at(at, "notifyAndReleaseJob: status is null");
        if ( !status->NotifyAndRelease(at) ) context->throw_error_at(at, "notifyAndReleaseJob: nothing to notify");
        status = nullptr;
    }

    int getTotalHwJobs( Context * context, LineInfoArg * at ) {
        if ( !g_jobQue ) context->throw_error_at(at, "need to be in a 'with_job_que' block, or call create_job_que() first");
        return g_jobQue->getTotalHwJobs();
    }

    int getTotalHwThreads () {
        return JobQue::get_num_threads();
    }

    int getTotalHwCores () {
        return JobQue::get_num_physical_cores();
    }

    void setJobqueThreadsCap ( int32_t cap ) {
        JobQue::set_default_threads_cap(cap);
    }

    // Affinity mode of a future JobQue: 0 off / 1 ideal-CPU hint / 2 hard mask (-1 = unset).
    // Applied at worker spawn — call BEFORE with_job_que / create_job_que. The
    // DAS_JOBQUE_AFFINITY env still overrides it (the A/B rail).
    void setJobqueAffinity ( int32_t mode ) {
        JobQue::set_default_affinity(mode);
    }

    int32_t getJobqueAffinity () {
        return JobQue::get_default_affinity();
    }

    class Module_JobQue : public Module {
    public:
        Module_JobQue() : Module("jobque") {
            DAS_PROFILE_SECTION("Module_JobQue");
            g_jobQueAvailable++;
            // libs
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            addBuiltinDependency(lib, Module::require("rtti_core"));
            // types
            addAnnotation(new JobStatusAnnotation(lib));
            auto cha = new ChannelAnnotation(lib);
            cha->from("JobStatus");
            addAnnotation(cha);
            auto lbx = new LockBoxAnnotation(lib);
            lbx->from("JobStatus");
            addAnnotation(lbx);
            auto stra = new StreamAnnotation(lib);
            stra->from("JobStatus");
            addAnnotation(stra);
            auto sqb = new SeqBoxAnnotation(lib);
            sqb->from("JobStatus");
            addAnnotation(sqb);
            auto a32 = new AtomicAnnotation<int32_t>("Atomic32",lib);
            addAnnotation(a32);
            auto a64 = new AtomicAnnotation<int64_t>("Atomic64",lib);
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
            addInterop<lockBoxFill,void,LockBox *,vec4f>(*this, lib,  "_builtin_lockbox_fill",
                SideEffects::modifyArgumentAndExternal, "lockBoxFill")
                    ->args({"box","data"});
            addExtern<DAS_BIND_FUN(lockBoxGrab)>(*this, lib,  "_builtin_lockbox_grab",
                SideEffects::modifyArgumentAndExternal, "lockBoxGrab")
                    ->args({"box","block","context","line"});
            // seq box
            // Neither create nor release is unsafe: the box cannot be deleted while another
            // holder still references it, so there is no lifetime hazard to gate.
            // seq_box_release is the only deleter: the family JobStatus `release` (like on every
            // other primitive) only drops a share, so a last reference dropped through it leaks.
            addConstant<int>(*this, "SEQ_BOX_PAYLOAD", SeqBox::PAYLOAD_BYTES);
            addExtern<DAS_BIND_FUN(seqBoxCreate)>(*this, lib, "seq_box_create",
                SideEffects::invoke, "seqBoxCreate")
                    ->args({ "context","line" });
            addExtern<DAS_BIND_FUN(seqBoxRelease)>(*this, lib, "seq_box_release",
                SideEffects::modifyArgumentAndAccessExternal, "seqBoxRelease")
                    ->args({ "box", "context","line" });
            addExtern<DAS_BIND_FUN(withSeqBox)>(*this, lib,  "with_seq_box",
                SideEffects::invoke, "withSeqBox")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(seqBoxPublish)>(*this, lib,  "_builtin_seq_box_publish",
                SideEffects::modifyArgumentAndExternal, "seqBoxPublish")
                    ->args({"box","data","size","context","line"});
            addExtern<DAS_BIND_FUN(seqBoxRead)>(*this, lib,  "_builtin_seq_box_read",
                SideEffects::modifyArgumentAndExternal, "seqBoxRead")
                    ->args({"box","out","size","context","line"});
            addExtern<DAS_BIND_FUN(seqBoxClear)>(*this, lib,  "clear",
                SideEffects::modifyArgumentAndExternal, "seqBoxClear")
                    ->args({"box","context","line"});
            // channel
            addInterop<channelPush,void,Channel *,vec4f>(*this, lib,  "_builtin_channel_push",
                SideEffects::modifyArgumentAndExternal, "channelPush")
                    ->args({"channel","data"});
            addInterop<channelPushBatch,void,Channel *,vec4f>(*this, lib,  "_builtin_channel_push_batch",
                SideEffects::modifyArgumentAndExternal, "channelPushBatch")
                    ->args({"channel","data"});
            addExtern<DAS_BIND_FUN(channelPop)>(*this, lib,  "_builtin_channel_pop",
                SideEffects::modifyArgumentAndExternal, "channelPop")
                    ->args({"channel","block","context","line"});
            addExtern<DAS_BIND_FUN(channelTryPop)>(*this, lib,  "_builtin_channel_try_pop",
                SideEffects::modifyArgumentAndExternal, "channelTryPop")
                    ->args({"channel","block","context","line"});
            addExtern<DAS_BIND_FUN(channelPopWithTimeout)>(*this, lib,  "_builtin_channel_pop_with_timeout",
                SideEffects::modifyArgumentAndExternal, "channelPopWithTimeout")
                    ->args({"channel","timeout_ms","block","context","line"});
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
            // stream
            addExtern<DAS_BIND_FUN(streamPush)>(*this, lib, "_builtin_stream_push",
                SideEffects::modifyArgumentAndExternal, "streamPush")
                    ->args({"stream","data","context","line"});
            addExtern<DAS_BIND_FUN(streamPushBatch)>(*this, lib, "_builtin_stream_push_batch",
                SideEffects::modifyArgumentAndExternal, "streamPushBatch")
                    ->args({"stream","data","context","line"});
            addExtern<DAS_BIND_FUN(streamPop)>(*this, lib, "_builtin_stream_pop",
                SideEffects::modifyArgumentAndExternal, "streamPop")
                    ->args({"stream","block","context","line"});
            addExtern<DAS_BIND_FUN(streamTryPop)>(*this, lib, "_builtin_stream_try_pop",
                SideEffects::modifyArgumentAndExternal, "streamTryPop")
                    ->args({"stream","block","context","line"});
            addExtern<DAS_BIND_FUN(streamPopWithTimeout)>(*this, lib, "_builtin_stream_pop_with_timeout",
                SideEffects::modifyArgumentAndExternal, "streamPopWithTimeout")
                    ->args({"stream","timeout_ms","block","context","line"});
            addExtern<DAS_BIND_FUN(streamGather)>(*this, lib, "_builtin_stream_gather",
                SideEffects::modifyArgumentAndExternal, "streamGather")
                    ->args({"stream","block","context","line"});
            addExtern<DAS_BIND_FUN(streamPeek)>(*this, lib, "_builtin_stream_peek",
                SideEffects::modifyArgumentAndExternal, "streamPeek")
                    ->args({"stream","block","context","line"});
            addExtern<DAS_BIND_FUN(withStream)>(*this, lib, "with_stream",
                SideEffects::invoke, "withStream")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(withStreamEx)>(*this, lib, "with_stream",
                SideEffects::invoke, "withStreamEx")
                    ->args({"count","block","context","line"});
            addExtern<DAS_BIND_FUN(streamCreate)>(*this, lib, "stream_create",
                SideEffects::invoke, "streamCreate")
                    ->args({ "context","line" })->unsafeOperation = true;
            addExtern<DAS_BIND_FUN(streamRemove)>(*this, lib, "stream_remove",
                SideEffects::invoke, "streamRemove")
                    ->args({ "stream", "context","line" })->unsafeOperation = true;
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
            addExtern<DAS_BIND_FUN(set_jobque_fork_pool)>(*this, lib,  "set_jobque_fork_pool",
                SideEffects::modifyExternal, "set_jobque_fork_pool")
                    ->args({"keep","skip_init","context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_fork_skip_heap_reset)>(*this, lib,  "set_jobque_fork_skip_heap_reset",
                SideEffects::modifyExternal, "set_jobque_fork_skip_heap_reset")
                    ->args({"skip","context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_worker_spin)>(*this, lib,  "set_jobque_worker_spin",
                SideEffects::modifyExternal, "set_jobque_worker_spin")
                    ->args({"usec","context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_batch_dispatch)>(*this, lib,  "set_jobque_batch_dispatch",
                SideEffects::modifyExternal, "set_jobque_batch_dispatch")
                    ->args({"batch","context","line"});
            addExtern<DAS_BIND_FUN(flush_jobque_batch)>(*this, lib,  "flush_jobque_batch",
                SideEffects::modifyExternal, "flush_jobque_batch")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(jobque_try_run_one)>(*this, lib,  "jobque_try_run_one",
                SideEffects::modifyExternal, "jobque_try_run_one")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_join_spin)>(*this, lib,  "set_jobque_join_spin",
                SideEffects::modifyExternal, "set_jobque_join_spin")
                    ->args({"level","context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_team_mode)>(*this, lib,  "set_jobque_team_mode",
                SideEffects::modifyExternal, "set_jobque_team_mode")
                    ->args({"on","context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_thread_team_mode)>(*this, lib,  "set_jobque_thread_team_mode",
                SideEffects::modifyExternal, "set_jobque_thread_team_mode")
                    ->args({"on","context","line"});
            addExtern<DAS_BIND_FUN(get_jobque_thread_team_mode)>(*this, lib,  "get_jobque_thread_team_mode",
                SideEffects::accessExternal, "get_jobque_thread_team_mode")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_worker_limit)>(*this, lib,  "set_jobque_worker_limit",
                SideEffects::modifyExternal, "set_jobque_worker_limit")
                    ->args({"limit","context","line"});
            addExtern<DAS_BIND_FUN(get_jobque_worker_limit)>(*this, lib,  "get_jobque_worker_limit",
                SideEffects::accessExternal, "get_jobque_worker_limit")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_team_rank_gate)>(*this, lib,  "set_jobque_team_rank_gate",
                SideEffects::modifyExternal, "set_jobque_team_rank_gate")
                    ->args({"on","context","line"});
            addExtern<DAS_BIND_FUN(get_jobque_team_rank_gate)>(*this, lib,  "get_jobque_team_rank_gate",
                SideEffects::accessExternal, "get_jobque_team_rank_gate")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(get_jobque_team_mode)>(*this, lib,  "get_jobque_team_mode",
                SideEffects::accessExternal, "get_jobque_team_mode")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(set_jobque_team_prof)>(*this, lib,  "set_jobque_team_prof",
                SideEffects::modifyExternal, "set_jobque_team_prof")
                    ->args({"on","context","line"});
            addExtern<DAS_BIND_FUN(reset_jobque_team_prof)>(*this, lib,  "reset_jobque_team_prof",
                SideEffects::modifyExternal, "reset_jobque_team_prof")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(get_jobque_team_prof)>(*this, lib,  "get_jobque_team_prof",
                SideEffects::accessExternal, "get_jobque_team_prof")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(get_jobque_team_prof_counts)>(*this, lib,  "get_jobque_team_prof_counts",
                SideEffects::accessExternal, "get_jobque_team_prof_counts")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(get_jobque_team_prof_react)>(*this, lib,  "get_jobque_team_prof_react",
                SideEffects::accessExternal, "get_jobque_team_prof_react")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(jobque_trace_start)>(*this, lib,  "jobque_trace_start",
                SideEffects::modifyExternal, "jobque_trace_start")
                    ->args({"events_per_lane","context","line"});
            addExtern<DAS_BIND_FUN(jobque_trace_stop)>(*this, lib,  "jobque_trace_stop",
                SideEffects::modifyExternal, "jobque_trace_stop")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(jobque_trace_save)>(*this, lib,  "jobque_trace_save",
                SideEffects::modifyExternal, "jobque_trace_save")
                    ->args({"path","context","line"});
            addExtern<DAS_BIND_FUN(jobque_trace_tag)>(*this, lib,  "jobque_trace_tag",
                SideEffects::modifyExternal, "jobque_trace_tag")
                    ->args({"tag","context","line"});
            addExtern<DAS_BIND_FUN(jobque_trace_category)>(*this, lib,  "jobque_trace_category",
                SideEffects::modifyExternal, "jobque_trace_category")
                    ->args({"id","name","color","context","line"});
            addExtern<DAS_BIND_FUN(jobque_trace_marker_name)>(*this, lib,  "jobque_trace_marker_name",
                SideEffects::modifyExternal, "jobque_trace_marker_name")
                    ->args({"name","context","line"});
            addExtern<DAS_BIND_FUN(jobque_trace_marker)>(*this, lib,  "jobque_trace_marker",
                SideEffects::modifyExternal, "jobque_trace_marker")
                    ->args({"id","arg","context","line"});
            addExtern<DAS_BIND_FUN(jobque_set_thread_priority)>(*this, lib,  "set_current_thread_priority",
                SideEffects::modifyExternal, "jobque_set_thread_priority")
                    ->args({"level","context","line"});
            addExtern<DAS_BIND_FUN(team_parallel_for_invoke)>(*this, lib,  "team_parallel_for_invoke",
                SideEffects::modifyExternal, "team_parallel_for_invoke")
                    ->args({"range_begin","range_end","num_chunks","lambda","function","lambdaSize","context","line"});
            addExtern<DAS_BIND_FUN(team_parallel_for_indexed_invoke)>(*this, lib,  "team_parallel_for_indexed_invoke",
                SideEffects::modifyExternal, "team_parallel_for_indexed_invoke")
                    ->args({"range_begin","range_end","num_chunks","lambda","function","lambdaSize","context","line"});
            addExtern<DAS_BIND_FUN(team_parallel_stages_invoke)>(*this, lib,  "team_parallel_stages_invoke",
                SideEffects::modifyExternal, "team_parallel_stages_invoke")
                    ->args({"stages","lambda","function","lambdaSize","context","line"});
            addExtern<DAS_BIND_FUN(withJobQue)>(*this, lib,  "with_job_que",
                SideEffects::modifyExternal, "withJobQue")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(createJobQue)>(*this, lib,  "create_job_que",
                SideEffects::modifyExternal, "createJobQue")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(destroyJobQue)>(*this, lib,  "destroy_job_que",
                SideEffects::modifyExternal, "destroyJobQue")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(getTotalHwJobs)>(*this, lib,  "get_total_hw_jobs",
                SideEffects::accessExternal, "getTotalHwJobs")
                    ->args({"context","line"});
            addExtern<DAS_BIND_FUN(getTotalHwThreads)>(*this, lib,  "get_total_hw_threads",
                SideEffects::accessExternal, "getTotalHwThreads");
            addExtern<DAS_BIND_FUN(getTotalHwCores)>(*this, lib,  "get_total_hw_cores",
                SideEffects::accessExternal, "getTotalHwCores");
            addExtern<DAS_BIND_FUN(setJobqueThreadsCap)>(*this, lib,  "set_jobque_threads_cap",
                SideEffects::modifyExternal, "setJobqueThreadsCap")
                    ->args({"cap"});
            addExtern<DAS_BIND_FUN(setJobqueAffinity)>(*this, lib,  "set_jobque_affinity",
                SideEffects::modifyExternal, "setJobqueAffinity")
                    ->args({"mode"});
            addExtern<DAS_BIND_FUN(getJobqueAffinity)>(*this, lib,  "get_jobque_affinity",
                SideEffects::accessExternal, "getJobqueAffinity");
            addExtern<DAS_BIND_FUN(new_thread_invoke)>(*this, lib,  "new_thread_invoke",
                SideEffects::modifyExternal, "new_thread_invoke")
                    ->args({"lambda","function","lambdaSize","context","line"});
            addExtern<DAS_BIND_FUN(new_debugger_thread)>(*this, lib,  "new_debugger_thread",
                SideEffects::modifyExternal, "new_debugger_thread")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(is_job_que_shutting_down)>(*this, lib,  "is_job_que_shutting_down",
                SideEffects::modifyExternal, "is_job_que_shutting_down");
            addExtern<DAS_BIND_FUN(is_job_que_available)>(*this, lib,  "is_job_que_available",
                SideEffects::accessExternal, "is_job_que_available");
            addExtern<DAS_BIND_FUN(count_jobque_leaks)>(*this, lib,  "count_jobque_leaks",
                SideEffects::accessExternal, "count_jobque_leaks");
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
                g_persistentJobQue.reset();  // module unload: drop the create_job_que pin so the pool is torn down even if destroy_job_que was never called
                g_jobQue.reset();
            }
        }
    protected:

        // bool needShutdown = false;
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_JobQue,das);

