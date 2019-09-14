/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_matmultrrandinc_dbl.c     
 *
 * Abstract:
 *      Real-Time Workshop support routine for matrix multiplication
 *      and increment for two real double precision float operands
 *
 */

#include "rt_matrixlib.h"

/*
 * Function: rt_MatMultAndIncRR_Dbl
 * Abstract:
 *      2-input matrix multiply and increment function
 *      Input 1: Real, double-precision
 *      Input 2: Real, double-precision
 */
void rt_MatMultAndIncRR_Dbl(real_T       *y, 
                            const real_T *A,
                            const real_T *B, 
                            const int_T    dims[3])
{
  int_T k;
  for(k=dims[2]; k-- > 0; ) {
    const real_T *A1 = A;
    int_T i;
    for(i=dims[0]; i-- > 0; ) {
      const real_T *A2 = A1;
      const real_T *B1 = B;
      real_T acc = 0.0;
      int_T j;
      A1++;
      for(j=dims[1]; j-- > 0; ) {
        acc += *A2 * *B1;
        B1++;
        A2 += dims[0];
      }
      *y++ += acc;
    }
    B += dims[1];
  }
}

/* [EOF] rt_matmultandincrr_dbl.c */
