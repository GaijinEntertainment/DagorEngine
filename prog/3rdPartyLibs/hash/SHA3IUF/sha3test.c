/* -------------------------------------------------------------------------
 * Works when compiled for either 32-bit or 64-bit targets, optimized for 
 * 64 bit.
 *
 * Canonical implementation of Init/Update/Finalize for SHA-3 byte input. 
 *
 * SHA3-256, SHA3-384, SHA-512 are implemented. SHA-224 can easily be added.
 *
 * Based on code from http://keccak.noekeon.org/ .
 *
 * I place the code that I wrote into public domain, free to use. 
 *
 * I would appreciate if you give credits to this work if you used it to 
 * write or test * your code.
 *
 * Aug 2015. Andrey Jivsov. crypto@brainhub.org
 * ---------------------------------------------------------------------- */

/* *************************** Self Tests ************************ */

/* 
 * There are two set of mutually exclusive tests, based on SHA3_USE_KECCAK,
 * which is undefined in the production version.
 *
 * Known answer tests are from NIST SHA3 test vectors at
 * http://csrc.nist.gov/groups/ST/toolkit/examples.html
 *
 * SHA3-256:
 *   http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA3-256_Msg0.pdf
 *   http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA3-256_1600.pdf
 * SHA3-384: 
 *   http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA3-384_1600.pdf 
 * SHA3-512: 
 *   http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA3-512_1600.pdf 
 *
 * These are refered to as [FIPS 202] tests.
 *
 * -----
 *
 * A few Keccak algorithm tests (when M and not M||01 is hashed) are
 * added here. These are from http://keccak.noekeon.org/KeccakKAT-3.zip,
 * ShortMsgKAT_256.txt for sizes even to 8. There is also one test for 
 * ExtremelyLongMsgKAT_256.txt.
 *
 * These will work with this code when SHA3_USE_KECCAK converts Finalize
 * to use "pure" Keccak algorithm.
 *
 *
 * These are referred to as [Keccak] test.
 *
 * -----
 *
 * In one case the input from [Keccak] test was used to test SHA3
 * implementation. In this case the calculated hash was compared with
 * the output of the sha3sum on Fedora Core 20 (which is Perl's based).
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sha3.h"

int
main()
{
    uint8_t buf[200];
    sha3_context c;
    const void *hash;
    unsigned i;
    const uint8_t c1 = 0xa3;

    /* [FIPS 202] KAT follow */
    static const uint8_t sha3_256_empty[256 / 8] = {
        0xa7, 0xff, 0xc6, 0xf8, 0xbf, 0x1e, 0xd7, 0x66,
	0x51, 0xc1, 0x47, 0x56, 0xa0, 0x61, 0xd6, 0x62,
	0xf5, 0x80, 0xff, 0x4d, 0xe4, 0x3b, 0x49, 0xfa, 
	0x82, 0xd8, 0x0a, 0x4b, 0x80, 0xf8, 0x43, 0x4a
    };
    static const uint8_t sha3_256_0xa3_200_times[256 / 8] = {
        0x79, 0xf3, 0x8a, 0xde, 0xc5, 0xc2, 0x03, 0x07,
        0xa9, 0x8e, 0xf7, 0x6e, 0x83, 0x24, 0xaf, 0xbf,
        0xd4, 0x6c, 0xfd, 0x81, 0xb2, 0x2e, 0x39, 0x73,
        0xc6, 0x5f, 0xa1, 0xbd, 0x9d, 0xe3, 0x17, 0x87
    };
    static const uint8_t sha3_384_0xa3_200_times[384 / 8] = {
        0x18, 0x81, 0xde, 0x2c, 0xa7, 0xe4, 0x1e, 0xf9,
        0x5d, 0xc4, 0x73, 0x2b, 0x8f, 0x5f, 0x00, 0x2b,
        0x18, 0x9c, 0xc1, 0xe4, 0x2b, 0x74, 0x16, 0x8e,
        0xd1, 0x73, 0x26, 0x49, 0xce, 0x1d, 0xbc, 0xdd,
        0x76, 0x19, 0x7a, 0x31, 0xfd, 0x55, 0xee, 0x98,
        0x9f, 0x2d, 0x70, 0x50, 0xdd, 0x47, 0x3e, 0x8f
    };
    static const uint8_t sha3_512_0xa3_200_times[512 / 8] = {
        0xe7, 0x6d, 0xfa, 0xd2, 0x20, 0x84, 0xa8, 0xb1,
        0x46, 0x7f, 0xcf, 0x2f, 0xfa, 0x58, 0x36, 0x1b,
        0xec, 0x76, 0x28, 0xed, 0xf5, 0xf3, 0xfd, 0xc0,
        0xe4, 0x80, 0x5d, 0xc4, 0x8c, 0xae, 0xec, 0xa8,
        0x1b, 0x7c, 0x13, 0xc3, 0x0a, 0xdf, 0x52, 0xa3,
        0x65, 0x95, 0x84, 0x73, 0x9a, 0x2d, 0xf4, 0x6b,
        0xe5, 0x89, 0xc5, 0x1c, 0xa1, 0xa4, 0xa8, 0x41,
        0x6d, 0xf6, 0x54, 0x5a, 0x1c, 0xe8, 0xba, 0x00
    };

    /* ---- "pure" Keccak algorithm begins; from [Keccak] ----- */

    sha3_HashBuffer(256, SHA3_FLAGS_KECCAK, "abc", 3, buf, sizeof(buf));
    if(memcmp(buf, "\x4e\x03\x65\x7a\xea\x45\xa9\x4f"
                   "\xc7\xd4\x7b\xa8\x26\xc8\xd6\x67"
                   "\xc0\xd1\xe6\xe3\x3a\x64\xa0\x36"
                   "\xec\x44\xf5\x8f\xa1\x2d\x6c\x45", 256 / 8) != 0) {
        printf("SHA3-256(abc) "
                "doesn't match known answer (single buffer)\n");
        return 10;
    }


    sha3_Init256(&c);
    sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);
    sha3_Update(&c, "\xcc", 1);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\xee\xad\x6d\xbf\xc7\x34\x0a\x56"
                    "\xca\xed\xc0\x44\x69\x6a\x16\x88"
                    "\x70\x54\x9a\x6a\x7f\x6f\x56\x96"
                    "\x1e\x84\xa5\x4b\xd9\x97\x0b\x8a", 256 / 8) != 0) {
        printf("SHA3-256(cc) "
                "doesn't match known answer (single buffer)\n");
        return 11;
    }

    sha3_Init256(&c);
    sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);
    sha3_Update(&c, "\x41\xfb", 2);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\xa8\xea\xce\xda\x4d\x47\xb3\x28"
                    "\x1a\x79\x5a\xd9\xe1\xea\x21\x22"
                    "\xb4\x07\xba\xf9\xaa\xbc\xb9\xe1"
                    "\x8b\x57\x17\xb7\x87\x35\x37\xd2", 256 / 8) != 0) {
        printf("SHA3-256(41fb) "
                "doesn't match known answer (single buffer)\n");
        return 12;
    }

    sha3_Init256(&c);
    sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);
    sha3_Update(&c,
            "\x52\xa6\x08\xab\x21\xcc\xdd\x8a"
            "\x44\x57\xa5\x7e\xde\x78\x21\x76", 128 / 8);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\x0e\x32\xde\xfa\x20\x71\xf0\xb5"
                    "\xac\x0e\x6a\x10\x8b\x84\x2e\xd0"
                    "\xf1\xd3\x24\x97\x12\xf5\x8e\xe0"
                    "\xdd\xf9\x56\xfe\x33\x2a\x5f\x95", 256 / 8) != 0) {
        printf("SHA3-256(52a6...76) "
                "doesn't match known answer (single buffer)\n");
        return 13;
    }

    sha3_Init256(&c);
    sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);
    sha3_Update(&c,
            "\x43\x3c\x53\x03\x13\x16\x24\xc0"
            "\x02\x1d\x86\x8a\x30\x82\x54\x75"
            "\xe8\xd0\xbd\x30\x52\xa0\x22\x18"
            "\x03\x98\xf4\xca\x44\x23\xb9\x82"
            "\x14\xb6\xbe\xaa\xc2\x1c\x88\x07"
            "\xa2\xc3\x3f\x8c\x93\xbd\x42\xb0"
            "\x92\xcc\x1b\x06\xce\xdf\x32\x24"
            "\xd5\xed\x1e\xc2\x97\x84\x44\x4f"
            "\x22\xe0\x8a\x55\xaa\x58\x54\x2b"
            "\x52\x4b\x02\xcd\x3d\x5d\x5f\x69"
            "\x07\xaf\xe7\x1c\x5d\x74\x62\x22"
            "\x4a\x3f\x9d\x9e\x53\xe7\xe0\x84" "\x6d\xcb\xb4\xce", 800 / 8);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\xce\x87\xa5\x17\x3b\xff\xd9\x23"
                    "\x99\x22\x16\x58\xf8\x01\xd4\x5c"
                    "\x29\x4d\x90\x06\xee\x9f\x3f\x9d"
                    "\x41\x9c\x8d\x42\x77\x48\xdc\x41", 256 / 8) != 0) {
        printf("SHA3-256(433C...CE) "
                "doesn't match known answer (single buffer)\n");
        return 14;
    }

    /* SHA3-256 byte-by-byte: 16777216 steps. ExtremelyLongMsgKAT_256
     * [Keccak] */
    i = 16777216;
    sha3_Init256(&c);
    sha3_SetFlags(&c, SHA3_FLAGS_KECCAK);
    while (i--) {
        sha3_Update(&c,
                "abcdefghbcdefghicdefghijdefghijk"
                "efghijklfghijklmghijklmnhijklmno", 64);
    }
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\x5f\x31\x3c\x39\x96\x3d\xcf\x79"
                    "\x2b\x54\x70\xd4\xad\xe9\xf3\xa3"
                    "\x56\xa3\xe4\x02\x17\x48\x69\x0a"
                    "\x95\x83\x72\xe2\xb0\x6f\x82\xa4", 256 / 8) != 0) {
        printf("SHA3-256( abcdefgh...[16777216 times] ) "
                "doesn't match known answer\n");
        return 15;
    }

    printf("Keccak-256 tests passed OK\n");

    /* ----- SHA3 testing begins ----- */

    /* SHA-256 on an empty buffer */
    sha3_Init256(&c);
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_256_empty, hash, sizeof(sha3_256_empty)) != 0) {
        printf("SHA3-256() doesn't match known answer\n");
        return 1;
    }

    sha3_HashBuffer(256, SHA3_FLAGS_NONE, "abc", 3, buf, sizeof(buf));
    if(memcmp(buf, 
                    "\x3a\x98\x5d\xa7\x4f\xe2\x25\xb2"
                    "\x04\x5c\x17\x2d\x6b\xd3\x90\xbd"
                    "\x85\x5f\x08\x6e\x3e\x9d\x52\x5b"
                    "\x46\xbf\xe2\x45\x11\x43\x15\x32", 256 / 8) != 0) {
        printf("SHA3-256(abc) "
                "doesn't match known answer (single buffer)\n");
        return 10;
    }

    memset(buf, c1, sizeof(buf));       /* set to value c1 */

    /* SHA3-256 as a single buffer. [FIPS 202] */
    sha3_Init256(&c);
    sha3_Update(&c, buf, sizeof(buf));
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_256_0xa3_200_times, hash,
                    sizeof(sha3_256_0xa3_200_times)) != 0) {
        printf("SHA3-256( 0xa3 ... [200 times] ) "
                "doesn't match known answer (1 buffer)\n");
        return 1;
    }

    /* SHA3-256 in two steps. [FIPS 202] */
    sha3_Init256(&c);
    sha3_Update(&c, buf, sizeof(buf) / 2);
    sha3_Update(&c, buf + sizeof(buf) / 2, sizeof(buf) / 2);
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_256_0xa3_200_times, hash,
                    sizeof(sha3_256_0xa3_200_times)) != 0) {
        printf("SHA3-256( 0xa3 ... [200 times] ) "
                "doesn't match known answer (2 steps)\n");
        return 2;
    }

    /* SHA3-256 byte-by-byte: 200 steps. [FIPS 202] */
    i = 200;
    sha3_Init256(&c);
    while (i--) {
        sha3_Update(&c, &c1, 1);
    }
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_256_0xa3_200_times, hash,
                    sizeof(sha3_256_0xa3_200_times)) != 0) {
        printf("SHA3-256( 0xa3 ... [200 times] ) "
                "doesn't match known answer (200 steps)\n");
        return 3;
    }

    /* SHA3-256 byte-by-byte: 135 bytes. Input from [Keccak]. Output
     * matched with sha3sum. */
    sha3_Init256(&c);
    sha3_Update(&c,
            "\xb7\x71\xd5\xce\xf5\xd1\xa4\x1a"
            "\x93\xd1\x56\x43\xd7\x18\x1d\x2a"
            "\x2e\xf0\xa8\xe8\x4d\x91\x81\x2f"
            "\x20\xed\x21\xf1\x47\xbe\xf7\x32"
            "\xbf\x3a\x60\xef\x40\x67\xc3\x73"
            "\x4b\x85\xbc\x8c\xd4\x71\x78\x0f"
            "\x10\xdc\x9e\x82\x91\xb5\x83\x39"
            "\xa6\x77\xb9\x60\x21\x8f\x71\xe7"
            "\x93\xf2\x79\x7a\xea\x34\x94\x06"
            "\x51\x28\x29\x06\x5d\x37\xbb\x55"
            "\xea\x79\x6f\xa4\xf5\x6f\xd8\x89"
            "\x6b\x49\xb2\xcd\x19\xb4\x32\x15"
            "\xad\x96\x7c\x71\x2b\x24\xe5\x03"
            "\x2d\x06\x52\x32\xe0\x2c\x12\x74"
            "\x09\xd2\xed\x41\x46\xb9\xd7\x5d"
            "\x76\x3d\x52\xdb\x98\xd9\x49\xd3"
            "\xb0\xfe\xd6\xa8\x05\x2f\xbb", 1080 / 8);
    hash = sha3_Finalize(&c);
    if(memcmp(hash, "\xa1\x9e\xee\x92\xbb\x20\x97\xb6"
                    "\x4e\x82\x3d\x59\x77\x98\xaa\x18"
                    "\xbe\x9b\x7c\x73\x6b\x80\x59\xab"
                    "\xfd\x67\x79\xac\x35\xac\x81\xb5", 256 / 8) != 0) {
        printf("SHA3-256( b771 ... ) doesn't match the known answer\n");
        return 4;
    }

    /* SHA3-384 as a single buffer. [FIPS 202] */
    sha3_Init384(&c);
    sha3_Update(&c, buf, sizeof(buf));
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_384_0xa3_200_times, hash,
                    sizeof(sha3_384_0xa3_200_times)) != 0) {
        printf("SHA3-384( 0xa3 ... [200 times] ) "
                "doesn't match known answer (1 buffer)\n");
        return 5;
    }

    /* SHA3-384 in two steps. [FIPS 202] */
    sha3_Init384(&c);
    sha3_Update(&c, buf, sizeof(buf) / 2);
    sha3_Update(&c, buf + sizeof(buf) / 2, sizeof(buf) / 2);
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_384_0xa3_200_times, hash,
                    sizeof(sha3_384_0xa3_200_times)) != 0) {
        printf("SHA3-384( 0xa3 ... [200 times] ) "
                "doesn't match known answer (2 steps)\n");
        return 6;
    }

    /* SHA3-384 byte-by-byte: 200 steps. [FIPS 202] */
    i = 200;
    sha3_Init384(&c);
    while (i--) {
        sha3_Update(&c, &c1, 1);
    }
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_384_0xa3_200_times, hash,
                    sizeof(sha3_384_0xa3_200_times)) != 0) {
        printf("SHA3-384( 0xa3 ... [200 times] ) "
                "doesn't match known answer (200 steps)\n");
        return 7;
    }

    /* SHA3-512 as a single buffer. [FIPS 202] */
    sha3_Init512(&c);
    sha3_Update(&c, buf, sizeof(buf));
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_512_0xa3_200_times, hash,
                    sizeof(sha3_512_0xa3_200_times)) != 0) {
        printf("SHA3-512( 0xa3 ... [200 times] ) "
                "doesn't match known answer (1 buffer)\n");
        return 8;
    }

    /* SHA3-512 in two steps. [FIPS 202] */
    sha3_Init512(&c);
    sha3_Update(&c, buf, sizeof(buf) / 2);
    sha3_Update(&c, buf + sizeof(buf) / 2, sizeof(buf) / 2);
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_512_0xa3_200_times, hash,
                    sizeof(sha3_512_0xa3_200_times)) != 0) {
        printf("SHA3-512( 0xa3 ... [200 times] ) "
                "doesn't match known answer (2 steps)\n");
        return 9;
    }

    /* SHA3-512 byte-by-byte: 200 steps. [FIPS 202] */
    i = 200;
    sha3_Init512(&c);
    while (i--) {
        sha3_Update(&c, &c1, 1);
    }
    hash = sha3_Finalize(&c);
    if(memcmp(sha3_512_0xa3_200_times, hash,
                    sizeof(sha3_512_0xa3_200_times)) != 0) {
        printf("SHA3-512( 0xa3 ... [200 times] ) "
                "doesn't match known answer (200 steps)\n");
        return 10;
    }

    printf("SHA3-256, SHA3-384, SHA3-512 tests passed OK\n");

    return 0;
}
