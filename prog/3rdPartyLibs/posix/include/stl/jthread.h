// https://codereview.stackexchange.com/questions/282843/jthread-drop-in-replacement
#pragma once

#ifndef __cpp_lib_jthread
#include <thread>
#include <memory>
#include <functional>

//#include "defines.h"

namespace std
{
struct StopState;

struct nostopstate_t {};
constexpr auto nostopstate = nostopstate_t{};

class stop_token
{
public:
    stop_token();
    explicit stop_token(std::shared_ptr<StopState> state);
    stop_token(const stop_token& other) noexcept;
    stop_token(stop_token&& other) noexcept;
    ~stop_token();

    stop_token& operator = (const stop_token& other) noexcept;
    stop_token& operator = (stop_token&& other) noexcept;

    [[nodiscard]] bool stop_requested() noexcept;
    [[nodiscard]] bool stop_possible() const noexcept;

    void swap(stop_token& other) noexcept;

private:
    std::shared_ptr<StopState> state;

friend class stop_callback;
};

inline void swap(stop_token& lhs, stop_token& rhs) noexcept
{
    lhs.swap(rhs);
}

class stop_source
{
public:
    stop_source();
    explicit stop_source(nostopstate_t nss) noexcept;
    stop_source(const stop_source& other) noexcept;
    stop_source(stop_source&& other) noexcept;
    ~stop_source();

    stop_source& operator = (const stop_source& other) noexcept;
    stop_source& operator = (stop_source&& other) noexcept;

    [[nodiscard]] stop_token get_token() const noexcept;

    bool request_stop() noexcept;
    [[nodiscard]] bool stop_possible() const noexcept;

    void swap(stop_source& other) noexcept;

private:
    std::shared_ptr<StopState> state;
};

inline void swap(stop_source& lhs, stop_source& rhs) noexcept
{
    lhs.swap(rhs);
}

class stop_callback
{
public:
    template<class C>
    explicit stop_callback(const stop_token& st, C&& cb)
    : token(st), callback(cb)
    {
        self_register();
    }

    template<class C>
    explicit stop_callback(stop_token&& st, C&& cb )
    : token(std::forward<stop_token>(st)), callback(cb)
    {
        self_register();
    }

    ~stop_callback();

private:
    stop_token             token;
    std::function<void ()> callback;

    void self_register();

    stop_callback(const stop_callback&) = delete;
    stop_callback(stop_callback&&) = delete;
    stop_callback& operator = (const stop_callback& other) noexcept = delete;
    stop_callback& operator = (stop_callback&& other) noexcept = delete;

friend struct StopState;
};

//! Drop in replacement for all cases where std::jthread is not yet implemented.
//!
//! @see std::jthread
class jthread
{
public:
    using id =  std::thread::id;

    jthread() noexcept;
    jthread(jthread&& other) noexcept;

    template<class Function, class... Args>
    requires std::is_invocable_v<std::decay_t<Function>, std::decay_t<Args>...>
    explicit jthread(Function&& f, Args&&... args)
    : impl(std::forward<Function>(f), std::forward<Args>(args)...) {}

    template<class Function, class... Args>
    requires std::is_invocable_v<std::decay_t<Function>, stop_token, std::decay_t<Args>...>
    explicit jthread(Function&& f, Args&&... args)
    : impl(std::forward<Function>(f), stop.get_token(), std::forward<Args>(args)...) {}

    ~jthread();

    jthread& operator = (jthread&& other) noexcept;

    [[nodiscard]] id get_id() const noexcept;
    [[nodiscard]] bool joinable() const noexcept;

    void join();
    void detach();

    [[nodiscard]] stop_source get_stop_source() noexcept;
    [[nodiscard]] stop_token get_stop_token() noexcept;
    bool request_stop() noexcept;

    void swap(jthread& other) noexcept;

private:
    stop_source stop = {};
    std::thread impl;

    jthread(const jthread&) = delete;
    jthread& operator = (const jthread& other) noexcept = delete;
};

inline void swap(jthread& lhs, jthread& rhs) noexcept
{
    lhs.swap(rhs);
}

struct StopState
{
    std::atomic<bool> request_stop = false;

    // this is not efficient, but it only needs to be correct
    std::mutex callbacks_mutex;
    std::vector<stop_callback*> callbacks;

    void add_callback(stop_callback* callback)
    {
        auto lk = std::unique_lock<std::mutex>(callbacks_mutex);
        if (!request_stop)
        {
            callbacks.push_back(callback);
        }
        else
        {
            callback->callback();
        }
    }

    void remove_callback(stop_callback* callback)
    {
        auto lk = std::unique_lock<std::mutex>(callbacks_mutex);
        auto i = std::find(begin(callbacks), end(callbacks), callback);
        if (i != end(callbacks))
        {
            callbacks.erase(i);
        }
    }

    void exec_callbacks()
    {
        auto lk = std::unique_lock<std::mutex>(callbacks_mutex);
        for (const auto& cb : callbacks)
        {
            assert(cb->callback);
            cb->callback();
        }
        callbacks.clear();
    }
};

stop_token::stop_token() = default;

stop_token::stop_token(std::shared_ptr<StopState> s)
: state(s) {}

stop_token::stop_token(const stop_token& other) noexcept = default;
stop_token::stop_token(stop_token&& other) noexcept = default;
stop_token::~stop_token() = default;

stop_token& stop_token::operator = (const stop_token& other) noexcept = default;
stop_token& stop_token::operator = (stop_token&& other) noexcept = default;

bool stop_token::stop_requested() noexcept
{
    if (state)
    {
        return state->request_stop;
    }
    return false;
}

bool stop_token::stop_possible() const noexcept
{
    return static_cast<bool>(state);
}

void stop_token::swap(stop_token& other) noexcept
{
    state.swap(other.state);
}

stop_source::stop_source()
: state(std::make_shared<StopState>()) {}

stop_source::stop_source(nostopstate_t nss) noexcept {}

stop_source::stop_source(const stop_source& other) noexcept = default;
stop_source::stop_source(stop_source&& other) noexcept = default;
stop_source::~stop_source() = default;

stop_source& stop_source::operator = (const stop_source& other) noexcept = default;
stop_source& stop_source::operator = (stop_source&& other) noexcept = default;

stop_token stop_source::get_token() const noexcept
{
    return stop_token{state};
}

bool stop_source::request_stop() noexcept
{
    if (state && (state->request_stop.exchange(true) == false))
    {
        state->exec_callbacks();
        return true;
    }
    return false;
}

bool stop_source::stop_possible() const noexcept
{
    return static_cast<bool>(state);
}

void stop_source::swap(stop_source& other) noexcept
{
    state.swap(other.state);
}

stop_callback::~stop_callback()
{
    if (token.state)
    {
        token.state->remove_callback(this);
    }
}

void stop_callback::self_register()
{
    if (token.state)
    {
        token.state->add_callback(this);
    }
    else
    {
        callback();
    }
}

jthread::jthread() noexcept
: stop{nostopstate} {}

jthread::jthread(jthread&& other) noexcept = default;

jthread::~jthread()
{
    if (joinable())
    {
        request_stop();
        join();
    }
}

jthread& jthread::operator = (jthread&& other) noexcept = default;

jthread::id jthread::get_id() const noexcept
{
    return impl.get_id();
}

bool jthread::joinable() const noexcept
{
    return impl.joinable();
}

void jthread::join()
{
    impl.join();
}

void jthread::detach()
{
    impl.detach();
}

stop_source jthread::get_stop_source() noexcept
{
    if (joinable())
    {
        return stop;
    }
    else
    {
        return stop_source{nostopstate};
    }
}

stop_token jthread::get_stop_token() noexcept
{
    return get_stop_source().get_token();
}

bool jthread::request_stop() noexcept
{
    return stop.request_stop();
}

void jthread::swap(jthread& other) noexcept
{
    stop.swap(other.stop);
    impl.swap(other.impl);
}
}
#endif
