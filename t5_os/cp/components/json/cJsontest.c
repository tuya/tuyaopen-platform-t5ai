/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
#include <common/bk_include.h>
#include "cJsontest.h"

#if 1//CJSON_TEST_DEMO
#include <stdio.h>
#include "cJSON.h"
#include <os/mem.h>
#include "uart_pub.h"
#include "stdarg.h"
#include <common/bk_include.h>
#include <os/os.h>
//#include "Error.h"
#include "portmacro.h"

/* a bunch of json: */
const char text1[] = "{\n\"name\": \"beken\", \n\"format\": {\"type\":\"rect\",\n\"interlace\": false,\"frame rate\": 24\n}\n}";
const char text2[] = "[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]";

void json_item_show(cJSON *item)
{
    char *out = NULL;

	if (item->string)
		BK_LOGD(NULL,"name:%s,", item->string);
    switch(item->type)
    {
    case cJSON_False:
        BK_LOGD(NULL,"%s\n", "false");
        break;
    case cJSON_True:
        BK_LOGD(NULL,"%s\n", "true");
        break;
    case cJSON_NULL:
        BK_LOGD(NULL,"%s\n", "JSON_NULL");
        break;
    case cJSON_Number:
        BK_LOGD(NULL,"%d\n", item->valueint);
        break;
    case cJSON_String:
        BK_LOGD(NULL,"%s\n", item->valuestring);
        break;
    case cJSON_Array:
        BK_LOGD(NULL,"%s\n", item->string);
        break;
    case cJSON_Object:
        out = cJSON_Print(item);
        BK_LOGD(NULL,"%s\n", out);
        if(out)
            os_free(out);
        break;
    default:
        BK_LOGD(NULL,"error\r\n");
        break;
    }
}

/* Parse text to JSON, then render back to text, and print! */
void doit(char *text)
{
    char *out;
    cJSON *json;

    json = cJSON_Parse(text);
    if (!json)
    {
        BK_LOGD(NULL,"Error before: [%s]\n", cJSON_GetErrorPtr());
    }
    else
    {
        out = cJSON_Print(json);
        cJSON_Delete(json);
        BK_LOGD(NULL,"%s\n", out);
        if(out)
            os_free(out);
    }
}

/* Used by some code below as an example datatype. */
struct record
{
    const char *precision;
    double lat, lon;
    const char *address, *city, *state, *zip, *country;
};

/* Create a bunch of objects as demonstration. */
void create_objects()
{
#if 0
    cJSON *root, *fmt;
    //char *out;
    int i = 0;
    cJSON *item;
    int array_size;
    cJSON *cmp;

    root = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "Cmp", cJSON_CreateString("Beken"));
    cJSON_AddItemToObject(root, "fmt", fmt = cJSON_CreateObject());
    cJSON_AddStringToObject(fmt, "type", "rect");
    cJSON_AddNumberToObject(fmt, "wdth", 1920);
    cJSON_AddNumberToObject(fmt, "hght", 1080);
    cJSON_AddFalseToObject (fmt, "interlace");
    cJSON_AddNumberToObject(fmt, "rate",	24);

    json_item_show(root);

    cmp = cJSON_GetObjectItem(root, "fmt");
    if(!cmp)
    {
        BK_LOGD(NULL,"no fmt\r\n");
        return;
    }

    array_size = cJSON_GetArraySize(cmp);
    BK_LOGD(NULL,"array size is %d\r\n", array_size);

    for(i = 0; i < array_size; i++)
    {
        item = cJSON_GetArrayItem(cmp, i);
        json_item_show(item);
    }

    if(root)
        cJSON_Delete(root);
#else
	cJSON *root, *array, *ssid;
    root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "code", 200);
	cJSON_AddItemToObject(root, "data", array = cJSON_CreateArray());
    //cJSON_AddItemToObject(array, "data", ssid = cJSON_CreateObject());
	for (int i = 0; i < 2; i++) {
		ssid = cJSON_CreateObject();
		cJSON_AddStringToObject(ssid, "wifiName", "aclsemi");
		cJSON_AddItemToArray(array, ssid);
	}
    /*cJSON_AddStringToObject(fmt, "type", "rect");
    cJSON_AddNumberToObject(fmt, "wdth", 1920);
    cJSON_AddNumberToObject(fmt, "hght", 1080);
    cJSON_AddFalseToObject (fmt, "interlace");
    cJSON_AddNumberToObject(fmt, "rate",	24);*/
	cJSON_AddStringToObject(root, "msg", "success");
    json_item_show(root);

    if(root)
        cJSON_Delete(root);
#endif
}


void cjson_test_main ( beken_thread_arg_t arg )
{
#if 0
    BK_LOGD(NULL,"\r\n\r\n cjson testing..................\r\n ");
    rtos_delay_milliseconds( 1500 );

    /* Process each json textblock by parsing, then rebuilding: */
    doit((char *)text1);

    /* Now some samplecode for building objects concisely: */
    create_objects();

    BK_LOGD(NULL," cjson test over..................\r\n\r\n ");
#else
	create_objects();
	rtos_delay_milliseconds( 100000 );
#endif
    rtos_delete_thread( NULL );
}

void demo_start(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    //bk_err_t err = kNoErr;

    rtos_create_thread( NULL, BEKEN_APPLICATION_PRIORITY,
                              "json_test",
                              (beken_thread_function_t)cjson_test_main,
                              10*1024,
                              (beken_thread_arg_t)0 );
    //return err;
}

#endif

