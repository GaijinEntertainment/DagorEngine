// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <debug/dag_log.h>
#include <fmod.hpp>
#include <fmod_dsp.h>
#include "../internal/steamAudio/steamAudio_internal.h"
#include "../internal/steamAudio/steamAudioSpatializer_internal.h"
#include "../internal/soundSystem_internal.h"
#include <phonon.h>

#define STEAMAUDIO_MSG(MSG) "[SteamAudio] " MSG

static IPLContext g_context = nullptr;
static IPLHRTF g_hrtf = nullptr;
static IPLAudioSettings g_audio_settings = {};
static bool g_inited = false;

static TMatrix g_listener_tm = TMatrix::IDENT;

static void IPLCALL steam_audio_log_cb(IPLLogLevel level, const char *message)
{
  logmessage(level == IPL_LOGLEVEL_ERROR ? LOGLEVEL_ERR : LOGLEVEL_DEBUG, "[SNDSYS] SteamAudio: %s", message);
}

static constexpr int MAX_FRAME_SIZE = 1024;

// Per-instance state stored in FMOD_DSP_STATE::plugindata
struct SpatializerInstance
{
  IPLBinauralEffect effect;
  IPLAudioBuffer in_buf;
  IPLAudioBuffer out_buf;
  float in_data_l[MAX_FRAME_SIZE];
  float in_data_r[MAX_FRAME_SIZE];
  float out_data_l[MAX_FRAME_SIZE];
  float out_data_r[MAX_FRAME_SIZE];
  float *in_ptr;
  float *in_ptrs[2];
  float *out_ptrs[2];
  float direction_x;
  float direction_y;
  float direction_z;
  float spatial_blend;
  bool valid;
};

enum SpatializerParam
{
  PARAM_DIR_X = 0,
  PARAM_DIR_Y,
  PARAM_DIR_Z,
  PARAM_SPATIAL_BLEND,
  PARAM_COUNT
};

static FMOD_DSP_PARAMETER_DESC g_param_dir_x;
static FMOD_DSP_PARAMETER_DESC g_param_dir_y;
static FMOD_DSP_PARAMETER_DESC g_param_dir_z;
static FMOD_DSP_PARAMETER_DESC g_param_blend;
static FMOD_DSP_PARAMETER_DESC *g_param_descs[PARAM_COUNT];

static FMOD_RESULT F_CALL dsp_create(FMOD_DSP_STATE *dsp)
{
  if (!g_inited || !g_context || !g_hrtf)
    return FMOD_ERR_PLUGIN;

  SpatializerInstance *inst = new SpatializerInstance();
  inst->valid = false;
  inst->direction_x = 0.f;
  inst->direction_y = 0.f;
  inst->direction_z = 1.f;
  inst->spatial_blend = 1.f;

  IPLBinauralEffectSettings settings = {};
  settings.hrtf = g_hrtf;
  IPLerror err = iplBinauralEffectCreate(g_context, &g_audio_settings, &settings, &inst->effect);
  if (err != IPL_STATUS_SUCCESS)
  {
    delete inst;
    return FMOD_ERR_PLUGIN;
  }

  inst->in_ptr = nullptr;
  inst->in_ptrs[0] = inst->in_data_l;
  inst->in_ptrs[1] = inst->in_data_r;
  inst->in_buf.numChannels = 1;
  inst->in_buf.numSamples = g_audio_settings.frameSize;
  inst->in_buf.data = &inst->in_ptr;
  inst->out_ptrs[0] = inst->out_data_l;
  inst->out_ptrs[1] = inst->out_data_r;
  inst->out_buf.numChannels = 2;
  inst->out_buf.numSamples = g_audio_settings.frameSize;
  inst->out_buf.data = inst->out_ptrs;
  inst->valid = true;

  dsp->plugindata = inst;
  return FMOD_OK;
}

static FMOD_RESULT F_CALL dsp_release(FMOD_DSP_STATE *dsp)
{
  SpatializerInstance *inst = static_cast<SpatializerInstance *>(dsp->plugindata);
  if (inst)
  {
    if (inst->effect)
      iplBinauralEffectRelease(&inst->effect);
    delete inst;
    dsp->plugindata = nullptr;
  }
  return FMOD_OK;
}

static FMOD_RESULT F_CALL dsp_reset(FMOD_DSP_STATE *dsp)
{
  SpatializerInstance *inst = static_cast<SpatializerInstance *>(dsp->plugindata);
  if (inst && inst->effect)
    iplBinauralEffectReset(inst->effect);
  return FMOD_OK;
}

static FMOD_RESULT F_CALL dsp_process(FMOD_DSP_STATE *dsp, unsigned int length, const FMOD_DSP_BUFFER_ARRAY *inbufferarray,
  FMOD_DSP_BUFFER_ARRAY *outbufferarray, FMOD_BOOL inputsidle, FMOD_DSP_PROCESS_OPERATION op)
{
  SpatializerInstance *inst = static_cast<SpatializerInstance *>(dsp->plugindata);
  if (!inst || !inst->valid)
    return FMOD_ERR_PLUGIN;

  if (op == FMOD_DSP_PROCESS_QUERY)
  {
    if (inputsidle)
      return FMOD_ERR_DSP_DONTPROCESS;
    if (outbufferarray)
    {
      outbufferarray->speakermode = FMOD_SPEAKERMODE_STEREO;
      outbufferarray->buffernumchannels[0] = 2;
    }
    return FMOD_OK;
  }

  const int frame_size = (int)length <= MAX_FRAME_SIZE ? (int)length : MAX_FRAME_SIZE;

  inst->in_buf.numSamples = frame_size;
  inst->out_buf.numSamples = frame_size;

  // Use the channel layout actually provided by FMOD.
  const int inputChannels = inbufferarray->buffernumchannels[0];
  const float *fmod_in = inbufferarray->buffers[0];
  if (inputChannels >= 2)
  {
    // FMOD provides an interleaved buffer here; Steam Audio expects deinterleaved (planar) channels.
    for (int i = 0; i < frame_size; ++i)
    {
      inst->in_data_l[i] = fmod_in[inputChannels * i];
      inst->in_data_r[i] = fmod_in[inputChannels * i + 1];
    }
    inst->in_buf.numChannels = 2;
    inst->in_buf.data = inst->in_ptrs;
  }
  else
  {
    inst->in_ptr = const_cast<float *>(fmod_in);
    inst->in_buf.numChannels = 1;
    inst->in_buf.data = &inst->in_ptr;
  }

  IPLBinauralEffectParams params = {};
  params.direction = {inst->direction_x, inst->direction_y, inst->direction_z};
  params.interpolation = IPL_HRTFINTERPOLATION_BILINEAR;
  params.spatialBlend = inst->spatial_blend;
  params.hrtf = g_hrtf;
  params.peakDelays = nullptr;

  iplBinauralEffectApply(inst->effect, &params, &inst->in_buf, &inst->out_buf);

  // float peak = 0.f; // TODO: may check and fix output to prevent distortion
  // for (int i = 0; i < frame_size; ++i)
  //   peak = max(max(peak, fabs(inst->out_data_l[i])), fabs(inst->out_data_r[i]));

  // Handle the output layout allocated by FMOD.
  if (outbufferarray->numbuffers >= 2)
  {
    memcpy(outbufferarray->buffers[0], inst->out_data_l, frame_size * sizeof(float));
    memcpy(outbufferarray->buffers[1], inst->out_data_r, frame_size * sizeof(float));
  }
  else if (outbufferarray->buffernumchannels[0] >= 2)
  {
    for (int i = 0; i < frame_size; ++i)
    {
      outbufferarray->buffers[0][2 * i] = inst->out_data_l[i];
      outbufferarray->buffers[0][2 * i + 1] = inst->out_data_r[i];
    }
  }
  else
  {
    memcpy(outbufferarray->buffers[0], inst->out_data_l, frame_size * sizeof(float));
  }

  return FMOD_OK;
}

static FMOD_RESULT F_CALL dsp_set_float(FMOD_DSP_STATE *dsp, int index, float value)
{
  SpatializerInstance *inst = static_cast<SpatializerInstance *>(dsp->plugindata);
  if (!inst)
    return FMOD_ERR_PLUGIN;
  switch (index)
  {
    case PARAM_DIR_X: inst->direction_x = value; break;
    case PARAM_DIR_Y: inst->direction_y = value; break;
    case PARAM_DIR_Z: inst->direction_z = value; break;
    case PARAM_SPATIAL_BLEND: inst->spatial_blend = value; break;
    default: return FMOD_ERR_INVALID_PARAM;
  }
  return FMOD_OK;
}

static FMOD_RESULT F_CALL dsp_get_float(FMOD_DSP_STATE *dsp, int index, float *value, char *)
{
  SpatializerInstance *inst = static_cast<SpatializerInstance *>(dsp->plugindata);
  if (!inst)
    return FMOD_ERR_PLUGIN;
  switch (index)
  {
    case PARAM_DIR_X: *value = inst->direction_x; break;
    case PARAM_DIR_Y: *value = inst->direction_y; break;
    case PARAM_DIR_Z: *value = inst->direction_z; break;
    case PARAM_SPATIAL_BLEND: *value = inst->spatial_blend; break;
    default: return FMOD_ERR_INVALID_PARAM;
  }
  return FMOD_OK;
}

static FMOD_DSP_DESCRIPTION g_dsp_desc = {};
static bool g_dsp_desc_inited = false;

static void init_dsp_description()
{
  if (g_dsp_desc_inited)
    return;
  FMOD_DSP_INIT_PARAMDESC_FLOAT(g_param_dir_x, "DirX", "", "Direction X component", -1.f, 1.f, 0.f);
  FMOD_DSP_INIT_PARAMDESC_FLOAT(g_param_dir_y, "DirY", "", "Direction Y component", -1.f, 1.f, 0.f);
  FMOD_DSP_INIT_PARAMDESC_FLOAT(g_param_dir_z, "DirZ", "", "Direction Z component", -1.f, 1.f, 1.f);
  FMOD_DSP_INIT_PARAMDESC_FLOAT(g_param_blend, "Blend", "", "Spatial blend 0=off 1=full", 0.f, 1.f, 1.f);
  g_param_descs[0] = &g_param_dir_x;
  g_param_descs[1] = &g_param_dir_y;
  g_param_descs[2] = &g_param_dir_z;
  g_param_descs[3] = &g_param_blend;

  g_dsp_desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
  strncpy(g_dsp_desc.name, "SteamAudio Spatializer", sizeof(g_dsp_desc.name) - 1);
  g_dsp_desc.version = 0x00010000;
  g_dsp_desc.numinputbuffers = 1;
  g_dsp_desc.numoutputbuffers = 1;
  g_dsp_desc.create = dsp_create;
  g_dsp_desc.release = dsp_release;
  g_dsp_desc.reset = dsp_reset;
  g_dsp_desc.process = dsp_process;
  g_dsp_desc.numparameters = PARAM_COUNT;
  g_dsp_desc.paramdesc = g_param_descs;
  g_dsp_desc.setparameterfloat = dsp_set_float;
  g_dsp_desc.getparameterfloat = dsp_get_float;
  g_dsp_desc_inited = true;
}

namespace sndsys::steam_audio
{

bool is_inited() { return g_inited; }

void init(const DataBlock &blk, FMOD::System *fmod_sys)
{
  if (g_inited || !fmod_sys)
    return;

  const DataBlock &sa_blk = *blk.getBlockByNameEx("steamAudio");
  if (!sa_blk.getBool("init", false))
  {
    debug(STEAMAUDIO_MSG("disabled"));
    return;
  }

  IPLContextSettings ctx_settings = {};
  ctx_settings.version = STEAMAUDIO_VERSION;
  ctx_settings.logCallback = steam_audio_log_cb;

  IPLerror err = iplContextCreate(&ctx_settings, &g_context);
  if (err != IPL_STATUS_SUCCESS)
  {
    logerr(STEAMAUDIO_MSG("iplContextCreate failed: %d"), (int)err);
    return;
  }

  // Read DSP buffer size directly from FMOD so frameSize stays in sync
  unsigned fmod_buf_len = 1024;
  int num_bufs = 0;
  fmod_sys->getDSPBufferSize(&fmod_buf_len, &num_bufs);
  fmod_buf_len = min<unsigned>(fmod_buf_len, (unsigned)MAX_FRAME_SIZE);

  int sampleRate;
  FMOD_SPEAKERMODE mode;
  int raw;

  fmod_sys->getSoftwareFormat(&sampleRate, &mode, &raw);
  g_audio_settings.samplingRate = sampleRate;
  g_audio_settings.frameSize = (int)fmod_buf_len;

  IPLHRTFSettings hrtf_settings = {};
  hrtf_settings.type = IPL_HRTFTYPE_DEFAULT;
  hrtf_settings.volume = 1.f;
  hrtf_settings.normType = IPL_HRTFNORMTYPE_NONE;

  err = iplHRTFCreate(g_context, &g_audio_settings, &hrtf_settings, &g_hrtf);
  if (err != IPL_STATUS_SUCCESS)
  {
    logerr(STEAMAUDIO_MSG("iplHRTFCreate failed: %d"), (int)err);
    iplContextRelease(&g_context);
    return;
  }

  g_inited = true;
  debug(STEAMAUDIO_MSG("inited, frameSize=%d sampleRate=%d"), g_audio_settings.frameSize, g_audio_settings.samplingRate);

  spatializer::init(blk, fmod_sys);
}

void shutdown()
{
  if (!g_inited)
    return;
  spatializer::close();
  if (g_hrtf)
    iplHRTFRelease(&g_hrtf);
  if (g_context)
    iplContextRelease(&g_context);
  g_audio_settings = {};
  g_listener_tm = TMatrix::IDENT;
  g_dsp_desc_inited = false;
  g_inited = false;
}

void update_listener(const TMatrix &tm)
{
  if (!g_inited)
    return;
  g_listener_tm = tm;
}

void update() { spatializer::update(); }

void set_spatialized_direction(FMOD::DSP *dsp, const Point3 &source_pos)
{
  if (!dsp)
    return;

  // project into listener space
  Point3 dir = source_pos - g_listener_tm.getcol(3);
  if (lengthSq(dir) < 1e-8f)
    dir = Point3(0, 0, 1);
  dir = normalize(dir);

  const TMatrix &listener = g_listener_tm;
  const Point3 localDir = {dot(dir, listener.getcol(0)), dot(dir, listener.getcol(1)), dot(dir, listener.getcol(2))};

  dsp->setParameterFloat(PARAM_DIR_X, localDir.x);
  dsp->setParameterFloat(PARAM_DIR_Y, localDir.y);
  dsp->setParameterFloat(PARAM_DIR_Z, localDir.z);
}

bool get_spatializer_description(FMOD_DSP_DESCRIPTION &out_desc)
{
  if (!g_inited)
    return false;
  init_dsp_description();
  out_desc = g_dsp_desc;
  return true;
}

} // namespace sndsys::steam_audio
