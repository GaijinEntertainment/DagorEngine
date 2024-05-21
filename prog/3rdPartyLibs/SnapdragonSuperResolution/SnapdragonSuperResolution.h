//============================================================================================================
//   DO NOT REMOVE THIS HEADER UNDER QUALCOMM PRODUCT KIT LICENSE AGREEMENT
//
//                  Copyright (c) 2023 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//                       Snapdragon™ Game Super Resolution
//                       Snapdragon™ GSR
//
//                       Developed by Snapdragon Studios™ (https://www.qualcomm.com/snapdragon-studios)
//               
//============================================================================================================
#pragma once
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderVar.h>
#include <generic/dag_carray.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_drv3d.h>

class SnapdragonSuperResolution : public PostFxRenderer
{
public:
    SnapdragonSuperResolution() :  PostFxRenderer("SnapdragonSuperResolution")
    {
        viewport[0] = 0.f;
        viewport[1] = 0.f;
        viewport[2] = 0.f;
        viewport[3] = 0.f;
    }

    ~SnapdragonSuperResolution() {}

    inline void SetViewport(int32_t x, int32_t y)
    {
        viewport[2] = (float)x;
        viewport[3] = (float)y;
        viewport[0] = viewport[2] != 0.f ? (1.f / viewport[2]) : 0.f;
        viewport[1] = viewport[3] != 0.f ? (1.f / viewport[3]) : 0.f;
    }

    inline void render(const ManagedTex &input, BaseTexture* output)
    {
        Color4 viewportData = Color4(viewport);

        d3d::set_render_target(output, 0);
        ShaderGlobal::set_texture(get_shader_variable_id("snapdragon_super_resolution_input"), input);
        ShaderGlobal::set_color4(get_shader_variable_id("snapdragon_super_resolution_ViewportInfo"), viewportData);
        PostFxRenderer::render();
    }

private:
    float viewport[4];
};
