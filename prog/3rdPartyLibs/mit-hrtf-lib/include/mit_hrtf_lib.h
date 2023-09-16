/*############################################################################*/
/*#                                                                          #*/
/*#  MIT HRTF C Library                                                      #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:  mit_hrtf_lib.h                                               #*/
/*#  Version:   0.1                                                          #*/
/*#  Date:      04/05/2007                                                   #*/
/*#  Author(s): Aristotel Digenis                                            #*/
/*#  Credit:    Bill Gardner and Keith Martin                                #*/
/*#  Licence:   MIT                                                          #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _MIT_HRTF_LIB_H_
#define _MIT_HRTF_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif



/**
	Checks if an HRTF set is available for the specified settings.

	"azimuth" is in the range of [-180, 180] degrees, where position 0 is the 
	centre in front of the listener, negative values are to the left of the 
	listener, and positive values are to the right of the front centre.

	"elevation" is in the range of [-90, 90] degrees, where position 0 is the 
	centre in front of the listener, negative values are below the listener,
	and positive values are above the listener.

	"samplerate" can be one of the following: 44100, 48000, 88200, 96000

	"diffused" can be 0(false) for the normal set, or 1(true) for the diffused
	set, which is equalised for less accurate localization in exchange for
	a flatter frequency response. The diffused set is better suited for music.
	
	Returns the number of taps needed for the each channel of the available 
	set. Returns 0 if the requested HRTF set is not available.
*/
unsigned int mit_hrtf_availability(int azimuth, int elevation, unsigned int samplerate, unsigned int diffused);



/**
	Copies HRTF taps to given buffers, for specified HRTF set. 

	"pAzimuth" is in the range of [-180, 180] degrees, where position 0 is the 
	centre in front of the listener, negative values are to the left of the 
	listener, and positive values are to the right of the front centre. The
	variable is a pointer, so that once the function returns, the actual
	azimuth position used is written to that pointed variable.

	"pElevation" is in the range of [-90, 90] degrees, where position 0 is the 
	centre in front of the listener, negative values are below the listener,
	and positive values are above the listener. The variable is a pointer, so
	that once the function returns, the actual azimuth position used is 
	written to that pointed variable.

	"samplerate" can be one of the following: 44100, 48000, 88200, 96000

	"diffused" can be 0(false) for the normal set, or 1(true) for the diffused
	set, which is equalised for less accurate localization in exchange for
	a flatter frequency response. The diffused set is better suited for music.

	"pLeft" and "pRight" are pointers to buffers allocated (and later 
	deallocated) by the user based on the return value of the 
	"mit_hrtf_availability" function.
	
	Returns the number of taps copied to each of the "psLeft" and "psRight" 
	buffers. Returns 0 if the requested HRTF set is not available or if there
	was an error.
*/
unsigned int mit_hrtf_get(int* pAzimuth, int* pElevation, unsigned int samplerate, unsigned int diffused, short* pLeft, short* pRight);


#ifdef __cplusplus
} 
#endif

#endif // _MIT_HRTF_LIB_H_