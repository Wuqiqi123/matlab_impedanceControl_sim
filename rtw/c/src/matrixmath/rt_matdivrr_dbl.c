/* Copyright 1994-2013 The MathWorks, Inc.
 *
 * File: rt_matdivrr_dbl.c     
 *
 * Abstract:
 *      Real-Time Workshop support routine which performs
 *      matrix inversion for two real double precision float operands
 *
 */

#include <string.h>   /* needed for memcpy */
#include "rt_matrixlib.h"

/*
 * Function: rt_MatDivRR_Dbl
 * Abstract:
 *      2-real double input matrix division function
 */
void rt_MatDivRR_Dbl(real_T        *Out,
                     const real_T  *In1,
                     const real_T  *In2,
                     real_T        *lu,
                     int32_T       *piv,
                     real_T        *x,
                     const int_T    dims[3])
{
  int_T N = dims[0];
  int_T N2 = N * N;
  int_T P = dims[2];
  int_T NP = N * P;
  const boolean_T unit_upper = false;
  const boolean_T unit_lower = true;

  (void)memcpy(lu, In1, N2*sizeof(real_T));

  rt_lu_real(lu, N, piv);

  rt_ForwardSubstitutionRR_Dbl(lu, In2, x, N, P, piv, unit_lower);

  rt_BackwardSubstitutionRR_Dbl(lu + N2 -1, x + NP -1, Out, N, P, unit_upper);
}

/* [EOF] rt_matdivrr_dbl.c */
