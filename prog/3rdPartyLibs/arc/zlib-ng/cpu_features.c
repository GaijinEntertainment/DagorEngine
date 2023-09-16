/* cpu_features.c -- CPU architecture feature check
 * Copyright (C) 2017 Hans Kristian Rosbach
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "zbuild.h"

#include "cpu_features.h"

Z_INTERNAL void cpu_check_features(void) {
    static int features_checked = 0;
    if (features_checked)
        return;
#if defined(X86_FEATURES)
    x86_check_features();
#elif defined(ARM_FEATURES)
    arm_check_features();
#elif defined(PPC_FEATURES) || defined(POWER_FEATURES)
    power_check_features();
#elif defined(S390_FEATURES)
    s390_check_features();
#endif
    features_checked = 1;
}
