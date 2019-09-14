/* Copyright 1994-2013 The MathWorks, Inc.
 *
 * File: rt_matdivcr_dbl.c     
 *
 * Abstract:
 *      Real-Time Workshop support routine which performs
 *      matrix inversion: Inv(In1)*In2 using LU factorization.
 *
 */

#include <string.h>   /* needed for memcpy */
#include "rt_matrixlib.h"

/* Function: rt_MatDivCR_Dbl ===================================================
 * Abstract: 
 *           Calculate Inv(In1)*In2 using LU factorization.
 */
#ifdef CREAL_T
void rt_MatDivCR_Dbl(creal_T       *Out,
                     const creal_T *In1,
                     const real_T  *In2,
                     creal_T       *lu,
                     int32_T       *piv,
                     creal_T       *x,
                     const int_T    dims[3])
{
  int_T N = dims[0];
  int_T N2 = N * N;
  int_T P = dims[2];
  int_T NP = N * P;
  const boolean_T unit_upper = false;
  const boolean_T unit_lower = true;

  (void)memcpy(lu, In1, N2*sizeof(real_T)*2);

  rt_lu_cplx(lu, N, piv);

  rt_ForwardSubstitutionCR_Dbl(lu, In2, x, N, P, piv, unit_lower);

  rt_BackwardSubstitutionCC_Dbl(lu + N2 -1, x + NP -1, Out, N, P, unit_upper);
}
#endif
/* [EOF] rt_matdivcr_dbl.c */
