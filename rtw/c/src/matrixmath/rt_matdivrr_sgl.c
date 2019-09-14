/* Copyright 1994-2014 The MathWorks, Inc.
 *
 * File: rt_matdivrr_sgl.c     
 *
 * Abstract:
 *      Real-Time Workshop support routine which performs
 *      matrix inversion for two real double precision float operands
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

/*
 * Function: rt_MatDivRR_Sgl
 * Abstract:
 *      2-real double input matrix division function
 */
void rt_MatDivRR_Sgl(real32_T       *Out,
                     const real32_T *In1,
                     const real32_T *In2,
                     real32_T       *lu,
                     int32_T        *piv,
                     real32_T       *x,
                     const int_T     dims[3])
{
  int_T N = dims[0];
  int_T N2 = N * N;
  int_T P = dims[2];
  int_T NP = N * P;
  const boolean_T unit_upper = false;
  const boolean_T unit_lower = true;

  (void)memcpy(lu, In1, N2*sizeof(real32_T));

  rt_lu_real_sgl(lu, N, piv);

  rt_ForwardSubstitutionRR_Sgl(lu, In2, x, N, P, piv, unit_lower);

  rt_BackwardSubstitutionRR_Sgl(lu + N2 -1, x + NP -1, Out, N, P, unit_upper);
}

/* [EOF] rt_matdivrr_dbl.c */
