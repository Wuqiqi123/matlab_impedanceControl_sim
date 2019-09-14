/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_matmultrr_sgl.c     
 *
 * Abstract:
 *      Simulink Coder support routine for matrix multiplication.
 *      Complex multiplied by real, single precision float operands
 *
 */

#include "rt_matrixlib.h"

/*
 * Function: rt_MatMultCR_Sgl
 * Abstract:
 *      2-input matrix multiply function
 *      Input 1: Complex, single-precision
 *      Input 2: Real, single-precision
 */
#ifdef CREAL_T
void rt_MatMultCR_Sgl(creal32_T       *y,
                      const creal32_T *A,
                      const real32_T  *B,
                      const int_T       dims[3])
{
  int_T k;
  for(k=dims[2]; k-- > 0; ) {
    const creal32_T *A1 = A;
    int_T i;
    for(i=dims[0]; i-- > 0; ) {
      const creal32_T *A2 = A1;
      const real32_T *B1 = B;
      creal32_T acc;
      int_T j;
      A1++;
      acc.re = 0.0F;
      acc.im = 0.0F;
      for(j=dims[1]; j-- > 0; ) {
        creal32_T c;
        creal32_T b1c;
        b1c.re = *B1;
        b1c.im = 0.0F;
        rt_ComplexTimes_Sgl(&c, *A2, b1c);
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
/* [EOF] rt_matmultcr_sgl.c */
