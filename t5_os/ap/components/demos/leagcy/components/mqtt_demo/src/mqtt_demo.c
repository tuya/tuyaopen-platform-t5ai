#include <os/os.h>
#include "wlan_cli_pub.h"
#include "sockets.h"

#include "string.h"
#include "stdio.h"
#include "tcp_mqtt_client_port.h"
#include "mqtt_client_core.h"
#include "mqtt_client_com_port.h"


extern void bk_printf(const char *fmt, ...);

#if 0
#include "MQTTFreeRTOS.h"
#include "MQTTClient.h"

#define ECHO_STACK_SIZE         (1024 * 6)
#define ECHO_THD_PRORITY        (BEKEN_DEFAULT_WORKER_PRIORITY)


void echo_msg_arrived_cb(MessageData* data)
{
	BK_LOGD(NULL, "Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len,
		data->topicName->lenstring.data,
		data->message->payloadlen, data->message->payload);
}


static void echo_task_handler(void *pvParameters)
{
	/* connect to m2m.eclipse.org, subscribe to a topic, send and receive 
	 * messages regularly every 1 second
	   mqtt server:
	 			"test.mosquitto.org"  "test.mosca.io"
	 			"iot.eclipse.org" "broker.hivemq.com"
	 */
	Network network;

	MQTTClient client;
	uint32_t fail_cnt;
	int rc = 0, count = 0;
	
	unsigned char sendbuf[80], readbuf[80];
	char topicName[30];
#if 0
	char* link_name = "broker.mqttdashboard.com";
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
#else
	char* link_name = "192.168.19.21";///"broker.mqttdashboard.com";

	MQTTPacket_connectData connectData = { 
											{'M', 'Q', 'T', 'C'},   ///char struct_id[4];
											0,  ///int struct_version;
											4,  ///unsigned char MQTTVersion;
											{NULL, {0, NULL}},  ///MQTTString clientID;
											60,   ///unsigned short keepAliveInterval;
											1,   ///unsigned char cleansession;
											0,   //unsigned char willFlag;
											MQTTPacket_willOptions_initializer,  ///MQTTPacket_willOptions will;
											{"mqtt_user1", {0, NULL}},  ///MQTTString username;
											{"123", {0, NULL}}   //MQTTString password;
										};
#endif

	NetworkInit(&network);	
	BK_LOGD(NULL, "[MQTT]mqtt_client_init\r\n");
	mqtt_client_init(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));
	
	BK_LOGD(NULL, "[MQTT]NetworkConnect\r\n");
	if ((rc = NetworkConnect(&network, link_name, 1883)) != 0)
	{
		BK_LOGD(NULL, "[MQTT]NetworkConnect failed:%d\n", rc);
		goto failed_exit;
	}

#if MQTT_TASK
	BK_LOGD(NULL, "[MQTT]StartTask\r\n");
	if ((rc = mqtt_start_task(&client)) != pdPASS)
		BK_LOGD(NULL, "[MQTT]Return code from start tasks is %d\n", rc);
#endif

	snprintf(topicName,sizeof(topicName),"bk72xx/sample/%x",rtos_get_time()&0xFF);
	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "bk72xx_mqtt";

	BK_LOGD(NULL, "[MQTT]Connecting\r\n");
	if ((rc = mqtt_connect(&client, &connectData)) != 0)
	{
		BK_LOGD(NULL, "[MQTT]Return code from connect is %d\n", rc);
		goto failed_exit;
	}
	else
	{
		BK_LOGD(NULL, "[MQTT]Connected\n");
	}
	
	BK_LOGD(NULL, "[MQTT]Subscribe\r\n");
	if ((rc = mqtt_subscribe(&client, "bk72xx/sample/#", 2, echo_msg_arrived_cb)) != 0)
	{
		BK_LOGD(NULL, "[MQTT]Return code from subscribe is %d\n", rc);
	}
	
	MQTTMessage message;
	char payload[30];
	
	fail_cnt = 0;
	while (++ count)
	{
		message.qos = 1;
		message.retained = 0;
		message.payload = payload;
		sprintf(payload, "message number %d", count);
		message.payloadlen = strlen(payload);

		BK_LOGD(NULL, "[MQTT]Publish\r\n");

		if ((rc = mqtt_publish(&client, topicName, &message)) != 0)
		{
			BK_LOGD(NULL, "[MQTT]Return code from publish is %d\n", rc);
			fail_cnt ++;

			if(fail_cnt > 20)
			{
				break;
			}
		}
		else
		{
			BK_LOGD(NULL, "[MQTT]Publish Succeed\r\n");
		}

#if !defined(MQTT_TASK)
		if ((rc = mqtt_yield(&client, 1000)) != 0)
		{
			BK_LOGD(NULL, "[MQTT]Return code from yield is %d\n", rc);
		}
#endif
	}

failed_exit:
	BK_LOGD(NULL, "[MQTT]exit\r\n\n");
	
	rtos_delete_thread( NULL );
	/* do not return */
}

void echo_start_tasks(int usTaskStackSize, int uxTaskPriority)
{
	rtos_create_thread(NULL,
                     uxTaskPriority,
                     "MQTTEcho0",
                     (beken_thread_function_t)echo_task_handler,
                     usTaskStackSize,
			         NULL);
}
#endif
static int test_is_started = 0;

static mqtt_client_session _mqttc_ses = {0};
struct sub_msg_handlers sub_topic[2] = {0};

char *mqt_strdup(const char *s)
{
    char *res;
    size_t len;

    if (s == NULL)
        return NULL;

    len = os_strlen(s);
    res = os_malloc(len + 1);
    if (res)
        os_memcpy(res, s, len + 1);

    return res;
}

static void mqtt_sub_callback(mqtt_client_session *c, MessageData *msg_data)
{
	TMQTT_LOG("Message arrived on topic %.*s: %.*s\r\n", msg_data->topicName->lenstring.len,
		msg_data->topicName->lenstring.data,
		msg_data->message->payloadlen, msg_data->message->payload);
    return;
}

static void mqtt_default_sub_callback(mqtt_client_session *c, MessageData *msg_data)
{
	TMQTT_LOG("default Message arrived on topic %.*s: %.*s\n", msg_data->topicName->lenstring.len,
		msg_data->topicName->lenstring.data,
		msg_data->message->payloadlen, msg_data->message->payload);
    return;
}

static void mqtt_connect_callback(mqtt_client_session *c)
{
    TMQTT_LOG("inter mqtt_connect_callback!\r\n");
}

static void mqtt_online_callback(mqtt_client_session *c)
{
    TMQTT_LOG("inter mqtt_online_callback!\r\n");
}

static void mqtt_offline_callback(mqtt_client_session *c,MQTT_EVNT_T ev)
{
    TMQTT_LOG("inter mqtt_offline_callback event %d\r\n",ev);
}

static void mqtt_notice_event_callback(mqtt_client_session *c,MQTT_EVNT_T ev)
{
    TMQTT_LOG("mqtt nitice event %d\r\n",ev);
}


static void mqtt_thread_pub(void *parameter)
{
	unsigned int pub_count = 0;
    char payload[60];

    while (1)
    {
    	snprintf(payload,sizeof(payload), "message number %d", pub_count);
        if (!mqtt_client_publish(&_mqttc_ses, QOS1, "7231s/mqtt_test/7231s", payload))
        {
            ++ pub_count;
			BK_LOGD(NULL, "pub:\"%s\" succeed\r\n",payload);
			if(pub_count > 50)
			{
				BK_LOGD(NULL, "mqtt client disconnect\r\n");
				mqtt_client_disconnect(&_mqttc_ses);
				rtos_delay_milliseconds(1000);
				mqtt_net_disconnect(&_mqttc_ses);
				break;
			}
        }
		else
		{
			BK_LOGD(NULL, "pub:\"%s\" failed\r\n",payload);
        }
		rtos_delay_milliseconds(1000);
    }
	mqtt_client_session_deinit(&_mqttc_ses);
	BK_LOGD(NULL, "[MQTT]exit\r\n\n");
	test_is_started = 0;
	rtos_delete_thread( NULL );
}


static int mqtt_demo_start_connect(char *host)
{
	char* link_name = "broker.mqttdashboard.com";
	int ret;
	
	if(host != NULL)
	{
		link_name = host;
	}
	
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	if(mqtt_client_session_init(&_mqttc_ses) != MQTT_OK)
	{
		goto _exit;
	}
#if 0
	tcp_mqtt_client_api_register( &_mqttc_ses.netport );

	if(mqtt_net_connect(&_mqttc_ses,link_name,1883) != MQTT_OK)
	{
		goto _exit;
	}
#else
	ssl_mqtt_client_api_register( &_mqttc_ses.netport );

	if(mqtt_net_connect(&_mqttc_ses,link_name,8883) != MQTT_OK)
	{
		goto _exit;
	}

#endif
	
	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "my_client_bk72xx_mqtt";
	connectData.username.cstring = "mqtt_user1";
	connectData.password.cstring = "123";
	connectData.keepAliveInterval = 30000;
	connectData.cleansession = 1;

	_mqttc_ses.buf_size = MQTT_DEFUALT_BUF_SIZE;
	_mqttc_ses.buf = os_malloc(_mqttc_ses.buf_size);
	_mqttc_ses.readbuf_size = MQTT_DEFUALT_BUF_SIZE;
	_mqttc_ses.readbuf = os_malloc(_mqttc_ses.readbuf_size);

	if (!(_mqttc_ses.readbuf && _mqttc_ses.buf))
    {
        BK_LOGD(NULL, "no memory for MQTT client buffer!\n");
        goto _exit;
    }
	
	/* set event callback function */
    _mqttc_ses.connect_callback = mqtt_connect_callback;
    _mqttc_ses.online_callback = mqtt_online_callback;
    _mqttc_ses.offline_callback = mqtt_offline_callback;
	_mqttc_ses.mqtt_notice_cb = mqtt_notice_event_callback;
	
	/* set subscribe table and event callback */
	_mqttc_ses.sub.messageHandlers = sub_topic;
	if(sub_topic[0].topicFilter == NULL)
	{
    	sub_topic[0].topicFilter = mqt_strdup("7231s/mqtt_test_sub/#");
	}
    sub_topic[0].callback = mqtt_sub_callback;
	sub_topic[0].qos = QOS0;
	if(sub_topic[1].topicFilter == NULL)
	{
		sub_topic[1].topicFilter = mqt_strdup("7231s/mqtt_win_sub/windows10");
	}
    sub_topic[1].callback = mqtt_sub_callback;
	sub_topic[1].qos = QOS1;
	_mqttc_ses.sub.sub_topic_num = sizeof(sub_topic)/sizeof(struct sub_msg_handlers);
	_mqttc_ses.defaultMessageHandler =  mqtt_default_sub_callback;
	
	ret = matt_client_connect(&_mqttc_ses,&connectData);
	if(MQTT_OK != ret)
	{
		BK_LOGD(NULL, "[MQTT]connect failed\r\n");
		goto _exit;
	}
	BK_LOGD(NULL, "[MQTT]connect succeed\r\n");

	return 0;
_exit:
	if(_mqttc_ses.readbuf)
	{
		os_free(_mqttc_ses.readbuf);
	}
	if(_mqttc_ses.buf)
	{
		os_free(_mqttc_ses.buf);
	}
	mqtt_client_session_deinit(&_mqttc_ses);
	return -1;
}

static void mqtt_test_start(void)
{
	if (test_is_started)
	{
		return;
	}

    if(mqtt_demo_start_connect("192.168.19.24") != 0)
	{
		return;
    }

    if (!_mqttc_ses.is_connected)
    {
        BK_LOGD(NULL, "Waiting for mqtt connection...\n");
		return;
    }

    rtos_create_thread(NULL,
                     BEKEN_DEFAULT_WORKER_PRIORITY,
                     "MQTT_pub",
                     (beken_thread_function_t)mqtt_thread_pub,
                     3 * 1024,
			         NULL);
	
	test_is_started = 1;

    return;
}


static void mqtt_demo_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

	if(argc >= 2)
	{
		if(strcmp(argv[1], "create_echo") == 0)
		{
			///echo_start_tasks(ECHO_STACK_SIZE, ECHO_THD_PRORITY);
		}
		else if(strcmp(argv[1], "con") == 0)
		{
			mqtt_test_start();
		}
		else if(strcmp(argv[1], "stop") == 0)
		{
			
		}
	}

	
}

static const struct cli_command mqtt_cli_cmd[] =
{
    {"MQTT", "MQTT", mqtt_demo_command},
};

void mqtt_app_demo_init(void)
{
	BK_LOGD(NULL, "[MQTT]%s:%d\r\n",__FUNCTION__,__LINE__);
	
	cli_register_commands(mqtt_cli_cmd, sizeof(mqtt_cli_cmd) / sizeof(struct cli_command));
	mqtt_core_handler_thread_init();

	tcp_mqtt_client_init();
}













