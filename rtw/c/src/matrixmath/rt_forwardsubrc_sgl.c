/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_forwardsubrc_sgl.c     
 *
 * Abstract:
 *      Simulink Coder support routine which performs
 *      forward substitution: solving Lx=b
 *
 */

#include "rt_matrixlib.h"

/* Function: rt_ForwardSubstitutionRC_Sgl =====================================
 * Abstract: Forward substitution: solving Lx=b 
 *           L: Real,    double
 *           b: Complex, double
 *           L is a lower (or unit lower) triangular full matrix.
 *           The entries in the upper triangle are ignored.
 *           L is a NxN matrix
 *           X is a NxP matrix
 *           B is a NxP matrix
 */
#ifdef CREAL_T
void rt_ForwardSubstitutionRC_Sgl(real32_T        *pL,
                                  const creal32_T *pb,
                                  creal32_T       *x,
                                  int_T            N,
                                  int_T            P,
                                  const int32_T   *piv,
                                  boolean_T        unit_lower)
{
  int_T i, k;
  for(k=0; k<P; k++) {
    real32_T *pLcol = pL;
    for(i=0; i<N; i++) {
      creal32_T *xj = x + k*N;
      creal32_T s = {0.0F, 0.0F};
      real32_T *pLrow = pLcol++;          /* access current row of L */
      creal32_T cb;

      {
        int_T j = i;
        while(j-- > 0) {
          creal32_T c;
          creal32_T cL;
          cL.re = *pLrow;
          cL.im = 0.0F;
          rt_ComplexTimes_Sgl(&c, cL, *xj);

          s.re += c.re;
          s.im += c.im;
          xj++;
          pLrow += N;
        }
      }

      cb = *(pb+piv[i]);
      if (unit_lower) {
        xj->re = cb.re - s.re;
        xj->im = cb.im - s.im;
      } else {
        creal32_T cL;
        creal32_T cdiff;
        cL.re = *pLrow;
        cL.im = 0.0F;
        cdiff.re = cb.re - s.re;
        cdiff.im = cb.im - s.im;

        rt_ComplexRDivide_Sgl(xj, cdiff, cL);
      }
    }
    pb += N;
  }
}
#endif
/* [EOF] rt_forwardsubrc_sgl.c */
