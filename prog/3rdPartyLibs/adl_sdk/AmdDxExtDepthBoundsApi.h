//--------------------------------------------------------------------------------------
// @file  amddxextdepthboundsapi.h
// @brief
//    AMD D3D Depth Bounds Extension API include file.
//   This is the main include file for apps using the Depth Bounds extension.
//
// Copyright © AMD Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef _AMDDXEXTDEPTHBOUNDSAPI_H_
#define _AMDDXEXTDEPTHBOUNDSAPI_H_

#include "AmdDxExtApi.h"
#include "AmdDxExtIface.h"
#include "AmdDxExtDepthBounds.h"

// Depth Bounds extension ID passed to IAmdDxExt::GetExtInterface()
const unsigned int AmdDxExtDepthBoundsID = 11;

/**
***************************************************************************************************
* @brief Abstract Depth Bounds extension interface class
*
***************************************************************************************************
*/
class IAmdDxExtDepthBounds : public IAmdDxExtInterface
{
public:
    virtual HRESULT SetDepthBounds(BOOL enabled, FLOAT minDepth, FLOAT maxDepth) = 0;
};

#endif //_AMDDXEXTDEPTHBOUNDSAPI_H_
