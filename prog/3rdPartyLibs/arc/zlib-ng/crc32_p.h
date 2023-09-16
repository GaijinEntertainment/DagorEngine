#ifndef CRC32_P_H_
#define CRC32_P_H_

#include "zbuild.h"
#include "zendian.h"

#define GF2_DIM 32      /* dimension of GF(2) vectors (length of CRC) */


static inline uint32_t gf2_matrix_times(const uint32_t *mat, uint32_t vec) {
    uint32_t sum = 0;
    while (vec) {
        if (vec & 1)
            sum ^= *mat;
        vec >>= 1;
        mat++;
    }
    return sum;
}


extern uint32_t crc32_byfour(uint32_t, const unsigned char *, uint64_t);

#endif /* CRC32_P_H_ */
