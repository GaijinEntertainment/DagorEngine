// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2025 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#include "../../api/include/ffx_api_types.h"

/*
Tuning varianceFactor and safetyMarginInMs Tips:
Calculation of frame pacing algorithm's next target timestamp: 
target frametime delta = average Frametime - (variance * varianceFactor) - safetyMarginInMs

Default Tuning uses safetyMarginInMs==0.1ms and varianceFactor==0.1.
Say Tuning set A uses safetyMarginInMs==0.75ms, and varianceFactor==0.1.
Say Tuning Set B uses safetyMarginInMs==0.01ms and varianceFactor==0.3.

Example #1 - Actual Game Cutscene. Game's framerate after FG ON during camera pan from normal (19ms avg frametime) to complex (37ms avg frametime over 1 sec) and back to normal scene complexity (19ms avg frametime).
After the panning is done, 
- Default Tuning now gets stuck at targeting ~33ms after panning to a complex scene. GPU utilization significantly lower in this case.
- Tuning Set B and Tuning Set A are able to recover close to ~19ms because of these 2 tuning result in lower "target frametime delta" than Default Tuning. 

However, larger varianceFactor or safetyMarginInMs results in higher variance. As seen in Example #2 bellow.

Example #2 - FFX_API_FSR sample. Set app fps cap to 33.33ms. Use OCAT to capture 10s at default camera position.
FSR 3.1.0 FG msbetweenpresents ping-pong between 16.552 (5th-percentile) and 16.832 (95th-percentile). Variance is 0.01116.
Tuning set A FG msbetweenpresents ping-pong between 15.901 (5th-percentile) and 17.500 (95th-percentile). Variance is 0.057674.
Tuning Set B FG msbetweenpresents ping-pong between 16.589 (5th-percentile) and 16.971 (95th-percentile). Variance is 0.014452.

| FG output Frames timestamp | n | n+1    | n+2   | n+3    | frame delta n+1 to n+2 | frame delta n+2 to n+3 |
| -------------------------- | - | ------ | ----- | ------ | ---------------------- | ---------------------- |
| App real frame presents    | 0 |        | 33.33 |        |                        |                        |
| Default Tuning             | 0 | 16.552 | 33.33 | 49.882 | 16.778                 | 16.552                 |
| Tuning Set A               | 0 | 15.901 | 33.33 | 49.231 | 17.429                 | 15.901                 |
| Tuning Set B               | 0 | 16.589 | 33.33 | 49.919 | 16.741                 | 16.589                 |

Analysis of table data in words:
Ignoring the cost of FI,
"Tuning set A"'s "target frametime delta" of 15.901 results in larger frame to frame delta (or in other words larger variance) vs Default tuning.
"Tuning set B"'s "target frametime delta" of 16.589 results in a bit larger frame to frame delta (or in other words a bit larger variance) vs Default tuning.

TLDR:
If your game when using FG, frame rate is running at unexpectly low frame rate, after gradual transition from rendering complex to easy scene complexity, you could try setting "Tuning Set B" to recover lost FPS at cost of a bit higher variance.

*/

//struct definition matches FfxSwapchainFramePacingTuning
typedef struct FfxApiSwapchainFramePacingTuning
{
    float safetyMarginInMs; // in Millisecond. Default is 0.1ms
    float varianceFactor; // valid range [0.0,1.0]. Default is 0.1
    bool     allowHybridSpin; //Allows pacing spinlock to sleep. Default is false.
    uint32_t hybridSpinTime;  //How long to spin if allowHybridSpin is true. Measured in timer resolution units. Not recommended to go below 2. Will result in frequent overshoots. Default is 2.
    bool     allowWaitForSingleObjectOnFence; //Allows WaitForSingleObject instead of spinning for fence value. Default is false.
} FfxApiSwapchainFramePacingTuning;
