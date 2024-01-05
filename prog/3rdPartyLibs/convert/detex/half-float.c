/*

Copyright (c) 2015 Harm Hanemaaijer <fgenfb@yahoo.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <pthread.h>

#include "detex.h"
#include "half-float.h"

/******************************************************************************
 *
 * Filename:    ieeehalfprecision.c
 * Programmer:  James Tursa
 * Version:     1.0
 * Date:        March 3, 2009
 * Copyright:   (c) 2009 by James Tursa, All Rights Reserved
 *
 *  This code uses the BSD License:
 *
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are 
 *  met:
 *
 *     * Redistributions of source code must retain the above copyright 
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright 
 *       notice, this list of conditions and the following disclaimer in 
 *       the documentation and/or other materials provided with the distribution
 *      
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * This file contains C code to convert between IEEE double, single, and half
 * precision floating point formats. The intended use is for standalone C code
 * that does not rely on MATLAB mex.h. The bit pattern for the half precision
 * floating point format is stored in a 16-bit unsigned int variable. The half
 * precision bit pattern definition is:
 *
 * 1 bit sign bit
 * 5 bits exponent, biased by 15
 * 10 bits mantissa, hidden leading bit, normalized to 1.0
 *
 * Special floating point bit patterns recognized and supported:
 *
 * All exponent bits zero:
 * - If all mantissa bits are zero, then number is zero (possibly signed)
 * - Otherwise, number is a denormalized bit pattern
 *
 * All exponent bits set to 1:
 * - If all mantissa bits are zero, then number is +Infinity or -Infinity
 * - Otherwise, number is NaN (Not a Number)
 *
 * For the denormalized cases, note that 2^(-24) is the smallest number that can
 * be represented in half precision exactly. 2^(-25) will convert to 2^(-24)
 * because of the rounding algorithm used, and 2^(-26) is too small and underflows
 * to zero.
 *
 ********************************************************************************/

//-----------------------------------------------------------------------------
//
// Routine:  singles2halfp
//
// Input:  source = Address of 32-bit floating point data to convert
//         numel  = Number of values at that address to convert
//
// Output: target = Address of 16-bit data to hold output (numel values)
//         return value = 0 if native floating point format is IEEE
//                      = 1 if native floating point format is not IEEE
//
// Programmer:  James Tursa
//
//-----------------------------------------------------------------------------

static DETEX_INLINE_ONLY void singles2halfp(void * DETEX_RESTRICT target,
void * DETEX_RESTRICT source, int numel)
{
    uint16_t *hp = (uint16_t *) target; // Type pun output as an unsigned 16-bit int
    uint32_t *xp = (uint32_t *) source; // Type pun input as an unsigned 32-bit int
    uint16_t    hs, he, hm;
    uint32_t x, xs, xe, xm;
    int hes;
#if 0
    static int next;  // Little Endian adjustment
    static int checkieee = 1;  // Flag to check for IEEE754, Endian, and word size
    double one = 1.0; // Used for checking IEEE754 floating point format
    uint32_t *ip; // Used for checking IEEE754 floating point format
    
    if( checkieee ) { // 1st call, so check for IEEE754, Endian, and word size
        ip = (uint32_t *) &one;
        if( *ip ) { // If Big Endian, then no adjustment
            next = 0;
        } else { // If Little Endian, then adjustment will be necessary
            next = 1;
            ip++;
        }
        if( *ip != 0x3FF00000u ) { // Check for exact IEEE 754 bit pattern of 1.0
            return 1;  // Floating point bit pattern is not IEEE 754
        }
        if( sizeof(int16_t) != 2 || sizeof(int32_t) != 4 ) {
            return 1;  // short is not 16-bits, or long is not 32-bits.
        }
        checkieee = 0; // Everything checks out OK
    }
    
    if( source == NULL || target == NULL ) { // Nothing to convert (e.g., imag part of pure real)
        return 0;
    }
#endif
    
    while( numel-- ) {
        x = *xp++;
        if( (x & 0x7FFFFFFFu) == 0 ) {  // Signed zero
            *hp++ = (uint16_t) (x >> 16);  // Return the signed zero
        } else { // Not zero
            xs = x & 0x80000000u;  // Pick off sign bit
            xe = x & 0x7F800000u;  // Pick off exponent bits
            xm = x & 0x007FFFFFu;  // Pick off mantissa bits
            if( xe == 0 ) {  // Denormal will underflow, return a signed zero
                *hp++ = (uint16_t) (xs >> 16);
            } else if( xe == 0x7F800000u ) {  // Inf or NaN (all the exponent bits are set)
                if( xm == 0 ) { // If mantissa is zero ...
                    *hp++ = (uint16_t) ((xs >> 16) | 0x7C00u); // Signed Inf
                } else {
                    *hp++ = (uint16_t) 0xFE00u; // NaN, only 1st mantissa bit set
                }
            } else { // Normalized number
                hs = (uint16_t) (xs >> 16); // Sign bit
                hes = ((int)(xe >> 23)) - 127 + 15; // Exponent unbias the single, then bias the halfp
                if( hes >= 0x1F ) {  // Overflow
                    *hp++ = (uint16_t) ((xs >> 16) | 0x7C00u); // Signed Inf
                } else if( hes <= 0 ) {  // Underflow
                    if( (14 - hes) > 24 ) {  // Mantissa shifted all the way off & no rounding possibility
                        hm = (uint16_t) 0u;  // Set mantissa to zero
                    } else {
                        xm |= 0x00800000u;  // Add the hidden leading bit
                        hm = (uint16_t) (xm >> (14 - hes)); // Mantissa
                        if( (xm >> (13 - hes)) & 0x00000001u ) // Check for rounding
                            hm += (uint16_t) 1u; // Round, might overflow into exp bit, but this is OK
                    }
                    *hp++ = (hs | hm); // Combine sign bit and mantissa bits, biased exponent is zero
                } else {
                    he = (uint16_t) (hes << 10); // Exponent
                    hm = (uint16_t) (xm >> 13); // Mantissa
                    if( xm & 0x00001000u ) // Check for rounding
                        *hp++ = (hs | he | hm) + (uint16_t) 1u; // Round, might overflow to inf, this is OK
                    else
                        *hp++ = (hs | he | hm);  // No rounding
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Routine:  halfp2singles
//
// Input:  source = address of 16-bit data to convert
//         numel  = Number of values at that address to convert
//
// Output: target = Address of 32-bit floating point data to hold output (numel values)
//         return value = 0 if native floating point format is IEEE
//                      = 1 if native floating point format is not IEEE
//
// Programmer:  James Tursa
//
//-----------------------------------------------------------------------------

static DETEX_INLINE_ONLY void halfp2singles(void * DETEX_RESTRICT target,
void * DETEX_RESTRICT source, int numel)
{
    uint16_t *hp = (uint16_t *) source; // Type pun input as an unsigned 16-bit int
    uint32_t *xp = (uint32_t *) target; // Type pun output as an unsigned 32-bit int
    uint16_t h, hs, he, hm;
    uint32_t xs, xe, xm;
    int32_t xes;
    int e;
#if 0
    static int next;  // Little Endian adjustment
    static int checkieee = 1;  // Flag to check for IEEE754, Endian, and word size
    double one = 1.0; // Used for checking IEEE754 floating point format
    uint32_t *ip; // Used for checking IEEE754 floating point format
    
    if( checkieee ) { // 1st call, so check for IEEE754, Endian, and word size
        ip = (uint32_t *) &one;
        if( *ip ) { // If Big Endian, then no adjustment
            next = 0;
        } else { // If Little Endian, then adjustment will be necessary
            next = 1;
            ip++;
        }
        if( *ip != 0x3FF00000u ) { // Check for exact IEEE 754 bit pattern of 1.0
            return 1;  // Floating point bit pattern is not IEEE 754
        }
        if( sizeof(int16_t) != 2 || sizeof(int32_t) != 4 ) {
            return 1;  // short is not 16-bits, or long is not 32-bits.
        }
        checkieee = 0; // Everything checks out OK
    }
    
    if( source == NULL || target == NULL ) // Nothing to convert (e.g., imag part of pure real)
        return 0;
#endif
    
    while( numel-- ) {
        h = *hp++;
        if( (h & 0x7FFFu) == 0 ) {  // Signed zero
            *xp++ = ((uint32_t) h) << 16;  // Return the signed zero
        } else { // Not zero
            hs = h & 0x8000u;  // Pick off sign bit
            he = h & 0x7C00u;  // Pick off exponent bits
            hm = h & 0x03FFu;  // Pick off mantissa bits
            if( he == 0 ) {  // Denormal will convert to normalized
                e = -1; // The following loop figures out how much extra to adjust the exponent
                do {
                    e++;
                    hm <<= 1;
                } while( (hm & 0x0400u) == 0 ); // Shift until leading bit overflows into exponent bit
                xs = ((uint32_t) hs) << 16; // Sign bit
                xes = ((int32_t) (he >> 10)) - 15 + 127 - e; // Exponent unbias the halfp, then bias the single
                xe = (uint32_t) (xes << 23); // Exponent
                xm = ((uint32_t) (hm & 0x03FFu)) << 13; // Mantissa
                *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
            } else if( he == 0x7C00u ) {  // Inf or NaN (all the exponent bits are set)
                if( hm == 0 ) { // If mantissa is zero ...
                    *xp++ = (((uint32_t) hs) << 16) | ((uint32_t) 0x7F800000u); // Signed Inf
                } else {
                    *xp++ = (uint32_t) 0xFFC00000u; // NaN, only 1st mantissa bit set
                }
            } else { // Normalized number
                xs = ((uint32_t) hs) << 16; // Sign bit
                xes = ((int32_t) (he >> 10)) - 15 + 127; // Exponent unbias the halfp, then bias the single
                xe = (uint32_t) (xes << 23); // Exponent
                xm = ((uint32_t) hm) << 13; // Mantissa
                *xp++ = (xs | xe | xm); // Combine sign bit, exponent bits, and mantissa bits
            }
        }
    }
}

// Precalculated half-float table management.

float *detex_half_float_table = NULL;

static void detexCalculateHalfFloatTable() {
	detex_half_float_table = (float *)malloc(65536 * sizeof(float));
	uint16_t *hf_buffer = (uint16_t *)malloc(65536 * sizeof(uint16_t));
	for (int i = 0; i <= 0xFFFF; i++)
		hf_buffer[i] = i;
	halfp2singles(detex_half_float_table, hf_buffer, 65536);
	free(hf_buffer);
}

static pthread_mutex_t mutex_half_float_table = PTHREAD_MUTEX_INITIALIZER;

void detexValidateHalfFloatTable() {
	pthread_mutex_lock(&mutex_half_float_table);
	if (detex_half_float_table == NULL)
		detexCalculateHalfFloatTable();
	pthread_mutex_unlock(&mutex_half_float_table);
}

// Conversion functions.

void detexConvertHalfFloatToFloat(uint16_t *source_buffer, int n, float *target_buffer) {
	detexValidateHalfFloatTable();
	for (int i = 0; i < n; i++)
		target_buffer[i] = detexGetFloatFromHalfFloat(source_buffer[i]);
}
 
void detexConvertFloatToHalfFloat(float *source_buffer, int n, uint16_t *target_buffer) {
	singles2halfp(target_buffer, source_buffer, n);
}

// Convert normalized half floats to unsigned 16-bit integers in place.
void detexConvertNormalizedHalfFloatToUInt16(uint16_t *buffer, int n) {
	detexValidateHalfFloatTable();
	fesetround(FE_DOWNWARD);
	for (int i = 0; i < n; i++) {
		float f = detexGetFloatFromHalfFloat(buffer[i]);
		int u = lrintf(detexClamp0To1(f) * 65535.0f + 0.5f);
		buffer[i] = (uint16_t)u;
	}
}

// Convert normalized floats to unsigned 16-bit integers.
void detexConvertNormalizedFloatToUInt16(float * DETEX_RESTRICT source_buffer, int n,
uint16_t * DETEX_RESTRICT target_buffer) {
	fesetround(FE_DOWNWARD);
	for (int i = 0; i < n; i++) {
		int u = lrintf(detexClamp0To1(source_buffer[i]) * 65535.0f + 0.5f);
		target_buffer[i] = (uint16_t)u;
	}
}

