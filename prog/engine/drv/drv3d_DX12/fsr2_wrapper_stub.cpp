// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device.h"
#include "fsr2_wrapper.h"

using namespace drv3d_dx12;

struct FfxFsr2ContextDescription
{};
struct FfxFsr2Context
{};

bool Fsr2Wrapper::init(void *) { return false; }
bool Fsr2Wrapper::createContext(uint32_t, uint32_t, int) { return false; }
void Fsr2Wrapper::evaluateFsr2(void *, const void *) {}
void Fsr2Wrapper::shutdown() {}
Fsr2State Fsr2Wrapper::getFsr2State() const { return Fsr2State::NOT_CHECKED; }
void Fsr2Wrapper::getFsr2RenderingResolution(int &, int &) const {}
Fsr2Wrapper::Fsr2Wrapper() = default;
Fsr2Wrapper::~Fsr2Wrapper() = default;
