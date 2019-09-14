/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_matmultandinccc_sgl.c     
 *
 * Abstract:
 *      Real-Time Workshop support routine for matrix multiplication
 *      and increment for two complex single precision float operands
 *
 */

#include "rt_matrixlib.h"

/*
 * Function: rt_MatMultAndIncCC_Sgl
 * Abstract:
 *      2-input matrix multiply and increment function
 *      Input 1: Complex, single-precision
 *      Input 2: Complex, single-precision
 */
#ifdef CREAL_T
void rt_MatMultAndIncCC_Sgl(creal32_T       *y,
                            const creal32_T *A,
                            const creal32_T *B,
                            const int_T       dims[3])
{
  int_T k;
  for(k=dims[2]; k-- > 0; ) {
    const creal32_T *A1 = A;
    int_T i;
    for(i=dims[0]; i-- > 0; ) {
      const creal32_T *A2 = A1;
      const creal32_T *B1 = B;
      creal32_T acc;
      int_T j;
      A1++;
      acc.re = (real32_T)0.0;
      acc.im = (real32_T)0.0;
      for(j=dims[1]; j-- > 0; ) {
        creal32_T c;
        rt_ComplexTimes_Sgl(&c, *A2, *B1);
        acc.re += c.re;
        acc.im += c.im;
        B1++;
        A2 += dims[0];
      }
      y->re += acc.re;
      y->im += acc.im;
      y++;
    }
    B += dims[1];
  }
}
#endif
/* [EOF] rt_matmultandinccc_sgl.c */
