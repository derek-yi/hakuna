#ifndef _VOS_H_
#define _VOS_H_

#ifndef T_DESC
#define T_DESC(x, y)    y
#endif

int pipe_read(char *cmd_str, char *buff, int buf_len);

int sys_node_readstr(char *node_str, char *rd_buf, int buf_len);

int sys_node_read(char *node_str, int *value);

int sys_node_writestr(char *node_str, char *wr_buf);

int sys_node_write(char *node_str, int value);

typedef void (* timer_callback)(void *param);

int vos_create_timer(timer_t *ret_tid, int interval, timer_callback callback, void *param);

void vos_msleep(uint32 milliseconds);

int vos_run_cmd(char *cmd_str);

int vos_print(const char * format,...);

void vos_msleep(uint32 milliseconds);

#endif


