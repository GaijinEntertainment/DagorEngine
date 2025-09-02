// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;
struct E3DCOLOR;

namespace GizmoSettings
{

void load(DataBlock &block);
void save(DataBlock &block);
void updateAxisColors();
void retrieveAxisColors(E3DCOLOR &colorX, E3DCOLOR &colorY, E3DCOLOR &colorZ);

inline int lineThickness = 2;
inline float viewportGizmoScaleFactor = 1.0f;
inline bool overrideColor[3] = {false, false, false};
extern E3DCOLOR axisColor[3];

}; // namespace GizmoSettings
