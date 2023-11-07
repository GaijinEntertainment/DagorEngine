#include "daScript/misc/platform.h"

#include "daScript/misc/job_que.h"

namespace das {

    JobQue::JobQue()
        : mSleepMs(1)
        , mShutdown(false)
        , mThreadCount( 0 )
        , mJobsRunning(0) {
        mThreadCount = max(1,(static_cast<int>(thread::hardware_concurrency())));
        SetCurrentThreadPriority(JobPriority::High);
        for (int j = 0, js = mThreadCount; j < js; j++) {
            mThreads.emplace_back(make_unique<thread>([=]() {
                string thread_name = "JobQue_Job_" + to_string(j);
                SetCurrentThreadName(thread_name);
                job(j);
            }));
        }
    }

    JobQue::~JobQue () {
        join();
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

    void JobQue::join() {
        mShutdown = true;
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
    }

    void JobQue::push(Job && job, JobCategory category, JobPriority priority) {
        lock_guard<mutex> lock(mFifoMutex);
        submit(das::move(job), category, priority);
        mCond.notify_one();
    }

    void JobQue::job(int threadIndex) {
        while (!mShutdown) {
            Job job;
            {
                unique_lock<mutex> lock(mFifoMutex);
                if ( mCond.wait_for(lock, chrono::milliseconds(mSleepMs), [&]() { return mFifo.size() != 0; }) ) {
                    DAS_ASSERTF(mFifo.size() > 0, "There must be at least one job available");
                    job = das::move(mFifo.front().function);
                    mThreads[threadIndex].currentPriority = mFifo.front().priority;
                    mThreads[threadIndex].currentCategory = mFifo.front().category;
                    mFifo.pop_front();
                    mJobsRunning++;
                } else {
                    this_thread::yield();
                    continue;
                }
            }
            SetCurrentThreadPriority(mThreads[threadIndex].currentPriority);
            job();
            {
                unique_lock<mutex> lock(mFifoMutex);
                mThreads[threadIndex].currentPriority = JobPriority::Inactive;
                mJobsRunning--;
            }
        }
        mThreadCount--;
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
                    if (condition.wait_for(producerFifoLock, chrono::milliseconds(mSleepMs), [&]() {return producerFifoJobs.size() > 0; })) {
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

    JobStatus::~JobStatus() {
        DAS_ASSERT(mMagic==STATUS_MAGIC);
        DAS_ASSERT(mRef==0);
        mMagic = 0;
    }

    void JobStatus::Notify() {
        lock_guard<mutex> guard(mCompleteMutex);
        DAS_ASSERTF(mRemaining != 0, "Nothing to notify!");
        --mRemaining;
        if ( mRemaining==0 ) {
            mCond.notify_all();
        }
    }

    void JobStatus::NotifyAndRelease() {
        lock_guard<mutex> guard(mCompleteMutex);
        mRef--;
        DAS_ASSERTF(mRemaining != 0, "Nothing to notify!");
        --mRemaining;
        if ( mRemaining==0 ) {
            mCond.notify_all();
        }
    }

    void JobStatus::Wait() {
        unique_lock<mutex> lock(mCompleteMutex);
        mCond.wait(lock, [this] {
            return mRemaining==0;
        });
    }

    bool JobStatus::isReady() {
        lock_guard<mutex> guard(mCompleteMutex);
        return mRemaining==0;
    }

    void JobStatus::Clear(uint32_t count) {
        lock_guard<mutex> guard(mCompleteMutex);
        mRemaining = count;
    }
}

#if defined(__APPLE__)

namespace das {
    void SetCurrentThreadName ( const string & str ) {
        pthread_setname_np(str.c_str());
    }

    void SetCurrentThreadPriority ( JobPriority _priority ) {
        int priority = int(_priority);
        float minPlatformPriority = sched_get_priority_min(SCHED_OTHER);
        float maxPlatformPriority = sched_get_priority_max(SCHED_OTHER);
        struct sched_param sched_param;
        int platformPriority = (int)(minPlatformPriority + (maxPlatformPriority - minPlatformPriority)
            * ((float)(priority - int(JobPriority::Minimum)) / (float)(int(JobPriority::Maximum) - int(JobPriority::Minimum))));
        sched_param.sched_priority = platformPriority;
        pthread_setschedparam(pthread_self(), SCHED_OTHER, &sched_param);
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
        default:					DAS_ASSERTF(0, "Windows takes prefixed priority values"); break;
        }
        SetThreadPriority(GetCurrentThread(), winPriority);
    }
}

#elif defined(__linux__) || defined __HAIKU__

#include <pthread.h>

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
}

#else

namespace das
{
    void SetCurrentThreadName ( const string & ) {}
    void SetCurrentThreadPriority ( JobPriority ) {}
}

#endif
