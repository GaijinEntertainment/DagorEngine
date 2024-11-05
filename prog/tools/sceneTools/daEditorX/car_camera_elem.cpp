// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_camera_elem.h>
#include <EditorCore/ec_camera_dlg.h>

#include <debug/dag_debug3d.h>
#include <workCycle/dag_workCycle.h>
#include <shaders/dag_shaderMesh.h>

#include "services/carCamera/camera_controller.h"
#include "services/carCamera/car_obj.h"

const float ZOOM_SPEED = 0.1;
const float MIN_ZOOM = 1.0;
const float MAX_ZOOM = 5.0;
const Point3 CAMERA_POS = Point3(0, 4, -10);

//==============================================================================

extern void register_phys_car_gameres_factory();


CarCameraElem::CarCameraElem() : TpsCameraElem(CAR_CAMERA), vehicleViewer(NULL), camController(NULL), zoom(MIN_ZOOM)
{
  ::register_phys_car_gameres_factory();
  vehicleViewer = new VehicleViewer();
}

CarCameraElem::~CarCameraElem()
{
  del_it(camController);
  del_it(vehicleViewer);
}


bool CarCameraElem::begin()
{
  if (vehicleViewer && vehicleViewer->begin(vpw))
  {
    camController = new CameraController(vehicleViewer->getCarObj());
    if (camController)
      camController->setParamsPreset(0);

    return camController;
  }

  return false;
}


void CarCameraElem::end()
{
  if (vehicleViewer)
  {
    del_it(camController);
    vehicleViewer->end();
  }
}

//------------------------------------------------------------------

void CarCameraElem::actInternal()
{
  if (getCamera() != thisCamType || !vehicleViewer)
    return;

  if (vehicleViewer->isSimulation())
  {
    camController->update(::dagor_game_act_time);
    vehicleViewer->actObjects(::dagor_game_act_time);

    TMatrix tm = TMatrix::IDENT;
    camController->getITm(tm);
    vpw->setCameraTransform(tm);
  }
  else
    TpsCameraElem::actInternal();
}

void CarCameraElem::render()
{
  if (vehicleViewer && vehicleViewer->isSimulation())
    vehicleViewer->renderInfo();
}

void CarCameraElem::externalRender()
{
  if (vehicleViewer && vehicleViewer->isSimulation())
  {
    vehicleViewer->beforeRenderObjects();
    vehicleViewer->renderObjects();
  }
}


void CarCameraElem::handleMouseWheel(int dz)
{
  zoom += ZOOM_SPEED * dz;
  zoom = (zoom < MIN_ZOOM) ? MIN_ZOOM : (zoom > MAX_ZOOM ? MAX_ZOOM : zoom);
}


//==============================================================================
