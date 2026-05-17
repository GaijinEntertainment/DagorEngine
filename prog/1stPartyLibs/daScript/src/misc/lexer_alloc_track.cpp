#include "daScript/misc/platform.h"

#if DAS_TRACK_ALLOC

#include "daScript/misc/lexer_alloc_track.h"

#include <unordered_map>
#include <mutex>
#include <string>

namespace das {

    namespace {
        using StringMap = std::unordered_map<std::string*, std::string>;

        // Placement-new into static storage so the dtor never runs — atexit
        // dump fires after Meyers-singleton teardown. Mirrors getMutex() in
        // alloc_tracker.cpp.
        std::mutex & lexer_track_mu() {
            alignas(std::mutex) static unsigned char storage[sizeof(std::mutex)];
            static std::mutex *m = ::new (storage) std::mutex();
            return *m;
        }
        StringMap & lexer_track_map() {
            alignas(StringMap) static unsigned char storage[sizeof(StringMap)];
            static StringMap *m = ::new (storage) StringMap();
            return *m;
        }

        // Map rehash inside lexer_track_alloc frees old buckets, which routes
        // through track_free_hook -> lexer_track_free and would re-lock our
        // non-recursive mutex.
        thread_local bool tl_inside = false;
        struct ReentryGuard {
            ReentryGuard() { tl_inside = true; }
            ~ReentryGuard() { tl_inside = false; }
        };
    }

    void lexer_track_alloc(std::string *p, const char *tokenText) noexcept {
        if (!p || tl_inside) return;
        ReentryGuard g;
        std::lock_guard<std::mutex> lk(lexer_track_mu());
        lexer_track_map()[p] = tokenText ? tokenText : "";
    }

    void lexer_track_free(std::string *p) noexcept {
        if (!p || tl_inside) return;
        ReentryGuard g;
        std::lock_guard<std::mutex> lk(lexer_track_mu());
        auto & m = lexer_track_map();
        auto it = m.find(p);
        if (it != m.end()) m.erase(it);
    }

    int dump_lexer_string_leaks(FILE *out) noexcept {
        ReentryGuard g;
        std::lock_guard<std::mutex> lk(lexer_track_mu());
        auto & m = lexer_track_map();
        if (m.empty()) return 0;
        fprintf(out, "\n=== leaked lexer NAME tokens (%zu) ===\n", m.size());
        for (auto & kv : m) {
            fprintf(out, "  %p \"%s\"\n", (void*)kv.first, kv.second.c_str());
        }
        fprintf(out, "=== end leaked lexer NAME tokens ===\n");
        return (int)m.size();
    }

}

#endif // DAS_TRACK_ALLOC
