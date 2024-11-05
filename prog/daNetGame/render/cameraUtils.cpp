// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cameraUtils.h"
#define INSIDE_RENDERER 1
#include "world/private_worldRenderer.h"

void screen_to_world(IPoint2 screen_size,
  const TMatrix &camera,
  const TMatrix4 &projMatrix,
  const Point2 &screen_pos,
  Point3 &world_pos,
  Point3 &world_dir)
{
  Point2 normalizedScreen;
  normalizedScreen.x = (2 * screen_pos.x / screen_size.x - 1) / projMatrix[0][0];
  normalizedScreen.y = (1 - 2 * screen_pos.y / screen_size.y) / projMatrix[1][1];

  world_pos = camera.getcol(3);
  world_dir = normalize(camera.getcol(0) * normalizedScreen.x + camera.getcol(1) * normalizedScreen.y + camera.getcol(2));
}

void screen_to_world(const Point2 &screen_pos, Point3 &world_pos, Point3 &world_dir)
{
  if (auto wr = (const WorldRenderer *)get_world_renderer())
  {
    IPoint2 screenSize;
    wr->getDisplayResolution(screenSize.x, screenSize.y);
    screen_to_world(screenSize, wr->getCameraViewItm(), wr->getCameraProjtm(), screen_pos, world_pos, world_dir);
  }
}