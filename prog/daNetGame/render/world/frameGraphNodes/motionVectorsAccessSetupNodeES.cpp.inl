// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>

#include <render/renderEvent.h>
#include <render/world/cameraParams.h>
#include <render/motionVectorAccess.h>
#include <render/world/frameGraphHelpers.h>


ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_motion_vector_access_setup_node_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(dafg::register_node("motion_vector_access_setup_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
    auto heroMatrixParamsHndl =
      registry.readBlob<eastl::optional<motion_vector_access::HeroMatrixParams>>("hero_matrix_params").handle();

    registry.createBlob<OrderingToken>("motion_vector_access_token"); // something to use in dependent
                                                                      // nodes

    return [cameraHndl, prevCameraHndl, heroMatrixParamsHndl] {
      motion_vector_access::CameraParams currentCamera{cameraHndl.ref().viewTm, cameraHndl.ref().viewItm,
        cameraHndl.ref().noJitterProjTm, cameraHndl.ref().cameraWorldPos, cameraHndl.ref().znear, cameraHndl.ref().zfar};
      motion_vector_access::CameraParams previousCamera{prevCameraHndl.ref().viewTm, prevCameraHndl.ref().viewItm,
        prevCameraHndl.ref().noJitterProjTm, prevCameraHndl.ref().cameraWorldPos, prevCameraHndl.ref().znear,
        prevCameraHndl.ref().zfar};
      motion_vector_access::set_params(currentCamera, previousCamera, cameraHndl.ref().jitterOffsetUv,
        prevCameraHndl.ref().jitterOffsetUv, heroMatrixParamsHndl.ref());
    };
  }));
}
