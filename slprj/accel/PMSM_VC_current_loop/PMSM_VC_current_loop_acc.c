#include "__cf_PMSM_VC_current_loop.h"
#include <math.h>
#include "PMSM_VC_current_loop_acc.h"
#include "PMSM_VC_current_loop_acc_private.h"
#include <stdio.h>
#include "slexec_vm_simstruct_bridge.h"
#include "slexec_vm_zc_functions.h"
#include "slexec_vm_lookup_functions.h"
#include "slsv_diagnostic_codegen_c_api.h"
#include "simstruc.h"
#include "fixedpoint.h"
#define CodeFormat S-Function
#define AccDefine1 Accelerator_S-Function
#include "simtarget/slAccSfcnBridge.h"
#ifndef __RTW_UTFREE__  
extern void * utMalloc ( size_t ) ; extern void utFree ( void * ) ;
#endif
boolean_T PMSM_VC_current_loop_acc_rt_TDelayUpdateTailOrGrowBuf ( int_T *
bufSzPtr , int_T * tailPtr , int_T * headPtr , int_T * lastPtr , real_T
tMinusDelay , real_T * * tBufPtr , real_T * * uBufPtr , real_T * * xBufPtr ,
boolean_T isfixedbuf , boolean_T istransportdelay , int_T * maxNewBufSzPtr )
{ int_T testIdx ; int_T tail = * tailPtr ; int_T bufSz = * bufSzPtr ; real_T
* tBuf = * tBufPtr ; real_T * xBuf = ( NULL ) ; int_T numBuffer = 2 ; if (
istransportdelay ) { numBuffer = 3 ; xBuf = * xBufPtr ; } testIdx = ( tail <
( bufSz - 1 ) ) ? ( tail + 1 ) : 0 ; if ( ( tMinusDelay <= tBuf [ testIdx ] )
&& ! isfixedbuf ) { int_T j ; real_T * tempT ; real_T * tempU ; real_T *
tempX = ( NULL ) ; real_T * uBuf = * uBufPtr ; int_T newBufSz = bufSz + 1024
; if ( newBufSz > * maxNewBufSzPtr ) { * maxNewBufSzPtr = newBufSz ; } tempU
= ( real_T * ) utMalloc ( numBuffer * newBufSz * sizeof ( real_T ) ) ; if (
tempU == ( NULL ) ) { return ( false ) ; } tempT = tempU + newBufSz ; if (
istransportdelay ) tempX = tempT + newBufSz ; for ( j = tail ; j < bufSz ; j
++ ) { tempT [ j - tail ] = tBuf [ j ] ; tempU [ j - tail ] = uBuf [ j ] ; if
( istransportdelay ) tempX [ j - tail ] = xBuf [ j ] ; } for ( j = 0 ; j <
tail ; j ++ ) { tempT [ j + bufSz - tail ] = tBuf [ j ] ; tempU [ j + bufSz -
tail ] = uBuf [ j ] ; if ( istransportdelay ) tempX [ j + bufSz - tail ] =
xBuf [ j ] ; } if ( * lastPtr > tail ) { * lastPtr -= tail ; } else { *
lastPtr += ( bufSz - tail ) ; } * tailPtr = 0 ; * headPtr = bufSz ; utFree (
uBuf ) ; * bufSzPtr = newBufSz ; * tBufPtr = tempT ; * uBufPtr = tempU ; if (
istransportdelay ) * xBufPtr = tempX ; } else { * tailPtr = testIdx ; }
return ( true ) ; } real_T PMSM_VC_current_loop_acc_rt_TDelayInterpolate (
real_T tMinusDelay , real_T tStart , real_T * tBuf , real_T * uBuf , int_T
bufSz , int_T * lastIdx , int_T oldestIdx , int_T newIdx , real_T initOutput
, boolean_T discrete , boolean_T minorStepAndTAtLastMajorOutput ) { int_T i ;
real_T yout , t1 , t2 , u1 , u2 ; if ( ( newIdx == 0 ) && ( oldestIdx == 0 )
&& ( tMinusDelay > tStart ) ) return initOutput ; if ( tMinusDelay <= tStart
) return initOutput ; if ( ( tMinusDelay <= tBuf [ oldestIdx ] ) ) { if (
discrete ) { return ( uBuf [ oldestIdx ] ) ; } else { int_T tempIdx =
oldestIdx + 1 ; if ( oldestIdx == bufSz - 1 ) tempIdx = 0 ; t1 = tBuf [
oldestIdx ] ; t2 = tBuf [ tempIdx ] ; u1 = uBuf [ oldestIdx ] ; u2 = uBuf [
tempIdx ] ; if ( t2 == t1 ) { if ( tMinusDelay >= t2 ) { yout = u2 ; } else {
yout = u1 ; } } else { real_T f1 = ( t2 - tMinusDelay ) / ( t2 - t1 ) ;
real_T f2 = 1.0 - f1 ; yout = f1 * u1 + f2 * u2 ; } return yout ; } } if (
minorStepAndTAtLastMajorOutput ) { if ( newIdx != 0 ) { if ( * lastIdx ==
newIdx ) { ( * lastIdx ) -- ; } newIdx -- ; } else { if ( * lastIdx == newIdx
) { * lastIdx = bufSz - 1 ; } newIdx = bufSz - 1 ; } } i = * lastIdx ; if (
tBuf [ i ] < tMinusDelay ) { while ( tBuf [ i ] < tMinusDelay ) { if ( i ==
newIdx ) break ; i = ( i < ( bufSz - 1 ) ) ? ( i + 1 ) : 0 ; } } else { while
( tBuf [ i ] >= tMinusDelay ) { i = ( i > 0 ) ? i - 1 : ( bufSz - 1 ) ; } i =
( i < ( bufSz - 1 ) ) ? ( i + 1 ) : 0 ; } * lastIdx = i ; if ( discrete ) {
double tempEps = ( DBL_EPSILON ) * 128.0 ; double localEps = tempEps *
muDoubleScalarAbs ( tBuf [ i ] ) ; if ( tempEps > localEps ) { localEps =
tempEps ; } localEps = localEps / 2.0 ; if ( tMinusDelay >= ( tBuf [ i ] -
localEps ) ) { yout = uBuf [ i ] ; } else { if ( i == 0 ) { yout = uBuf [
bufSz - 1 ] ; } else { yout = uBuf [ i - 1 ] ; } } } else { if ( i == 0 ) {
t1 = tBuf [ bufSz - 1 ] ; u1 = uBuf [ bufSz - 1 ] ; } else { t1 = tBuf [ i -
1 ] ; u1 = uBuf [ i - 1 ] ; } t2 = tBuf [ i ] ; u2 = uBuf [ i ] ; if ( t2 ==
t1 ) { if ( tMinusDelay >= t2 ) { yout = u2 ; } else { yout = u1 ; } } else {
real_T f1 = ( t2 - tMinusDelay ) / ( t2 - t1 ) ; real_T f2 = 1.0 - f1 ; yout
= f1 * u1 + f2 * u2 ; } } return ( yout ) ; } real_T look1_binlxpw ( real_T
u0 , const real_T bp0 [ ] , const real_T table [ ] , uint32_T maxIndex ) {
real_T frac ; uint32_T iRght ; uint32_T iLeft ; uint32_T bpIdx ; if ( u0 <=
bp0 [ 0U ] ) { iLeft = 0U ; frac = ( u0 - bp0 [ 0U ] ) / ( bp0 [ 1U ] - bp0 [
0U ] ) ; } else if ( u0 < bp0 [ maxIndex ] ) { bpIdx = maxIndex >> 1U ; iLeft
= 0U ; iRght = maxIndex ; while ( iRght - iLeft > 1U ) { if ( u0 < bp0 [
bpIdx ] ) { iRght = bpIdx ; } else { iLeft = bpIdx ; } bpIdx = ( iRght +
iLeft ) >> 1U ; } frac = ( u0 - bp0 [ iLeft ] ) / ( bp0 [ iLeft + 1U ] - bp0
[ iLeft ] ) ; } else { iLeft = maxIndex - 1U ; frac = ( u0 - bp0 [ maxIndex -
1U ] ) / ( bp0 [ maxIndex ] - bp0 [ maxIndex - 1U ] ) ; } return ( table [
iLeft + 1U ] - table [ iLeft ] ) * frac + table [ iLeft ] ; } void
rt_ssGetBlockPath ( SimStruct * S , int_T sysIdx , int_T blkIdx , char_T * *
path ) { _ssGetBlockPath ( S , sysIdx , blkIdx , path ) ; } void
rt_ssSet_slErrMsg ( SimStruct * S , void * diag ) { if ( !
_ssIsErrorStatusAslErrMsg ( S ) ) { _ssSet_slErrMsg ( S , diag ) ; } else {
_ssDiscardDiagnostic ( S , diag ) ; } } void rt_ssReportDiagnosticAsWarning (
SimStruct * S , void * diag ) { _ssReportDiagnosticAsWarning ( S , diag ) ; }
void PMSM_VC_current_loop_Subsystempi2delay_Term ( SimStruct * const S ) { }
real_T rt_urand_Upu32_Yd_f_pw_snf ( uint32_T * u ) { uint32_T lo ; uint32_T
hi ; lo = * u % 127773U * 16807U ; hi = * u / 127773U * 2836U ; if ( lo < hi
) { * u = 2147483647U - ( hi - lo ) ; } else { * u = lo - hi ; } return (
real_T ) * u * 4.6566128752457969E-10 ; } real_T rt_nrand_Upu32_Yd_f_pw_snf (
uint32_T * u ) { real_T y ; real_T sr ; real_T si ; do { sr = 2.0 *
rt_urand_Upu32_Yd_f_pw_snf ( u ) - 1.0 ; si = 2.0 *
rt_urand_Upu32_Yd_f_pw_snf ( u ) - 1.0 ; si = sr * sr + si * si ; } while (
si > 1.0 ) ; y = muDoubleScalarSqrt ( - 2.0 * muDoubleScalarLog ( si ) / si )
* sr ; return y ; } static void mdlOutputs ( SimStruct * S , int_T tid ) {
real_T B_7_77_0 ; real_T B_7_81_0 ; real_T B_7_86_0 ; real_T B_7_90_0 ;
real_T B_7_95_0 ; real_T B_7_99_0 ; real_T rtb_B_7_1_0 ; real_T rtb_B_7_1_1 ;
real_T rtb_B_7_0_0 ; real_T rtb_B_7_2_0 ; real_T rtb_B_7_3_0 ; real_T
rtb_B_7_12_0 [ 3 ] ; real_T rtb_B_7_40_0 ; boolean_T rtb_B_7_75_0 ; boolean_T
rtb_B_7_80_0 ; boolean_T rtb_B_7_85_0 ; boolean_T rtb_B_7_89_0 ; boolean_T
rtb_B_7_94_0 ; boolean_T rtb_B_7_98_0 ; real_T B_7_23_0_idx_0 ; real_T
B_7_23_0_idx_1 ; int32_T isHit ; real_T tmp ; B_PMSM_VC_current_loop_T * _rtB
; P_PMSM_VC_current_loop_T * _rtP ; DW_PMSM_VC_current_loop_T * _rtDW ; _rtDW
= ( ( DW_PMSM_VC_current_loop_T * ) ssGetRootDWork ( S ) ) ; _rtP = ( (
P_PMSM_VC_current_loop_T * ) ssGetModelRtp ( S ) ) ; _rtB = ( (
B_PMSM_VC_current_loop_T * ) _ssGetModelBlockIO ( S ) ) ; rtb_B_7_0_0 = _rtDW
-> DiscreteTimeIntegrator1_DSTATE ; muDoubleScalarSinCos ( _rtDW ->
DiscreteTimeIntegrator1_DSTATE , & rtb_B_7_1_0 , & rtb_B_7_1_1 ) ;
rtb_B_7_2_0 = _rtDW -> DiscreteTimeIntegrator_DSTATE ; rtb_B_7_3_0 = _rtDW ->
DiscreteTimeIntegrator_DSTATE_a ; _rtB -> B_7_4_0 = _rtDW ->
DiscreteTimeIntegrator_DSTATE * rtb_B_7_1_1 + _rtDW ->
DiscreteTimeIntegrator_DSTATE_a * rtb_B_7_1_0 ; _rtB -> B_7_5_0 = ( ( - _rtDW
-> DiscreteTimeIntegrator_DSTATE - 1.7320508075688772 * _rtDW ->
DiscreteTimeIntegrator_DSTATE_a ) * rtb_B_7_1_1 + ( 1.7320508075688772 *
_rtDW -> DiscreteTimeIntegrator_DSTATE - _rtDW ->
DiscreteTimeIntegrator_DSTATE_a ) * rtb_B_7_1_0 ) * 0.5 ; ssCallAccelRunBlock
( S , 7 , 7 , SS_CALL_MDL_OUTPUTS ) ; _rtB -> B_7_9_0 = ( 0.0 - _rtB ->
B_7_5_0 ) - _rtB -> B_7_4_0 ; isHit = ssIsSampleHit ( S , 2 , 0 ) ; if (
isHit != 0 ) { _rtB -> B_7_10_0 [ 0 ] = _rtB -> B_7_4_0 ; _rtB -> B_7_10_0 [
1 ] = _rtB -> B_7_5_0 ; _rtB -> B_7_10_0 [ 2 ] = _rtB -> B_7_9_0 ; for (
isHit = 0 ; isHit < 3 ; isHit ++ ) { rtb_B_7_12_0 [ isHit ] = _rtP -> P_15 *
( _rtP -> P_14 [ isHit + 6 ] * _rtB -> B_7_10_0 [ 2 ] + ( _rtP -> P_14 [
isHit + 3 ] * _rtB -> B_7_10_0 [ 1 ] + _rtP -> P_14 [ isHit ] * _rtB ->
B_7_10_0 [ 0 ] ) ) ; } } rtb_B_7_0_0 = ( rtb_B_7_0_0 - 1.5707963267948966 ) /
5.0 ; _rtB -> B_7_14_0 = _rtP -> P_16 * rtb_B_7_0_0 ; isHit = ssIsSampleHit (
S , 2 , 0 ) ; if ( isHit != 0 ) { _rtB -> B_7_15_0 = _rtB -> B_7_14_0 ; if (
_rtB -> B_7_18_0 > 0 ) { _rtB -> B_2_0_0 = rtb_B_7_12_0 [ 0 ] *
muDoubleScalarCos ( _rtB -> B_7_15_0 ) + rtb_B_7_12_0 [ 1 ] *
muDoubleScalarSin ( _rtB -> B_7_15_0 ) ; _rtB -> B_2_1_0 = - rtb_B_7_12_0 [ 0
] * muDoubleScalarSin ( _rtB -> B_7_15_0 ) + rtb_B_7_12_0 [ 1 ] *
muDoubleScalarCos ( _rtB -> B_7_15_0 ) ; srUpdateBC ( _rtDW ->
Subsystem1_SubsysRanBC_d ) ; } if ( _rtB -> B_7_21_0 > 0 ) { _rtB -> B_1_0_0
= rtb_B_7_12_0 [ 0 ] * muDoubleScalarSin ( _rtB -> B_7_15_0 ) - rtb_B_7_12_0
[ 1 ] * muDoubleScalarCos ( _rtB -> B_7_15_0 ) ; _rtB -> B_1_1_0 =
rtb_B_7_12_0 [ 0 ] * muDoubleScalarCos ( _rtB -> B_7_15_0 ) + rtb_B_7_12_0 [
1 ] * muDoubleScalarSin ( _rtB -> B_7_15_0 ) ; srUpdateBC ( _rtDW ->
Subsystempi2delay_SubsysRanBC_j ) ; } if ( _rtB -> B_7_18_0 != 0 ) {
B_7_23_0_idx_0 = _rtB -> B_2_0_0 ; B_7_23_0_idx_1 = _rtB -> B_2_1_0 ; } else
{ B_7_23_0_idx_0 = _rtB -> B_1_0_0 ; B_7_23_0_idx_1 = _rtB -> B_1_1_0 ; }
B_7_23_0_idx_0 = _rtB -> B_7_8_0 - B_7_23_0_idx_0 ; _rtB -> B_7_27_0 = _rtP
-> P_20 * B_7_23_0_idx_0 + _rtDW -> Integrator_DSTATE ; } _rtB -> B_7_32_0 =
_rtDW -> DiscreteTimeIntegrator_DSTATE_l ; _rtB -> B_7_33_0 = _rtP -> P_28 *
_rtB -> B_7_32_0 ; if ( ssGetT ( S ) > _rtP -> P_25 ) { tmp = _rtB ->
B_7_28_0 ; } else { tmp = _rtB -> B_7_30_0 ; } _rtB -> B_7_34_0 = tmp - _rtB
-> B_7_33_0 ; isHit = ssIsSampleHit ( S , 3 , 0 ) ; if ( isHit != 0 ) { _rtB
-> B_7_35_0 = _rtB -> B_7_34_0 ; } isHit = ssIsSampleHit ( S , 2 , 0 ) ; if (
isHit != 0 ) { rtb_B_7_40_0 = _rtP -> P_29 * _rtB -> B_7_35_0 + _rtDW ->
Integrator_DSTATE_d ; if ( rtb_B_7_40_0 > _rtP -> P_32 ) { rtb_B_7_40_0 =
_rtP -> P_32 ; } else { if ( rtb_B_7_40_0 < _rtP -> P_33 ) { rtb_B_7_40_0 =
_rtP -> P_33 ; } } rtb_B_7_40_0 -= B_7_23_0_idx_1 ; _rtB -> B_7_43_0 = _rtP
-> P_34 * rtb_B_7_40_0 + _rtDW -> Integrator_DSTATE_dh ; _rtB -> B_7_45_0 =
muDoubleScalarHypot ( _rtB -> B_7_27_0 , _rtB -> B_7_43_0 ) ;
ssCallAccelRunBlock ( S , 7 , 46 , SS_CALL_MDL_OUTPUTS ) ;
ssCallAccelRunBlock ( S , 7 , 47 , SS_CALL_MDL_OUTPUTS ) ; } _rtB -> B_7_48_0
= rtb_B_7_0_0 - _rtB -> B_7_47_0 [ 0 ] ; ssCallAccelRunBlock ( S , 7 , 49 ,
SS_CALL_MDL_OUTPUTS ) ; ssCallAccelRunBlock ( S , 7 , 50 ,
SS_CALL_MDL_OUTPUTS ) ; isHit = ssIsSampleHit ( S , 2 , 0 ) ; if ( isHit != 0
) { ssCallAccelRunBlock ( S , 7 , 51 , SS_CALL_MDL_OUTPUTS ) ;
ssCallAccelRunBlock ( S , 7 , 52 , SS_CALL_MDL_OUTPUTS ) ; _rtB -> B_7_54_0 =
_rtP -> P_40 * _rtDW -> NextOutput ; ssCallAccelRunBlock ( S , 7 , 55 ,
SS_CALL_MDL_OUTPUTS ) ; } B_7_23_0_idx_1 = look1_binlxpw ( muDoubleScalarRem
( ssGetT ( S ) - _rtB -> B_7_57_0 , _rtB -> B_7_59_0 ) , _rtP -> P_43 , _rtP
-> P_42 , 2U ) ; isHit = ssIsSampleHit ( S , 2 , 0 ) ; if ( isHit != 0 ) {
_rtB -> B_7_62_0 = _rtB -> B_7_14_0 ; if ( _rtB -> B_7_65_0 > 0 ) { _rtB ->
B_6_0_0 = _rtB -> B_7_27_0 * muDoubleScalarCos ( _rtB -> B_7_62_0 ) - _rtB ->
B_7_43_0 * muDoubleScalarSin ( _rtB -> B_7_62_0 ) ; _rtB -> B_6_1_0 = _rtB ->
B_7_27_0 * muDoubleScalarSin ( _rtB -> B_7_62_0 ) + _rtB -> B_7_43_0 *
muDoubleScalarCos ( _rtB -> B_7_62_0 ) ; srUpdateBC ( _rtDW ->
Subsystem1_SubsysRanBC ) ; } if ( _rtB -> B_7_68_0 > 0 ) { _rtB -> B_5_0_0 =
_rtB -> B_7_27_0 * muDoubleScalarSin ( _rtB -> B_7_62_0 ) + _rtB -> B_7_43_0
* muDoubleScalarCos ( _rtB -> B_7_62_0 ) ; _rtB -> B_5_1_0 = - _rtB ->
B_7_27_0 * muDoubleScalarCos ( _rtB -> B_7_62_0 ) + _rtB -> B_7_43_0 *
muDoubleScalarSin ( _rtB -> B_7_62_0 ) ; srUpdateBC ( _rtDW ->
Subsystempi2delay_SubsysRanBC ) ; } if ( _rtB -> B_7_65_0 != 0 ) { _rtB ->
B_7_70_0 [ 0 ] = _rtB -> B_6_0_0 ; _rtB -> B_7_70_0 [ 1 ] = _rtB -> B_6_1_0 ;
} else { _rtB -> B_7_70_0 [ 0 ] = _rtB -> B_5_0_0 ; _rtB -> B_7_70_0 [ 1 ] =
_rtB -> B_5_1_0 ; } } _rtB -> B_7_71_0 = _rtDW -> UnitDelay3_DSTATE ; isHit =
ssIsSampleHit ( S , 2 , 0 ) ; if ( isHit != 0 ) { _rtB -> B_7_72_0 = _rtB ->
B_7_71_0 ; ssCallAccelRunBlock ( S , 0 , 0 , SS_CALL_MDL_OUTPUTS ) ; _rtB ->
B_7_74_0 = ( 1.0 + _rtB -> B_0_0_1 ) / 4.0 * 0.0001 ; } rtb_B_7_75_0 = (
B_7_23_0_idx_1 <= _rtB -> B_7_74_0 ) ; { real_T * * uBuffer = ( real_T * * )
& _rtDW -> VariableTimeDelay_PWORK . TUbufferPtrs [ 0 ] ; real_T * * tBuffer
= ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK . TUbufferPtrs [ 1 ] ;
real_T simTime = ssGetT ( S ) ; real_T appliedDelay ; appliedDelay = _rtB ->
B_7_76_0 ; if ( appliedDelay > _rtP -> P_49 ) { appliedDelay = _rtP -> P_49 ;
} if ( appliedDelay < 0.0 ) { appliedDelay = 0.0 ; } B_7_77_0 =
PMSM_VC_current_loop_acc_rt_TDelayInterpolate ( simTime - appliedDelay , 0.0
, * tBuffer , * uBuffer , _rtDW -> VariableTimeDelay_IWORK . CircularBufSize
, & _rtDW -> VariableTimeDelay_IWORK . Last , _rtDW ->
VariableTimeDelay_IWORK . Tail , _rtDW -> VariableTimeDelay_IWORK . Head ,
_rtP -> P_50 , 0 , ( boolean_T ) ( ssIsMinorTimeStep ( S ) && (
ssGetTimeOfLastOutput ( S ) == ssGetT ( S ) ) ) ) ; } _rtB -> B_7_79_0 = (
rtb_B_7_75_0 && ( B_7_77_0 != 0.0 ) ) ; rtb_B_7_80_0 = ! rtb_B_7_75_0 ; {
real_T * * uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_c .
TUbufferPtrs [ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_c . TUbufferPtrs [ 1 ] ; real_T simTime = ssGetT ( S
) ; real_T appliedDelay ; appliedDelay = _rtB -> B_7_76_0 ; if ( appliedDelay
> _rtP -> P_51 ) { appliedDelay = _rtP -> P_51 ; } if ( appliedDelay < 0.0 )
{ appliedDelay = 0.0 ; } B_7_81_0 =
PMSM_VC_current_loop_acc_rt_TDelayInterpolate ( simTime - appliedDelay , 0.0
, * tBuffer , * uBuffer , _rtDW -> VariableTimeDelay_IWORK_g .
CircularBufSize , & _rtDW -> VariableTimeDelay_IWORK_g . Last , _rtDW ->
VariableTimeDelay_IWORK_g . Tail , _rtDW -> VariableTimeDelay_IWORK_g . Head
, _rtP -> P_52 , 0 , ( boolean_T ) ( ssIsMinorTimeStep ( S ) && (
ssGetTimeOfLastOutput ( S ) == ssGetT ( S ) ) ) ) ; } _rtB -> B_7_83_0 = (
rtb_B_7_80_0 && ( B_7_81_0 != 0.0 ) ) ; isHit = ssIsSampleHit ( S , 2 , 0 ) ;
if ( isHit != 0 ) { _rtB -> B_7_84_0 = ( 1.0 + _rtB -> B_0_0_2 ) / 4.0 *
0.0001 ; } rtb_B_7_85_0 = ( B_7_23_0_idx_1 <= _rtB -> B_7_84_0 ) ; { real_T *
* uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_e .
TUbufferPtrs [ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_e . TUbufferPtrs [ 1 ] ; real_T simTime = ssGetT ( S
) ; real_T appliedDelay ; appliedDelay = _rtB -> B_7_76_0 ; if ( appliedDelay
> _rtP -> P_53 ) { appliedDelay = _rtP -> P_53 ; } if ( appliedDelay < 0.0 )
{ appliedDelay = 0.0 ; } B_7_86_0 =
PMSM_VC_current_loop_acc_rt_TDelayInterpolate ( simTime - appliedDelay , 0.0
, * tBuffer , * uBuffer , _rtDW -> VariableTimeDelay_IWORK_c .
CircularBufSize , & _rtDW -> VariableTimeDelay_IWORK_c . Last , _rtDW ->
VariableTimeDelay_IWORK_c . Tail , _rtDW -> VariableTimeDelay_IWORK_c . Head
, _rtP -> P_54 , 0 , ( boolean_T ) ( ssIsMinorTimeStep ( S ) && (
ssGetTimeOfLastOutput ( S ) == ssGetT ( S ) ) ) ) ; } _rtB -> B_7_88_0 = (
rtb_B_7_85_0 && ( B_7_86_0 != 0.0 ) ) ; rtb_B_7_89_0 = ! rtb_B_7_85_0 ; {
real_T * * uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_g .
TUbufferPtrs [ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_g . TUbufferPtrs [ 1 ] ; real_T simTime = ssGetT ( S
) ; real_T appliedDelay ; appliedDelay = _rtB -> B_7_76_0 ; if ( appliedDelay
> _rtP -> P_55 ) { appliedDelay = _rtP -> P_55 ; } if ( appliedDelay < 0.0 )
{ appliedDelay = 0.0 ; } B_7_90_0 =
PMSM_VC_current_loop_acc_rt_TDelayInterpolate ( simTime - appliedDelay , 0.0
, * tBuffer , * uBuffer , _rtDW -> VariableTimeDelay_IWORK_gl .
CircularBufSize , & _rtDW -> VariableTimeDelay_IWORK_gl . Last , _rtDW ->
VariableTimeDelay_IWORK_gl . Tail , _rtDW -> VariableTimeDelay_IWORK_gl .
Head , _rtP -> P_56 , 0 , ( boolean_T ) ( ssIsMinorTimeStep ( S ) && (
ssGetTimeOfLastOutput ( S ) == ssGetT ( S ) ) ) ) ; } _rtB -> B_7_92_0 = (
rtb_B_7_89_0 && ( B_7_90_0 != 0.0 ) ) ; isHit = ssIsSampleHit ( S , 2 , 0 ) ;
if ( isHit != 0 ) { _rtB -> B_7_93_0 = ( 1.0 + _rtB -> B_0_0_3 ) / 4.0 *
0.0001 ; } rtb_B_7_94_0 = ( B_7_23_0_idx_1 <= _rtB -> B_7_93_0 ) ; { real_T *
* uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_o .
TUbufferPtrs [ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_o . TUbufferPtrs [ 1 ] ; real_T simTime = ssGetT ( S
) ; real_T appliedDelay ; appliedDelay = _rtB -> B_7_76_0 ; if ( appliedDelay
> _rtP -> P_57 ) { appliedDelay = _rtP -> P_57 ; } if ( appliedDelay < 0.0 )
{ appliedDelay = 0.0 ; } B_7_95_0 =
PMSM_VC_current_loop_acc_rt_TDelayInterpolate ( simTime - appliedDelay , 0.0
, * tBuffer , * uBuffer , _rtDW -> VariableTimeDelay_IWORK_a .
CircularBufSize , & _rtDW -> VariableTimeDelay_IWORK_a . Last , _rtDW ->
VariableTimeDelay_IWORK_a . Tail , _rtDW -> VariableTimeDelay_IWORK_a . Head
, _rtP -> P_58 , 0 , ( boolean_T ) ( ssIsMinorTimeStep ( S ) && (
ssGetTimeOfLastOutput ( S ) == ssGetT ( S ) ) ) ) ; } _rtB -> B_7_97_0 = (
rtb_B_7_94_0 && ( B_7_95_0 != 0.0 ) ) ; rtb_B_7_98_0 = ! rtb_B_7_94_0 ; {
real_T * * uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_p .
TUbufferPtrs [ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_p . TUbufferPtrs [ 1 ] ; real_T simTime = ssGetT ( S
) ; real_T appliedDelay ; appliedDelay = _rtB -> B_7_76_0 ; if ( appliedDelay
> _rtP -> P_59 ) { appliedDelay = _rtP -> P_59 ; } if ( appliedDelay < 0.0 )
{ appliedDelay = 0.0 ; } B_7_99_0 =
PMSM_VC_current_loop_acc_rt_TDelayInterpolate ( simTime - appliedDelay , 0.0
, * tBuffer , * uBuffer , _rtDW -> VariableTimeDelay_IWORK_d .
CircularBufSize , & _rtDW -> VariableTimeDelay_IWORK_d . Last , _rtDW ->
VariableTimeDelay_IWORK_d . Tail , _rtDW -> VariableTimeDelay_IWORK_d . Head
, _rtP -> P_60 , 0 , ( boolean_T ) ( ssIsMinorTimeStep ( S ) && (
ssGetTimeOfLastOutput ( S ) == ssGetT ( S ) ) ) ) ; } _rtB -> B_7_101_0 = (
rtb_B_7_98_0 && ( B_7_99_0 != 0.0 ) ) ; _rtB -> B_7_102_0 [ 0 ] = _rtB ->
B_7_79_0 ; _rtB -> B_7_102_0 [ 1 ] = _rtB -> B_7_83_0 ; _rtB -> B_7_102_0 [ 2
] = _rtB -> B_7_88_0 ; _rtB -> B_7_102_0 [ 3 ] = _rtB -> B_7_92_0 ; _rtB ->
B_7_102_0 [ 4 ] = _rtB -> B_7_97_0 ; _rtB -> B_7_102_0 [ 5 ] = _rtB ->
B_7_101_0 ; isHit = ssIsSampleHit ( S , 2 , 0 ) ; if ( isHit != 0 ) { _rtB ->
B_7_110_0 = _rtP -> P_62 * rtb_B_7_40_0 ; _rtB -> B_7_111_0 = _rtP -> P_63 *
B_7_23_0_idx_0 ; _rtB -> B_7_112_0 = _rtP -> P_64 * _rtB -> B_7_35_0 ; } _rtB
-> B_7_117_0 = _rtP -> P_66 * _rtB -> B_7_32_0 ; _rtB -> B_7_121_0 = ( ( (
2.0 * _rtB -> B_7_7_0 [ 6 ] + _rtB -> B_7_7_0 [ 7 ] ) * rtb_B_7_1_0 + -
1.7320508075688772 * _rtB -> B_7_7_0 [ 7 ] * rtb_B_7_1_1 ) *
0.33333333333333331 * _rtP -> P_65 - _rtP -> P_68 * rtb_B_7_3_0 ) + _rtB ->
B_7_117_0 * rtb_B_7_2_0 * _rtP -> P_67 ; _rtB -> B_7_127_0 = ( ( ( ( 2.0 *
_rtB -> B_7_7_0 [ 6 ] + _rtB -> B_7_7_0 [ 7 ] ) * rtb_B_7_1_1 +
1.7320508075688772 * _rtB -> B_7_7_0 [ 7 ] * rtb_B_7_1_0 ) *
0.33333333333333331 * _rtP -> P_69 - _rtP -> P_71 * rtb_B_7_2_0 ) -
rtb_B_7_3_0 * _rtB -> B_7_117_0 * _rtP -> P_70 ) - _rtP -> P_72 * _rtB ->
B_7_117_0 ; _rtB -> B_7_133_0 = ( ( ( 0.0 * rtb_B_7_2_0 * rtb_B_7_3_0 +
0.04278 * rtb_B_7_2_0 ) * 7.5 - _rtB -> B_7_109_0 ) - ( _rtP -> P_74 *
muDoubleScalarSign ( _rtB -> B_7_32_0 ) + _rtP -> P_73 * _rtB -> B_7_32_0 ) )
* _rtP -> P_75 ; ssCallAccelRunBlock ( S , 7 , 139 , SS_CALL_MDL_OUTPUTS ) ;
_rtB -> B_7_140_0 = rtb_B_7_98_0 ; _rtB -> B_7_141_0 = rtb_B_7_94_0 ; _rtB ->
B_7_142_0 = rtb_B_7_89_0 ; _rtB -> B_7_143_0 = rtb_B_7_85_0 ; _rtB ->
B_7_144_0 = rtb_B_7_80_0 ; _rtB -> B_7_145_0 = rtb_B_7_75_0 ; _rtB ->
B_7_146_0 = rtb_B_7_0_0 - _rtB -> B_7_54_0 ; _rtB -> B_7_147_0 = _rtP -> P_76
* _rtB -> B_7_7_0 [ 8 ] ; _rtB -> B_7_158_0 = _rtP -> P_80 * _rtB -> B_7_7_0
[ 9 ] ; ssCallAccelRunBlock ( S , 7 , 159 , SS_CALL_MDL_OUTPUTS ) ;
ssCallAccelRunBlock ( S , 7 , 163 , SS_CALL_MDL_OUTPUTS ) ; UNUSED_PARAMETER
( tid ) ; } static void mdlOutputsTID4 ( SimStruct * S , int_T tid ) {
B_PMSM_VC_current_loop_T * _rtB ; P_PMSM_VC_current_loop_T * _rtP ; _rtP = (
( P_PMSM_VC_current_loop_T * ) ssGetModelRtp ( S ) ) ; _rtB = ( (
B_PMSM_VC_current_loop_T * ) _ssGetModelBlockIO ( S ) ) ; _rtB -> B_7_6_0 =
_rtP -> P_10 ; _rtB -> B_7_8_0 = _rtP -> P_13 ; _rtB -> B_7_18_0 = ( uint8_T
) ( _rtP -> P_17 == _rtP -> P_18 ) ; _rtB -> B_7_21_0 = ( uint8_T ) ( _rtP ->
P_17 == _rtP -> P_19 ) ; _rtB -> B_7_28_0 = _rtP -> P_23 ; _rtB -> B_7_30_0 =
_rtP -> P_24 ; _rtB -> B_7_59_0 = _rtP -> P_41 ; _rtB -> B_7_65_0 = ( uint8_T
) ( _rtP -> P_44 == _rtP -> P_45 ) ; _rtB -> B_7_68_0 = ( uint8_T ) ( _rtP ->
P_44 == _rtP -> P_46 ) ; _rtB -> B_7_76_0 = _rtP -> P_48 ; _rtB -> B_7_109_0
= _rtP -> P_61 ; _rtB -> B_7_153_0 = ( uint8_T ) ( _rtP -> P_77 == _rtP ->
P_78 ) ; _rtB -> B_7_155_0 = ( uint8_T ) ( _rtP -> P_77 == _rtP -> P_79 ) ;
UNUSED_PARAMETER ( tid ) ; }
#define MDL_UPDATE
static void mdlUpdate ( SimStruct * S , int_T tid ) { int32_T isHit ;
B_PMSM_VC_current_loop_T * _rtB ; P_PMSM_VC_current_loop_T * _rtP ;
DW_PMSM_VC_current_loop_T * _rtDW ; _rtDW = ( ( DW_PMSM_VC_current_loop_T * )
ssGetRootDWork ( S ) ) ; _rtP = ( ( P_PMSM_VC_current_loop_T * )
ssGetModelRtp ( S ) ) ; _rtB = ( ( B_PMSM_VC_current_loop_T * )
_ssGetModelBlockIO ( S ) ) ; _rtDW -> DiscreteTimeIntegrator1_DSTATE += _rtP
-> P_4 * _rtB -> B_7_117_0 ; _rtDW -> DiscreteTimeIntegrator_DSTATE += _rtP
-> P_6 * _rtB -> B_7_127_0 ; _rtDW -> DiscreteTimeIntegrator_DSTATE_a += _rtP
-> P_8 * _rtB -> B_7_121_0 ; ssCallAccelRunBlock ( S , 7 , 7 ,
SS_CALL_MDL_UPDATE ) ; isHit = ssIsSampleHit ( S , 2 , 0 ) ; if ( isHit != 0
) { _rtDW -> Integrator_DSTATE += _rtP -> P_21 * _rtB -> B_7_111_0 ; } _rtDW
-> DiscreteTimeIntegrator_DSTATE_l += _rtP -> P_26 * _rtB -> B_7_133_0 ;
isHit = ssIsSampleHit ( S , 2 , 0 ) ; if ( isHit != 0 ) { _rtDW ->
Integrator_DSTATE_d += _rtP -> P_30 * _rtB -> B_7_112_0 ; _rtDW ->
Integrator_DSTATE_dh += _rtP -> P_35 * _rtB -> B_7_110_0 ;
ssCallAccelRunBlock ( S , 7 , 47 , SS_CALL_MDL_UPDATE ) ; _rtDW -> NextOutput
= rt_nrand_Upu32_Yd_f_pw_snf ( & _rtDW -> RandSeed ) * _rtP -> P_38 + _rtP ->
P_37 ; } _rtDW -> UnitDelay3_DSTATE = _rtB -> B_7_147_0 ; { real_T * *
uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK . TUbufferPtrs [
0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK
. TUbufferPtrs [ 1 ] ; real_T * * xBuffer = ( NULL ) ; real_T simTime =
ssGetT ( S ) ; _rtDW -> VariableTimeDelay_IWORK . Head = ( ( _rtDW ->
VariableTimeDelay_IWORK . Head < ( _rtDW -> VariableTimeDelay_IWORK .
CircularBufSize - 1 ) ) ? ( _rtDW -> VariableTimeDelay_IWORK . Head + 1 ) : 0
) ; if ( _rtDW -> VariableTimeDelay_IWORK . Head == _rtDW ->
VariableTimeDelay_IWORK . Tail ) { if ( !
PMSM_VC_current_loop_acc_rt_TDelayUpdateTailOrGrowBuf ( & _rtDW ->
VariableTimeDelay_IWORK . CircularBufSize , & _rtDW ->
VariableTimeDelay_IWORK . Tail , & _rtDW -> VariableTimeDelay_IWORK . Head ,
& _rtDW -> VariableTimeDelay_IWORK . Last , simTime - _rtP -> P_49 , tBuffer
, uBuffer , xBuffer , ( boolean_T ) 0 , ( boolean_T ) 0 , & _rtDW ->
VariableTimeDelay_IWORK . MaxNewBufSize ) ) { ssSetErrorStatus ( S ,
"vtdelay memory allocation error" ) ; return ; } } ( * tBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK . Head ] = simTime ; ( * uBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK . Head ] = _rtB -> B_7_145_0 ; } { real_T * * uBuffer
= ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_c . TUbufferPtrs [ 0 ] ;
real_T * * tBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_c .
TUbufferPtrs [ 1 ] ; real_T * * xBuffer = ( NULL ) ; real_T simTime = ssGetT
( S ) ; _rtDW -> VariableTimeDelay_IWORK_g . Head = ( ( _rtDW ->
VariableTimeDelay_IWORK_g . Head < ( _rtDW -> VariableTimeDelay_IWORK_g .
CircularBufSize - 1 ) ) ? ( _rtDW -> VariableTimeDelay_IWORK_g . Head + 1 ) :
0 ) ; if ( _rtDW -> VariableTimeDelay_IWORK_g . Head == _rtDW ->
VariableTimeDelay_IWORK_g . Tail ) { if ( !
PMSM_VC_current_loop_acc_rt_TDelayUpdateTailOrGrowBuf ( & _rtDW ->
VariableTimeDelay_IWORK_g . CircularBufSize , & _rtDW ->
VariableTimeDelay_IWORK_g . Tail , & _rtDW -> VariableTimeDelay_IWORK_g .
Head , & _rtDW -> VariableTimeDelay_IWORK_g . Last , simTime - _rtP -> P_51 ,
tBuffer , uBuffer , xBuffer , ( boolean_T ) 0 , ( boolean_T ) 0 , & _rtDW ->
VariableTimeDelay_IWORK_g . MaxNewBufSize ) ) { ssSetErrorStatus ( S ,
"vtdelay memory allocation error" ) ; return ; } } ( * tBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_g . Head ] = simTime ; ( * uBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_g . Head ] = _rtB -> B_7_144_0 ; } { real_T * *
uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_e . TUbufferPtrs
[ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_e . TUbufferPtrs [ 1 ] ; real_T * * xBuffer = ( NULL
) ; real_T simTime = ssGetT ( S ) ; _rtDW -> VariableTimeDelay_IWORK_c . Head
= ( ( _rtDW -> VariableTimeDelay_IWORK_c . Head < ( _rtDW ->
VariableTimeDelay_IWORK_c . CircularBufSize - 1 ) ) ? ( _rtDW ->
VariableTimeDelay_IWORK_c . Head + 1 ) : 0 ) ; if ( _rtDW ->
VariableTimeDelay_IWORK_c . Head == _rtDW -> VariableTimeDelay_IWORK_c . Tail
) { if ( ! PMSM_VC_current_loop_acc_rt_TDelayUpdateTailOrGrowBuf ( & _rtDW ->
VariableTimeDelay_IWORK_c . CircularBufSize , & _rtDW ->
VariableTimeDelay_IWORK_c . Tail , & _rtDW -> VariableTimeDelay_IWORK_c .
Head , & _rtDW -> VariableTimeDelay_IWORK_c . Last , simTime - _rtP -> P_53 ,
tBuffer , uBuffer , xBuffer , ( boolean_T ) 0 , ( boolean_T ) 0 , & _rtDW ->
VariableTimeDelay_IWORK_c . MaxNewBufSize ) ) { ssSetErrorStatus ( S ,
"vtdelay memory allocation error" ) ; return ; } } ( * tBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_c . Head ] = simTime ; ( * uBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_c . Head ] = _rtB -> B_7_143_0 ; } { real_T * *
uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_g . TUbufferPtrs
[ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_g . TUbufferPtrs [ 1 ] ; real_T * * xBuffer = ( NULL
) ; real_T simTime = ssGetT ( S ) ; _rtDW -> VariableTimeDelay_IWORK_gl .
Head = ( ( _rtDW -> VariableTimeDelay_IWORK_gl . Head < ( _rtDW ->
VariableTimeDelay_IWORK_gl . CircularBufSize - 1 ) ) ? ( _rtDW ->
VariableTimeDelay_IWORK_gl . Head + 1 ) : 0 ) ; if ( _rtDW ->
VariableTimeDelay_IWORK_gl . Head == _rtDW -> VariableTimeDelay_IWORK_gl .
Tail ) { if ( ! PMSM_VC_current_loop_acc_rt_TDelayUpdateTailOrGrowBuf ( &
_rtDW -> VariableTimeDelay_IWORK_gl . CircularBufSize , & _rtDW ->
VariableTimeDelay_IWORK_gl . Tail , & _rtDW -> VariableTimeDelay_IWORK_gl .
Head , & _rtDW -> VariableTimeDelay_IWORK_gl . Last , simTime - _rtP -> P_55
, tBuffer , uBuffer , xBuffer , ( boolean_T ) 0 , ( boolean_T ) 0 , & _rtDW
-> VariableTimeDelay_IWORK_gl . MaxNewBufSize ) ) { ssSetErrorStatus ( S ,
"vtdelay memory allocation error" ) ; return ; } } ( * tBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_gl . Head ] = simTime ; ( * uBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_gl . Head ] = _rtB -> B_7_142_0 ; } { real_T * *
uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_o . TUbufferPtrs
[ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_o . TUbufferPtrs [ 1 ] ; real_T * * xBuffer = ( NULL
) ; real_T simTime = ssGetT ( S ) ; _rtDW -> VariableTimeDelay_IWORK_a . Head
= ( ( _rtDW -> VariableTimeDelay_IWORK_a . Head < ( _rtDW ->
VariableTimeDelay_IWORK_a . CircularBufSize - 1 ) ) ? ( _rtDW ->
VariableTimeDelay_IWORK_a . Head + 1 ) : 0 ) ; if ( _rtDW ->
VariableTimeDelay_IWORK_a . Head == _rtDW -> VariableTimeDelay_IWORK_a . Tail
) { if ( ! PMSM_VC_current_loop_acc_rt_TDelayUpdateTailOrGrowBuf ( & _rtDW ->
VariableTimeDelay_IWORK_a . CircularBufSize , & _rtDW ->
VariableTimeDelay_IWORK_a . Tail , & _rtDW -> VariableTimeDelay_IWORK_a .
Head , & _rtDW -> VariableTimeDelay_IWORK_a . Last , simTime - _rtP -> P_57 ,
tBuffer , uBuffer , xBuffer , ( boolean_T ) 0 , ( boolean_T ) 0 , & _rtDW ->
VariableTimeDelay_IWORK_a . MaxNewBufSize ) ) { ssSetErrorStatus ( S ,
"vtdelay memory allocation error" ) ; return ; } } ( * tBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_a . Head ] = simTime ; ( * uBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_a . Head ] = _rtB -> B_7_141_0 ; } { real_T * *
uBuffer = ( real_T * * ) & _rtDW -> VariableTimeDelay_PWORK_p . TUbufferPtrs
[ 0 ] ; real_T * * tBuffer = ( real_T * * ) & _rtDW ->
VariableTimeDelay_PWORK_p . TUbufferPtrs [ 1 ] ; real_T * * xBuffer = ( NULL
) ; real_T simTime = ssGetT ( S ) ; _rtDW -> VariableTimeDelay_IWORK_d . Head
= ( ( _rtDW -> VariableTimeDelay_IWORK_d . Head < ( _rtDW ->
VariableTimeDelay_IWORK_d . CircularBufSize - 1 ) ) ? ( _rtDW ->
VariableTimeDelay_IWORK_d . Head + 1 ) : 0 ) ; if ( _rtDW ->
VariableTimeDelay_IWORK_d . Head == _rtDW -> VariableTimeDelay_IWORK_d . Tail
) { if ( ! PMSM_VC_current_loop_acc_rt_TDelayUpdateTailOrGrowBuf ( & _rtDW ->
VariableTimeDelay_IWORK_d . CircularBufSize , & _rtDW ->
VariableTimeDelay_IWORK_d . Tail , & _rtDW -> VariableTimeDelay_IWORK_d .
Head , & _rtDW -> VariableTimeDelay_IWORK_d . Last , simTime - _rtP -> P_59 ,
tBuffer , uBuffer , xBuffer , ( boolean_T ) 0 , ( boolean_T ) 0 , & _rtDW ->
VariableTimeDelay_IWORK_d . MaxNewBufSize ) ) { ssSetErrorStatus ( S ,
"vtdelay memory allocation error" ) ; return ; } } ( * tBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_d . Head ] = simTime ; ( * uBuffer ) [ _rtDW ->
VariableTimeDelay_IWORK_d . Head ] = _rtB -> B_7_140_0 ; } UNUSED_PARAMETER (
tid ) ; }
#define MDL_UPDATE
static void mdlUpdateTID4 ( SimStruct * S , int_T tid ) { UNUSED_PARAMETER (
tid ) ; } static void mdlInitializeSizes ( SimStruct * S ) { ssSetChecksumVal
( S , 0 , 3036744577U ) ; ssSetChecksumVal ( S , 1 , 1055025236U ) ;
ssSetChecksumVal ( S , 2 , 867054852U ) ; ssSetChecksumVal ( S , 3 ,
2875622468U ) ; { mxArray * slVerStructMat = NULL ; mxArray * slStrMat =
mxCreateString ( "simulink" ) ; char slVerChar [ 10 ] ; int status =
mexCallMATLAB ( 1 , & slVerStructMat , 1 , & slStrMat , "ver" ) ; if ( status
== 0 ) { mxArray * slVerMat = mxGetField ( slVerStructMat , 0 , "Version" ) ;
if ( slVerMat == NULL ) { status = 1 ; } else { status = mxGetString (
slVerMat , slVerChar , 10 ) ; } } mxDestroyArray ( slStrMat ) ;
mxDestroyArray ( slVerStructMat ) ; if ( ( status == 1 ) || ( strcmp (
slVerChar , "9.2" ) != 0 ) ) { return ; } } ssSetOptions ( S ,
SS_OPTION_EXCEPTION_FREE_CODE ) ; if ( ssGetSizeofDWork ( S ) != sizeof (
DW_PMSM_VC_current_loop_T ) ) { ssSetErrorStatus ( S ,
"Unexpected error: Internal DWork sizes do "
"not match for accelerator mex file." ) ; } if ( ssGetSizeofGlobalBlockIO ( S
) != sizeof ( B_PMSM_VC_current_loop_T ) ) { ssSetErrorStatus ( S ,
"Unexpected error: Internal BlockIO sizes do "
"not match for accelerator mex file." ) ; } { int ssSizeofParams ;
ssGetSizeofParams ( S , & ssSizeofParams ) ; if ( ssSizeofParams != sizeof (
P_PMSM_VC_current_loop_T ) ) { static char msg [ 256 ] ; sprintf ( msg ,
"Unexpected error: Internal Parameters sizes do "
"not match for accelerator mex file." ) ; } } _ssSetModelRtp ( S , ( real_T *
) & PMSM_VC_current_loop_rtDefaultP ) ; rt_InitInfAndNaN ( sizeof ( real_T )
) ; } static void mdlInitializeSampleTimes ( SimStruct * S ) { { SimStruct *
childS ; SysOutputFcn * callSysFcns ; childS = ssGetSFunction ( S , 0 ) ;
callSysFcns = ssGetCallSystemOutputFcnList ( childS ) ; callSysFcns [ 3 + 0 ]
= ( SysOutputFcn ) ( NULL ) ; } slAccRegPrmChangeFcn ( S , mdlOutputsTID4 ) ;
} static void mdlTerminate ( SimStruct * S ) { }
#include "simulink.c"
