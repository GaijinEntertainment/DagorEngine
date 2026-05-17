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
        static void DumpJobQueLeaks();
        static uint64_t CountJobQueLeaks();
        void trackEvent ( LineInfo * at, bool isAddRef );
    protected:
        void trackInsert();
        void trackRemove();
    protected:
        mutable mutex       mCompleteMutex;
        uint32_t            mRemaining = 0;
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
        bool areJobsPending(JobCategory category);
        int getNumberOfQueuedJobs();
        int getTotalHwJobs();
        void push(Job && job, JobCategory category, JobPriority priority);
        void parallel_for ( JobStatus & status, int from, int to, const JobChunk & chunk, JobCategory category, JobPriority priority, int chunk_count = -1, int step = 1 );
        void parallel_for ( int from, int to, const JobChunk & chunk, JobCategory category, JobPriority priority, int chunk_count = -1, int step = 1 );
        void parallel_for_with_consume (int from, int to, const JobChunk & chunk, const JobChunk & consume, JobCategory category, JobPriority priority, int chunk_count = -1, int step = 1);
        static int get_num_threads();
        void EvalOnMainThread(Job && expr);
        void EvalMainThreadJobs();
        void wait();
        void Reset() { wait( ); }
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
            JobCategory         currentCategory = 0;
        };
    protected:
        void join();
        void job(int threadIndex);
        void submit(Job && job, JobCategory category, JobPriority priority);
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
    protected:
        mutex mEvalMainThreadMutex;
        vector<Job> mEvalMainThread;
    };

    void SetCurrentThreadName ( const string & str );
    void SetCurrentThreadPriority ( JobPriority priority );             // change priority of current thread, 0 - normal, >0 above normal, <0 below normal
}
