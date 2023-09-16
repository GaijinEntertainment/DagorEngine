#pragma once

/*
    based on ideas from https://stackoverflow.com/a/38478032

    this is NOT a general purpose replacement of std::function
    this concept wraps around lambda, which is passed on the stack, and only exists at eval time, i.e
        foo ( callable([&](...){}) )
    this is as close to daScript block, as we can get
*/

namespace das {

    template <typename T>
    class callable;

    template <typename R, typename... Args>
    class callable<R(Args...)> {
        typedef R (*invoke_fn_t)(const char*, Args...);
        template <typename Functor>
        static __forceinline R invoke_fn(Functor* fn, Args... args) {
            return (*fn)(das::forward<Args>(args)...);
        }
        invoke_fn_t     invoke_f;
        const char *    data_ptr;
    public:
        callable(callable && rhs) = delete;
        callable(callable const& rhs) = delete;
        __forceinline callable() = default;
        template <typename Functor>
        __forceinline callable(const Functor & f)
            : invoke_f(reinterpret_cast<invoke_fn_t>(invoke_fn<Functor>))
            , data_ptr(reinterpret_cast<const char *>(&f)) {
        }
        __forceinline R operator()(Args... args) const {
            return this->invoke_f(this->data_ptr, das::forward<Args>(args)...);
        }
    };

    template <typename... Args>
    class callable<void(Args...)> {
        typedef void (*invoke_fn_t)(const char*, Args...);
        template <typename Functor>
        static __forceinline void invoke_fn(Functor* fn, Args... args) {
            (*fn)(das::forward<Args>(args)...);
        }
        invoke_fn_t     invoke_f;
        const char *    data_ptr;
    public:
        callable(callable && rhs) = delete;
        callable(callable const& rhs) = delete;
        __forceinline callable() = default;
        template <typename Functor>
        __forceinline callable(const Functor & f)
            : invoke_f(reinterpret_cast<invoke_fn_t>(invoke_fn<Functor>))
            , data_ptr(reinterpret_cast<const char *>(&f)) {
        }
        __forceinline void operator()(Args... args) const {
            this->invoke_f(this->data_ptr, das::forward<Args>(args)...);
        }
    };
}

