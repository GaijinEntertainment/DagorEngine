/*
Copyright (c) 2017, Insomniac Games
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <cstdint>

namespace CacheSim
{
  /// Initializes the API. Only call once.
  void Init();

  /// Returns a thread ID suitable for use with CachesimSetThreadCoreMapping
  uint64_t GetCurrentThreadId();

  /// Set what Jaguar core (0-7) this Win32 thread ID will map to.
  /// Threads without a jaguar core id will not be recorded, so you'll need to set up atleast one.
  /// A core id of -1 will disable recording the thread (e.g., upon thread completion)
  void SetThreadCoreMapping(uint64_t thread_id, int logical_core_id);

  /// Resets thread mapping that were already set.
  void ResetThreadCoreMappings();

  /// Start recording a capture, buffering it to memory.
  bool StartCapture();

  /// Stop recording and optionally save the capture to disk.
  void EndCapture(bool save);

  /// Remove the exception handler machinery.
  void RemoveHandler(void);
}