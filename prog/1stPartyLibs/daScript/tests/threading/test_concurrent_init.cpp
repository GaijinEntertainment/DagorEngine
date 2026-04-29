// Test for issue #2027: Race condition in concurrent module registration.
//
// Multiple threads call NEED_ALL_DEFAULT_MODULES + Module::Initialize()
// simultaneously, which triggers SIGSEGV / access violations due to
// unprotected shared state inside module constructors.
//
// Build:  cmake --build build --config Release --target test_concurrent_init
// Run:    bin/Release/test_concurrent_init.exe

#include "daScript/daScript.h"
#include "daScript/misc/platform.h"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>

using namespace das;

// ── helpers ────────────────────────────────────────────────────────────
static std::mutex coutMtx;

template <typename... Args>
static void log(Args&&... args) {
    std::lock_guard<std::mutex> lk(coutMtx);
    (std::cout << ... << std::forward<Args>(args)) << "\n" << std::flush;
}

// Simple spin barrier for C++17
struct SpinBarrier {
    std::atomic<int> count;
    std::atomic<int> generation{0};
    int total;
    SpinBarrier(int n) : count(n), total(n) {}
    void arrive_and_wait() {
        int gen = generation.load(std::memory_order_acquire);
        if (count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // Last thread to arrive — release everyone
            count.store(total, std::memory_order_relaxed);
            generation.fetch_add(1, std::memory_order_release);
        } else {
            // Spin until the generation advances
            while (generation.load(std::memory_order_acquire) == gen) {
                std::this_thread::yield();
            }
        }
    }
};

// ── worker ─────────────────────────────────────────────────────────────
// Register every module that daslang_static uses — the same set the real
// compiler loads.  This maximises the chance of hitting shared-state races
// inside module constructors.
static void workerThread(int id, std::atomic<int>& ok, std::atomic<int>& fail,
                         SpinBarrier* startBarrier = nullptr) {
    try {
        // If a barrier is provided, wait so all threads start simultaneously
        if (startBarrier) {
            startBarrier->arrive_and_wait();
        }

        // Use the same macro as the issue #2027 reporter's repro case
        NEED_ALL_DEFAULT_MODULES;
        NEED_MODULE(Module_UriParser);
        NEED_MODULE(Module_JobQue);
        Module::Initialize();

        // Quick sanity: make sure the "$" (builtin) module exists.
        auto *m = Module::require("$");
        if (!m) {
            log("[Thread ", id, "] FAIL - builtin module not found");
            ++fail;
        } else {
            ++ok;
        }

        Module::Shutdown();
    } catch (const std::exception& e) {
        log("[Thread ", id, "] EXCEPTION: ", e.what());
        ++fail;
    } catch (...) {
        log("[Thread ", id, "] UNKNOWN EXCEPTION");
        ++fail;
    }
}

// ── main ───────────────────────────────────────────────────────────────
int main(int argc, char**) {
    int numThreads = 128;     // default: high thread count
    int numRounds  = 10;      // default: many rounds

    // ── Test 1: sequential (baseline) ──────────────────────────────────
    {
        log("=== Test 1: sequential init (", numThreads, " iterations) ===");
        std::atomic<int> ok{0}, fail{0};
        for (int i = 0; i < numThreads; ++i) {
            workerThread(i, ok, fail);
        }
        log("sequential: ", ok.load(), " ok, ", fail.load(), " fail");
        if (fail.load() != 0) {
            log("FAILED (sequential should never fail)");
            return 1;
        }
        log("=== Test 1 PASSED ===\n");
    }

    // ── Test 2: concurrent with barrier (maximum contention) ───────────
    for (int round = 0; round < numRounds; ++round) {
        const int N = numThreads;
        log("=== Test 2 round ", round, ": concurrent init (", N,
            " threads, barrier sync) ===");
        std::atomic<int> ok{0}, fail{0};
        SpinBarrier startBarrier(N);
        std::vector<std::thread> threads;
        threads.reserve(N);

        for (int i = 0; i < N; ++i) {
            threads.emplace_back(workerThread, 1000 + round * N + i,
                                 std::ref(ok), std::ref(fail), &startBarrier);
        }
        for (auto& t : threads) {
            t.join();
        }

        log("concurrent: ", ok.load(), " ok, ", fail.load(), " fail");
        if (fail.load() != 0) {
            log("FAILED at round ", round);
            return 1;
        }
        log("=== Test 2 round ", round, " PASSED ===\n");
    }

    log("\nAll tests passed: ", numRounds, " rounds x ", numThreads, " threads.");
    return 0;
}
