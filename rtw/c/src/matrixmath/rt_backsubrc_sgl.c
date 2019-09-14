/* Copyright 1994-2010 The MathWorks, Inc.
 *
 * File: rt_backsubrc_dbl.c     
 *
 * Abstract:
 *      Simulink Coder support routine which performs
 *      backward substitution: solving Ux=b
 *
 */

#include "rt_matrixlib.h"

/* Function: rt_BackwardSubstitutionRC_Sgl =====================================
 * Abstract: Backward substitution: Solving Ux=b 
 *           U: real, double
 *           b: complex, double
 *           U is an upper (or unit upper) triangular full matrix.
 *           The entries in the lower triangle are ignored.
 *           U is a NxN matrix
 *           X is a NxP matrix
 *           B is a NxP matrix
 */
#ifdef CREAL_T
void rt_BackwardSubstitutionRC_Sgl(real32_T          *pU,
                                   const creal32_T   *pb,
                                   creal32_T         *x,
                                   int_T              N,
                                   int_T              P,
                                   boolean_T          unit_upper)
{
  int_T i,k;
  for(k=P; k>0; k--) {
    real32_T *pUcol = pU;
    for(i=0; i<N; i++) {
      creal32_T *xj = x + k*N-1;
      creal32_T s = {0.0F, 0.0F};
      real32_T *pUrow = pUcol--;          /* access current row of U */
      creal32_T cb;

      {
        int_T j = i;
        while(j-- > 0) {
          creal32_T c;
          creal32_T cUp;
          cUp.re = *pUrow;
          cUp.im = 0.0F;
          rt_ComplexTimes_Sgl(&c, cUp, *xj);

          s.re += c.re;
          s.im += c.im;
          xj--;
          pUrow -= N;
        }
      }

      cb = (*pb--);
      if (unit_upper) {
        xj->re = cb.re - s.re;
        xj->im = cb.im - s.im;
      } else {
        creal32_T cU;
        creal32_T cdiff;
        cU.re = *pUrow;
        cU.im = 0.0F;
        cdiff.re = cb.re - s.re;
        cdiff.im = cb.im - s.im;

        rt_ComplexRDivide_Sgl(xj, cdiff, cU);
      }
    }
  }
}
#endif
/* [EOF] rt_backsubrc_dbl.c */
