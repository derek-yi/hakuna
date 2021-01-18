#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmodule.h"
#include "cJSON.h"
#include "tiny_cli.h"

#if 1

typedef struct _DYN_CFG{
    struct _DYN_CFG *next;
    char    *key;
    char    *value;
}DYN_CFG_S;

typedef struct _SYS_CONF_PARAM
{
//"fix.config"
    char    *top_cfg;
    char    *build_time;
    char    *app_name;
    int     app_role;

//"dyn.config"
    DYN_CFG_S *dyn_cfg;
}SYS_CONF_PARAM;


SYS_CONF_PARAM sys_conf;

char *get_app_name(void)
{   
    return sys_conf.app_name;
}

int sys_conf_set(char *key_str, char *value)
{
    DYN_CFG_S *p;

    if (key_str == NULL) {
        return -1;
    }

    p = sys_conf.dyn_cfg;
    while (p != NULL) {
        if( !strcmp(key_str, p->key) ) {
            p->value = strdup(value);
            return 0;
        }
        p = p->next;
    }

    p = (DYN_CFG_S *)malloc(sizeof(DYN_CFG_S));
    if (p == NULL) {
        return -1;
    }
    
    p->key = strdup(key_str);
    p->value = strdup(value);
    p->next = sys_conf.dyn_cfg;
    sys_conf.dyn_cfg = p;
        
    return 0;
}

char* sys_conf_get(char *key_str)
{
    DYN_CFG_S *p;

    if (key_str == NULL) return NULL;

    p = sys_conf.dyn_cfg;
    while (p != NULL) {
        if( !strcmp(key_str, p->key) ) {
            return p->value;
        }
        p = p->next;
    }

    return NULL;
}

int sys_conf_geti(char *key_str)
{
    DYN_CFG_S *p;

    if (key_str == NULL) return 0;

    p = sys_conf.dyn_cfg;
    while (p != NULL) {
        if( !strcmp(key_str, p->key) ) {
            return strtol(p->value, NULL, 0);
        }
        p = p->next;
    }

    return 0;
}

int parse_json_cfg(char *json_file)
{
    char *json = NULL;
    cJSON* root_tree;
    int list_cnt;

    printf("load conf %s ...", json_file);
    json = read_file(json_file);
	if (json == NULL) {
		printf("file content is null");
		return VOS_ERR;
	}

	root_tree = cJSON_Parse(json);
	if (root_tree == NULL) {
		printf("parse json file fail");
        return VOS_ERR;
	}

    sys_conf.top_cfg = strdup(json_file);
	list_cnt = cJSON_GetArraySize(root_tree);
	for (int i = 0; i < list_cnt; ++i) {
		cJSON* tmp_node = cJSON_GetArrayItem(root_tree, i);
        DYN_CFG_S *dyn_cfg;
        char num_str[64];

        dyn_cfg = (DYN_CFG_S *)malloc(sizeof(DYN_CFG_S));
        if (dyn_cfg == NULL) {
            printf("malloc failed");
            goto EXIT_PROC;
        }
        
        dyn_cfg->key = strdup(tmp_node->string);
        if (tmp_node->valuestring) {
            dyn_cfg->value = strdup(tmp_node->valuestring);
        } else {
            sprintf(num_str, "%d", tmp_node->valueint);
            dyn_cfg->value = strdup(num_str);
        }
        dyn_cfg->next = sys_conf.dyn_cfg;
        sys_conf.dyn_cfg = dyn_cfg;
	}

EXIT_PROC:
    if (root_tree != NULL) {
        cJSON_Delete(root_tree);
    }
    
    return VOS_OK;
}

int store_json_cfg(char *file_name)
{
    cJSON* root_tree;
    int ret = VOS_ERR;
    char * out;
    DYN_CFG_S *p;

    root_tree = cJSON_CreateObject();
    if (root_tree == NULL) return VOS_ERR;

    //sem_wait(&sysconf_sem);
    p = sys_conf.dyn_cfg;
    while (p != NULL) {
        cJSON_AddItemToObject(root_tree, p->key, cJSON_CreateString(p->value));
        p = p->next;
    }
    //sem_post(&sysconf_sem);

    out = cJSON_Print(root_tree);
    if (out) {
        ret = write_file(file_name, out, strlen(out));
        vos_print("file content: \r\n %s \r\n", out);
    } 

//EXIT_PROC:
    if (out != NULL) free(out);
    if (root_tree != NULL) cJSON_Delete(root_tree);
    
    return ret;
}

int cli_sys_cfg_list(void)
{
    DYN_CFG_S *p;

    p = sys_conf.dyn_cfg;
    while (p != NULL) {
        if ( p->key && p->value ) {
            vos_print(" --> %s: %s \r\n", p->key, p->value);
        }
        p = p->next;
    }    

    return VOS_OK;
}

int cli_sys_cfg_proc(int argc, char **argv)
{
    int cp_len;

    if (argc < 2) {
        vos_print("usage: \r\n");
        vos_print("  cfg show               -- list cfg \r\n");
        vos_print("  cfg set <key> <value>  -- add cfg \r\n");
        vos_print("  cfg save               -- save cfg \r\n");
        vos_print("  cfg clear              -- clear saved file \r\n");
        return VOS_OK;
    }

    cp_len = strlen(argv[1]);
    if (!strncasecmp(argv[1], "show", cp_len)) {
        cli_sys_cfg_list();
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "set", cp_len)) {
        if (argc < 4) {
            vos_print("invalid param \r\n");
            return CMD_ERR_PARAM;
        }
        sys_conf_set(argv[2], argv[3]);
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "save", cp_len)) {
        store_json_cfg("./top_cfg.json");
        return VOS_OK;
    }

    if (!strncasecmp(argv[1], "clear", cp_len)) {
        //unlink("./top_cfg.json");
        return VOS_OK;
    }
    
    return VOS_OK;
}

#endif

void xmodule_cmd_init(void)
{
    cli_cmd_init();

    cli_cmd_reg("cfg",      "sys cfg operation",            &cli_sys_cfg_proc);
}

int xmodule_init(char *json_file)
{
    char *app_name;
    
    if (parse_json_cfg(json_file) != VOS_OK) {
        printf("invalid json cfg \r\n");
        return VOS_ERR;
    }
    
    app_name = sys_conf_get("app_name");
    if (app_name == NULL) {
        printf("no app_name in cfg file \r\n");
        return VOS_ERR;
    }
    sys_conf.app_name = strdup(app_name);
    sys_conf.app_role = sys_conf_geti("app_role");

    xlog_init(app_name);
    
    devm_msg_init(sys_conf.app_name, sys_conf.app_role);

    xmodule_cmd_init();
    if (sys_conf_geti("telnet_enable")) {
        telnet_task_init();
    }
    if (sys_conf_geti("cli_enable")) {
        cli_task_init();
    }

    return VOS_OK;
}

