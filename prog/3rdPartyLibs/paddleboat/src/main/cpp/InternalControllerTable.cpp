/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "InternalControllerTable.h"

#include <android/input.h>
#include <android/keycodes.h>

#define ARRAY_COUNTOF(array) (sizeof(array) / sizeof(array[0]))

#define PADDLEBOAT_AXIS_BUTTON_DPAD_UP 0
#define PADDLEBOAT_AXIS_BUTTON_DPAD_LEFT 1
#define PADDLEBOAT_AXIS_BUTTON_DPAD_DOWN 2
#define PADDLEBOAT_AXIS_BUTTON_DPAD_RIGHT 3
#define PADDLEBOAT_AXIS_BUTTON_L2 9
#define PADDLEBOAT_AXIS_BUTTON_R2 12

namespace paddleboat {

// New, interim
const Paddleboat_Controller_Mapping_File_Axis_Entry pb_internal_axis_table[] = {
            { // 00
                 1, // "Generic_Axis"
                 {
                         /* LX */ AMOTION_EVENT_AXIS_X,
                         /* LY */ AMOTION_EVENT_AXIS_Y,
                         /* RX */ AMOTION_EVENT_AXIS_Z,
                         /* RY */ AMOTION_EVENT_AXIS_RZ,
                         /* L1 */ PADDLEBOAT_AXIS_IGNORED,
                         /* L2 */ AMOTION_EVENT_AXIS_BRAKE,
                         /* R1 */ PADDLEBOAT_AXIS_IGNORED,
                         /* R2 */ AMOTION_EVENT_AXIS_GAS,
                         /* HX */ AMOTION_EVENT_AXIS_HAT_X,
                         /* HY */ AMOTION_EVENT_AXIS_HAT_Y,
                 },
                 {
                         /* LX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* LY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L2 */ PADDLEBOAT_AXIS_BUTTON_L2,
                         /* R1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R2 */ PADDLEBOAT_AXIS_BUTTON_R2,
                         /* HX */ PADDLEBOAT_AXIS_BUTTON_DPAD_RIGHT,
                         /* HY */ PADDLEBOAT_AXIS_BUTTON_DPAD_DOWN,
                 },
                 {
                         /* LX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* LY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L2 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R2 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* HX */ PADDLEBOAT_AXIS_BUTTON_DPAD_LEFT,
                         /* HY */ PADDLEBOAT_AXIS_BUTTON_DPAD_UP,
                 },
                 0
            },
            { // 01
                 8, // "DS5_Compat_Axis"
                 {
                        /* LX */ AMOTION_EVENT_AXIS_X,
                        /* LY */ AMOTION_EVENT_AXIS_Y,
                        /* RX */ AMOTION_EVENT_AXIS_Z,
                        /* RY */ AMOTION_EVENT_AXIS_RZ,
                        /* L1 */ PADDLEBOAT_AXIS_IGNORED,
                        /* L2 */ AMOTION_EVENT_AXIS_RX,
                        /* R1 */ PADDLEBOAT_AXIS_IGNORED,
                        /* R2 */ AMOTION_EVENT_AXIS_RY,
                        /* HX */ AMOTION_EVENT_AXIS_HAT_X,
                        /* HY */ AMOTION_EVENT_AXIS_HAT_Y,
                 },
                 {
                         /* LX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* LY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L2 */ PADDLEBOAT_AXIS_BUTTON_L2,
                         /* R1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R2 */ PADDLEBOAT_AXIS_BUTTON_R2,
                         /* HX */ PADDLEBOAT_AXIS_BUTTON_DPAD_RIGHT,
                         /* HY */ PADDLEBOAT_AXIS_BUTTON_DPAD_DOWN,
                 },
                 {
                         /* LX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* LY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L2 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R2 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* HX */ PADDLEBOAT_AXIS_BUTTON_DPAD_LEFT,
                         /* HY */ PADDLEBOAT_AXIS_BUTTON_DPAD_UP,
                 },
                 0
            },
            { // 02
                 9, // "Nimbus_Axis"
                 {
                         /* LX */ AMOTION_EVENT_AXIS_X,
                         /* LY */ AMOTION_EVENT_AXIS_Y,
                         /* RX */ AMOTION_EVENT_AXIS_Z,
                         /* RY */ AMOTION_EVENT_AXIS_RZ,
                         /* L1 */ PADDLEBOAT_AXIS_IGNORED,
                         /* L2 */ PADDLEBOAT_AXIS_IGNORED,
                         /* R1 */ PADDLEBOAT_AXIS_IGNORED,
                         /* R2 */ PADDLEBOAT_AXIS_IGNORED,
                         /* HX */ PADDLEBOAT_AXIS_IGNORED,
                         /* HY */ PADDLEBOAT_AXIS_IGNORED,
                 },
                 {
                         /* LX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* LY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L2 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R2 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* HX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* HY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                 },
                 {
                         /* LX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* LY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* RY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* L2 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R1 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* R2 */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* HX */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                         /* HY */ PADDLEBOAT_AXIS_BUTTON_IGNORED,
                 },
                 10
            }
};

const Paddleboat_Controller_Mapping_File_Button_Entry pb_internal_button_table[] = {
        { // 00
             2,     // "XB_Button"
             {      /* UP     */ AKEYCODE_DPAD_UP,
                    /* LEFT   */ AKEYCODE_DPAD_LEFT,
                    /* DOWN   */ AKEYCODE_DPAD_DOWN,
                    /* RIGHT  */ AKEYCODE_DPAD_RIGHT,
                    /* A      */ AKEYCODE_BUTTON_A,
                    /* B      */ AKEYCODE_BUTTON_B,
                    /* X      */ AKEYCODE_BUTTON_X,
                    /* Y      */ AKEYCODE_BUTTON_Y,
                    /* L1     */ AKEYCODE_BUTTON_L1,
                    /* L2     */ AKEYCODE_BUTTON_L2,
                    /* L3     */ AKEYCODE_BUTTON_THUMBL,
                    /* R1     */ AKEYCODE_BUTTON_R1,
                    /* R2     */ AKEYCODE_BUTTON_R2,
                    /* R3     */ AKEYCODE_BUTTON_THUMBR,
                    /* SELECT */ AKEYCODE_BUTTON_SELECT,
                    /* START  */ AKEYCODE_BUTTON_START,
                    /* SYSTEM */ AKEYCODE_BUTTON_MODE,
                    /* TOUCHP */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX1   */ AKEYCODE_MEDIA_RECORD,
                    /* AUX2   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX3   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX4   */ PADDLEBOAT_BUTTON_IGNORED
             }
        },
        { // 01
            3,      // "DS_Button"
            {
                    /* UP     */ AKEYCODE_DPAD_UP,
                    /* LEFT   */ AKEYCODE_DPAD_LEFT,
                    /* DOWN   */ AKEYCODE_DPAD_DOWN,
                    /* RIGHT  */ AKEYCODE_DPAD_RIGHT,
                    /* A      */ AKEYCODE_BUTTON_A,
                    /* B      */ AKEYCODE_BUTTON_B,
                    /* X      */ AKEYCODE_BUTTON_X,
                    /* Y      */ AKEYCODE_BUTTON_Y,
                    /* L1     */ AKEYCODE_BUTTON_L1,
                    /* L2     */ AKEYCODE_BUTTON_L2,
                    /* L3     */ AKEYCODE_BUTTON_THUMBL,
                    /* R1     */ AKEYCODE_BUTTON_R1,
                    /* R2     */ AKEYCODE_BUTTON_R2,
                    /* R3     */ AKEYCODE_BUTTON_THUMBR,
                    /* SELECT */ AKEYCODE_BUTTON_SELECT,
                    /* START  */ AKEYCODE_BUTTON_START,
                    /* SYSTEM */ AKEYCODE_BUTTON_MODE,
                    /* TOUCHP */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX1   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX2   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX3   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX4   */ PADDLEBOAT_BUTTON_IGNORED
            }
        },
        { // 02
            4,      // "DS5_Compat_Button"
            {
                    /* UP     */ AKEYCODE_DPAD_UP,
                    /* LEFT   */ AKEYCODE_DPAD_LEFT,
                    /* DOWN   */ AKEYCODE_DPAD_DOWN,
                    /* RIGHT  */ AKEYCODE_DPAD_RIGHT,
                    /* A      */ AKEYCODE_BUTTON_B,
                    /* B      */ AKEYCODE_BUTTON_C,
                    /* X      */ AKEYCODE_BUTTON_A,
                    /* Y      */ AKEYCODE_BUTTON_X,
                    /* L1     */ AKEYCODE_BUTTON_Y,
                    /* L2     */ PADDLEBOAT_BUTTON_IGNORED,
                    /* L3     */ AKEYCODE_BUTTON_SELECT,
                    /* R1     */ AKEYCODE_BUTTON_Z,
                    /* R2     */ PADDLEBOAT_BUTTON_IGNORED,
                    /* R3     */ AKEYCODE_BUTTON_START,
                    /* SELECT */ AKEYCODE_BUTTON_L2,
                    /* START  */ AKEYCODE_BUTTON_R2,
                    /* SYSTEM */ AKEYCODE_BUTTON_MODE,
                    /* TOUCHP */ AKEYCODE_BUTTON_THUMBL,
                    /* AUX1   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX2   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX3   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX4   */ PADDLEBOAT_BUTTON_IGNORED
            }
        },
        { // 03
            5,      // "NinSPro"
            {
                    /* UP     */ AKEYCODE_DPAD_UP,
                    /* LEFT   */ AKEYCODE_DPAD_LEFT,
                    /* DOWN   */ AKEYCODE_DPAD_DOWN,
                    /* RIGHT  */ AKEYCODE_DPAD_RIGHT,
                    /* A      */ AKEYCODE_BUTTON_B,
                    /* B      */ AKEYCODE_BUTTON_A,
                    /* X      */ AKEYCODE_BUTTON_Y,
                    /* Y      */ AKEYCODE_BUTTON_X,
                    /* L1     */ AKEYCODE_BUTTON_L1,
                    /* L2     */ AKEYCODE_BUTTON_L2,
                    /* L3     */ AKEYCODE_BUTTON_THUMBL,
                    /* R1     */ AKEYCODE_BUTTON_R1,
                    /* R2     */ AKEYCODE_BUTTON_R2,
                    /* R3     */ AKEYCODE_BUTTON_THUMBR,
                    /* SELECT */ AKEYCODE_BUTTON_SELECT,
                    /* START  */ AKEYCODE_BUTTON_START,
                    /* SYSTEM */ AKEYCODE_BUTTON_MODE,
                    /* TOUCHP */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX1   */ AKEYCODE_MEDIA_RECORD,
                    /* AUX2   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX3   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX4   */ PADDLEBOAT_BUTTON_IGNORED
            }
        },
        { // 04
            6,      // "Generic_Button"
            {
                    /* UP     */ AKEYCODE_DPAD_UP,
                    /* LEFT   */ AKEYCODE_DPAD_LEFT,
                    /* DOWN   */ AKEYCODE_DPAD_DOWN,
                    /* RIGHT  */ AKEYCODE_DPAD_RIGHT,
                    /* A      */ AKEYCODE_BUTTON_A,
                    /* B      */ AKEYCODE_BUTTON_B,
                    /* X      */ AKEYCODE_BUTTON_X,
                    /* Y      */ AKEYCODE_BUTTON_Y,
                    /* L1     */ AKEYCODE_BUTTON_L1,
                    /* L2     */ AKEYCODE_BUTTON_L2,
                    /* L3     */ AKEYCODE_BUTTON_THUMBL,
                    /* R1     */ AKEYCODE_BUTTON_R1,
                    /* R2     */ AKEYCODE_BUTTON_R2,
                    /* R3     */ AKEYCODE_BUTTON_THUMBR,
                    /* SELECT */ AKEYCODE_BUTTON_SELECT,
                    /* START  */ AKEYCODE_BUTTON_START,
                    /* SYSTEM */ AKEYCODE_BUTTON_MODE,
                    /* TOUCHP */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX1   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX2   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX3   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX4   */ PADDLEBOAT_BUTTON_IGNORED
            }
        },
        { // 05
            6,      // "NGPro_PC2"
            {
                    /* UP     */ AKEYCODE_DPAD_UP,
                    /* LEFT   */ AKEYCODE_DPAD_LEFT,
                    /* DOWN   */ AKEYCODE_DPAD_DOWN,
                    /* RIGHT  */ AKEYCODE_DPAD_RIGHT,
                    /* A      */ AKEYCODE_BUTTON_A,
                    /* B      */ AKEYCODE_BUTTON_B,
                    /* X      */ AKEYCODE_BUTTON_X,
                    /* Y      */ AKEYCODE_BUTTON_Y,
                    /* L1     */ AKEYCODE_BUTTON_L1,
                    /* L2     */ AKEYCODE_BUTTON_R1,
                    /* L3     */ PADDLEBOAT_BUTTON_IGNORED,
                    /* R1     */ AKEYCODE_BUTTON_Z,
                    /* R2     */ AKEYCODE_BUTTON_C,
                    /* R3     */ PADDLEBOAT_BUTTON_IGNORED,
                    /* SELECT */ AKEYCODE_BUTTON_SELECT,
                    /* START  */ AKEYCODE_BUTTON_START,
                    /* SYSTEM */ PADDLEBOAT_BUTTON_IGNORED,
                    /* TOUCHP */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX1   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX2   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX3   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX4   */ PADDLEBOAT_BUTTON_IGNORED
            }
        },
        { // 06
            10,      // "Nimbus_Button"
            {
                    /* UP     */ PADDLEBOAT_BUTTON_IGNORED,
                    /* LEFT   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* DOWN   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* RIGHT  */ PADDLEBOAT_BUTTON_IGNORED,
                    /* A      */ AKEYCODE_BUTTON_A,
                    /* B      */ AKEYCODE_BUTTON_B,
                    /* X      */ AKEYCODE_BUTTON_C,
                    /* Y      */ AKEYCODE_BUTTON_X,
                    /* L1     */ AKEYCODE_BUTTON_Y,
                    /* L2     */ AKEYCODE_BUTTON_L1,
                    /* L3     */ PADDLEBOAT_BUTTON_IGNORED,
                    /* R1     */ AKEYCODE_BUTTON_Z,
                    /* R2     */ AKEYCODE_BUTTON_R1,
                    /* R3     */ PADDLEBOAT_BUTTON_IGNORED,
                    /* SELECT */ PADDLEBOAT_BUTTON_IGNORED,
                    /* START  */ PADDLEBOAT_BUTTON_IGNORED,
                    /* SYSTEM */ PADDLEBOAT_BUTTON_IGNORED,
                    /* TOUCHP */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX1   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX2   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX3   */ PADDLEBOAT_BUTTON_IGNORED,
                    /* AUX4   */ PADDLEBOAT_BUTTON_IGNORED
            }
        }
};

const Paddleboat_Controller_Mapping_File_Controller_Entry pb_internal_controller_table[] = {
        {
                // Steelseries Nimbus (bluetooth)
                16,00,0x0111,0x1420,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                2,6,0,0
        },
        {
                // Microsoft Xbox 360 controller (usb)
                16,00,0x045e,0x028e,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,0,0,0
        },
        {
                // Microsoft Xbox One non-BT controller (usb)
                16,00,0x045e,0x02d1,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,0,0,0
        },
        {
                // 8BitDo Arcade Stick, XInput mode, Bluetooth
                16,00,0x045e,0x02e0,
                PADDLEBOAT_CONTROLLER_LAYOUT_ARCADE_STICK,
                0,0,0,0
        },
        {
                // Microsoft Xbox One BT controller (usb)
                16,00,0x045e,0x02ea,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,0,0,0
        },
        {
                // Microsoft Xbox One BT controller (bluetooth)
                16,00,0x045e,0x02fd,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,0,0,0
        },
        {
                // Microsoft Xbox Series controller (usb)
                16,00,0x045e,0x0b12,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,0,0,0
        },
        {
                //  Microsoft Xbox Series controller (bluetooth)
                16,00,0x045e,0x0b13,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,0,0,0
        },
        {
                //  Sony PlayStation 4 controller (usb/bluetooth)
                16,00,0x054C,0x05C4,
                PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES | PADDLEBOAT_CONTROLLER_FLAG_TOUCHPAD,
                0,1,0,0
        },
        {
                //  Sony PlayStation 4 controller (usb/bluetooth) - alternate deviceId
                16,00,0x054C,0x09CC,
                PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES | PADDLEBOAT_CONTROLLER_FLAG_TOUCHPAD,
                0,1,0,0
        },
        {
                //  Sony PlayStation 5 controller (usb/bluetooth) API <= 30
                16,30,0x054C,0x0CE6,
                PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES,
                1,2,0,0
        },
        {
                //  Sony PlayStation 5 controller (usb/bluetooth) API >= 31
                31,00,0x054C,0x0CE6,
                PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES | PADDLEBOAT_CONTROLLER_FLAG_TOUCHPAD,
                0,1,0,0
        },
        {
                //  Nintendo Switch Pro controller (usb/bluetooth)
                16,30,0x057e,0x2009,
                PADDLEBOAT_CONTROLLER_LAYOUT_REVERSE,
                0,3,0,0
        },
        {
                //  Nvidia Shield TV controller (usb)
                16,00,0x0955,0x7210,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,4,0,0
        },
        {
                //  Google Stadia controller (usb)
                16,00,0x18d1,0x9400,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,4,0,0
        },
        {
                //  NeoGeo Pro, PC 2 setting, USB
                16,00,0x20bc,0x5501,
                PADDLEBOAT_CONTROLLER_LAYOUT_ARCADE_STICK,
                0,5,0,0
        },
        {
                // Razer Kishi v1 (usb)
                16,00,0x27f8,0x0bbf,
                PADDLEBOAT_CONTROLLER_LAYOUT_STANDARD,
                0,0,0,0
        }
};

const Paddleboat_Controller_Mapping_File_String_Entry pb_internal_string_table[] = {
        {"None"},               // 00
        {"Generic_Axis"},       // 01
        {"XB_Button"},          // 02
        {"DS_Button"},          // 03
        {"DS5_Compat_Button"},  // 04
        {"NinSPro_Button"},     // 05
        {"Generic_Button"},     // 06
        {"NGPro_PC2_Button"},   // 07
        {"DS5_Compat_Axis"},    // 08
        {"Nimbus_Axis"},        // 09
        {"Nimbus_Button"}       // 10
};

const Paddleboat_Internal_Mapping_Header pb_internal_header = {
     ARRAY_COUNTOF(pb_internal_axis_table),
     ARRAY_COUNTOF(pb_internal_button_table),
     ARRAY_COUNTOF(pb_internal_controller_table),
     ARRAY_COUNTOF(pb_internal_string_table),
     pb_internal_axis_table,
     pb_internal_button_table,
     pb_internal_controller_table,
     pb_internal_string_table
};

const Paddleboat_Internal_Mapping_Header *GetInternalMappingHeader() {
     return &pb_internal_header;
}
}  // namespace paddleboat