#include <stdio.h>
#include <stdint.h>
#include "net_common.h"

// define packet format
// format: "(start_flag head_len body_len head_msg body_msg end_flag)"
#define PACKET_START            0x28
#define PACKET_END              0x29
#define PACKET_MSG_LEN_MIN      10   
#define PACKET_HEAD_LEN_OFFSET  (1)
#define PACKET_BODY_LEN_OFFSET  (1+4)
#define PACKET_HEAD_OFFSET      (PACKET_MSG_LEN_MIN-1)

// packet max size: 64Kb
#define PACKET_BUF_SIZE         (64*1024)


class CPacketCodec
{
public:
    static int32_t ParseHead(const char* buf, int32_t len, google::protobuf::Message* head);
    static int32_t ParseBody(const char* buf, int32_t len, google::protobuf::Message* body);
    static int32_t CheckPacket(const char* buf, int32_t len);
    static int32_t BuildPacket(const google::protobuf::Message* head, 
                               const google::protobuf::Message* body,
                               char* send_buf, 
                               int32_t& send_len);
};
