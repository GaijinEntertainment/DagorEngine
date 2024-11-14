// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "streamline_adapter.h"

eastl::optional<StreamlineAdapter> StreamlineAdapter::create(StreamlineAdapter::RenderAPI, SupportOverrideMap)
{
  return eastl::nullopt;
}

void *StreamlineAdapter::getInterposerSymbol(const char *) { return nullptr; }

void StreamlineAdapter::setD3DDevice(void *) {}

void *StreamlineAdapter::hook(void *o) { return o; }

StreamlineAdapter::StreamlineAdapter(InterposerHandleType, SupportOverrideMap) : interposer(nullptr, [](void *) { return false; }) {}

StreamlineAdapter::~StreamlineAdapter() {}

nv::SupportState StreamlineAdapter::isDlssSupported() const { return nv::SupportState::NotSupported; }

nv::SupportState StreamlineAdapter::isDlssGSupported() const { return nv::SupportState::NotSupported; }

nv::SupportState StreamlineAdapter::isReflexSupported() const { return nv::SupportState::NotSupported; }

void StreamlineAdapter::startFrame(uint32_t) {}

void StreamlineAdapter::initializeReflexState() {}

bool StreamlineAdapter::setMarker(uint32_t, lowlatency::LatencyMarkerType) { return false; }

bool StreamlineAdapter::setReflexOptions(nv::Reflex::ReflexMode, unsigned) { return false; }

eastl::optional<nv::Reflex::ReflexState> StreamlineAdapter::getReflexState() const { return eastl::nullopt; }

bool StreamlineAdapter::sleep() { return false; }

nv::Reflex::ReflexMode StreamlineAdapter::getReflexMode() const { return nv::Reflex::ReflexMode::Off; }

uint32_t StreamlineAdapter::getFrameId() const { return 0; }

eastl::optional<nv::DLSS::OptimalSettings> StreamlineAdapter::getOptimalSettings(nv::DLSS::Mode, IPoint2) const
{
  return eastl::nullopt;
}

bool StreamlineAdapter::setOptions(int, nv::DLSS::Mode, IPoint2, float) { return false; }

bool StreamlineAdapter::evaluateDlss(uint32_t, int, const nv::DlssParams<void> &, void *) { return false; }

void StreamlineAdapter::initializeDlssState() {}

bool StreamlineAdapter::createDlssFeature(int, IPoint2, void *) { return false; }

bool StreamlineAdapter::releaseDlssFeature(int) { return false; }

dag::Expected<eastl::string, nv::SupportState> StreamlineAdapter::getDlssVersion() const
{
  return dag::Unexpected(nv::SupportState::NotSupported);
}

bool StreamlineAdapter::createDlssGFeature(int, void *) { return false; }

bool StreamlineAdapter::releaseDlssGFeature(int) { return false; }

bool StreamlineAdapter::isFrameGenerationSupported() const { return false; }

bool StreamlineAdapter::enableDlssG(int) { return false; }

bool StreamlineAdapter::disableDlssG(int) { return false; }

void StreamlineAdapter::setDlssGSuppressed(bool) {}

bool StreamlineAdapter::evaluateDlssG(int, const nv::DlssGParams<void> &, void *) { return false; }

unsigned StreamlineAdapter::getActualFramesPresented() const { return 0; }