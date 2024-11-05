// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class StaticGeometryContainer;
class RemoverNotificationCB
{
public:
  virtual void note(const char *) = 0;
  virtual void warning(const char *) = 0;
};

void remove_inside_faces(StaticGeometryContainer &cont, RemoverNotificationCB *cb);
