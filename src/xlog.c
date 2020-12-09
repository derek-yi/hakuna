
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>   //timer_t
#include <signal.h>
#include <sys/time.h>
#include <stdarg.h>

#include "xmodule.h"
#include "xlog.h"
#include "vos.h"


#define XLOG_BUFF_MAX           1024

int xlog_print_file(char *filename)
{
    FILE *fp;
    char temp_buf[XLOG_BUFF_MAX];
    const char *cp_file = "temp_file";
    
    sprintf(temp_buf, "cp -f /var/log/%s %s", filename, cp_file);
    vos_run_cmd(temp_buf);
    
    fp = fopen(cp_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "open failed, %s\n", strerror(errno));
        unlink(cp_file);
        return VOS_ERR;
    }

    vos_print("%s: \r\n", filename);
    memset(temp_buf, 0, sizeof(temp_buf));
    while (fgets(temp_buf, XLOG_BUFF_MAX-1, fp) != NULL) {  
        vos_print("%s\r", temp_buf); //linux-\n, windows-\n\r
        memset(temp_buf, 0, sizeof(temp_buf));
    }

    fclose(fp);
    unlink(cp_file);
    return VOS_OK;
}

void fmt_time_str(char *time_str, int max_len)
{
    struct tm *tp;
    time_t t = time(NULL);
    tp = localtime(&t);
     
    if (!time_str) return ;
    
    snprintf(time_str, max_len, "[%04d-%02d-%02d %02d:%02d:%02d]", 
            tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
}

#ifdef INCLUDE_SYSLOG

int _xlog(char *file, int line, int level, const char *format, ...)
{
    va_list args;
    char buf[XLOG_BUFF_MAX];
    int len;
    int facility = LOG_LOCAL0;

    va_start(args, format);
    len = vsnprintf(buf, XLOG_BUFF_MAX, format, args);
    va_end(args);

    openlog(NULL, LOG_CONS, facility);
    //setlogmask(LOG_UPTO(LOG_NOTICE));
    syslog(level, "%s", buf);
    closelog();

    if ( (level == XLOG_WARN) || (level == XLOG_ERROR) ){
        vos_print("%s\r\n", buf);
    }

    return len;    
}

int xlog_init(char *app_name)
{
    return 0;
}

#elif defined (INCLUDE_ZLOG)

zlog_category_t *my_cat = NULL; 

int xlog_init(char *app_name)
{
    int rc;
    
	rc = zlog_init("/etc/zlog.conf");
	if (rc) {
		printf("init failed\n");
		return -1;
	}

	my_cat = zlog_get_category("app_name");
	if (!my_cat) {
		printf("get category fail\n");
		zlog_fini();
		return -3;
	}
    return 0;
}

#else
int xlog_init(char *app_name)
{
    return 0;
}

int _xlog(char *file, int line, int level, const char *format, ...)
{
    va_list args;
    char buf[XLOG_BUFF_MAX];
    int len;

    va_start(args, format);
    len = vsnprintf(buf, XLOG_BUFF_MAX, format, args);
    va_end(args);

    if ( (level == XLOG_WARN) || (level == XLOG_ERROR) ){
        vos_print("%s\r\n", buf);
    } else {
        //vos_print("%s\r\n", buf);
    }

    return len;    
}

#endif

