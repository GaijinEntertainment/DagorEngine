// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/fileTexFactory.h>

SymbolicTextureFactory SymbolicTextureFactory::self;


TextureFactory *get_symbolic_tex_factory() { return &SymbolicTextureFactory::self; }
