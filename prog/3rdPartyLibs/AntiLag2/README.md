# Anti-Lag 2 SDK

## Overview
Reducing latency from mouse click to image-on-screen is critical in delivering a satisfying gaming experience, especially for fast-paced and competitive games. This latency occurs at various stages of the system between the user providing input via the mouse and keyboard to finally seeing the results of those actions as photons reaching the eye. This is known as end-to-end system latency.

While some parts of the system are broadly out of the game developer's control such as the input peripheral latency, or the display latency, the bit in the middle - the game itself - is somewhat more controllable.

Obviously, a higher framerate will have a pretty-much linear impact on the game's latency, but this will typically require reducing image quality and resolution. The key factor here is how much latency exists between the user input being polled and the point at which the content is sent to the display. This is where Anti-Lag comes in.

### Anti-Lag 1
Anti-Lag 1 is a pure driver-based technology that introduces a carefully calculated delay into the driver-side processing of game's graphics commands, such that CPU and GPU frames are optimally aligned. This way the CPU frames are prevented from running too far ahead of GPU frames, and as a result the lag (input-to-image latency) is reduced.

### Anti-Lag 2
Anti-Lag 2 does a similar thing to the driver-based AntiLag 1, but the point of insertion of the delay is now at the optimal point inside the game's logic, just before the user controls (mouse/gamepad/keyboard) are sampled.

This allows for an optimal alignment of the game's internal processing pipeline (and not just the in-driver producer-consumer logic), achieving a significantly greater latency reduction.

In order to achieve this improved latency reduction, the game developer needs to integrate the Anti-Lag 2 SDK into their game and (optionally) expose the its controls in the game UI.

## Integration Guide

# DirectX®11

* Include the DirectX®11 header in your game: ffx_antilag2_dx11.h
* Declare a persistent `AMD::AntiLag2DX11::Context` object initialized with `= {}` or mem-zeroed.
* Call `AMD::AntiLag2DX11::Initialize(&context)`. If this function returns `S_OK`, then Anti-Lag 2 is present in your game.
* Call `AMD::AntiLag2DX11::Update(&context,true,0)` at the point just before the game polls for input. Specify true to enable Anti-Lag 2. False, to disable it. The second parameter is an optional framerate limiter. Specify zero to disable it.
* Call `AMD::AntiLag2DX11::DeInitialize(&context)` to clean up the references to the SDK on game exit.

# DirectX®12

* Include the DirectX®12 header in your game: ffx_antilag2_dx12.h
* Declare a persistent `AMD::AntiLag2DX12::Context` object initialized with `= {}` or mem-zeroed.
* Call `AMD::AntiLag2DX12::Initialize(&context,pDevice)` passing the DX12 device into this function. If this function returns `S_OK`, then Anti-Lag 2 is present in your game.
* Call `AMD::AntiLag2DX12::Update(&context,true,0)` at the point just before the game polls for input. Specify true to enable Anti-Lag 2. False, to disable it. The second parameter is an optional framerate limiter. Specify zero to disable it.
* Call `AMD::AntiLag2DX12::DeInitialize(&context)` to clean up the references to the SDK on game exit.

## FSR 3 Frame Generation Support
Anti-Lag 2 requires some special attention when FSR 3 frame generation is enabled. There are a couple of extra Anti-Lag 2 functions required to be called to let Anti-Lag 2 know whether the presented frames are interpolated or not.

`AMD::AntiLag2DX12::MarkEndOfFrameRendering(&context)` should be called once the game's main rendering workload has been submitted, before the FSR 3 Present call is made.

`AMD::AntiLag2DX12::SetFrameGenFrameType(&context,bInterpolatedFrame)` also needs to be called, but FSR 3.1.1 onwards calls this for you. However, for this to work, you will need to poke some data into the DXGI swapchain's private data each frame using a specific struct format using a specific GUID.

An example of this would look something as follows:

```C++
// {5083ae5b-8070-4fca-8ee5-3582dd367d13}
static const GUID IID_IFfxAntiLag2Data = 
    {0x5083ae5b, 0x8070, 0x4fca, {0x8e, 0xe5, 0x35, 0x82, 0xdd, 0x36, 0x7d, 0x13}};

struct AntiLag2Data
{
    AMD::AntiLag2DX12::Context* context;
    bool                        enabled;
} data;

data.context = &m_AntiLag2Context;
data.enabled = m_AntiLag2Enabled;
pSwapChain->SetPrivateData( IID_IFfxAntiLag2Data, sizeof( data ), &data );
```

# Testing

Drivers supporting Anti-Lag 2 include a built-in Radeon Anti-Lag 2 Latency Monitor:
* Use the Alt+Shift+L hotkey to cycle through the modes which can display the FPS and latency onscreen.
* The three values displayed (from top to bottom) are FPS (yellow), latency [milliseconds] (white) and latency [gpu frames] (green). The latter is most useful to make sure Anti-Lag 2 was integrated correctly.
* Normally, the latency values will be white and green - but they can also be dark grey or light gray.
* Dark grey color means that the Update() function is not being called, which would mean that there is a problem with Anti-Lag 2 integration.
* Light grey color means that the values have not self-calibrated yet - that should resolve itself within a second or two. If it doesn't, that probably means there is a problem with the integration.
* When holding down the Right Ctrl key, Anti-Lag 2 is bypassed. In this case the green number (latency in frames) indicates the native (pre-Anti-Lag 2) game latency. Usually it would be close to an integer number 3, 4 or 5.
* When Anti-Lag 2 is active, the green number (latency in frames) should be between 1.0 and 2.0, or perhaps slightly higher than 2.0. If it is higher than 3 - then there could be a problem with integration.
* Both white and green numbers should roughly match the numbers measured by FLM (see below) when VSync is not used. In DirectX®11 though, if the game is not running in fullscreen exclusive mode - there might be a one frame discrepancy (FLM values will be one frame higher).

Another way to validate the SDK integration is to use the Frame Latency Meter (FLM) which can be downloaded, along with full source, here: https://github.com/GPUOpen-Tools/frame_latency_meter/releases. Full instructions on how to use this tool are included.

# Support

Hardware:
* RDNA™ 1-based products, including the AMD Radeon™ RX 5000 Series and newer.

Driver:
* Adrenalin Edition™ 24.6.1 and onwards for DirectX®11 support.
* Adrenalin Edition™ 24.7.1 and onwards for DirectX®12 support.

OS:
* Windows®10
* Windows®11

# Strings

Recommended UI text can be found here: https://gpuopen.com/fidelityfx-naming-guidelines/#antilag2

<h2>Open source</h2>

AMD Anti-Lag 2 SDK is open source, and available under the MIT license.

For more information on the license terms please refer to [license](LICENSE.txt).

<h2>Disclaimer</h2>

The information contained herein is for informational purposes only, and is subject to change without notice. While every
precaution has been taken in the preparation of this document, it may contain technical inaccuracies, omissions and typographical
errors, and AMD is under no obligation to update or otherwise correct this information. Advanced Micro Devices, Inc. makes no
representations or warranties with respect to the accuracy or completeness of the contents of this document, and assumes no
liability of any kind, including the implied warranties of noninfringement, merchantability or fitness for particular purposes, with
respect to the operation or use of AMD hardware, software or other products described herein. No license, including implied or
arising by estoppel, to any intellectual property rights is granted by this document. Terms and limitations applicable to the purchase
or use of AMD's products are as set forth in a signed agreement between the parties or in AMD's Standard Terms and Conditions
of Sale.

AMD, the AMD Arrow logo, Radeon, Ryzen, CrossFire, RDNA and combinations thereof are trademarks of Advanced Micro Devices, Inc.

Other product names used in this publication are for identification purposes only and may be trademarks of their respective companies.

DirectX is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

Vulkan and the Vulkan logo are registered trademarks of the Khronos Group Inc.

Microsoft is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

Windows is a registered trademark of Microsoft Corporation in the US and other jurisdictions.

© 2023-2024 Advanced Micro Devices, Inc. All rights reserved.
