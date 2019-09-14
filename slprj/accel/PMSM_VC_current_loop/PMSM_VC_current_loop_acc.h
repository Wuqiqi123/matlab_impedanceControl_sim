#include "__cf_PMSM_VC_current_loop.h"
#ifndef RTW_HEADER_PMSM_VC_current_loop_acc_h_
#define RTW_HEADER_PMSM_VC_current_loop_acc_h_
#include <stddef.h>
#include <float.h>
#ifndef PMSM_VC_current_loop_acc_COMMON_INCLUDES_
#define PMSM_VC_current_loop_acc_COMMON_INCLUDES_
#include <stdlib.h>
#define S_FUNCTION_NAME simulink_only_sfcn 
#define S_FUNCTION_LEVEL 2
#define RTW_GENERATED_S_FUNCTION
#include "rtwtypes.h"
#include "simstruc.h"
#include "fixedpoint.h"
#endif
#include "PMSM_VC_current_loop_acc_types.h"
#include "multiword_types.h"
#include "mwmathutil.h"
#include "rt_defines.h"
#include "rt_nonfinite.h"
typedef struct { int32_T Subsystempi2delay_sysIdxToRun ; int8_T
Subsystempi2delay_SubsysRanBC ; char_T pad_Subsystempi2delay_SubsysRanBC [ 3
] ; } DW_Subsystempi2delay_PMSM_VC_current_loop_T ; typedef struct { real_T
B_7_4_0 ; real_T B_7_5_0 ; real_T B_7_6_0 ; real_T B_7_7_0 [ 10 ] ; real_T
B_7_7_1 [ 6 ] ; real_T B_7_8_0 ; real_T B_7_9_0 ; real_T B_7_10_0 [ 3 ] ;
real_T B_7_14_0 ; real_T B_7_15_0 ; real_T B_7_27_0 ; real_T B_7_28_0 ;
real_T B_7_30_0 ; real_T B_7_32_0 ; real_T B_7_33_0 ; real_T B_7_34_0 ;
real_T B_7_35_0 ; real_T B_7_43_0 ; real_T B_7_45_0 ; real_T B_7_47_0 [ 3 ] ;
real_T B_7_48_0 ; real_T B_7_54_0 ; real_T B_7_57_0 ; real_T B_7_59_0 ;
real_T B_7_62_0 ; real_T B_7_70_0 [ 2 ] ; real_T B_7_71_0 ; real_T B_7_72_0 ;
real_T B_7_74_0 ; real_T B_7_76_0 ; real_T B_7_84_0 ; real_T B_7_93_0 ;
real_T B_7_102_0 [ 6 ] ; real_T B_7_109_0 ; real_T B_7_110_0 ; real_T
B_7_111_0 ; real_T B_7_112_0 ; real_T B_7_117_0 ; real_T B_7_121_0 ; real_T
B_7_127_0 ; real_T B_7_133_0 ; real_T B_7_140_0 ; real_T B_7_141_0 ; real_T
B_7_142_0 ; real_T B_7_143_0 ; real_T B_7_144_0 ; real_T B_7_145_0 ; real_T
B_7_146_0 ; real_T B_7_147_0 ; real_T B_7_158_0 ; real_T B_6_0_0 ; real_T
B_6_1_0 ; real_T B_5_0_0 ; real_T B_5_1_0 ; real_T B_2_0_0 ; real_T B_2_1_0 ;
real_T B_1_0_0 ; real_T B_1_1_0 ; real_T B_0_0_1 ; real_T B_0_0_2 ; real_T
B_0_0_3 ; real_T B_0_0_4 ; uint8_T B_7_18_0 ; uint8_T B_7_21_0 ; uint8_T
B_7_65_0 ; uint8_T B_7_68_0 ; uint8_T B_7_153_0 ; uint8_T B_7_155_0 ;
boolean_T B_7_79_0 ; boolean_T B_7_83_0 ; boolean_T B_7_88_0 ; boolean_T
B_7_92_0 ; boolean_T B_7_97_0 ; boolean_T B_7_101_0 ; char_T pad_B_7_101_0 [
4 ] ; } B_PMSM_VC_current_loop_T ; typedef struct { real_T
DiscreteTimeIntegrator1_DSTATE ; real_T DiscreteTimeIntegrator_DSTATE ;
real_T DiscreteTimeIntegrator_DSTATE_a ; real_T Integrator_DSTATE ; real_T
DiscreteTimeIntegrator_DSTATE_l ; real_T Integrator_DSTATE_d ; real_T
Integrator_DSTATE_dh ; real_T SFunction1_DSTATE [ 2 ] ; real_T
UnitDelay3_DSTATE ; real_T NextOutput ; struct { real_T modelTStart ; }
VariableTimeDelay_RWORK ; struct { real_T modelTStart ; }
VariableTimeDelay_RWORK_e ; struct { real_T modelTStart ; }
VariableTimeDelay_RWORK_c ; struct { real_T modelTStart ; }
VariableTimeDelay_RWORK_p ; struct { real_T modelTStart ; }
VariableTimeDelay_RWORK_m ; struct { real_T modelTStart ; }
VariableTimeDelay_RWORK_a ; struct { void * AS ; void * BS ; void * CS ; void
* DS ; void * DX_COL ; void * BD_COL ; void * TMP1 ; void * TMP2 ; void *
XTMP ; void * SWITCH_STATUS ; void * SWITCH_STATUS_INIT ; void * SW_CHG ;
void * G_STATE ; void * USWLAST ; void * XKM12 ; void * XKP12 ; void * XLAST
; void * ULAST ; void * IDX_SW_CHG ; void * Y_SWITCH ; void * SWITCH_TYPES ;
void * IDX_OUT_SW ; void * SWITCH_TOPO_SAVED_IDX ; void * SWITCH_MAP ; }
StateSpace_PWORK ; void * Scope15_PWORK ; void * Scope16_PWORK ; void *
Scope18_PWORK ; void * Scope19_PWORK ; void * Scope2_PWORK [ 2 ] ; void *
Scope20_PWORK ; struct { void * TUbufferPtrs [ 2 ] ; }
VariableTimeDelay_PWORK ; struct { void * TUbufferPtrs [ 2 ] ; }
VariableTimeDelay_PWORK_c ; struct { void * TUbufferPtrs [ 2 ] ; }
VariableTimeDelay_PWORK_e ; struct { void * TUbufferPtrs [ 2 ] ; }
VariableTimeDelay_PWORK_g ; struct { void * TUbufferPtrs [ 2 ] ; }
VariableTimeDelay_PWORK_o ; struct { void * TUbufferPtrs [ 2 ] ; }
VariableTimeDelay_PWORK_p ; void * Scope3_PWORK [ 6 ] ; void * Scope4_PWORK ;
void * Scope17_PWORK ; int32_T Subsystem1_sysIdxToRun ; int32_T
Subsystempi2delay_sysIdxToRun ; int32_T Subsystem1_sysIdxToRun_a ; int32_T
Subsystempi2delay_sysIdxToRun_c ; int32_T TIDSPSVPWM_sysIdxToRun ; uint32_T
RandSeed ; int_T StateSpace_IWORK [ 11 ] ; struct { int_T Tail ; int_T Head ;
int_T Last ; int_T CircularBufSize ; int_T MaxNewBufSize ; }
VariableTimeDelay_IWORK ; struct { int_T Tail ; int_T Head ; int_T Last ;
int_T CircularBufSize ; int_T MaxNewBufSize ; } VariableTimeDelay_IWORK_g ;
struct { int_T Tail ; int_T Head ; int_T Last ; int_T CircularBufSize ; int_T
MaxNewBufSize ; } VariableTimeDelay_IWORK_c ; struct { int_T Tail ; int_T
Head ; int_T Last ; int_T CircularBufSize ; int_T MaxNewBufSize ; }
VariableTimeDelay_IWORK_gl ; struct { int_T Tail ; int_T Head ; int_T Last ;
int_T CircularBufSize ; int_T MaxNewBufSize ; } VariableTimeDelay_IWORK_a ;
struct { int_T Tail ; int_T Head ; int_T Last ; int_T CircularBufSize ; int_T
MaxNewBufSize ; } VariableTimeDelay_IWORK_d ; int8_T Subsystem1_SubsysRanBC ;
int8_T Subsystempi2delay_SubsysRanBC ; int8_T Subsystem1_SubsysRanBC_d ;
int8_T Subsystempi2delay_SubsysRanBC_j ; uint8_T
DiscreteTimeIntegrator1_NumInitCond ; uint8_T
DiscreteTimeIntegrator_NumInitCond ; uint8_T
DiscreteTimeIntegrator_NumInitCond_f ; uint8_T
DiscreteTimeIntegrator_NumInitCond_l ; char_T
pad_DiscreteTimeIntegrator_NumInitCond_l [ 4 ] ;
DW_Subsystempi2delay_PMSM_VC_current_loop_T Subsystem1_o ;
DW_Subsystempi2delay_PMSM_VC_current_loop_T Subsystempi2delay_g ; }
DW_PMSM_VC_current_loop_T ; struct
P_Subsystempi2delay_PMSM_VC_current_loop_T_ { real_T P_0 [ 2 ] ; } ; struct
P_PMSM_VC_current_loop_T_ { real_T P_0 [ 2 ] ; real_T P_1 [ 2 ] ; real_T P_2
[ 2 ] ; real_T P_3 [ 2 ] ; real_T P_4 ; real_T P_5 ; real_T P_6 ; real_T P_7
; real_T P_8 ; real_T P_9 ; real_T P_10 ; real_T P_11 [ 2 ] ; real_T P_12 [
90 ] ; real_T P_13 ; real_T P_14 [ 9 ] ; real_T P_15 ; real_T P_16 ; real_T
P_17 ; real_T P_18 ; real_T P_19 ; real_T P_20 ; real_T P_21 ; real_T P_22 ;
real_T P_23 ; real_T P_24 ; real_T P_25 ; real_T P_26 ; real_T P_27 ; real_T
P_28 ; real_T P_29 ; real_T P_30 ; real_T P_31 ; real_T P_32 ; real_T P_33 ;
real_T P_34 ; real_T P_35 ; real_T P_36 ; real_T P_37 ; real_T P_38 ; real_T
P_39 ; real_T P_40 ; real_T P_41 ; real_T P_42 [ 3 ] ; real_T P_43 [ 3 ] ;
real_T P_44 ; real_T P_45 ; real_T P_46 ; real_T P_47 ; real_T P_48 ; real_T
P_49 ; real_T P_50 ; real_T P_51 ; real_T P_52 ; real_T P_53 ; real_T P_54 ;
real_T P_55 ; real_T P_56 ; real_T P_57 ; real_T P_58 ; real_T P_59 ; real_T
P_60 ; real_T P_61 ; real_T P_62 ; real_T P_63 ; real_T P_64 ; real_T P_65 ;
real_T P_66 ; real_T P_67 ; real_T P_68 ; real_T P_69 ; real_T P_70 ; real_T
P_71 ; real_T P_72 ; real_T P_73 ; real_T P_74 ; real_T P_75 ; real_T P_76 ;
real_T P_77 ; real_T P_78 ; real_T P_79 ; real_T P_80 ;
P_Subsystempi2delay_PMSM_VC_current_loop_T Subsystem1_o ;
P_Subsystempi2delay_PMSM_VC_current_loop_T Subsystempi2delay_g ; } ; extern
P_PMSM_VC_current_loop_T PMSM_VC_current_loop_rtDefaultP ;
#endif
