// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dlss.h"

namespace drv3d_dx12
{

DLSSSuperResolutionDirect::DLSSSuperResolutionDirect() {}
DLSSSuperResolutionDirect::~DLSSSuperResolutionDirect() {}

bool DLSSSuperResolutionDirect::Initialize(ID3D12Device *, IDXGIAdapter *) { return false; }

void DLSSSuperResolutionDirect::Teardown(ID3D12Device *) {}

bool DLSSSuperResolutionDirect::evaluate(const nv::DlssParams<void> &, void *) { return false; }

eastl::optional<nv::DLSS::OptimalSettings> DLSSSuperResolutionDirect::getOptimalSettings(Mode, IPoint2) const
{
  return eastl::nullopt;
}

bool DLSSSuperResolutionDirect::setOptions(Mode, IPoint2, bool, bool) { return false; }

void DLSSSuperResolutionDirect::DeleteFeature() {}

nv::DLSS::State DLSSSuperResolutionDirect::getState() { return nv::DLSS::State::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE; }

bool DLSSSuperResolutionDirect::supportRayReconstruction() { return false; }

uint64_t DLSSSuperResolutionDirect::getMemoryUsage() { return 0; }

dag::Expected<eastl::string, nv::SupportState> DLSSSuperResolutionDirect::getVersion() const
{
  return dag::Unexpected(nv::SupportState::NotSupported);
}

bool DLSSSuperResolutionDirect::setOptionsBackend(ID3D12GraphicsCommandList *, Mode, IPoint2, bool, bool) { return false; }

} // namespace drv3d_dx12
