



#ifndef _TINY_CLI_H_
#define _TINY_CLI_H_

#define INCLUDE_CONSOLE
#define INCLUDE_TELNETD
#define CHECK_AMBIGUOUS
//#define CLI_PWD_CHECK 

#define TELNETD_LISTEN_PORT     2300

int cli_telnet_active(void);

void cli_cmd_init(void);

int cli_task_init(void);

int telnet_task_init(void);

typedef int (*CLI_OUT_CB)(void *cookie, char *buff);

int cli_set_output_cb(CLI_OUT_CB cb, void *cookie);

int cli_cmd_exec(char *buff);



#endif

