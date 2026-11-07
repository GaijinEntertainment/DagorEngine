// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

// Devices with joystick axes in evdev that are not game controllers (keyboards, mice, LED controllers).
// From https://github.com/denilsonsa/udev-joystick-blacklist and SDL initial_blacklist_devices.
namespace HumanInput
{
struct JoyVidPid
{
  uint16_t vid, pid;
};

static const JoyVidPid ignored_joystick_ids[] = {
  // Microsoft Microsoft Wireless Optical Desktop 2.10
  // Microsoft Wireless Desktop - Comfort Edition
  {0x045e, 0x009d},

  // Microsoft Microsoft Digital Media Pro Keyboard
  // Microsoft Corp. Digital Media Pro Keyboard
  {0x045e, 0x00b0},

  // Microsoft Microsoft Digital Media Keyboard
  // Microsoft Corp. Digital Media Keyboard 1.0A
  {0x045e, 0x00b4},

  // Microsoft Microsoft Digital Media Keyboard 3000
  {0x045e, 0x0730},

  // Microsoft Microsoft 2.4GHz Transceiver v6.0
  // Microsoft Microsoft 2.4GHz Transceiver v8.0
  // Microsoft Corp. Nano Transceiver v1.0 for Bluetooth
  // Microsoft Wireless Mobile Mouse 1000
  // Microsoft Wireless Desktop 3000
  {0x045e, 0x0745},

  // Microsoft SideWinder(TM) 2.4GHz Transceiver
  {0x045e, 0x0748},

  // Microsoft Corp. Wired Keyboard 600
  {0x045e, 0x0750},

  // Microsoft Corp. Sidewinder X4 keyboard
  {0x045e, 0x0768},

  // Microsoft Corp. Arc Touch Mouse Transceiver
  {0x045e, 0x0773},

  // Microsoft 2.4GHz Transceiver v9.0
  // Microsoft Nano Transceiver v2.1
  // Microsoft Sculpt Ergonomic Keyboard (5KV-00001)
  {0x045e, 0x07a5},

  // Microsoft Nano Transceiver v1.0
  // Microsoft Wireless Keyboard 800
  {0x045e, 0x07b2},

  // Microsoft Nano Transceiver v2.0
  {0x045e, 0x0800},

  {0x046d, 0xc30a}, // Logitech, Inc. iTouch Composite keyboard

  {0x04d9, 0xa0df}, // Tek Syndicate Mouse (E-Signal USB Gaming Mouse)

  // List of Wacom devices at: http://linuxwacom.sourceforge.net/wiki/index.php/Device_IDs
  {0x056a, 0x0010}, // Wacom ET-0405 Graphire
  {0x056a, 0x0011}, // Wacom ET-0405A Graphire2 (4x5)
  {0x056a, 0x0012}, // Wacom ET-0507A Graphire2 (5x7)
  {0x056a, 0x0013}, // Wacom CTE-430 Graphire3 (4x5)
  {0x056a, 0x0014}, // Wacom CTE-630 Graphire3 (6x8)
  {0x056a, 0x0015}, // Wacom CTE-440 Graphire4 (4x5)
  {0x056a, 0x0016}, // Wacom CTE-640 Graphire4 (6x8)
  {0x056a, 0x0017}, // Wacom CTE-450 Bamboo Fun (4x5)
  {0x056a, 0x0018}, // Wacom CTE-650 Bamboo Fun 6x8
  {0x056a, 0x0019}, // Wacom CTE-631 Bamboo One
  {0x056a, 0x00d1}, // Wacom Bamboo Pen and Touch CTH-460
  {0x056a, 0x030e}, // Wacom Intuos Pen (S) CTL-480

  {0x09da, 0x054f}, // A4 Tech Co., G7 750 mouse
  {0x09da, 0x1410}, // A4 Tech Co., Ltd Bloody AL9 mouse
  {0x09da, 0x3043}, // A4 Tech Co., Ltd Bloody R8A Gaming Mouse
  {0x09da, 0x31b5}, // A4 Tech Co., Ltd Bloody TL80 Terminator Laser Gaming Mouse
  {0x09da, 0x3997}, // A4 Tech Co., Ltd Bloody RT7 Terminator Wireless
  {0x09da, 0x3f8b}, // A4 Tech Co., Ltd Bloody V8 mouse
  {0x09da, 0x51f4}, // Modecom MC-5006 Keyboard
  {0x09da, 0x5589}, // A4 Tech Co., Ltd Terminator TL9 Laser Gaming Mouse
  {0x09da, 0x7b22}, // A4 Tech Co., Ltd Bloody V5
  {0x09da, 0x7f2d}, // A4 Tech Co., Ltd Bloody R3 mouse
  {0x09da, 0x8090}, // A4 Tech Co., Ltd X-718BK Oscar Optical Gaming Mouse
  {0x09da, 0x9033}, // A4 Tech Co., X7 X-705K
  {0x09da, 0x9066}, // A4 Tech Co., Sharkoon Fireglider Optical
  {0x09da, 0x9090}, // A4 Tech Co., Ltd XL-730K / XL-750BK / XL-755BK Laser Mouse
  {0x09da, 0x90c0}, // A4 Tech Co., Ltd X7 G800V keyboard
  {0x09da, 0xf012}, // A4 Tech Co., Ltd Bloody V7 mouse
  {0x09da, 0xf32a}, // A4 Tech Co., Ltd Bloody B540 keyboard
  {0x09da, 0xf613}, // A4 Tech Co., Ltd Bloody V2 mouse
  {0x09da, 0xf624}, // A4 Tech Co., Ltd Bloody B120 Keyboard

  {0x1b1c, 0x1b3c}, // Corsair Harpoon RGB gaming mouse

  {0x1d57, 0xad03}, // [T3] 2.4GHz and IR Air Mouse Remote Control

  {0x1e7d, 0x2e4a}, // Roccat Tyon Mouse

  {0x20a0, 0x422d}, // Winkeyless.kr Keyboards

  {0x2516, 0x001f}, // Cooler Master Storm Mizar Mouse
  {0x2516, 0x0028}, // Cooler Master Storm Alcor Mouse

  // SDL additions
  {0x04d9, 0x8008}, // OBINLB USB-HID Keyboard (Anne Pro II)
  {0x04d9, 0x8009}, // OBINLB USB-HID Keyboard (Anne Pro II)
  {0x04d9, 0xa292}, // OBINLB USB-HID Keyboard (Anne Pro II)
  {0x04d9, 0xa293}, // OBINLB USB-HID Keyboard (Anne Pro II)
  {0x04f2, 0xa13c}, // HP Deluxe Webcam KQ246AA
  {0x0e6f, 0x018a}, // PDP REALMz Wireless Controller for Switch, USB charging
  {0x1532, 0x0266}, // Razer Huntsman V2 Analog, non-functional DInput device
  {0x1532, 0x0282}, // Razer Huntsman Mini Analog, non-functional DInput device
  {0x20d6, 0x0002}, // PowerA Enhanced Wireless Controller for Nintendo Switch (charging port only)
  {0x256c, 0x006d}, // Huion Tablet_GS1331, Huion Tablet_GS1331 Touch Strip
  {0x256c, 0x006e}, // Huion Tablet Kamvas Pro 22, Huion Tablet Kamvas Pro 22 Touch Strip
  {0x26ce, 0x01a2}, // ASRock LED Controller
  {0x3297, 0x1969}, // Moonlander MK1 Keyboard
  {0x3434, 0x0121}, // Keychron Q3 System Control
  {0x3434, 0x0163}, // Keychron Q6 System Control
  {0x3434, 0x0211}, // Keychron K1 Pro System Control
  {0x3434, 0x02a0}, // Keychron K10 Pro System Control
  {0x3434, 0x0353}, // Keychron V5 System Control
  {0x3434, 0xd030}, // Keychron Link
};
} // namespace HumanInput
