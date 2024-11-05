// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_picture.h>
#include "scriptBinding.h"
#include <sqModules/sqModules.h>

// To consider: move this to some public engine header
extern void lottie_discard_animation(PICTUREID pid);

namespace darg
{

void bind_lottie_animation(SqModules *module_mgr, Sqrat::Table *exps)
{
  HSQUIRRELVM sqvm = module_mgr->getVM();

  ///@class daRg/LottieAnimation
  ///@extends Picture
  Sqrat::DerivedClass<LottieAnimation, Picture, Sqrat::NoCopy<LottieAnimation>> lottieAnimationClass(sqvm, "LottieAnimation");
  lottieAnimationClass.SquirrelCtor(binding::picture_script_ctor<LottieAnimation>, 0, ".o|s");

  auto exports = exps ? exps : module_mgr->findNativeModule("daRg");
  G_ASSERT_RETURN(exports, );
  static_cast<Sqrat::Table *>(exports)->Bind("LottieAnimation", lottieAnimationClass);
}

LottieAnimation::~LottieAnimation()
{
  if (pic.pic != BAD_PICTUREID)
    lottie_discard_animation(pic.pic);
}

} // namespace darg
