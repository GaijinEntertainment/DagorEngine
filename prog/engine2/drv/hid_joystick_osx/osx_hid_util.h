//     File: HID_Utilities_External.h
// Abstract: External interface for HID Utilities, can be used with either library or source.
//  Version: 5.0
//
// Copyright (C) 2010 Apple Inc. All Rights Reserved.
//
// *****************************************************
#pragma once

#define Ptr OsxPtr
#include <IOKit/hid/IOHIDLib.h>
#undef Ptr

extern void HIDRebuildDevices(void);
extern Boolean HIDBuildMultiDeviceList();

extern long IOHIDDevice_GetVendorID(IOHIDDeviceRef inIOHIDDeviceRef);
extern long IOHIDDevice_GetProductID(IOHIDDeviceRef inIOHIDDeviceRef);
extern long IOHIDDevice_GetLocationID(IOHIDDeviceRef inIOHIDDeviceRef);

extern IOHIDManagerRef gIOHIDManagerRef;
extern CFMutableArrayRef gDeviceCFArrayRef;
extern CFArrayRef gElementCFArrayRef;
