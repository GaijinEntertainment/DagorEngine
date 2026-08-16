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

    // Lock-free snapshot box: one writer publishes a POD value, any number of readers copy the
    // latest one. Where LockBox holds a mutex for the whole duration of the block it invokes, this
    // is a seqlock — the writer bumps a version, stores the payload and bumps again; a reader that
    // catches a write in progress retries. Neither side can ever wait for the other, which is what
    // makes it safe to publish from a realtime thread (an audio mix callback) that must never
    // block on a game thread.
    //
    // The value is copied byte-wise, so it must be POD with no heap references, and both sides
    // must agree on its type — the box validates only the size. Payload words are atomic so the
    // deliberate read/write race the version resolves is not also a data race under TSAN.
    class SeqBox : public JobStatus {
    public:
        enum { PAYLOAD_WORDS = 8, PAYLOAD_BYTES = PAYLOAD_WORDS * 8 };
        SeqBox() { mTrackMagic = TRACK_SEQBOX; }
        virtual ~SeqBox() {}
        // The odd/even counter is a seqlock, which admits exactly one writer: two writers bumping it
        // concurrently can leave it EVEN while the payload is half-written, and a reader accepts that
        // as a stable snapshot. So a writer CLAIMS the box with a CAS from even to odd instead.
        // publish never waits for the claim - it is the realtime side, and a dropped status snapshot
        // is replaced by the next one a block later, which is cheaper than any wait.
        bool publish ( const void * data, uint32_t size ) {
            if ( !size || size > PAYLOAD_BYTES ) return false;
            uint64_t words[PAYLOAD_WORDS] = {};
            memcpy(words, data, size);
            uint32_t s0 = mSeq.load(std::memory_order_acquire);
            if ( (s0 & 1u) || !mSeq.compare_exchange_strong(s0, s0+1,
                    std::memory_order_acq_rel, std::memory_order_relaxed) ) return false;  // odd: writing
            for ( uint32_t w=0; w!=PAYLOAD_WORDS; ++w ) mPayload[w].store(words[w], std::memory_order_relaxed);
            mSize.store(size, std::memory_order_relaxed);
            mSeq.store(s0+2, std::memory_order_release);        // even: stable
            return true;
        }
        bool read ( void * out, uint32_t size ) const {
            if ( !size || size > PAYLOAD_BYTES ) return false;
            // The write is a handful of stores, so needing more than a couple of retries is already
            // pathological; give up rather than spin unboundedly on a caller's thread.
            for ( int attempt=0; attempt!=16; ++attempt ) {
                uint32_t s0 = mSeq.load(std::memory_order_acquire);
                if ( s0 & 1u ) continue;
                if ( mSize.load(std::memory_order_relaxed)!=size ) return false;    // empty, or another type
                uint64_t words[PAYLOAD_WORDS];
                for ( uint32_t w=0; w!=PAYLOAD_WORDS; ++w ) words[w] = mPayload[w].load(std::memory_order_relaxed);
                std::atomic_thread_fence(std::memory_order_acquire);
                if ( mSeq.load(std::memory_order_relaxed)!=s0 ) continue;
                memcpy(out, words, size);
                return true;
            }
            return false;
        }
        // clear is the control side, so it waits for the claim rather than dropping the request. The
        // wait is bounded in practice: a claim is held for a handful of stores and never across a call.
        void clear () {
            for ( ;; ) {
                uint32_t s0 = mSeq.load(std::memory_order_acquire);
                if ( s0 & 1u ) continue;
                if ( mSeq.compare_exchange_weak(s0, s0+1,
                        std::memory_order_acq_rel, std::memory_order_relaxed) ) {
                    mSize.store(0, std::memory_order_relaxed);
                    mSeq.store(s0+2, std::memory_order_release);
                    return;
                }
            }
        }
        bool hasValue () const { return mSize.load(std::memory_order_acquire)!=0; }
    protected:
        mutable atomic<uint32_t> mSeq { 0 };
        atomic<uint32_t> mSize { 0 };
        atomic<uint64_t> mPayload[PAYLOAD_WORDS] = {};
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
        // gather/gatherEx/gather_and_forward drain the pipe under the lock, then run
        // the callback OUTSIDE it. The callback is arbitrary user code — it may push
        // to or query this very channel (reentrancy), and it may be arbitrarily slow
        // (the wasm AudioWorklet interprets the audio command ladder here; running it
        // under the lock starved the main thread's per-frame push into a dead page).
        // Items pushed during a gather join the next drain, never the current one.
        template <typename TT>
        void gather ( TT && tt ) {
            decltype(pipe) drained;
            {
                lock_guard<mutex> guard(mCompleteMutex);
                drained.swap(pipe);
            }
            for ( auto & f : drained ) {
                tt(f.data, f.type, f.from ? f.from : owner);
            }
        }
        template <typename TT>
        void gatherEx ( Context * ctx, TT && tt ) {
            decltype(pipe) drained;
            {
                lock_guard<mutex> guard(mCompleteMutex);
                for ( auto f = pipe.begin(); f != pipe.end(); ) {
                    auto itOwner = f->from ? f->from : owner;
                    if ( itOwner == ctx ) {
                        drained.emplace_back(das::move(*f));
                        f = pipe.erase(f);
                    } else {
                        ++f;
                    }
                }
            }
            for ( auto & f : drained ) {
                tt(f.data, f.type, f.from ? f.from : owner);
            }
        }
        template <typename TT>
        void gather_and_forward ( Channel * that, TT && tt ) {
            decltype(pipe) drained;
            {
                lock_guard<mutex> guard(mCompleteMutex);
                drained.swap(pipe);
            }
            for ( auto & f : drained ) {
                tt(f.data, f.type, f.from ? f.from : owner);
            }
            {
                lock_guard<mutex> guard2(that->mCompleteMutex);
                for ( auto & f : drained ) {
                    that->pipe.emplace_back(das::move(f));
                }
            }
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
        void push ( const uint8_t * data, uint64_t size );
        void pushBatch ( const uint8_t * const * data, const uint64_t * sizes, int64_t count );
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
                array_mark_locked(arr, (void *)v.data(), v.size());
                tt(&arr);
            }
        }
        // Same drain-first rule as Channel::gather above: the callback (the audio
        // command ladder, arbitrary user code) runs OUTSIDE the lock and may push
        // to or query this stream; items pushed during a gather join the next drain.
        template <typename TT>
        void gather ( TT && tt ) {
            decltype(pipe) drained;
            {
                lock_guard<mutex> guard(mCompleteMutex);
                drained.swap(pipe);
            }
            for ( auto & v : drained ) {
                Array arr;
                array_mark_locked(arr, (void *)v.data(), v.size());
                tt(&arr);
            }
        }
    protected:
        uint32_t                mSleepMs = 1;
        deque<vector<uint8_t>>  pipe;
    };

    DAS_API bool is_job_que_shutting_down();
    DAS_API bool is_job_que_available();
    DAS_API void set_jobque_worker_limit ( int32_t n, Context * context, LineInfoArg * at );
    DAS_API int32_t get_jobque_worker_limit ( Context * context, LineInfoArg * at );
    DAS_API void set_jobque_team_rank_gate ( bool on, Context * context, LineInfoArg * at );
    DAS_API bool get_jobque_team_rank_gate ( Context * context, LineInfoArg * at );
    DAS_API uint64_t count_jobque_leaks();
    DAS_API void new_job_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    DAS_API void set_jobque_fork_pool ( bool keep, bool skipInit, Context * context, LineInfoArg * at );
    DAS_API void set_jobque_fork_skip_heap_reset ( bool skip, Context * context, LineInfoArg * at );
    DAS_API void set_jobque_worker_spin ( int32_t usec, Context * context, LineInfoArg * at );
    DAS_API void set_jobque_batch_dispatch ( bool batch, Context * context, LineInfoArg * at );
    DAS_API void flush_jobque_batch ( Context * context, LineInfoArg * at );
    DAS_API void set_jobque_join_spin ( int32_t level, Context * context, LineInfoArg * at );
    DAS_API void set_jobque_team_mode ( bool on, Context * context, LineInfoArg * at );
    DAS_API bool get_jobque_team_mode ( Context * context, LineInfoArg * at );
    DAS_API void set_jobque_thread_team_mode ( bool on, Context * context, LineInfoArg * at );
    DAS_API bool get_jobque_thread_team_mode ( Context * context, LineInfoArg * at );
    DAS_API void set_jobque_team_prof ( bool on, Context * context, LineInfoArg * at );
    DAS_API void reset_jobque_team_prof ( Context * context, LineInfoArg * at );
    DAS_API float4 get_jobque_team_prof ( Context * context, LineInfoArg * at );
    DAS_API int4 get_jobque_team_prof_counts ( Context * context, LineInfoArg * at );
    DAS_API float2 get_jobque_team_prof_react ( Context * context, LineInfoArg * at );
    DAS_API void jobque_trace_start ( int32_t eventsPerLane, Context * context, LineInfoArg * at );
    DAS_API void jobque_trace_stop ( Context * context, LineInfoArg * at );
    DAS_API bool jobque_trace_save ( const char * path, Context * context, LineInfoArg * at );
    DAS_API void jobque_trace_tag ( int32_t tag, Context * context, LineInfoArg * at );
    DAS_API void jobque_trace_category ( int32_t id, const char * name, uint32_t color, Context * context, LineInfoArg * at );
    DAS_API int32_t jobque_trace_marker_name ( const char * name, Context * context, LineInfoArg * at );
    DAS_API void jobque_trace_marker ( int32_t id, int32_t arg, Context * context, LineInfoArg * at );
    DAS_API void jobque_set_thread_priority ( int32_t level, Context * context, LineInfoArg * at );
    DAS_API void team_parallel_for_invoke ( int32_t rangeBegin, int32_t rangeEnd, int32_t numChunks, Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    DAS_API void team_parallel_for_indexed_invoke ( int32_t rangeBegin, int32_t rangeEnd, int32_t numChunks, Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    DAS_API void team_parallel_stages_invoke ( const TArray<int3> & stages, Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    DAS_API bool jobque_try_run_one ( Context * context, LineInfoArg * at );
    DAS_API void new_thread_invoke ( Lambda lambda, Func fn, int32_t lambdaSize, Context * context, LineInfoArg * lineinfo );
    DAS_API void withJobQue ( const TBlock<void> & block, Context * context, LineInfoArg * lineInfo );
    DAS_API void createJobQue ( Context * context, LineInfoArg * lineInfo );
    DAS_API void destroyJobQue ( Context * context, LineInfoArg * lineInfo );
    DAS_API int getTotalHwJobs( Context * context, LineInfoArg * at );
    DAS_API int getTotalHwThreads ();
    DAS_API int getTotalHwCores ();
    DAS_API void setJobqueThreadsCap ( int32_t cap );
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
    DAS_API SeqBox * seqBoxCreate( Context *, LineInfoArg * );
    DAS_API void seqBoxRelease( SeqBox * & box, Context * context, LineInfoArg * at );
    DAS_API void withSeqBox ( const TBlock<void,SeqBox *> & blk, Context * context, LineInfoArg * at );
    DAS_API bool seqBoxPublish ( SeqBox * box, void * data, int32_t size, Context * context, LineInfoArg * at );
    DAS_API bool seqBoxRead ( SeqBox * box, void * out, int32_t size, Context * context, LineInfoArg * at );
    DAS_API void seqBoxClear ( SeqBox * box, Context * context, LineInfoArg * at );

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
