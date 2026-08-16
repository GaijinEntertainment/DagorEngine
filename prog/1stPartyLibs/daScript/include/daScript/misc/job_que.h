#pragma once

#include <condition_variable>
#include <deque>
#include <thread>
#include <atomic>

namespace das {
    struct LineInfo;

    // shared tracking counter for JobStatus and Feature
    DAS_API extern atomic<uint64_t> g_jobque_track_total;
    DAS_API extern atomic<uint64_t> g_jobque_track_id;       // ID to detail-trace (0 = none)
    // single job
    typedef function<void()> Job;
    typedef function<void(int,int)> JobChunk;

    typedef uint32_t JobCategory;

    enum class JobPriority : int32_t {
        Inactive = 0x66,                    // just some high number to indicate the thread is inactive
        Minimum = -2,
        Low = -1,
        Medium = 0,
        High = 1,
        Maximum = 2,
        Default = Medium,
        Realtime = High,
    };

    class DAS_API JobStatus {
    public:
        enum { STATUS_MAGIC = 0xdeadbeef };
        JobStatus();
        JobStatus(uint32_t count);
        JobStatus ( JobStatus && ) = delete;
        JobStatus ( const JobStatus & ) = delete;
        virtual ~JobStatus();
        JobStatus & operator = ( JobStatus && ) = delete;
        JobStatus & operator = ( const JobStatus & ) = delete;
        bool Notify();
        bool NotifyAndRelease( LineInfo * at = nullptr );
        bool isReady();
        void Wait();
        void Clear(uint32_t count = 1);
        int addRef( LineInfo * at = nullptr );
        int releaseRef( LineInfo * at = nullptr );
        int size() const;
        int append(int size);
        bool isValid() const { return mMagic==uint32_t(STATUS_MAGIC); }
    // tracking
    public:
        uint64_t        mTrackId = 0;
        JobStatus *     mTrackNext = nullptr;
        JobStatus *     mTrackPrev = nullptr;
        uint32_t        mTrackMagic = 0;
        string          mCreatedAt;
        static JobStatus *  sTrackHead;
        static mutex        sTrackMutex;
        static constexpr uint32_t TRACK_JOB_STATUS = 0xDA5CA001;
        static constexpr uint32_t TRACK_CHANNEL    = 0xDA5CA002;
        static constexpr uint32_t TRACK_LOCKBOX    = 0xDA5CA003;
        static constexpr uint32_t TRACK_STREAM     = 0xDA5CA004;
        static constexpr uint32_t TRACK_SEQBOX     = 0xDA5CA005;
        // Join poll level (0 = park on the condvar immediately, the default). Wait() polls
        // mRemaining for level*1024*128 relax-rounds before blocking — the ggml hybrid-poll shape
        // and denomination (their --poll 0..100; 50 = their default ≈ 6.5M rounds). With the
        // calling thread executing parallel_for chunks itself, the join wait is just the workers'
        // tail — the poll removes the last per-fork OS park/wake. Global (all wait groups),
        // opt-in via set_jobque_join_spin: polling burns idle CPU.
        static atomic<int32_t> sJoinSpin;
        static void DumpJobQueLeaks();
        static uint64_t CountJobQueLeaks();
        void trackEvent ( LineInfo * at, bool isAddRef );
    protected:
        void trackInsert();
        void trackRemove();
    protected:
        mutable mutex       mCompleteMutex;
        atomic<uint32_t>    mRemaining{0};  // atomic so Wait()'s spin phase can poll lock-free; all writes stay under mCompleteMutex for the condvar protocol
        condition_variable  mCond;
        atomic<int>         mRef{0};
        uint32_t            mMagic = uint32_t(STATUS_MAGIC);
    };

    class JobQue {
    public:
        JobQue();
        JobQue ( const JobQue & ) = delete;
        JobQue ( JobQue && ) = delete;
        JobQue & operator = ( const JobQue & ) = delete;
        JobQue & operator = ( JobQue && ) = delete;
        ~JobQue ();
        bool isEmpty (bool includingMainThreadJobs = false);
        // Wait until nothing is queued or running, up to timeoutMs (0 = wait forever). Returns false
        // on timeout. join() does NOT do this -- it flags shutdown and waits for the THREADS, and a
        // worker tests that flag before the fifo, so queued-but-unstarted jobs are discarded.
        bool drain ( int timeoutMs );
        bool areJobsPending(JobCategory category);
        int getNumberOfQueuedJobs();
        int getTotalHwJobs();
        void push(Job && job, JobCategory category, JobPriority priority);
        void pushBatch(vector<Job> && jobs, JobCategory category, JobPriority priority);   // one lock hold + one notify for the whole batch (workers wake-propagate, see JobQue::job)
        bool tryRunOneJob();    // pop one queued job and execute it on the CALLING thread (dispatcher work-stealing at a fork/join); false when the fifo is empty
        void parallel_for ( JobStatus & status, int from, int to, const JobChunk & chunk, JobCategory category, JobPriority priority, int chunk_count = -1, int step = 1 );
        void parallel_for ( int from, int to, const JobChunk & chunk, JobCategory category, JobPriority priority, int chunk_count = -1, int step = 1 );
        void parallel_for_with_consume (int from, int to, const JobChunk & chunk, const JobChunk & consume, JobCategory category, JobPriority priority, int chunk_count = -1, int step = 1);
        static int get_num_threads();
        static int get_num_physical_cores();            // physical cores (SMT siblings collapsed); logical count where topology is unknown
        static void set_default_threads_cap(int cap);   // cap the DEFAULT worker count of a future JobQue (min with the stock rule); 0 = off. DAS_JOBQUE_THREADS still overrides
        static int get_default_threads_cap();
        static void set_default_affinity(int mode);     // affinity mode of a future JobQue: 0 off / 1 ideal-CPU hint / 2 hard mask (-1 = unset). DAS_JOBQUE_AFFINITY still overrides
        static int get_default_affinity();
        void EvalOnMainThread(Job && expr);
        void EvalMainThreadJobs();
        void wait();
        void Reset() { wait( ); }
        // Worker spin-before-park window, microseconds (0 = park on the condvar immediately, the
        // default). An idle worker spin-polls the fifo for this long before blocking, so a
        // fork/join burst (e.g. an LLM decode token's back-to-back matmuls) never pays the OS
        // thread-wake per job — measured ~3-4.5us per notify_one to a PARKED worker vs ~0.1us to a
        // spinning one, paid serially by the DISPATCHING thread. Opt-in: spinning burns idle CPU.
        void setWorkerSpin ( int usec ) { mSpinUs = usec; }
        // Team mode (opt-in, sticky): alongside the fifo, workers poll a single published team op
        // and self-serve chunks off one atomic (the ggml threadpool shape). teamParallelFor
        // replaces the whole per-chunk fifo path — no push, no mutex, no wait group, no condvar —
        // with one seq bump per dispatch; join is a spin on a remaining counter. Hybrid poll/park
        // like ggml's --poll: workers spin only for the setWorkerSpin window, then park; a publish
        // that finds parked workers takes the fifo mutex and notifies (zero mutex traffic while
        // everyone spins). Pair with setWorkerSpin — with a 0 window every publish pays the wake.
        // Concurrent callers are serialized at the published-operation boundary. The worker team
        // still executes each operation in parallel; only one dispatcher may own the shared
        // publish slots at once.
        void setTeamMode ( bool on );
        bool getTeamMode () const { return mTeamMode.load(std::memory_order_relaxed) != 0; }
        // Worker-pool limit (runtime dial): workers with threadIndex >= limit go DORMANT — they
        // drift out of the spin loop at the next check and park WITHOUT joining the publish wake
        // gate (no mParkedWorkers bump), so team publishes and fifo pushes never wake them; only
        // setWorkerLimit raising the limit (or shutdown) does. A woken worker re-checks the limit
        // before rejoining, so a wake racing a lower re-parks it dormant. Chunks are self-served
        // and the caller drains solo if need be, so ANY limit (0 included) completes every
        // dispatch — this is a performance dial (a tiny model's inline-dominated token runs
        // faster with fewer awake workers), never a correctness knob. Negative = unlimited.
        void setWorkerLimit ( int n );
        int getWorkerLimit () const { return mWorkerLimit.load(std::memory_order_relaxed); }
        int getNumWorkers () const { return mThreadCount.load(); }
        // Per-op worker participation gate (opt-in: DAS_JOBQUE_TEAM_RANK_GATE=1 or this setter):
        // a team op admits only as many workers as its widest stage has chunks — limit-rank r
        // serves only when r + 1 < maxChunks (the +1 reserves a lane for the caller, who always
        // serves). Higher ranks consume the op's seq and keep spinning: they never touch the
        // claim words, so a tiny op's rendezvous involves only the workers it can feed, and the
        // next big op re-admits everyone at the cost of one relaxed load — no dormant kernel
        // wake on the raise path (the asymmetry that makes a per-op setWorkerLimit unviable).
        // Purely a performance dial: chunks self-serve and the caller drains, so any admit set
        // completes every dispatch. Composes with setWorkerLimit (dormant stays dormant).
        void setTeamRankGate ( bool on ) { mTeamRankGate.store(on ? 1 : 0, std::memory_order_relaxed); }
        bool getTeamRankGate () const { return mTeamRankGate.load(std::memory_order_relaxed) != 0; }
        // Eager worker exit (default ON; DAS_JOBQUE_TEAM_EAGER_EXIT=0 or this setter opts out):
        // workers skip the FINAL stage-barrier spin and leave the in-flight window as soon as
        // their last-stage claims exhaust — the final join belongs to the caller alone, and a
        // worker parked there holds no claim. Why it matters: a worker OS-preempted in that spin
        // keeps mTeamInFlight nonzero long after the join, and the NEXT publish eats the whole
        // preemption (the measured 10-500us "fat publish" tail — A/B'd to -90% on gemma-4-E2B).
        // Intermediate barriers still spin in-flight — the claim loop doesn't re-verify the seq
        // between chunks, which is the invariant the publisher's in-flight drain protects.
        void setTeamEagerExit ( bool on ) { mTeamEagerExit.store(on ? 1 : 0, std::memory_order_relaxed); }
        bool getTeamEagerExit () const { return mTeamEagerExit.load(std::memory_order_relaxed) != 0; }
        void teamParallelFor ( int numChunks, const JobChunk & work );   // work(chunkIndex, workerSlot); caller participates as slot getNumWorkers()
        // Multi-stage team dispatch: one rendezvous, numStages dependent phases. Every lane
        // (workers + caller) serves stage s chunks, then waits on that stage's remaining counter
        // before advancing — stage s+1 may read anything stage s wrote. One publish + one join
        // for the whole chain; the inter-stage barriers are worker-side spins, so a fused
        // norm→matmul→matmul chain pays the dispatch rendezvous once instead of per op.
        static constexpr int MAX_TEAM_STAGES = 6;
        struct TeamStage {
            const JobChunk * work;      // work(chunkIndex, workerSlot)
            int numChunks;
        };
        void teamParallelForStages ( const TeamStage * stages, int numStages );
        // Programmatic access to the DAS_TEAM_PROF aggregates (below) so probes can
        // reset, run a dispatch storm, and snapshot without destroying the queue.
        // All times are ns (ref_time_ticks deltas). Set/reset from the dispatching
        // thread with no dispatch in flight — worker-side stores are gated on the
        // same unsynchronized bool the env path uses.
        struct TeamProfSnapshot {
            uint64_t ops = 0, chunks = 0, callerChunks = 0, solo = 0;
            uint64_t publishT = 0, serveT = 0, tailT = 0, totalT = 0;   // summed ns per phase
            uint64_t reactT = 0, reactN = 0, lastT = 0, lastN = 0;      // worker claim latencies
        };
        void setTeamProf ( bool on ) { mTeamProf = on; }
        void resetTeamProf ();
        TeamProfSnapshot getTeamProf () const;
        // Per-lane event tracer (opt-in): every lane records events into its OWN pre-allocated
        // ring (plain stores, no atomics on the record path) stamped with the global
        // ref_time_ticks clock, so all lanes' parallel event chains line up on one timeline.
        // traceSave writes chrome://tracing JSON (open in Perfetto / chrome://tracing): one row
        // per lane — chunks, stage waits, wakes and fifo jobs as spans; the gaps ARE the view.
        // Arm/disarm from the dispatching thread with no dispatch in flight. Ring full = that
        // lane stops recording (no wrap — the window you armed is the window you get).
        void traceStart ( int eventsPerLane );
        void traceStop () { mTraceOn.store(0, std::memory_order_seq_cst); }
        bool traceSave ( const char * path );   // false = nothing recorded or io error
        // Op tag for subsequent publishes (viewer color channel): the dispatching code stamps a
        // small app-defined id before a dispatch; ChainPublish records numStages | (tag << 8) in
        // its stage field, and workers' chunk events join to it via the chain id.
        void traceSetTag ( int tag ) { mTraceTag.store(tag, std::memory_order_relaxed); }
        // Self-describing traces: categories name the traceSetTag ids (viewer color/legend
        // channel), markers stamp instant "unit" events (frame, token) on the caller lane so
        // viewers navigate unit-to-unit. Registries persist across trace sessions; traceSave
        // emits them in a "jobqueProfile" sibling key Perfetto ignores.
        void traceCategory ( int id, const char * name, uint32_t rgb );    // rgb = 0xRRGGBB
        int  traceMarkerName ( const char * name );                        // idempotent per name
        void traceMarker ( int nameId, int arg );                          // no-op unless tracing
    protected:
        struct JobEntry {
            JobEntry( Job&& _function, JobCategory _category, JobPriority _priority) {
                function = das::move(_function);
                priority = _priority;
                category = _category;
            }
            Job                function = nullptr;
            JobPriority        priority = JobPriority::Inactive;
            JobCategory        category = 0;
        };
        struct ThreadEntry {
            ThreadEntry( unique_ptr<thread> && thread) {
                threadPointer = das::move(thread);
            };
            unique_ptr<thread>  threadPointer;
            JobPriority         currentPriority = JobPriority::Inactive;
            JobPriority         appliedPriority = JobPriority::Inactive;  // last priority actually pushed to the OS thread
            JobCategory         currentCategory = 0;
        };
    protected:
        void join();
        void job(int threadIndex);
        void submit(Job && job, JobCategory category, JobPriority priority);
        bool runTeamChunks(int threadIndex, int limitRank, uint32_t & seqSeen);
    protected:
        condition_variable mCond;
        int mSleepMs;
        atomic<bool>    mShutdown{false};
        atomic<int>     mThreadCount{0};
        static thread::id mTheMainThread;
        mutex mFifoMutex;
    protected:
        deque<JobEntry> mFifo;
        vector<ThreadEntry> mThreads;
        atomic<int> mJobsRunning{0};
        atomic<int32_t> mSpinUs{0};
        atomic<int32_t> mFifoCount{0};  // lock-free mirror of mFifo.size() for the spin poll
    protected:
        // team-mode publish slots, one per stage. Publish protocol (seqlock + entrant gate):
        // the dispatcher bumps mTeamSeq to ODD ("publishing"), waits for mTeamInFlight == 0,
        // writes work/remaining/op words for every stage, then bumps mTeamSeq to EVEN. Workers
        // enter with mTeamInFlight++ then re-verify the (even) seq — the seq_cst store/load pairs
        // on both sides make it a Dekker: either the publisher sees the entrant and waits, or the
        // entrant sees the odd/new seq and backs out. So no lane can hold a stage claim word
        // while it is rewritten — which is what makes the INTER-CHAIN stage-order invariant hold
        // (a straggler can never claim a later chain's stage-s chunk before that chain's stage
        // s-1 completed, because it can't be inside the chain loop across a publish at all).
        // Claim words still pack {numChunks:32 | counter:32} so a claim carries its stage's chunk
        // count with the index (overflow self-describes, the single-op epoch-race fix). A valid
        // claim's pending remaining-decrement blocks that stage's barrier, which keeps
        // mTeamWorkS (the dispatcher's stack) alive while dereferenced.
        atomic<int32_t>  mTeamMode{0};
        mutex            mTeamDispatchMutex;
        atomic<uint32_t> mTeamSeq{0};
        atomic<int32_t>  mTeamInFlight{0};
        int              mTeamStageCount = 0;
        atomic<uint64_t> mTeamOpS[MAX_TEAM_STAGES] = {};
        atomic<int32_t>  mTeamRemainingS[MAX_TEAM_STAGES] = {};
        const JobChunk * mTeamWorkS[MAX_TEAM_STAGES] = {};
        // workers currently in cond_wait — the publish-side wake gate. Dekker pair with mTeamSeq
        // (worker: parked++ then read seq; publisher: bump seq then read parked — all four seq_cst),
        // so either the worker sees the new op before sleeping or the publisher sees it parked.
        atomic<int32_t>  mParkedWorkers{0};
        // worker-pool limit (see setWorkerLimit): limit-rank >= this ⇒ dormant. Dormant workers
        // are deliberately NOT in mParkedWorkers — the publish wake gate must skip them — and
        // they park on their OWN condvar: sharing mCond would let a dormant waiter absorb a
        // push()/wake-propagation notify_one meant for an eligible parked worker (the token is
        // consumed, the predicate re-sleeps it, the fifo job stalls), and every team-publish
        // notify_all would kernel-wake the whole dormant pool just to re-sleep it. Only
        // setWorkerLimit raising the limit (and shutdown) notifies mLimitCond.
        atomic<int32_t>  mWorkerLimit{0x7fffffff};
        condition_variable mLimitCond;   // dormant workers wait here (under mFifoMutex)
        // worker-limit eligibility order (fixed at construction, DAS_JOBQUE_LIMIT_ORDER=spread):
        // prefix (default) gates on the raw threadIndex; spread ranks workers along a
        // golden-stride visit so every runtime limit L selects a near-uniform lattice over the
        // creation order — which the OS's round-robin ideal-processor assignment maps loosely
        // onto physical cores (CCD/CCX spread on chiplet parts).
        bool             mLimitOrderSpread = false;
        // per-op worker participation gate (see setTeamRankGate)
        atomic<int32_t>  mTeamRankGate{0};
        atomic<int32_t>  mTeamEagerExit{1};
        // team dispatch anatomy profiler (env DAS_TEAM_PROF=1): per-op phase aggregates, dumped
        // at queue destruction. Caller-side counters except the two claim-timestamp slots;
        // worker stores happen only under the flag (they perturb the op being measured).
        bool mTeamProf = false;
        atomic<uint64_t> mTeamFirstClaimT{0};   // current op: first worker claim ts (0 = none yet)
        atomic<uint64_t> mTeamLastClaimT{0};    // current op: last worker claim ts
        uint64_t mTPn = 0, mTPchunks = 0, mTPcallerChunks = 0, mTPsolo = 0;
        uint64_t mTPpub = 0, mTPserve = 0, mTPtail = 0, mTPtotal = 0;
        uint64_t mTPreact = 0, mTPreactN = 0, mTPlastRel = 0, mTPlastN = 0;
        // per-lane trace rings (see traceStart). lane == worker threadIndex; the dispatching
        // caller records as lane getNumWorkers().
        enum class TraceTag : uint32_t { ChainPublish, Chunk, StageWait, Wake, FifoJob, Marker };
        struct TraceEvent {
            uint64_t t0, t1;        // ref_time_ticks (ns, one clock for every lane)
            uint32_t tag, stage;    // TraceTag; stage index (Chunk/StageWait) or stage count (ChainPublish)
            uint32_t arg, chain;    // chunk index / total chunks / parked flag; team seq of the chain
        };
        struct LaneTrace {
            vector<TraceEvent> ev;
            uint32_t n = 0;
            char pad[64];           // keep the hot write index off the neighbor lane's cache line
        };
        atomic<int32_t> mTraceOn{0};
        atomic<int32_t> mTraceTag{0};   // current op tag (traceSetTag) — folded into ChainPublish.stage's high bits
        uint64_t mTraceT0 = 0;          // session start tick — wake spans clamp their park-t0 to it
        vector<LaneTrace> mTrace;
        struct TraceCategory { int id; string name; uint32_t rgb; };
        mutex mTraceRegMutex;                    // guards both registries (cold path only)
        vector<TraceCategory> mTraceCategories;
        vector<string> mTraceMarkerNames;        // marker nameId = index
        __forceinline void traceEvent ( int lane, TraceTag tag, uint64_t t0, uint64_t t1,
                uint32_t stage, uint32_t arg, uint32_t chain ) {
            if ( uint32_t(lane) >= uint32_t(mTrace.size()) ) return;
            auto & L = mTrace[lane];
            if ( L.n >= uint32_t(L.ev.size()) ) return;
            L.ev[L.n++] = { t0, t1, uint32_t(tag), stage, arg, chain };
        }
    protected:
        mutex mEvalMainThreadMutex;
        vector<Job> mEvalMainThread;
    };

    void SetCurrentThreadName ( const string & str );
    void SetCurrentThreadPriority ( JobPriority priority );             // change priority of current thread, 0 - normal, >0 above normal, <0 below normal
    void SetCurrentThreadAffinityCpu ( int cpu, bool hard );            // bind current thread to a logical CPU: hard mask, or scheduler hint (Windows ideal processor); no-op where unsupported
}
