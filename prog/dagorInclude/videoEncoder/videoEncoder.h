//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_tex3d.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <3d/dag_resPtr.h>

struct VideoSettings
{
  eastl::string fname;
  eastl::string audioDataFName;
  uint32_t width = 640, height = 480;
  uint32_t bitrate = 30000000, fps = 60;
  bool hdr = false;
};

using AudioBuffer = eastl::vector<eastl::vector<uint8_t>>;

#if _TARGET_PC_WIN
bool list_devices();
bool list_default_device();

class AudioRecorder
{
  AudioBuffer audioBuffersPerSample;
  bool recording = false;

public:
  bool init();
  bool start();
  bool process();
  bool stop(AudioBuffer &&audioBuffersPerSample);
  void shutdown();

  bool isRecording() const { return recording; }
};

class VideoEncoder
{
  unsigned long videoStreamIndex = 0, audioStreamIndex = 1;
  bool recording = false;
  unsigned long long rtStart = 0;
  uint64_t duration = 0;
  int32_t cbWidth = 0;
  uint32_t cbBuffer = 0;
  AudioBuffer externalAudioBuffersPerSample;
  uint32_t currentAudioSample = 0;
  bool setAudioSample(const eastl::vector<uint8_t> &audioBuf);
  uint32_t w = 1, h = 1;

public:
  AudioRecorder audioRecorder;
  ~VideoEncoder();
  bool init(void *pUnkDevice);
  bool start(const VideoSettings &settings);
  bool process(BaseTexture *tex);
  bool stop();
  void shutdown();
  void setExternalAudioBuffer(AudioBuffer &&audioBuffersPerSample);

  bool isRecording() const { return recording; }
};
#else
class AudioRecorder
{
public:
  bool init() { return true; }
  bool start() { return true; }
  bool process() { return true; }
  bool stop(AudioBuffer &&) { return true; }
  void shutdown() {}

  bool isRecording() const { return false; }
};
class VideoEncoder
{
public:
  AudioRecorder audioRecorder;
  bool init(void *) { return true; }
  bool start(const VideoSettings &) { return true; }
  bool process(BaseTexture *) { return true; }
  bool stop() { return true; }
  void shutdown() {}
  void setExternalAudioBuffer(AudioBuffer &&) {}
  bool isRecording() const { return false; }
};
inline bool list_devices() { return true; }
inline bool list_default_device() { return true; }
#endif
