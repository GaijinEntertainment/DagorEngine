#include "xess_wrapper.h"

using namespace drv3d_dx12;

namespace drv3d_dx12
{
class XessWrapperImpl
{};
} // namespace drv3d_dx12

bool XessWrapper::xessInit(void *) { return false; }

bool XessWrapper::xessCreateFeature(int, uint32_t, uint32_t) { return false; }

bool XessWrapper::xessShutdown() { return false; }

bool XessWrapper::evaluateXess(void *, const void *) { return false; }

XessState XessWrapper::getXessState() const { return XessState::DISABLED; }

void XessWrapper::getXeSSRenderResolution(int &, int &) const {}

void XessWrapper::setVelocityScale(float, float) {}

bool XessWrapper::isXessQualityAvailableAtResolution(uint32_t, uint32_t, int) const { return false; }

XessWrapper::XessWrapper() = default;

XessWrapper::~XessWrapper() = default;
