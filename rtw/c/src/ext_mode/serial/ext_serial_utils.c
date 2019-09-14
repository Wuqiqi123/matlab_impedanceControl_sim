/*
 * Copyright 1994-2013 The MathWorks, Inc.
 *
 * File: ext_serial_utils.c     
 *
 * Abstract:
 *  External mode shared data structures and functions used by the external
 *  communication, mex link, and generated code.  This file is for definitions
 *  related to custom external mode implementations (e.g., tcpip, serial).
 *  See ext_share.h for definitions common to all implementations of external
 *  mode (ext_share.h should NOT be modified).
 */

/***************** TRANSPORT-DEPENDENT DEFS AND INCLUDES **********************/

#include <stdlib.h>
#include <string.h>

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
 * Buffers used for storing incoming and outgoing packets.
 */
PRIVATE ExtSerialPacket buffer1st;
PRIVATE ExtSerialPacket buffer2nd;
PRIVATE ExtSerialPacket *InBuffer  = &buffer1st;
PRIVATE ExtSerialPacket *OutBuffer = &buffer2nd;

/*
 * If waitForAck is true, a packet can not be sent until the other side sends
 * an ack indicating there is space to store the unsent packet.
 */
PRIVATE boolean_T waitForAck = false;

/*
 * The maximum number of bytes a serial packet may contain.  Serial pkts
 * larger than the maximum size will be broken up into multiple pkts
 * (each equal to or less than the max size) for transmission.
 *
 * Notes:
 *  o TARGET_SERIAL_RECEIVE_BUFFER_SIZE should be the same value for both target and host
 *  o A low value of 64 is chosen so this works out of the box for targets 
 *    that have a small serial receive buffer. For example, the arduino 
 *    target has a serial receive buffer of size 64.
 *  o If you reduce this value even further, you need to be careful not to
 *    end up with negative value for MAX_SERIAL_PAYLOAD_SIZE! If uint32_T
 *    is 4 bytes, TARGET_SERIAL_RECEIVE_BUFFER_SIZE should not be set below
 *    32.  
 */
#define TARGET_SERIAL_RECEIVE_BUFFER_SIZE 64

/* MAX_SERIAL_PAYLOAD_SIZE determines the maximum number of payload bytes a 
 * serial packet may contain. We divide by 2 to handle the worst case
 * scenario when all the bytes are escaped. See ext_serial_pkt.h for a 
 * description of the structure of a serial packet. With a serial receive
 * buffer size of 64, the payload size is typically (64-(14+14))/2 = 18 bytes
 *
 * Note: We subtract MAX_ACK_PACKET_SIZE to handle the case where we might 
 * end up sending a data packet + ACK packet down to the target (which mean
 * means that the serial receive buffer size must be big enough handle 
 * this). This scenario arises if the host sends a data packet down to the
 * target, and  the target sends a data packet up to the host. The host 
 * will need to ACK the target's data packet, so it sends an ACK packet.
 */
#define MAX_SERIAL_PAYLOAD_SIZE \
((TARGET_SERIAL_RECEIVE_BUFFER_SIZE - (MAX_ACK_PACKET_SIZE + MAX_NON_PAYLOAD_SIZE) )/2)

/*
 * The maximum number of (maximum sized) received packets that can be stored at
 * any one time.
 */
#define NUM_FIFO_BUFFERS 5

/* data structure for a single FIFO packet buffer element - see
 * FreeFIFO and PktFIFO below */
typedef struct FIFOBuffer_tag {
    char                  pktBuf[TARGET_SERIAL_RECEIVE_BUFFER_SIZE];
    uint32_T              size;
    uint32_T              offset;
    struct FIFOBuffer_tag *next;
} FIFOBuffer;

/* "Free FIFO" is the linked list of all free buffers. Initially, when
 * the serial connection is opened, NUM_FIFO_BUFFERS are allocated to 
 * Free FIFO.
 *
 * "Pkt FIFO" is the linked list of all received packets that are
 * yet to be processed by the application.   Initially, this list
 * is empty.
 *
 * ACKS for received packets are only transmitted so long as Free FIFO
 * is not empty - i.e. this side has somewhere left to store incoming 
 * packets.   This throttles back the other side because packets are only
 * transmitted if an ACK for the previous packet has been received.
 *
 * As packets are received, buffers are transferred from Free FIFO to 
 * Pkt FIFO.
 *
 * As packets are processed by the application, buffers are transferred
 * from Pkt FIFO to Free FIFO.
 */
PRIVATE FIFOBuffer *FreeFIFOHead = NULL;
PRIVATE FIFOBuffer *FreeFIFOTail = NULL;
PRIVATE FIFOBuffer *PktFIFOHead  = NULL;
PRIVATE FIFOBuffer *PktFIFOTail  = NULL;

PRIVATE boolean_T isFIFOEmpty(FIFOBuffer *head, FIFOBuffer *tail)
{
    if ((head == NULL) && (tail == NULL))
        return true;

    return false;
}

PRIVATE void InsertFIFO(FIFOBuffer **head, FIFOBuffer **tail, FIFOBuffer *buf)
{
    if (isFIFOEmpty(*head, *tail)) {
        *head = *tail = buf;
        buf->next = NULL;
    } else {
        (*tail)->next = buf;
        *tail = buf;
        buf->next = NULL;
    }
}

PRIVATE FIFOBuffer *RemoveFIFO(FIFOBuffer **head, FIFOBuffer **tail)
{
    FIFOBuffer *buf = NULL;

    if (isFIFOEmpty(*head, *tail)) {
        return NULL;
    } else {
        buf = *head;
        *head = buf->next;
        if (*head == NULL) *tail = NULL;
        buf->next = NULL;
    }

    return buf;
}

PRIVATE boolean_T isFIFOFreeEmpty(void)
{
    return isFIFOEmpty(FreeFIFOHead, FreeFIFOTail);
}

PRIVATE boolean_T isFIFOPktEmpty(void)
{
    return isFIFOEmpty(PktFIFOHead, PktFIFOTail);
}

PRIVATE void InsertFIFOFree(FIFOBuffer *buf)
{
    InsertFIFO(&FreeFIFOHead, &FreeFIFOTail, buf);
}

PRIVATE FIFOBuffer *RemoveFIFOFree(void)
{
    return RemoveFIFO(&FreeFIFOHead, &FreeFIFOTail);
}

PRIVATE void InsertFIFOPkt(FIFOBuffer *buf)
{
    InsertFIFO(&PktFIFOHead, &PktFIFOTail, buf);
}

PRIVATE FIFOBuffer *RemoveFIFOPkt(void)
{
    return RemoveFIFO(&PktFIFOHead, &PktFIFOTail);
}

PRIVATE FIFOBuffer *GetFIFOPkt(void)
{
    return PktFIFOHead;
}

/* create new buffer elements and add them to FIFO Free - 
 * called once initially and then during runtime during 
 * "deadlock" conditions to allow receipt of extra packets */
PRIVATE boolean_T AddToFIFOFree(void)
{
    int       i;
    boolean_T error = EXT_NO_ERROR;

    for (i=0 ; i<NUM_FIFO_BUFFERS ; i++) {
        FIFOBuffer *buf = calloc(1, sizeof(FIFOBuffer));
        if (buf == NULL) {
            error = EXT_ERROR;
            goto EXIT_POINT;
        }
        InsertFIFOFree(buf);
    }
  EXIT_POINT:
    return(error);
}

/* Take a buffer from Free FIFO, copy data into it
 * and add it to Pkt FIFO for later processing by the 
 * application */
PRIVATE void SavePkt(char *mem, int size)
{
    FIFOBuffer *buf = RemoveFIFOFree();

    memcpy(buf->pktBuf,mem,size);
    buf->size   = size;
    buf->offset = 0;

    InsertFIFOPkt(buf);
}


/***************** PRIVATE FUNCTIONS ******************************************/

/* Forward declaration */
PRIVATE boolean_T ExtSetPkt(ExtSerialPort*, const char*, int, int*, PacketTypeEnum);

/* Function: ExtGetPktBlocking =================================================
 * Abstract:
 *  Blocks until a packet is available on the comm line.  If the incoming packet
 *  is an ACK, the packet is processed and thrown away.  Otherwise, the packet
 *  is saved.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 */
PRIVATE boolean_T ExtGetPktBlocking(ExtSerialPort *portDev)
{
    boolean_T error = EXT_NO_ERROR;

    /* Block until a packet is available from the comm line. */
    error = GetExtSerialPacket(InBuffer, portDev);
    if (error != EXT_NO_ERROR) goto EXIT_POINT;

    /* Process ACK packets, don't pass on to application. */
    if (InBuffer->PacketType == ACK_PACKET) {
        waitForAck = false;
        goto EXIT_POINT;
    }

    /* store the packet in Pkt FIFO */
    SavePkt(InBuffer->Buffer, InBuffer->size);

    /* if we have some free space available in Free FIFO, send an ACK to allow
     * the other end to send another packet */
    if (!isFIFOFreeEmpty()) {
        int bytesWritten;

        error = ExtSetPkt(portDev, NULL, 0, &bytesWritten, ACK_PACKET);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;
    }

  EXIT_POINT:
    return error;

} /* end ExtGetPktBlocking */


/* Function: ExtSetPkt =========================================================
 * Abstract:
 *  Sets (sends) the specified number of bytes on the comm line.  As long as an
 *  error does not occur, this function is guaranteed to set the requested
 *  number of bytes.  The number of bytes set is returned via the 'nBytesSet'
 *  parameter.  If a previously sent packet has not yet received an ACK, then we
 *  block for the next packet which must be an ACK.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 */
PRIVATE boolean_T ExtSetPkt(ExtSerialPort *portDev,
                            const char *pktData,
                            int pktSize,
                            int *bytesWritten,
                            PacketTypeEnum pktType)
{
    int       bytesToSend;
    int       deadlockCntr;    
    boolean_T error        = EXT_NO_ERROR;
    /* Avoid compiler warning about comparing signed and unsigned values */
    unsigned int pktSize_u = (unsigned int)pktSize;

    bytesToSend = (pktSize_u > MAX_SERIAL_PAYLOAD_SIZE) ?
        MAX_SERIAL_PAYLOAD_SIZE : pktSize_u;
    *bytesWritten = bytesToSend;

    /*
     * Wait for an ACK packet if needed. Every packet sent must be ACKed before
     * another can be sent (the only exceptions are ACK packets which are never
     * ACKed).
     */
    deadlockCntr = 0;
    while (waitForAck && (pktType != ACK_PACKET)) {
       boolean_T dataPending = 0;

       error = ExtSerialPortDataPending(portDev, &dataPending);
       if (error != EXT_NO_ERROR) goto EXIT_POINT;

       if (dataPending) {
          /* NOTE: if ExtGetPktBlocking receives an ACK then
           * it will set waitForAck to false and allow us to 
           * exit from the while loop */
          error = ExtGetPktBlocking(portDev);
          if (error != EXT_NO_ERROR) goto EXIT_POINT;
       }

       /*
        * If we are in this loop, it means we are trying to send a pkt but
        * have not received an ACK from the other side.  If we don't get an
        * ACK after some amount of time, the other end may just be busy or 
        * if Free FIFO is empty (i.e. we have no space to receive more packets) 
        * we may be in a deadlock condition where both sides are waiting for an ACK.  
        * In this case, allocating some more free packets (to Free FIFO)
        * will break the deadlock condition.
        */
       if (isFIFOFreeEmpty()) {
          /* wait for many iterations before allocating 
           * more buffer space */
          if (deadlockCntr++ >= 200) {           
             int temp;
             deadlockCntr = 0;

             /* grow the buffer */
             error = AddToFIFOFree();
             if (error != EXT_NO_ERROR) goto EXIT_POINT;

             /* send ACK to break deadlock */
             error = ExtSetPkt(portDev, NULL, 0, &temp, ACK_PACKET);
             if (error != EXT_NO_ERROR) goto EXIT_POINT;
          }
       }
       else {
          /* whenever there is some buffer space available, 
           * reset the counter */
          deadlockCntr = 0;
       }
    }

    /*
     * Write the packet to the outgoing buffer. The output buffer is
     * guaranteed to be big enough to hold the maximum size packet.
     */
    OutBuffer->size = (uint32_T)bytesToSend;
    OutBuffer->PacketType = (char)pktType;
    memcpy(OutBuffer->Buffer,pktData,(uint32_T)bytesToSend);

    /* Send the packet over the comm line. */
    error = SetExtSerialPacket(OutBuffer,portDev);
    if (error != EXT_NO_ERROR) goto EXIT_POINT;

  EXIT_POINT:
    return error;

} /* end ExtSetPkt */


/* Function: ExtSetPktWithACK ==================================================
 * Abstract:
 *  Sets (sends) the specified number of bytes on the comm line and waits for a
 *  return packet.  As long as an error does not occur, this function is
 *  guaranteed to set the requested number of bytes.  The number of bytes set is
 *  returned via the 'nBytesSet' parameter.
 *
 *  If the return packet is an ACK, the ACK is processed and thrown away.  If
 *  the return packet is something other than an ACK, the packet is saved and
 *  processed and a global flag is set to indicate we are still waiting for an
 *  ACK packet.  A typical scenario where the return packet is not an ACK is
 *  when both the host and target send a packet to each other simultaneously.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 */
PRIVATE boolean_T ExtSetPktWithACK(ExtSerialPort *portDev,
                                   const char *pktData,
                                   int pktSize,
                                   PacketTypeEnum pktType)
{
    int       bytesWritten      = 0;
    int       totalBytesWritten = 0;
    boolean_T error             = EXT_NO_ERROR;

    while (totalBytesWritten < pktSize) {
        /* Send the packet. */
        error = ExtSetPkt(portDev, pktData+totalBytesWritten,
                          pktSize-totalBytesWritten, &bytesWritten, pktType);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        totalBytesWritten += bytesWritten;

        /* We must get an ACK back. */
        waitForAck = true;
    }

  EXIT_POINT:
    return error;

} /* end ExtSetPktWithACK */


/* Function: ExtGetPkt =========================================================
 * Abstract:
 *  Attempts to get the specified number of bytes from the comm line.  The
 *  number of bytes read is returned via the 'nBytesGot' parameter.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 *
 * NOTES:
 *  o it is not an error for 'nBytesGot' to be returned as 0
 *  o this function blocks if no data is available
 */
PRIVATE boolean_T ExtGetPkt(ExtSerialPort *portDev,
                            char *dst,
                            int nBytesToGet,
                            int *returnBytesGot)
{
    FIFOBuffer *buf;
    int        UnusedBytes;
    boolean_T  error = EXT_NO_ERROR;

    /* see if any received packets need processing in the buffer */
    buf = GetFIFOPkt();
    while (buf == NULL) {
        /* no packets in the buffer => wait for a packet */
        error = ExtGetPktBlocking(portDev);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        buf = GetFIFOPkt();
    }

    UnusedBytes = buf->size - buf->offset;
    /* Test packet size. */
    if (nBytesToGet >= UnusedBytes) {
	*returnBytesGot = UnusedBytes;
    } else {
	*returnBytesGot = nBytesToGet;
    }

    /* Save packet using char* for proper math. */
    {
	char *tempPtr = buf->pktBuf;
	tempPtr += buf->offset;
	(void)memcpy(dst, tempPtr, *returnBytesGot);
    }

    /* Determine if the packet can be discarded. */
    buf->offset += *returnBytesGot;

    if (buf->offset == buf->size) {
        int       bytesWritten;
        boolean_T isEmpty = isFIFOFreeEmpty();

        buf->size   = 0;
        buf->offset = 0;

        /* transfer buffer back to FIFO Free so it can be used
         * to store a new received packet */
        InsertFIFOFree(RemoveFIFOPkt());

        if (isEmpty) {
            /* FIFO Free only just became non empty so it is our job to 
             * send an ACK and indicate that there is no space for to receive
             * more data */
            error = ExtSetPkt(portDev, NULL, 0, &bytesWritten, ACK_PACKET);
            if (error != EXT_NO_ERROR) goto EXIT_POINT;
        }
    }

  EXIT_POINT:
    return error;

} /* end ExtGetPkt */


/* Function: ExtPktPending =====================================================
 * Abstract:
 *  Returns true, via the 'pending' arg, if data is pending on the comm line.
 *  Returns false otherwise.  If data is pending, the packet is read from the
 *  comm line.  If that packet is an ACK packet, false is returned.  If the
 *  packet is an extmode packet, it is saved and true is returned.
 *
 *  EXT_NO_ERROR is returned on success, EXT_ERROR on failure.
 */
PRIVATE boolean_T ExtPktPending(ExtSerialPort *portDev,
                                boolean_T *pending)
{
    boolean_T error = EXT_NO_ERROR;

    *pending = false;

    if (isFIFOPktEmpty()) {
        /*
         * Is there a pkt already waiting?  If so, return true for pending pkt.
         * Otherwise, try to grab a pkt from the comm line (if one exists).
         */
        boolean_T dataPending = 0;

        error = ExtSerialPortDataPending(portDev, &dataPending);
        if (error != EXT_NO_ERROR) goto EXIT_POINT;

        if (dataPending) {
            error = ExtGetPktBlocking(portDev);
            if (error != EXT_NO_ERROR) goto EXIT_POINT;

            /*
             * Only if the pkt is saved in the FIFO do we return a pkt is
             * pending.  If the acquired pkt was an ACK, it would have been
             * thrown away.
             */
            if (!isFIFOPktEmpty()) *pending = true;
        }
    } else {
        *pending = true;
    }

  EXIT_POINT:
    return error;

} /* end ExtPktPending */


/* Function: ExtClearSerialConnection ==========================================
 * Abstract:
 *  Clear the connection by setting certain global variables to their initial
 *  states.  The difference between ExtResetSerialConnection() and
 *  ExtClearSerialConnection() is that the reset function frees all allocated
 *  memory and nulls out all pointers.  The clear function only initializes
 *  some global variables without freeing any memory.  When the connection is
 *  being opened or closed, use the reset function.  If the host and target
 *  are only disconnecting, use the clear function.
 */
PRIVATE void ExtClearSerialConnection(void)
{
    waitForAck = false;

} /* end ExtClearSerialConnection */


/* Function: ExtResetSerialConnection ==========================================
 * Abstract:
 *  Reset the connection with the target by initializing some global variables
 *  and freeing/nulling all allocated memory.
 */
PRIVATE void ExtResetSerialConnection(void)
{
    while (!isFIFOFreeEmpty()) {
        free(RemoveFIFOFree());
    }

    while (!isFIFOPktEmpty()) {
        free(RemoveFIFOPkt());
    }

    free(InBuffer->Buffer);
    memset(InBuffer, 0, sizeof(ExtSerialPacket));

    free(OutBuffer->Buffer);
    memset(OutBuffer, 0, sizeof(ExtSerialPacket));

    PktFIFOHead  = PktFIFOTail  = NULL;
    FreeFIFOHead = FreeFIFOTail = NULL;

    ExtClearSerialConnection();

} /* end ExtResetSerialConnection */


/* Function: ExtOpenSerialConnection ===========================================
 * Abstract:
 *  Open the connection with the target.
 */
PRIVATE ExtSerialPort *ExtOpenSerialConnection(const int argc, const char ** argv)
{
    ExtSerialPort *portDev = NULL;
    uint32_T      maxSize  = TARGET_SERIAL_RECEIVE_BUFFER_SIZE;
    boolean_T     error    = EXT_NO_ERROR;

    portDev = ExtSerialPortCreate();
    if (portDev == NULL) goto EXIT_POINT;

    ExtResetSerialConnection();

    /*
     * Allocate the buffers for sending and receiving packets big enough to
     * hold the maximum size packet possible for transmission.
     */
    OutBuffer->Buffer = (char *)malloc(maxSize);
    if (OutBuffer->Buffer == NULL) {
        error = EXT_ERROR;
        goto EXIT_POINT;
    }
    OutBuffer->BufferSize = maxSize;

    InBuffer->Buffer = (char *)malloc(maxSize);
    if (InBuffer->Buffer == NULL) {
        error = EXT_ERROR;
        goto EXIT_POINT;
    }
    InBuffer->BufferSize = maxSize;

    /* set up FIFO Free with the initial 
     * allocation of buffers */
    error = AddToFIFOFree();
    if (error != EXT_NO_ERROR) goto EXIT_POINT;

    error = ExtSerialPortConnect(portDev, argc, argv);
    if (error != EXT_NO_ERROR) goto EXIT_POINT;

  EXIT_POINT:
    if (error != EXT_NO_ERROR) portDev = NULL;
    return portDev;

} /* end ExtOpenSerialConnection */


/* Function: ExtCloseSerialConnection ==========================================
 * Abstract:
 *  Close the connection with the target.
 */
PRIVATE boolean_T ExtCloseSerialConnection(ExtSerialPort *portDev)
{
    ExtResetSerialConnection();

    return(ExtSerialPortDisconnect(portDev));

} /* end ExtCloseSerialConnection */


/* [EOF] ext_serial_utils.c */
