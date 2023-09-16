#include <scene/dag_occlusion.h>
#include <ioSys/dag_genIo.h>

static const int version = 1;
static const int magic_number = MAKE4C('O', 'C', 'C', 'L');

void save_occlusion(class IGenSave &cb, const Occlusion &occlusion)
{
  cb.beginTaggedBlock(magic_number);
  cb.writeInt(version);

  cb.beginBlock();
  const mat44f view = occlusion.getCurView();
  const mat44f proj = occlusion.getCurProj();
  const mat44f viewProj = occlusion.getCurViewProj();
  const vec3f pos = occlusion.getCurViewPos();
  const float cockpitDist = occlusion.getCockpitDistance();
  const CockpitReprojectionMode cockpitMode = occlusion.getCockpitReprojectionMode();
  const bool reprojectionUseCameraTranslatedSpace = occlusion.getReprojectionUseCameraTranslatedSpace();
  const mat44f cockpitAnim = occlusion.getCockpitAnim();
  cb.write(&view, sizeof(view));
  cb.write(&proj, sizeof(proj));
  cb.write(&viewProj, sizeof(viewProj));
  cb.write(&pos, sizeof(pos));
  cb.write(&cockpitDist, sizeof(cockpitDist));
  cb.write(&cockpitMode, sizeof(cockpitMode));
  cb.write(&reprojectionUseCameraTranslatedSpace, sizeof(reprojectionUseCameraTranslatedSpace));
  cb.write(&cockpitAnim, sizeof(cockpitAnim));
  cb.endBlock();

  cb.beginBlock();
  cb.write(OcclusionTest<OCCLUSION_W, OCCLUSION_H>::getZbuffer(), sizeof(float) * OCCLUSION_W * OCCLUSION_H);
  cb.endBlock();
}

bool load_occlusion(class IGenLoad &cb, Occlusion &occlusion)
{
  if (cb.beginTaggedBlock() != magic_number || cb.readInt() != version)
  {
    cb.endBlock();
    return false;
  }
  cb.beginBlock();
  mat44f view;
  mat44f proj;
  mat44f viewProj;
  vec3f pos;
  float cockpitDist;
  CockpitReprojectionMode cockpitMode;
  bool reprojectionUseCameraTranslatedSpace;
  mat44f cockpitAnim;
  cb.read(&view, sizeof(view));
  cb.read(&proj, sizeof(proj));
  cb.read(&viewProj, sizeof(viewProj));
  cb.read(&pos, sizeof(pos));
  cb.read(&cockpitDist, sizeof(cockpitDist));
  cb.read(&cockpitMode, sizeof(cockpitMode));
  cb.read(&reprojectionUseCameraTranslatedSpace, sizeof(reprojectionUseCameraTranslatedSpace));
  cb.read(&cockpitAnim, sizeof(cockpitAnim));
  cb.endBlock();
  occlusion.setReprojectionUseCameraTranslatedSpace(reprojectionUseCameraTranslatedSpace);
  occlusion.startFrame(pos, view, proj, viewProj, cockpitDist, cockpitMode, cockpitAnim, 0, 0);

  cb.beginBlock();
  cb.read(OcclusionTest<OCCLUSION_W, OCCLUSION_H>::getZbuffer(), sizeof(float) * OCCLUSION_W * OCCLUSION_H);
  OcclusionTest<OCCLUSION_W, OCCLUSION_H>::buildMips();
  cb.endBlock();
  return true;
}
