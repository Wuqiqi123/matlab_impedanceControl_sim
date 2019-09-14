/*
 * Copyright 1994-2003 The MathWorks, Inc.
 *
 * File: ext_serial_pkt.h     
 *
 * Abstract:
 *  Function prototypes for the External Mode Serial Packet object.
 */

#ifndef __EXT_SERIAL_PKT__
#define __EXT_SERIAL_PKT__

/* Fixed sizes */
#define HEAD_SIZE        2
#define TAIL_SIZE        2
#define	PACKET_TYPE_SIZE 1

/* The following diagram shows the packet structure that the external mode 
 * serial ACKs layer uses to send/receive packets to/from the serial comm 
 * line:
 *
 * -----------------------------------------------------------------------------
 * |   Head    | Packet Type | Payload Size |       Payload           |  Tail   |
 * -----------------------------------------------------------------------------
 * | 2 bytes   |    1 byte   |    4 bytes   | MAX_SERIAL_PAYLOAD_SIZE | 2 bytes |
 * -----------------------------------------------------------------------------
 *
 * Note that each char (except for head and tail chars) within a packet 
 * might be escaped according to the HDLC Framing standard.
 */

/* MAX_NON_PAYLOAD_SIZE is the maximum size of all the fields of the packet 
 * except the payload. 
 * Note that we multiply by 2 for the PACKET_TYPE_SIZE and size of 
 * bufferSize to handle the worst case scenario where these get escaped. 
 * If uint32_T is 4 bytes, MAX_NON_PAYLOAD_SIZE would be 14 bytes.
 */
#define MAX_NON_PAYLOAD_SIZE (HEAD_SIZE + TAIL_SIZE) + 2*(PACKET_TYPE_SIZE + sizeof(uint32_T))

/* An ACK packet is a normal packet but with 0 bytes for payload */
#define MAX_ACK_PACKET_SIZE   MAX_NON_PAYLOAD_SIZE

/* Conform to HDLC Framing standard */
#define packet_head      ((char)0x7e)
#define packet_tail      ((char)0x03)
#define mask_character   ((char)0x20)
#define escape_character ((char)0x7d)
#define xon_character	 ((char)0x13)
#define xoff_character	 ((char)0x11)

enum ExtSerialPacketState {
    ESP_NoPacket, 
    ESP_InHead,
    ESP_InType,
    ESP_InSize,
    ESP_InPayload,
    ESP_InTail,
    ESP_Complete
};

typedef enum PacketTypes_tag
{
   UNDEFINED_PACKET = 0,
   EXTMODE_PACKET,
   ACK_PACKET
} PacketTypeEnum;

typedef union numString_tag {
    uint32_T num;
    char string[sizeof(uint32_T)];
} numString;

typedef struct ExtSerialPacket_tag
{
    char      head[HEAD_SIZE];
    char      PacketType;
    uint32_T  size;
    char      *Buffer;
    uint32_T  BufferSize;
    char      tail[TAIL_SIZE];
    int       state;
    char      *cursor;
    uint32_T  DataCount;
    boolean_T inQuote;
} ExtSerialPacket;

extern boolean_T SetExtSerialPacket(ExtSerialPacket *pkt,
                                    ExtSerialPort *portDev);
extern boolean_T GetExtSerialPacket(ExtSerialPacket *pkt,
                                    ExtSerialPort *portDev);

#endif /* __EXT_SERIAL_PKT__ */

/* [EOF] ext_serial_pkt.h */
