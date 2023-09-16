/* is01r.f -- translated by f2c (version 20000817).
   You must link the resulting object file with the libraries:
        -lf2c -lm   (in that order)
*/

#include "f2c.h"

/* Table of constant values */

static integer c__1 = 1;

/* Subroutine */ int is01r_c(real *x, real *f, real *df, integer *nx, real *sm,
         real *y, real *c__, real *rab, integer *ierr)
{
    /* Initialized data */

    static real zero = 0.f;
    static real one = 1.f;
    static struct {
        real e_1;
        } equiv_0 = { .3333333f };

    static struct {
        real e_1;
        } equiv_1 = { .6666667f };

    /* System generated locals */
    integer i__1;

    /* Builtin functions */
    double __cdecl sqrt(doublereal);

    /* Local variables */
#define oned3 ((real *)&equiv_0)
#define twod3 ((real *)&equiv_1)
    static real e, g, h__;
    static integer i__, j;
#define sys037 ((real *)&equiv_0)
#define sys038 ((real *)&equiv_1)
    static real p, onedh;
    static real f2;
    static integer m2;
    static real ff;
    static integer np1, np3;
    static real hmg;

#define c___ref(a_1,a_2) c__[(a_2)*3 + a_1]
#define rab_ref(a_1,a_2) rab[(a_2)*7 + a_1]

    /* Parameter adjustments */
    rab -= 8;
    c__ -= 4;
    --y;
    --df;
    --f;
    --x;

    /* Function Body */
    *ierr = 0;
    if (*nx < 2) {
        goto L13;
    }
    m2 = *nx + 2;
    np1 = *nx + 1;
    rab_ref(1, 1) = zero;
    rab_ref(1, 2) = zero;
    rab_ref(2, np1) = zero;
    rab_ref(3, m2) = zero;
    rab_ref(3, np1) = zero;
    rab_ref(6, 1) = zero;
    rab_ref(6, 2) = zero;
    rab_ref(6, m2) = zero;
    rab_ref(6, np1) = zero;
    p = zero;
    h__ = x[2] - x[1];
    if (h__ <= zero) {
        goto L14;
    }
    f2 = -(*sm);
    ff = (f[2] - f[1]) / h__;
    if (*nx < 3) {
        goto L6;
    }
    i__1 = *nx;
    for (i__ = 3; i__ <= i__1; ++i__) {
        g = h__;
        h__ = x[i__] - x[i__ - 1];
        if (h__ <= zero) {
            goto L14;
        }
        onedh = one / h__;
        e = ff;
        ff = (f[i__] - f[i__ - 1]) * onedh;
        y[i__] = ff - e;
        rab_ref(4, i__) = (g + h__) * *twod3;
        rab_ref(5, i__) = h__ * *oned3;
        rab_ref(3, i__) = df[i__ - 2] / g;
        rab_ref(1, i__) = df[i__] * onedh;
        rab_ref(2, i__) = -df[i__ - 1] / g - df[i__ - 1] * onedh;
/* L1: */
    }
    i__1 = *nx;
    for (i__ = 3; i__ <= i__1; ++i__) {
        c___ref(1, i__ - 1) = rab_ref(1, i__) * rab_ref(1, i__) +
                rab_ref(2, i__) * rab_ref(2, i__) + rab_ref(3, i__) *
                rab_ref(3, i__);
        c___ref(2, i__ - 1) = rab_ref(1, i__) * rab_ref(2, i__ + 1) +
                rab_ref(2, i__) * rab_ref(3, i__ + 1);
        c___ref(3, i__ - 1) = rab_ref(1, i__) * rab_ref(3, i__ + 2);
/* L2: */
    }
L3:
    if (*nx < 3) {
        goto L6;
    }
    i__1 = *nx;
    for (i__ = 3; i__ <= i__1; ++i__) {
        rab_ref(2, i__ - 1) = ff * rab_ref(1, i__ - 1);
        rab_ref(3, i__ - 2) = g * rab_ref(1, i__ - 2);
        rab_ref(1, i__) = one / (p * c___ref(1, i__ - 1) +
        rab_ref(4, i__) - ff * rab_ref(2, i__ - 1) - g * rab_ref(3, i__ - 2));
        rab_ref(6, i__) = y[i__] - rab_ref(2, i__ - 1) * rab_ref(6, i__ - 1) 
                - rab_ref(3, i__ - 2) * rab_ref(6, i__ - 2);
        ff = p * c___ref(2, i__ - 1) + rab_ref(5, i__) -
                h__ * rab_ref(2, i__ - 1);
        g = h__;
        h__ = c___ref(3, i__ - 1) * p;
/* L4: */
    }
    np3 = *nx + 3;
    i__1 = *nx;
    for (i__ = 3; i__ <= i__1; ++i__) {
        j = np3 - i__;
        rab_ref(6, j) = rab_ref(1, j) * rab_ref(6, j) - rab_ref(2, j) * 
                rab_ref(6, j + 1) - rab_ref(3, j) * rab_ref(6, j + 2);
/* L5: */
    }
L6:
    e = zero;
    h__ = zero;
    i__1 = *nx;
    for (i__ = 2; i__ <= i__1; ++i__) {
        g = h__;
        h__ = (rab_ref(6, i__ + 1) - rab_ref(6, i__)) / (x[i__] - x[i__ - 1]);
        hmg = h__ - g;
        rab_ref(7, i__) = hmg * df[i__ - 1] * df[i__ - 1];
        e += rab_ref(7, i__) * hmg;
/* L7: */
    }
    g = -h__ * df[*nx] * df[*nx];
    rab_ref(7, np1) = g;
    e -= g * h__;
    g = f2;
    f2 = e * p * p;
    if (f2 >= *sm || f2 <= g) {
        goto L10;
    }
    ff = zero;
    h__ = (rab_ref(7, 3) - rab_ref(7, 2)) / (x[2] - x[1]);
    if (*nx < 3) {
        goto L9;
    }
    i__1 = *nx;
    for (i__ = 3; i__ <= i__1; ++i__) {
        g = h__;
        h__ = (rab_ref(7, i__ + 1) - rab_ref(7, i__)) / (x[i__] - x[i__ - 1]);
        g = h__ - g - rab_ref(2, i__ - 1) * rab_ref(1, i__ - 1) -
                rab_ref(3, i__ - 2) * rab_ref(1, i__ - 2);
        ff += g * rab_ref(1, i__) * g;
        rab_ref(1, i__) = g;
/* L8: */
    }
L9:
    h__ = e - p * ff;
    if (h__ <= zero) {
        goto L10;
    }
    p += (*sm - f2) / (((real)sqrt((real)(*sm / e)) + p) * h__);
    goto L3;
L10:
    np1 = *nx - 1;
    i__1 = np1;
    for (i__ = 1; i__ <= i__1; ++i__) {
        y[i__] = f[i__] - p * rab_ref(7, i__ + 1);
        c___ref(2, i__) = rab_ref(6, i__ + 1);
        rab_ref(1, i__) = y[i__];
/* L11: */
    }
    rab_ref(1, *nx) = f[*nx] - p * rab_ref(7, *nx + 1);
    y[*nx] = rab_ref(1, *nx);
    i__1 = *nx;
    for (i__ = 2; i__ <= i__1; ++i__) {
        h__ = x[i__] - x[i__ - 1];
        c___ref(3, i__ - 1) = (rab_ref(6, i__ + 1) - c___ref(2, i__ - 1)) /
                (h__ + h__ + h__);
        c___ref(1, i__ - 1) = (rab_ref(1, i__) - y[i__ - 1]) / h__ -
                (h__ * c___ref(3, i__ - 1) + c___ref(2, i__ - 1)) * h__;
/* L12: */
    }
    goto L16;
L13:
    *ierr = 65;
    goto L15;
L14:
    *ierr = 66;
L15:
L16:
    return 0;
} /* is01r_c */

#undef rab_ref
#undef c___ref
#undef sys038
#undef sys037
#undef twod3
#undef oned3
