// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class IECSTemplateSelectorClient
{
public:
  virtual void onTemplateSelectorTemplateSelected(const char *template_name) = 0;
};
