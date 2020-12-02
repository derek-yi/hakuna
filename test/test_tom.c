

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmodule.h"

char *json_cfg  = \
"{  \
    \"module_name\": \"tom\",   \
    \"telnet_enable\": \"1(0-disable, 1-enable)\",  \
    \"cli_enable\": \"1\",  \
    \"end_key\": 0    \
}";


int main(int argc, char **argv)
{
    //printf("%s\n", json_cfg);
    xmodule_init(json_cfg);
    
    while(1) sleep(1);
}

