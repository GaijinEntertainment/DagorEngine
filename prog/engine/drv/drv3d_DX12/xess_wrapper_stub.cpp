// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "xess_wrapper.h"

using namespace drv3d_dx12;

namespace drv3d_dx12
{
class XessWrapperImpl
{};
} // namespace drv3d_dx12

eastl::string XessWrapper::errorKindToString(ErrorKind) { return {}; }

bool XessWrapper::xessInit(void *) { return false; }

bool XessWrapper::xessCreateFeature(int, uint32_t, uint32_t) { return false; }

bool XessWrapper::xessShutdown() { return false; }

bool XessWrapper::evaluateXess(void *, const void *) { return false; }

XessState XessWrapper::getXessState() const { return XessState::DISABLED; }

void XessWrapper::getXeSSRenderResolution(int &, int &) const {}

void XessWrapper::setVelocityScale(float, float) {}

bool XessWrapper::isXessQualityAvailableAtResolution(uint32_t, uint32_t, int) const { return false; }

void XessWrapper::startDump(const char *, int) {}

dag::Expected<eastl::string, XessWrapper::ErrorKind> XessWrapper::getVersion() const { return dag::Unexpected(ErrorKind::Unknown); }

XessWrapper::XessWrapper() = default;

XessWrapper::~XessWrapper() = default;
