// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// @TODO: add to versioning docs
// Increase this when changes are made to stcode compilation process, which require recompiling the generated c++ sources without
// changing their content, for example when compilation cache pathes change. This version is replicated in the obj cache, compiler
// binary, shader dump and stcode dll (the last two are checked to be the same on load).

inline constexpr int CPP_STCODE_COMMON_VER = 7;
