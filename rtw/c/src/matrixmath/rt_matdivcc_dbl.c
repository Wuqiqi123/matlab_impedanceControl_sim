/* Copyright 1994-2014 The MathWorks, Inc.
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

/* Logical definitions */
#if (!defined(__cplusplus))
#  ifndef false
#   define false                       (0U)
#  endif
#  ifndef true
#   define true                        (1U)
#  endif
#endif

/* Function: rt_MatDivCC_Dbl ===================================================
 * Abstract: 
 *           Calculate inv(In1)*In2 using LU factorization.
 */
#ifdef CREAL_T
void rt_MatDivCC_Dbl(creal_T       *Out,
                     const creal_T *In1,
                     const creal_T *In2,
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

  rt_ForwardSubstitutionCC_Dbl(lu, In2, x, N, P, piv,unit_lower);

  rt_BackwardSubstitutionCC_Dbl(lu + N2 -1, x + NP -1, Out, N, P, unit_upper);
}
#endif
/* [EOF] rt_matdivcc_dbl.c */
