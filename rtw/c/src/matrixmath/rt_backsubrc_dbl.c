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

/* Function: rt_BackwardSubstitutionRC_Dbl =====================================
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
void rt_BackwardSubstitutionRC_Dbl(real_T          *pU,
                                   const creal_T   *pb,
                                   creal_T         *x,
                                   int_T            N,
                                   int_T            P,
                                   boolean_T        unit_upper)
{
  int_T i,k;
  for(k=P; k>0; k--) {
    real_T *pUcol = pU;
    for(i=0; i<N; i++) {
      creal_T *xj = x + k*N-1;
      creal_T s = {0.0, 0.0};
      real_T *pUrow = pUcol--;          /* access current row of U */
      creal_T cb;

      {
        int_T j = i;
        while(j-- > 0) {
          creal_T c;
          creal_T cUp;
          cUp.re = *pUrow;
          cUp.im = 0.0;
          rt_ComplexTimes_Dbl(&c, cUp, *xj);

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
        creal_T cU;
        creal_T cdiff;
        cU.re = *pUrow;
        cU.im = 0.0;
        cdiff.re = cb.re - s.re;
        cdiff.im = cb.im - s.im;

        rt_ComplexRDivide_Dbl(xj, cdiff, cU);
      }
    }
  }
}
#endif
/* [EOF] rt_backsubrc_dbl.c */
