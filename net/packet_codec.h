#include <stdio.h>
#include <stdint.h>
#include "net_common.h"

// define packet format
// format: "(start_flag msg_len msg_body end_flag)"
#define PACKET_START            0x28
#define PACKET_END              0x29
#define PACKET_MSG_LEN_MIN      6   

// packet max size: 64Kb
#define PACKET_BUF_SIZE         (64*1024)


class CPacketCodec
{
public:
    static int32_t CheckPacket(const char* buf, int32_t len);
    static int32_t BuildPacket(google::protobuf::Message* msg, char* send_buf, int32_t& send_len);
};
