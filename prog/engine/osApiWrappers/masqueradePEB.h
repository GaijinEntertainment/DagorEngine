// https://github.com/multikill/TLS_Reader

// based on http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FThread%2FTEB.html
// https://github.com/kevoreilly/capemon/commit/aa1fd55c13b09aca976d0864470f05e31a2f7794
// see https://github.com/bigzz/WRK.git

typedef LONG NTSTATUS;
#define RTL_MAX_DRIVE_LETTERS    32
#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60
#define GDI_BATCH_BUFFER_SIZE    310

#if !defined(_M_X64)
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#endif

typedef ULONG GDI_HANDLE_BUFFER32[GDI_HANDLE_BUFFER_SIZE32];
typedef ULONG GDI_HANDLE_BUFFER64[GDI_HANDLE_BUFFER_SIZE64];
typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];

typedef struct _UNICODE_STRING
{
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;


typedef struct _STRING
{
  USHORT Length;
  USHORT MaximumLength;
  PCHAR Buffer;
} STRING;
typedef STRING *PSTRING;

typedef struct _CLIENT_ID
{
  HANDLE UniqueProcess;
  HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _CLIENT_ID64
{
  ULONG64 UniqueProcess;
  ULONG64 UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;

typedef struct _LDR_DATA_TABLE_ENTRY_COMPATIBLE
{
  LIST_ENTRY InLoadOrderLinks;
  LIST_ENTRY InMemoryOrderLinks;
  union
  {
    LIST_ENTRY InInitializationOrderLinks;
    LIST_ENTRY InProgressLinks;
  } DUMMYUNION0;
  PVOID DllBase;
  PVOID EntryPoint;
  ULONG SizeOfImage;
  UNICODE_STRING FullDllName;
  UNICODE_STRING BaseDllName;
  union
  {
    ULONG Flags;
    struct
    {
      ULONG PackagedBinary : 1;          // Size=4 Offset=104 BitOffset=0 BitCount=1
      ULONG MarkedForRemoval : 1;        // Size=4 Offset=104 BitOffset=1 BitCount=1
      ULONG ImageDll : 1;                // Size=4 Offset=104 BitOffset=2 BitCount=1
      ULONG LoadNotificationsSent : 1;   // Size=4 Offset=104 BitOffset=3 BitCount=1
      ULONG TelemetryEntryProcessed : 1; // Size=4 Offset=104 BitOffset=4 BitCount=1
      ULONG ProcessStaticImport : 1;     // Size=4 Offset=104 BitOffset=5 BitCount=1
      ULONG InLegacyLists : 1;           // Size=4 Offset=104 BitOffset=6 BitCount=1
      ULONG InIndexes : 1;               // Size=4 Offset=104 BitOffset=7 BitCount=1
      ULONG ShimDll : 1;                 // Size=4 Offset=104 BitOffset=8 BitCount=1
      ULONG InExceptionTable : 1;        // Size=4 Offset=104 BitOffset=9 BitCount=1
      ULONG ReservedFlags1 : 2;          // Size=4 Offset=104 BitOffset=10 BitCount=2
      ULONG LoadInProgress : 1;          // Size=4 Offset=104 BitOffset=12 BitCount=1
      ULONG LoadConfigProcessed : 1;     // Size=4 Offset=104 BitOffset=13 BitCount=1
      ULONG EntryProcessed : 1;          // Size=4 Offset=104 BitOffset=14 BitCount=1
      ULONG ProtectDelayLoad : 1;        // Size=4 Offset=104 BitOffset=15 BitCount=1
      ULONG ReservedFlags3 : 2;          // Size=4 Offset=104 BitOffset=16 BitCount=2
      ULONG DontCallForThreads : 1;      // Size=4 Offset=104 BitOffset=18 BitCount=1
      ULONG ProcessAttachCalled : 1;     // Size=4 Offset=104 BitOffset=19 BitCount=1
      ULONG ProcessAttachFailed : 1;     // Size=4 Offset=104 BitOffset=20 BitCount=1
      ULONG CorDeferredValidate : 1;     // Size=4 Offset=104 BitOffset=21 BitCount=1
      ULONG CorImage : 1;                // Size=4 Offset=104 BitOffset=22 BitCount=1
      ULONG DontRelocate : 1;            // Size=4 Offset=104 BitOffset=23 BitCount=1
      ULONG CorILOnly : 1;               // Size=4 Offset=104 BitOffset=24 BitCount=1
      ULONG ChpeImage : 1;               // Size=4 Offset=104 BitOffset=25 BitCount=1
      ULONG ReservedFlags5 : 2;          // Size=4 Offset=104 BitOffset=26 BitCount=2
      ULONG Redirected : 1;              // Size=4 Offset=104 BitOffset=28 BitCount=1
      ULONG ReservedFlags6 : 2;          // Size=4 Offset=104 BitOffset=29 BitCount=2
      ULONG CompatDatabaseProcessed : 1; // Size=4 Offset=104 BitOffset=31 BitCount=1
    };
  } ENTRYFLAGSUNION;
  WORD ObsoleteLoadCount;
  WORD TlsIndex;
  union
  {
    LIST_ENTRY HashLinks;
    struct
    {
      PVOID SectionPointer;
      ULONG CheckSum;
    };
  } DUMMYUNION1;
  union
  {
    ULONG TimeDateStamp;
    PVOID LoadedImports;
  } DUMMYUNION2;
  // fields below removed for compatibility
} LDR_DATA_TABLE_ENTRY_COMPATIBLE, *PLDR_DATA_TABLE_ENTRY_COMPATIBLE;
typedef LDR_DATA_TABLE_ENTRY_COMPATIBLE LDR_DATA_TABLE_ENTRY;

typedef LDR_DATA_TABLE_ENTRY *PCLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA
{
  ULONG Length;
  BOOLEAN Initialized;
  HANDLE SsHandle;
  LIST_ENTRY InLoadOrderModuleList;
  LIST_ENTRY InMemoryOrderModuleList;
  LIST_ENTRY InInitializationOrderModuleList;
  PVOID EntryInProgress;
  BOOLEAN ShutdownInProgress;
  HANDLE ShutdownThreadId;
} PEB_LDR_DATA, *PPEB_LDR_DATA;


typedef struct _CURDIR
{
  UNICODE_STRING DosPath;
  HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
  USHORT Flags;
  USHORT Length;
  ULONG TimeStamp;
  STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;


typedef struct _RTL_USER_PROCESS_PARAMETERS
{
  ULONG MaximumLength;
  ULONG Length;

  ULONG Flags;
  ULONG DebugFlags;

  HANDLE ConsoleHandle;
  ULONG ConsoleFlags;
  HANDLE StandardInput;
  HANDLE StandardOutput;
  HANDLE StandardError;

  CURDIR CurrentDirectory;
  UNICODE_STRING DllPath;
  UNICODE_STRING ImagePathName;
  UNICODE_STRING CommandLine;
  PVOID Environment;

  ULONG StartingX;
  ULONG StartingY;
  ULONG CountX;
  ULONG CountY;
  ULONG CountCharsX;
  ULONG CountCharsY;
  ULONG FillAttribute;

  ULONG WindowFlags;
  ULONG ShowWindowFlags;
  UNICODE_STRING WindowTitle;
  UNICODE_STRING DesktopInfo;
  UNICODE_STRING ShellInfo;
  UNICODE_STRING RuntimeData;
  RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];

  ULONG EnvironmentSize;
  ULONG EnvironmentVersion;
  PVOID PackageDependencyData; // 8+
  ULONG ProcessGroupId;
  // ULONG LoaderThreads;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB
{
  BOOLEAN InheritedAddressSpace;
  BOOLEAN ReadImageFileExecOptions;
  BOOLEAN BeingDebugged;
  union
  {
    BOOLEAN BitField;
    struct
    {
      BOOLEAN ImageUsesLargePages : 1;
      BOOLEAN IsProtectedProcess : 1;
      BOOLEAN IsImageDynamicallyRelocated : 1;
      BOOLEAN SkipPatchingUser32Forwarders : 1;
      BOOLEAN IsPackagedProcess : 1;
      BOOLEAN IsAppContainer : 1;
      BOOLEAN IsProtectedProcessLight : 1;
      BOOLEAN IsLongPathAwareProcess : 1;
    };
  };
  HANDLE Mutant;

  PVOID ImageBaseAddress;
  PPEB_LDR_DATA Ldr;
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  PVOID SubSystemData;
  PVOID ProcessHeap;
  PRTL_CRITICAL_SECTION FastPebLock;
  PVOID AtlThunkSListPtr;
  PVOID IFEOKey;
  union
  {
    ULONG CrossProcessFlags;
    struct
    {
      ULONG ProcessInJob : 1;
      ULONG ProcessInitializing : 1;
      ULONG ProcessUsingVEH : 1;
      ULONG ProcessUsingVCH : 1;
      ULONG ProcessUsingFTH : 1;
      ULONG ProcessPreviouslyThrottled : 1;
      ULONG ProcessCurrentlyThrottled : 1;
      ULONG ReservedBits0 : 25;
    };
    ULONG EnvironmentUpdateCount;
  };
  union
  {
    PVOID KernelCallbackTable;
    PVOID UserSharedInfoPtr;
  };
  ULONG SystemReserved[1];
  ULONG AtlThunkSListPtr32;
  PVOID ApiSetMap;
  ULONG TlsExpansionCounter;
  PVOID TlsBitmap;
  ULONG TlsBitmapBits[2];
  PVOID ReadOnlySharedMemoryBase;
  PVOID HotpatchInformation;
  PVOID *ReadOnlyStaticServerData;
  PVOID AnsiCodePageData;
  PVOID OemCodePageData;
  PVOID UnicodeCaseTableData;

  ULONG NumberOfProcessors;
  ULONG NtGlobalFlag;

  LARGE_INTEGER CriticalSectionTimeout;
  SIZE_T HeapSegmentReserve;
  SIZE_T HeapSegmentCommit;
  SIZE_T HeapDeCommitTotalFreeThreshold;
  SIZE_T HeapDeCommitFreeBlockThreshold;

  ULONG NumberOfHeaps;
  ULONG MaximumNumberOfHeaps;
  PVOID *ProcessHeaps;

  PVOID GdiSharedHandleTable;
  PVOID ProcessStarterHelper;
  ULONG GdiDCAttributeList;

  PRTL_CRITICAL_SECTION LoaderLock;

  ULONG OSMajorVersion;
  ULONG OSMinorVersion;
  USHORT OSBuildNumber;
  USHORT OSCSDVersion;
  ULONG OSPlatformId;
  ULONG ImageSubsystem;
  ULONG ImageSubsystemMajorVersion;
  ULONG ImageSubsystemMinorVersion;
  ULONG_PTR ImageProcessAffinityMask;
  GDI_HANDLE_BUFFER GdiHandleBuffer;
  PVOID PostProcessInitRoutine;

  PVOID TlsExpansionBitmap;
  ULONG TlsExpansionBitmapBits[32];

  ULONG SessionId;

  ULARGE_INTEGER AppCompatFlags;
  ULARGE_INTEGER AppCompatFlagsUser;
  PVOID pShimData;
  PVOID AppCompatInfo;

  UNICODE_STRING CSDVersion;

  PVOID ActivationContextData;
  PVOID ProcessAssemblyStorageMap;
  PVOID SystemDefaultActivationContextData;
  PVOID SystemAssemblyStorageMap;

  SIZE_T MinimumStackCommit;

  PVOID *FlsCallback;
  LIST_ENTRY FlsListHead;
  PVOID FlsBitmap;
  ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)];
  ULONG FlsHighIndex;

  PVOID WerRegistrationData;
  PVOID WerShipAssertPtr;
  PVOID pContextData;
  PVOID pImageHeaderHash;
  union
  {
    ULONG TracingFlags;
    struct
    {
      ULONG HeapTracingEnabled : 1;
      ULONG CritSecTracingEnabled : 1;
      ULONG LibLoaderTracingEnabled : 1;
      ULONG SpareTracingBits : 29;
    };
  };
  ULONGLONG CsrServerReadOnlySharedMemoryBase;
} PEB, *PPEB;

typedef struct _GDI_TEB_BATCH
{
  ULONG Offset;
  UCHAR Alignment[4];
  ULONG_PTR HDC;
  ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT
{
  ULONG Flags;
  PSTR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME
{
  ULONG Flags;
  struct _TEB_ACTIVE_FRAME *Previous;
  PTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

typedef struct _TEB
{
  NT_TIB NtTib;

  PVOID EnvironmentPointer;
  CLIENT_ID ClientId;
  PVOID ActiveRpcHandle;
  PVOID ThreadLocalStoragePointer;
  PPEB ProcessEnvironmentBlock;

  ULONG LastErrorValue;
  ULONG CountOfOwnedCriticalSections;
  PVOID CsrClientThread;
  PVOID Win32ThreadInfo;
  ULONG User32Reserved[26];
  ULONG UserReserved[5];
  PVOID WOW32Reserved;
  LCID CurrentLocale;
  ULONG FpSoftwareStatusRegister;
  PVOID SystemReserved1[54];
  NTSTATUS ExceptionCode;
  PVOID ActivationContextStackPointer;
#if defined(_M_X64)
  UCHAR SpareBytes[24];
#else
  UCHAR SpareBytes[36];
#endif
  ULONG TxFsContext;

  GDI_TEB_BATCH GdiTebBatch;
  CLIENT_ID RealClientId;
  HANDLE GdiCachedProcessHandle;
  ULONG GdiClientPID;
  ULONG GdiClientTID;
  PVOID GdiThreadLocalInfo;
  ULONG_PTR Win32ClientInfo[62];
  PVOID glDispatchTable[233];
  ULONG_PTR glReserved1[29];
  PVOID glReserved2;
  PVOID glSectionInfo;
  PVOID glSection;
  PVOID glTable;
  PVOID glCurrentRC;
  PVOID glContext;

  NTSTATUS LastStatusValue;
  UNICODE_STRING StaticUnicodeString;
  WCHAR StaticUnicodeBuffer[261];

  PVOID DeallocationStack;
  PVOID TlsSlots[64];
  LIST_ENTRY TlsLinks;

  PVOID Vdm;
  PVOID ReservedForNtRpc;
  PVOID DbgSsReserved[2];

  ULONG HardErrorMode;
#if defined(_M_X64)
  PVOID Instrumentation[11];
#else
  PVOID Instrumentation[9];
#endif
  GUID ActivityId;

  PVOID SubProcessTag;
  PVOID EtwLocalData;
  PVOID EtwTraceData;
  PVOID WinSockData;
  ULONG GdiBatchCount;

  union
  {
    PROCESSOR_NUMBER CurrentIdealProcessor;
    ULONG IdealProcessorValue;
    struct
    {
      UCHAR ReservedPad0;
      UCHAR ReservedPad1;
      UCHAR ReservedPad2;
      UCHAR IdealProcessor;
    };
  };

  ULONG GuaranteedStackBytes;
  PVOID ReservedForPerf;
  PVOID ReservedForOle;
  ULONG WaitingOnLoaderLock;
  PVOID SavedPriorityState;
  ULONG_PTR SoftPatchPtr1;
  PVOID ThreadPoolData;
  PVOID *TlsExpansionSlots;
#if defined(_M_X64)
  PVOID DeallocationBStore;
  PVOID BStoreLimit;
#endif
  ULONG MuiGeneration;
  ULONG IsImpersonating;
  PVOID NlsCache;
  PVOID pShimData;
  ULONG HeapVirtualAffinity;
  HANDLE CurrentTransactionHandle;
  PTEB_ACTIVE_FRAME ActiveFrame;
  PVOID FlsData;

  PVOID PreferredLanguages;
  PVOID UserPrefLanguages;
  PVOID MergedPrefLanguages;
  ULONG MuiImpersonation;

  union
  {
    USHORT CrossTebFlags;
    USHORT SpareCrossTebBits : 16;
  };
  union
  {
    USHORT SameTebFlags;
    struct
    {
      USHORT SafeThunkCall : 1;
      USHORT InDebugPrint : 1;
      USHORT HasFiberData : 1;
      USHORT SkipThreadAttach : 1;
      USHORT WerInShipAssertCode : 1;
      USHORT RanProcessInit : 1;
      USHORT ClonedThread : 1;
      USHORT SuppressDebugMsg : 1;
      USHORT DisableUserStackWalk : 1;
      USHORT RtlExceptionAttached : 1;
      USHORT InitialThread : 1;
      USHORT SpareSameTebBits : 1;
    };
  };

  PVOID TxnScopeEnterCallback;
  PVOID TxnScopeExitCallback;
  PVOID TxnScopeContext;
  ULONG LockCount;
  ULONG SpareUlong0;
  PVOID ResourceRetValue;
} TEB, *PTEB;

typedef LONG KPRIORITY;
typedef ULONG_PTR KAFFINITY;

typedef struct _THREAD_BASIC_INFORMATION
{
  NTSTATUS ExitStatus;
  PVOID TebBaseAddress;
  CLIENT_ID ClientId;
  KAFFINITY AffinityMask;
  KPRIORITY Priority;
  KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef enum EnumThreadInfoClass
{
  ThreadBasicInformation,
  ThreadTimes,
  ThreadPriority,
  ThreadBasePriority,
  ThreadAffinityMask,
  ThreadImpersonationToken,
  ThreadDescriptorTableEntry,
  ThreadEnableAlignmentFaultFixup,
  ThreadEventPair_Reusable,
  ThreadQuerySetWin32StartAddress,
  ThreadZeroTlsCell,
  ThreadPerformanceCount,
  ThreadAmILastThread,
  ThreadIdealProcessor,
  ThreadPriorityBoost,
  ThreadSetTlsArrayAddress,
  ThreadIsIoPending,
  ThreadHideFromDebugger,
  ThreadBreakOnTermination,
  ThreadSwitchLegacyState,
  ThreadIsTerminated,
  MaxThreadInfoClass
} THREADINFOCLASS;

extern "C"
{
  NTSYSAPI
  NTSTATUS
  NTAPI
  NtQueryInformationThread_PROTOTYPE(IN HANDLE ThreadHandle, IN THREADINFOCLASS ThreadInformationClass, OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength, OUT PULONG ReturnLength);
}
typedef NTSTATUS(WINAPI *pNtQueryInformationThread)(HANDLE, LONG, PVOID, ULONG, PULONG);

// main article:
// https://github.com/multikill/TLS_Reader

// http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FThread%2FTEB.html
// https://github.com/kevoreilly/capemon/commit/aa1fd55c13b09aca976d0864470f05e31a2f7794
// see https://github.com/bigzz/WRK.git

// https://guidedhacking.com/threads/thread-local-storage-tls-variants-how-to-read-them.17036/
// https://github.com/multikill/TLS_Reader

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#ifndef TLS_MINIMUM_AVAILABLE
#define TLS_MINIMUM_AVAILABLE 64
#endif
#ifndef TLS_EXPANSION_SLOTS
#define TLS_EXPANSION_SLOTS 1024
#endif

static _TEB *get_thread_pteb(pNtQueryInformationThread NtQueryInformationThread, HANDLE thread)
{
  if (!NtQueryInformationThread)
    return NULL;
  THREAD_BASIC_INFORMATION basicInfo = {0};
  auto ret = NtQueryInformationThread(thread, ThreadBasicInformation, &basicInfo, sizeof(basicInfo), NULL);
  if (!NT_SUCCESS(ret))
    return NULL;
  return (_TEB *)basicInfo.TebBaseAddress;
}

/*
//#include <winternl.h>

#if 1
//http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FThread%2FTEB.html
//https://github.com/kevoreilly/capemon/commit/aa1fd55c13b09aca976d0864470f05e31a2f7794
//see https://github.com/bigzz/WRK.git
//and
//https://github.com/3gstudent/test/blob/master/MasqueradePEB.cpp
//highly based on undocumented NTAPI functions
//http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FThread%2FTEB.html

#include "masqueradePEB.h"

static bool IsLoaderLockActive()
{
  _TEB* teb2 = (_TEB*)NtCurrentTeb();
  if (teb2->WaitingOnLoaderLock)
    return true;

}

#elif 0
// https://microsoft.public.win32.programmer.kernel.narkive.com/qxCAoEXI/using-rtllookupfunctionentry-for-profiling

#if defined(_WIN64)
extern "C" void* __fastcall __getPEB();
#else
inline __declspec(naked) void* __getPEB()
{
  __asm
  {
    mov eax, fs:[0x30]
    ret
  }
}
#endif

extern "C"
{
NTSYSAPI BOOL NTAPI RtlTryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
NTSYSAPI LONG NTAPI RtlLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
}

bool IsLoaderLockActive()
{
#if defined(_M_AMD64)
  RTL_CRITICAL_SECTION* pLoaderLock(*(RTL_CRITICAL_SECTION**)(((BYTE*)__getPEB())+0x0110));//Undocumented
#elif defined(_M_IX86)
  RTL_CRITICAL_SECTION* pLoaderLock(*(RTL_CRITICAL_SECTION**)(((BYTE*)__getPEB())+0x00a0));
#else
  #pragma error("unknown target")
#endif
  if (!RtlTryEnterCriticalSection(pLoaderLock)) // from ntdll.dll
    return true;
  RtlLeaveCriticalSection(pLoaderLock);
  return false;
}
#else
//https://www.geoffchappell.com/studies/windows/win32/ntdll/api/ldrapi/lockloaderlock.htm
extern "C"
{
//NTSTATUS
  NTSYSAPI ULONG NTAPI LdrLockLoaderLock (    ULONG Flags,    ULONG *State,    ULONG *Cookie);
  NTSYSAPI ULONG NTAPI LdrUnlockLoaderLock (ULONG Flags, ULONG Cookie);
}
static bool IsLoaderLockActive()
{
  ULONG Cookie, State;
  if (LdrLockLoaderLock(0x02,//do not wait, return if busy
          &State, &Cookie) != 0)
    return true;
  if (State == 0x02)//the loader lock was busy, the function did not wait
    return true;
  if (State == 0x01)//the function entered the loader lock
    LdrUnlockLoaderLock(0, Cookie);
  return false;
}
#endif
*/