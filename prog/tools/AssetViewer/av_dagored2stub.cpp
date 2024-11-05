// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <oldEditor/de_interface.h>

IDagorEd2Engine *IDagorEd2Engine::__dagored_global_instance = NULL;
bool(__stdcall *external_traceRay_for_rigen)(const Point3 &from, const Point3 &dir, float &current_t, Point3 *out_norm) = NULL;
