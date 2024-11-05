// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// *******************************************************************************
// *
// * Module Name:
// *   NPClient.h
// *
// * Doyle Nickless -- 13 Jan, 2003 -- for Eye Control Technology.
// *
// * Abstract:
// *   Header for NaturalPoint Game Client API.
// *
// * Environment:
// *   Microsoft Windows -- User mode
// *
// *******************************************************************************


#pragma pack(push, npclient_h) // Save current pack value
#pragma pack(1)

//////////////////
/// Defines //////////////////////////////////////////////////////////////////////
/////////////////
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_BUILD 1

// magic to get the preprocessor to do what we want
#define LITA(arg)      #arg
#define XLITA(arg)     LITA(arg)
#define CAT3(w, x, z)  w##.##x##.##z##\000
#define XCAT3(w, x, z) CAT3(w, x, z)
#define VERSION_STRING XLITA(XCAT3(VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD))
//
// Versioning hasn't been worked out yet...
//
// The following is the previous spec definition of versioning info -- I can probably do
// something very similar to this -- will keep you posted.
//
// request version information using 2 messages, they cannot be expected to arrive in a specific order - so always parse using the High
// byte the messages have a NPCONTROL byte in the first parameter, and the second parameter has packed bytes.
//   Message 1) (first parameter)NPCONTROL : (second parameter) (High Byte)NPVERSIONMAJOR (Low Byte) major version number data
//   Message 2) (first parameter)NPCONTROL : (second parameter) (High Byte)NPVERSIONMINOR (Low Byte) minor version number data

#define NPQUERYVERSION 1040

#define NPSTATUS_REMOTEACTIVE   0
#define NPSTATUS_REMOTEDISABLED 1

// CONTROL DATA SUBFIELDS
#define NPVERSIONMAJOR 1
#define NPVERSIONMINOR 2

// DATA FIELDS
#define NPCONTROL \
  8 // indicates a control data field
    // the second parameter of a message bearing control data information contains a packed data format.
    // The High byte indicates what the data is, and the Low byte contains the actual data
// roll, pitch, yaw
#define NPROLL  1 // +/- 16383 (representing +/- 180) [data = input - 16383]
#define NPPITCH 2 // +/- 16383 (representing +/- 180) [data = input - 16383]
#define NPYAW   4 // +/- 16383 (representing +/- 180) [data = input - 16383]

// x, y, z - remaining 6dof coordinates
#define NPX 16 // +/- 16383 [data = input - 16383]
#define NPY 32 // +/- 16383 [data = input - 16383]
#define NPZ 64 // +/- 16383 [data = input - 16383]

// raw object position from imager
#define NPRAWX 128 // 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]
#define NPRAWY 256 // 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]
#define NPRAWZ 512 // 0..25600 (actual value is multiplied x 100 to pass two decimal places of precision)  [data = input / 100]

// x, y, z deltas from raw imager position
#define NPDELTAX \
  1024 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]
#define NPDELTAY \
  2048 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]
#define NPDELTAZ \
  4096 // +/- 2560 (actual value is multiplied x 10 to pass two decimal places of precision)  [data = (input / 10) - 256]

// raw object position from imager
#define NPSMOOTHX 8192  // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]
#define NPSMOOTHY 16384 // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]
#define NPSMOOTHZ 32768 // 0..32766 (actual value is multiplied x 10 to pass one decimal place of precision) [data = input / 10]


//////////////////
/// Typedefs /////////////////////////////////////////////////////////////////////
/////////////////

// NPESULT values are returned from the Game Client API functions.
//
typedef enum tagNPResult
{
  NP_OK = 0,
  NP_ERR_DEVICE_NOT_PRESENT,
  NP_ERR_UNSUPPORTED_OS,
  NP_ERR_INVALID_ARG,
  NP_ERR_DLL_NOT_FOUND,
  NP_ERR_NO_DATA,
  NP_ERR_INTERNAL_DATA

} NPRESULT;

typedef struct tagTrackIRSignature
{
  char DllSignature[200];
  char AppSignature[200];

} SIGNATUREDATA, *LPTRACKIRSIGNATURE;

typedef struct tagTrackIRData
{
  unsigned short wNPStatus;
  unsigned short wPFrameSignature;
  unsigned long dwNPIOData;

  float fNPRoll;
  float fNPPitch;
  float fNPYaw;
  float fNPX;
  float fNPY;
  float fNPZ;
  float fNPRawX;
  float fNPRawY;
  float fNPRawZ;
  float fNPDeltaX;
  float fNPDeltaY;
  float fNPDeltaZ;
  float fNPSmoothX;
  float fNPSmoothY;
  float fNPSmoothZ;

} TRACKIRDATA, *LPTRACKIRDATA;


//
// Typedef for pointer to the notify callback function that is implemented within
// the client -- this function receives head tracker reports from the game client API
//
typedef NPRESULT(__stdcall *PF_NOTIFYCALLBACK)(unsigned short, unsigned short);

// Typedefs for game client API functions (useful for declaring pointers to these
// functions within the client for use during GetProcAddress() ops)
//
typedef NPRESULT(__stdcall *PF_NP_REGISTERWINDOWHANDLE)(HWND);
typedef NPRESULT(__stdcall *PF_NP_UNREGISTERWINDOWHANDLE)(void);
typedef NPRESULT(__stdcall *PF_NP_REGISTERPROGRAMPROFILEID)(unsigned short);
typedef NPRESULT(__stdcall *PF_NP_QUERYVERSION)(unsigned short *);
typedef NPRESULT(__stdcall *PF_NP_REQUESTDATA)(unsigned short);
typedef NPRESULT(__stdcall *PF_NP_GETSIGNATURE)(LPTRACKIRSIGNATURE);
typedef NPRESULT(__stdcall *PF_NP_GETDATA)(LPTRACKIRDATA);
typedef NPRESULT(__stdcall *PF_NP_REGISTERNOTIFY)(PF_NOTIFYCALLBACK);
typedef NPRESULT(__stdcall *PF_NP_UNREGISTERNOTIFY)(void);
typedef NPRESULT(__stdcall *PF_NP_STARTCURSOR)(void);
typedef NPRESULT(__stdcall *PF_NP_STOPCURSOR)(void);
typedef NPRESULT(__stdcall *PF_NP_RECENTER)(void);
typedef NPRESULT(__stdcall *PF_NP_STARTDATATRANSMISSION)(void);
typedef NPRESULT(__stdcall *PF_NP_STOPDATATRANSMISSION)(void);

//// Function Prototypes ///////////////////////////////////////////////
//
// Functions exported from game client API DLL ( note __stdcall calling convention
// is used for ease of interface to clients of differing implementations including
// C, C++, Pascal (Delphi) and VB. )
//
NPRESULT __stdcall NP_RegisterWindowHandle(HWND hWnd);
NPRESULT __stdcall NP_UnregisterWindowHandle(void);
NPRESULT __stdcall NP_RegisterProgramProfileID(unsigned short wPPID);
NPRESULT __stdcall NP_QueryVersion(unsigned short *pwVersion);
NPRESULT __stdcall NP_RequestData(unsigned short wDataReq);
NPRESULT __stdcall NP_GetSignature(LPTRACKIRSIGNATURE pSignature);
NPRESULT __stdcall NP_GetData(LPTRACKIRDATA pTID);
NPRESULT __stdcall NP_RegisterNotify(PF_NOTIFYCALLBACK pfNotify);
NPRESULT __stdcall NP_UnregisterNotify(void);
NPRESULT __stdcall NP_StartCursor(void);
NPRESULT __stdcall NP_StopCursor(void);
NPRESULT __stdcall NP_ReCenter(void);
NPRESULT __stdcall NP_StartDataTransmission(void);
NPRESULT __stdcall NP_StopDataTransmission(void);

/////////////////////////////////////////////////////////////////////////

#pragma pack(pop, npclient_h) // Ensure previous pack value is restored

//
// *** End of file: NPClient.h ***
//
