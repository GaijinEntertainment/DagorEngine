//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <util/dag_stdint.h>
#include <generic/dag_tab.h>
#include <dag/dag_vectorMap.h>
#include <EASTL/string_view.h>

class WinCritSec;

namespace streamio
{

// Some of possible values that passed as 'error' in completion callback
enum
{
  ERR_OK = 0,
  ERR_NOT_MODIFIED = 32, // Note: implementation detail - not used CURL error code
  ERR_UNKNOWN = -1,
  ERR_ABORTED = -2,
};

enum class ProcessResult
{
  Consumed,
  Discarded,
  IoError
};

typedef dag::VectorMap<eastl::string_view, eastl::string_view> StringMap;

typedef void (*completion_cb_t)(const char *name, int error, IGenLoad *ptr, void *cb_arg, int64_t last_modified, intptr_t req_id);
typedef ProcessResult (*stream_data_cb_t)(dag::Span<const char> data, void *cb_arg, intptr_t req_id);
typedef void (*resp_headers_cb_t)(StringMap const &resp_headers, void *cb_arg);
typedef void (*progress_cb_t)(const char *name, size_t dltotal, size_t dlnow);

class Context
{
public:
  virtual ~Context() {}

  // Create stream capable reading http(s)/ftp(s) files
  // If name doesn't look like url, file stream will be created instead.
  // Callback might get called before function return
  // Caller should delete returned stream when it no longer needed
  // If modified_since >= 0 then request callback might get called with errcode ERR_NOT_MODIFIED
  virtual intptr_t createStream(const char *name, completion_cb_t complete_cb, stream_data_cb_t stream_cb,
    resp_headers_cb_t resp_headers_cb, progress_cb_t progress_cb, void *cb_arg, int64_t modified_since = -1, bool do_sync = false) = 0;
  virtual void poll() = 0;
  virtual void abort() = 0;
  virtual void abort_request(intptr_t req_id) = 0;
};

struct CreationParams
{
  int jobMgr = -1; // if < 0 then job manager will be created automatically

  struct Timeouts
  {
    int reqTimeoutSec = 20;    // total request timeout in seconds (including time for connect). 0 - unlimited
    int connectTimeoutSec = 5; // timeout for network connection

    // lower limit of average transfer speed during lowSpeedTime below that connection considered to slow
    // and transfer is aborted
    int lowSpeedLimitBps = 0; // bytes per second
    int lowSpeedTimeSec = 0;
  } timeouts;

  int connectTimeoutSec = -1; // timeout for connect to server only (expected to be less than timeoutSec)
  WinCritSec *csMgr = NULL, *csJob = NULL;
};

Context *create(CreationParams *params = NULL);

}; // namespace streamio
