#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "xmodule.h"

#define MSG_TYPE_TIMER1         (MSG_TYPE_USER_START + 1)
#define MSG_TYPE_USER_CFG       (MSG_TYPE_USER_START + 2)

int timer1_msg_proc(DEVM_MSG_S *rx_msg)
{
    if (!rx_msg) return VOS_ERR;
    printf("%s: %s \n", __FUNCTION__, rx_msg->msg_payload);

    return VOS_OK;
}

void* demo_main_task(void *param)  
{
    devm_set_msg_func(MSG_TYPE_TIMER1, timer1_msg_proc);

    while(1) {
        //todo

        vos_msleep(100);
    }
    
    return NULL;
}

int demo_timer_1(void *param)
{
    char usr_msg[512];
    static int timer_cnt = 0;

    snprintf(usr_msg, 512, "%s %d", get_app_name(), timer_cnt++);
    app_send_msg(0, get_app_name(), MSG_TYPE_TIMER1, usr_msg, strlen(usr_msg) + 1);
    app_send_msg(0, "jerry", MSG_TYPE_TIMER1, usr_msg, strlen(usr_msg) + 1);
    
    return VOS_OK;
}

TIMER_INFO_S my_timer_list[] = 
{
    {1, 30, 0, demo_timer_1, NULL}, 
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
    char *cfg_file = "./top_cfg.json";
    int ret = VOS_OK;
    pthread_t threadid;
    timer_t timer_id;

    if (access(cfg_file, F_OK) != 0) {
        if (argc < 2) {
            xlog(XLOG_ERROR, "no cfg file\r\n");
            return VOS_ERR;
        }
        cfg_file = argv[1];  //as init cfg
    }
    xmodule_init(cfg_file);

    ret = pthread_create(&threadid, NULL, demo_main_task, NULL);  
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
        return VOS_ERR;  
    } 

    ret = vos_create_timer(&timer_id, 1, demo_timer_callback, NULL);
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "vos_create_timer failed");
        return -1;  
    } 
    
    while(1) sleep(1);
}


