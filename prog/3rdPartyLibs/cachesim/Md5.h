#pragma once

/*
Copyright (c) 2017, Insomniac Games
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

typedef unsigned char md5_byte_t; /*!< 8-bit byte */
typedef unsigned int md5_word_t; /*!< 32-bit word */

/*! Define the state of the MD5 Algorithm. */
typedef struct md5_state_s
{
  md5_word_t count[2];  /*!< message length in bits, lsw first */
  md5_word_t abcd[4];   /*!< digest buffer */
  md5_byte_t buf[64];   /*!< accumulate block */
} md5_state_t;

void md5_process(md5_state_t *pms, const md5_byte_t *data /*[64]*/);
void md5_init(md5_state_t *pms);
void md5_append(md5_state_t *pms, const md5_byte_t *data, int nbytes);
void md5_finish(md5_state_t *pms, md5_byte_t digest[16]);
