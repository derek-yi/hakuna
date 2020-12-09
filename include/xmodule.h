
#ifndef _XMODULE_H_
#define _XMODULE_H_

#include "vos.h"
#include "xlog.h"
#include "xmsg.h"

/****************************************************************************************
 * xmodule
 ****************************************************************************************/
#define APP_ROLE_SLAVE          0
#define APP_ROLE_MASTER         1

int xmodule_init(char *json);

char* sys_conf_get(char *key_str);

int sys_conf_geti(char *key_str);

int sys_conf_set(char *key_str, char *value);

char *get_app_name(void);

/****************************************************************************************
 * cli
 ****************************************************************************************/
#define CMD_OK                  0x00
#define CMD_ERR                 0x01
#define CMD_ERR_PARAM           0x02
#define CMD_ERR_NOT_MATCH       0x03
#define CMD_ERR_AMBIGUOUS       0x04
#define CMD_ERR_EXIT            0x99

typedef int (* CMD_FUNC)(int argc, char **argv);

int cli_cmd_reg(const char *cmd, const char *help, CMD_FUNC func);

int vos_print(const char * format,...);

/****************************************************************************************
 * xx
 ****************************************************************************************/

#endif
