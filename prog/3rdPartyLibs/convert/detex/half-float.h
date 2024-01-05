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

void detexConvertHalfFloatToFloat(uint16_t *source_buffer, int n, float *target_buffer);

void detexConvertFloatToHalfFloat(float *source_buffer, int n, uint16_t *target_buffer);

void detexConvertNormalizedHalfFloatToUInt16(uint16_t *buffer, int n);

void detexConvertNormalizedFloatToUInt16(float *source_buffer, int n, uint16_t *target_buffer);

extern float *detex_half_float_table;

void detexValidateHalfFloatTable(void);

static DETEX_INLINE_ONLY float detexGetFloatFromHalfFloat(uint16_t hf) {
	return detex_half_float_table[hf];
}

