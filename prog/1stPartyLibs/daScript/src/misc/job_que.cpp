#include "daScript/misc/platform.h"

#include "daScript/misc/job_que.h"
#include "daScript/misc/performance_time.h"
#include "daScript/simulate/debug_info.h"
#include "daScript/simulate/aot_builtin_jobque.h"
#include "daScript/misc/string_writer.h"
#include "daScript/misc/env_cfg.h"

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#if defined(__EMSCRIPTEN__)
#include <emscripten/threading.h>   // emscripten_num_logical_cores
#endif

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#include <emmintrin.h>  // _mm_pause
#elif defined(_MSC_VER) && defined(_M_ARM64)
#include <intrin.h>     // __yield
#endif

namespace das {
    // A single CPU spin-wait hint — the poll-loop relax (one PAUSE/YIELD, the ggml
    // ggml_thread_cpu_relax shape; the loop itself is a counted number of these).
    static inline void jobque_spin_relax() {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
        _mm_pause();
#elif defined(_MSC_VER) && defined(_M_ARM64)
        __yield();
#elif defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#elif defined(__aarch64__)
        __asm__ __volatile__("yield");
#else
        ;
#endif
    }

    // One short burst of the CPU's spin-wait hint (worker spin-before-park, see JobQue::job).
    static inline void jobque_spin_pause() {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
        for ( int i = 0; i != 64; ++i ) _mm_pause();
#elif defined(_MSC_VER) && defined(_M_ARM64)
        for ( int i = 0; i != 64; ++i ) __yield();
#elif defined(__x86_64__) || defined(__i386__)
        for ( int i = 0; i != 64; ++i ) __builtin_ia32_pause();
#elif defined(__aarch64__)
        for ( int i = 0; i != 64; ++i ) __asm__ __volatile__("yield");
#else
        this_thread::yield();
#endif
    }
}

// Feature tracking statics
das::Feature *  das::Feature::sTrackHead = nullptr;
das::mutex      das::Feature::sTrackMutex;

namespace das {

#if defined(__APPLE__)
    // Performance ("good") core count on heterogeneous Apple Silicon (P-cores). Returns 0 on homogeneous
    // Macs (Intel) and wherever the keys are missing, so the caller falls back to the generic rule.
    static int apple_perf_core_count() {
        int nperf = 0; size_t len = sizeof(nperf);
        if ( sysctlbyname("hw.nperflevels", &nperf, &len, nullptr, 0) != 0 || nperf < 2 ) return 0;
        int good = 0; len = sizeof(good);
        if ( sysctlbyname("hw.perflevel0.logicalcpu", &good, &len, nullptr, 0) != 0 ) return 0;
        return good;
    }
#endif

    // Worker count. Generic rule: logical cores - 1, capped at DAS_MAX_HW_JOBS — parallel_for's
    // CALLING (main) thread now executes queued chunks itself (jobque_try_run_one), so it IS a
    // compute thread and occupies a core; cores-1 workers + main == cores avoids the oversubscription
    // that otherwise makes the surplus thread "trickle in" over the first ~37% of every matmul.
    // DAS_MAX_HW_JOBS (4 on wasm so a web build doesn't spawn one Web Worker per logical core;
    // effectively uncapped elsewhere). On heterogeneous Apple Silicon the same -1 applies to the
    // PERFORMANCE-core count: P-1 workers + the computing main == P. Landing any compute thread on a
    // slow E-core stalls every parallel_for on its straggler chunk (measured ~1.6x slower on an
    // M-series 8P+2E prefill vs P-only) — pre-main-steal that ruled out logical cores-1 here; now
    // that the main thread computes too, the -1 is what keeps every compute thread on a P-core.
    // DAS_JOBQUE_THREADS is an explicit override that bypasses everything (unset = the default).
    // The value is TOTAL cores, so it follows the same cores-1 rule as the default: N-1 workers +
    // the computing main == N lanes (matches llama.cpp -t N; the old "N workers + main" oversubscribed
    // by one core and left a ragged final wave when the split didn't divide N+1). Values <= 1 are a
    // FATAL ERROR: a 0-worker JobQue can't serve new_job, and the old floor-to-1-worker silently ran
    // "t=1" as 2 lanes — every baseline built on it was inflated 2x.
    static int jobque_thread_count(int hw) {
        static int forced = []{
            const char * e = get_dasenv_jobque_threads();
            if ( !e || !*e ) return 0;   // exported-but-empty behaves like unset (CI environments do this)
            int v = atoi(e);
            if ( v <= 1 ) {
                DAS_FATAL_ERROR("DAS_JOBQUE_THREADS='%s': value is TOTAL compute lanes (N-1 workers + the computing main) and must be >= 2; unset it for the default\n", e);
            }
            return v;
        }();
        if ( forced > 0 ) return forced - 1;
        int def = 0;
#if defined(__APPLE__)
        if ( int good = apple_perf_core_count() ) def = max(1, good - 1);
#endif
        if ( def == 0 ) def = max(1, min(DAS_MAX_HW_JOBS, hw - 1));
        if ( int cap = JobQue::get_default_threads_cap() ) def = max(1, min(def, cap));
        return def;
    }

    // Sticky cap on the DEFAULT worker count of a future JobQue (min with the stock rule, so it
    // can only lower it; 0 = off). For workloads that want physical cores only — SMT siblings
    // share the FMA/load ports, so lockstep linear algebra runs slower oversubscribed (dasLLAMA
    // caps to cores-1 at [init]). DAS_JOBQUE_THREADS still overrides everything.
    static atomic<int> g_jobqueDefaultThreadsCap{0};
    void JobQue::set_default_threads_cap(int cap) { g_jobqueDefaultThreadsCap = max(cap, 0); }
    int JobQue::get_default_threads_cap() { return g_jobqueDefaultThreadsCap.load(); }

    // App-declared affinity mode of a future JobQue (das set_jobque_affinity — apps expose it in
    // their config next to threads). -1 = unset (off). Applied at thread spawn, so it must be set
    // BEFORE the que exists; the DAS_JOBQUE_AFFINITY env still overrides it (the A/B rail).
    static atomic<int> g_jobqueDefaultAffinity{-1};
    void JobQue::set_default_affinity(int mode) { g_jobqueDefaultAffinity = mode; }
    int JobQue::get_default_affinity() { return g_jobqueDefaultAffinity.load(); }

#if defined(_WIN32) && !defined(_GAMING_XBOX) && !defined(_DURANGO) && !defined(_M_ARM64)
    int GetPhysicalCoreCountInWindows();   // defined in sysos.cpp under the same _WIN32 gate (incl. mingw)
#endif

    int JobQue::get_num_physical_cores() {
#if defined(__APPLE__)
        int ncores = 0; size_t len = sizeof(ncores);
        if ( sysctlbyname("hw.physicalcpu", &ncores, &len, nullptr, 0) == 0 && ncores > 0 ) return ncores;
#elif defined(_WIN32) && !defined(_GAMING_XBOX) && !defined(_DURANGO) && !defined(_M_ARM64)
        if ( int ncores = GetPhysicalCoreCountInWindows() ) return ncores;
#elif defined(__linux__)
        // 2-way SMT is the universal x86 layout; smt/active is authoritative and cheap
        if ( FILE * f = fopen("/sys/devices/system/cpu/smt/active", "r") ) {
            int active = 0;
            int got = fscanf(f, "%d", &active);
            fclose(f);
            int logical = max(1, (int)thread::hardware_concurrency());
            if ( got == 1 ) return active ? max(1, logical / 2) : logical;
        }
#endif
        return max(1, (int)thread::hardware_concurrency());
    }

#if defined(_MSC_VER) && !defined(_GAMING_XBOX) && !defined(_DURANGO) && !defined(_M_ARM64)

    int GetLogicalProcessorCountInWindows();

    int JobQue::get_num_threads() {
        int hw = GetLogicalProcessorCountInWindows();
        if ( hw <= 0 ) hw = max(1,static_cast<int>(thread::hardware_concurrency()));
        return jobque_thread_count(hw);
    }


#else

    int JobQue::get_num_threads() {
#if defined(__EMSCRIPTEN__)
        // emscripten's std::thread::hardware_concurrency() does NOT reflect
        // navigator.hardwareConcurrency in the wasm64/memory64 build (it returns a
        // low default ~2); emscripten_num_logical_cores() reads it directly and is
        // correct on both wasm32/64.
        int hw = emscripten_num_logical_cores();
#else
        int hw = static_cast<int>(thread::hardware_concurrency());
#endif
        return jobque_thread_count(das::max<int>(1, hw));
    }

#endif

    // Slot -> logical-CPU affinity: fill DISTINCT physical cores first (even logical ids — SMT
    // siblings are adjacent logical CPUs on Windows/Linux x86), then the odd siblings. Slot 0 is
    // the que-creating (dispatch caller) thread, workers take slots 1..N. Without this the OS
    // placement is a lottery: 32 busy lanes on a 32c/64t box sometimes land on 16 cores' SMT
    // pairs — measured pp512 bimodal 306 vs 199 tok/s on the 3990X. Modes (DAS_JOBQUE_AFFINITY):
    // 0 = off (default — placement hints fight ambient load on shared boxes; opt in per run),
    // 1 = ideal-processor hint (scheduler can still migrate under contention),
    // 2 = hard mask (pins survive ambient load but also can't dodge it).
    static void jobque_apply_affinity_slot ( int slot, int mode ) {
        if ( mode <= 0 ) return;
        int hw = static_cast<int>(thread::hardware_concurrency());
        if ( hw <= 1 ) return;
        int half = hw / 2;
        int cpu;
        if ( half > 0 && slot < half ) cpu = slot * 2;               // distinct cores first
        else if ( half > 0 && slot < hw ) cpu = (slot - half) * 2 + 1;   // then SMT siblings
        else cpu = slot % hw;
        SetCurrentThreadAffinityCpu(cpu, mode >= 2);
    }

    JobQue::JobQue()
        : mSleepMs(1)
        , mShutdown(false)
        , mThreadCount( 0 )
        , mJobsRunning(0) {
        mThreadCount = get_num_threads();
        const char * teamProfEnv = get_dasenv_team_prof();   // =0 / empty is off, matching the documented =1 contract
        mTeamProf = teamProfEnv != nullptr && atoi(teamProfEnv) != 0;
        const char * limitOrderEnv = get_dasenv_jobque_limit_order();   // "spread" = golden-stride worker-limit eligibility
        mLimitOrderSpread = limitOrderEnv != nullptr && strcmp(limitOrderEnv, "spread") == 0;
        const char * rankGateEnv = get_dasenv_jobque_team_rank_gate();   // =1: per-op worker participation gate (see setTeamRankGate)
        mTeamRankGate = (rankGateEnv != nullptr && atoi(rankGateEnv) != 0) ? 1 : 0;
        const char * eagerExitEnv = get_dasenv_jobque_team_eager_exit();   // =0 re-enables the final-barrier spin (see setTeamEagerExit)
        mTeamEagerExit = (eagerExitEnv != nullptr) ? ((atoi(eagerExitEnv) != 0) ? 1 : 0) : 1;
        const char * affinityEnv = get_dasenv_jobque_affinity();   // 0 off (default) / 1 ideal-CPU hint / 2 hard mask; overrides the app's set_default_affinity
        int affinityMode = affinityEnv ? atoi(affinityEnv) : (max)(0, get_default_affinity());
        SetCurrentThreadPriority(JobPriority::High);
        jobque_apply_affinity_slot(0, affinityMode);   // the que creator = the dispatch caller
        for (int j = 0, js = mThreadCount; j < js; j++) {
            mThreads.emplace_back(make_unique<thread>([this, j, affinityMode]() {
                string thread_name = "JobQue_Job_" + to_string(j);
                SetCurrentThreadName(thread_name);
                jobque_apply_affinity_slot(j + 1, affinityMode);
                job(j);
            }));
        }
    }

    JobQue::~JobQue () {
        join();
        if ( mTeamProf && mTPn ) {
            auto avg = [&](uint64_t sum, uint64_t n) -> unsigned long long { return n ? (unsigned long long)(sum / n) : 0ull; };
            fprintf(stderr, "[team-prof] ops=%llu avg-chunks=%llu caller-chunk-share=%llu%% solo-ops(no worker claimed)=%llu%%\n",
                (unsigned long long)mTPn, avg(mTPchunks, mTPn),
                mTPchunks ? (unsigned long long)(mTPcallerChunks * 100 / mTPchunks) : 0ull,
                (unsigned long long)(mTPsolo * 100 / mTPn));
            fprintf(stderr, "[team-prof] avg ns/op: publish=%llu caller-serve=%llu join-tail=%llu total=%llu\n",
                avg(mTPpub, mTPn), avg(mTPserve, mTPn), avg(mTPtail, mTPn), avg(mTPtotal, mTPn));
            fprintf(stderr, "[team-prof] worker claims, ns after publish: first=+%llu (on %llu%% of ops) last=+%llu\n",
                avg(mTPreact, mTPreactN), (unsigned long long)(mTPreactN * 100 / mTPn), avg(mTPlastRel, mTPlastN));
        }
    }

    void JobQue::resetTeamProf () {
        mTPn = mTPchunks = mTPcallerChunks = mTPsolo = 0;
        mTPpub = mTPserve = mTPtail = mTPtotal = 0;
        mTPreact = mTPreactN = mTPlastRel = mTPlastN = 0;
        mTeamFirstClaimT.store(0, std::memory_order_relaxed);
        mTeamLastClaimT.store(0, std::memory_order_relaxed);
    }

    JobQue::TeamProfSnapshot JobQue::getTeamProf () const {
        TeamProfSnapshot s;
        s.ops = mTPn; s.chunks = mTPchunks; s.callerChunks = mTPcallerChunks; s.solo = mTPsolo;
        s.publishT = mTPpub; s.serveT = mTPserve; s.tailT = mTPtail; s.totalT = mTPtotal;
        s.reactT = mTPreact; s.reactN = mTPreactN; s.lastT = mTPlastRel; s.lastN = mTPlastN;
        return s;
    }

    void JobQue::traceStart ( int eventsPerLane ) {
        int lanes = mThreadCount.load() + 1;    // workers + the dispatching caller
        mTrace.clear();
        mTrace.resize(size_t(lanes));
        for ( auto & L : mTrace ) {
            L.ev.resize(size_t(das::max(eventsPerLane, 1024)));
            L.n = 0;
        }
        mTraceT0 = uint64_t(ref_time_ticks());  // wake spans clamp their park-t0 to the session start
        mTraceOn.store(1, std::memory_order_seq_cst);
    }

    void JobQue::traceCategory ( int id, const char * name, uint32_t rgb ) {
        lock_guard<mutex> guard(mTraceRegMutex);
        for ( auto & c : mTraceCategories ) {
            if ( c.id == id ) { c.name = name ? name : ""; c.rgb = rgb; return; }
        }
        mTraceCategories.push_back({id, name ? name : "", rgb});
    }

    int JobQue::traceMarkerName ( const char * name ) {
        lock_guard<mutex> guard(mTraceRegMutex);
        string n = name ? name : "";
        for ( size_t i = 0; i != mTraceMarkerNames.size(); ++i ) {
            if ( mTraceMarkerNames[i] == n ) return int(i);
        }
        mTraceMarkerNames.push_back(n);
        return int(mTraceMarkerNames.size()) - 1;
    }

    void JobQue::traceMarker ( int nameId, int arg ) {
        if ( !mTraceOn.load(std::memory_order_relaxed) ) return;
        // caller lane — markers come from the orchestrating thread (per-token/per-frame loops)
        uint64_t t = uint64_t(ref_time_ticks());
        traceEvent(int(mTrace.size()) - 1, TraceTag::Marker, t, t, uint32_t(nameId), uint32_t(arg), 0);
    }

    static void fputsJsonString ( FILE * f, const char * s ) {
        fputc('"', f);
        for ( ; s && *s; ++s ) {
            char c = *s;
            if ( c == '"' || c == '\\' ) { fputc('\\', f); fputc(c, f); }
            else if ( uint8_t(c) >= 0x20 ) fputc(c, f);
            else fprintf(f, "\\u%04x", c);
        }
        fputc('"', f);
    }

    bool JobQue::traceSave ( const char * path ) {
        if ( mTraceOn.load(std::memory_order_relaxed) ) return false;   // stop before saving
        uint64_t tmin = ~0ull;
        for ( auto & L : mTrace ) {
            for ( uint32_t i = 0; i != L.n; ++i ) tmin = das::min(tmin, L.ev[i].t0);
        }
        if ( tmin == ~0ull ) return false;
        FILE * f = fopen(path, "wb");
        if ( !f ) return false;
        vector<TraceCategory> cats;
        vector<string> markerNames;
        {
            lock_guard<mutex> guard(mTraceRegMutex);
            cats = mTraceCategories;
            markerNames = mTraceMarkerNames;
        }
        static const char * tagName[] = { "publish", "chunk", "stage_wait", "wake", "fifo_job" };
        fprintf(f, "{\"traceEvents\":[");
        bool first = true;
        for ( size_t lane = 0; lane != mTrace.size(); ++lane ) {
            auto & L = mTrace[lane];
            const bool caller = lane + 1 == mTrace.size();
            fprintf(f, "%s\n{\"ph\":\"M\",\"pid\":0,\"tid\":%d,\"name\":\"thread_name\",\"args\":{\"name\":\"%s%d\"}}",
                first ? "" : ",", int(lane), caller ? "caller " : "worker ", int(lane));
            first = false;
            for ( uint32_t i = 0; i != L.n; ++i ) {
                const auto & e = L.ev[i];
                if ( e.tag == uint32_t(TraceTag::Marker) ) {
                    // perfetto instant event; name = the registered marker kind. arg is
                    // signed in the API (jobque_trace_marker) — emit %d so it round-trips.
                    // scope "g" is deliberate: unit boundaries (frame/token) draw across ALL
                    // lanes in Perfetto; tid still records the emitting (caller) lane for
                    // viewers that read it directly
                    fprintf(f, ",\n{\"ph\":\"i\",\"s\":\"g\",\"pid\":0,\"tid\":%d,\"ts\":%.3f,\"name\":",
                        int(lane), double(e.t0 - tmin) / 1000.0);
                    fputsJsonString(f, e.stage < uint32_t(markerNames.size()) ? markerNames[e.stage].c_str() : "marker");
                    fprintf(f, ",\"args\":{\"arg\":%d}}", int32_t(e.arg));
                    continue;
                }
                fprintf(f, ",\n{\"ph\":\"X\",\"pid\":0,\"tid\":%d,\"ts\":%.3f,\"dur\":%.3f,\"name\":\"%s\","
                    "\"args\":{\"stage\":%u,\"arg\":%u,\"chain\":%u}}",
                    int(lane), double(e.t0 - tmin) / 1000.0, double(e.t1 - e.t0) / 1000.0,
                    tagName[e.tag < 5 ? e.tag : 4], e.stage, e.arg, e.chain);
            }
        }
        // sibling key after traceEvents (JSON Object Format) — Perfetto/chrome ignore unknown keys
        fprintf(f, "\n],\"jobqueProfile\":{\"version\":1,\"lanes\":%d,\"categories\":[", int(mTrace.size()));
        for ( size_t i = 0; i != cats.size(); ++i ) {
            fprintf(f, "%s{\"id\":%d,\"name\":", i ? "," : "", cats[i].id);
            fputsJsonString(f, cats[i].name.c_str());
            fprintf(f, ",\"color\":\"#%06X\"}", cats[i].rgb & 0xFFFFFFu);
        }
        fprintf(f, "],\"markers\":[");
        for ( size_t i = 0; i != markerNames.size(); ++i ) {
            fprintf(f, "%s{\"id\":%d,\"name\":", i ? "," : "", int(i));
            fputsJsonString(f, markerNames[i].c_str());
            fputc('}', f);
        }
        fprintf(f, "]}}\n");
        fclose(f);
        return true;
    }

    void JobQue::EvalOnMainThread(Job && expr) {
        lock_guard<mutex> guard(mEvalMainThreadMutex);
        mEvalMainThread.emplace_back(das::move(expr));
    }

    void JobQue::EvalMainThreadJobs() {
        vector<Job> results;
        {
            lock_guard<mutex> guard(mEvalMainThreadMutex);
            swap(results, mEvalMainThread);
        }
        for (auto & job : results) {
            job();
        }
    }

    void JobQue::wait() {
        while ( !isEmpty(true) ) {
            while ( !isEmpty() ) {
                this_thread::yield();
            }
            EvalMainThreadJobs();
        }
    }

    bool JobQue::drain ( int timeoutMs ) {
        // Poll rather than condvar: completion has no single signalling point (jobs finish on N
        // workers and on the calling thread), and this runs once per scope exit, not in a hot path.
        // Pumping EvalMainThreadJobs is required, not optional: isEmpty(true) counts jobs that only
        // the calling thread can run, so without this the queue never empties and every drain with
        // main-thread work outstanding would spin to the timeout. Same shape as wait() above.
        auto deadline = ref_time_ticks();
        for ( ;; ) {
            if ( isEmpty(true) ) return true;
            if ( timeoutMs > 0 && get_time_usec(deadline) >= int64_t(timeoutMs) * 1000 ) return false;
            EvalMainThreadJobs();
            this_thread::yield();
        }
    }

    void JobQue::join() {
        {
            // Set the flag and wake every parked worker under the lock so none misses the wakeup.
            lock_guard<mutex> lock(mFifoMutex);
            mShutdown = true;
            mCond.notify_all();
            mLimitCond.notify_all();   // dormant over-limit workers wait on their own condvar
        }
        while ( mThreadCount ) {
            this_thread::yield();
        }
        for (auto & th : mThreads) {
            th.threadPointer->join();
        }
        mThreads.clear();
    }

    bool JobQue::isEmpty ( bool includingMainThreadJobs ) {
        lock_guard<mutex> lock(mFifoMutex);
        bool queue_is_empty = (mJobsRunning == 0) && (mFifo.size() == 0);
        if ( includingMainThreadJobs ) {
            lock_guard<mutex> mainThreadLock(mEvalMainThreadMutex);
            return queue_is_empty && mEvalMainThread.empty();
        }
        return queue_is_empty;
    }

    bool JobQue::areJobsPending(JobCategory category) {
        lock_guard<mutex> lock(mFifoMutex);
        if (find_if(mFifo.begin(), mFifo.end(), [=](const JobEntry& jobEntry) {
                return jobEntry.category == category; }) != mFifo.end()) {
            return true;
        }
        if (find_if(mThreads.begin(), mThreads.end(), [=](const ThreadEntry& threadEntry) {
                return threadEntry.currentPriority != JobPriority::Inactive && threadEntry.currentCategory == category; }) != mThreads.end()) {
            return true;
        }
        return false;
    }

    int JobQue::getTotalHwJobs() {
        return mThreadCount;
    }

    int JobQue::getNumberOfQueuedJobs() {
        lock_guard<mutex> lock(mFifoMutex);
        return int(mFifo.size());
    }

    void JobQue::submit(Job && job, JobCategory category, JobPriority priority) {
        auto  it = lower_bound(mFifo.begin(), mFifo.end(), priority, [](const JobEntry& lhs, JobPriority priority) {
            return lhs.priority >= priority; });
        mFifo.emplace(it, das::move(job), category, priority);
        mFifoCount.fetch_add(1, std::memory_order_relaxed);
    }

    void JobQue::push(Job && job, JobCategory category, JobPriority priority) {
        lock_guard<mutex> lock(mFifoMutex);
        submit(das::move(job), category, priority);
        mCond.notify_one();
    }

    void JobQue::pushBatch(vector<Job> && jobs, JobCategory category, JobPriority priority) {
        {
            lock_guard<mutex> lock(mFifoMutex);
            for ( auto & j : jobs ) {
                submit(das::move(j), category, priority);
            }
        }
        // ONE wake, not notify_all: waking N parked threads is ~N serialized OS wakes (~5-8us each)
        // no matter who calls it. Workers propagate instead — every worker that pops and leaves work
        // behind wakes the next (see job()), so the wake tree fans out with log depth and the wake
        // syscalls are paid by the (parallel) workers, not the flushing thread.
        mCond.notify_one();
        jobs.clear();
    }

    bool JobQue::tryRunOneJob() {
        // Dispatcher-side work stealing: a thread sitting at a fork/join (parallel_for) pops queued
        // jobs and runs them itself instead of sleeping — the job closures are thread-agnostic (the
        // same closures any worker would run), so the only difference is who executes. Wake
        // propagation mirrors job(): if our pop left work behind, kick parked workers.
        Job job;
        size_t moreWork = 0;
        {
            lock_guard<mutex> lock(mFifoMutex);
            if ( mFifo.empty() ) return false;
            job = das::move(mFifo.front().function);
            mFifo.pop_front();
            mFifoCount.fetch_sub(1, std::memory_order_relaxed);
            mJobsRunning++;
            moreWork = mFifo.size();
        }
        if ( moreWork >= 1 ) mCond.notify_one();
        if ( moreWork >= 2 ) mCond.notify_one();
        job();
        --mJobsRunning;
        return true;
    }

    // Worker-limit eligibility rank (see mLimitOrderSpread). Spread order: rank workers along
    // the golden-stride visit v(k) = (k*s) mod T with s co-prime ≈ T/φ — by the three-distance
    // theorem every prefix {v(0..L-1)} is a near-uniform lattice over the index space, so ANY
    // runtime limit keeps the live sliver spread instead of clumped at the low indices.
    // O(T) once per worker at startup.
    static int workerLimitRank ( int threadIndex, int total, bool spread ) {
        if ( !spread || total <= 2 ) return threadIndex;
        int s = int(double(total) * 0.6180339887498949 + 0.5);
        auto gcd = [](int a, int b) { while ( b ) { int t = a % b; a = b; b = t; } return a; };
        while ( s <= 1 || gcd(s, total) != 1 ) s++;
        for ( int k = 0; k < total; ++k ) {
            if ( (k * s) % total == threadIndex ) return k;
        }
        return threadIndex;   // unreachable: v is a bijection when gcd(s,total)==1
    }

    void JobQue::job(int threadIndex) {
        const int limitRank = workerLimitRank(threadIndex,
            mThreadCount.load(std::memory_order_relaxed), mLimitOrderSpread);
        uint32_t teamSeqSeen = mTeamSeq.load(std::memory_order_relaxed);
        while (!mShutdown) {
            // Worker-pool limit (setWorkerLimit): over-limit workers go DORMANT here — no spin,
            // no fifo service, and deliberately NO mParkedWorkers bump, so the team publish wake
            // gate skips them entirely (waking 40 dormant sleepers per dispatch is exactly the
            // storm the limit removes). Only setWorkerLimit raising the limit (or shutdown)
            // notifies; the limit is re-checked HERE on every wake, so a wake that raced a
            // lower re-parks us dormant.
            if ( limitRank >= mWorkerLimit.load(std::memory_order_relaxed) ) {
                unique_lock<mutex> lock(mFifoMutex);
                mLimitCond.wait(lock, [&]() {
                    return mShutdown.load()
                        || limitRank < mWorkerLimit.load(std::memory_order_seq_cst);
                });
                if ( mShutdown ) break;
                teamSeqSeen = mTeamSeq.load(std::memory_order_relaxed);  // don't serve ops published while dormant
                continue;
            }
            Job job;
            bool gotJob = false;
            bool goDormant = false;
            size_t moreWork = 0;    // work left behind by our pop → wake-propagate (see pushBatch)
            // Spin-before-park (opt-in via setWorkerSpin): poll the fifo for mSpinUs before blocking
            // on the condvar. notify_one to a PARKED worker costs the DISPATCHING thread an OS
            // thread-wake (~3-4.5us, serially per job — the dominant term of the fork/join tax); to
            // a spinning worker it's a ~0.1us no-waiter check. A fork/join burst (LLM decode token =
            // dozens of back-to-back parallel_for matmuls) keeps workers in the spin window the whole
            // time, so only the burst's first dispatch pays the wakes.
            // Team mode extends the same window: poll the team slot too. The window stays BOUNDED
            // (the ggml hybrid poll/park shape) — after it expires the worker parks, and a team
            // publish that finds parked workers notifies (see teamParallelFor's wake gate).
            int spinUs = mSpinUs.load(std::memory_order_relaxed);
            bool teamMode = mTeamMode.load(std::memory_order_relaxed) != 0;
            if ( (spinUs > 0 || teamMode) && !mShutdown.load(std::memory_order_relaxed) ) {
                auto deadline = std::chrono::steady_clock::now() + std::chrono::microseconds(spinUs);
                for (;;) {
                    if ( mShutdown.load(std::memory_order_relaxed) ) break;
                    // limit drift-out: exit the spin window; the dormant branch at the top of the
                    // outer loop parks us (the !gotJob park below is for ELIGIBLE workers only)
                    if ( limitRank >= mWorkerLimit.load(std::memory_order_relaxed) ) {
                        goDormant = true;
                        break;
                    }
                    teamMode = mTeamMode.load(std::memory_order_relaxed) != 0;
                    if ( teamMode && runTeamChunks(threadIndex, limitRank, teamSeqSeen) ) {
                        // saw a new team op (served it, or was rank-gated past it) — fresh spin
                        // window, same as a fifo raid restarting the loop. Team activity keeps
                        // the worker hot even when the gate starves it of chunks, so a decode
                        // stream of gated tiny ops never drains the pool into parked/wake churn.
                        deadline = std::chrono::steady_clock::now() + std::chrono::microseconds(spinUs);
                    }
                    // try_lock, NOT lock: every spinner that sees the same count blip races here, and
                    // blocking losers would queue on the mutex — a convoy the dispatcher's next push
                    // waits behind (measured 45us push stalls). try_lock losers just keep spinning,
                    // and a raided worker resumes the SAME spin window rather than parking, so the
                    // rest of the burst still finds it awake.
                    if ( mFifoCount.load(std::memory_order_relaxed) != 0 && mFifoMutex.try_lock() ) {
                        {
                            lock_guard<mutex> lock(mFifoMutex, std::adopt_lock);
                            if ( !mFifo.empty() ) {
                                job = das::move(mFifo.front().function);
                                mThreads[threadIndex].currentPriority = mFifo.front().priority;
                                mThreads[threadIndex].currentCategory = mFifo.front().category;
                                mFifo.pop_front();
                                mFifoCount.fetch_sub(1, std::memory_order_relaxed);
                                mJobsRunning++;
                                gotJob = true;
                                moreWork = mFifo.size();
                            }
                        }
                        if ( gotJob ) break;
                    }
                    jobque_spin_pause();
                    if ( std::chrono::steady_clock::now() >= deadline ) break;
                }
            }
            if ( goDormant ) continue;   // top-of-loop dormant branch parks us
            if ( !gotJob ) {
                unique_lock<mutex> lock(mFifoMutex);
                // Block until a job is available or we're shutting down. A plain wait (no periodic
                // timeout) means idle workers stay parked instead of waking every mSleepMs to grab
                // mFifoMutex and yield — that periodic wakeup contended on exactly the lock the
                // dispatch path needs, throttling parallel_for when most workers are idle between
                // a token's many small matmuls. push()/parallel_for already notify, so latency is
                // unaffected; join() sets mShutdown under the lock and notifies to wake all workers.
                // A team publish notifies only when it sees us parked (the seq_cst parked++ /
                // seq-check pair below vs the publisher's seq-bump / parked-check closes the
                // sleep/wake race), hence the team clause in the predicate + the empty-fifo continue.
                uint64_t p0 = mTraceOn.load(std::memory_order_relaxed) ? uint64_t(ref_time_ticks()) : 0;
                mParkedWorkers.fetch_add(1, std::memory_order_seq_cst);
                mCond.wait(lock, [&]() {
                    return mFifo.size() != 0 || mShutdown.load()
                        || mTeamSeq.load(std::memory_order_seq_cst) != teamSeqSeen;
                });
                mParkedWorkers.fetch_sub(1, std::memory_order_relaxed);
                if ( p0 && mTraceOn.load(std::memory_order_relaxed) ) {
                    // re-check: tracing may have stopped while we were parked; a wake into a NEWER
                    // session clamps its park-t0 to that session's start (no pre-session spans)
                    traceEvent(threadIndex, TraceTag::Wake, das::max(p0, mTraceT0), uint64_t(ref_time_ticks()), 0, 1, 0);
                }
                if ( mShutdown ) break;
                if ( mFifo.empty() ) continue;      // team wake, no fifo work → back to the spin/team poll
                job = das::move(mFifo.front().function);
                mThreads[threadIndex].currentPriority = mFifo.front().priority;
                mThreads[threadIndex].currentCategory = mFifo.front().category;
                mFifo.pop_front();
                mFifoCount.fetch_sub(1, std::memory_order_relaxed);
                mJobsRunning++;
                moreWork = mFifo.size();
            }
            // Wake propagation (outside the lock so the wakee doesn't bounce off it): if our pop left
            // work behind, wake up to TWO more workers before running the job. A batch flush wakes
            // only one worker; each wakee doubling the wake front fans the backlog out as a binary
            // tree (log2 depth ≈ 5 wake latencies for 32 jobs, vs 32 serialized wakes for a
            // notify_all), paid in parallel by the workers instead of serially by the dispatcher.
            // No-op when no one is parked (spinners pick work up themselves).
            if ( moreWork >= 1 ) mCond.notify_one();
            if ( moreWork >= 2 ) mCond.notify_one();
            // Only touch the OS thread priority when it actually changes — every job otherwise pays
            // a pthread_setschedparam syscall, and a parallel_for fires the same priority on every
            // chunk (~thousands of redundant syscalls per LLM token). Only this worker writes its own
            // appliedPriority, so no lock needed.
            if ( mThreads[threadIndex].currentPriority != mThreads[threadIndex].appliedPriority ) {
                SetCurrentThreadPriority(mThreads[threadIndex].currentPriority);
                mThreads[threadIndex].appliedPriority = mThreads[threadIndex].currentPriority;
            }
            if ( mTraceOn.load(std::memory_order_relaxed) ) {
                uint64_t j0 = uint64_t(ref_time_ticks());
                job();
                traceEvent(threadIndex, TraceTag::FifoJob, j0, uint64_t(ref_time_ticks()), 0, 0, 0);
            } else {
                job();
            }
            {
                unique_lock<mutex> lock(mFifoMutex);
                mThreads[threadIndex].currentPriority = JobPriority::Inactive;
                mJobsRunning--;
            }
        }
        mThreadCount--;
    }

    void JobQue::setTeamMode ( bool on ) {
        // no wake needed: parked workers are woken by the first publish's wake gate
        mTeamMode.store(on ? 1 : 0, std::memory_order_relaxed);
    }

    void JobQue::setWorkerLimit ( int n ) {
        int lim = n < 0 ? 0x7fffffff : n;
        int old = mWorkerLimit.exchange(lim, std::memory_order_seq_cst);
        if ( lim > old ) {
            // raising re-admits dormant workers — they only ever wake from here (or shutdown).
            // The lock pairs with the dormant wait's predicate read: a worker between its limit
            // check and cond_wait holds mFifoMutex, so this notify can't slip into that window.
            // Lowering never notifies: workers drift out at their next spin-loop check (or
            // re-park dormant on their next wake).
            lock_guard<mutex> guard(mFifoMutex);
            mLimitCond.notify_all();
        }
    }

    bool JobQue::runTeamChunks ( int threadIndex, int limitRank, uint32_t & seqSeen ) {
        uint32_t seq = mTeamSeq.load(std::memory_order_acquire);
        if ( seq == seqSeen || (seq & 1) ) return false;   // nothing new, or publish in progress
        // Per-op rank gate (opt-in, setTeamRankGate): admit only as many workers as the widest
        // stage has chunks — rank r serves only when r + 1 < maxChunks (+1 = the caller's lane;
        // it always serves). A gated worker consumes the seq and reports "saw activity", so its
        // spin window refreshes and the next, bigger op re-admits it for one relaxed load. The
        // pre-entrant count read is racy vs a concurrent publish and that is fine: skipping is
        // ALWAYS correct (chunks self-serve, the caller drains — the property that makes the
        // worker limit correctness-free), and the enter path re-verifies the seq in the entrant
        // gate below. Reads ONLY the atomic op words — mTeamStageCount is unsynchronized here —
        // and publish zeroes the unused stage slots so a stale wider chain can't over-admit.
        if ( mTeamRankGate.load(std::memory_order_relaxed) ) {
            uint32_t maxChunks = 0;
            for ( int s = 0; s != MAX_TEAM_STAGES; ++s ) {
                uint32_t k = uint32_t(mTeamOpS[s].load(std::memory_order_relaxed) >> 32);
                maxChunks = k > maxChunks ? k : maxChunks;
            }
            if ( uint32_t(limitRank) + 1 >= maxChunks ) {
                seqSeen = seq;
                return true;
            }
        }
        // entrant gate (Dekker with the publisher): declare in-flight, then re-verify the seq.
        // Publisher: bump seq to odd (seq_cst store), then read mTeamInFlight. Us: bump
        // mTeamInFlight (seq_cst store), then read seq. Either the publisher sees us in-flight
        // and waits, or we see its odd/new seq and back out — so the stage words can never be
        // rewritten while any lane is inside the chain loop. That is the inter-chain stage-order
        // guarantee: a straggler cannot claim a NEWER chain's stage-s chunk (whose stage s-1 it
        // never synchronized with), because it cannot still be here when that chain publishes.
        mTeamInFlight.fetch_add(1, std::memory_order_seq_cst);
        uint32_t seq2 = mTeamSeq.load(std::memory_order_seq_cst);
        if ( seq2 != seq ) {
            mTeamInFlight.fetch_sub(1, std::memory_order_release);
            return false;                                  // raced a publish — retry next poll
        }
        seqSeen = seq;
        int nStages = mTeamStageCount;
        const bool tracing = mTraceOn.load(std::memory_order_relaxed) != 0;
        for ( int s = 0; s != nStages; ++s ) {
            for (;;) {
                // one fetch_add returns {numChunks, index} as a unit, so overflow (c >= K)
                // self-describes and an index can never pair with another stage's count. A valid
                // claim's pending decrement blocks this stage's barrier, which is what keeps
                // mTeamWorkS (the dispatcher's stack) alive for exactly as long as we dereference it.
                uint64_t claim = mTeamOpS[s].fetch_add(1, std::memory_order_acq_rel);
                uint32_t c = uint32_t(claim);
                if ( c >= uint32_t(claim >> 32) ) break;
                if ( mTeamProf ) {   // anatomy profiler: stamp this chain's first/last worker claim
                    uint64_t t = uint64_t(ref_time_ticks());
                    uint64_t z = 0;
                    mTeamFirstClaimT.compare_exchange_strong(z, t, std::memory_order_relaxed);
                    mTeamLastClaimT.store(t, std::memory_order_relaxed);
                }
                if ( tracing ) {
                    uint64_t c0 = uint64_t(ref_time_ticks());
                    (*mTeamWorkS[s])(int(c), threadIndex);
                    traceEvent(threadIndex, TraceTag::Chunk, c0, uint64_t(ref_time_ticks()), uint32_t(s), c, seq);
                } else {
                    (*mTeamWorkS[s])(int(c), threadIndex);
                }
                mTeamRemainingS[s].fetch_sub(1, std::memory_order_release);
            }
            // eager exit: the FINAL join belongs to the caller (its remaining==0 spin), and we
            // hold no claim after the overflow — spinning here only widens the in-flight window
            // the next publish must drain, so a preemption in this spin becomes a fat publish.
            if ( s == nStages - 1 && mTeamEagerExit.load(std::memory_order_relaxed) ) break;
            // stage barrier: stage s+1 reads what stage s wrote, so wait for ALL of stage s —
            // the acquire pairs with every serving lane's release decrement.
            uint64_t w0 = tracing ? uint64_t(ref_time_ticks()) : 0;
            while ( mTeamRemainingS[s].load(std::memory_order_acquire) != 0 ) {
                if ( mShutdown.load(std::memory_order_relaxed) ) { s = nStages - 1; break; }
                jobque_spin_relax();
            }
            if ( tracing ) traceEvent(threadIndex, TraceTag::StageWait, w0, uint64_t(ref_time_ticks()), uint32_t(s), 0, seq);
        }
        mTeamInFlight.fetch_sub(1, std::memory_order_release);
        // true = seqSeen advanced (we saw this op, whether or not we won any chunks) — the
        // caller reads it as "team activity, refresh the spin window", not "did work"
        return true;
    }

    void JobQue::teamParallelFor ( int numChunks, const JobChunk & work ) {
        TeamStage stage = { &work, numChunks };
        teamParallelForStages(&stage, 1);
    }

    void JobQue::teamParallelForStages ( const TeamStage * stages, int numStages ) {
        DAS_ASSERT(numStages >= 1 && numStages <= MAX_TEAM_STAGES);
        int nW = mThreadCount.load();
        int maxChunks = 0;
        for ( int s = 0; s != numStages; ++s ) maxChunks = das::max(maxChunks, stages[s].numChunks);
        if ( maxChunks <= 1 || nW == 0 || !getTeamMode() ) {
            // trivial/off: run the stages in order on the calling thread — same stage semantics
            for ( int s = 0; s != numStages; ++s )
                for ( int c = 0; c != stages[s].numChunks; ++c ) (*stages[s].work)(c, nW);
            return;
        }
        // The team publish slots are intentionally shared by the whole pool. Multiple inference
        // contexts may dispatch from different caller threads (for example LLM + ASR); serialize
        // those publishes while retaining full worker-team parallelism inside each operation.
        lock_guard<mutex> dispatchLock(mTeamDispatchMutex);
        uint64_t t0 = 0, tp = 0, tc = 0;
        int callerChunks = 0;
        int totalChunks = 0;
        const bool tracing = mTraceOn.load(std::memory_order_relaxed) != 0;
        uint64_t tt0 = tracing ? uint64_t(ref_time_ticks()) : 0;
        if ( mTeamProf ) {
            t0 = uint64_t(ref_time_ticks());
            mTeamFirstClaimT.store(0, std::memory_order_relaxed);
            mTeamLastClaimT.store(0, std::memory_order_relaxed);
        }
        // publish, phase 1 (seqlock): odd seq = "publishing", then wait out in-flight stragglers
        // of prior chains (normally none by join time — a lane lingers only for the tail of an
        // overflow walk). Only after mTeamInFlight drains is it safe to rewrite the stage words.
        mTeamSeq.fetch_add(1, std::memory_order_seq_cst);
        while ( mTeamInFlight.load(std::memory_order_seq_cst) != 0 ) jobque_spin_relax();
        mTeamStageCount = numStages;
        for ( int s = 0; s != numStages; ++s ) {
            mTeamWorkS[s] = stages[s].work;
            mTeamRemainingS[s].store(stages[s].numChunks, std::memory_order_relaxed);
            // {numChunks, counter=0} in one word — see runTeamChunks
            mTeamOpS[s].store(uint64_t(uint32_t(stages[s].numChunks)) << 32, std::memory_order_relaxed);
            totalChunks += stages[s].numChunks;
        }
        // zero the unused slots: the rank gate pre-reads all four op words (it can't touch
        // mTeamStageCount unsynchronized), so a stale wider chain left here would over-admit
        // every later tiny op — the gate would be neutered after the first fused chain
        for ( int s = numStages; s != MAX_TEAM_STAGES; ++s ) {
            mTeamOpS[s].store(0, std::memory_order_relaxed);
        }
        // publish, phase 2: even seq = "published". seq_cst bump + parked check: the Dekker pair
        // with the worker's parked++ / seq-check — either the parking worker sees this chain, or
        // we see it parked and pay the wake. No mutex touched while everyone spins.
        uint32_t chainSeq = mTeamSeq.fetch_add(1, std::memory_order_seq_cst) + 1;
        if ( mParkedWorkers.load(std::memory_order_seq_cst) > 0 ) {
            lock_guard<mutex> lock(mFifoMutex);
            mCond.notify_all();
        }
        if ( mTeamProf ) tp = uint64_t(ref_time_ticks());
        if ( tracing ) traceEvent(nW, TraceTag::ChainPublish, tt0, uint64_t(ref_time_ticks()),
            uint32_t(numStages) | (uint32_t(mTraceTag.load(std::memory_order_relaxed)) << 8),
            uint32_t(totalChunks), chainSeq);
        // caller serves every stage and participates in every barrier (slot nW)
        for ( int s = 0; s != numStages; ++s ) {
            for (;;) {
                uint64_t claim = mTeamOpS[s].fetch_add(1, std::memory_order_acq_rel);
                uint32_t c = uint32_t(claim);
                if ( c >= uint32_t(claim >> 32) ) break;
                if ( tracing ) {
                    uint64_t c0 = uint64_t(ref_time_ticks());
                    (*stages[s].work)(int(c), nW);
                    traceEvent(nW, TraceTag::Chunk, c0, uint64_t(ref_time_ticks()), uint32_t(s), c, chainSeq);
                } else {
                    (*stages[s].work)(int(c), nW);
                }
                mTeamRemainingS[s].fetch_sub(1, std::memory_order_release);
                ++callerChunks;
            }
            if ( mTeamProf && s == numStages - 1 ) tc = uint64_t(ref_time_ticks());
            // stage barrier / final join: acquire pairs with the workers' release decrements, so
            // their chunk writes are visible. Unbounded spin — the tail is at most one chunk per lane.
            uint64_t w0 = tracing ? uint64_t(ref_time_ticks()) : 0;
            while ( mTeamRemainingS[s].load(std::memory_order_acquire) != 0 ) jobque_spin_relax();
            if ( tracing ) traceEvent(nW, TraceTag::StageWait, w0, uint64_t(ref_time_ticks()), uint32_t(s), 0, chainSeq);
        }
        if ( mTeamProf ) {
            uint64_t tj = uint64_t(ref_time_ticks());
            mTPn++; mTPchunks += uint64_t(totalChunks); mTPcallerChunks += uint64_t(callerChunks);
            mTPpub += tp - t0; mTPserve += tc - tp; mTPtail += tj - tc; mTPtotal += tj - t0;
            uint64_t fc = mTeamFirstClaimT.load(std::memory_order_relaxed);
            if ( fc ) { mTPreact += fc > tp ? fc - tp : 0; mTPreactN++; } else { mTPsolo++; }
            uint64_t lc = mTeamLastClaimT.load(std::memory_order_relaxed);
            if ( lc ) { mTPlastRel += lc > tp ? lc - tp : 0; mTPlastN++; }
        }
    }

    void JobQue::parallel_for ( JobStatus & status, int from, int to, const JobChunk & chunk,
            JobCategory category, JobPriority priority, int chunk_count, int step ) {
        if ( from >= to ) return;
        if ( chunk_count==-1 ) chunk_count = mThreadCount * 4;
        step = max ( ( to - from + 1 ) / chunk_count, step );
        int numChunks = (to - from + step) / step;
        if ( numChunks==1 ) {
            chunk(from, to);
            return;
        }
        int onMainThread = max ( (numChunks + mThreadCount)  / (mThreadCount+1), 1 );
        int onThreads  = numChunks - onMainThread;
        status.Clear(onThreads);
        {
            lock_guard<mutex> lock(mFifoMutex);
            for (int ch = 0; ch < onThreads; ++ch) {
                int i0 = from + ch * step;
                int i1 = i0 + step;
                submit([=,&status](){
                    chunk(i0, i1);
                    status.Notify();
                }, category, priority);
            }
            mCond.notify_all();
        }
        chunk(from + onThreads * step, to);
    }

    void JobQue::parallel_for ( int from, int to, const JobChunk & chunk, JobCategory category, JobPriority priority, int chunk_count, int step ) {
        JobStatus status;
        parallel_for(status, from, to, chunk, category, priority, chunk_count, step);
        status.Wait();
    }

    void JobQue::parallel_for_with_consume(int from, int to, const JobChunk & chunk, const JobChunk & consume,
        JobCategory category, JobPriority priority, int chunk_count, int step) {
        if (from >= to) return;
        if (chunk_count == -1) chunk_count = mThreadCount * 4;
        step = max((to - from) / chunk_count, step);
        int numChunks = (to - from) / step;
        if (numChunks == 1) {
            chunk(from, to);
            consume(from, to);
            return;
        }
        deque<Job> producerFifoJobs;
        mutex producerFifoMutex;
        condition_variable condition;
        {
            lock_guard<mutex> lock(mFifoMutex);
            for (int ch = 0; ch < numChunks; ++ch) {
                int i0 = from + ch * step;
                int i1 = min(i0 + step, to);
                submit([=, &chunk, &producerFifoJobs, &producerFifoMutex, &condition]() {
                    chunk(i0, i1);
                    {
                        lock_guard<mutex> producerFifoLock(producerFifoMutex);
                        producerFifoJobs.push_back(([=]() { consume(i0, i1); }));
                        condition.notify_one();
                    }
                }, category, priority);
            }
            mCond.notify_all();
        }
        {
            int chunksRemaining = numChunks;
            while (chunksRemaining > 0) {
                deque<Job> consumerFifoJobs;
                {
                    unique_lock<mutex> producerFifoLock(producerFifoMutex);
                    if (condition.wait_for(producerFifoLock, std::chrono::milliseconds(mSleepMs), [&]() {return producerFifoJobs.size() > 0; })) {
                        consumerFifoJobs.swap(producerFifoJobs);
                    } else {
                        this_thread::yield();
                        continue;
                    }
                }
                for (auto & job : consumerFifoJobs) {
                    job();
                    --chunksRemaining;
                }
            }
        }
    }

    // JobStatus tracking

    void JobStatus::trackInsert() {
        lock_guard<mutex> guard(sTrackMutex);
        mTrackId = ++g_jobque_track_total;
        mTrackNext = sTrackHead;
        mTrackPrev = nullptr;
        if (sTrackHead) sTrackHead->mTrackPrev = this;
        sTrackHead = this;
    }

    void JobStatus::trackRemove() {
        lock_guard<mutex> guard(sTrackMutex);
        if (mTrackPrev) mTrackPrev->mTrackNext = mTrackNext;
        else sTrackHead = mTrackNext;
        if (mTrackNext) mTrackNext->mTrackPrev = mTrackPrev;
    }

    void JobStatus::trackEvent ( LineInfo * at, bool isAddRef ) {
        if ( !at ) return;
        if ( mTrackId != g_jobque_track_id.load() ) return;
        TextPrinter tp;
        tp << "JobStatus #" << HEX << mTrackId << DEC;
        if ( mTrackMagic == TRACK_CHANNEL ) tp << " (Channel)";
        else if ( mTrackMagic == TRACK_LOCKBOX ) tp << " (LockBox)";
        else if ( mTrackMagic == TRACK_STREAM ) tp << " (Stream)";
        else if ( mTrackMagic == TRACK_SEQBOX ) tp << " (SeqBox)";
        else tp << " (JobStatus)";
        tp << (isAddRef ? " addRef" : " releaseRef")
           << " (rc=" << int(mRef.load()) << ")";
        tp << " at " << at->describe() << "\n";
    }

    int JobStatus::addRef( LineInfo * at ) {
        trackEvent(at, true);
        return mRef++;
    }

    int JobStatus::releaseRef( LineInfo * at ) {
        trackEvent(at, false);
        return --mRef;
    }

    // JobStatus constructors/destructor

    JobStatus::JobStatus() {
        mTrackMagic = TRACK_JOB_STATUS;
        trackInsert();
    }

    JobStatus::JobStatus(uint32_t count) {
        mTrackMagic = TRACK_JOB_STATUS;
        trackInsert();
        Clear(count);
    }

    JobStatus::~JobStatus() {
        DAS_VERIFY(mMagic==uint32_t(STATUS_MAGIC));
        DAS_VERIFY(mRef==0);
        mMagic = 0;
        trackRemove();
    }

    bool JobStatus::Notify() {
        lock_guard<mutex> guard(mCompleteMutex);
        if ( mRemaining == 0 ) return false;
        --mRemaining;
        if ( mRemaining==0 ) {
            mCond.notify_all();
        }
        return true;
    }

    bool JobStatus::NotifyAndRelease( LineInfo * at ) {
        lock_guard<mutex> guard(mCompleteMutex);
        trackEvent(at, false);
        mRef--;
        if ( mRemaining == 0 ) return false;
        --mRemaining;
        if ( mRemaining==0 ) {
            mCond.notify_all();
        }
        return true;
    }

    void JobStatus::Wait() {
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
        // Single-threaded wasm (no -pthread): nothing can decrement mRemaining
        // except the calling thread, which is the very thread blocked here — a
        // condition_variable wait would deadlock forever. join()/Wait() exist to
        // fence against a worker thread that does not exist in this build, so the
        // wait is a no-op. (A miniaudio Web Audio callback that would otherwise
        // drain a Stream/LockBox runs as a separate main-thread task and likewise
        // cannot make progress while we block, so there is nothing to wait for.)
        return;
#else
        // Join spin-before-park (opt-in via set_jobque_join_spin, default 0 = park immediately):
        // poll the counter before blocking — the ggml hybrid-poll shape verbatim
        // (ggml_graph_compute_poll_for_work): a COUNTED loop of one relaxed load + one cpu-relax
        // per round, no clock anywhere in the loop (a steady_clock read per round costs more than
        // the pause and caps the poll rate). level*1024*128 rounds, level 50 ≈ ggml's default
        // window. The final seq_cst load reading 0 synchronizes with every notifier's decrement
        // (RMW release sequence), so the workers' results are visible without taking mCompleteMutex.
        int level = sJoinSpin.load(std::memory_order_relaxed);
        if ( level > 0 ) {
            const uint64_t nRounds = uint64_t(level) * 1024ull * 128ull;
            for ( uint64_t i = 0; mRemaining.load(std::memory_order_relaxed) != 0 && i < nRounds; ++i ) {
                jobque_spin_relax();
            }
            if ( mRemaining.load() == 0 ) {
                // Lifetime handshake: the last notifier decrements under mCompleteMutex and can
                // still be inside notify_all/unlock ON THIS OBJECT when the lock-free load sees 0.
                // Callers destroy the JobStatus right after Wait returns (with_wait_group holds it
                // on the stack), so take the lock once to wait the notifier out of its locked
                // region. Uncontended in the common case — the notifier is nanoseconds from done.
                lock_guard<mutex> guard(mCompleteMutex);
                return;
            }
        }
        unique_lock<mutex> lock(mCompleteMutex);
        mCond.wait(lock, [this] {
            return mRemaining==0;
        });
#endif
    }

    bool JobStatus::isReady() {
        lock_guard<mutex> guard(mCompleteMutex);
        return mRemaining==0;
    }

    void JobStatus::Clear(uint32_t count) {
        lock_guard<mutex> guard(mCompleteMutex);
        mRemaining = count;
    }

    void JobStatus::DumpJobQueLeaks() {
        {
            lock_guard<mutex> guard(sTrackMutex);
            TextPrinter tp;
            int total = 0;
            for ( auto p = sTrackHead; p; p = p->mTrackNext ) {
                tp << "  JobStatus #" << HEX << p->mTrackId << DEC
                   << " (rc=" << int(p->mRef.load()) << ")";
                if ( p->mTrackMagic == TRACK_CHANNEL ) tp << " Channel";
                else if ( p->mTrackMagic == TRACK_LOCKBOX ) tp << " LockBox";
                else if ( p->mTrackMagic == TRACK_STREAM ) tp << " Stream";
                else if ( p->mTrackMagic == TRACK_SEQBOX ) tp << " SeqBox";
                else tp << " JobStatus";
                if ( !p->mCreatedAt.empty() ) tp << " created at " << p->mCreatedAt;
                tp << "\n";
                total++;
            }
            if ( total ) tp << "total " << total << " leaked JobStatus objects\n";
        }
        {
            lock_guard<mutex> guard(Feature::sTrackMutex);
            TextPrinter tp;
            int total = 0;
            for ( auto f = Feature::sTrackHead; f; f = f->fTrackNext ) {
                tp << "  Feature #" << HEX << f->fTrackId << DEC;
                if ( f->fOwner ) {
                    tp << " owner=#" << HEX << f->fOwnerTrackId << DEC;
                }
                if ( !f->fCreatedAt.empty() ) tp << " created at " << f->fCreatedAt;
                tp << "\n";
                total++;
            }
            if ( total ) tp << "total " << total << " leaked Feature objects\n";
        }
    }

    uint64_t JobStatus::CountJobQueLeaks() {
        uint64_t total = 0;
        {
            lock_guard<mutex> guard(sTrackMutex);
            for ( auto p = sTrackHead; p; p = p->mTrackNext ) total++;
        }
        {
            lock_guard<mutex> guard(Feature::sTrackMutex);
            for ( auto f = Feature::sTrackHead; f; f = f->fTrackNext ) total++;
        }
        return total;
    }

    // Feature tracking

    void Feature::trackInsert() {
        lock_guard<mutex> guard(sTrackMutex);
        fTrackId = ++g_jobque_track_total;
        fTrackNext = sTrackHead;
        fTrackPrev = nullptr;
        if (sTrackHead) sTrackHead->fTrackPrev = this;
        sTrackHead = this;
    }

    void Feature::trackRemove() {
        lock_guard<mutex> guard(sTrackMutex);
        if (fTrackPrev) fTrackPrev->fTrackNext = fTrackNext;
        else sTrackHead = fTrackNext;
        if (fTrackNext) fTrackNext->fTrackPrev = fTrackPrev;
    }

    Feature::Feature() {
        trackInsert();
    }

    Feature::Feature ( void * d, TypeInfo * ti, Context * c ) : data(d), type(ti) {
        trackInsert();
        setFrom(c);
    }

    Feature::~Feature() {
        trackRemove();
    }

    Feature::Feature ( const Feature & f )
        : data(f.data), type(f.type), from(f.from), fromShared(f.fromShared) {
        fCreatedAt = f.fCreatedAt;
        fOwner = f.fOwner;
        fOwnerTrackId = f.fOwnerTrackId;
        trackInsert();
    }

    Feature::Feature ( Feature && f )
        : data(f.data), type(f.type), from(f.from), fromShared(das::move(f.fromShared)) {
        fCreatedAt = f.fCreatedAt;
        fOwner = f.fOwner;
        fOwnerTrackId = f.fOwnerTrackId;
        f.data = nullptr;
        f.type = nullptr;
        f.from = nullptr;
        trackInsert();
    }

    Feature & Feature::operator = ( const Feature & f ) {
        if ( this != &f ) {
            data = f.data;
            type = f.type;
            from = f.from;
            fromShared = f.fromShared;
            fCreatedAt = f.fCreatedAt;
            fOwner = f.fOwner;
            fOwnerTrackId = f.fOwnerTrackId;
        }
        return *this;
    }

    Feature & Feature::operator = ( Feature && f ) {
        if ( this != &f ) {
            data = f.data;
            type = f.type;
            from = f.from;
            fromShared = das::move(f.fromShared);
            fCreatedAt = f.fCreatedAt;
            fOwner = f.fOwner;
            fOwnerTrackId = f.fOwnerTrackId;
            f.data = nullptr;
            f.type = nullptr;
            f.from = nullptr;
        }
        return *this;
    }

    void Feature::setFrom ( Context * c ) {
        from = c;
        if ( c && c->sharedPtrContext ) {
            // Standalone-AOT/exe contexts are member objects (not make_shared), so their
            // weak_from_this() is empty and shared_from_this() throws bad_weak_ptr. Fall back
            // to a null owner: such a context lives for the whole program, so the Feature never
            // needs to keep it alive. Mirrors debugAgentContextOrNull in context.cpp.
            auto wp = c->weak_from_this();
            fromShared = wp.expired() ? nullptr : wp.lock();
        } else {
            fromShared.reset();
        }
    }

    void Feature::clear() {
        data = nullptr;
        type = nullptr;
        from = nullptr;
        fromShared.reset();
    }

    void Feature::DumpFeatures() {
        lock_guard<mutex> guard(sTrackMutex);
        TextPrinter tp;
        int total = 0;
        for ( auto f = sTrackHead; f; f = f->fTrackNext ) {
            tp << "  Feature #" << HEX << f->fTrackId << DEC;
            if ( f->fOwner ) {
                tp << " owner=#" << HEX << f->fOwnerTrackId << DEC;
            }
            if ( !f->fCreatedAt.empty() ) tp << " created at " << f->fCreatedAt;
            tp << "\n";
            total++;
        }
        if ( total ) tp << "total " << total << " tracked Feature objects\n";
    }
}

#if defined(__APPLE__)

#include <pthread/qos.h>

namespace das {
    void SetCurrentThreadName ( const string & str ) {
        pthread_setname_np(str.c_str());
    }

    // Apple scheduling runs on QoS classes, not sched_param — pthread_setschedparam opts the
    // thread OUT of QoS permanently (later QoS calls fail EPERM), and on asymmetric silicon the
    // QoS class is what decides P-core vs E-core placement, INCLUDING after a block: an
    // unclassed thread that slept or waited on the GPU wakes cold on an E-core (~half scalar
    // throughput) until sustained load re-promotes it, a classed one comes back to a P-core.
    void SetCurrentThreadPriority ( JobPriority _priority ) {
        qos_class_t qos = QOS_CLASS_DEFAULT;
        switch (_priority) {
        case JobPriority::Minimum:
        case JobPriority::Low:      qos = QOS_CLASS_UTILITY; break;
        case JobPriority::Medium:   qos = QOS_CLASS_DEFAULT; break;
        case JobPriority::High:     qos = QOS_CLASS_USER_INITIATED; break;
        case JobPriority::Maximum:  qos = QOS_CLASS_USER_INTERACTIVE; break;
        default:                    qos = QOS_CLASS_DEFAULT; break;
        }
        pthread_set_qos_class_self_np(qos, 0);
    }

    void SetCurrentThreadAffinityCpu ( int, bool hard ) {
        // no public pin API on macOS — the platform lever is the QoS class, a P-core BIAS the
        // scheduler honors across blocks; the hint mode biases, the hard mode takes the top class
        pthread_set_qos_class_self_np(hard ? QOS_CLASS_USER_INTERACTIVE : QOS_CLASS_USER_INITIATED, 0);
    }
}

#elif defined(_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;       // Must be 0x1000.
    LPCSTR szName;      // Pointer to name (in user addr space).
    DWORD dwThreadID;   // Thread ID (-1=caller thread).
    DWORD dwFlags;      // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

namespace das {
    // http://msdn.microsoft.com/en-us/library/xcb2z8hs(VS.90).aspx
    void SetCurrentThreadName ( const string & str ) {
        DWORD dwThreadID = GetCurrentThreadId();
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = str.c_str();
        info.dwThreadID = dwThreadID;
        info.dwFlags = 0;
        __try {
            const DWORD MS_VC_EXCEPTION = 0x406D1388;
            RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
        } __except(EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    void SetCurrentThreadPriority ( JobPriority priority ) {
        int winPriority = THREAD_PRIORITY_NORMAL;
        switch (priority) {
        case JobPriority::Minimum:  winPriority = THREAD_PRIORITY_LOWEST; break;
        case JobPriority::Low:      winPriority = THREAD_PRIORITY_BELOW_NORMAL; break;
        case JobPriority::Medium:   winPriority = THREAD_PRIORITY_NORMAL; break;
        case JobPriority::High:     winPriority = THREAD_PRIORITY_ABOVE_NORMAL; break;
        case JobPriority::Maximum:  winPriority = THREAD_PRIORITY_HIGHEST; break;
        default:                    DAS_VERIFYF(0, "Windows takes prefixed priority values"); break;
        }
        SetThreadPriority(GetCurrentThread(), winPriority);
    }

    void SetCurrentThreadAffinityCpu ( int cpu, bool hard ) {
        if ( cpu < 0 || cpu >= 64 ) return;   // single-group boxes only; >64-CPU groups need PROCESSOR_NUMBER plumbing
        if ( hard ) SetThreadAffinityMask(GetCurrentThread(), 1ull << cpu);
        else SetThreadIdealProcessor(GetCurrentThread(), DWORD(cpu));
    }
}

#elif defined(__linux__) || defined __HAIKU__

#include <pthread.h>
#include <sched.h>

namespace das
{
    void SetCurrentThreadName ( const string & str ) {
        pthread_setname_np(pthread_self(), str.c_str());
    }

    void SetCurrentThreadPriority ( JobPriority _priority ) {
        int priority = int(_priority);
        float minPlatformPriority = sched_get_priority_min(SCHED_OTHER);
        float maxPlatformPriority = sched_get_priority_max(SCHED_OTHER);
        struct sched_param sched_param;
        int platformPriority = (int)( minPlatformPriority + (maxPlatformPriority - minPlatformPriority)
            * ((float)(priority - int(JobPriority::Minimum)) / (float)(int(JobPriority::Maximum) - int(JobPriority::Minimum))));
        sched_param.sched_priority = platformPriority;
        pthread_setschedparam(pthread_self(), SCHED_OTHER, &sched_param);
    }

#if defined(__HAIKU__)
    void SetCurrentThreadAffinityCpu ( int, bool ) {}
#else
    void SetCurrentThreadAffinityCpu ( int cpu, bool hard ) {
        if ( !hard ) return;   // linux has no soft "ideal CPU" hint — only the hard-mask mode pins
        cpu_set_t cs;
        CPU_ZERO(&cs);
        CPU_SET(cpu, &cs);
        // pid 0 means the calling thread, same as pthread_setaffinity_np(pthread_self(), ...),
        // which bionic does not declare
        sched_setaffinity(0, sizeof(cs), &cs);
    }
#endif
}

#else

namespace das
{
    void SetCurrentThreadName ( const string & ) {}
    void SetCurrentThreadPriority ( JobPriority ) {}
    void SetCurrentThreadAffinityCpu ( int, bool ) {}
}

#endif
