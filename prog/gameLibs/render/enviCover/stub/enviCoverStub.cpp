// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/enviCover/enviCover.h>
#include <render/nodeBasedShader.h>
#include <shaders/dag_computeShaders.h>

EnviCover::EnviCover() {}
EnviCover::~EnviCover() {}
void EnviCover::initShader(const String &) {}
bool EnviCover::updateShaders(const String &, const DataBlock &, String &) { return false; }
void EnviCover::initRender(EnviCoverUseType) {}
void EnviCover::render(int, int, const eastl::array<BaseTexture *, 4> &) {}
void EnviCover::closeShaders() {}