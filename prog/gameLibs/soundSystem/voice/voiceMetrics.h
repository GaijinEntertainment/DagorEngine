// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "voiceRecorder.h"
#include "voiceSourceControl.h"
#include <generic/dag_span.h>

namespace voicechat::metrics
{

struct Capture
{
  VoiceRecorder::Stats sessionCounts;
  int sessionLateReaderDroppedMs = 0;
  MicLevel level;
  float recordVolume = 0.0f;
  int recordDeviceId = -1;
  bool recording = false;
};

#if DAGOR_DBGLEVEL > 0
// constraint: any thread; 0 while the overlay window does not draw, so a collapsed window ends the session
int active_session();
// constraint: the capture thread and a communicator's owner thread pass active_session() at least once a second;
// publish_playback keys a row by the communicator pointer, which stays one communicator's while it publishes
void publish_capture(int session, const Capture &capture);
void publish_playback(const void *communicator, int session, dag::ConstSpan<VoiceSourceControl::Metrics> sources);
#else
inline int active_session() { return 0; }
inline void publish_capture(int, const Capture &) {}
inline void publish_playback(const void *, int, dag::ConstSpan<VoiceSourceControl::Metrics>) {}
#endif

} // namespace voicechat::metrics
