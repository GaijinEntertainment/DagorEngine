// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_camera_elem.h>
#include <EditorCore/ec_camera_dlg.h>

CarCameraElem::CarCameraElem() : TpsCameraElem(CAR_CAMERA), vehicleViewer(NULL), camController(NULL) {}
CarCameraElem::~CarCameraElem() {}

bool CarCameraElem::begin() { return false; }
void CarCameraElem::end() {}
void CarCameraElem::actInternal() {}
void CarCameraElem::render() {}
void CarCameraElem::externalRender() {}
void CarCameraElem::handleMouseWheel(int dz) {}
