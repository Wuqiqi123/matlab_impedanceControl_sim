/*
 * Copyright 1994-2009 The MathWorks, Inc.
 *
 * File: ode5.c        
 *
 */

#include <math.h>
#include <string.h>
#include "tmwtypes.h"
#ifdef USE_RTMODEL
# include "simstruc_types.h"
#else
# include "simstruc.h"
#endif
#include "odesup.h"

#define MAT13 {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}
#define TWODMAT13 {MAT13,MAT13,MAT13,MAT13,MAT13,MAT13,MAT13,MAT13,MAT13,MAT13,MAT13,MAT13,MAT13}
#define NSTAGES 13
#define ODE8_CONSTANTS  rt_ODE8_C[1] = 5.555555555555556E-2; \
						rt_ODE8_C[2] = 8.333333333333333E-2; \
						rt_ODE8_C[3] = 1.25E-1; \
						rt_ODE8_C[4] = 3.125E-1; \
						rt_ODE8_C[5] = 3.75E-1; \
						rt_ODE8_C[6] = 1.475E-1; \
						rt_ODE8_C[7] = 4.65E-1; \
						rt_ODE8_C[8] = 5.648654513822596E-1; \
						rt_ODE8_C[9] = 6.5E-1; \
						rt_ODE8_C[10] = 9.246562776405044E-1; \
						rt_ODE8_C[11] = 1.0; \
						rt_ODE8_C[12] = 1.0; \
						rt_ODE8_A[1][0] = 5.555555555555556E-2; \
						rt_ODE8_A[2][0] = 2.083333333333333E-2; \
						rt_ODE8_A[2][1] = 6.25E-2; \
						rt_ODE8_A[3][0] = 3.125E-2; \
						rt_ODE8_A[3][2] = 9.375E-2; \
						rt_ODE8_A[4][0] = 3.125E-1; \
						rt_ODE8_A[4][2] = -1.171875; \
						rt_ODE8_A[4][3] = 1.171875; \
						rt_ODE8_A[5][0] = 3.75E-2; \
						rt_ODE8_A[5][3] = 1.875E-1; \
						rt_ODE8_A[5][4] = 1.5E-1; \
						rt_ODE8_A[6][0] = 4.791013711111111E-2; \
						rt_ODE8_A[6][3] = 1.122487127777778E-1; \
						rt_ODE8_A[6][4] = -2.550567377777778E-2; \
						rt_ODE8_A[6][5] = 1.284682388888889E-2; \
						rt_ODE8_A[7][0] = 1.691798978729228E-2; \
						rt_ODE8_A[7][3] = 3.878482784860432E-1; \
						rt_ODE8_A[7][4] = 3.597736985150033E-2; \
						rt_ODE8_A[7][5] = 1.969702142156661E-1; \
						rt_ODE8_A[7][6] = -1.727138523405018E-1; \
						rt_ODE8_A[8][0] = 6.90957533591923E-2; \
						rt_ODE8_A[8][3] = -6.342479767288542E-1; \
						rt_ODE8_A[8][4] = -1.611975752246041E-1; \
						rt_ODE8_A[8][5] = 1.386503094588253E-1; \
						rt_ODE8_A[8][6] = 9.409286140357563E-1; \
						rt_ODE8_A[8][7] = 2.11636326481944E-1; \
						rt_ODE8_A[9][0] = 1.835569968390454E-1; \
						rt_ODE8_A[9][3] = -2.468768084315592; \
						rt_ODE8_A[9][4] = -2.912868878163005E-1; \
						rt_ODE8_A[9][5] = -2.647302023311738E-2; \
						rt_ODE8_A[9][6] = 2.8478387641928; \
						rt_ODE8_A[9][7] = 2.813873314698498E-1; \
						rt_ODE8_A[9][8] = 1.237448998633147E-1; \
						rt_ODE8_A[10][0] = -1.215424817395888; \
						rt_ODE8_A[10][3] = 1.667260866594577E1; \
						rt_ODE8_A[10][4] = 9.157418284168179E-1; \
						rt_ODE8_A[10][5] = -6.056605804357471; \
						rt_ODE8_A[10][6] = -1.600357359415618E1; \
						rt_ODE8_A[10][7] = 1.484930308629766E1; \
						rt_ODE8_A[10][8] = -1.337157573528985E1; \
						rt_ODE8_A[10][9] = 5.134182648179638; \
						rt_ODE8_A[11][0] = 2.588609164382643E-1; \
						rt_ODE8_A[11][3] = -4.774485785489205; \
						rt_ODE8_A[11][4] = -4.350930137770325E-1; \
						rt_ODE8_A[11][5] = -3.049483332072241; \
						rt_ODE8_A[11][6] = 5.577920039936099; \
						rt_ODE8_A[11][7] = 6.155831589861039; \
						rt_ODE8_A[11][8] = -5.062104586736938; \
						rt_ODE8_A[11][9] = 2.193926173180679; \
						rt_ODE8_A[11][10] = 1.346279986593349E-1; \
						rt_ODE8_A[12][0] = 8.224275996265075E-1; \
						rt_ODE8_A[12][3] = -1.165867325727766E1; \
						rt_ODE8_A[12][4] = -7.576221166909362E-1; \
						rt_ODE8_A[12][5] = 7.139735881595818E-1; \
						rt_ODE8_A[12][6] = 1.207577498689006E1; \
						rt_ODE8_A[12][7] = -2.127659113920403; \
						rt_ODE8_A[12][8] = 1.990166207048956; \
						rt_ODE8_A[12][9] = -2.342864715440405E-1; \
						rt_ODE8_A[12][10] = 1.758985777079423E-1; \
						rt_ODE8_B[0] = 4.174749114153025E-2; \
						rt_ODE8_B[5] = -5.54523286112393E-2; \
						rt_ODE8_B[6] = 2.393128072011801E-1; \
						rt_ODE8_B[7] = 7.03510669403443E-1; \
						rt_ODE8_B[8] = -7.597596138144609E-1; \
						rt_ODE8_B[9] = 6.605630309222863E-1; \
						rt_ODE8_B[10] = 1.581874825101233E-1; \
						rt_ODE8_B[11] = -2.381095387528628E-1; \
						rt_ODE8_B[12] = 2.5E-1;

static real_T rt_ODE8_B[13] = MAT13;

static real_T rt_ODE8_C[13] = MAT13;

static real_T rt_ODE8_A[13][13] = TWODMAT13;

typedef struct IntgData_tag {
    real_T *deltaY;
    real_T *f[13];
	real_T *x0;
} IntgData;

#ifndef RT_MALLOC
  /* statically declare data */
  static real_T   rt_ODE8_DELTAY[NCSTATES];
  static real_T   rt_ODE8_F[13][NCSTATES];
  static real_T   rt_ODE8_X0[NCSTATES];
  static IntgData rt_ODE8_IntgData = {rt_ODE8_DELTAY,
                                      {rt_ODE8_F[0],
                                       rt_ODE8_F[1],
                                       rt_ODE8_F[2],
                                       rt_ODE8_F[3],
                                       rt_ODE8_F[4],
                                       rt_ODE8_F[5],
									   rt_ODE8_F[6],
									   rt_ODE8_F[7],
									   rt_ODE8_F[8],
									   rt_ODE8_F[9],
									   rt_ODE8_F[10],
									   rt_ODE8_F[11],
									   rt_ODE8_F[12],
									  },
									  rt_ODE8_X0
									 };

  void rt_ODECreateIntegrationData(RTWSolverInfo *si)
  {
	  ODE8_CONSTANTS
      rtsiSetSolverData(si,(void *)&rt_ODE8_IntgData);
      rtsiSetSolverName(si,"ode8");
  }
#else
  /* dynamically allocated data */

  void rt_ODECreateIntegrationData(RTWSolverInfo *si)
  {
      int_T nX = rtsiGetNumContStates(si);
      IntgData *id = (IntgData *) malloc(sizeof(IntgData));
   
	  if(id == NULL) {
          rtsiSetErrorStatus(si, RT_MEMORY_ALLOCATION_ERROR);
          return;
      }
      
      id->deltaY = (real_T *) malloc(15*rtsiGetNumContStates(si) * sizeof(real_T));
      if(id->deltaY == NULL) {
          rtsiSetErrorStatus(si, RT_MEMORY_ALLOCATION_ERROR);
          return;
      }
      id->f[0]  = id->deltaY+ nX;
      id->f[1]  = id->f[0]  + nX;
      id->f[2]  = id->f[1]  + nX;
      id->f[3]  = id->f[2]  + nX;
      id->f[4]  = id->f[3]  + nX;
      id->f[5]  = id->f[4]  + nX;
	  id->f[6]  = id->f[5]  + nX;
	  id->f[7]  = id->f[6]  + nX;
	  id->f[8]  = id->f[7]  + nX;
	  id->f[9]  = id->f[8]  + nX;
	  id->f[10] = id->f[9]  + nX;
	  id->f[11] = id->f[10] + nX;
	  id->f[12] = id->f[11] + nX;
	  id->x0    = id->f[12] + nX;

	  ODE8_CONSTANTS

	  rtsiSetSolverData(si, (void *)id);
      rtsiSetSolverName(si,"ode8");
  }

  void rt_ODEDestroyIntegrationData(RTWSolverInfo *si)
  {
      IntgData *id = rtsiGetSolverData(si);
      
      if (id != NULL) {
          if (id->deltaY != NULL) {
              free(id->deltaY);
          }
          free(id);
          rtsiSetSolverData(si, NULL);
      }
  }
#endif

void rt_ODEUpdateContinuousStates(RTWSolverInfo *si)
{
    time_T    t          = rtsiGetT(si);
    time_T    tnew       = rtsiGetSolverStopTime(si);
    time_T    h          = rtsiGetStepSize(si);
    real_T    *x         = rtsiGetContStates(si);
    IntgData  *intgData  = rtsiGetSolverData(si);
    real_T    *deltaY    = intgData->deltaY;
	real_T    *x0        = intgData->x0;
	real_T*	  f[NSTAGES];
	int idx,stagesIdx,statesIdx;
	double deltaX;
    
#ifdef NCSTATES
    int_T     nXc        = NCSTATES;
#else
    int_T     nXc        = rtsiGetNumContStates(si);
#endif

	f[0]        = intgData->f[0];
    f[1]        = intgData->f[1];
    f[2]        = intgData->f[2];
    f[3]        = intgData->f[3];
    f[4]        = intgData->f[4];
    f[5]        = intgData->f[5];
	f[6]        = intgData->f[6];
	f[7]        = intgData->f[7];
	f[8]        = intgData->f[8];
	f[9]        = intgData->f[9];
	f[10]       = intgData->f[10];
	f[11]       = intgData->f[11];
	f[12]       = intgData->f[12];

    rtsiSetSimTimeStep(si,MINOR_TIME_STEP);

    /* Save the state values at time t in y, we'll use x as ynew. */
    (void)memset(deltaY, 0, nXc*sizeof(real_T));
	(void)memcpy(x0, x, nXc*sizeof(real_T));

    for(stagesIdx=0;stagesIdx<NSTAGES;stagesIdx++)
	{
		memcpy(x,x0,nXc*sizeof(real_T));
		
		for(statesIdx=0;statesIdx<nXc;statesIdx++)
		{
			deltaX = 0;
			for(idx=0;idx<stagesIdx;idx++)
			{
				deltaX = deltaX + h*rt_ODE8_A[stagesIdx][idx]*f[idx][statesIdx];
			}
			x[statesIdx] = x0[statesIdx] + deltaX;
		}
		
        if(stagesIdx==0)
        {
            rtsiSetdX(si, f[stagesIdx]);
            DERIVATIVES(si);
        }else
        {
            (stagesIdx==NSTAGES-1)? rtsiSetT(si, tnew) : rtsiSetT(si, t + h*rt_ODE8_C[stagesIdx]);
            rtsiSetdX(si, f[stagesIdx]);
            OUTPUTS(si,0);
            DERIVATIVES(si);
        }
				
		for(statesIdx=0;statesIdx<nXc;statesIdx++) 
		{
			deltaY[statesIdx]  = deltaY[statesIdx] + h*rt_ODE8_B[stagesIdx]*f[stagesIdx][statesIdx];
		}
	}
	
	for(statesIdx=0;statesIdx<nXc;statesIdx++) 
	{
		x[statesIdx] = x0[statesIdx] + deltaY[statesIdx];
	}

    PROJECTION(si);
    REDUCTION(si);

    rtsiSetSimTimeStep(si,MAJOR_TIME_STEP);
}

/* [EOF] ode5.c */
