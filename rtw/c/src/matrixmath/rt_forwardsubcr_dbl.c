/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_forwardsubcr_dbl.c     
 *
 * Abstract:
 *      Simulink Coder support routine which performs
 *      forward substitution: solving Lx=b
 *
 */

#include <math.h>
#include "rt_matrixlib.h"

/* Function: rt_ForwardSubstitutionCR_Dbl ======================================
 * Abstract: Forward substitution: solving Lx=b 
 *           L: Complex, double
 *           b: Real,    double
 *           L is a lower (or unit lower) triangular full matrix.
 *           The entries in the upper triangle are ignored.
 *           L is a NxN matrix
 *           X is a NxP matrix
 *           B is a NxP matrix
 */
#ifdef CREAL_T
void rt_ForwardSubstitutionCR_Dbl(creal_T       *pL,
                                  const real_T  *pb,
                                  creal_T       *x,
                                  int_T          N,
                                  int_T          P,
                                  const int32_T *piv,
                                  boolean_T      unit_lower)
{
  int_T i, k;
  for(k=0; k<P; k++) {
    creal_T *pLcol = pL;
    for(i=0; i<N; i++) {
      creal_T *xj = x + k*N;
      creal_T s = {0.0, 0.0};
      creal_T *pLrow = pLcol++;
      real_T cb;

      {
        int_T j = i;
        while(j-- > 0) {
          /* Compute: s += L * xj, in complex */
          creal_T cL = *pLrow;
          creal_T c;
          rt_ComplexTimes_Dbl(&c, cL, *xj);

          s.re += c.re;
          s.im += c.im;
          xj++;
          pLrow += N;
        }
      }

      cb = pb[piv[i]];
      if (unit_lower) {
        xj->re = cb - s.re;
        xj->im = -s.im;
      } else {
        /* Complex divide: *xj = cdiff / *cL */
        creal_T cL = *pLrow;
        creal_T cdiff;
        cdiff.re = cb - s.re;
        cdiff.im = -s.im;

        rt_ComplexRDivide_Dbl(xj, cdiff, cL);
      }
    }
    pb += N;
  }
}
#endif
/* [EOF] rt_forwardsubcr_dbl.c */
