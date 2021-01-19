
#ifndef _XMODULE_H_
#define _XMODULE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>

#include "vos.h"
#include "xlog.h"
#include "xmsg.h"

/****************************************************************************************
 * xmodule
 ****************************************************************************************/
#define APP_ROLE_SLAVE          0
#define APP_ROLE_MASTER         1

int xmodule_init(char *json_file);

char* sys_conf_get(char *key_str);

int sys_conf_geti(char *key_str);

int sys_conf_set(char *key_str, char *value);

char *get_app_name(void);

char* read_file(const char *filename);

int write_file(const char *filename, char *buff, int buff_size);

/****************************************************************************************
 * cli
 ****************************************************************************************/
#define CMD_OK                  0x00
#define CMD_ERR                 0x01
#define CMD_ERR_PARAM           0x02
#define CMD_ERR_NOT_MATCH       0x03
#define CMD_ERR_EXIT            0x99

typedef int (* CMD_FUNC)(int argc, char **argv);

int cli_cmd_reg(const char *cmd, const char *help, CMD_FUNC func);

int vos_print(const char * format,...);

/****************************************************************************************
 * xx
 ****************************************************************************************/

#endif
