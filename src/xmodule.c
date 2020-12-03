#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmodule.h"
#include "xlog.h"
#include "devm_msg.h"

#include "cJSON.h"
#include "tiny_cli.h"


typedef struct _DYN_CFG{
    struct _DYN_CFG *next;
    char    *key;
    char    *value;
}DYN_CFG_S;


typedef struct _SYS_CONF_PARAM
{
//"fix.config"
    char    *build_version;
    char    *build_time;
    char    *mod_name;

//"dyn.config"
    DYN_CFG_S *dyn_cfg;
}SYS_CONF_PARAM;


SYS_CONF_PARAM sys_conf;

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

int parse_json_cfg(char *json)
{
    cJSON* root_tree;
    int list_cnt;

	root_tree = cJSON_Parse(json);
	if (root_tree == NULL) {
		xlog_err("parse json file fail");
        return VOS_ERR;
	}

	list_cnt = cJSON_GetArraySize(root_tree);
	for (int i = 0; i < list_cnt; ++i) {
		cJSON* tmp_node = cJSON_GetArrayItem(root_tree, i);
        DYN_CFG_S *dyn_cfg;
        char num_str[64];

        dyn_cfg = (DYN_CFG_S *)malloc(sizeof(DYN_CFG_S));
        if (dyn_cfg == NULL) {
            xlog_err("malloc failed");
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


int cli_sys_cfg_list(int argc, char **argv)
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


int cli_sys_cfg_set(int argc, char **argv)
{
    if (argc < 3) {
        vos_print("usage: %s <key> <value> \r\n", argv[0]);
        return VOS_OK;
    }

    sys_conf_set(argv[1], argv[2]);
    return VOS_OK;
}

void xmodule_cmd_init(void)
{
    cli_cmd_reg("cfg_list",        "sys cfg list",           &cli_sys_cfg_list);
    cli_cmd_reg("cfg_set",         "sys cfg set",            &cli_sys_cfg_set);
}


int xmodule_init(char *json)
{
    char *mod_name;
    int listen_port;
    
    parse_json_cfg(json);
    sys_conf_set("hakuna", "100");
    mod_name = sys_conf_get("module_name");
    if (mod_name != NULL) {
        sys_conf.mod_name = strdup(mod_name);
        xlog_init(mod_name);
    }

    cli_cmd_init();
    xmodule_cmd_init();

    listen_port = sys_conf_geti("listen_port");
    devm_msg_init(sys_conf.mod_name, listen_port);

    if (sys_conf_geti("telnet_enable")) {
        telnet_task_init();
    }

    if (sys_conf_geti("cli_enable")) {
        cli_task_init();
    }

    return VOS_OK;
}

