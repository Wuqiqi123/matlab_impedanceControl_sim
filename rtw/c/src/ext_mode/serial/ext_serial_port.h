/*
 * Copyright 1994-2012 The MathWorks, Inc.
 *
 * File: ext_serial_port.h     
 *
 * Abstract:
 *  Function prototypes for the External Mode Serial Port object.
 */

#ifndef __EXT_SERIAL_PORT__
#define __EXT_SERIAL_PORT__

typedef struct ExtSerialPort_tag
{
    boolean_T isLittleEndian; /* Endianess of target. */
    boolean_T fConnected;     /* Connected or not.    */
} ExtSerialPort;

extern ExtSerialPort *ExtSerialPortCreate       (void);
extern void          ExtSerialPortDestroy       (ExtSerialPort *portDev);
extern boolean_T     ExtSerialPortConnect       (ExtSerialPort *portDev,
                                                 const int argc,
                                                 const char ** argv);
extern boolean_T     ExtSerialPortDisconnect    (ExtSerialPort *portDev);
extern boolean_T     ExtSerialPortSetData       (ExtSerialPort *portDev,
                                                 char *data,
                                                 uint32_T size);
extern boolean_T     ExtSerialPortDataPending   (ExtSerialPort *portDev,
                                                 boolean_T *pending);
extern boolean_T     ExtSerialPortGetData       (ExtSerialPort *portDev,
                                                 char *c,
                                                 uint32_T bytesToRead,
                                                 uint32_T *bytesRead);

#endif /* __EXT_SERIAL_PORT__ */

/* [EOF] ext_serial_port.h */
