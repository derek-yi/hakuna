

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xmodule.h"

char *json_cfg  = \
"{  \
    \"app_name\": \"jerry\",   \
    \"eth_name\": \"ens33\",   \
    \"end_key\": 0    \
}";



int main(int argc, char **argv)
{
    xmodule_init(json_cfg);
    
    while(1) sleep(1);
}


