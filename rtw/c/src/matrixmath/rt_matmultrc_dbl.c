/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_matmultrc_dbl.c     
 *
 * Abstract:
 *      Simulink Coder support routine for matrix multiplication.
 *      Real multiplied by complex, double precision float operands
 *
 */

#include "rt_matrixlib.h"

/*
 * Function: rt_MatMultRC_Dbl
 * Abstract:
 *      2-input matrix multiply function
 *      Input 1: Real, double-precision
 *      Input 2: Complex, double-precision
 */
#ifdef CREAL_T
void rt_MatMultRC_Dbl(creal_T       *y, 
                      const real_T  *A,
                      const creal_T *B,
                      const int_T     dims[3])
{
  int_T k;
  for(k=dims[2]; k-- > 0; ) {
    const real_T *A1 = A;
    int_T i;
    for(i=dims[0]; i-- > 0; ) {
      const real_T *A2 = A1;
      const creal_T *B1 = B;
      creal_T acc;
      int_T j;
      A1++;
      acc.re = 0.0;
      acc.im = 0.0;
      for(j=dims[1]; j-- > 0; ) {
        creal_T c;
        creal_T a1c;
        a1c.re = *A2;
        a1c.im = 0.0;
        rt_ComplexTimes_Dbl(&c, a1c, *B1);
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
/* [EOF] rt_matmultrc_dbl.c */
