#pragma once

#include <stdint.h>
#include <sys/types.h>

struct DabuildJobSharedMem
{
  unsigned fullMemSize;
  unsigned pid;
  intptr_t mapHandle;
  unsigned jobCount;
#if _TARGET_PC_WIN
  void *jobHandle[64];
#else
  pid_t jobHandle[64];
#endif
  int logLevel;
  bool quiet;
  bool nopbar;
  bool showImportantWarnings;
  bool dryRun;
  bool stopOnFirstError;
  bool stripD3Dres;
  bool collapsePacks;
  bool expTex, expRes;

  volatile int cmdGen, respGen;

  struct JobCtx
  {
    volatile int state; // 0=waiting, 1=assigned, 2=accepted, 3=done, 4=inoperable, 5=inoperable-accepted
    volatile int cmd; // 0=nop, 1=build tex, 2=build texpack, 3=build respack, 7=reload common hash and prepare packs, 9=prepare packs
    volatile int result;
    volatile unsigned targetCode;
    volatile char profileName[32];
    volatile int pkgId, packId;
    volatile int donePk, totalPk;
    volatile int64_t szDiff;
    volatile int64_t szChangedTotal;
    char data[1024 - 8 * 4 - 8 * 2 - 32];

    void setup(unsigned tc, const char *prof);
  };
  alignas(4096) JobCtx jobCtx[64];

  unsigned forceRebuildAssetIdxCount, forceRebuildAssetIdx[0x4000 - 1];

  void changeCtxState(JobCtx &ctx, int state);
};

class DabuildJobPool
{
public:
  typedef DabuildJobSharedMem::JobCtx Ctx;

  DabuildJobPool(DabuildJobSharedMem *_m) : m(_m)
  {
    jobs = _m ? _m->jobCount : 0;
    cJobIdx = 0;
    respGen = 0;
  }

  // returns pointer to context of waiting job; when all jobs are busy waits for completion at most timeout_msec
  // NULL may be returned only when all jobs are busy for timeout_msec
  Ctx *getAvailableJobId(int timeout_msec);

  // returns true when all jobs completed; optional out_done pointer is set with single completed context
  bool waitAllJobsDone(int timeout_msec, Ctx **out_done);

  // marks job context as ready-to-be-processed
  void startJob(Ctx *ctx, int cmd);

  // marks all contexts to process broadcast_cmd; returns false if one or more jobs were busy
  bool startAllJobs(int broadcast_cmd, unsigned target_code, const char *profile);

public:
  DabuildJobSharedMem *m;
  int jobs;
  int cJobIdx;
  int respGen;
};
