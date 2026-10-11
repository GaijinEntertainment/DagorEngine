// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceRecorder.h"
#include "helpers.h"
#include <soundSystem/voiceCommunicator.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_threads.h>
#include <EASTL/algorithm.h>

namespace voicechat
{

static const int sample_rate = VoiceProcessor::captureRate();
static const int frame_size = VoiceProcessor::frameSamples();
static const FMOD_SOUND_FORMAT sample_format = FMOD_SOUND_FORMAT_PCM16;
static const int sample_size = get_bytes_from_format(sample_format);
static const int record_ring_ms = 500;

VoiceRecorder::VoiceRecorder(int output_samplerate) : processor(output_samplerate)
{
  memset(&recordSoundInfo, 0, sizeof(recordSoundInfo));
}

VoiceRecorder::~VoiceRecorder()
{
#if DAGOR_DBGLEVEL > 0
  finishDump();
#endif
  if (feedback)
    feedback->shutdown();
}

dag::ConstSpan<int16_t> VoiceRecorder::readFrames(FMOD::System *fmod_sys)
{
  unsigned int curPos = 0;
  FMOD_RESULT fmodRes = fmod_sys->getRecordPosition(currentRecordDevId, &curPos);
  if (fmodRes != FMOD_OK)
    return {};
  if (curPos == recordPos)
    return {};

  int bufSamples = curPos - recordPos;
  if (bufSamples < 0)
    bufSamples += recordSoundSamples;

  // constraint: a lag past the whole ring wraps unseen
  if (bufSamples > (int)recordSoundSamples / 2)
  {
    const int lagMs = samples_to_ms(bufSamples, sample_rate);
    logwarn("[VC] record reader lags by %d of %d ms", lagMs, record_ring_ms);
    ++stats.lags;
    stats.maxLagMs = max(stats.maxLagMs, lagMs);
  }

  if (bufSamples < frame_size)
    return {};

  int numFrames = bufSamples / frame_size;
  captureBuf.resize(frame_size * numFrames);
  dag::Span<int16_t> recordSource = {captureBuf.data(), (int)captureBuf.size()};

  {
    int16_t *ptr1;
    int16_t *ptr2;
    unsigned int len1;
    unsigned int len2;
    fmodRes =
      recordSound->lock(recordPos * sample_size, numFrames * frame_size * sample_size, (void **)&ptr1, (void **)&ptr2, &len1, &len2);
    CHECK_FMOD_RETURN_VAL(fmodRes, "failed to lock record buffer", {});
    memcpy(recordSource.data(), ptr1, len1);
    if (ptr2)
      memcpy(&recordSource[len1 / sample_size], ptr2, len2); // -V609
    recordSound->unlock(ptr1, ptr2, len1, len2);
    unsigned readSamples = (len1 + len2) / sample_size;
    recordPos = (recordPos + readSamples) % recordSoundSamples;
  }

  return recordSource;
}

void VoiceRecorder::dropSpeechGatePreroll()
{
  processor.dropSpeechGatePreroll();
#if DAGOR_DBGLEVEL > 0
  if (dump)
    dump->onPrerollDropped();
#endif
}

dag::ConstSpan<int16_t> VoiceRecorder::readMicData(FMOD::System *fmod_sys, const SoundSettings &sound_settings, bool want_raw)
{
  if (!isRecording || !sound_settings.micEnabled || !recordSound.get() || sound_settings.recordDeviceId < 0)
  {
    processor.breakStream();
    return {};
  }
#if DAGOR_DBGLEVEL > 0
  if (!dump)
    if (const int seconds = take_capture_dump_request())
      dump.reset(new CaptureDump(sample_rate, frame_size, processor.firstOutputRate(), seconds));
#endif
  const dag::ConstSpan<int16_t> capture = readFrames(fmod_sys);
  const dag::ConstSpan<int16_t> out = processor.process(
    capture, sound_settings.recordVolume,
    [this](dag::Span<int16_t> frame) {
      const bool fed = feedback->popFrame(frame);
#if DAGOR_DBGLEVEL > 0
      if (dump)
        dump->onReferencePop(fed ? dag::ConstSpan<int16_t>(frame) : dag::ConstSpan<int16_t>());
#endif
      return fed;
    },
    want_raw);
#if DAGOR_DBGLEVEL > 0
  if (dump)
  {
    dump->onProcessed(capture, processor.output(dump->outputSampleRate()), sound_settings.recordVolume);
    if (dump->isFull())
      finishDump();
  }
#endif
  return out;
}

void VoiceRecorder::setOutputRates(dag::ConstSpan<int> rates)
{
  processor.setOutputRates(rates);
#if DAGOR_DBGLEVEL > 0
  if (dump && eastl::find(rates.begin(), rates.end(), dump->outputSampleRate()) == rates.end())
    finishDump();
#endif
}

void VoiceRecorder::Stats::add(const Stats &other)
{
  lags += other.lags;
  maxLagMs = max(maxLagMs, other.maxLagMs);
  processing.add(other.processing);
}

VoiceRecorder::Stats VoiceRecorder::takeStats()
{
  Stats taken = stats;
  taken.processing = processor.takeStats();
  stats = {};
  return taken;
}

bool VoiceRecorder::init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group)
{
  int mixerRate = 0;
  FMOD_RESULT fmodRes = fmod_sys->getSoftwareFormat(&mixerRate, nullptr, nullptr);
  CHECK_FMOD_RETURN_VAL(fmodRes, "getSoftwareFormat", false);
  feedback.reset(new VoiceFeedback(sample_rate, frame_size, mixerRate, record_ring_ms));
  if (!feedback->init(fmod_sys, channel_group))
  {
    debug("[VC] failed to initialize voice feedback");
    return false;
  }
  return true;
}

bool VoiceRecorder::createRecordSound(FMOD::System *fmod_sys)
{
  if (!recordSound || recordSoundInfo.format != sample_format)
  {
    const int nsamples = ms_to_samples(record_ring_ms, sample_rate);
    const int nbytes = nsamples * sample_size;

    FMOD_CREATESOUNDEXINFO exinfo = recordSoundInfo;
    exinfo.defaultfrequency = sample_rate;
    exinfo.format = sample_format;
    exinfo.length = nbytes;
    exinfo.numchannels = 1;
    exinfo.cbsize = sizeof(exinfo);

    debug("[VC] Create record sound: sr %u, ns %d, nb %d", sample_rate, nsamples, nbytes);
    FMOD::Sound *sound = nullptr;
    FMOD_RESULT fmodRes = fmod_sys->createSound(nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_CREATESAMPLE, &exinfo, &sound);
    CHECK_FMOD_RETURN_VAL(fmodRes, "Failed to create record sound", false);
    recordSoundInfo = exinfo;
    recordSound.reset(sound);
    fmodRes = recordSound->getLength(&recordSoundSamples, FMOD_TIMEUNIT_PCM);
    CHECK_FMOD_RETURN_VAL(fmodRes, "Sound::getLength", false);
    G_ASSERT(recordSoundSamples == nsamples);
  }

  return true;
}

bool VoiceRecorder::startRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings)
{
  debug("[VC] Start record on device %d", sound_settings.recordDeviceId);
  FMOD_RESULT fmodRes;
  FMOD_DRIVER_STATE state;
  fmodRes = fmod_sys->getRecordDriverInfo(sound_settings.recordDeviceId, nullptr, 0, nullptr, nullptr, nullptr, nullptr, &state);

  if (fmodRes != FMOD_OK || ((state & FMOD_DRIVER_STATE_CONNECTED) == 0))
  {
    debug("[VC] Record start failed. Device not connected");
    return false;
  }

  // constraint: Initialize() resets AGC2 and AEC3 adaptation
  if (sound_settings.recordDeviceId != currentRecordDevId)
  {
    debug("[VC] Reinit audio processing for record device %d", sound_settings.recordDeviceId);
    processor.resetAdaptation();
  }
  currentRecordDevId = sound_settings.recordDeviceId;
  if (createRecordSound(fmod_sys))
  {
    fmodRes = fmod_sys->recordStart(sound_settings.recordDeviceId, recordSound.get(), true);
    CHECK_FMOD_RETURN_VAL(fmodRes, "Failed to start record", false);
    isRecording = true;
    fmodRes = fmod_sys->getRecordPosition(sound_settings.recordDeviceId, &recordPos);
    if (fmodRes != FMOD_OK)
    {
      LOGWARN_CTX("[VC] %s: %s: %s", __FUNCTION__, "getRecordPosition", FMOD_ErrorString(fmodRes));
      stopRecord(fmod_sys, sound_settings);
      return false;
    }
    processor.startStream();
    // the counts start with the recording, after the gate dropped the preroll of the previous stream
    takeStats();
    feedback->enable();
  }
  return true;
}

void VoiceRecorder::stopRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings)
{
  debug("[VC] Stop record, was recording: %d", isRecording);
  // stop only a driver this recorder started: isRecording is set on a successful recordStart
  if (isRecording)
    fmod_sys->recordStop(sound_settings.recordDeviceId);
  isRecording = false;
  feedback->disable();
#if DAGOR_DBGLEVEL > 0
  finishDump();
#endif
}

#if DAGOR_DBGLEVEL > 0
void VoiceRecorder::finishDump()
{
  if (!dump)
    return;
  if (dump->isEmpty())
  {
    dump.reset();
    return;
  }
  // the names share the log directory of the process, and the capture makes a new recorder for each client set
  static int dumpsWritten = 0;
  const char *dir = get_log_directory();
  String prefix(0, "%s%svoice_dump_%d", dir, *dir ? "/" : "", ++dumpsWritten);
  execute_in_new_thread(
    [finished = eastl::move(dump), prefix = eastl::move(prefix)](auto) {
      if (finished->write(prefix))
        debug("[VC] capture dump written to %s_mic.wav, _far.wav, _out.wav and _feed.txt", prefix.str());
      else
        logwarn("[VC] capture dump: cannot write %s_*", prefix.str());
    },
    "VoiceCaptureDump");
}
#endif

} // namespace voicechat
