/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_matmultrc_sgl.c     
 *
 * Abstract:
 *      Simulink Coder support routine for matrix multiplication.
 *      Real multiplied by complex, single precision float operands
 *
 */

#include "rt_matrixlib.h"

/*
 * Function: rt_MatMultRC_Sgl
 * Abstract:
 *      2-input matrix multiply function
 *      Input 1: Real, single-precision
 *      Input 2: Complex, single-precision
 */
#ifdef CREAL_T
void rt_MatMultRC_Sgl(creal32_T       *y,
                      const real32_T  *A,
                      const creal32_T *B,
                      const int_T       dims[3])
{
  int_T k;
  for(k=dims[2]; k-- > 0; ) {
    const real32_T *A1 = A;
    int_T i;
    for(i=dims[0]; i-- > 0; ) {
      const real32_T *A2 = A1;
      const creal32_T *B1 = B;
      creal32_T acc;
      int_T j;
      A1++;
      acc.re = 0.0F;
      acc.im = 0.0F;
      for(j=dims[1]; j-- > 0; ) {
        creal32_T c;
        creal32_T a1c;
        a1c.re = *A2;
        a1c.im = 0.0F;
        rt_ComplexTimes_Sgl(&c, a1c, *B1);
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
/* [EOF] rt_matmultrc_sgl.c */
