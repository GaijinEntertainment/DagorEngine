// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scene.h"

#include <dasModules/dasModulesCommon.h>
#include <render/daFrameGraph/das/registerBlobType.h>


namespace bind_dascript
{

struct OcclusionAnnotation : das::ManagedStructureAnnotation<Occlusion, false>
{
  OcclusionAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Occlusion", ml, "::Occlusion") {}
};

class SceneModule final : public das::Module
{
public:
  SceneModule() : das::Module("Scene")
  {
    das::ModuleLibrary lib(this);

    addAnnotation(das::make_smart<OcclusionAnnotation>(lib));
    dafg::das::register_interop_type<Occlusion *>(lib);

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &) const override { return das::ModuleAotType::cpp; }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(SceneModule, bind_dascript);
