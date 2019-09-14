/* Copyright 1994-2013 The MathWorks, Inc.
 *
 * File: rt_matdivcc_dbl.c     
 *
 * Abstract:
 *      Real-Time Workshop support routine which performs
 *      matrix inversion: inv(In1)*In2 using LU factorization.
 *
 */

#include <string.h>   /* needed for memcpy */
#include "rt_matrixlib.h"

/* Function: rt_MatDivCC_Sgl ===================================================
 * Abstract: 
 *           Calculate inv(In1)*In2 using LU factorization.
 */
#ifdef CREAL_T
void rt_MatDivCC_Sgl(creal32_T       *Out,
                     const creal32_T *In1,
                     const creal32_T *In2,
                     creal32_T       *lu,
                     int32_T         *piv,
                     creal32_T       *x,
                     const int_T      dims[3])
{
  int_T N = dims[0];
  int_T N2 = N * N;
  int_T P = dims[2];
  int_T NP = N * P;
  const boolean_T unit_upper = false;
  const boolean_T unit_lower = true;

  (void)memcpy(lu, In1, N2*sizeof(real32_T)*2);

  rt_lu_cplx_sgl(lu, N, piv);

  rt_ForwardSubstitutionCC_Sgl(lu, In2, x, N, P, piv,unit_lower);

  rt_BackwardSubstitutionCC_Sgl(lu + N2 -1, x + NP -1, Out, N, P, unit_upper);
}
#endif
/* [EOF] rt_matdivcc_dbl.c */
