from "%darg/ui_imports.nut" import *
from "dagor.math" import Color4

let cursors = require("samples_prog/_cursors.nut")


// --- Shader effect components ---

// Animated plasma effect - single .hlsl shared by both dev and production paths.
// shaderName loads pre-compiled DSHL; shaderSource overrides with runtime compilation on PC.
let plasma = {
  rendObj = ROBJ_SHADER
  size = sh(25)
  shaderName = "darg_test_plasma"
  shaderSource = "test_plasma_darg.hlsl"  // same file as DSHL #include
}

// Same shader with custom color params
let plasmaCustom = {
  rendObj = ROBJ_SHADER
  size = sh(25)
  shaderName = "darg_test_plasma"
  shaderSource = "test_plasma_darg.hlsl"
  shaderParams = {
    params0 = Color4(0.8, 0.2, 1.0, 1.0) // purple tint
  }
}

// Radial gradient effect
let gradient = {
  rendObj = ROBJ_SHADER
  size = sh(25)
  shaderSource = "gradient_darg.hlsl"
}

// Cursor-interactive shader (hover to see effect)
let cursorTest = {
  rendObj = ROBJ_SHADER
  size = sh(25)
  shaderSource = "mousetest_darg.hlsl"
}


// --- Layout ---

function mkCard(title, child) {
  return {
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    gap = sh(1)
    children = [
      child
      {
        rendObj = ROBJ_TEXT
        text = title
        color = Color(200, 200, 200)
      }
    ]
  }
}


return {
  rendObj = ROBJ_SOLID
  color = Color(30, 40, 50)
  cursor = cursors.normal
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = sh(3)
  children = [
    {
      rendObj = ROBJ_TEXT
      text = "ROBJ_SHADER - User-Authored Pixel Shaders"
      fontSize = hdpx(20)
      color = Color(255, 255, 255)
    }
    {
      flow = FLOW_VERTICAL
      gap = sh(3)
      halign = ALIGN_CENTER
      children = [
        {
          flow = FLOW_HORIZONTAL
          gap = sh(3)
          children = [
            mkCard("Plasma", plasma)
            mkCard("Plasma (custom params)", plasmaCustom)
          ]
        }
        {
          flow = FLOW_HORIZONTAL
          gap = sh(3)
          children = [
            mkCard("Radial gradient", gradient)
            mkCard("Cursor interactive", cursorTest)
          ]
        }
      ]
    }
    {
      rendObj = ROBJ_TEXTAREA
      behavior = Behaviors.TextArea
      maxWidth = sh(80)
      text = "Each effect is a single .hlsl file with float4 darg_ps(float2 uv).\nBuilt-in globals: darg_time, darg_opacity, darg_resolution, darg_cursor, darg_params0..3\nHover the 'Cursor interactive' panel to see the glow effect."
      color = Color(160, 160, 160)
    }
  ]
}
