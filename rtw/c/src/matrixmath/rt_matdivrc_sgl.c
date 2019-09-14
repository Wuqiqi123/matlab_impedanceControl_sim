/* Copyright 1994-2013 The MathWorks, Inc.
 *
 * File: rt_matdivrc_sgl.c     
 *
 * Abstract:
 *      Real-Time Workshop support routine which performs
 *      matrix inversion: inv(In1)*In2 using LU factorization.
 *
 */

#include <string.h>   /* needed for memcpy */
#include "rt_matrixlib.h"

/* Function: rt_MatDivRC_Sgl ===================================================
 * Abstract: 
 *           Calculate inv(In1)*In2 using LU factorization.
 */
#ifdef CREAL_T
void rt_MatDivRC_Sgl(creal32_T       *Out,
                     const real32_T  *In1,
                     const creal32_T *In2,
                     real32_T        *lu,
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

  (void)memcpy(lu, In1, N2*sizeof(real32_T));

  rt_lu_real_sgl(lu, N, piv);

  rt_ForwardSubstitutionRC_Sgl(lu, In2, x, N, P, piv, unit_lower);

  rt_BackwardSubstitutionRC_Sgl(lu + N2 -1, x + NP -1, Out, N, P, unit_upper);
}
#endif
/* [EOF] rt_matdivrc_sgl.c */
