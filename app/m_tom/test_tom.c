
#include "xmodule.h"

#define MSG_TYPE_TIMER1         (MSG_TYPE_USER_START + 1)
#define MSG_TYPE_USER_CFG       (MSG_TYPE_USER_START + 2)

typedef struct {
    int  cmd;
    int  ret;
    char buff[256];
}USER_MSG_S;

int timer1_msg_proc(DEVM_MSG_S *rx_msg)
{
    if (!rx_msg) return VOS_ERR;
    printf("%s: %s \n", __FUNCTION__, rx_msg->msg_payload);

    return VOS_OK;
}

int cli_rpc_call_test(int argc, char **argv)
{
    int ret;
    USER_MSG_S tx_msg;
    USER_MSG_S rx_msg;

    if (argc < 2) {
        vos_print("usage: <%s> <dst_mod> \r\n", argv[0]);
        return CMD_ERR_PARAM;
    }

    tx_msg.cmd = 0x1;
    ret = app_rpc_call(0, argv[1], (char *)&tx_msg, sizeof(tx_msg), (char *)&rx_msg, sizeof(rx_msg));
    if (ret != VOS_OK) {
        vos_print("rpc failed\r\n");
        return CMD_ERR;
    }
    vos_print("rx_msg: %s \r\n", rx_msg.buff);

    return VOS_OK;
}

int usr_rpc_call_proc(void *rx_data, void *tx_data, int *tx_len)
{
    USER_MSG_S *rx_msg = (USER_MSG_S *)rx_data;
    USER_MSG_S *tx_msg = (USER_MSG_S *)tx_data;

    if (rx_msg->cmd == 0x1) {
        fmt_time_str(tx_msg->buff, 64);
        strcat(tx_msg->buff, get_app_name());
        *tx_len = sizeof(USER_MSG_S);
    }
    
    return VOS_OK;
}

void* demo_main_task(void *param)  
{
    rpc_set_callback(usr_rpc_call_proc);
    devm_set_msg_func(MSG_TYPE_TIMER1,      timer1_msg_proc);
    
    cli_cmd_reg("rcall",      "rpc call test",            &cli_rpc_call_test);

    while(1) {
        //todo

        vos_msleep(100);
    }
    
    return NULL;
}

int demo_timer_1(void *param)
{
    char usr_data[128];
    static int timer_cnt = 0;

    snprintf(usr_data, 128, "%s %d", get_app_name(), timer_cnt++);
    app_send_msg(0, get_app_name(), MSG_TYPE_TIMER1, usr_data, strlen(usr_data) + 1);
    //app_send_msg(0, "jerry", MSG_TYPE_TIMER1, usr_data, strlen(usr_data) + 1);
    
    return VOS_OK;
}

TIMER_INFO_S my_timer_list[] = 
{
    {1, 30, 0, demo_timer_1, NULL}, //debug only
};

int demo_timer_callback(void *param)
{
    static uint32 timer_cnt = 0;
    
    if (sys_conf_geti("demo_timer_disable")) {
        return VOS_OK;
    }
    
    timer_cnt++;
    for (int i = 0; i < sizeof(my_timer_list)/sizeof(TIMER_INFO_S); i++) {
        if ( (my_timer_list[i].enable) && (timer_cnt%my_timer_list[i].interval == 0) ) {
            my_timer_list[i].run_cnt++;
            if (my_timer_list[i].cb_func) {
                my_timer_list[i].cb_func(my_timer_list[i].cookie);
            }
        }
    }
    
    return VOS_OK;
}

int main(int argc, char **argv)
{
    char *init_cfg = NULL;
    int ret;
    pthread_t threadid;
    timer_t timer_id;

    if (argc > 1) init_cfg = argv[1];
    ret = xmodule_init(init_cfg);
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "xmodule_init failed");
        return VOS_ERR;  
    } 

    ret = pthread_create(&threadid, NULL, demo_main_task, NULL);  
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
        return VOS_ERR;  
    } 

    ret = vos_create_timer(&timer_id, 1, demo_timer_callback, NULL);
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "vos_create_timer failed");
        return VOS_ERR;  
    } 
    
    //while(1) sleep(1);
    pthread_join(threadid, NULL);
    return VOS_OK;
}


