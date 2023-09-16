/*############################################################################*/
/*#                                                                          #*/
/*#  MIT HRTF C Library                                                      #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:  mit_hrtf_lib.c                                               #*/
/*#  Version:   0.1                                                          #*/
/*#  Date:      04/05/2007                                                   #*/
/*#  Author(s): Aristotel Digenis                                            #*/
/*#  Credit:    Bill Gardner and Keith Martin                                #*/
/*#  Licence:   MIT                                                          #*/
/*#                                                                          #*/
/*############################################################################*/


#include "../include/mit_hrtf_lib.h"
#include "normal/mit_hrtf_normal_44100.h"
#include "diffuse/mit_hrtf_diffuse_44100.h"
#include "normal/mit_hrtf_normal_48000.h"
#include "diffuse/mit_hrtf_diffuse_48000.h"
#include "normal/mit_hrtf_normal_88200.h"
#include "diffuse/mit_hrtf_diffuse_88200.h"
#include "normal/mit_hrtf_normal_96000.h"
#include "diffuse/mit_hrtf_diffuse_96000.h"



/*	Internal functions for handling the indexing of the -/+40 degree elevation 
	data which has irregular azimuth positions. */
int mit_hrtf_findAzimuthFor40Elev(int azimuth);
int mit_hrtf_findIndexFor40Elev(int azimuth);



unsigned int mit_hrtf_availability(int azimuth, int elevation, unsigned int samplerate, unsigned int diffused)
{
	if (azimuth > 180 || azimuth < -180 || elevation > 90 || elevation < -40 || diffused > 1)
	{
		return 0;
	}
  
	switch (samplerate)
	{
	case 44100: return MIT_HRTF_44_TAPS;
	case 48000: return MIT_HRTF_48_TAPS;
	case 88200: return MIT_HRTF_88_TAPS;
	case 96000: return MIT_HRTF_96_TAPS;
	}

	return 0;
}



unsigned int mit_hrtf_get(int* pAzimuth, int* pElevation, unsigned int samplerate, unsigned int diffused, short* pLeft, short* pRight)
{
	// Stash local copies of azimuth and elevation
	int localAzimuth = *pAzimuth;
	int localElevation = *pElevation;

	// Check if the requested HRTF exists
	if (mit_hrtf_availability(localAzimuth, localElevation, samplerate, diffused) == 0)
	{
		return 0;
	}

	// Snap elevation to the nearest available elevation in the filter set
	if (localElevation < 0)
	{
		localElevation = ((localElevation - 5) / 10) * 10;
	}
	else
	{
		localElevation = ((localElevation + 5) / 10) * 10;
	}

	// Elevation of 50 has a maximum 176 in the azimuth plane so we need to handle that.
	if(localElevation == 50)
	{
		if (localAzimuth < 0)
		{
			localAzimuth = localAzimuth < -176 ? -176 : localAzimuth;
		}
		else
		{
			localAzimuth = localAzimuth > 176 ? 176 : localAzimuth;
		}
	}

	// Snap azimuth to the nearest available azimuth in the filter set.
	float azimuthIncrement = 0;
	switch(localElevation)
	{
	case 0:		azimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_00 - 1);		break;	// 180 5
	case 10:	
	case -10:	azimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_10 - 1);		break;	// 180 5
	case 20:	
	case -20:	azimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_20 - 1);		break;	// 180 5
	case 30:	
	case -30:	azimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_30 - 1);		break;	// 180 6
	case 40:	
	case -40:	azimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_40 - 1);		break;	// 180 6.43
	case 50:	azimuthIncrement = 176.f / (MIT_HRTF_AZI_POSITIONS_50 - 1);		break;	// 176 8
	case 60:	azimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_60 - 1);		break;	// 180 10
	case 70:	azimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_70 - 1);		break;	// 180 15
	case 80:	azimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_80 - 1);		break;	// 180 30
	case 90:	azimuthIncrement = 0;											break;	// 0   1
	};

	unsigned int swapLeftRight = 0;
	if(localAzimuth < 0)
	{
		localAzimuth = (int)((int)((-localAzimuth + azimuthIncrement / 2.f) / azimuthIncrement) * azimuthIncrement + 0.5f);
		swapLeftRight = 1;
	}
	else
	{
		localAzimuth = (int)((int)((localAzimuth + azimuthIncrement / 2.f) / azimuthIncrement) * azimuthIncrement + 0.5f);
	}

	// Determine array index for azimuth based on elevation
	int azimuthIndex = 0;
	switch(localElevation)
	{
	case 0:		azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_00 - 1));	break;
	case 10: 
	case -10:	azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_10 - 1));	break;
	case 20:	
	case -20:	azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_20 - 1));	break;
	case 30:	
	case -30:	azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_30 - 1));	break;
	case 40:	
	case -40:	azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_40 - 1));	break;
	case 50:	azimuthIndex = (int)((localAzimuth / 176.f) * (MIT_HRTF_AZI_POSITIONS_50 - 1));	break;
	case 60:	azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_60 - 1));	break;
	case 70:	azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_70 - 1));	break;
	case 80:	azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_80 - 1));	break;
	case 90:	azimuthIndex = (int)((localAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_90 - 1));	break;
	};

	// The azimuths for +/- elevations need special handling
	if(localElevation == 40 || localElevation == -40)
	{
		localAzimuth = mit_hrtf_findAzimuthFor40Elev(localAzimuth);
		azimuthIndex = mit_hrtf_findIndexFor40Elev(localAzimuth);
	}

	// Assign pointer to appropriate array depending on sample rate, normal or diffused filters, elevation, and azimuth index.
	const short* pLeftTaps = 0;
	const short* pRightTaps = 0;
	unsigned int totalTaps = 0;
	switch(samplerate)
	{
        const mit_hrtf_filter_set_44* pFilter44 = 0;
        const mit_hrtf_filter_set_48* pFilter48 = 0;
        const mit_hrtf_filter_set_88* pFilter88 = 0;
        const mit_hrtf_filter_set_96* pFilter96 = 0;
            
	case 44100:
        pFilter44 = diffused ? &diffuse_44 : &normal_44;

		switch(localElevation)
		{	
		case -10:	pLeftTaps = pFilter44->e_10[azimuthIndex].left;
					pRightTaps = pFilter44->e_10[azimuthIndex].right;		break;
		case -20:	pLeftTaps = pFilter44->e_20[azimuthIndex].left;
					pRightTaps = pFilter44->e_20[azimuthIndex].right;		break;
		case -30:	pLeftTaps = pFilter44->e_30[azimuthIndex].left;
					pRightTaps = pFilter44->e_30[azimuthIndex].right;		break;
		case -40:	pLeftTaps = pFilter44->e_40[azimuthIndex].left;
					pRightTaps = pFilter44->e_40[azimuthIndex].right;		break;
		case 0:		pLeftTaps = pFilter44->e00[azimuthIndex].left;
					pRightTaps = pFilter44->e00[azimuthIndex].right;		break;
		case 10:	pLeftTaps = pFilter44->e10[azimuthIndex].left;
					pRightTaps = pFilter44->e10[azimuthIndex].right;		break;
		case 20:	pLeftTaps = pFilter44->e20[azimuthIndex].left;
					pRightTaps = pFilter44->e20[azimuthIndex].right;		break;
		case 30:	pLeftTaps = pFilter44->e30[azimuthIndex].left;
					pRightTaps = pFilter44->e30[azimuthIndex].right;		break;
		case 40:	pLeftTaps = pFilter44->e40[azimuthIndex].left;
					pRightTaps = pFilter44->e40[azimuthIndex].right;		break;
		case 50:	pLeftTaps = pFilter44->e50[azimuthIndex].left;
					pRightTaps = pFilter44->e50[azimuthIndex].right;		break;
		case 60:	pLeftTaps = pFilter44->e60[azimuthIndex].left;
					pRightTaps = pFilter44->e60[azimuthIndex].right;		break;
		case 70:	pLeftTaps = pFilter44->e70[azimuthIndex].left;
					pRightTaps = pFilter44->e70[azimuthIndex].right;		break;
		case 80:	pLeftTaps = pFilter44->e80[azimuthIndex].left;
					pRightTaps = pFilter44->e80[azimuthIndex].right;		break;
		case 90:	pLeftTaps = pFilter44->e90[azimuthIndex].left;
					pRightTaps = pFilter44->e90[azimuthIndex].right;		break;
		};
		
		// How many taps to copy later to user's buffers
		totalTaps = MIT_HRTF_44_TAPS;
		break;
	case 48000:
		pFilter48 = diffused ? &diffuse_48 : &normal_48;

		switch(localElevation)
		{	
		case -10:	pLeftTaps = pFilter48->e_10[azimuthIndex].left;
					pRightTaps = pFilter48->e_10[azimuthIndex].right;		break;
		case -20:	pLeftTaps = pFilter48->e_20[azimuthIndex].left;
					pRightTaps = pFilter48->e_20[azimuthIndex].right;		break;
		case -30:	pLeftTaps = pFilter48->e_30[azimuthIndex].left;
					pRightTaps = pFilter48->e_30[azimuthIndex].right;		break;
		case -40:	pLeftTaps = pFilter48->e_40[azimuthIndex].left;
					pRightTaps = pFilter48->e_40[azimuthIndex].right;		break;
		case 0:		pLeftTaps = pFilter48->e00[azimuthIndex].left;
					pRightTaps = pFilter48->e00[azimuthIndex].right;		break;
		case 10:	pLeftTaps = pFilter48->e10[azimuthIndex].left;
					pRightTaps = pFilter48->e10[azimuthIndex].right;		break;
		case 20:	pLeftTaps = pFilter48->e20[azimuthIndex].left;
					pRightTaps = pFilter48->e20[azimuthIndex].right;		break;
		case 30:	pLeftTaps = pFilter48->e30[azimuthIndex].left;
					pRightTaps = pFilter48->e30[azimuthIndex].right;		break;
		case 40:	pLeftTaps = pFilter48->e40[azimuthIndex].left;
					pRightTaps = pFilter48->e40[azimuthIndex].right;		break;
		case 50:	pLeftTaps = pFilter48->e50[azimuthIndex].left;
					pRightTaps = pFilter48->e50[azimuthIndex].right;		break;
		case 60:	pLeftTaps = pFilter48->e60[azimuthIndex].left;
					pRightTaps = pFilter48->e60[azimuthIndex].right;		break;
		case 70:	pLeftTaps = pFilter48->e70[azimuthIndex].left;
					pRightTaps = pFilter48->e70[azimuthIndex].right;		break;
		case 80:	pLeftTaps = pFilter48->e80[azimuthIndex].left;
					pRightTaps = pFilter48->e80[azimuthIndex].right;		break;
		case 90:	pLeftTaps = pFilter48->e90[azimuthIndex].left;
					pRightTaps = pFilter48->e90[azimuthIndex].right;		break;
		};
		
		// How many taps to copy later to user's buffers
		totalTaps = MIT_HRTF_48_TAPS;
		break;
	case 88200:
		pFilter88 = diffused ? &diffuse_88 : &normal_88;

		switch(localElevation)
		{	
		case -10:	pLeftTaps = pFilter88->e_10[azimuthIndex].left;
					pRightTaps = pFilter88->e_10[azimuthIndex].right;		break;
		case -20:	pLeftTaps = pFilter88->e_20[azimuthIndex].left;
					pRightTaps = pFilter88->e_20[azimuthIndex].right;		break;
		case -30:	pLeftTaps = pFilter88->e_30[azimuthIndex].left;
					pRightTaps = pFilter88->e_30[azimuthIndex].right;		break;
		case -40:	pLeftTaps = pFilter88->e_40[azimuthIndex].left;
					pRightTaps = pFilter88->e_40[azimuthIndex].right;		break;
		case 0:		pLeftTaps = pFilter88->e00[azimuthIndex].left;
					pRightTaps = pFilter88->e00[azimuthIndex].right;		break;
		case 10:	pLeftTaps = pFilter88->e10[azimuthIndex].left;
					pRightTaps = pFilter88->e10[azimuthIndex].right;		break;
		case 20:	pLeftTaps = pFilter88->e20[azimuthIndex].left;
					pRightTaps = pFilter88->e20[azimuthIndex].right;		break;
		case 30:	pLeftTaps = pFilter88->e30[azimuthIndex].left;
					pRightTaps = pFilter88->e30[azimuthIndex].right;		break;
		case 40:	pLeftTaps = pFilter88->e40[azimuthIndex].left;
					pRightTaps = pFilter88->e40[azimuthIndex].right;		break;
		case 50:	pLeftTaps = pFilter88->e50[azimuthIndex].left;
					pRightTaps = pFilter88->e50[azimuthIndex].right;		break;
		case 60:	pLeftTaps = pFilter88->e60[azimuthIndex].left;
					pRightTaps = pFilter88->e60[azimuthIndex].right;		break;
		case 70:	pLeftTaps = pFilter88->e70[azimuthIndex].left;
					pRightTaps = pFilter88->e70[azimuthIndex].right;		break;
		case 80:	pLeftTaps = pFilter88->e80[azimuthIndex].left;
					pRightTaps = pFilter88->e80[azimuthIndex].right;		break;
		case 90:	pLeftTaps = pFilter88->e90[azimuthIndex].left;
					pRightTaps = pFilter88->e90[azimuthIndex].right;		break;
		};
		
		// How many taps to copy later to user's buffers
		totalTaps = MIT_HRTF_88_TAPS;
		break;
	case 96000:
		pFilter96 = diffused ? &diffuse_96 : &normal_96;

		switch(localElevation)
		{	
		case -10:	pLeftTaps = pFilter96->e_10[azimuthIndex].left;
					pRightTaps = pFilter96->e_10[azimuthIndex].right;		break;
		case -20:	pLeftTaps = pFilter96->e_20[azimuthIndex].left;
					pRightTaps = pFilter96->e_20[azimuthIndex].right;		break;
		case -30:	pLeftTaps = pFilter96->e_30[azimuthIndex].left;
					pRightTaps = pFilter96->e_30[azimuthIndex].right;		break;
		case -40:	pLeftTaps = pFilter96->e_40[azimuthIndex].left;
					pRightTaps = pFilter96->e_40[azimuthIndex].right;		break;
		case 0:		pLeftTaps = pFilter96->e00[azimuthIndex].left;
					pRightTaps = pFilter96->e00[azimuthIndex].right;		break;
		case 10:	pLeftTaps = pFilter96->e10[azimuthIndex].left;
					pRightTaps = pFilter96->e10[azimuthIndex].right;		break;
		case 20:	pLeftTaps = pFilter96->e20[azimuthIndex].left;
					pRightTaps = pFilter96->e20[azimuthIndex].right;		break;
		case 30:	pLeftTaps = pFilter96->e30[azimuthIndex].left;
					pRightTaps = pFilter96->e30[azimuthIndex].right;		break;
		case 40:	pLeftTaps = pFilter96->e40[azimuthIndex].left;
					pRightTaps = pFilter96->e40[azimuthIndex].right;		break;
		case 50:	pLeftTaps = pFilter96->e50[azimuthIndex].left;
					pRightTaps = pFilter96->e50[azimuthIndex].right;		break;
		case 60:	pLeftTaps = pFilter96->e60[azimuthIndex].left;
					pRightTaps = pFilter96->e60[azimuthIndex].right;		break;
		case 70:	pLeftTaps = pFilter96->e70[azimuthIndex].left;
					pRightTaps = pFilter96->e70[azimuthIndex].right;		break;
		case 80:	pLeftTaps = pFilter96->e80[azimuthIndex].left;
					pRightTaps = pFilter96->e80[azimuthIndex].right;		break;
		case 90:	pLeftTaps = pFilter96->e90[azimuthIndex].left;
					pRightTaps = pFilter96->e90[azimuthIndex].right;		break;
		};
		
		// How many taps to copy later to user's buffers
		totalTaps = MIT_HRTF_96_TAPS;
		break;
	};

	// Switch left and right ear if the azimuth is to the left of front centre (azimuth < 0)
	if(swapLeftRight)
	{
		const short* pTempTaps = pRightTaps;
		pRightTaps = pLeftTaps;
		pLeftTaps = pTempTaps;
	}
	
	// Copy taps to user's arrays
	for(unsigned int tap = 0; tap < totalTaps; tap++)
	{
		pLeft[tap] = pLeftTaps[tap];
		pRight[tap] = pRightTaps[tap];
	}

	// Assign the real azimuth and elevation used
	*pAzimuth = localAzimuth;
	*pElevation = localElevation;

	return totalTaps;	
}



int mit_hrtf_findAzimuthFor40Elev(int azimuth)
{
	if(azimuth >= 0 && azimuth < 4)
		return 0;
	else if(azimuth >= 4 && azimuth < 10)
		return 6;
	else if(azimuth >= 10 && azimuth < 17)
		return 13;
	else if(azimuth >= 17 && azimuth < 23)
		return 19;
	else if(azimuth >= 23 && azimuth < 30)
		return 26;
	else if(azimuth >= 30 && azimuth < 36)
		return 32;
	else if(azimuth >= 36 && azimuth < 43)
		return 39;
	else if(azimuth >= 43 && azimuth < 49)
		return 45;
	else if(azimuth >= 49 && azimuth < 55)
		return 51;
	else if(azimuth >= 55 && azimuth < 62)
		return 58;
	else if(azimuth >= 62 && azimuth < 68)
		return 64;
	else if(azimuth >= 68 && azimuth < 75)
		return 71;
	else if(azimuth >= 75 && azimuth < 81)
		return 77;
	else if(azimuth >= 81 && azimuth < 88)
		return 84;
	else if(azimuth >= 88 && azimuth < 94)
		return 90;
	else if(azimuth >= 94 && azimuth < 100)
		return 96;
	else if(azimuth >= 100 && azimuth < 107)
		return 103;
	else if(azimuth >= 107 && azimuth < 113)
		return 109;
	else if(azimuth >= 113 && azimuth < 120)
		return 116;
	else if(azimuth >= 120 && azimuth < 126)
		return 122;
	else if(azimuth >= 126 && azimuth < 133)
		return 129;
	else if(azimuth >= 133 && azimuth < 139)
		return 135;
	else if(azimuth >= 139 && azimuth < 145)
		return 141;
	else if(azimuth >= 145 && azimuth < 152)
		return 148;
	else if(azimuth >= 152 && azimuth < 158)
		return 154;
	else if(azimuth >= 158 && azimuth < 165)
		return 161;
	else if(azimuth >= 165 && azimuth < 171)
		return 167;
	else if(azimuth >= 171 && azimuth < 178)
		return 174;
	else
		return 180;
};



int mit_hrtf_findIndexFor40Elev(int azimuth)
{
	if(azimuth >= 0 && azimuth < 4)
		return 0;
	else if(azimuth >= 4 && azimuth < 10)
		return 1;
	else if(azimuth >= 10 && azimuth < 17)
		return 2;
	else if(azimuth >= 17 && azimuth < 23)
		return 3;
	else if(azimuth >= 23 && azimuth < 30)
		return 4;
	else if(azimuth >= 30 && azimuth < 36)
		return 5;
	else if(azimuth >= 36 && azimuth < 43)
		return 6;
	else if(azimuth >= 43 && azimuth < 49)
		return 7;
	else if(azimuth >= 49 && azimuth < 55)
		return 8;
	else if(azimuth >= 55 && azimuth < 62)
		return 9;
	else if(azimuth >= 62 && azimuth < 68)
		return 10;
	else if(azimuth >= 68 && azimuth < 75)
		return 11;
	else if(azimuth >= 75 && azimuth < 81)
		return 12;
	else if(azimuth >= 81 && azimuth < 88)
		return 13;
	else if(azimuth >= 88 && azimuth < 94)
		return 14;
	else if(azimuth >= 94 && azimuth < 100)
		return 15;
	else if(azimuth >= 100 && azimuth < 107)
		return 16;
	else if(azimuth >= 107 && azimuth < 113)
		return 17;
	else if(azimuth >= 113 && azimuth < 120)
		return 18;
	else if(azimuth >= 120 && azimuth < 126)
		return 19;
	else if(azimuth >= 126 && azimuth < 133)
		return 20;
	else if(azimuth >= 133 && azimuth < 139)
		return 21;
	else if(azimuth >= 139 && azimuth < 145)
		return 22;
	else if(azimuth >= 145 && azimuth < 152)
		return 23;
	else if(azimuth >= 152 && azimuth < 158)
		return 24;
	else if(azimuth >= 158 && azimuth < 165)
		return 25;
	else if(azimuth >= 165 && azimuth < 171)
		return 26;
	else if(azimuth >= 171 && azimuth < 178)
		return 27;
	else
		return 28;
}
