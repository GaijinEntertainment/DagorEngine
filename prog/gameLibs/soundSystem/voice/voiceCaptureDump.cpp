// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceCaptureDump.h"
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_console.h>
#include <util/dag_string.h>
#include <EASTL/algorithm.h>
#include <EASTL/utility.h>
#include <stdlib.h>

namespace voicechat
{

static const int max_dump_seconds = 60;
static volatile int requested_seconds = 0;

int take_capture_dump_request() { return interlocked_exchange(requested_seconds, 0); }

CaptureDump::CaptureDump(int capture_rate, int frame_samples, int output_rate, int seconds) :
  captureRate(capture_rate), frameSamples(frame_samples), outputRate(output_rate), limitSamples(size_t(capture_rate) * seconds)
{
  mic.reserve(limitSamples + frame_samples);
}

void CaptureDump::onReferencePop(dag::ConstSpan<int16_t> frame)
{
  if (!frame.empty())
  {
    reference.insert(reference.end(), frame.begin(), frame.end());
    ++pendingFed;
    return;
  }
  frames.push_back(Frame{pendingFed});
  pendingFed = 0;
}

void CaptureDump::onProcessed(dag::ConstSpan<int16_t> capture, dag::ConstSpan<int16_t> output, float record_volume)
{
  mic.insert(mic.end(), capture.begin(), capture.end());
  out.insert(out.end(), output.begin(), output.end());
  // with the echo canceller off the chain pops no reference, so its frames took none
  frames.resize(mic.size() / frameSamples);
  if (framesProcessed < frames.size())
    frames[framesProcessed].prerollDropped = eastl::exchange(prerollDropPending, false);
  for (; framesProcessed < frames.size(); ++framesProcessed)
    frames[framesProcessed].recordVolume = record_volume;
}

static bool write_wav(const char *path, const eastl::vector<int16_t> &pcm, int rate)
{
  file_ptr_t f = df_open(path, DF_WRITE | DF_CREATE);
  if (!f)
    return false;
  const uint32_t dataSize = pcm.size() * sizeof(int16_t), riffSize = 36 + dataSize, fmtSize = 16, byteRate = rate * 2;
  const uint32_t sampleRate = rate;
  const uint16_t pcmFormat = 1, channels = 1, align = 2, bits = 16;
  bool ok = df_write(f, "RIFF", 4) == 4 && df_write(f, &riffSize, 4) == 4 && df_write(f, "WAVEfmt ", 8) == 8 &&
            df_write(f, &fmtSize, 4) == 4 && df_write(f, &pcmFormat, 2) == 2 && df_write(f, &channels, 2) == 2 &&
            df_write(f, &sampleRate, 4) == 4 && df_write(f, &byteRate, 4) == 4 && df_write(f, &align, 2) == 2 &&
            df_write(f, &bits, 2) == 2 && df_write(f, "data", 4) == 4 && df_write(f, &dataSize, 4) == 4 &&
            df_write(f, pcm.data(), dataSize) == int(dataSize);
  df_close(f);
  return ok;
}

bool CaptureDump::write(const char *prefix) const
{
  String feedText;
  for (const Frame &frame : frames)
    feedText.aprintf(32, "%d %.9g %d\n", frame.fed, frame.recordVolume, frame.prerollDropped ? 1 : 0);
  file_ptr_t f = df_open(String(0, "%s_feed.txt", prefix), DF_WRITE | DF_CREATE);
  if (!f)
    return false;
  const bool feedOk = df_write(f, feedText.data(), feedText.length()) == int(feedText.length());
  df_close(f);
  return feedOk && write_wav(String(0, "%s_mic.wav", prefix), mic, captureRate) &&
         write_wav(String(0, "%s_far.wav", prefix), reference, captureRate) &&
         write_wav(String(0, "%s_out.wav", prefix), out, outputRate);
}

} // namespace voicechat

static bool capture_dump_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME_EX("voice", "dump_capture", 1, 2, "records the capture chain for the voiceProcessing WAV mode", "[seconds]")
  {
    const int seconds = eastl::clamp(argc > 1 ? atoi(argv[1]) : 10, 1, voicechat::max_dump_seconds);
    interlocked_release_store(voicechat::requested_seconds, seconds);
    console::print_d("voice.dump_capture: the next %d s of capture go to voice_dump_*.wav in the log directory, or the current one",
      seconds);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(capture_dump_console_handler);
