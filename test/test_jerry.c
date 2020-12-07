

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xmodule.h"

char *json_cfg  = \
"{"
"    \"app_name\": \"jerry\", "
"    \"telnet_enable\": \"0(0-disable, 1-enable)\", "
"    \"cli_enable\": \"0\", "
"    \"end_key\": 0 "
"}";


int main(int argc, char **argv)
{
    xmodule_init(json_cfg);
    
    while(1) sleep(1);
}


