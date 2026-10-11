// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceCapture.h"
#include "helpers.h"
#include "voiceMetrics.h"
#include "voiceRecorder.h"
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <perfMon/dag_cpuFreq.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/voiceCommunicator.h>
#include <util/dag_string.h>
#include <EASTL/algorithm.h>

namespace voicechat
{

static constexpr int capture_cycle_ms = 5;
static constexpr int stats_period_s = 300;
static constexpr int client_queue_ms = 500;
// constraint: a read carries up to one 10 ms recorder frame and the resampler delay of older audio
static constexpr int joining_client_skip_ms = 15;

class SharedVoiceCapture final : public DaThread
{
public:
  explicit SharedVoiceCapture(unsigned first_client_rate);
  ~SharedVoiceCapture();

  bool init();

private:
  friend class VoiceCaptureClient;

  void execute() override;
  void step(FMOD::System *fmod_sys);
  void distribute();
  void stopRecord(const char *reason);
  void collectStats();
  void reportStats(const char *reason);
  void publishLevel(const MicLevel &measured);

  VoiceRecorder recorder;
  // guards clients with their queues and flags, requested, settingsUpdated and level
  WinCritSec mutex;
  eastl::vector<VoiceCaptureClient *> clients;
  SoundSettings requested;
  SoundSettings active;
  MicLevel level;
  // the capture thread owns the rates and the stats
  eastl::vector<int> outputRates;
  eastl::vector<int> clientRates;
  VoiceRecorder::Stats periodStats;
  VoiceRecorder::Stats metricsStats;
  int64_t statsStartRef = 0;
  int lateReaderDroppedMs = 0;
  int metricsLateReaderDroppedMs = 0;
  int metricsSession = 0;
  bool statsEnabled = false;
  bool settingsUpdated = false;
  bool recording = false;
  bool deviceFailed = false;
};

static WinCritSec capture_mutex;
static SharedVoiceCapture *shared_capture = nullptr; // guarded by capture_mutex

SharedVoiceCapture::SharedVoiceCapture(unsigned first_client_rate) :
  DaThread("VoiceCaptureThread", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK),
  recorder(first_client_rate),
  outputRates{int(first_client_rate)}
{}

SharedVoiceCapture::~SharedVoiceCapture()
{
  DaThread::terminate(true);
  if (recording)
    stopRecord("capture ended");
}

// the sound system can be gone before the capture ends; the stats are reported all the same
void SharedVoiceCapture::stopRecord(const char *reason)
{
  if (sndsys::is_inited())
    recorder.stopRecord(sndsys::fmodapi::get_system(), active);
  recording = false;
  // the next start drops the gate preroll unsent, so this recording's line counts it
  recorder.dropSpeechGatePreroll();
  // constraint: VoiceRecorder::startRecord clears the recorder counts, so they are taken before the next start
  collectStats();
  // from the destructor the switch can have turned off after the last step read it
  if (statsEnabled && are_stats_enabled())
    reportStats(reason);
}

void SharedVoiceCapture::collectStats()
{
  const VoiceRecorder::Stats taken = recorder.takeStats();
  periodStats.add(taken);
  if (metricsSession)
    metricsStats.add(taken);
}

void SharedVoiceCapture::reportStats(const char *reason)
{
  const int seconds = ref_time_delta_to_usec(ref_time_ticks() - statsStartRef) / 1000000;
  statsStartRef = ref_time_ticks();
  collectStats();
  const VoiceRecorder::Stats rec = periodStats;
  periodStats = {};
  const ProcessingStats &proc = rec.processing;
  String gain(0, "gain not measured");
  if (proc.gainFrames > 0)
    gain.printf(0, "gain %.1f to %.1f dB over %d frames", proc.minGainDb, proc.maxGainDb, proc.gainFrames);
  String timing(0, "chain time not measured");
  if (proc.timedFrames > 0)
    timing.printf(0, "chain %d us mean and %d us max per frame, RNNoise %s", int(proc.chainUsec / proc.timedFrames), proc.chainMaxUsec,
      proc.denoiseFrames > 0 ? String(0, "%d us mean", int(proc.denoiseUsec / proc.denoiseFrames)).str() : "not run");
  debug("[VC] capture stats of record device %d, %s, %d s: %d frames read and processed, %d ring lags (max %d ms), "
        "%d ms dropped for late readers; %d of %d dropped by the speech gate, %d of %d without echo reference, %d APM errors, %s, "
        "%s",
    active.recordDeviceId, reason, seconds, proc.framesProcessed, rec.lags, rec.maxLagMs, lateReaderDroppedMs, proc.gateDroppedFrames,
    proc.gateFrames, proc.framesWithoutEcho, proc.echoFrames, proc.apmErrors, gain.str(), timing.str());
  lateReaderDroppedMs = 0;
}

bool SharedVoiceCapture::init()
{
  if (!sndsys::is_inited())
    return false;
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();
  FMOD::ChannelGroup *masterChannelGroup = nullptr;
  FMOD_RESULT fmodRes = fmodSys->getMasterChannelGroup(&masterChannelGroup);
  CHECK_FMOD_RETURN_VAL(fmodRes, "getMasterChannelGroup", false);
  if (!recorder.init(fmodSys, masterChannelGroup))
    return false;
  return start();
}

void SharedVoiceCapture::execute()
{
  int64_t refTime = ref_time_ticks();
  while (!isThreadTerminating())
  {
    sleep_usec(max(capture_cycle_ms * 1000 - get_time_usec(refTime), 0));
    refTime = ref_time_ticks();
    if (sndsys::is_inited())
      step(sndsys::fmodapi::get_system());
    else
      publishLevel({});
  }
}

void SharedVoiceCapture::publishLevel(const MicLevel &measured)
{
  WinAutoLock lock(mutex);
  level = measured;
}

void SharedVoiceCapture::step(FMOD::System *fmod_sys)
{
  const bool statsOn = are_stats_enabled();
  const int metricsNow = metrics::active_session();
  recorder.setStatsEnabled(statsOn || metricsNow);
  if (metricsNow != metricsSession)
  {
    periodStats.add(recorder.takeStats());
    metricsStats = {};
    metricsLateReaderDroppedMs = 0;
    metricsSession = metricsNow;
  }
  // the first line after the switch holds only the counts since it
  if (statsOn && !statsEnabled)
  {
    collectStats();
    periodStats = {};
    lateReaderDroppedMs = 0;
    statsStartRef = ref_time_ticks();
  }
  statsEnabled = statsOn;
  SoundSettings wanted;
  bool wantMic = false;
  bool wantRaw = false;
  bool newListener = false;
  {
    WinAutoLock lock(mutex);
    wanted = requested;
    if (settingsUpdated)
      deviceFailed = false;
    settingsUpdated = false;
    clientRates.clear();
    for (VoiceCaptureClient *client : clients)
    {
      if (client->micWanted && eastl::find(clientRates.begin(), clientRates.end(), int(client->sampleRate)) == clientRates.end())
        clientRates.push_back(int(client->sampleRate));
      wantMic |= client->micWanted;
      wantRaw |= client->micWanted && interlocked_acquire_load(client->rawWanted);
      const bool joins = client->micWanted && !client->micWantedLastStep;
      if (joins)
        client->pcmSkip = client->rawSkip = recording ? ms_to_samples(joining_client_skip_ms, client->sampleRate) : 0;
      newListener |= joins;
      // the audio read in this step was captured after the previous step started
      client->receiving = client->micWanted && client->micWantedLastStep;
      client->micWantedLastStep = client->micWanted;
    }
  }

  if (!clientRates.empty() && clientRates != outputRates)
  {
    outputRates = clientRates;
    recorder.setOutputRates(make_span_const(outputRates));
  }
  wanted.micEnabled = wantMic;
  if (deviceFailed)
    wanted.recordDeviceId = -1;

  if (recording)
  {
    bool isRealRecording = false;
    fmod_sys->isRecording(active.recordDeviceId, &isRealRecording);
    if (!wantMic || wanted.recordDeviceId != active.recordDeviceId || !isRealRecording)
      stopRecord("record stopped");
  }
  active = wanted;
  if (!recording && wantMic && active.recordDeviceId >= 0)
  {
    recording = recorder.startRecord(fmod_sys, active);
    deviceFailed = !recording;
    if (recording)
    {
      periodStats = {};
      statsStartRef = ref_time_ticks();
    }
  }
  dag::ConstSpan<int16_t> pcm = recorder.readMicData(fmod_sys, active, wantRaw);
  if (!pcm.empty())
    distribute();
  const MicLevel measured = recording ? recorder.micLevel() : MicLevel{};
  publishLevel(measured);
  // the gate preroll holds audio up to this read, from before a new listener wanted the mic
  if (newListener)
    recorder.dropSpeechGatePreroll();
  if (metricsSession)
  {
    collectStats();
    metrics::publish_capture(metricsSession,
      {metricsStats, metricsLateReaderDroppedMs, measured, active.recordVolume, active.recordDeviceId, recording});
  }
  if (statsEnabled && recording && ref_time_delta_to_usec(ref_time_ticks() - statsStartRef) >= stats_period_s * 1000000ll)
    reportStats("periodic");
}

// returns the number of samples dropped from the queue front
static unsigned append_bounded(eastl::vector<int16_t> &queue, dag::ConstSpan<int16_t> samples, unsigned &skip, unsigned max_samples)
{
  const unsigned skipped = eastl::min<unsigned>(skip, samples.size());
  skip -= skipped;
  samples = samples.subspan(skipped);
  queue.insert(queue.end(), samples.begin(), samples.end());
  if (queue.size() <= max_samples)
    return 0;
  const unsigned dropped = queue.size() - max_samples;
  queue.erase(queue.begin(), queue.begin() + dropped);
  return dropped;
}

void SharedVoiceCapture::distribute()
{
  WinAutoLock lock(mutex);
  for (VoiceCaptureClient *client : clients)
  {
    if (!client->receiving || !client->micWanted)
      continue;
    const unsigned maxQueueSamples = ms_to_samples(client_queue_ms, client->sampleRate);
    const unsigned dropped = append_bounded(client->pcmQueue, recorder.output(client->sampleRate), client->pcmSkip, maxQueueSamples);
    const int droppedMs = samples_to_ms(dropped, client->sampleRate);
    lateReaderDroppedMs += droppedMs;
    if (metricsSession)
      metricsLateReaderDroppedMs += droppedMs;
    if (dropped && !client->lagReported)
    {
      logwarn("[VC] voice capture client read more than %d ms late, older audio dropped", client_queue_ms);
      client->lagReported = true;
    }
    if (interlocked_acquire_load(client->rawWanted))
      append_bounded(client->rawQueue, recorder.rawOutput(client->sampleRate), client->rawSkip, maxQueueSamples);
    else
      client->rawSkip = 0;
  }
}

bool VoiceCaptureClient::init(const SoundSettings &settings)
{
  G_ASSERT_RETURN(!capture, true);
  WinAutoLock lock(capture_mutex);
  if (!shared_capture)
  {
    SharedVoiceCapture *created = new SharedVoiceCapture(sampleRate);
    if (!created->init())
    {
      logwarn("[VC] voice capture init failed");
      delete created;
      return false;
    }
    shared_capture = created;
  }
  capture = shared_capture;
  {
    WinAutoLock clientsLock(capture->mutex);
    capture->clients.push_back(this);
  }
  updateSettings(settings);
  return true;
}

VoiceCaptureClient::~VoiceCaptureClient()
{
  if (!capture)
    return;
  WinAutoLock lock(capture_mutex);
  bool lastClient = false;
  {
    WinAutoLock clientsLock(capture->mutex);
    capture->clients.erase(eastl::remove(capture->clients.begin(), capture->clients.end(), this), capture->clients.end());
    lastClient = capture->clients.empty();
  }
  if (lastClient)
  {
    delete shared_capture;
    shared_capture = nullptr;
  }
}

void VoiceCaptureClient::setMicWanted(bool wanted)
{
  if (!capture)
    return;
  WinAutoLock lock(capture->mutex);
  micWanted = wanted;
  if (!wanted)
  {
    pcmQueue.clear();
    rawQueue.clear();
    micWantedLastStep = false;
    receiving = false;
  }
}

void VoiceCaptureClient::setRawWanted(bool wanted) { interlocked_release_store(rawWanted, wanted ? 1 : 0); }

void VoiceCaptureClient::updateSettings(const SoundSettings &settings)
{
  if (!capture)
    return;
  WinAutoLock lock(capture->mutex);
  capture->requested.recordDeviceId = settings.recordDeviceId;
  capture->requested.recordVolume = settings.recordVolume;
  capture->settingsUpdated = true;
}

MicLevel VoiceCaptureClient::micLevel() const
{
  if (!capture)
    return {};
  WinAutoLock lock(capture->mutex);
  return capture->level;
}

void VoiceCaptureClient::read(eastl::vector<int16_t> &pcm, eastl::vector<int16_t> &raw)
{
  pcm.clear();
  raw.clear();
  if (!capture)
    return;
  WinAutoLock lock(capture->mutex);
  pcm.swap(pcmQueue);
  lagReported = false;
  if (interlocked_acquire_load(rawWanted))
    raw.swap(rawQueue);
  else
    rawQueue.clear();
}

} // namespace voicechat
