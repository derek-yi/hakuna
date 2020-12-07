
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "xmodule.h"
#include "xlog.h"
#include "devm_msg.h"
#include "tiny_cli.h"


#define MAX_CONNECT_NUM 32

msg_func msg_func_list[MSG_TYPE_MAX] = {0};

typedef struct {
    int  sock_id;
    int  used;
    int  dst_ip;
    char *app_name;  //as module_name
}SOCK_INFO;

SOCK_INFO sock_list[MAX_CONNECT_NUM] = {0};

sem_t tx_sem;

int local_ip_addr = 0;

int devm_msg_forward(DEVM_MSG_S *tx_msg);

int get_local_ip(char *if_name)
{
    int inet_sock;  
    struct ifreq ifr;  

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);  
    strcpy(ifr.ifr_name, if_name);  
    if ( ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0) {
        return -1;
    }
    
    return (int)((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
}

int devm_set_msg_func(int msg_type, msg_func func)
{
    if (msg_type >= MSG_TYPE_MAX) return VOS_ERR;

    msg_func_list[msg_type] = func;
    
    return VOS_OK;
}

void* common_rx_task(void *param)  
{
    long int temp_val = (long int)param; //suppress warning
    int socket_id = (int)temp_val;
    
    while(1) {
        DEVM_MSG_S rx_msg;
        
        int ret = recv(socket_id, &rx_msg, sizeof(DEVM_MSG_S), 0);
        if (ret < MSG_HEAD_LEN) {
            xlog(XLOG_ERROR, "recv failed(%s) \n", strerror(errno));
            break;
        }

        if (strcmp(rx_msg.dst_app, get_app_name())) {
            if (sys_conf_geti("app_role")) { 
                ret = devm_msg_forward(&rx_msg);
                xlog(XLOG_ERROR, "forward msg to %s, ret %d\n", rx_msg.dst_app, ret);
            } else {
                xlog(XLOG_ERROR, "drop msg on wrong route\n");
            }
            continue;
        }
        
        xlog(XLOG_DEBUG, "%s:%d: new msg %d \n", __FILE__, __LINE__, rx_msg.msg_type);
        if ( (rx_msg.msg_type < MSG_TYPE_MAX) && (msg_func_list[rx_msg.msg_type] != NULL) ) {
            msg_func_list[rx_msg.msg_type](&rx_msg);
        }
    }
    
    close(socket_id);
    return NULL;
}

void* uds_listen_task(void *param)  
{
    int fd,new_fd,ret;
    struct sockaddr_un un;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        xlog(XLOG_ERROR, "socket failed(%s) \n", strerror(errno));
        return NULL;
    }
    
    un.sun_family = AF_UNIX;
    unlink(get_app_name());
    strcpy(un.sun_path, get_app_name());
    if (bind(fd, (struct sockaddr *)&un, sizeof(un)) <0 ) {
        xlog(XLOG_ERROR, "bind failed(%s) \n", strerror(errno));
        return NULL;
    }
    
    if (listen(fd, MAX_CONNECT_NUM) < 0) {
        xlog(XLOG_ERROR, "listen failed(%s) \n", strerror(errno));
        return NULL;
    }

    xlog(XLOG_DEBUG, "uds_listen_task: listen %s \n", get_app_name());
    while(1){
        pthread_t unused_tid;
        long int temp_val; //suppress warning
        
        new_fd = accept(fd, NULL, NULL);
        if (new_fd < 0) {
            xlog(XLOG_ERROR, "accept failed(%s) \n", strerror(errno));
            continue;
        }

        xlog(XLOG_DEBUG, "uds_listen_task: new_fd %d \n", new_fd);
        temp_val = new_fd;
        ret = pthread_create(&unused_tid, NULL, common_rx_task, (void *)temp_val);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s) \n", strerror(errno));
            close(new_fd);
            continue;
        } 
    }
    
    close(fd);
    return NULL;
}

void* inet_listen_task(void *param)  
{
    int listen_fd, ret;
    struct sockaddr_in inet_addr;

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0) {
        xlog(XLOG_ERROR, "socket failed(%s) \n", strerror(errno));
        return NULL;
    }
    
    memset(&inet_addr, 0, sizeof(inet_addr));
    inet_addr.sin_family =  AF_INET; 
    inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_addr.sin_port = htons(DEF_LISTEN_PORT);
    if (bind(listen_fd,(struct sockaddr *)&inet_addr, sizeof(inet_addr)) < 0 ) {
        xlog(XLOG_ERROR, "bind failed(%s) \n", strerror(errno));
        return NULL;
    }
    
    if ( listen(listen_fd, MAX_CONNECT_NUM) < 0){
        xlog(XLOG_ERROR, "listen failed(%s) \n", strerror(errno));
        return NULL;
    }

    xlog(XLOG_DEBUG, "inet_listen_task: listen %d \n", DEF_LISTEN_PORT);
    while(1){
        pthread_t unused_tid;
        long int temp_val; //suppress warning
        int new_fd;
        
        new_fd = accept(listen_fd, NULL, NULL);
        if (new_fd < 0) {
            xlog(XLOG_ERROR, "accept failed(%s) \n", strerror(errno));
            continue;
        }

        xlog(XLOG_DEBUG, "inet_listen_task: new_fd %d \n", new_fd);
        temp_val = new_fd;
        ret = pthread_create(&unused_tid, NULL, common_rx_task, (void *)temp_val);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s) \n", strerror(errno));
            close(new_fd);
            continue;
        } 
    }
    
    close(listen_fd);
    return NULL;
}

int devm_rebuild_usock(char *app_name)
{
    int i, ret;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (!strcmp(app_name, sock_list[i].app_name)) {
            break;
        }
    }

    if (i == MAX_CONNECT_NUM) { //not found
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }
    
    int socket_id = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_id < 0) {
        xlog(XLOG_ERROR, "socket failed(%s) \n", strerror(errno));
        return VOS_ERR;
    }

    struct sockaddr_un un;
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, app_name);
    ret = connect(socket_id, (struct sockaddr *)&un, sizeof(un));
    if (ret < 0) {
        xlog(XLOG_ERROR, "connect failed(%s) \n", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }

    close(sock_list[i].sock_id); //close old fd
    sock_list[i].sock_id = socket_id;
    return VOS_OK;
}

int devm_rebuild_isock(int dst_ip)
{
    int i, ret;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (sock_list[i].dst_ip == dst_ip) {
            break;
        }
    }

    if (i == MAX_CONNECT_NUM) { //not found
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }
    
    int socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id < 0) {
        xlog(XLOG_ERROR, "socket failed(%s) \n", strerror(errno));
        return VOS_ERR;
    }

    struct sockaddr_in dst_addr;
    memset( &dst_addr, 0, sizeof(dst_addr) );
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(DEF_LISTEN_PORT);
    dst_addr.sin_addr.s_addr = dst_ip;
    
    ret = connect(socket_id, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
    if ( ret < 0) {
        xlog(XLOG_ERROR, "connect failed(%s) \n", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }

    close(sock_list[i].sock_id); //close old fd
    sock_list[i].sock_id = socket_id;
    return VOS_OK;
}

int devm_connect_uds(char *app_name)
{
    int i, j = -1;
    int ret;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (sock_list[i].used == FALSE) {
            if (j < 0) j = i;
        }
        else if (!strcmp(app_name, sock_list[i].app_name)) {
            return sock_list[i].sock_id;
        }
    }

    if (j < 0) { //full
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }
    
    int socket_id = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_id < 0) {
        xlog(XLOG_ERROR, "socket failed(%s) \n", strerror(errno));
        return VOS_ERR;
    }

    struct sockaddr_un un;
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, app_name);
    ret = connect(socket_id, (struct sockaddr *)&un, sizeof(un));
    if ( ret < 0) {
        xlog(XLOG_ERROR, "connect failed(%s) \n", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }

    sock_list[j].sock_id = socket_id;
    sock_list[j].app_name = strdup(app_name);
    sock_list[j].used = TRUE;
    return socket_id;
}

int devm_connect_inet(int dst_ip)
{
    int i, j = -1;
    int ret;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (sock_list[i].used == FALSE) {
            if (j < 0) j = i;
        }
        else if (sock_list[i].dst_ip == dst_ip) {
            return sock_list[i].sock_id;
        }
    }

    if (j < 0) { //full
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }
    
    //int socket_id = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id < 0) {
        xlog(XLOG_ERROR, "socket failed(%s) \n", strerror(errno));
        return VOS_ERR;
    }

    struct sockaddr_in dst_addr;
    memset( &dst_addr, 0, sizeof(dst_addr) );
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(DEF_LISTEN_PORT);
    dst_addr.sin_addr.s_addr = dst_ip;
    
    ret = connect(socket_id, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
    if ( ret < 0) {
        xlog(XLOG_ERROR, "connect failed(%s) \n", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }

    sock_list[j].sock_id = socket_id;
    sock_list[i].dst_ip = dst_ip;
    sock_list[j].used = TRUE;
    return socket_id;
}

int devm_msg_forward(DEVM_MSG_S *tx_msg)
{
    if (tx_msg == NULL) {
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }

    sem_wait(&tx_sem);
    int tx_socket = devm_connect_uds(tx_msg->dst_app);
    if (tx_socket < 0) {
        xlog(XLOG_ERROR, "Error at %s:%d, devm_get_socket failed \n", __FILE__, __LINE__);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    if ( send(tx_socket, tx_msg, tx_msg->payload_len + MSG_HEAD_LEN, 0) < MSG_HEAD_LEN ) {
        xlog(XLOG_ERROR, "send failed(%s) \n", strerror(errno));
        devm_rebuild_usock(tx_msg->dst_app);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    sem_post(&tx_sem);
    return VOS_OK;
}

int devm_msg_send(char *dst_app, DEVM_MSG_S *tx_msg)
{
    if (tx_msg == NULL) {
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }

    sem_wait(&tx_sem);
    int tx_socket = devm_connect_uds(dst_app);
    if (tx_socket < 0) {
        xlog(XLOG_ERROR, "Error at %s:%d, devm_get_socket failed \n", __FILE__, __LINE__);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    tx_msg->src_ip = 0;
    snprintf(tx_msg->src_app, APP_NAME_LEN, "%s", get_app_name());
    snprintf(tx_msg->dst_app, APP_NAME_LEN, "%s", dst_app);
    
    if ( send(tx_socket, tx_msg, tx_msg->payload_len + MSG_HEAD_LEN, 0) < MSG_HEAD_LEN ) {
        xlog(XLOG_ERROR, "send failed(%s) \n", strerror(errno));
        devm_rebuild_usock(dst_app);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    sem_post(&tx_sem);
    return VOS_OK;
}

int devm_msg_send2(int dst_ip, char *dst_app, DEVM_MSG_S *tx_msg)
{
    if (tx_msg == NULL) {
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }

    if ( (dst_ip == local_ip_addr) || (dst_ip == 0x0e000001) ){
        return devm_msg_send(dst_app, tx_msg);
    }

    sem_wait(&tx_sem);
    int tx_socket = devm_connect_inet(dst_ip);
    if (tx_socket < 0) {
        xlog(XLOG_ERROR, "Error at %s:%d, devm_get_socket failed \n", __FILE__, __LINE__);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    tx_msg->src_ip = local_ip_addr;
    snprintf(tx_msg->src_app, APP_NAME_LEN, "%s", get_app_name());
    snprintf(tx_msg->dst_app, APP_NAME_LEN, "%s", dst_app);

    if ( send(tx_socket, tx_msg, tx_msg->payload_len + MSG_HEAD_LEN, 0) < MSG_HEAD_LEN ) {
        xlog(XLOG_ERROR, "send failed(%s) \n", strerror(errno));
        devm_rebuild_isock(dst_ip);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    sem_post(&tx_sem);
    return VOS_OK;
}

#if 1
int echo_msg_proc(DEVM_MSG_S *rx_msg)
{
    if (!rx_msg) return VOS_ERR;
    
    printf("%s", rx_msg->msg_payload);

    return VOS_OK;
}

/*int cli_fake_print(void *cookie, char *buff)
{
    DEVM_MSG_S *rx_msg = (DEVM_MSG_S *)cookie;
    DEVM_MSG_S tx_msg;

    tx_msg.msg_type = MSG_TYPE_ECHO;
    snprintf(tx_msg.msg_payload, 510, "%s", buff);
    tx_msg.payload_len = strlen(tx_msg.msg_payload) + 1;
    devm_msg_send2(rx_msg->src_ip, rx_msg->src_app, &tx_msg);
    
    return VOS_OK;
}*/

char fake_buff[500];
int fake_ptr = 0;

int cli_fake_print(void *cookie, char *buff)
{
    if (fake_ptr > 500) return VOS_ERR;
    
    fake_ptr += snprintf(fake_buff + fake_ptr, 500 - fake_ptr, "%s", buff);        
    return VOS_OK;
}

int rcmd_msg_proc(DEVM_MSG_S *rx_msg)
{
    DEVM_MSG_S tx_msg;

    if (!rx_msg) return VOS_ERR;
    printf("rcmd_msg_proc: %s \n", rx_msg->msg_payload);

    fake_ptr = 0;
    cli_set_output_cb(cli_fake_print, NULL);
    cli_cmd_exec(rx_msg->msg_payload);
    cli_set_output_cb(NULL, NULL);

    tx_msg.msg_type = MSG_TYPE_ECHO;
    snprintf(tx_msg.msg_payload, 512, "%s", fake_buff);
    tx_msg.payload_len = strlen(tx_msg.msg_payload) + 1;
    devm_msg_send2(rx_msg->src_ip, rx_msg->src_app, &tx_msg);

    return VOS_OK;
}

int cli_send_echo_cmd(int argc, char **argv)
{
    int ret;
    DEVM_MSG_S tx_msg;
    static int echo_cnt = 0;

    if (argc < 2) {
        vos_print("usage: %s <dst_name> \r\n", argv[0]);
        return VOS_OK;
    }

    tx_msg.msg_type = MSG_TYPE_ECHO;
    sprintf(tx_msg.msg_payload, "tx_msg %d", echo_cnt++);
    tx_msg.payload_len  = strlen(tx_msg.msg_payload) + 1;
    ret = devm_msg_send(argv[1], &tx_msg);
    if (ret != VOS_OK) {  
        xlog(XLOG_ERROR, "Error at %s:%d, devm_send_msg failed(%d) \n", __FILE__, __LINE__, ret);
        return ret;
    } 

    return VOS_OK;
}

int cli_send_remote_echo(int argc, char **argv)
{
    int ret;
    DEVM_MSG_S tx_msg;
    static int echo_cnt = 100;
    int dst_ip;

    if (argc < 3) {
        vos_print("usage: %s <dst_ip> <dst_mod>\r\n", argv[0]);
        return VOS_OK;
    }

    tx_msg.msg_type = MSG_TYPE_ECHO;
    sprintf(tx_msg.msg_payload, "tx_msg %d", echo_cnt++);
    tx_msg.payload_len  = strlen(tx_msg.msg_payload) + 1;
    inet_pton(AF_INET, argv[1], &dst_ip);
    
    ret = devm_msg_send2(dst_ip, argv[2], &tx_msg);
    if (ret != VOS_OK) {  
        xlog(XLOG_ERROR, "Error at %s:%d, devm_msg_send2 failed(%d) \n", __FILE__, __LINE__, ret);
        return ret;
    } 
    
    return VOS_OK;
}

int cli_send_remote_cmd(int argc, char **argv)
{
    int ret;
    DEVM_MSG_S tx_msg;
    int dst_ip;

    if (argc < 4) {
        vos_print("usage: %s <dst_ip> <dst_mod> ...\r\n", argv[0]);
        return VOS_OK;
    }

    tx_msg.msg_type = MSG_TYPE_RCMD;
    sprintf(tx_msg.msg_payload, "%s", argv[3]);
    tx_msg.payload_len  = strlen(tx_msg.msg_payload) + 1;
    inet_pton(AF_INET, argv[1], &dst_ip);
    
    ret = devm_msg_send2(dst_ip, argv[2], &tx_msg);
    if (ret != VOS_OK) {  
        xlog(XLOG_ERROR, "Error at %s:%d, devm_msg_send2 failed(%d) \n", __FILE__, __LINE__, ret);
        return ret;
    } 
    
    return VOS_OK;
}

#endif

int devm_msg_init(char *app_name, int master)
{
    int ret;
    char *cfg_str;
    pthread_t unused_tid;

    cfg_str = sys_conf_get("eth_name");
    if (cfg_str) {
        local_ip_addr = get_local_ip(cfg_str);
        xlog(XLOG_INFO, "local_ip_addr 0x%x \n", local_ip_addr);
    }

    ret = sem_init(&tx_sem, 0, 1);
    if(ret != 0)  {  
        xlog(XLOG_ERROR, "sem_init failed(%s) \n", strerror(errno));
        return VOS_ERR;  
    } 

    ret = pthread_create(&unused_tid, NULL, uds_listen_task, NULL);  
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "pthread_create failed(%s) \n", strerror(errno));
        return VOS_ERR;  
    } 

    if (master) {
        ret = pthread_create(&unused_tid, NULL, inet_listen_task, NULL);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s) \n", strerror(errno));
            return VOS_ERR;  
        } 
    }

    devm_set_msg_func(MSG_TYPE_ECHO, echo_msg_proc);
    devm_set_msg_func(MSG_TYPE_RCMD, rcmd_msg_proc);
    cli_cmd_reg("echo",     "send echo cmd",        &cli_send_echo_cmd);
    cli_cmd_reg("tx2",      "send2 test",           &cli_send_remote_echo);
    cli_cmd_reg("rcmd",     "remote cmd",           &cli_send_remote_cmd);

    return VOS_OK;
}

