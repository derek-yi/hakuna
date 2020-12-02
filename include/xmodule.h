
#ifndef _HAKUNA_H_
#define _HAKUNA_H_

/****************************************************************************************
 * global config
 ****************************************************************************************/
#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

#define VOS_OK      0
#define VOS_ERR     (-1)

#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif

/****************************************************************************************
 * config
 ****************************************************************************************/
/* 
char *json_cfg  = \
"{"
"    \"module_name\": \"jerry\", "
"    \"telnet_enable\": \"0(0-disable, 1-enable)\", "
"    \"cli_enable\": \"0\", "
"    \"end_key\": 0 "
"}";
*/
int xmodule_init(char *json);


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

void cli_cmd_init(void);

void cli_main_task(void);

int telnet_task_init(void);

int vos_print(const char * format,...);

#define xlog_err printf
/****************************************************************************************
 * vos
 ****************************************************************************************/

/****************************************************************************************
 * msg
 ****************************************************************************************/

#endif