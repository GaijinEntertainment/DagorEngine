#pragma once

#include "daScript/misc/job_que.h"
#include "daScript/simulate/simulate.h"
#include "aot.h"

#include <queue>
#include <vector>

namespace das {

    struct DAS_API Feature {
        // tracking
        uint64_t            fTrackId = 0;
        Feature *           fTrackNext = nullptr;
        Feature *           fTrackPrev = nullptr;
        string              fCreatedAt;
        JobStatus *         fOwner = nullptr;
        uint64_t            fOwnerTrackId = 0;
        static Feature *    sTrackHead;
        static mutex        sTrackMutex;
        static void DumpFeatures();
        void trackInsert();
        void trackRemove();
        // data
        void *              data = nullptr;
        TypeInfo *          type = nullptr;
        Context *           from = nullptr;
        shared_ptr<Context> fromShared;
        Feature();
        Feature ( void * d, TypeInfo * ti, Context * c );
        ~Feature();
        Feature ( const Feature & f );
        Feature ( Feature && f );
        Feature & operator = ( const Feature & f );
        Feature & operator = ( Feature && f );
        void setFrom ( Context * c );
        void clear();
    };

    class LockBox : public JobStatus {
    public:
        LockBox() { mTrackMagic = TRACK_LOCKBOX; }
        virtual ~LockBox();
        void set ( void * data, TypeInfo * ti, Context * context );
        void get ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
        void update ( const TBlock<void *,void *> & blk, TypeInfo * ti, Context * context, LineInfoArg * at );
        void fill ( void * data, TypeInfo * ti, Context * context );
        void grab ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    public:
        template <typename TT>
        void peek ( TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            if ( box.data ) {
                tt(box.data, box.type, box.from);
            }
        }
    protected:
        Feature box;
    };

    template <typename TT>
    class AtomicTT {
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
    using Atomic32 = AtomicTT<int32_t>;
    using Atomic64 = AtomicTT<int64_t>;

    class DAS_API Channel : public JobStatus {
    public:
        Channel( Context * ctx ) : owner(ctx) { mTrackMagic = TRACK_CHANNEL; }
        Channel( Context * ctx, int count) : owner(ctx) { mTrackMagic = TRACK_CHANNEL; mRemaining = count; }
        virtual ~Channel();
        void push ( void * data, TypeInfo * ti, Context * context );
        void pushBatch ( void ** data, int count, TypeInfo * ti, Context * context );
        void pop ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
        bool tryPop ( const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
        bool popWithTimeout ( int timeoutMs, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
        bool isEmpty() const;
        int total() const;
        Context * getOwner() { return owner; }
    public:
        template <typename TT>
        void for_each_item ( TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto & f : pipe ) {
                tt(f.data, f.type, f.from ? f.from : owner);
            }
        }
        template <typename TT>
        void gather ( TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto & f : pipe ) {
                tt(f.data, f.type, f.from ? f.from : owner);
            }
            pipe.clear();
        }
        template <typename TT>
        void gatherEx ( Context * ctx, TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto f = pipe.begin(); f != pipe.end(); ) {
                auto itOwner = f->from ? f->from : owner;
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
                tt(f.data, f.type, f.from ? f.from : owner);
            }
            lock_guard<mutex> guard2(that->mCompleteMutex);
            for ( auto & f : pipe ) {
                that->pipe.emplace_back(das::move(f));
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

    class DAS_API Stream : public JobStatus {
    public:
        Stream() { mTrackMagic = TRACK_STREAM; }
        Stream( int count ) { mTrackMagic = TRACK_STREAM; mRemaining = count; }
        virtual ~Stream();
        void push ( const uint8_t * data, uint32_t size );
        void pushBatch ( const uint8_t * const * data, const uint32_t * sizes, int count );
        void pop ( const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at );
        bool tryPop ( const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at );
        bool popWithTimeout ( int timeoutMs, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at );
        bool isEmpty() const;
        int  total() const;
    public:
        template <typename TT>
        void for_each_item ( TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto & v : pipe ) {
                Array arr;
                array_mark_locked(arr, (void *)v.data(), (uint32_t)v.size());
                tt(&arr);
            }
        }
        template <typename TT>
        void gather ( TT && tt ) {
            lock_guard<mutex> guard(mCompleteMutex);
            for ( auto & v : pipe ) {
                Array arr;
                array_mark_locked(arr, (void *)v.data(), (uint32_t)v.size());
                tt(&arr);
            }
            pipe.clear();
        }
    protected:
        uint32_t                mSleepMs = 1;
        deque<vector<uint8_t>>  pipe;
    };

    DAS_API bool is_job_que_shutting_down();
    DAS_API void new_job_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    DAS_API void new_thread_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    DAS_API void withJobQue ( const TBlock<void> & block, Context * context, LineInfoArg * lineInfo );
    DAS_API int getTotalHwJobs( Context * context, LineInfoArg * at );
    DAS_API int getTotalHwThreads ();
    DAS_API void withJobStatus ( int32_t total, const TBlock<void,JobStatus *> & block, Context * context, LineInfoArg * lineInfo );
    DAS_API void jobStatusAddRef ( JobStatus * status, Context * context, LineInfoArg * at );
    DAS_API void jobStatusReleaseRef ( JobStatus * & status, Context * context, LineInfoArg * at );
    DAS_API JobStatus * jobStatusCreate( Context * context, LineInfoArg * );
    DAS_API void jobStatusRemove( JobStatus * & ch, Context * context, LineInfoArg * at );
    DAS_API void waitForJob ( JobStatus * status, Context * context, LineInfoArg * at );
    DAS_API void notifyJob ( JobStatus * status, Context * context, LineInfoArg * at );
    DAS_API void notifyAndReleaseJob ( JobStatus * & status, Context * context, LineInfoArg * at );
    DAS_API vec4f channelPush ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API vec4f channelPushBatch ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API void channelPop ( Channel * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at );
    DAS_API bool channelTryPop ( Channel * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at );
    DAS_API bool channelPopWithTimeout ( Channel * ch, int32_t timeoutMs, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at );
    DAS_API int jobAppend ( JobStatus * ch, int size, Context * context, LineInfoArg * at );
    DAS_API void withChannel ( const TBlock<void,Channel *> & blk, Context * context, LineInfoArg * lineinfo );
    DAS_API void withChannelEx ( int32_t count, const TBlock<void,Channel *> & blk, Context * context, LineInfoArg * lineinfo );
    DAS_API Channel* channelCreate( Context * context, LineInfoArg * at);
    DAS_API void channelRemove(Channel * & ch, Context * context, LineInfoArg * at);
    DAS_API void channelGather ( Channel * ch, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    DAS_API void channelGatherEx ( Channel * ch, const TBlock<void,void *,const TypeInfo *,Context &> & blk, Context * context, LineInfoArg * at );
    DAS_API void channelGatherAndForward ( Channel * ch, Channel * toCh, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    DAS_API void channelPeek ( Channel * ch, const TBlock<void,void *> & blk, Context * context, LineInfoArg * at );
    DAS_API void channelVerify ( Channel * ch, Context * context, LineInfoArg * at );
    DAS_API Stream * streamCreate ( Context * context, LineInfoArg * at );
    DAS_API void streamRemove ( Stream * & ch, Context * context, LineInfoArg * at );
    DAS_API void withStream ( const TBlock<void, Stream *> & blk, Context * context, LineInfoArg * at );
    DAS_API void withStreamEx ( int32_t count, const TBlock<void, Stream *> & blk, Context * context, LineInfoArg * at );
    DAS_API void streamPush ( Stream * ch, const TArray<uint8_t> & data, Context * context, LineInfoArg * at );
    DAS_API void streamPushBatch ( Stream * ch, const TArray<TArray<uint8_t>> & data, Context * context, LineInfoArg * at );
    DAS_API void streamPop ( Stream * ch, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at );
    DAS_API bool streamTryPop ( Stream * ch, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at );
    DAS_API bool streamPopWithTimeout ( Stream * ch, int32_t timeoutMs, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at );
    DAS_API void streamGather ( Stream * ch, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at );
    DAS_API void streamPeek ( Stream * ch, const TBlock<void, TTemporary<TArray<uint8_t> const>> & blk, Context * context, LineInfoArg * at );
    DAS_API LockBox * lockBoxCreate( Context *, LineInfoArg * );
    DAS_API void lockBoxRemove( LockBox * & ch, Context * context, LineInfoArg * at );
    DAS_API void withLockBox ( const TBlock<void,LockBox *> & blk, Context * context, LineInfoArg * at );
    DAS_API vec4f lockBoxSet ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API void lockBoxGet ( LockBox * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at );
    DAS_API vec4f lockBoxFill ( Context & context, SimNode_CallBase * call, vec4f * args );
    DAS_API void lockBoxGrab ( LockBox * ch, const TBlock<void,void*> & blk, Context * context, LineInfoArg * at );
    DAS_API void lockBoxUpdate ( LockBox * ch, TypeInfo * ti, const TBlock<void *,void*> & blk, Context * context, LineInfoArg * at );

    template <typename TT>
    AtomicTT<TT> * atomicCreate( Context *, LineInfoArg * ) {
        auto ch = new AtomicTT<TT>();
        ch->set(0);
        return ch;
    }

    template <typename TT>
    void atomicRemove( AtomicTT<TT> * & ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomicRemove: atomic is null");
        delete ch;
        ch = nullptr;
    }

    template <typename TT>
    void withAtomic ( const TBlock<void,AtomicTT<TT> *> & blk, Context * context, LineInfoArg * at ) {
        AtomicTT<TT> ch;
        ch.set(0);
        das::das_invoke<void>::invoke<AtomicTT<TT> *>(context, at, blk, &ch);
    }

    template <typename TT>
    TT atomicGet ( AtomicTT<TT> * ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomic is null");
        return ch->get();
    }

    template <typename TT>
    void atomicSet ( AtomicTT<TT> * ch, TT val, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomic is null");
        ch->set(val);
    }

    template <typename TT>
    TT atomicInc ( AtomicTT<TT> * ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomic is null");
        return ch->inc();
    }

    template <typename TT>
    TT atomicDec ( AtomicTT<TT> * ch, Context * context, LineInfoArg * at ) {
        if ( !ch ) context->throw_error_at(at, "atomic is null");
        return ch->dec();
    }
}
