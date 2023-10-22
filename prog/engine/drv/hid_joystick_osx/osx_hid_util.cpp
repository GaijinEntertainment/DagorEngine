//     File: HID_Config_Utilities.c
//     File: IOHIDDevice_.c
// Abstract: Implementation of the HID configuration utilities
//  Version: 5.0
//
// Copyright (C) 2010 Apple Inc. All Rights Reserved.
//
// *****************************************************
#include "osx_hid_util.h"
#include <debug/dag_debug.h>

static void CFSetApplierFunctionCopyToCFArray(const void *value, void *context);
static CFComparisonResult CFDeviceArrayComparatorFunction(const void *val1, const void *val2, void *context);

// ***************************************************
#pragma mark - exported globals
// -----------------------------------------------------

IOHIDManagerRef gIOHIDManagerRef = NULL;
CFMutableArrayRef gDeviceCFArrayRef = NULL;
CFArrayRef gElementCFArrayRef = NULL;

// *************************************************************************
//
// HIDBuildMultiDeviceList(inUsagePages, inUsages, inNumDeviceTypes)
//
// Purpose: builds list of devices with elements
//
Boolean HIDBuildMultiDeviceList()
{
  Boolean result = FALSE;              // assume failure (pessimist!)
  Boolean first = (!gIOHIDManagerRef); // not yet created?

  if (first)
  {
    // create the manager
    gIOHIDManagerRef = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
  }
  if (gIOHIDManagerRef)
  {
    CFMutableArrayRef hidMatchingCFMutableArrayRef = NULL;

    // set it for IOHIDManager to use to match against
    IOHIDManagerSetDeviceMatchingMultiple(gIOHIDManagerRef, hidMatchingCFMutableArrayRef);
    if (hidMatchingCFMutableArrayRef)
    {
      CFRelease(hidMatchingCFMutableArrayRef);
    }
    if (first)
    {
      // open it
      IOReturn tIOReturn = IOHIDManagerOpen(gIOHIDManagerRef, kIOHIDOptionsTypeNone);
      if (kIOReturnSuccess != tIOReturn)
      {
        logerr("%s: Couldn't open IOHIDManager.", __PRETTY_FUNCTION__);
        goto Oops;
      }
    }

    HIDRebuildDevices();
    result = TRUE;
  }
  else
  {
    logerr("%s: Couldn't create a IOHIDManager.", __PRETTY_FUNCTION__);
  }

Oops:;
  return (result);
}

// *************************************************************************
//
// HIDRebuildDevices()
//
// Purpose: rebuilds the (internal) list of IOHIDDevices
//
void HIDRebuildDevices(void)
{
  // get the set of devices from the IOHID manager
  CFSetRef devCFSetRef = IOHIDManagerCopyDevices(gIOHIDManagerRef);

  if (devCFSetRef)
  {
    // if the existing array isn't empty...
    if (gDeviceCFArrayRef)
    {
      // release it
      CFRelease(gDeviceCFArrayRef);
    }

    // create an empty array
    gDeviceCFArrayRef = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    // now copy the set to the array
    CFSetApplyFunction(devCFSetRef, CFSetApplierFunctionCopyToCFArray, (void *)gDeviceCFArrayRef);
    // now sort the array by location ID's
    CFIndex cnt = CFArrayGetCount(gDeviceCFArrayRef);
    CFArraySortValues(gDeviceCFArrayRef, CFRangeMake(0, cnt), CFDeviceArrayComparatorFunction, NULL);

    // and release the set we copied from the IOHID manager
    CFRelease(devCFSetRef);
  }
}


// *************************************************************************
//
// CFSetApplierFunctionCopyToCFArray(value, context)
//
// Purpose: CFSetApplierFunction to copy the CFSet to a CFArray
//
// Notes: called one time for each item in the CFSet
//
static void CFSetApplierFunctionCopyToCFArray(const void *value, void *context)
{
  CFArrayAppendValue((CFMutableArrayRef)context, value);
}

// ---------------------------------
// used to sort the CFDevice array after copying it from the (unordered) (CF)set.
// we compare based on the location ID's since they're consistant (across boots & launches).
//
static CFComparisonResult CFDeviceArrayComparatorFunction(const void *val1, const void *val2, void *)
{
  CFComparisonResult result = kCFCompareEqualTo;

  long loc1 = IOHIDDevice_GetLocationID((IOHIDDeviceRef)val1);
  long loc2 = IOHIDDevice_GetLocationID((IOHIDDeviceRef)val2);
  if (loc1 < loc2)
  {
    result = kCFCompareLessThan;
  }
  else if (loc1 > loc2)
  {
    result = kCFCompareGreaterThan;
  }

  return (result);
}


// *************************************************************************
//
// IOHIDDevice_GetLongProperty(inIOHIDDeviceRef, inKey, outValue)
//
// Purpose: convieance function to return a long property of a device
//
static Boolean IOHIDDevice_GetLongProperty(IOHIDDeviceRef inIOHIDDeviceRef, CFStringRef inKey, uint32_t *outValue)
{
  Boolean result = FALSE;

  if (inIOHIDDeviceRef)
  {
    assert(IOHIDDeviceGetTypeID() == CFGetTypeID(inIOHIDDeviceRef));

    CFTypeRef tCFTypeRef = IOHIDDeviceGetProperty(inIOHIDDeviceRef, inKey);
    if (tCFTypeRef)
    {
      // if this is a number
      if (CFNumberGetTypeID() == CFGetTypeID(tCFTypeRef))
      {
        // get it's value
        result = CFNumberGetValue((CFNumberRef)tCFTypeRef, kCFNumberSInt32Type, outValue);
      }
    }
  }

  return (result);
}

// *************************************************************************
//
// IOHIDDevice_GetVendorID(inIOHIDDeviceRef)
//
// Purpose: get the vendor ID for this device
//
long IOHIDDevice_GetVendorID(IOHIDDeviceRef inIOHIDDeviceRef)
{
  uint32_t result = 0;

  (void)IOHIDDevice_GetLongProperty(inIOHIDDeviceRef, CFSTR(kIOHIDVendorIDKey), &result);
  return (result);
}

// *************************************************************************
//
// IOHIDDevice_GetProductID(inIOHIDDeviceRef)
//
// Purpose: get the product ID for this device
//
long IOHIDDevice_GetProductID(IOHIDDeviceRef inIOHIDDeviceRef)
{
  uint32_t result = 0;

  (void)IOHIDDevice_GetLongProperty(inIOHIDDeviceRef, CFSTR(kIOHIDProductIDKey), &result);
  return (result);
}

// *************************************************************************
//
// IOHIDDevice_GetLocationID(inIOHIDDeviceRef)
//
// Purpose: get the location ID for this device
//
long IOHIDDevice_GetLocationID(IOHIDDeviceRef inIOHIDDeviceRef)
{
  uint32_t result = 0;

  (void)IOHIDDevice_GetLongProperty(inIOHIDDeviceRef, CFSTR(kIOHIDLocationIDKey), &result);
  return (result);
}
