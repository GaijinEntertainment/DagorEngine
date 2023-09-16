/*############################################################################*/
/*#                                                                          #*/
/*#  MIT HRTF C Library                                                      #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:  mit_hrtf_filter.h                                            #*/
/*#  Version:   0.1                                                          #*/
/*#  Date:      04/05/2007                                                   #*/
/*#  Author(s): Aristotel Digenis                                            #*/
/*#  Credit:    Bill Gardner and Keith Martin                                #*/
/*#  Licence:   MIT                                                          #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _MIT_HRTF_FILTER_H
#define _MIT_HRTF_FILTER_H

#define MIT_HRTF_AZI_POSITIONS_00	37
#define MIT_HRTF_AZI_POSITIONS_10	37
#define MIT_HRTF_AZI_POSITIONS_20	37
#define MIT_HRTF_AZI_POSITIONS_30	31
#define MIT_HRTF_AZI_POSITIONS_40	29
#define MIT_HRTF_AZI_POSITIONS_50	23
#define MIT_HRTF_AZI_POSITIONS_60	19
#define MIT_HRTF_AZI_POSITIONS_70	13
#define MIT_HRTF_AZI_POSITIONS_80	7
#define MIT_HRTF_AZI_POSITIONS_90	1

#define MIT_HRTF_44_TAPS	128
#define MIT_HRTF_48_TAPS	140
#define MIT_HRTF_88_TAPS	256
#define MIT_HRTF_96_TAPS	279

typedef struct mit_hrtf_filter_44_str
{
	short left[MIT_HRTF_44_TAPS];
	short right[MIT_HRTF_44_TAPS];
}mit_hrtf_filter_44;

typedef struct mit_hrtf_filter_48_str
{
	short left[MIT_HRTF_48_TAPS];
	short right[MIT_HRTF_48_TAPS];
}mit_hrtf_filter_48;

typedef struct mit_hrtf_filter_88_str
{
	short left[MIT_HRTF_88_TAPS];
	short right[MIT_HRTF_88_TAPS];
}mit_hrtf_filter_88;

typedef struct mit_hrtf_filter_96_str
{
	short left[MIT_HRTF_96_TAPS];
	short right[MIT_HRTF_96_TAPS];
}mit_hrtf_filter_96;

typedef struct mit_hrtf_filter_set_44_str
{
	mit_hrtf_filter_44 e_10[MIT_HRTF_AZI_POSITIONS_10];
	mit_hrtf_filter_44 e_20[MIT_HRTF_AZI_POSITIONS_20];
	mit_hrtf_filter_44 e_30[MIT_HRTF_AZI_POSITIONS_30];
	mit_hrtf_filter_44 e_40[MIT_HRTF_AZI_POSITIONS_40];
	mit_hrtf_filter_44 e00[MIT_HRTF_AZI_POSITIONS_00];
	mit_hrtf_filter_44 e10[MIT_HRTF_AZI_POSITIONS_10];
	mit_hrtf_filter_44 e20[MIT_HRTF_AZI_POSITIONS_20];
	mit_hrtf_filter_44 e30[MIT_HRTF_AZI_POSITIONS_30];
	mit_hrtf_filter_44 e40[MIT_HRTF_AZI_POSITIONS_40];
	mit_hrtf_filter_44 e50[MIT_HRTF_AZI_POSITIONS_50];
	mit_hrtf_filter_44 e60[MIT_HRTF_AZI_POSITIONS_60];
	mit_hrtf_filter_44 e70[MIT_HRTF_AZI_POSITIONS_70];
	mit_hrtf_filter_44 e80[MIT_HRTF_AZI_POSITIONS_80];
	mit_hrtf_filter_44 e90[MIT_HRTF_AZI_POSITIONS_90];
}mit_hrtf_filter_set_44;

typedef struct mit_hrtf_filter_set_48_str
{
	mit_hrtf_filter_48 e_10[MIT_HRTF_AZI_POSITIONS_10];
	mit_hrtf_filter_48 e_20[MIT_HRTF_AZI_POSITIONS_20];
	mit_hrtf_filter_48 e_30[MIT_HRTF_AZI_POSITIONS_30];
	mit_hrtf_filter_48 e_40[MIT_HRTF_AZI_POSITIONS_40];
	mit_hrtf_filter_48 e00[MIT_HRTF_AZI_POSITIONS_00];
	mit_hrtf_filter_48 e10[MIT_HRTF_AZI_POSITIONS_10];
	mit_hrtf_filter_48 e20[MIT_HRTF_AZI_POSITIONS_20];
	mit_hrtf_filter_48 e30[MIT_HRTF_AZI_POSITIONS_30];
	mit_hrtf_filter_48 e40[MIT_HRTF_AZI_POSITIONS_40];
	mit_hrtf_filter_48 e50[MIT_HRTF_AZI_POSITIONS_50];
	mit_hrtf_filter_48 e60[MIT_HRTF_AZI_POSITIONS_60];
	mit_hrtf_filter_48 e70[MIT_HRTF_AZI_POSITIONS_70];
	mit_hrtf_filter_48 e80[MIT_HRTF_AZI_POSITIONS_80];
	mit_hrtf_filter_48 e90[MIT_HRTF_AZI_POSITIONS_90];
}mit_hrtf_filter_set_48;

typedef struct mit_hrtf_filter_set_88_str
{
	mit_hrtf_filter_88 e_10[MIT_HRTF_AZI_POSITIONS_10];
	mit_hrtf_filter_88 e_20[MIT_HRTF_AZI_POSITIONS_20];
	mit_hrtf_filter_88 e_30[MIT_HRTF_AZI_POSITIONS_30];
	mit_hrtf_filter_88 e_40[MIT_HRTF_AZI_POSITIONS_40];
	mit_hrtf_filter_88 e00[MIT_HRTF_AZI_POSITIONS_00];
	mit_hrtf_filter_88 e10[MIT_HRTF_AZI_POSITIONS_10];
	mit_hrtf_filter_88 e20[MIT_HRTF_AZI_POSITIONS_20];
	mit_hrtf_filter_88 e30[MIT_HRTF_AZI_POSITIONS_30];
	mit_hrtf_filter_88 e40[MIT_HRTF_AZI_POSITIONS_40];
	mit_hrtf_filter_88 e50[MIT_HRTF_AZI_POSITIONS_50];
	mit_hrtf_filter_88 e60[MIT_HRTF_AZI_POSITIONS_60];
	mit_hrtf_filter_88 e70[MIT_HRTF_AZI_POSITIONS_70];
	mit_hrtf_filter_88 e80[MIT_HRTF_AZI_POSITIONS_80];
	mit_hrtf_filter_88 e90[MIT_HRTF_AZI_POSITIONS_90];
}mit_hrtf_filter_set_88;

typedef struct mit_hrtf_filter_set_96_str
{
	mit_hrtf_filter_96 e_10[MIT_HRTF_AZI_POSITIONS_10];
	mit_hrtf_filter_96 e_20[MIT_HRTF_AZI_POSITIONS_20];
	mit_hrtf_filter_96 e_30[MIT_HRTF_AZI_POSITIONS_30];
	mit_hrtf_filter_96 e_40[MIT_HRTF_AZI_POSITIONS_40];
	mit_hrtf_filter_96 e00[MIT_HRTF_AZI_POSITIONS_00];
	mit_hrtf_filter_96 e10[MIT_HRTF_AZI_POSITIONS_10];
	mit_hrtf_filter_96 e20[MIT_HRTF_AZI_POSITIONS_20];
	mit_hrtf_filter_96 e30[MIT_HRTF_AZI_POSITIONS_30];
	mit_hrtf_filter_96 e40[MIT_HRTF_AZI_POSITIONS_40];
	mit_hrtf_filter_96 e50[MIT_HRTF_AZI_POSITIONS_50];
	mit_hrtf_filter_96 e60[MIT_HRTF_AZI_POSITIONS_60];
	mit_hrtf_filter_96 e70[MIT_HRTF_AZI_POSITIONS_70];
	mit_hrtf_filter_96 e80[MIT_HRTF_AZI_POSITIONS_80];
	mit_hrtf_filter_96 e90[MIT_HRTF_AZI_POSITIONS_90];
}mit_hrtf_filter_set_96;

#endif // _MIT_HRTF_FILTER_H