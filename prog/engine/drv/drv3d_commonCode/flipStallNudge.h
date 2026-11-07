// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <windows.h>

// Flip stall nudge (workaround for windows DWM present mode switch issues).
//
// After an alt-tab, DWM can put the swapchain back into independent flip
// with a broken flip queue. Our flips never complete, DWM keeps the back
// buffers, and the GPU waits until the window loses focus.
//
// Fix: show a 1x1 topmost window over the game. DWM has to compose again
// and cancels the stuck flips. The window hides 1 s after the last stall.
//
// Usage:
//   init_flip_nudge()      - with the swapchain. Reads the kill switches and
//                            starts the thread that owns the window and runs
//                            a heartbeat for stalls that no wait sees
//                            (e.g. inside Present).
//   flip_nudge_on_frame()  - once per frame, main thread. Tracks frame time
//                            and activation.
//   wait_with_flip_nudge() - replaces WaitForSingleObject in every wait the
//                            stall can block: frame latency and latched frame.
//   shutdown_flip_nudge()  - with the swapchain. Stops the thread.
//
// Mechanism:
//
// init launches a thread, that creates a 1x1 window as a cancel-trigger.
// wait_with_flip_nudge checks how much time we consider a missed timeout,
// and if eligible, shows the window and waits again.
//
// Criteria for deciding how much waiting is too much (and we need a nudge):
// - Are we in 1s window since the last app activation?
// - What has the average frame time been lately?
// The longer the average frame is, the more timeout we consider ok, and
// in the activation window we allow shorter timeouts.
//
// In order to support this logic, flip_nudge_on_frame must be called in
// screen update for frame stats.
//
// For the case where drivers don't use timed-out waits for latency,
// the thread also has a heartbeat timer that wakes up every 100ms,
// and if it finds itself in the activation window and not having recent
// frames, it also nudges the DWM.
//
// init_flip_nudge and shutdown_flip_nudge must be externally synced with
// all flip_nudge_on_frame/wait_with_flip_nudge calls inbetween.
// flip_nudge_on_frame and wait_with_flip_nudge take a critsec internally,
// so they don't require additional external sync.
//
// Kill switches: directx/flipStallNudge, directx/flipStallNudgeHeartbeat.
//
// @NOTE: currently used only in dx11 and dx12, since vulkan handles presentation slightly differently.
// However, in case of DWM-related hangs with vulkan this workaround should be extended and reused there.

void init_flip_nudge();
void flip_nudge_on_frame(HWND window);
DWORD wait_with_flip_nudge(HANDLE handle, DWORD timeout_ms, const char *what);
void shutdown_flip_nudge();
