/* Copyright 1994-2013 The MathWorks, Inc.
 *
 * File: rt_matrixlib_sgl.c
 *
 * Abstract:
 *      Simulink Coder utility functions
 *
 */

#include <math.h>
#include "rt_matrixlib.h"

#ifdef CREAL_T
void rt_ComplexTimes_Sgl(creal32_T* c,
                         const creal32_T a,
                         const creal32_T b)
{
    if (a.im == 0.0F) {
        c->re = a.re * b.re;
        c->im = a.re * b.im;
        if (b.im == 0.0F) {
            c->im = 0.0F;
        } else if (b.re == 0.0F || a.re == 0.0F) {
            c->re = 0.0F;
        }
    } else if (b.im == 0.0F) {
        c->re = a.re * b.re;
        c->im = a.im * b.re;
        if (b.re == 0.0F || a.re == 0.0F) { 
            c->re = 0.0F;
        }
    } else if (a.re == 0.0F) {
        c->re = -a.im * b.im;
        c->im = a.im * b.re;
        if (b.re == 0.0F) {
            c->im = 0.0F;
        }
    } else if (b.re == 0.0F) {
        c->re = -a.im * b.im;
        c->im = a.re * b.im;
    } else {
        c->re = a.re * b.re - a.im * b.im;
        c->im = a.re * b.im + a.im * b.re;
    }
}

void rt_ComplexRDivide_Sgl(creal32_T* c,
                           const creal32_T a,
                           const creal32_T b)
{
    if (b.im == 0.0F) {
        c->re = a.re / b.re;
        if (a.im == 0.0F) {
            c->im = 0.0F;
        } else { /* a.im != 0.0F */
            c->im = a.im / b.re;
            if (a.re == 0.0F) {
                c->re = 0.0F;
            }
        }
    } else if (b.re == 0.0F) { /* b.im != 0.0F */
        c->re = a.im / b.im;
        c->im = -a.re / b.im;
        if (a.re == 0.0F) {
            c->im = 0.0F;
        } else if (a.im == 0.0F) {
            c->re = 0.0F;
        }
    } else {
        real32_T brm = (real32_T)fabs(b.re);
        real32_T bim = (real32_T)fabs(b.im);
        if (brm > bim) {
            real32_T s = b.im / b.re;
            real32_T d = b.re + s * b.im;
            c->im = (a.im - s * a.re) / d;
            c->re = (a.re + s * a.im) / d;
        } else if (brm == bim) {
            real32_T half = 0.5F;
            real32_T sgnbr = b.re > 0.0F ? half : -half;
            real32_T sgnbi = b.im > 0.0F ? half : -half;
            c->im = (a.im*sgnbr - a.re*sgnbi)/brm;
            c->re = (a.re*sgnbr + a.im*sgnbi)/brm;
        } else {
            real32_T s = b.re / b.im;
            real32_T d = b.im + s * b.re;
            c->im = (s * a.im - a.re) / d;
            c->re = (s * a.re + a.im) / d;
        }
    }
}

void rt_ComplexReciprocal_Sgl(creal32_T* Out,
                              const creal32_T In1)
{
    creal32_T y;
    y.re = 1.0F;
    y.im = 0.0F;

    rt_ComplexRDivide_Sgl(Out, y, In1);
}

#endif

real32_T rt_Hypot_Sgl(real32_T a, real32_T b)
{
    real32_T y;
    if (a != a || b != b) {
        y = a + b;
    } else {
        real32_T t;
        if ((real32_T)fabs(a) > (real32_T)fabs(b)) {
            t = b/a;
            y = (real32_T)fabs(a)*(real32_T)sqrt(1.0F + t*t);
        } else {
            if (b == 0.0F) {
                y = 0.0F;
            } else {
                t = a/b;
                y = (real32_T)fabs(b)*(real32_T)sqrt(1.0F + t*t);
            }
        }
    }
    
    return y;
}

