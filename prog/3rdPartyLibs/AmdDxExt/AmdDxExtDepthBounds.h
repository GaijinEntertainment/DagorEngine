//--------------------------------------------------------------------------------------
// @file  amddxextdepthbounds.h
// @brief AMD D3D Depth Bounds Extension API include file.
//
// Copyright © AMD Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef _AMDDXEXTDEPTHBOUNDS_H_
#define _AMDDXEXTDEPTHBOUNDS_H_

// This include file contains all the Depth Bounds extension definitions
// (structures, enums, constants) shared between the driver and the application

/**
***************************************************************************************************
* @brief
*    AmdDxDepthBounds struct - Depth Bounds input struct. Contains the app-requested depth
*    bounds max and min depth values to be used for the depth bounds test as well as the enable.
*
***************************************************************************************************
*/
struct AmdDxDepthBounds
{
    BOOL enabled;
    FLOAT minDepth;
    FLOAT maxDepth;
};

#endif //_AMDDXEXTDEPTHBOUNDS_H_
