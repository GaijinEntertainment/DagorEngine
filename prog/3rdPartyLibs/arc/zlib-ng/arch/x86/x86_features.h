/* x86_features.h -- check for CPU features
* Copyright (C) 2013 Intel Corporation Jim Kukunas
* For conditions of distribution and use, see copyright notice in zlib.h
*/

#ifndef X86_FEATURES_H_
#define X86_FEATURES_H_

extern int x86_cpu_has_avx2;
extern int x86_cpu_has_avx512;
extern int x86_cpu_has_avx512vnni;
extern int x86_cpu_has_sse2;
extern int x86_cpu_has_ssse3;
extern int x86_cpu_has_sse41;
extern int x86_cpu_has_sse42;
extern int x86_cpu_has_pclmulqdq;
extern int x86_cpu_has_vpclmulqdq;
extern int x86_cpu_has_tzcnt;

void Z_INTERNAL x86_check_features(void);

#endif /* CPU_H_ */
