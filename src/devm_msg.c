
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

#include "xmodule.h"
#include "xlog.h"
#include "devm_msg.h"


#define MAX_CONNECT_NUM 32

msg_func msg_func_list[MSG_TYPE_MAX] = {0};

typedef struct {
    int  sock_id;
    int  used;
    int  dst_ip;
    int  port;
    char *app_name;  //as module_name
}SOCK_INFO;

SOCK_INFO sock_list[MAX_CONNECT_NUM] = {0};

sem_t tx_sem;

int inet_listen_port = 0;

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
    char *mod_name = sys_conf_get("module_name");

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        xlog(XLOG_ERROR, "socket failed(%s) \n", strerror(errno));
        return NULL;
    }
    
    un.sun_family = AF_UNIX;
    unlink(mod_name);
    strcpy(un.sun_path, mod_name);
    if (bind(fd, (struct sockaddr *)&un, sizeof(un)) <0 ) {
        xlog(XLOG_ERROR, "bind failed(%s) \n", strerror(errno));
        return NULL;
    }
    
    if (listen(fd, MAX_CONNECT_NUM) < 0) {
        xlog(XLOG_ERROR, "listen failed(%s) \n", strerror(errno));
        return NULL;
    }

    xlog(XLOG_DEBUG, "uds_listen_task: listen %s \n", mod_name);
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
    inet_addr.sin_port = htons(inet_listen_port);
    if (bind(listen_fd,(struct sockaddr *)&inet_addr, sizeof(inet_addr)) < 0 ) {
        xlog(XLOG_ERROR, "bind failed(%s) \n", strerror(errno));
        return NULL;
    }
    
    if ( listen(listen_fd, MAX_CONNECT_NUM) < 0){
        xlog(XLOG_ERROR, "listen failed(%s) \n", strerror(errno));
        return NULL;
    }

    xlog(XLOG_DEBUG, "inet_listen_task: listen %d \n", inet_listen_port);
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

int devm_rebuild_socket(char *app_name)
{
    int i;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (!strcmp(app_name, sock_list[i].app_name)) {
            break;
        }
    }

    if (i == MAX_CONNECT_NUM) { //not found
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }
    
    int socket_id = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socket_id < 0) {
        xlog(XLOG_ERROR, "socket failed(%s) \n", strerror(errno));
        return VOS_ERR;
    }

    struct sockaddr_un un;
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, app_name);
    if (connect(socket_id, (struct sockaddr *)&un, sizeof(un)) < 0) {
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
    struct timeval  tv;

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
    
    int socket_id = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
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

    tv.tv_sec = 3;
    tv.tv_usec = 0;
    ret = setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if ( ret < 0) {
        xlog(XLOG_ERROR, "setsockopt failed(%s) \n", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }

    sock_list[j].sock_id = socket_id;
    sock_list[j].app_name = strdup(app_name);
    sock_list[j].used = TRUE;
    return socket_id;
}

int devm_connect_inet(int dst_ip, int port)
{
    int i, j = -1;
    int ret;
    struct timeval  tv;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (sock_list[i].used == FALSE) {
            if (j < 0) j = i;
        }
        else if ( (sock_list[i].dst_ip == dst_ip) && (sock_list[i].port == port) ) {
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
    dst_addr.sin_port = htons(port);
    dst_addr.sin_addr.s_addr = dst_ip;
    
    ret = connect(socket_id, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
    if ( ret < 0) {
        xlog(XLOG_ERROR, "connect failed(%s) \n", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }

#if 0
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    ret = setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if ( ret < 0) {
        xlog(XLOG_ERROR, "setsockopt failed(%s) \n", strerror(errno));
        close(socket_id);
        return VOS_ERR;
    }
#endif    

    sock_list[j].sock_id = socket_id;
    sock_list[i].dst_ip = dst_ip;
    sock_list[i].port = port;
    sock_list[j].used = TRUE;
    return socket_id;
}


int devm_msg_send(char *app_name, DEVM_MSG_S *tx_msg)
{
    if (tx_msg == NULL) {
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }

    sem_wait(&tx_sem);
    int tx_socket = devm_connect_uds(app_name);
    if (tx_socket < 0) {
        xlog(XLOG_ERROR, "Error at %s:%d, devm_get_socket failed \n", __FILE__, __LINE__);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    if ( send(tx_socket, tx_msg, tx_msg->payload_len + MSG_HEAD_LEN, 0) < MSG_HEAD_LEN ) {
        xlog(XLOG_ERROR, "send failed(%s) \n", strerror(errno));
        devm_rebuild_socket(app_name);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    sem_post(&tx_sem);
    return VOS_OK;
}

int devm_msg_send2(int dst_ip, int port, char *dst_mod, DEVM_MSG_S *tx_msg)
{
    if (tx_msg == NULL) {
        xlog(XLOG_ERROR, "Error at %s:%d \n", __FILE__, __LINE__);
        return VOS_ERR;
    }

    sem_wait(&tx_sem);
    int tx_socket = devm_connect_inet(dst_ip, port);
    if (tx_socket < 0) {
        xlog(XLOG_ERROR, "Error at %s:%d, devm_get_socket failed \n", __FILE__, __LINE__);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    if ( send(tx_socket, tx_msg, tx_msg->payload_len + MSG_HEAD_LEN, 0) < MSG_HEAD_LEN ) {
        xlog(XLOG_ERROR, "send failed(%s) \n", strerror(errno));
        devm_rebuild_socket(dst_mod);
        sem_post(&tx_sem);
        return VOS_ERR;
    }

    sem_post(&tx_sem);
    return VOS_OK;
}

int echo_msg_proc(DEVM_MSG_S *rx_msg)
{
    if (!rx_msg) return VOS_ERR;
    
    printf("echo_msg_proc: %s \n", rx_msg->msg_payload);

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
    int dst_ip, port;

    if (argc < 4) {
        vos_print("usage: %s <dst_ip> <dst_port> <dst_mod>\r\n", argv[0]);
        return VOS_OK;
    }

    tx_msg.msg_type = MSG_TYPE_ECHO;
    sprintf(tx_msg.msg_payload, "tx_msg %d", echo_cnt++);
    tx_msg.payload_len  = strlen(tx_msg.msg_payload) + 1;
    inet_pton(AF_INET, argv[1], &dst_ip);
    port = atoi(argv[2]);
    
    ret = devm_msg_send2(dst_ip, port, argv[3], &tx_msg);
    if (ret != VOS_OK) {  
        xlog(XLOG_ERROR, "Error at %s:%d, devm_msg_send2 failed(%d) \n", __FILE__, __LINE__, ret);
        return ret;
    } 
    
    return VOS_OK;
}


int devm_msg_init(char *mod_name, int listen_port)
{
    int ret;
    pthread_t unused_tid;

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

    if (listen_port > 0) {
        inet_listen_port = listen_port;
        ret = pthread_create(&unused_tid, NULL, inet_listen_task, NULL);  
        if (ret != 0)  {  
            xlog(XLOG_ERROR, "pthread_create failed(%s) \n", strerror(errno));
            return VOS_ERR;  
        } 
    }

    devm_set_msg_func(MSG_TYPE_ECHO, echo_msg_proc);
    cli_cmd_reg("echo",     "send echo cmd",        &cli_send_echo_cmd);
    cli_cmd_reg("tx2",      "send2 test",           &cli_send_remote_echo);

    return VOS_OK;
}

