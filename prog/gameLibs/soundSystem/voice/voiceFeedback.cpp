// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceFeedback.h"
#include <math/dag_mathBase.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <fmod_errors.h>
#include "helpers.h"

namespace voicechat
{

static const int echo_resampler_quality = 5;

static FMOD_RESULT F_CALL dsp_process(FMOD_DSP_STATE *dsp_state, unsigned int length, const FMOD_DSP_BUFFER_ARRAY *inbufferarray,
  FMOD_DSP_BUFFER_ARRAY *outbufferarray, FMOD_BOOL inputsidle, FMOD_DSP_PROCESS_OPERATION op)
{
  if (op == FMOD_DSP_PROCESS_QUERY)
    return inputsidle == 0 ? FMOD_OK : FMOD_ERR_DSP_DONTPROCESS;

  for (int n = 0; n < outbufferarray->numbuffers; ++n)
    memcpy(outbufferarray->buffers[n], inbufferarray->buffers[n], length * sizeof(float) * outbufferarray->buffernumchannels[n]);

  void *userData = nullptr;
  if (dsp_state->functions->getuserdata(dsp_state, &userData) == FMOD_OK && userData)
    static_cast<VoiceFeedback *>(userData)->processEchoSource(inbufferarray->buffers[0], length, inbufferarray->buffernumchannels[0]);

  return FMOD_OK;
}

// constraint: the DSP runs at the mixer rate, the software format rate of the FMOD system.
// The ring holds two windows: below capacity less a window no mixer block can have been rejected.
VoiceFeedback::VoiceFeedback(unsigned int sample_rate, unsigned int frame_samples, unsigned int mixer_rate, int window_ms) :
  ring(2 * ms_to_samples(window_ms, mixer_rate)),
  samplerate(sample_rate),
  frameSize(frame_samples),
  mixerRate(mixer_rate),
  windowSamples(ms_to_samples(window_ms, mixer_rate)),
  fullSamples(ring.capacity() - windowSamples)
{}

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

// constraint: the FMOD mixer thread must not wait or allocate, so it only downmixes into the ring
void VoiceFeedback::processEchoSource(const float *inbuffer, unsigned int nsamples, int nchannels)
{
  if (!interlocked_acquire_load(enabled))
    return;

  const bool written = ring.writeWith(nsamples, [&](uint32_t i) {
    float sample = 0;
    for (int chan = 0; chan < nchannels; ++chan)
      sample += inbuffer[i * nchannels + chan];
    return sample / nchannels;
  });
  if (!written)
    interlocked_add(droppedSamples, nsamples);
}

// the reference kept so far no longer continues into what the ring gives next
void VoiceFeedback::dropPending()
{
  pendingPcm.clear();
  if (resampler)
    speex_resampler_reset_mem(resampler);
}

void VoiceFeedback::restart()
{
  ring.discard();
  dropPending();
}

void VoiceFeedback::drainRing()
{
  uint32_t count = ring.available();
  // the ring only grows between reads; this full it may have refused a block, and the gap can be anywhere in it
  if (count > fullSamples)
  {
    discardedTotal += count;
    restart();
    return;
  }
  if (count > windowSamples)
  {
    // gapless, but its oldest part is older than the mic
    ring.skip(count - windowSamples);
    discardedTotal += count - windowSamples;
    dropPending();
    count = windowSamples;
  }
  if (count == 0)
    return;
  mixerRateBuf.resize(count);
  ring.read(make_span(mixerRateBuf));

  dag::ConstSpan<float> mono = make_span_const(mixerRateBuf);
  if (mixerRate != samplerate)
  {
    if (!resampler)
      resampler = speex_resampler_init(1, mixerRate, samplerate, echo_resampler_quality, nullptr);
    G_ASSERT_RETURN(resampler, );
    resampledBuf.resize(re_rate(count, mixerRate, samplerate) + 16);
    spx_uint32_t inLen = count;
    spx_uint32_t outLen = resampledBuf.size();
    speex_resampler_process_float(resampler, 0, mixerRateBuf.data(), &inLen, resampledBuf.data(), &outLen);
    G_ASSERT(inLen == count);
    mono = make_span_const(resampledBuf.data(), outLen);
  }

  const size_t at = pendingPcm.size();
  pendingPcm.resize(at + mono.size());
  for (size_t i = 0; i < mono.size(); ++i)
    pendingPcm[at + i] = clamp<int>(mono[i] * INT16_MAX, INT16_MIN, INT16_MAX);
}

bool VoiceFeedback::popFrame(dag::Span<int16_t> out)
{
  G_ASSERT_RETURN(out.size() == frameSize, false);
  drainRing();
  if (pendingPcm.size() < frameSize)
    return false;
  memcpy(out.data(), pendingPcm.data(), frameSize * sizeof(int16_t));
  pendingPcm.erase(pendingPcm.begin(), pendingPcm.begin() + frameSize);
  return true;
}

void VoiceFeedback::enable()
{
  interlocked_exchange(droppedSamples, 0u);
  restart();
  interlocked_release_store(enabled, 1);
}

void VoiceFeedback::disable()
{
  interlocked_release_store(enabled, 0);
  const uint32_t refused = interlocked_exchange(droppedSamples, 0u);
  if (discardedTotal || refused)
    logwarn("[VC] echo reference lost: %d ms discarded, %d ms refused by a full ring", samples_to_ms(discardedTotal, mixerRate),
      samples_to_ms(refused, mixerRate));
  discardedTotal = 0;
}

} // namespace voicechat
