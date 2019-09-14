/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_matmultcr_dbl.c     
 *
 * Abstract:
 *      Simulink Coder support routine for matrix multiplication.
 *      Complex muliplied by real, double precision float operands
 *
 */

#include "rt_matrixlib.h"

/*
 * Function: rt_MatMultCR_Dbl
 * Abstract:
 *      2-input matrix multiply function
 *      Input 1: Complex, double-precision
 *      Input 2: Real, double-precision
 */
#ifdef CREAL_T
void rt_MatMultCR_Dbl(creal_T       *y,
                      const creal_T *A,
                      const real_T  *B,
                      const int_T     dims[3])
{
  int_T k;
  for(k=dims[2]; k-- > 0; ) {
    const creal_T *A1 = A;
    int_T i;
    for(i=dims[0]; i-- > 0; ) {
      const creal_T *A2 = A1;
      const real_T *B1 = B;
      creal_T acc;
      int_T j;
      A1++;
      acc.re = 0.0;
      acc.im = 0.0;
      for(j=dims[1]; j-- > 0; ) {
        creal_T c;
        creal_T b1c;
        b1c.re = *B1;
        b1c.im = 0.0;
        rt_ComplexTimes_Dbl(&c, *A2, b1c);
        acc.re += c.re;
        acc.im += c.im;
        B1++;
        A2 += dims[0];
      }
      *y++ = acc;
    }
    B += dims[1];
  }
}
#endif
/* [EOF] rt_matmultcr_dbl.c */
