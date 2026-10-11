#pragma once

#include <chrono>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

namespace das {
namespace detail {

#if defined(__EMSCRIPTEN__)
// src/misc/ARCHITECTURE.md#spin-window-clock-stride
struct JobQueSpinClock {
    using rep = double;
    using period = std::milli;
    using duration = std::chrono::duration<rep, period>;
    static constexpr bool is_steady = false;
    using time_point = std::chrono::time_point<JobQueSpinClock, duration>;
    static time_point now() {
        return time_point(duration(emscripten_get_now()));
    }
};
#else
using JobQueSpinClock = std::chrono::steady_clock;
#endif

// src/misc/ARCHITECTURE.md#spin-window-clock-stride
template <typename Clock>
class JobQueSpinDeadline {
    using Duration = typename Clock::time_point::duration;
public:
    explicit JobQueSpinDeadline(int spinUs)
        : mSpinUs(spinUs)
        , mDeadline(spinUs > 0
            ? Clock::now() + std::chrono::duration_cast<Duration>(std::chrono::microseconds(spinUs))
            : typename Clock::time_point()) {}

    bool expired(bool servedTeamWork) {
        if (mSpinUs <= 0) return true;
        auto now = Clock::now();
        if (servedTeamWork) {
            mDeadline = now + std::chrono::duration_cast<Duration>(std::chrono::microseconds(mSpinUs));
            return false;
        }
        return now >= mDeadline;
    }

private:
    int mSpinUs;
    typename Clock::time_point mDeadline;
};

} // namespace detail
} // namespace das
