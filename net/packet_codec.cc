#include <stdio.h>
#include <arpa/inet.h>  // htonl, ntohl
#include <google/protobuf/message.h> // Message
#include "packet_codec.h"


int32_t CPacketCodec::ParseHead(const char* buf, int32_t len, google::protobuf::Message* head)
{
    if ((NULL == buf) || (NULL == head) || (0 >= len))
    {
        return ERROR;
    }

    // check packet format
    if ((len < PACKET_MSG_LEN_MIN)
        || (buf[0] != PACKET_START)
        || (buf[len-1] != PACKET_END))
    {
        printf("parse head packet format error\n");
        return ERROR;
    }

    // check packet len
    int32_t head_len = ::ntohl(*(int32_t *)(buf + PACKET_HEAD_LEN_OFFSET));
    int32_t body_len = ::ntohl(*(int32_t *)(buf + PACKET_BODY_LEN_OFFSET));
    int32_t msg_len = head_len + body_len + PACKET_MSG_LEN_MIN;

    if ((head_len <= 0) || (body_len <= 0) || (msg_len != len))
    {
        printf("parse head packet len error\n");
        return ERROR;
    }

    std::string head_meta;
    head_meta.assign(buf + PACKET_HEAD_OFFSET, head_len);
    if (!head->ParseFromString(head_meta))
    {
        printf("parse head packet protobuf error\n");
        return ERROR;
    }

    return OK;
}

int32_t CPacketCodec::ParseBody(const char* buf, int32_t len, google::protobuf::Message* body)
{
    if ((NULL == buf) || (NULL == body) || (0 >= len))
    {
        return ERROR;
    }

    // check packet format
    if ((len < PACKET_MSG_LEN_MIN)
        || (buf[0] != PACKET_START)
        || (buf[len-1] != PACKET_END))
    {
        printf("parse body packet format error\n");
        return ERROR;
    }

    // check packet len
    int32_t head_len = ::ntohl(*(int32_t *)(buf + PACKET_HEAD_LEN_OFFSET));
    int32_t body_len = ::ntohl(*(int32_t *)(buf + PACKET_BODY_LEN_OFFSET));
    int32_t msg_len = head_len + body_len + PACKET_MSG_LEN_MIN;

    if ((head_len <= 0) || (body_len <= 0) || (msg_len != len))
    {
        printf("parse body packet len error\n");
        return ERROR;
    }
    
    std::string body_meta;
    body_meta.assign(buf + PACKET_HEAD_OFFSET + head_len, body_len);
    if (!body->ParseFromString(body_meta))
    {
        printf("parse body packet protobuf error\n");
        return ERROR;
    }

    return OK;
}

int32_t CPacketCodec::BuildPacket(const google::protobuf::Message* head, 
                                  const google::protobuf::Message* body, 
                                  char* send_buf, 
                                  int32_t& send_len)
{
    if ((NULL == head) || (NULL == body) || (NULL == send_buf))
    {
        return ERROR;
    }

    int32_t head_len = head->ByteSize();
    int32_t body_len = body->ByteSize();
    int32_t msg_len = head_len + body_len + PACKET_MSG_LEN_MIN;

    // check packet head
    if (!head->IsInitialized() || !head_len)
    {
        printf("build packet head error\n");
        return ERROR;
    }

    // check packet body
    if (!body->IsInitialized() || !body_len)
    {
        printf("build packet body error\n");
        return ERROR;
    }

    // check packet space
    if (send_len < msg_len)
    {
        printf("build packet buf not enough error\n");
        return ERROR;
    }

    std::string head_meta;
    std::string body_meta;
    head->SerializeToString(&head_meta);
    body->SerializeToString(&body_meta);
    assert(head_len == head_meta.size());
    assert(body_len == body_meta.size());

    // build packet: "(start_flag head_len body_len head body end_flag)"
    *(char *)(send_buf + 0)                         = PACKET_START;
    *(int32_t *)(send_buf + PACKET_HEAD_LEN_OFFSET) = ::htonl(head_len);
    *(int32_t *)(send_buf + PACKET_BODY_LEN_OFFSET) = ::htonl(body_len);    
    ::memcpy(send_buf + PACKET_HEAD_OFFSET, head_meta.c_str(), head_meta.size());
    ::memcpy(send_buf + PACKET_HEAD_OFFSET + head_len, body_meta.c_str(), body_meta.size());
    *(char *)(send_buf + msg_len - 1)               = PACKET_END;
    
    // assign packet full len
    send_len = msg_len;

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

    // check head_len and body_len
    int32_t head_len = ::ntohl(*(int32_t*)(buf + PACKET_HEAD_LEN_OFFSET));
    int32_t body_len = ::ntohl(*(int32_t*)(buf + PACKET_BODY_LEN_OFFSET));
    int32_t msg_len = head_len + body_len + PACKET_MSG_LEN_MIN;

    if ((head_len <= 0) || (body_len <= 0) || (msg_len <= PACKET_MSG_LEN_MIN))
    {
        printf("packet codec length error\n");
        return ERROR;
    }

    // need continue receive packet, just return 0
    if (len < msg_len)
    {
        return 0;
    }

    // check end flag
    if (buf[msg_len-1] != PACKET_END)
    {
        printf("packet codec end flag error\n");
        return ERROR;
    }

    return msg_len;
}

