// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceFeedback.h"
#include <math/dag_mathBase.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <fmod_errors.h>
#include <osApiWrappers/dag_miscApi.h>
#include "helpers.h"

namespace voicechat
{

// This callback is called from FMOD's update thread.
// So we can perform CPU-greedy tasks here as downsampling and mixing
// But if it (unlikely) will overload FMOD thread this tasks should be moved to other threads
// using job manager
static FMOD_RESULT F_CALL dsp_process(FMOD_DSP_STATE *dsp_state, unsigned int length, const FMOD_DSP_BUFFER_ARRAY *inbufferarray,
  FMOD_DSP_BUFFER_ARRAY *outbufferarray, FMOD_BOOL inputsidle, FMOD_DSP_PROCESS_OPERATION op)
{
  if (op == FMOD_DSP_PROCESS_QUERY)
    return inputsidle == 0 ? FMOD_OK : FMOD_ERR_DSP_DONTPROCESS;

  for (int n = 0; n < outbufferarray->numbuffers; ++n)
    memcpy(outbufferarray->buffers[n], inbufferarray->buffers[n], length * sizeof(float) * outbufferarray->buffernumchannels[n]);

  int samplerate = 0;
  FMOD_RESULT res = dsp_state->functions->getsamplerate(dsp_state, &samplerate);
  if (res == FMOD_OK)
  {
    void *userData = nullptr;
    res = dsp_state->functions->getuserdata(dsp_state, &userData);
    if (res == FMOD_OK && userData)
    {
      static_cast<VoiceFeedback *>(userData)->processEchoSource(samplerate, inbufferarray->buffers[0], length,
        inbufferarray->buffernumchannels[0]);
    }
  }

  return FMOD_OK;
}

VoiceFeedback::VoiceFeedback(unsigned int sample_rate, FMOD_SOUND_FORMAT sample_format, unsigned int frame_samples) :
  samplerate(sample_rate), sampleSize(get_bytes_from_format(sample_format))
{
  frameSize = frame_samples;
  i16PCMOutput.resize(frameSize * framesCount);

  for (unsigned f = 0; f < framesCount; ++f)
    framesPool[f].samples = make_span(&i16PCMOutput[f * frameSize], frameSize);
}

bool VoiceFeedback::init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group)
{
  FMOD_DSP_DESCRIPTION dspdescr;
  memset(&dspdescr, 0, sizeof(dspdescr));
  dspdescr.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
  strcpy(dspdescr.name, "voice echo feedback");
  dspdescr.numinputbuffers = 1;
  dspdescr.numoutputbuffers = 1;
  dspdescr.process = dsp_process;
  dspdescr.userdata = this;

  G_ASSERT(channel_group);
  channelGroup = channel_group;

  FMOD_RESULT fmodRes;
  fmodRes = fmod_sys->createDSP(&dspdescr, &captureDSP);
  CHECK_FMOD_RETURN_VAL(fmodRes, "createDSP", false);
  fmodRes = channelGroup->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, captureDSP);
  CHECK_FMOD_RETURN_VAL(fmodRes, "addDSP", false);
  captureDSP->setActive(true);
  return true;
}

void VoiceFeedback::shutdown()
{
  if (captureDSP)
  {
    FMOD::System *fmodSys = nullptr;
    captureDSP->getSystemObject(&fmodSys);
    fmodSys->lockDSP();
    captureDSP->setUserData(nullptr);
    if (channelGroup)
      channelGroup->removeDSP(captureDSP);
    captureDSP->release();
    fmodSys->unlockDSP();
  }
}

VoiceFeedback::~VoiceFeedback()
{
  if (resampler)
    speex_resampler_destroy(resampler);
}


VoiceFeedback::PCMFrame *VoiceFeedback::nextFrameFromPool()
{
  PCMFrame *bestFrame = nullptr;
  for (PCMFrame &f : framesPool)
  {
    if (f.age == 0)
      return &f;
    if (!bestFrame || f.age < bestFrame->age)
      bestFrame = &f;
  }
  return bestFrame;
}

void VoiceFeedback::processEchoSource(int samplerate_in, const float *inbuffer, unsigned int nsamples, int nchannels)
{
  if (!enabled)
    return;
  if (samplerate_in != samplerate && !resampler)
  {
    resampler = speex_resampler_init(nchannels, samplerate_in, samplerate, 5, nullptr);
    G_ASSERT_RETURN(resampler, );
  }

  unsigned int nsamplesOut = nsamples;
  const float *resampledBuffer = inbuffer;

  if (resampler)
  {
    nsamplesOut = re_rate(nsamples, samplerate_in, samplerate);
    f32PCMBuffer.resize(nsamplesOut * nchannels);
    resampledBuffer = f32PCMBuffer.data();
    for (int chan = 0; chan < nchannels; ++chan)
      speex_resampler_process_float(resampler, chan, inbuffer, &nsamples, f32PCMBuffer.data(), &nsamplesOut);
    f32PCMBuffer.resize(nsamplesOut * nchannels);
  }

  WinAutoLock lock(mutex);
  size_t bufSize = i16PCMProcess.size();
  i16PCMProcess.resize(bufSize + nsamplesOut);
  dag::Span<int16_t> i16EchoSource(i16PCMProcess.data() + bufSize, i16PCMProcess.size() - bufSize);

  for (unsigned i = 0; i < nsamplesOut; ++i)
  {
    float sample = 0;
    for (int chan = 0; chan < nchannels; ++chan)
      sample += resampledBuffer[i + chan * nchannels];
    // convert stereo to mono
    i16EchoSource[i] = clamp<int>(sample / nchannels * INT16_MAX, INT16_MIN, INT16_MAX);
  }

  {
    unsigned readOffset = 0;
    while (i16PCMProcess.size() - readOffset >= frameSize)
    {
      while (outFrames.size() >= framesCount)
        outFrames.pop();

      PCMFrame *frame = nextFrameFromPool();
      frame->age = ++idCounter;
      mem_copy_from(frame->samples, &i16PCMProcess[readOffset]);
      outFrames.push(frame);
      readOffset += frameSize;
    }
    if (readOffset < i16PCMProcess.size())
    {
      memmove(i16PCMProcess.data(), &i16PCMProcess[readOffset], i16PCMProcess.size() - readOffset);
      i16PCMProcess.resize(i16PCMProcess.size() - readOffset);
    }
  }
}

dag::ConstSpan<int16_t> VoiceFeedback::lockFrame()
{
  mutex.lock();
  if (outFrames.size() < 4)
    return {};
  PCMFrame *frame = outFrames.front();
  outFrames.pop();
  return frame->samples;
}

void VoiceFeedback::unlockFrame() { mutex.unlock(); }

void VoiceFeedback::enable() { enabled = true; }

void VoiceFeedback::disable()
{
  mutex.lock();
  outFrames = {};
  i16PCMProcess.clear();
  idCounter = 0;
  enabled = false;
  for (PCMFrame &f : framesPool)
    f.age = 0;
  mutex.unlock();
}

} // namespace voicechat
