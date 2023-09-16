//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_DObject.h>
#include <3d/dag_drv3dConsts.h>

class ShaderMaterial;
class ShaderElement;

// Class that renders full-screen quad for post-effect shaders.
class PostFxRenderer
{
public:
  PostFxRenderer();
  ~PostFxRenderer();
  PostFxRenderer(PostFxRenderer &&other);
  PostFxRenderer(const PostFxRenderer &other);
  PostFxRenderer &operator=(PostFxRenderer &&other);
  PostFxRenderer &operator=(const PostFxRenderer &other);
  explicit PostFxRenderer(const char *shader_name);

  void clear();
  void init(const char *shader_name, const char *mat_script = NULL, bool do_fatal = true);

  // Use to set shader params.
  ShaderMaterial *getMat() { return shmat; }
  ShaderElement *getElem() { return shElem; }
  const ShaderMaterial *getMat() const { return shmat; }
  const ShaderElement *getElem() const { return shElem; }

  void render() const { drawInternal(1); }                            // draw optimal
  void renderTiled(int num_tiles) const { drawInternal(num_tiles); }; // With Flush between tiles.
protected:
  Ptr<ShaderMaterial> shmat;
  Ptr<ShaderElement> shElem;

  void drawInternal(int num_tiles) const;
};


inline PostFxRenderer *create_postfx_renderer(const char *shader_name, bool do_fatal)
{
  PostFxRenderer *r = new PostFxRenderer();
  r->init(shader_name, NULL, do_fatal);
  if (!r->getMat())
    del_it(r);
  return r;
}
