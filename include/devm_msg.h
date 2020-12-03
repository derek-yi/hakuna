
#ifndef _DEVM_MSG_H_
#define _DEVM_MSG_H_

#define MSG_TYPE_ECHO               0x01
#define MSG_TYPE_HWMON              0x02
#define MSG_TYPE_UPCFG              0x03
#define MSG_TYPE_MAX                0x20

#define MSG_HEAD_LEN                20

typedef struct {
    int  src_ip;
    int  src_mod[32];
    int  dst_mod[32];
    int  msg_type;
    int  sub_cmd;
    int  param[8];
    
    int  payload_len;
    char msg_payload[512];
}DEVM_MSG_S;



typedef int (*msg_func)(DEVM_MSG_S *rx_msg);

int devm_set_msg_func(int msg_type, msg_func func);

int devm_msg_init(char *mod_name, int listen_port);


#endif



