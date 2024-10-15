#include "diff2ya.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bsdiff_flash.h"
#include "ty_bsdiff_adapt.h"


extern int ty_ota_entry_linux_sim(void);
int main(int argc, char* argv[])
{
    int ret = 0;
    if( (ret = ty_ota_entry_linux_sim()) != 0) 
        ty_adapt_print("***** ty_bsdiff_entry  return error  %d\r\n", ret);

    return ret;
}

