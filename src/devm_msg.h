
#ifndef _DEVM_MSG_H_
#define _DEVM_MSG_H_

#ifdef APP_TEST

#define VOS_OK      0
#define VOS_ERR     (-1)

#define xlog(x, fmt, args...)   printf(fmt, ##args)
#define vos_msleep(x)           usleep((x)*1000)

#endif


#define APP_ORAN_MP                 "/tmp/oran.mp"
#define APP_ORAN_DAEMON             "/tmp/oran.daemon"

#define MSG_TYPE_ECHO               0x01
#define MSG_TYPE_HWMON              0x02
#define MSG_TYPE_UPCFG              0x03
#define MSG_TYPE_MAX                0x20

#define MSG_HEAD_LEN                20

typedef struct {
    int  serial_num;
    int  msg_type;
    int  ack_value;
    int  need_ack;
    
    int  payload_len;
    char msg_payload[256];
}DEVM_MSG_S;



typedef int (*msg_func)(DEVM_MSG_S *rx_msg, DEVM_MSG_S* tx_msg);

int devm_set_msg_func(int msg_type, msg_func func);

int devm_send_msg(char *app_id, DEVM_MSG_S *tx_msg, DEVM_MSG_S *rx_msg);

int devm_msg_init(char *cfg_file);


#endif



