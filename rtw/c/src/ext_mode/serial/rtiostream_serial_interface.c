/*
 * Copyright 2011-2014 The MathWorks, Inc.
 *
 * File: rtiostream_serial_interface.c     
 *
 * Abstract:
 *  The External Mode Serial Port is a logical object providing a standard
 *  interface between the external mode code and the physical serial port 
 *  through the rtiostream interface.
 *  The prototypes in the 'Visible Functions' section of this file provide
 *  the consistent front-end interface to external mode code.  The
 *  implementations of these functions provide the back-end interface to the
 *  physical serial port.  This layer of abstraction allows for minimal
 *  modifications to external mode code when the physical serial port is
 *  changed. The physical serial port functions are implemented by the 
 *  rtiostream serial interface.
 *
 *     ----------------------------------
 *     | Host/Target external mode code |
 *     ----------------------------------
 *                   / \
 *                  /| |\
 *                   | |
 *                   | |
 *                  \| |/
 *                   \ /  Provides a standard, consistent interface to extmode
 *     ----------------------------------
 *     | External Mode Serial Port      |
 *     ----------------------------------
 *                   / \  Function definitions specific to physical serial port
 *                  /| |\ (implemented by rtiostream interface)
 *                   | |
 *                   | |
 *                  \| |/
 *                   \ /
 *     ----------------------------------
 *     | HW/OS/Physical serial port     |
 *     ----------------------------------
 *
 *  See also ext_serial_pkt.c.
 */

#include <string.h>

#ifdef MATLAB_MEX_FILE
   #include "tmwtypes.h"
#else
   #include "rtwtypes.h"
#endif

#ifndef EXTMODE_DISABLEPRINTF  
#include <stdio.h>
#endif
        
#include <stdlib.h>
#include "ext_types.h"
#include "ext_share.h"
#include "ext_serial_port.h"
#include "ext_serial_pkt.h"
#include "rtiostream.h"
#include "rtiostream_utils.h"

/* Logical definitions */
#if (!defined(__cplusplus))
#  ifndef false
#   define false                       (0U)
#  endif
#  ifndef true
#   define true                        (1U)
#  endif
#endif

/* define SIZE_MAX if not already 
 * defined (e.g. by a C99 compiler) */
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/* This is used by ExtSerialPortDataPending to read and cache this number 
 * of bytes in case there is data on the comm line. Even the smallest 
 * external mode message (like the connect message) will at least be of 
 * size 8 bytes. Do not increase this value since the implementation of 
 * rtIOStreamRecv on the target side might block until it receives this 
 * number of bytes.
 */
#define PENDING_DATA_CACHE_SIZE 8
      
typedef struct UserData_tag {
    int streamID;
    char pendingRecvdData[PENDING_DATA_CACHE_SIZE];
    uint8_T numPending;
    uint8_T startIdxPending;
} UserData;

static UserData *UD;
    
/***************** VISIBLE FUNCTIONS ******************************************/

/* Function: ExtSerialPortCreate ===============================================
 * Abstract:
 *  Creates an External Mode Serial Port object.  The External Mode Serial Port
 *  is an abstraction of the physical serial port providing a standard
 *  interface for external mode code.  A pointer to the created object is
 *  returned.
 *
 */
PUBLIC ExtSerialPort *ExtSerialPortCreate(void)
{
    static ExtSerialPort serialPort;
    ExtSerialPort *portDev = &serialPort;

    /* Determine and save Endianess. */
    {
        union Char2Integer_tag
        {
            int IntegerMember;
            char CharMember[sizeof(int)];
        } temp;

        temp.IntegerMember = 1;
        if (temp.CharMember[0] != 1)
            portDev->isLittleEndian = false;
        else
            portDev->isLittleEndian = true;
    }

    portDev->fConnected = false;

    return portDev;

} /* end ExtSerialPortCreate */


/* Function: ExtSerialPortConnect ==============================================
 * Abstract:
 *  Performs a logical connection between the external mode code and the
 *  External Mode Serial Port object and a real connection between the External
 *  Mode Serial Port object and the physical serial port.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 */
PUBLIC boolean_T ExtSerialPortConnect(ExtSerialPort *portDev,
                                      const int argc,
                                      const char ** argv)
{
    boolean_T error = EXT_NO_ERROR;
        
    if (portDev->fConnected) {
        error = EXT_ERROR;
        goto EXIT_POINT;
    }

    /* allocate memory for UserData */
    UD = (UserData *)calloc(1UL, sizeof(UserData));
    if (UD==NULL) {
        error = EXT_ERROR;
        goto EXIT_POINT;
    }
    
    /* Initialize number of pending (cached) units of data */
    UD->numPending = 0;
    UD->startIdxPending = 0;

    portDev->fConnected = true;

    UD->streamID = rtIOStreamOpen(argc, (void *)argv);        
    if (UD->streamID == RTIOSTREAM_ERROR) {
        portDev->fConnected = false;
        error = EXT_ERROR;
        goto EXIT_POINT;
    }
    
  EXIT_POINT:
    return error;

} /* end ExtSerialPortConnect */


/* Function: ExtSerialPortDisconnect ===========================================
 * Abstract:
 *  Performs a logical disconnection between the external mode code and the
 *  External Mode Serial Port object and a real disconnection between the
 *  External Mode Serial Port object and the physical serial port.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 */
PUBLIC boolean_T ExtSerialPortDisconnect(ExtSerialPort *portDev)
{
   boolean_T error = EXT_NO_ERROR;
   int_T result;       

   if (!portDev->fConnected) return EXT_ERROR;
   
   portDev->fConnected = false;
   
   result = rtIOStreamClose(UD->streamID);         

   if (UD != NULL) free(UD);

   if (result == RTIOSTREAM_ERROR) {
        error = EXT_ERROR;
    }
   
    return(error);

} /* end ExtSerialPortDisconnect */


/* Function: ExtSerialPortSetData ==============================================
 * Abstract:
 *  Sets (sends) the specified number of bytes on the comm line.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 */
PUBLIC boolean_T ExtSerialPortSetData(ExtSerialPort *portDev,
                                      char *data,
                                      uint32_T size)
{    
    boolean_T errorCode = EXT_NO_ERROR;
    int_T rtIOStreamErrorStatus;   

    if (!portDev->fConnected) return EXT_ERROR;

    /* Blocks until all requested outgoing data is sent */
    rtIOStreamErrorStatus = rtIOStreamBlockingSend(UD->streamID,
                                                   (const void * const) data,
                                                   size);
    
    if (rtIOStreamErrorStatus == RTIOSTREAM_ERROR) {
        errorCode = EXT_ERROR;
    }
    
    return(errorCode);

} /* end ExtSerialPortSetData */


/* Function: ExtSerialPortDataPending ==========================================
 * Abstract:
 * Returns true, via the 'pending' arg, if data is pending on the comm line.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 */
PUBLIC boolean_T ExtSerialPortDataPending(ExtSerialPort *portDev,
                                          boolean_T *pending)
{
    boolean_T errorCode = EXT_NO_ERROR;
    int_T result;
    size_t sizeRecvd=0;
    if (!portDev->fConnected) return EXT_ERROR;
    
    if (UD->numPending > 0) {
        *pending = 1;
        return errorCode;
    } else {
        *pending = 0;
    }

     /* Call rtIOStreamRecv */
      result = rtIOStreamRecv(UD->streamID,
                                UD->pendingRecvdData,
                                (const size_t)PENDING_DATA_CACHE_SIZE,
                                &sizeRecvd);
               
    if (result == RTIOSTREAM_ERROR) {
        errorCode = EXT_ERROR;
        return errorCode;
    }

      if (sizeRecvd>0) {
          *pending = 1;
          UD->numPending = (uint8_T)sizeRecvd;
          UD->startIdxPending = 0;
      }

    return errorCode;    

} /* end ExtSerialPortDataPending */


/* Function: ExtSerialPortGetData ===========================================
 * Abstract:
 *  Attempts to get the requested data from the comm line.  As long as no 
 *  error happens, this function will loop to get all the requested data.
 *  The number of bytes read is returned via the 'bytesRead' parameter.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 *
 */
PUBLIC boolean_T ExtSerialPortGetData(ExtSerialPort *portDev,
                                         char *dst,
                                         uint32_T bytesToRead,
                                         uint32_T *bytesRead)
{
    boolean_T errorCode = EXT_NO_ERROR;
    int_T result = RTIOSTREAM_NO_ERROR;      
    size_t numFromCache = 0;
    uint32_T bytesToReadTemp = bytesToRead;
    *bytesRead = 0;
    
    if (!portDev->fConnected) return EXT_ERROR;
    
    if (UD->numPending > 0) {
        numFromCache = MIN((size_t)bytesToReadTemp, (size_t)UD->numPending);
        
        memcpy(dst, &(UD->pendingRecvdData[UD->startIdxPending]), numFromCache);
        UD->numPending -= (uint8_T) numFromCache;
        UD->startIdxPending += (uint8_T) numFromCache;
        
        dst += numFromCache;
        bytesToReadTemp -= (uint32_T) numFromCache;        
    }
        
    /* Call rtIOStreamBlockingRecv */
    result =  rtIOStreamBlockingRecv(UD->streamID,
                                    (void * const) dst,
                                    bytesToReadTemp);
    
    if (result == RTIOSTREAM_ERROR) {
        errorCode = EXT_ERROR;        
    } else {
        *bytesRead = bytesToRead;     
    }
    
    return errorCode;        

} /* end ExtSerialPortGetData */

/* [EOF] rtiostream_serial_interface.c */

/* LocalWords:  extmode Recv
 */
