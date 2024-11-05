// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_interface_ex.h>

class VisibilityFinderService : public IVisibilityFinderProvider
{
  VisibilityFinder &getVisibilityFinder() override { return visibilityFinder; }

protected:
  VisibilityFinder visibilityFinder;
};

static VisibilityFinderService srv;

void *get_visibility_finder_service() { return &srv; }