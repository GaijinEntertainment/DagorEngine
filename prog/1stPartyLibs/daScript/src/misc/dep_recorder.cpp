// The rule is merged and written here, in the compiler, rather than by the build system. A check
// runs several daslang processes - a dastest worker per batch, one invocation per suite, a
// `daslang -compile-only` per file - and each knows only what it read, so they merge under a
// lock file beside the depfile. Doing it here is what makes the depfile complete on its own:
// every build system just points at it, and none has to reimplement the merge.
// src/misc/ARCHITECTURE.md#dependency-recording

#include "daScript/misc/platform.h"

#include "daScript/misc/dep_recorder.h"
#include "daScript/misc/env_cfg.h"
#include "daScript/misc/sysos.h"

#if !DAS_NO_FILEIO
#include <stdio.h>
#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/file.h>
#endif
#endif

namespace das {

    int das_dep_flag_argc ( const char * arg ) {
        if ( !arg ) return 0;
        if ( strcmp(arg, "-MD")==0 ) return 1;
        return (strcmp(arg, "-MF")==0 || strcmp(arg, "-MT")==0) ? 2 : 0;
    }

    bool das_dep_is_env_entry ( const char * entry ) {
        return entry && strncmp(entry, "DAS_DEPFILE", 11)==0;
    }

#if DAS_NO_FILEIO

    void das_dep_arm ( const char *, const char * ) {}
    void das_dep_note_read ( const char * ) {}
    void das_dep_flush () {}

#else

    namespace {
        mutex               g_lock;
        das_set<string>     g_reads;
        vector<string>      g_excludes;
        string              g_path;
        string              g_target;
        bool                g_flushed = false;
        atomic<int>         g_state {0};

        string depNormal ( const char * path ) {
            string full = normalizeFileName(path);
            if ( full.empty() ) full = path;
            replace(full.begin(), full.end(), '\\', '/');
            return full;
        }

        void depSetup ( const char * path, const char * target ) {
            g_path = path;
            g_target = (target && target[0]) ? string(target) : g_path;
            if ( !target || !target[0] ) {
                auto len = g_target.length();
                if ( len>2 && g_target.compare(len-2, 2, ".d")==0 ) g_target.resize(len-2);
                replace(g_target.begin(), g_target.end(), '\\', '/');
            }
            for ( const char * ch = get_dasenv_depfile_exclude(); ch && *ch; ) {
                const char * next = strchr(ch, ';');
                string one(ch, next ? size_t(next-ch) : strlen(ch));
                if ( !one.empty() ) g_excludes.push_back(depNormal(one.c_str()));
                ch = next ? next+1 : nullptr;
            }
            g_state.store(2, std::memory_order_release);
            atexit(das_dep_flush);
        }

        bool depRecording () {
            if ( int state = g_state.load(std::memory_order_acquire) ) return state==2;
            lock_guard<mutex> guard(g_lock);
            if ( !g_state.load(std::memory_order_relaxed) ) {
                const char * path = get_dasenv_depfile();
                if ( path && path[0] ) depSetup(path, get_dasenv_depfile_target());
                else g_state.store(1, std::memory_order_release);
            }
            return g_state.load(std::memory_order_relaxed)==2;
        }

        bool depExcluded ( const string & key ) {
            for ( const auto & one : g_excludes ) {
                if ( key.compare(0, one.length(), one)==0
                    && (key.length()==one.length() || key[one.length()]=='/') ) return true;
            }
            return false;
        }

        struct DepFileLock {
#if defined(_WIN32)
            HANDLE handle;
            explicit DepFileLock ( const string & path ) {
                handle = CreateFileA((path + ".lock").c_str(), GENERIC_READ|GENERIC_WRITE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                OVERLAPPED ov = {};
                if ( handle != INVALID_HANDLE_VALUE ) LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov);
            }
            ~DepFileLock () {
                if ( handle == INVALID_HANDLE_VALUE ) return;
                OVERLAPPED ov = {};
                UnlockFileEx(handle, 0, MAXDWORD, MAXDWORD, &ov);
                CloseHandle(handle);
            }
#else
            int fd;
            explicit DepFileLock ( const string & path ) {
                fd = open((path + ".lock").c_str(), O_CREAT|O_RDWR, 0666);
                if ( fd >= 0 ) flock(fd, LOCK_EX);
            }
            ~DepFileLock () {
                if ( fd >= 0 ) { flock(fd, LOCK_UN); close(fd); }
            }
#endif
        };

        void depAppendLines ( const string & path, const das_set<string> & lines ) {
            string text;
            for ( const auto & one : lines ) { text += one; text += "\n"; }
            if ( FILE * f = das_fopen_utf8(path.c_str(), "ab") ) {
                fwrite(text.c_str(), 1, text.length(), f);
                fclose(f);
            }
        }

        void depReadLines ( const string & path, das_set<string> & into ) {
            FILE * f = das_fopen_utf8(path.c_str(), "rb");
            if ( !f ) return;
            char line[4096];
            while ( fgets(line, sizeof(line), f) ) {
                string one(line);
                while ( !one.empty() && (one.back()=='\n' || one.back()=='\r') ) one.pop_back();
                if ( !one.empty() ) into.insert(one);
            }
            fclose(f);
        }
    }

    void das_dep_arm ( const char * depFilePath, const char * target ) {
        if ( !depFilePath || !depFilePath[0] ) return;
        lock_guard<mutex> guard(g_lock);
        depSetup(depFilePath, target);
    }

    void das_dep_note_read ( const char * path ) {
        if ( !path || !path[0] || !depRecording() ) return;
        string key = depNormal(path);
        lock_guard<mutex> guard(g_lock);
        if ( !depExcluded(key) ) g_reads.insert(das::move(key));
    }

    void das_dep_flush () {
        lock_guard<mutex> guard(g_lock);
        if ( g_state.load(std::memory_order_relaxed)!=2 || g_flushed ) return;
        g_flushed = true;
        string listPath = g_path + ".list";
        DepFileLock fileLock(g_path);
        depAppendLines(listPath, g_reads);
        das_set<string> inputs;
        depReadLines(listPath, inputs);
        vector<string> sorted;
        for ( const auto & one : inputs ) {
            if ( FILE * f = das_fopen_utf8(one.c_str(), "rb") ) {
                fclose(f);
                sorted.push_back(one);
            }
        }
        sort(sorted.begin(), sorted.end());
        string rule = g_target + ":";
        for ( const auto & one : sorted ) {
            rule += " \\\n  ";
            for ( char ch : one ) {
                if ( ch==' ' || ch=='\t' || ch=='#' ) rule += '\\';
                if ( ch=='$' ) rule += '$';
                rule += ch;
            }
        }
        rule += "\n";
        if ( FILE * f = das_fopen_utf8(g_path.c_str(), "wb") ) {
            fwrite(rule.c_str(), 1, rule.length(), f);
            fclose(f);
        }
    }

#endif
}
