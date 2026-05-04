// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>

class DataBlock;
class IModelessWindowController;

class ModelessWindowControllerList
{
public:
  // Adds a window controller to the list. Do not add the same controller multiple time.
  void addWindowController(IModelessWindowController &controller);

  // Toggle the window's visibility state between shown and hidden.
  void toggleShowById(const char *id);

  // Releases all windows belonging to the registered controllers.
  void releaseAllWindows();

  // Resets the state of all registered windows to their defaults.
  void setDefaultState();

  void loadState(const DataBlock &blk);
  void saveState(DataBlock &blk);

private:
  int getIndexById(const char *id) const;

  dag::Vector<IModelessWindowController *> windowControllers;
};
