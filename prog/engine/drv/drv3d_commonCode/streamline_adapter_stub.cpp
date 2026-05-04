// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "streamline_adapter.h"

struct StreamlineAdapter::InitArgs
{};

StreamlineAdapter::StreamlineAdapter(eastl::unique_ptr<InitArgs> &&, SupportOverrideMap) {}
StreamlineAdapter::~StreamlineAdapter() {}

bool StreamlineAdapter::init(eastl::optional<StreamlineAdapter> &, RenderAPI, SupportOverrideMap) { return false; }
void StreamlineAdapter::setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D11Device *device) {}
void StreamlineAdapter::setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D12Device5 *device) {}
void StreamlineAdapter::setVulkan() {}
void *StreamlineAdapter::hook(IUnknown *object) { return object; }
void *StreamlineAdapter::unhook(IUnknown *object)
{
#if _TARGET_PC_WIN
  if (object)
    object->AddRef();
#endif
  return object;
}

void *StreamlineAdapter::getInterposerSymbol(const InterposerHandleType &, const char *) { return nullptr; }
StreamlineAdapter::InterposerHandleType StreamlineAdapter::loadInterposer() { return {nullptr, nullptr}; }
void StreamlineAdapter::preRecover() {}
void StreamlineAdapter::recover() {}

nv::SupportState StreamlineAdapter::isDlssSupported() const { return nv::SupportState::NotSupported; }
nv::SupportState StreamlineAdapter::isDlssGSupported() const { return nv::SupportState::NotSupported; }
nv::SupportState StreamlineAdapter::isDlssRRSupported() const { return nv::SupportState::NotSupported; }
nv::SupportState StreamlineAdapter::isReflexSupported() const { return nv::SupportState::NotSupported; }

dag::Expected<eastl::string, nv::SupportState> StreamlineAdapter::getDlssVersion() const
{
  return dag::Unexpected(nv::SupportState::NotSupported);
}
bool DLSSSuperResolution::isModeAvailableAtResolution(nv::DLSS::Mode, const IPoint2 &) { return false; }
nv::DLSS *StreamlineAdapter::createDlssFeature(int, IPoint2, void *) { return nullptr; }
DLSSFrameGeneration *StreamlineAdapter::createDlssGFeature(int, void *) { return nullptr; }
Reflex *StreamlineAdapter::createReflexFeature() { return nullptr; }
uint64_t StreamlineAdapter::getMemorySize() const { return {}; }

DLSSFrameGeneration::~DLSSFrameGeneration() {}
void DLSSFrameGeneration::setEnabled(int) {}
void DLSSFrameGeneration::setSuppressed(bool) {}
bool DLSSFrameGeneration::evaluate(const nv::DlssGParams<void> &, void *) { return true; }
unsigned DLSSFrameGeneration::getActualFramesPresented() const { return 1; }
int DLSSFrameGeneration::getMaximumNumberOfGeneratedFrames() { return 1; }

bool Reflex::setOptions(GpuLatency::Mode, unsigned) { return false; }
eastl::optional<Reflex::State> Reflex::getState() const { return eastl::nullopt; }
bool Reflex::sleep(uint32_t) { return false; }
void Reflex::startFrame(uint32_t) {}
bool Reflex::setMarker(uint32_t, lowlatency::LatencyMarkerType) { return false; }
