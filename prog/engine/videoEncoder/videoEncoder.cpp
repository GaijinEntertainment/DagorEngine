// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#include "videoEncoder/videoEncoder.h"
#include <debug/dag_log.h>
#include <util/dag_console.h>
#include <EASTL/type_traits.h>
#include <windows.h>
#include <objbase.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <Codecapi.h>
#include <combaseapi.h>
#include <d3d11.h>
#include <supp/dag_comPtr.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mftransform.h>
#include <mfreadwrite.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform_pc.h>
#include <stdio.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>
#include <Audioclient.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_unicode.h>
#include <perfMon/dag_statDrv.h>

#pragma comment(lib, "Avrt.lib")
#define CHECK_HR(x)                    \
  do                                   \
  {                                    \
    HRESULT hr = x;                    \
    if (FAILED(hr))                    \
    {                                  \
      logerr(#x " failed: %x08x", hr); \
      G_ASSERT(false);                 \
      return false;                    \
    }                                  \
  } while (0)

static HINSTANCE hMFPlat = nullptr;
static HINSTANCE hMFReadWrite = nullptr;
static ComPtr<IMFDXGIDeviceManager> deviceManager;
static ComPtr<IMFSinkWriter> sinkWriter;
static ID3D11Device *device;
static ComPtr<IMMDevice> pMMDevice;
static ComPtr<IAudioClient> pAudioClient;
static ComPtr<IAudioCaptureClient> pAudioCaptureClient;
static WAVEFORMATEX *pwfx = nullptr;
static HANDLE hCaptureStart = nullptr, hCaptureEnd = nullptr;
static HANDLE hProcessStart = nullptr;
static HANDLE hThread = nullptr, hProcessThread = nullptr;
static bool done = false;
static eastl::vector<BYTE> audioBuf;

#define DECL_MFA_APIS            \
  MFA(MFCreateDXGIDeviceManager) \
  MFA(MFCreateDXGISurfaceBuffer) \
  MFA(MFCreateAttributes)        \
  MFA(MFCreateMediaType)         \
  MFA(MFCreateMemoryBuffer)      \
  MFA(MFCreateSample)            \
  MFA(MFShutdown)                \
  MFA(MFStartup)

#define MFA(n) static eastl::add_pointer_t<decltype(::n)> p##n = nullptr;
DECL_MFA_APIS
#undef MFA
static eastl::add_pointer_t<decltype(MFCreateSinkWriterFromURL)> pMFCreateSinkWriterFromURL = nullptr;

// Audio loopback recording based on:
// https://github.com/mvaneerde/blog/blob/3589cd79a3db17e2a5111ff1d47c916307dde9e0/loopback-capture/loopback-capture/loopback-capture.cpp

bool get_default_device(ComPtr<IMMDevice> &ppMMDevice)
{
  HRESULT hr = S_OK;
  ComPtr<IMMDeviceEnumerator> pMMDeviceEnumerator;

  // activate a device enumerator
  CHECK_HR(
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&pMMDeviceEnumerator));

  // get the default render endpoint
  CHECK_HR(pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &ppMMDevice));

  return true;
}
bool list_default_device()
{
  HRESULT hr = S_OK;
  ComPtr<IMMDevice> pMMDevice;
  get_default_device(pMMDevice);
  // get the "n"th device
  // open the property store on that device
  ComPtr<IPropertyStore> pPropertyStore;
  CHECK_HR(pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore));

  // get the long name property
  PROPVARIANT pv;
  PropVariantInit(&pv);
  CHECK_HR(pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv));

  if (VT_LPWSTR != pv.vt)
  {
    logerr("PKEY_Device_FriendlyName variant type is %u - expected VT_LPWSTR", pv.vt);
    PropVariantClear(&pv);
    return false;
  }
  console::print_d("%ls", pv.pwszVal);
  PropVariantClear(&pv);

  return true;
}

bool list_devices()
{
  HRESULT hr = S_OK;

  // get an enumerator
  ComPtr<IMMDeviceEnumerator> pMMDeviceEnumerator;

  CHECK_HR(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
    (void **)&pMMDeviceEnumerator));

  ComPtr<IMMDeviceCollection> pMMDeviceCollection;

  // get all the active render endpoints
  CHECK_HR(pMMDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pMMDeviceCollection));

  UINT count;
  CHECK_HR(pMMDeviceCollection->GetCount(&count));

  for (UINT i = 0; i < count; i++)
  {
    ComPtr<IMMDevice> pMMDevice;

    // get the "n"th device
    CHECK_HR(pMMDeviceCollection->Item(i, &pMMDevice));
    // open the property store on that device
    ComPtr<IPropertyStore> pPropertyStore;
    CHECK_HR(pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore));

    // get the long name property
    PROPVARIANT pv;
    PropVariantInit(&pv);
    CHECK_HR(pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv));

    if (VT_LPWSTR != pv.vt)
    {
      logerr("PKEY_Device_FriendlyName variant type is %u - expected VT_LPWSTR", pv.vt);
      PropVariantClear(&pv);
      return false;
    }
    console::print_d("%ls", pv.pwszVal);
    PropVariantClear(&pv);
  }

  return true;
}

bool get_specific_device(LPCWSTR szLongName, ComPtr<IMMDevice> &ppMMDevice)
{
  HRESULT hr = S_OK;

  // get an enumerator
  ComPtr<IMMDeviceEnumerator> pMMDeviceEnumerator;

  CHECK_HR(::CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
    (void **)&pMMDeviceEnumerator));

  ComPtr<IMMDeviceCollection> pMMDeviceCollection;

  // get all the active render endpoints
  CHECK_HR(pMMDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pMMDeviceCollection));

  UINT count;
  CHECK_HR(pMMDeviceCollection->GetCount(&count));

  for (UINT i = 0; i < count; i++)
  {
    ComPtr<IMMDevice> pMMDevice;

    // get the "n"th device
    CHECK_HR(pMMDeviceCollection->Item(i, &pMMDevice));

    // open the property store on that device
    ComPtr<IPropertyStore> pPropertyStore;
    CHECK_HR(pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore));

    // get the long name property
    PROPVARIANT pv;
    PropVariantInit(&pv);
    hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
    if (FAILED(hr))
    {
      logerr("IPropertyStore::GetValue failed: hr = 0x%08x", hr);
      PropVariantClear(&pv);
      return false;
    }

    if (VT_LPWSTR != pv.vt)
    {
      logerr("PKEY_Device_FriendlyName variant type is %u - expected VT_LPWSTR", pv.vt);
      PropVariantClear(&pv);
      return false;
    }


    // is it a match?
    if (0 == _wcsicmp(pv.pwszVal, szLongName))
    {
      // did we already find it?
      if (!ppMMDevice)
      {
        ppMMDevice = pMMDevice;
        pMMDevice->AddRef();
      }
      else
      {
        PropVariantClear(&pv);
        logerr("Found (at least) two devices named %ls", szLongName);
        return false;
      }
    }
    PropVariantClear(&pv);
  }
  if (!ppMMDevice)
  {
    logerr("Could not find a device named %ls", szLongName);
    return false;
  }

  return true;
}

static bool LoopbackCapture();

static DWORD WINAPI LoopbackCaptureThreadFunction(LPVOID)
{
  LoopbackCapture();
  return 0;
}

static bool LoopbackCapture()
{
  HRESULT hr;
  UINT32 nBlockAlign = pwfx->nBlockAlign;

  // get the default device periodicity
  REFERENCE_TIME hnsDefaultDevicePeriod;
  CHECK_HR(pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, nullptr));

  // create a periodic waitable timer
  HANDLE hWakeUp = CreateWaitableTimer(nullptr, FALSE, nullptr);
  if (nullptr == hWakeUp)
  {
    DWORD dwErr = GetLastError();
    logerr("CreateWaitableTimer failed: last error = %u", dwErr);
    return false;
  }

  DWORD nTaskIndex = 0;
  HANDLE hTask = AvSetMmThreadCharacteristics("Audio", &nTaskIndex);
  if (nullptr == hTask)
  {
    DWORD dwErr = GetLastError();
    logerr("AvSetMmThreadCharacteristics failed: last error = %u", dwErr);
    ::CloseHandle(hWakeUp);
    return false;
  }

  // set the waitable timer
  LARGE_INTEGER liFirstFire;
  liFirstFire.QuadPart = -hnsDefaultDevicePeriod / 2;                      // negative means relative time
  LONG lTimeBetweenFires = (LONG)hnsDefaultDevicePeriod / 2 / (10 * 1000); // convert to milliseconds
  BOOL bOK = ::SetWaitableTimer(hWakeUp, &liFirstFire, lTimeBetweenFires, nullptr, nullptr, FALSE);
  if (!bOK)
  {
    DWORD dwErr = GetLastError();
    logerr("SetWaitableTimer failed: last error = %u", dwErr);
    ::CloseHandle(hTask);
    ::CloseHandle(hWakeUp);
    return false;
  }

  DWORD dwWaitResult;
  bool bFirstPacket = true;
  while (!done)
  {
    ::WaitForSingleObject(hCaptureStart, INFINITE);
    audioBuf.clear();
    UINT32 nNextPacketSize;
    for (hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize); SUCCEEDED(hr) && nNextPacketSize > 0;
         hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize))
    {
      BYTE *pData;
      UINT32 nNumFramesToRead;
      DWORD dwFlags;

      CHECK_HR(pAudioCaptureClient->GetBuffer(&pData, &nNumFramesToRead, &dwFlags, nullptr, nullptr));

      if (bFirstPacket && AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY == dwFlags)
      {
        logwarn("Probably spurious glitch reported on first packet");
      }
      else if (0 != dwFlags)
      {
        logwarn("IAudioCaptureClient::GetBuffer set flags to 0x%08x", dwFlags);
      }

      if (0 == nNumFramesToRead)
      {
        logerr("IAudioCaptureClient::GetBuffer said to read 0 frames");
        ::SetEvent(hCaptureEnd);
        break;
      }

      LONG lBytesToWrite = nNumFramesToRead * nBlockAlign;
      audioBuf.insert(audioBuf.end(), pData, pData + lBytesToWrite);
#pragma prefast(suppress : __WARNING_INCORRECT_ANNOTATION, "IAudioCaptureClient::GetBuffer SAL annotation implies a 1-byte buffer")

      CHECK_HR(pAudioCaptureClient->ReleaseBuffer(nNumFramesToRead));

      bFirstPacket = false;
    }
    ::WaitForSingleObject(hWakeUp, INFINITE);
    ::SetEvent(hCaptureEnd);
  }
  ::CloseHandle(hTask);
  ::CloseHandle(hWakeUp);
  return true;
}

static DWORD WINAPI ProcessThreadFunction(LPVOID data)
{
  VideoEncoder *video_encoder = (VideoEncoder *)data;
  while (video_encoder->isRecording())
  {
    ::WaitForSingleObject(hProcessStart, INFINITE);
    video_encoder->process();
  }
  return 0;
}

bool VideoEncoder::init(void *pUnkDevice)
{
  hMFPlat = ::LoadLibrary("MFPlat.DLL");
  if (!hMFPlat)
    return false;
  auto initFail = []() {
    pMFCreateDXGIDeviceManager = nullptr;
    FreeLibrary(eastl::exchange(hMFPlat, nullptr));
    return false;
  };
#define MFA(n)                                          \
  p##n = (decltype(p##n))::GetProcAddress(hMFPlat, #n); \
  if (!p##n)                                            \
    return initFail();
  DECL_MFA_APIS
#undef MFA
  hMFReadWrite = ::LoadLibrary("MFReadWrite.dll");
  pMFCreateSinkWriterFromURL =
    hMFReadWrite ? (decltype(pMFCreateSinkWriterFromURL))::GetProcAddress(hMFReadWrite, "MFCreateSinkWriterFromURL") : nullptr;
  if (!pMFCreateSinkWriterFromURL)
  {
    hMFReadWrite ? (void)FreeLibrary(eastl::exchange(hMFReadWrite, nullptr)) : (void)0;
    return initFail();
  }

  device = (ID3D11Device *)pUnkDevice;
  ComPtr<ID3D10Multithread> pMultithread;
  device->QueryInterface<ID3D10Multithread>(&pMultithread);
  pMultithread->SetMultithreadProtected(true);

  CHECK_HR(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
  pMFStartup(MF_VERSION, MFSTARTUP_FULL);
  UINT resetToken;
  CHECK_HR(pMFCreateDXGIDeviceManager(&resetToken, &deviceManager));
  CHECK_HR(deviceManager->ResetDevice((IUnknown *)pUnkDevice, resetToken));

  return audioRecorder.init();
}

void VideoEncoder::shutdown()
{
  audioRecorder.shutdown();
  if (!pMFCreateDXGIDeviceManager)
    return;
  deviceManager.Reset();
  pMFShutdown();
  ::CoUninitialize();
  FreeLibrary(eastl::exchange(hMFReadWrite, nullptr));
  FreeLibrary(eastl::exchange(hMFPlat, nullptr));
  pMFCreateDXGIDeviceManager = nullptr;
}

bool VideoEncoder::start(const VideoSettings &settings)
{
  if (!pMFCreateDXGIDeviceManager)
    return false;

  const char *fname = settings.fname.c_str();
  if (!dd_mkpath(fname))
  {
    logerr("Failed to create a path for '%s', try to create the directory manually", fname);
    return false;
  }

  w = (settings.width + 3) / 4 * 4;
  h = (settings.height + 3) / 4 * 4;
  if (w > h && w > 4096)
  {
    float aspect = (float)h / w;
    w = 4096;
    h = w * aspect;
    h = (h + 3) / 4 * 4;
  }
  else if (h > w && h > 4096)
  {
    float aspect = (float)w / h;
    h = 4096;
    w = aspect * h;
    w = (w + 3) / 4 * 4;
  }

  stretchedTex = dag::create_tex(nullptr, w, h, TEXCF_RTARGET, 1, "stretchedVideoTex");
  duration = 10 * 1000 * 1000 / settings.fps;
  ComPtr<IMFAttributes> attribs;
  pMFCreateAttributes(&attribs, 1);
  CHECK_HR(attribs->SetUnknown(MF_SINK_WRITER_D3D_MANAGER, deviceManager.Get()));

  wchar_t wfname[_MAX_PATH] = {L'\0'};
  utf8_to_wcs(fname, wfname, _MAX_PATH);
  CHECK_HR(pMFCreateSinkWriterFromURL(wfname, nullptr, attribs.Get(), &sinkWriter));

  HRESULT hr;
  ComPtr<IMFMediaType> pMediaTypeOut;
  CHECK_HR(pMFCreateMediaType(&pMediaTypeOut));
  CHECK_HR(pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
  CHECK_HR(pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));
  CHECK_HR(pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, settings.bitrate));
  CHECK_HR(pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
  CHECK_HR(::MFSetAttributeSize(pMediaTypeOut.Get(), MF_MT_FRAME_SIZE, w, h));
  CHECK_HR(::MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_FRAME_RATE, settings.fps, 1));
  CHECK_HR(::MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
  CHECK_HR(sinkWriter->AddStream(pMediaTypeOut.Get(), &videoStreamIndex));

  ComPtr<IMFMediaType> pMediaTypeIn;
  CHECK_HR(pMFCreateMediaType(&pMediaTypeIn));
  CHECK_HR(pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
  CHECK_HR(pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32));
  CHECK_HR(pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
  CHECK_HR(::MFSetAttributeSize(pMediaTypeIn.Get(), MF_MT_FRAME_SIZE, w, h));
  CHECK_HR(::MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_FRAME_RATE, settings.fps, 1));
  CHECK_HR(MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
  CHECK_HR(sinkWriter->SetInputMediaType(videoStreamIndex, pMediaTypeIn.Get(), nullptr));

  ComPtr<IMFMediaType> pAudioMediaTypeOut;
  CHECK_HR(pMFCreateMediaType(&pAudioMediaTypeOut));
  CHECK_HR(pAudioMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
  CHECK_HR(pAudioMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC));
  CHECK_HR(pAudioMediaTypeOut->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16));
  CHECK_HR(pAudioMediaTypeOut->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100));
  CHECK_HR(pAudioMediaTypeOut->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2));
  CHECK_HR(pAudioMediaTypeOut->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000));
  CHECK_HR(pAudioMediaTypeOut->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1));
  CHECK_HR(pAudioMediaTypeOut->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0x29));
  CHECK_HR(sinkWriter->AddStream(pAudioMediaTypeOut.Get(), &audioStreamIndex));

  ComPtr<IMFMediaType> pAudioMediaTypeIn;
  CHECK_HR(pMFCreateMediaType(&pAudioMediaTypeIn));
  CHECK_HR(pAudioMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
  CHECK_HR(pAudioMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM));
  CHECK_HR(pAudioMediaTypeIn->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2));
  CHECK_HR(pAudioMediaTypeIn->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100));
  CHECK_HR(pAudioMediaTypeIn->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16));
  CHECK_HR(pAudioMediaTypeOut->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1));
  CHECK_HR(sinkWriter->SetInputMediaType(audioStreamIndex, pAudioMediaTypeIn.Get(), nullptr));

  CHECK_HR(sinkWriter->BeginWriting());

  cbWidth = 4 * w;
  cbBuffer = cbWidth * h;
  recording = true;
  rtStart = 0;

  currentAudioSample = 0;

  bool ret = audioRecorder.start();
  hProcessThread = ::CreateThread(nullptr, 0, ProcessThreadFunction, this, 0, nullptr); //-V513
  if (nullptr == hProcessThread)
  {
    logerr("CreateThread failed: last error is %u", GetLastError());
    return false;
  }
  hProcessStart = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
  return ret;
}

bool VideoEncoder::process()
{
  ComPtr<IMFSample> pVideoSample;

  ID3D11Texture2D *surface = (ID3D11Texture2D *)d3d::pcwin::get_native_surface(stretchedTex.getTex2D());
  ComPtr<IMFMediaBuffer> inputBuffer;
  CHECK_HR(pMFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), surface, 0, FALSE, &inputBuffer));
  CHECK_HR(inputBuffer->SetCurrentLength(cbBuffer));
  CHECK_HR(pMFCreateSample(&pVideoSample));
  CHECK_HR(pVideoSample->AddBuffer(inputBuffer.Get()));
  CHECK_HR(pVideoSample->SetSampleTime(rtStart));
  CHECK_HR(pVideoSample->SetSampleDuration(duration));
  {
    TIME_D3D_PROFILE(videoEncoder_process_write);
    CHECK_HR(sinkWriter->WriteSample(videoStreamIndex, pVideoSample.Get()));
  }
  if (!externalAudioBuffersPerSample.empty())
  {
    if (currentAudioSample < externalAudioBuffersPerSample.size())
      if (!setAudioSample(externalAudioBuffersPerSample[currentAudioSample]))
        return false;
    ++currentAudioSample;
  }
  else
  {
    ::SetEvent(hCaptureStart);
    ::WaitForSingleObject(hCaptureEnd, INFINITE);
    if (!setAudioSample(audioBuf))
      return false;
  }
  rtStart += duration;
  return true;
}

bool VideoEncoder::process(BaseTexture *tex)
{
  if (!recording)
  {
    if (audioRecorder.isRecording())
      return audioRecorder.process();
    return true;
  }
  if (!pMFCreateDXGIDeviceManager)
    return false;
  d3d::stretch_rect(tex, stretchedTex.getTex2D());
  ::SetEvent(hProcessStart);
  return true;
}

bool VideoEncoder::stop()
{
  // Intentionally clear the possibly leftover audiobuffer contents, by moving out
  // from the recorder and clearing it, to get back to default state.
  audioRecorder.stop(static_cast<AudioBuffer &&>(externalAudioBuffersPerSample));
  externalAudioBuffersPerSample.clear();
  if (!recording)
    return false;
  if (!pMFCreateDXGIDeviceManager)
    return false;
  recording = false;
  ::CloseHandle(hProcessThread);
  hProcessThread = nullptr;
  ::CloseHandle(hProcessStart);
  hProcessStart = nullptr;
  stretchedTex.close();
  return true;
}

VideoEncoder::~VideoEncoder() = default;

bool VideoEncoder::setAudioSample(const eastl::vector<uint8_t> &audioBuf)
{
  if (!audioBuf.empty())
  {
    ComPtr<IMFSample> pAudioSample;
    ComPtr<IMFMediaBuffer> pBuffer;
    HRESULT hr = pMFCreateMemoryBuffer(audioBuf.size(), &pBuffer);
    BYTE *pOut;
    CHECK_HR(pBuffer->Lock(&pOut, nullptr, nullptr));
    memcpy(pOut, audioBuf.data(), audioBuf.size());
    pBuffer->Unlock();
    CHECK_HR(pBuffer->SetCurrentLength(audioBuf.size()));
    CHECK_HR(pMFCreateSample(&pAudioSample));
    CHECK_HR(pAudioSample->AddBuffer(pBuffer.Get()));
    CHECK_HR(pAudioSample->SetSampleDuration(duration));
    CHECK_HR(pAudioSample->SetSampleTime(rtStart));
    CHECK_HR(sinkWriter->WriteSample(audioStreamIndex, pAudioSample.Get()));
  }
  return true;
}

bool AudioRecorder::init()
{
  ::get_default_device(pMMDevice);

  CHECK_HR(pMMDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void **)&pAudioClient));

  CHECK_HR(pAudioClient->GetMixFormat(&pwfx));

  switch (pwfx->wFormatTag)
  {
    case WAVE_FORMAT_IEEE_FLOAT:
      pwfx->wFormatTag = WAVE_FORMAT_PCM;
      pwfx->wBitsPerSample = 16;
      pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
      pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
      break;

    case WAVE_FORMAT_EXTENSIBLE:
    {
      // naked scope for case-local variable
      PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
      if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat))
      {
        pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        pEx->Samples.wValidBitsPerSample = 16;
        pwfx->wBitsPerSample = 16;
        pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
        pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
      }
      else
      {
        logerr("Don't know how to coerce mix format to int-16");
        return false;
      }
    }
    break;

    default: logerr("Don't know how to coerce WAVEFORMATEX with wFormatTag = 0x%08x to int-16", pwfx->wFormatTag); return false;
  }

  // call IAudioClient::Initialize
  // note that AUDCLNT_STREAMFLAGS_LOOPBACK and AUDCLNT_STREAMFLAGS_EVENTCALLBACK
  // do not work together...
  // the "data ready" event never gets set
  // so we're going to do a timer-driven loop
  CHECK_HR(pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pwfx, 0));

  // activate an IAudioCaptureClient
  CHECK_HR(pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void **)&pAudioCaptureClient));

  CHECK_HR(pAudioClient->Start());

  return true;
}

bool AudioRecorder::start()
{
  done = false;
  hCaptureStart = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
  hCaptureEnd = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
  hThread = ::CreateThread(nullptr, 0, LoopbackCaptureThreadFunction, nullptr, 0, nullptr); //-V513
  if (nullptr == hThread)
  {
    logerr("CreateThread failed: last error is %u", GetLastError());
    return false;
  }
  recording = true;
  return true;
}

bool AudioRecorder::process()
{
  if (!recording)
    return false;
  ::SetEvent(hCaptureStart);
  ::WaitForSingleObject(hCaptureEnd, INFINITE);
  audioBuffersPerSample.emplace_back(std::move(audioBuf));
  return true;
}

bool AudioRecorder::stop(AudioBuffer &&audioBuffersPerSample)
{
  done = true;
  audioBuffersPerSample = std::move(this->audioBuffersPerSample);
  audioBuf.clear();
  if (pwfx)
  {
    ::CoTaskMemFree(pwfx);
    pwfx = nullptr;
  }
  if (pAudioClient)
    pAudioClient->Stop();
  if (sinkWriter)
  {
    sinkWriter->Finalize();
    sinkWriter.Reset();
  }
  recording = false;
  return true;
}

void AudioRecorder::shutdown()
{
  ::CloseHandle(hThread);
  hThread = nullptr;
  ::CloseHandle(hCaptureStart);
  hCaptureStart = nullptr;
  ::CloseHandle(hCaptureEnd);
  hCaptureEnd = nullptr;
  if (pMMDevice)
    pMMDevice.Reset();
  audioBuf.clear();
}

void VideoEncoder::setExternalAudioBuffer(AudioBuffer &&audioBuffersPerSample)
{
  externalAudioBuffersPerSample = eastl::move(audioBuffersPerSample);
}

#endif