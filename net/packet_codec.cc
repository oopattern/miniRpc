#include <stdio.h>
#include <arpa/inet.h>  // htonl, ntohl
#include <google/protobuf/message.h>    // Message
#include "packet_codec.h"


int32_t CPacketCodec::BuildPacket(google::protobuf::Message* msg, char* send_buf, int32_t& send_len)
{
    if (NULL == msg || NULL == send_buf)
    {
        return ERROR;
    }

    int32_t msg_len = msg->ByteSize();
    if ((!msg->IsInitialized()) || (!msg_len))
    {
        printf("build packet msg_len=%d error\n", msg_len);
        return ERROR;
    }
    
    int32_t need_len = msg_len + PACKET_MSG_LEN_MIN;
    if (send_len < need_len)
    {
        printf("build packet buf not enough error\n");
        return ERROR;
    }    

    std::string meta;
    msg->SerializeToString(&meta);

    assert(msg_len == meta.size());

    // build packet: "(start_flag msg_len msg_body end_flag)"
    *(char *)(send_buf + 0) = PACKET_START;     
    *(int32_t *)(send_buf + 1) = ::htonl(msg_len);
    ::memcpy(send_buf + 5, meta.c_str(), meta.size());
    *(send_buf + need_len - 1) = PACKET_END;    

    send_len = need_len;
    
    return OK;
}

// format: "(start_flag msg_len msg_body end_flag)"
int32_t CPacketCodec::CheckPacket(const char* buf, int32_t len)
{
    if ((NULL == buf) || (len <= 0))
    {
        printf("packet codec invalid len=%d\n", len);
        return ERROR;
    }

    // need continue receive packet, just return 0
    if (len <= PACKET_MSG_LEN_MIN)
    {        
        return 0;
    }

    // check start flag
    if (buf[0] != PACKET_START)
    {
        printf("packet codec start flag error\n");
        return ERROR;
    }

    // check msg len
    int32_t msg_len = ::ntohl(*(int32_t*)(buf + 1));    
    if (msg_len <= 0)
    {
        printf("packet codec length error\n");
        return ERROR;
    }

    // need continue receive packet, just return 0
    int32_t need_len = msg_len + PACKET_MSG_LEN_MIN;
    if (len < need_len)
    {
        return 0;
    }

    // check end flag
    if (buf[need_len-1] != PACKET_END)
    {
        printf("packet codec end flag error\n");
        return ERROR;
    }

    return need_len;
}

