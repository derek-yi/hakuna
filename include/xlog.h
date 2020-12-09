

#ifndef _XLOG_H_
#define _XLOG_H_

//#define INCLUDE_ZLOG

#ifdef INCLUDE_ZLOG
#include <zlog.h>

#define XLOG_DEBUG      ZLOG_LEVEL_DEBUG
#define XLOG_INFO       ZLOG_LEVEL_INFO
#define XLOG_WARN       ZLOG_LEVEL_WARN
#define XLOG_ERROR      ZLOG_LEVEL_ERROR

#elif defined (INCLUDE_SYSLOG)
#include <syslog.h>

#define XLOG_DEBUG      LOG_DEBUG
#define XLOG_INFO       LOG_INFO
#define XLOG_WARN       LOG_WARNING
#define XLOG_ERROR      LOG_ERR

#else
#define XLOG_DEBUG      0
#define XLOG_INFO       1
#define XLOG_WARN       2
#define XLOG_ERROR      3

#endif

void fmt_time_str(char *time_str, int max_len);

int xlog_print_file(char *filename);

int xlog_init(char *mod_name);

#ifdef INCLUDE_ZLOG

extern zlog_category_t *my_cat;

#define xlog(level, ...) \
    zlog(my_cat, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, level, __VA_ARGS__)


#elif defined (INCLUDE_SYSLOG)
int _xlog(char *file, int line, int level, const char *format, ...);

#define xlog(level, ...)  \
    _xlog(__FILE__, __LINE__, level, __VA_ARGS__)

#else
int _xlog(char *file, int line, int level, const char *format, ...);

#define xlog(level, ...)  \
    _xlog(__FILE__, __LINE__, level, __VA_ARGS__)

#endif

#endif
