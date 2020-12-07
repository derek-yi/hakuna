
#ifndef _DEVM_MSG_H_
#define _DEVM_MSG_H_

#define DEF_LISTEN_PORT             7100

#define MSG_TYPE_ECHO               0x01
#define MSG_TYPE_RCMD               0x02
#define MSG_TYPE_MAX                0x20

#define MSG_HEAD_LEN                128

#define APP_NAME_LEN                32
typedef struct {
    int  src_ip;
    char src_app[APP_NAME_LEN];
    char dst_app[APP_NAME_LEN];
    char resv[32]; //keep MSG_HEAD_LEN 128

//assign by user&app:
    int  msg_type;
    int  sub_cmd;
    int  param[4];
    int  payload_len;
    char msg_payload[512];
}DEVM_MSG_S;



typedef int (*msg_func)(DEVM_MSG_S *rx_msg);

int devm_set_msg_func(int msg_type, msg_func func);

int devm_msg_init(char *app_name, int listen_port);


#endif



