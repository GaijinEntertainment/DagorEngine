#pragma once

#include "daScript/misc/job_que.h"
#include "daScript/simulate/simulate.h"
#include "aot.h"

#include <queue>

namespace das {

    struct Feature {
        void *              data = nullptr;
        TypeInfo *          type = nullptr;
        shared_ptr<Context> from;
        Feature() {}
        __forceinline Feature ( void * d, TypeInfo * ti, Context * c) : data(d), type(ti), from(c ? c->shared_from_this() : nullptr) {}
        __forceinline void clear() {
            data = nullptr;
            type = nullptr;
            from.reset();
        }
    };

    class LockBox : public JobStatus {
    public:
        LockBox() {}
        virtual ~LockBox();
        void set ( void * data, TypeInfo * ti, Context * context );
        void get ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
        void update ( const TBlock<void *,void *> & blk, TypeInfo * ti, Context * context, LineInfoArg * at );
    public:
        template <typename TT>
        void peek ( TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            if ( box.data ) {
                tt(box.data, box.type, box.from.get());
            }
        }
    protected:
        Feature box;
    };

    template <typename TT>
    class AtomicTT : public JobStatus {
    public:
        TT inc () { return ++ value; }
        TT dec () { return -- value; }
        TT get () { return value.load(); }
        void set ( TT v ) { value.store(v); }
    public:
        atomic<TT>  value;
    };

    typedef AtomicTT<int32_t> AtomicInt;
    typedef AtomicTT<int64_t> AtomicInt64;

    class Channel : public JobStatus {
    public:
        Channel( Context * ctx ) : owner(ctx) {}
        Channel( Context * ctx, int count) : owner(ctx) { mRemaining = count; }
        virtual ~Channel();
        void push ( void * data, TypeInfo * ti, Context * context );
        void pushBatch ( void ** data, int count, TypeInfo * ti, Context * context );
        void pop ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
        bool isEmpty() const;
        int total() const;
        Context * getOwner() { return owner; }
    public:
        template <typename TT>
        void for_each_item ( TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto & f : pipe ) {
                tt(f.data, f.type, f.from ? f.from.get() : owner);
            }
        }
        template <typename TT>
        void gather ( TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto & f : pipe ) {
                tt(f.data, f.type, f.from ? f.from.get() : owner);
            }
            pipe.clear();
        }
        template <typename TT>
        void gatherEx ( Context * ctx, TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto f = pipe.begin(); f != pipe.end(); ) {
                auto itOwner = f->from ? f->from.get() : owner;
                if ( itOwner == ctx ) {
                    tt(f->data, f->type, itOwner);
                    f = pipe.erase(f);
                } else {
                    ++f;
                }
            }
        }
        template <typename TT>
        void gather_and_forward ( Channel * that, TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto & f : pipe ) {
                tt(f.data, f.type, f.from ? f.from.get() : owner);
            }
            lock_guard<mutex> guard2(that->mCompleteMutex);
            for ( auto & f : pipe ) {
                that->pipe.emplace_back(std::move(f));
            }
            pipe.clear();
            that->mCond.notify_all();  // notify_one??
        }
    protected:
        uint32_t            mSleepMs = 1;
        deque<Feature>      pipe;
        Feature             tail;
        Context *           owner = nullptr;
    };

    bool is_job_que_shutting_down();
    void new_job_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    void new_thread_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    void withJobQue ( const TBlock<void> & block, Context * context, LineInfoArg * lineInfo );
    int getTotalHwJobs( Context * context, LineInfoArg * at );
    int getTotalHwThreads ();
    void withJobStatus ( int32_t total, const TBlock<void,JobStatus *> & block, Context * context, LineInfoArg * lineInfo );
    void jobStatusAddRef ( JobStatus * status, Context * context, LineInfoArg * at );
    void jobStatusReleaseRef ( JobStatus * & status, Context * context, LineInfoArg * at );
    JobStatus * jobStatusCreate( Context * context, LineInfoArg * );
    void jobStatusRemove( JobStatus * & ch, Context * context, LineInfoArg * at );
    void waitForJob ( JobStatus * status, Context * context, LineInfoArg * at );
    void notifyJob ( JobStatus * status, Context * context, LineInfoArg * at );
    void notifyAndReleaseJob ( JobStatus * & status, Context * context, LineInfoArg * at );
    vec4f channelPush ( Context & context, SimNode_CallBase * call, vec4f * args );
    vec4f channelPushBatch ( Context & context, SimNode_CallBase * call, vec4f * args );
    void channelPop ( Channel * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at );
    int jobAppend ( JobStatus * ch, int size, Context * context, LineInfoArg * at );
    void withChannel ( const TBlock<void,Channel *> & blk, Context * context, LineInfoArg * lineinfo );
    void withChannelEx ( int32_t count, const TBlock<void,Channel *> & blk, Context * context, LineInfoArg * lineinfo );
    Channel* channelCreate( Context * context, LineInfoArg * at);
    void channelRemove(Channel * & ch, Context * context, LineInfoArg * at);
    void channelGather ( Channel * ch, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    void channelGatherEx ( Channel * ch, const TBlock<void,void *,const TypeInfo *,Context &> & blk, Context * context, LineInfoArg * at );
    void channelGatherAndForward ( Channel * ch, Channel * toCh, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    void channelPeek ( Channel * ch, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    void channelVerify ( Channel * ch, Context * context, LineInfoArg * at );
    LockBox * lockBoxCreate( Context *, LineInfoArg * );
    void lockBoxRemove( LockBox * & ch, Context * context, LineInfoArg * at );
    void withLockBox ( const TBlock<void,LockBox *> & blk, Context * context, LineInfoArg * at );
    vec4f lockBoxSet ( Context & context, SimNode_CallBase * call, vec4f * args );
    void lockBoxGet ( LockBox * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at );
    void lockBoxUpdate ( LockBox * ch, TypeInfo * ti, const TBlock<void *,void*> & blk, Context * context, LineInfoArg * at );

    template <typename TT>
    AtomicTT<TT> * atomicCreate( Context *, LineInfoArg * ) {
        auto ch = new AtomicTT<TT>();
        ch->addRef();
        return ch;
    }

    template <typename TT>
    void atomicRemove( AtomicTT<TT> * & ch, Context * context, LineInfoArg * at ) {
        if (!ch->isValid()) context->throw_error_at(at, "atomic is invalid (already deleted?)");
        if (ch->releaseRef()) context->throw_error_at(at, "atomic beeing deleted while being used");
        delete ch;
        ch = nullptr;
    }

    template <typename TT>
    void withAtomic ( const TBlock<void,AtomicTT<TT> *> & blk, Context * context, LineInfoArg * at ) {
        using TAtomic = AtomicTT<TT>;
        TAtomic ch;
        ch.addRef();
        das::das_invoke<void>::invoke<TAtomic *>(context, at, blk, &ch);
        if ( ch.releaseRef() ) {
            context->throw_error_at(at, "atomic box beeing deleted while being used");
        }
    }

    template <typename TT>
    TT atomicGet ( AtomicTT<TT> * ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomic is null");
        if (!ch->isValid()) context->throw_error_at(at, "atomic is invalid (already deleted?)");
        return ch->get();
    }

    template <typename TT>
    void atomicSet ( AtomicTT<TT> * ch, TT val, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomic is null");
        if (!ch->isValid()) context->throw_error_at(at, "atomic is invalid (already deleted?)");
        ch->set(val);
    }

    template <typename TT>
    TT atomicInc ( AtomicTT<TT> * ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomic is null");
        if (!ch->isValid()) context->throw_error_at(at, "atomic is invalid (already deleted?)");
        return ch->inc();
    }

    template <typename TT>
    TT atomicDec ( AtomicTT<TT> * ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomic is null");
        if (!ch->isValid()) context->throw_error_at(at, "atomic is invalid (already deleted?)");
        return ch->dec();
    }
}
