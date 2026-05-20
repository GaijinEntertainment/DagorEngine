#pragma once

#include "daScript/misc/platform.h"

// Pure-C++ registry for exposing std::shared_ptr<T>-owned C++ objects to das
// as value-sized, strongly-typed, reuse-safe handles. See plan:
// ../../../../plans/jaunty-chasing-crayon.md
//
// No AST dependency. Bindings live in daScript/ast/ast_handle.h.

namespace das {

    template <typename T>
    struct Handle {
        uint64_t value = 0;   // 0 = null/invalid; encodes (gen << 32) | (idx+1)
        Handle () = default;
        Handle ( uint64_t v ) : value(v) {}
        bool operator == ( Handle o ) const { return value == o.value; }
        bool operator != ( Handle o ) const { return value != o.value; }
        explicit operator bool () const { return value != 0; }
        explicit operator uint64_t () const { return value; }
    };

    using HandleLeakDumpFn = void (*)();
    using HandleLeakCountFn = uint64_t (*)();

    // Definitions in src/misc/handle_registry.cpp — single instance across
    // daslang.exe + all dasModule*.shared_module DLLs so registrations from any
    // module are visible at dump time.
    DAS_API vector<HandleLeakDumpFn> & handleRegistry_dumpHooks_impl ();
    DAS_API mutex & handleRegistry_dumpMutex_impl ();
    DAS_API void handleRegistry_registerDump_impl ( HandleLeakDumpFn fn );
    DAS_API void handleRegistry_dumpAll_impl ();
    DAS_API void handleRegistry_registerCount_impl ( HandleLeakCountFn fn );
    DAS_API uint64_t handleRegistry_countAll_impl ();

    inline vector<HandleLeakDumpFn> & handleRegistry_dumpHooks () {
        return handleRegistry_dumpHooks_impl();
    }
    inline mutex & handleRegistry_dumpMutex () {
        return handleRegistry_dumpMutex_impl();
    }
    inline void handleRegistry_registerDump ( HandleLeakDumpFn fn ) {
        handleRegistry_registerDump_impl(fn);
    }
    inline void handleRegistry_dumpAll () {
        handleRegistry_dumpAll_impl();
    }
    inline void handleRegistry_registerCount ( HandleLeakCountFn fn ) {
        handleRegistry_registerCount_impl(fn);
    }
    inline uint64_t handleRegistry_countAll () {
        return handleRegistry_countAll_impl();
    }

    template <typename T>
    class HandleRegistry {
    public:
        Handle<T> acquire ( const shared_ptr<T> & p ) {
            if ( !p ) return Handle<T>{};
            lock_guard<mutex> guard(m);
            uint32_t idx;
            if ( free_head ) {
                idx = free_head - 1;
                free_head = slots[idx].next_free;
            } else {
                idx = uint32_t(slots.size());
                slots.emplace_back();
            }
            Slot & s = slots[idx];
            s.ptr = p;
            s.in_use = true;
            s.next_free = 0;
            Handle<T> h;
            h.value = ( uint64_t(s.generation) << 32 ) | uint64_t(idx + 1);
            return h;
        }
        bool release ( Handle<T> h ) {
            if ( !h.value ) return false;
            lock_guard<mutex> guard(m);
            uint32_t idx = uint32_t(h.value & 0xFFFFFFFFu) - 1;
            uint32_t gen = uint32_t(h.value >> 32);
            if ( idx >= slots.size() ) return false;
            Slot & s = slots[idx];
            if ( !s.in_use || s.generation != gen ) return false;
            s.ptr.reset();
            s.in_use = false;
            s.generation++;
            s.next_free = free_head;
            free_head = idx + 1;
            return true;
        }
        shared_ptr<T> lookup ( Handle<T> h ) const {
            if ( !h.value ) return shared_ptr<T>();
            lock_guard<mutex> guard(m);
            uint32_t idx = uint32_t(h.value & 0xFFFFFFFFu) - 1;
            uint32_t gen = uint32_t(h.value >> 32);
            if ( idx >= slots.size() ) return shared_ptr<T>();
            const Slot & s = slots[idx];
            if ( !s.in_use || s.generation != gen ) return shared_ptr<T>();
            return s.ptr;
        }
        bool is_alive ( Handle<T> h ) const {
            if ( !h.value ) return false;
            lock_guard<mutex> guard(m);
            uint32_t idx = uint32_t(h.value & 0xFFFFFFFFu) - 1;
            uint32_t gen = uint32_t(h.value >> 32);
            if ( idx >= slots.size() ) return false;
            const Slot & s = slots[idx];
            return s.in_use && s.generation == gen;
        }
        size_t live_count () const {
            lock_guard<mutex> guard(m);
            size_t n = 0;
            for ( const Slot & s : slots ) if ( s.in_use ) n++;
            return n;
        }
        template <typename CB>
        void for_each_live ( const CB & cb ) const {
            lock_guard<mutex> guard(m);
            for ( uint32_t i=0; i<slots.size(); ++i ) {
                const Slot & s = slots[i];
                if ( s.in_use ) {
                    Handle<T> h;
                    h.value = ( uint64_t(s.generation) << 32 ) | uint64_t(i + 1);
                    cb(h, s.ptr);
                }
            }
        }
        static HandleRegistry & instance () {
            static HandleRegistry reg;
            return reg;
        }
    private:
        struct Slot {
            shared_ptr<T> ptr;
            uint32_t generation = 1;
            uint32_t next_free = 0;
            bool in_use = false;
        };
        vector<Slot> slots;
        uint32_t free_head = 0;
        mutable mutex m;
    };

}
