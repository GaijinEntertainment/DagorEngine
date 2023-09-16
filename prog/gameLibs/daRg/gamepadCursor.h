#pragma once

#include <math/dag_Point2.h>


namespace HumanInput
{
class IGenJoystick;
}


namespace darg
{

class GuiScene;

class GamepadCursor
{
public:
  GamepadCursor(GuiScene *gui_scene);
  void update(float dt);

public:
  static const int SECTOR_NUM = 8; //[0..7]
  static const int SECTOR_NONE = -1;

  bool isMovingMouse = false;

private:
  Point2 moveMouse(HumanInput::IGenJoystick *joy, const Point2 &mousePos, float dt);
  void scroll(HumanInput::IGenJoystick *joy, float dt);

private:
  int currentSector = SECTOR_NONE;
  float timeInCurrentSector = 0;

  GuiScene *guiScene = nullptr;
  float dxCup = 0;
  float dyCup = 0;
};

} // namespace darg
