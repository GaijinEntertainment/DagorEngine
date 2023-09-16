#include "daScript/misc/platform.h"

#include "daScript/misc/sysos.h"

#if defined(_MSC_VER) && !defined(_GAMING_XBOX) && !defined(_DURANGO)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    namespace das {

        __forceinline void setBits ( DWORD64 & dw, int lowBit, int bits, int newValue ) {
            DWORD64 mask = (DWORD64(1) << bits) - 1;
            dw = (dw & ~(mask << lowBit)) | (DWORD64(newValue) << lowBit);
        }

        __forceinline void setBits ( DWORD & dw, int lowBit, int bits, int newValue ) {
            DWORD mask = (DWORD(1) << bits) - 1;
            dw = (dw & ~(mask << lowBit)) | (DWORD(newValue) << lowBit);
        }

        bool g_isVHSet = false;
        void ( * g_HwBpHandler ) ( int, void * ) = nullptr;

        LONG NTAPI VEH_handler ( struct _EXCEPTION_POINTERS* ExceptionInfo ) {
            if ( ExceptionInfo->ContextRecord->Dr6 & 0x0f ) {
                if ( g_HwBpHandler ) {
                    if ( ExceptionInfo->ContextRecord->Dr6 & 1 ) g_HwBpHandler(0, (void *) ExceptionInfo->ContextRecord->Dr0 );
                    if ( ExceptionInfo->ContextRecord->Dr6 & 2 ) g_HwBpHandler(1, (void *) ExceptionInfo->ContextRecord->Dr1 );
                    if ( ExceptionInfo->ContextRecord->Dr6 & 4 ) g_HwBpHandler(2, (void *) ExceptionInfo->ContextRecord->Dr2 );
                    if ( ExceptionInfo->ContextRecord->Dr6 & 8 ) g_HwBpHandler(3, (void *) ExceptionInfo->ContextRecord->Dr3 );
                }
                return EXCEPTION_CONTINUE_EXECUTION;
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }

        void hwSetBreakpointHandler ( void (* handler ) ( int, void * ) ) {
            g_HwBpHandler = handler;
        }

        int hwBreakpointSet ( void * address, int len, int when ) {
            if ( !g_isVHSet ) {
                g_isVHSet = true;
                AddVectoredExceptionHandler(1, VEH_handler);
            }
            CONTEXT cxt;
	        HANDLE thisThread = GetCurrentThread();
        	cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            if ( !GetThreadContext(thisThread, &cxt) ) return -1;
            int bp_index = 0;
            for ( bp_index=0; bp_index!=4; ++bp_index ) {
                uint32_t mask = uint32_t(1) << (bp_index*2);
        		if ( (uint32_t(cxt.Dr7) & mask) == 0u ) {
                    break;
                }
            }
            switch ( bp_index ) {
            case 0:     cxt.Dr0 = intptr_t(address); break;
            case 1:     cxt.Dr1 = intptr_t(address); break;
            case 2:     cxt.Dr2 = intptr_t(address); break;
            case 3:     cxt.Dr3 = intptr_t(address); break;
            default:    return -1;
            }
	        setBits(cxt.Dr7, 16 + (bp_index*4), 2, when);
	        setBits(cxt.Dr7, 18 + (bp_index*4), 2, len);
	        setBits(cxt.Dr7, bp_index*2,        1, 1);
        	if ( !SetThreadContext(thisThread, &cxt) ) return -1;
            return bp_index;
        }

        bool hwBreakpointClear ( int bp_index ) {
            if ( bp_index==-1 ) return false;
            CONTEXT cxt;
            HANDLE thisThread = GetCurrentThread();
            cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            if (!GetThreadContext(thisThread, &cxt)) return false;
            setBits(cxt.Dr7, bp_index*2, 1, 0);
            if (!SetThreadContext(thisThread, &cxt)) return false;
            return true;
        }

        size_t getExecutablePathName(char* pathName, size_t pathNameCapacity) {
            return GetModuleFileNameA(NULL, pathName, (DWORD)pathNameCapacity);
        }
        void * loadDynamicLibrary ( const char * fileName ) {
            return LoadLibraryA(fileName);
        }
        void * getFunctionAddress ( void * module, const char * func ) {
            return GetProcAddress(HMODULE(module),func);
        }
        void * getLibraryHandle ( const char * moduleName ) {
            return GetModuleHandleA(moduleName);
        }
        string normalizeFileName ( const char * fileName ) {
            char buffer[MAX_PATH ];
            auto ret = GetFullPathNameA(fileName,MAX_PATH,buffer,nullptr);
            return ret ? buffer : "";
        }
    }
#elif defined(__linux__) || defined(_EMSCRIPTEN_VER)
    #include <unistd.h>
    #include <dlfcn.h>
    namespace das {
        void hwSetBreakpointHandler ( void (*) ( int, void * ) ) { }
        int hwBreakpointSet ( void *, int, int ) {
            return -1;
        }
        bool hwBreakpointClear ( int ) {
            return false;
        }
        size_t getExecutablePathName(char* pathName, size_t pathNameCapacity) {
            size_t pathNameSize = readlink("/proc/self/exe", pathName, pathNameCapacity - 1);
            pathName[pathNameSize] = '\0';
            return pathNameSize;
        }
        void * loadDynamicLibrary ( const char * lib ) {
            return dlopen(lib,RTLD_LAZY);
        }
        void * getFunctionAddress ( void * lib, const char * name ) {
            return lib ? dlsym(lib, name) : nullptr;
        }
        void * getLibraryHandle ( const char * lib ) {
            return dlopen(lib,RTLD_LAZY);
        }
        string normalizeFileName ( const char * fileName ) {
            // TODO: implement
            return "";
        }
    }
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <mach/mach_types.h>
    #include <mach/mach_init.h>
    #include <mach/thread_act.h>
    #include <pthread.h>
    #include <limits.h>
    #include <signal.h>
    #include <dlfcn.h>
    #include <inttypes.h>
    namespace das {

        /*
        // ARM64 debug registers
        __uint64_t __bvr[16];
        __uint64_t __bcr[16];
        __uint64_t __wvr[16];
        __uint64_t __wcr[16];
        __uint64_t __mdscr_el1;
        */

        __forceinline void setBits ( uint64_t & dw, int lowBit, int bits, int newValue ) {
            uint64_t mask = (uint64_t(1) << bits) - 1;
            dw = (dw & ~(mask << lowBit)) | (uint64_t(newValue) << lowBit);
        }

        static bool g_isHandlerSet = false;
        static void ( * g_HwBpHandler ) ( int, void * ) = nullptr;

        #if defined(__arm64__)

            #define ARM64_NUM_WP    16

            static uint64_t g_HwBpAddress = 0;
            static int      g_HwBpMask = 0;

            __forceinline int fls(int x) {
                return x ? sizeof(x) * 8 - __builtin_clz(x) : 0;
            }
            __forceinline int ffs(int x)   {
                return __builtin_ffs(x);
            }
            uint64_t get_distance_from_watchpoint (uint64_t addr, uint64_t val,	int len ) {
	            uint64_t wp_low, wp_high;
	            uint32_t lens, lene;
	            lens = ffs(len);
	            lene = fls(len);
	            wp_low = val + lens;
	            wp_high = val + lene;
	            if (addr < wp_low)
		            return wp_low - addr;
	            else if (addr > wp_high)
		            return addr - wp_high;
	            else
		            return 0;
            }

            uint64_t encode_ctrl_reg ( HwBpSize len, HwBpType type, bool enabled ) {
                uint64_t val = 0;
                if ( enabled ) val |= 0x1ul;
                switch ( len ) {
                    case HwBp_1: val |= (0x01) << 5; break;
                    case HwBp_2: val |= (0x03) << 5; break;
                    case HwBp_4: val |= (0x0f) << 5; break;
                    case HwBp_8: val |= (0xff) << 5; break;
                }
                switch ( type ) {
                    case HwBp_Execute:      val |= (4  )<<3;    break;
                    case HwBp_Write:        val |= (2  )<<3;    break;  // write=2
                    case HwBp_ReadWrite:    val |= (1|2)<<3;    break;  // read=1
                }
                uint64_t priv = 0;
                val |= priv << 1;
                return val;
            }
        #endif

        void sigTrapHandler (int sig, siginfo_t* siginfo, void * ptr) {
            thread_t mythread = mach_thread_self();
        #if defined(__x86_64__)
            struct x86_debug_state dr;
            mach_msg_type_number_t dr_count = x86_DEBUG_STATE_COUNT;
            thread_get_state(mythread, x86_DEBUG_STATE, (thread_state_t) &dr, &dr_count);
            if ( dr.uds.ds64.__dr6 & 0x0f ) {
                if ( g_HwBpHandler ) {
                    if ( dr.uds.ds64.__dr6 & 1 ) g_HwBpHandler(0, (void *) dr.uds.ds64.__dr0 );
                    if ( dr.uds.ds64.__dr6 & 2 ) g_HwBpHandler(1, (void *) dr.uds.ds64.__dr1 );
                    if ( dr.uds.ds64.__dr6 & 4 ) g_HwBpHandler(2, (void *) dr.uds.ds64.__dr2 );
                    if ( dr.uds.ds64.__dr6 & 8 ) g_HwBpHandler(3, (void *) dr.uds.ds64.__dr3 );
                }
            }
        #elif defined(__arm64__)
            arm_debug_state64_t dr;
            mach_msg_type_number_t dr_count = ARM_DEBUG_STATE64_COUNT;
            thread_get_state(mythread, ARM_DEBUG_STATE64, (thread_state_t) &dr, &dr_count);
            uint64_t min_dist = -1ul, dist;
            int closest_match = 0;
            ucontext_t * ucontext = (ucontext_t *) ptr;
            if ( g_HwBpAddress ) {   // if we are in single step mode
                // drop hw bp mode mode
                g_HwBpAddress = 0;
                auto hw_bp_index = 0;
                dr.__bcr[hw_bp_index] = 0;
                // re-enable breakpoints
                for ( int i=0; i!=ARM64_NUM_WP; ++i ) {
                    if ( g_HwBpMask & (1<<i) ) {
                        dr.__wcr[i] |= 1;
                    }
                }
                // and done
                thread_set_state(mythread, ARM_DEBUG_STATE64, (thread_state_t) &dr, dr_count);
            } else {
                // find which breakpoint caused it
                uint64_t addr = ucontext->uc_mcontext->__es.__far;
                for ( int wpi=0; wpi!=ARM64_NUM_WP; ++wpi ) {
                    uint64_t val = dr.__wvr[wpi];
                    uint64_t ctrl_reg = dr.__wcr[wpi];
                    bool enabled = ctrl_reg & 1;
                    int len = (ctrl_reg>>5) & 0xff;
                    if ( enabled ) {
                        dist = get_distance_from_watchpoint(addr, val, len);
                        if (dist < min_dist) {
                            min_dist = dist;
                            closest_match = wpi;
                        }
                        if ( dist==0 ) break;
                    }
                }
                // call handler of that breakpoint
                g_HwBpHandler(closest_match, (void*)addr);
                // save hw breakpoints in the mask, disable all hw breakpoints
                g_HwBpMask = 0;
                for ( int i=0; i!=ARM64_NUM_WP; ++i ) {
                    if ( dr.__wcr[i] & 1 ) {
                        g_HwBpMask |= 1<<i;
                        dr.__wcr[i] &= ~1ul;
                    }
                }
                // set hw breakpoint on next instruction
                uint32_t control_value = (0xfu << 5) | 7;
                g_HwBpAddress = ( ucontext->uc_mcontext->__ss.__pc + 4 ) & ~0x3ul;
                auto hw_bp_index = 0;
                if ( dr.__bcr[hw_bp_index] & 0x1ul ) {
                    DAS_VERIFYF(false, "hw bp already set");
                } else {
                    dr.__bcr[hw_bp_index] = control_value;
                    dr.__bvr[hw_bp_index] = g_HwBpAddress;
                }
                // and done
                thread_set_state(mythread, ARM_DEBUG_STATE64, (thread_state_t) &dr, dr_count);
            }
        #endif
        }

        void hwSetBreakpointHandler ( void (* handler ) ( int, void * ) ) {
            g_HwBpHandler = handler;
        }

        int hwBreakpointSet ( void * address, int len, int when ) {
            if ( !g_isHandlerSet ) {
                g_isHandlerSet = true;
                struct sigaction act;
                memset (&act, 0, sizeof(act));
                act.sa_sigaction = sigTrapHandler;
                act.sa_flags = SA_SIGINFO|SA_ONSTACK;
                if ( sigaction(SIGTRAP, &act, 0)!=0 ) {
                    g_isHandlerSet = false;
                    return -1;
                }
                if ( sigaction(SIGINT, &act, 0)!=0 ) {
                    g_isHandlerSet = false;
                    return -1;
                }
            }
        #if defined(__x86_64__)
            thread_t mythread = mach_thread_self();
            struct x86_debug_state dr;
            mach_msg_type_number_t dr_count = x86_DEBUG_STATE_COUNT;
            thread_get_state(mythread, x86_DEBUG_STATE, (thread_state_t) &dr, &dr_count);
            int bp_index = 0;
            for ( bp_index=0; bp_index!=4; ++bp_index ) {
                uint32_t mask = uint32_t(1) << (bp_index*2);
        		if ( (uint32_t(dr.uds.ds64.__dr7) & mask) == 0u ) {
                    break;
                }
            }
            switch ( bp_index ) {
            case 0:     dr.uds.ds64.__dr0 = intptr_t(address); break;
            case 1:     dr.uds.ds64.__dr1 = intptr_t(address); break;
            case 2:     dr.uds.ds64.__dr2 = intptr_t(address); break;
            case 3:     dr.uds.ds64.__dr3 = intptr_t(address); break;
            default:    return -1;
            }
	        setBits(dr.uds.ds64.__dr7, 16 + (bp_index*4), 2, when);
	        setBits(dr.uds.ds64.__dr7, 18 + (bp_index*4), 2, len);
	        setBits(dr.uds.ds64.__dr7, bp_index*2,        1, 1);
            dr_count = x86_DEBUG_STATE_COUNT;
            thread_set_state(mythread, x86_DEBUG_STATE, (thread_state_t) &dr, dr_count);
            return bp_index;
        #elif defined(__arm64__)
            thread_t mythread = mach_thread_self();
            arm_debug_state64_t dr;
            mach_msg_type_number_t dr_count = ARM_DEBUG_STATE64_COUNT;
            thread_get_state(mythread, ARM_DEBUG_STATE64, (thread_state_t) &dr, &dr_count);
            int bp_index = 0;
            for ( bp_index=0; bp_index!=ARM64_NUM_WP; ++bp_index ) {
                uint64_t ctrl_reg = dr.__wcr[bp_index];
                bool enabled = ctrl_reg & 1;
                if ( !enabled ) {
                    break;
                }
            }
            if ( bp_index<0 || bp_index>=ARM64_NUM_WP ) return -1;
            dr.__wvr[bp_index] = intptr_t(address);
            dr.__wcr[bp_index] = encode_ctrl_reg(HwBpSize(len),HwBpType(when),true);
            thread_set_state(mythread, ARM_DEBUG_STATE64, (thread_state_t) &dr, dr_count);
            return bp_index;
        #else
            return -1;
        #endif
        }

        bool hwBreakpointClear ( int bp_index ) {
            if ( bp_index==-1 ) return false;
        #if defined(__x86_64__)
            thread_t mythread = mach_thread_self();
            struct x86_debug_state dr;
            mach_msg_type_number_t dr_count = x86_DEBUG_STATE_COUNT;
            thread_get_state(mythread, x86_DEBUG_STATE, (thread_state_t) &dr, &dr_count);
            setBits(dr.uds.ds64.__dr7, bp_index*2, 1, 0);
            dr_count = x86_DEBUG_STATE_COUNT;
            thread_set_state(mythread, x86_DEBUG_STATE, (thread_state_t) &dr, dr_count);
            return true;
        #elif defined(__arm64__)
            thread_t mythread = mach_thread_self();
            arm_debug_state64_t dr;
            mach_msg_type_number_t dr_count = ARM_DEBUG_STATE64_COUNT;
            thread_get_state(mythread, ARM_DEBUG_STATE64, (thread_state_t) &dr, &dr_count);
            dr.__wcr[bp_index] = encode_ctrl_reg(HwBp_1,HwBp_ReadWrite,false);
            thread_set_state(mythread, ARM_DEBUG_STATE64, (thread_state_t) &dr, dr_count);
        #endif
            return false;
        }

        size_t getExecutablePathName(char* pathName, size_t pathNameCapacity) {
            uint32_t pathNameSize = 0;
            _NSGetExecutablePath(NULL, &pathNameSize);
            if (pathNameSize > pathNameCapacity)
                pathNameSize = pathNameCapacity;
            if (!_NSGetExecutablePath(pathName, &pathNameSize)) {
                char real[PATH_MAX];
                if (realpath(pathName, real) != NULL) {
                    pathNameSize = strlen(real);
                    memcpy(pathName, real, pathNameSize+1);
                }
                return pathNameSize;
            }
            return 0;
        }
        void * loadDynamicLibrary ( const char * lib ) {
            return dlopen(lib,RTLD_LAZY);
        }
        void * getFunctionAddress ( void * lib, const char * name ) {
            return lib ? dlsym(lib, name) : nullptr;
        }
        void * getLibraryHandle ( const char * lib ) {
            return dlopen(lib,RTLD_LAZY);
        }
        string normalizeFileName ( const char * fileName ) {
            // TODO: implement
            return "";
        }
    }
#else
    namespace das {
        void hwSetBreakpointHandler ( void (*) ( int, void * ) ) { }
        int hwBreakpointSet ( void *, int, int ) {
            return -1;
        }
        bool hwBreakpointClear ( int ) {
            return false;
        }
        size_t getExecutablePathName(char*, size_t) {
            DAS_FATAL_ERROR("platforms without getExecutablePathName should not use default getDasRoot");
            return 0;
        }
        void * loadDynamicLibrary ( const char *  ) {
            // TODO: implement
            return nullptr;
        }
        void * getFunctionAddress ( void *, const char * ) {
            // TODO: implement
            return nullptr;
        }
        void * getLibraryHandle ( const char *  ) {
            // TODO: implement
            return nullptr;
        }
        string normalizeFileName ( const char * fileName ) {
            // TODO: implement
            return "";
        }
    }
#endif

namespace das {
    string getExecutableFileName ( void ) {
        char buffer[1024];
        return getExecutablePathName(buffer,1024) ? buffer : "";
    }

    string get_prefix ( const string & req ) {
        auto np = req.find_last_of("\\/");
        if ( np != string::npos ) {
            return req.substr(0,np);
        } else {
            return req;
        }
    }

    string get_suffix ( const string & req ) {
        auto np = req.find_last_of("\\/");
        if ( np != string::npos ) {
            return req.substr(np+1);
        } else {
            return req;
        }
    }

    string g_dasRoot;

    void setDasRoot ( const string & dr ) {
        g_dasRoot = dr;
    }

    string getDasRoot ( void ) {
        if ( g_dasRoot.empty() ) {
            string efp = getExecutableFileName();   // ?/bin/debug/binary.exe
            auto np = efp.find_last_of("\\/");
            if ( np != string::npos ) {
                auto ep = get_prefix(efp);          // remove file name
                auto suffix = get_suffix(ep);
                if ( suffix != "bin" ) {
                    ep = get_prefix(ep);            // remove debug
                }
                if ( get_suffix(ep)!="bin" ) {
                    g_dasRoot = ".";
                } else {
                    g_dasRoot = get_prefix(ep);     // remove bin
                }
            } else {
                g_dasRoot = ".";
            }
        }
        return g_dasRoot;
    }
}
