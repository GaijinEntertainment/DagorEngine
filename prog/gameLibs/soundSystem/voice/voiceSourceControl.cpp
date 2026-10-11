// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceSourceControl.h"
#include "voiceTuning.h"
#include <soundSystem/fmodApi.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/voiceCommunicator.h>


namespace voicechat
{

// the mixer takes a whole DSP block at once, so one queued frame alone can run dry within it
static const int playback_on_demand_frames = 2;
static const int playback_resampler_quality = 5;
static const int playback_edge_fade_ms = 3;

static unsigned mixer_frame_samples(const IVoiceSource &source, int mixer_rate)
{
  return re_rate(source.getFrameSamples(), source.getSampleRate(), mixer_rate);
}

// room for one resampled frame above the most that pushFrames keeps queued at any tuning, so a frame it wants always fits
static uint32_t playback_ring_samples(const IVoiceSource &source, int mixer_rate)
{
  const unsigned frame = mixer_frame_samples(source, mixer_rate);
  if (source.makesAudioOnDemand())
    return (playback_on_demand_frames + 1) * frame + 16;
  return ms_to_samples(MAX_PLAYBACK_START_MARGIN_MS, mixer_rate) + (MAX_PLAYBACK_LEAD_FRAMES + 1) * frame + 16;
}

static FMOD_RESULT F_CALL play_process(FMOD_DSP_STATE *dsp_state, unsigned int length, const FMOD_DSP_BUFFER_ARRAY *,
  FMOD_DSP_BUFFER_ARRAY *outbufferarray, FMOD_BOOL, FMOD_DSP_PROCESS_OPERATION op)
{
  if (op == FMOD_DSP_PROCESS_QUERY)
  {
    // a generator has no input to take its format from
    if (outbufferarray)
    {
      outbufferarray->buffernumchannels[0] = 1;
      outbufferarray->bufferchannelmask[0] = 0;
      outbufferarray->speakermode = FMOD_SPEAKERMODE_MONO;
    }
    return FMOD_OK;
  }

  float *out = outbufferarray->buffers[0];
  const int channels = outbufferarray->buffernumchannels[0];
  void *userData = nullptr;
  if (dsp_state->functions->getuserdata(dsp_state, &userData) == FMOD_OK && userData)
    static_cast<VoiceSourceControl *>(userData)->readMix(out, length, channels);
  else
    memset(out, 0, length * channels * sizeof(float));
  return FMOD_OK;
}


VoiceSourceControl::VoiceSourceControl(IVoiceSource &source_, int mixer_rate) :
  source(source_),
  ring(playback_ring_samples(source_, mixer_rate)),
  chanSampleRate(source_.getSampleRate()),
  chanFrameSamples(source_.getFrameSamples()),
  mixerRate(mixer_rate),
  mixerFrameSamples(mixer_frame_samples(source_, mixer_rate)),
  silentSamples(mixerFrameSamples),
  edgeFader(ms_to_samples(playback_edge_fade_ms, mixer_rate))
{
  applyTuning();
}

// a spurt of a buffered source after at least a frame of silence starts late by the margin, so that a worker stall
// of up to this length inside the spurt plays on without a gap. An on-demand source is only kept
// playback_on_demand_frames ahead, so a margin would delay it and buy no lead
void VoiceSourceControl::applyTuning()
{
  tuning = playback_tuning(&tuningGeneration);
  const unsigned margin = source.makesAudioOnDemand() ? 0 : ms_to_samples(tuning.startMarginMs, mixerRate);
  interlocked_relaxed_store(marginSamples, margin);
  leadSamples = margin + tuning.leadFrames * mixerFrameSamples;
}


VoiceSourceControl::~VoiceSourceControl()
{
  detachFromMixer();
  if (resampler)
    speex_resampler_destroy(resampler);
}

void VoiceSourceControl::detachFromMixer()
{
  if (!playbackDSP)
    return;
  FMOD::System *fmodSys = nullptr;
  playbackDSP->getSystemObject(&fmodSys);
  // lifetime: once the user data is cleared under the DSP lock, the mixer thread no longer reaches this object
  fmodSys->lockDSP();
  playbackDSP->setUserData(nullptr);
  fmodSys->unlockDSP();
  if (playbackChannel)
    playbackChannel->stop();
  playbackDSP->release();
  playbackDSP = nullptr;
  playbackChannel = nullptr;
}

void VoiceSourceControl::stop()
{
  detachFromMixer();
  if (const int dryRuns = interlocked_exchange(underruns, 0))
    source.onPlaybackUnderruns(dryRuns);
}


bool VoiceSourceControl::init(float volume_)
{
  source.resetQueue();
  externalVolume = clamp(volume_, 0.0f, 1.0f);
  localVolume = source.getVolume();
  frameBuf.resize(chanFrameSamples);

  if (!sndsys::is_inited())
    return false;
  G_ASSERT_RETURN(chanFrameSamples > 0 && mixerRate > 0, false);
  if (mixerRate != chanSampleRate)
  {
    resampler = speex_resampler_init(1, chanSampleRate, mixerRate, playback_resampler_quality, nullptr);
    G_ASSERT_RETURN(resampler, false);
    resampledBuf.resize(mixerFrameSamples + 16);
  }

  FMOD_DSP_DESCRIPTION dspdescr;
  memset(&dspdescr, 0, sizeof(dspdescr));
  dspdescr.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
  strcpy(dspdescr.name, "voice playback");
  dspdescr.numinputbuffers = 0;
  dspdescr.numoutputbuffers = 1;
  dspdescr.process = play_process;
  dspdescr.userdata = this;

  FMOD_RESULT fmodRes = sndsys::fmodapi::get_system()->createDSP(&dspdescr, &playbackDSP);
  CHECK_FMOD_RETURN_VAL(fmodRes, "failed to create playback DSP", false);
  debug("[VC] Create playback DSP: sr %u, mixer %u", chanSampleRate, mixerRate);
  return true;
}


void VoiceSourceControl::readMix(float *out, unsigned int length, int channels)
{
  int16_t pcm[256];
  for (unsigned done = 0; done < length;)
  {
    unsigned chunk = eastl::min<unsigned>(countof(pcm), length - done);
    unsigned got = 0;
    if (marginLeft == 0 && silentSamples >= mixerFrameSamples && ring.available() > 0)
      marginLeft = interlocked_relaxed_load(marginSamples);
    if (marginLeft > 0)
    {
      chunk = eastl::min(chunk, marginLeft);
      marginLeft -= chunk;
      silentSamples = 0;
    }
    else
    {
      got = ring.read(make_span(pcm, chunk));
      // silence of a frame or more ends a spurt; a shorter gap is a spurt that ran dry
      if (got > 0 && silentSamples > 0 && silentSamples < mixerFrameSamples)
        interlocked_increment(underruns);
      silentSamples = got > 0 ? chunk - got : silentSamples + chunk;
    }
    edgeFader.apply(make_span(pcm, chunk), got);
    for (unsigned i = 0; i < chunk; ++i)
    {
      const float value = pcm[i] * (1.0f / 32768.0f);
      for (int c = 0; c < channels; ++c)
        out[(done + i) * channels + c] = value;
    }
    done += chunk;
  }
}


// a buffered source is read up to the lead; one that makes audio on demand is kept a little ahead, one
// frame per call
void VoiceSourceControl::pushFrames()
{
  if (tuning_generation() != tuningGeneration)
    applyTuning();
  if (const int dryRuns = interlocked_exchange(underruns, 0))
  {
    source.onPlaybackUnderruns(dryRuns);
    underrunsTaken += dryRuns;
  }
  const bool onDemand = source.makesAudioOnDemand();
  for (;;)
  {
    const uint32_t queued = ring.capacity() - ring.freeSpace();
    const bool wanted = onDemand ? queued < playback_on_demand_frames * mixerFrameSamples
                                 : queued + mixerFrameSamples <= leadSamples && source.hasAudioFrame();
    if (!wanted)
      return;
    source.receiveAudioFrame(frameBuf.data(), chanFrameSamples);
    dag::ConstSpan<int16_t> frame = make_span_const(frameBuf);
    if (resampler)
    {
      spx_uint32_t inLen = chanFrameSamples;
      spx_uint32_t outLen = resampledBuf.size();
      speex_resampler_process_int(resampler, 0, frameBuf.data(), &inLen, resampledBuf.data(), &outLen);
      frame = make_span_const(resampledBuf.data(), outLen);
    }
    if (!ring.write(frame) || onDemand)
      return;
  }
}


void VoiceSourceControl::updatePlayback(FMOD::System *fmod_sys)
{
  pushFrames();

  bool isPlaying = false;
  if (playbackChannel)
    playbackChannel->isPlaying(&isPlaying);

  if (isPlaying)
  {
    if (source.isPositional())
      update3dAttributes();
    return;
  }

  FMOD_RESULT fmodRes = fmod_sys->playDSP(playbackDSP, voiceChannelGroup, true, &playbackChannel);
  CHECK_FMOD_RETURN(fmodRes, "failed to play voice");
  // constraint: the room's max distance is where the server stops sending the voice, so a linear fade keeps it clear
  playbackChannel->setMode(source.isPositional() ? FMOD_3D | FMOD_3D_LINEARROLLOFF : FMOD_2D);
  applyVolume();

  if (source.isPositional())
  {
    playbackChannel->set3DMinMaxDistance(source.getMinDistance(), source.getMaxDistance());
    // The default doppler level of 1.0f seems to misbehave and the effect is exaggerated,
    // even despite distance scaling being 1.0f too and position data looking good (1.0 pos = 1m).
    // Probably this happens because we don't supply proper velocities, could not figure out a
    // good way to get them, so we turn doppler effect off for positional voice
    playbackChannel->set3DDopplerLevel(0.0f);
    update3dAttributes();
  }
  playbackChannel->setPaused(false);
}


VoiceSourceControl::Metrics VoiceSourceControl::metrics(int session)
{
  const int blocksDropped = source.getBlocksDroppedTotal();
  if (session != metricsSession)
  {
    metricsSession = session;
    underrunsTaken = 0;
    blocksDroppedStart = blocksDropped;
  }
  return {&source, samples_to_ms(ring.capacity() - ring.freeSpace(), mixerRate), samples_to_ms(ring.capacity(), mixerRate),
    underrunsTaken, blocksDropped < 0 ? -1 : blocksDropped - blocksDroppedStart};
}


void VoiceSourceControl::update3dAttributes()
{
  G_ASSERT(playbackChannel);

  Point3 pos = source.getSpeakerPos();
  FMOD_VECTOR fmodPos{.x = pos.x, .y = pos.y, .z = pos.z};
  FMOD_VECTOR fmodVelocity{0};
  playbackChannel->set3DAttributes(&fmodPos, &fmodVelocity);

  FMOD::System *fmodSys = nullptr;
  FMOD_VECTOR listenerPos{0};
  float rolloffScale = 1.0f;
  if (playbackChannel->getSystemObject(&fmodSys) != FMOD_OK ||
      fmodSys->get3DListenerAttributes(0, &listenerPos, nullptr, nullptr, nullptr) != FMOD_OK ||
      fmodSys->get3DSettings(nullptr, nullptr, &rolloffScale) != FMOD_OK)
    return;
  const float distance = length(pos - Point3(listenerPos.x, listenerPos.y, listenerPos.z));
  const float level = pan_level_at(tuning, distance);
  playbackChannel->set3DLevel(level);
  // constraint: FMOD scales its rolloff by the 3D level too, so the volume restores the linear fade to the max distance
  const float minDistance = eastl::min(source.getMinDistance(), source.getMaxDistance()), maxDistance = source.getMaxDistance();
  float rolloff = 1.0f;
  if (distance > minDistance && rolloffScale > 0.0f)
  {
    const float scaled = (distance - minDistance) * rolloffScale + minDistance;
    rolloff = scaled >= maxDistance ? 0.0f : (maxDistance - scaled) / (maxDistance - minDistance);
  }
  const float blended = 1.0f - level + level * rolloff;
  rolloffVolume = blended > 0.0f ? rolloff / blended : 0.0f;
  applyVolume();
}

void VoiceSourceControl::applyVolume() { playbackChannel->setVolume(externalVolume * localVolume * rolloffVolume); }

void VoiceSourceControl::setExternalVoiceVolume(float volume_)
{
  externalVolume = clamp(volume_, 0.0f, 1.0f);
  if (playbackChannel)
    applyVolume();
}

void VoiceSourceControl::updateSourceVolume()
{
  localVolume = source.getVolume();
  if (playbackChannel)
    applyVolume();
}

} // namespace voicechat
