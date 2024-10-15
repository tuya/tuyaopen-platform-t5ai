#include <common/bk_include.h>
#include <os/os.h>
#include <modules/pm.h>
#include "FreeRTOS.h"
#include "task.h"
#include "audio_pipeline.h"
#include "audio_mem.h"
#include "aec_algorithm.h"
#include "raw_stream.h"
#include "onboard_mic_stream.h"
#include "onboard_speaker_stream.h"
#include "g711_encoder.h"
#include "g711_decoder.h"

#include "voice_pipeline.h"


#define TAG "voice_pipeline"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define AEC_ENABLE    (1)
#define RAW_READ_SIZE    (160)//(1280)

#define TEST_CHECK_NULL(ptr) do {\
		if (ptr == NULL) {\
			BK_LOGI(TAG, "TEST_CHECK_NULL fail \n");\
			return BK_FAIL;\
		}\
	} while(0)

static audio_element_handle_t onboard_mic = NULL, g711_enc = NULL, raw_read = NULL;


static audio_element_handle_t raw_write = NULL, g711_dec  = NULL, onboard_spk = NULL;
static audio_pipeline_handle_t record_pipeline = NULL, play_pipeline = NULL;

#if AEC_ENABLE
static audio_element_handle_t aec_alg = NULL;
static ringbuf_handle_t aec_alg_ref_rb = NULL;
#endif

static voice_setup_t voice_context = {NULL};
static beken_thread_t voice_send_mic_task_hdl = NULL;
static beken_queue_t voice_send_mic_msg_que = NULL;

static bk_err_t record_pipeline_open(void)
{
	BK_LOGI(TAG, "--------- step1: record pipeline init ----------\n");
	audio_pipeline_cfg_t record_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	record_pipeline_cfg.rb_size = 320;
	record_pipeline = audio_pipeline_init(&record_pipeline_cfg);
	TEST_CHECK_NULL(record_pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
	onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
	onboard_mic_cfg.task_stack = 1024;
	onboard_mic = onboard_mic_stream_init(&onboard_mic_cfg);
	TEST_CHECK_NULL(onboard_mic);

#if AEC_ENABLE
	aec_algorithm_cfg_t aec_alg_cfg = DEFAULT_AEC_ALGORITHM_CONFIG();
	//aec_alg_cfg.aec_cfg.fs = 16000;
	aec_alg_cfg.aec_cfg.mode = AEC_MODE_SOFTWARE;
	aec_alg_cfg.out_rb_block = 1;
//	aec_alg_cfg.task_stack = 500;
	aec_alg = aec_algorithm_init(&aec_alg_cfg);
	TEST_CHECK_NULL(aec_alg);
#endif

	g711_encoder_cfg_t g711_encoder_cfg = DEFAULT_G711_ENCODER_CONFIG();
	g711_encoder_cfg.out_rb_size = 160;
//	g711_encoder_cfg.task_stack = 500;
	g711_enc = g711_encoder_init(&g711_encoder_cfg);
	TEST_CHECK_NULL(g711_enc);

	raw_stream_cfg_t raw_read_cfg = RAW_STREAM_CFG_DEFAULT();
	raw_read_cfg.type = AUDIO_STREAM_READER;
	raw_read_cfg.out_rb_size = 160;
	raw_read = raw_stream_init(&raw_read_cfg);
	TEST_CHECK_NULL(raw_read);


	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
	if (BK_OK != audio_pipeline_register(record_pipeline, onboard_mic, "onboard_mic")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#if AEC_ENABLE
	if (BK_OK != audio_pipeline_register(record_pipeline, aec_alg, "aec_alg")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif
	if (BK_OK != audio_pipeline_register(record_pipeline, g711_enc, "g711_enc")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(record_pipeline, raw_read, "raw_read")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
	/* pipeline record */
#if AEC_ENABLE
	if (BK_OK != audio_pipeline_link(record_pipeline, (const char *[]) {"onboard_mic", "aec_alg", "g711_enc", "raw_read"}, 4)) {
#else
	if (BK_OK != audio_pipeline_link(record_pipeline, (const char *[]) {"onboard_mic", "g711_enc", "raw_read"}, 3)) {
#endif
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

#if 0
	BK_LOGI(TAG, "--------- step5: init event listener ----------\n");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
	if (BK_OK != audio_pipeline_set_listener(record_pipeline, evt)) {
		BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif
	return BK_OK;
}

static bk_err_t record_pipeline_close(void)
{
	BK_LOGI(TAG, "%s \n", __func__);
#if 0
	BK_LOGI(TAG, "%s, stop record pipeline \n", __func__);
	if (BK_OK != audio_pipeline_stop(record_pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_wait_for_stop(record_pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif
	BK_LOGI(TAG, "%s, terminate record pipeline \n", __func__);
	if (BK_OK != audio_pipeline_terminate(record_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(record_pipeline, onboard_mic)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#if AEC_ENABLE
	if (BK_OK != audio_pipeline_unregister(record_pipeline, aec_alg)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif
	if (BK_OK != audio_pipeline_unregister(record_pipeline, g711_enc)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(record_pipeline, raw_read)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

#if 0
	if (BK_OK != audio_pipeline_remove_listener(record_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_event_iface_destroy(evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif

	if (BK_OK != audio_pipeline_deinit(record_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		record_pipeline = NULL;
	}
	if (BK_OK != audio_element_deinit(onboard_mic)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		onboard_mic = NULL;
	}
#if AEC_ENABLE
	if (BK_OK != audio_element_deinit(aec_alg)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		aec_alg = NULL;
	}
#endif
	if (BK_OK != audio_element_deinit(g711_enc)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		g711_enc = NULL;
	}
	if (BK_OK != audio_element_deinit(raw_read)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		raw_read = NULL;
	}

	return BK_OK;
}

static bk_err_t play_pipeline_open(void)
{
	BK_LOGI(TAG, "--------- step1: play pipeline init ----------\n");
	audio_pipeline_cfg_t play_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	play_pipeline_cfg.rb_size = 320;
	play_pipeline = audio_pipeline_init(&play_pipeline_cfg);
	TEST_CHECK_NULL(play_pipeline);

	BK_LOGI(TAG, "--------- step2: init elements ----------\n");
	raw_stream_cfg_t raw_write_cfg = RAW_STREAM_CFG_DEFAULT();
	raw_write_cfg.type = AUDIO_STREAM_WRITER;
	raw_write_cfg.out_rb_size = 320;
	raw_write = raw_stream_init(&raw_write_cfg);
	TEST_CHECK_NULL(raw_write);

	g711_decoder_cfg_t g711_decoder_cfg = DEFAULT_G711_DECODER_CONFIG();
	g711_decoder_cfg.out_rb_size = 320;
//	g711_decoder_cfg.task_stack = 500;
	g711_dec = g711_decoder_init(&g711_decoder_cfg);
	TEST_CHECK_NULL(g711_dec);

	onboard_speaker_stream_cfg_t onboard_spk_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
	onboard_spk_cfg.multi_out_rb_num = 1;
	onboard_spk_cfg.task_stack = 1024;
//	onboard_spk_cfg.task_stack = 500;
	onboard_spk = onboard_speaker_stream_init(&onboard_spk_cfg);
	TEST_CHECK_NULL(onboard_spk);

	BK_LOGI(TAG, "--------- step3: pipeline register ----------\n");
	if (BK_OK != audio_pipeline_register(play_pipeline, raw_write, "raw_write")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(play_pipeline, g711_dec, "g711_dec")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_register(play_pipeline, onboard_spk, "onboard_spk")) {
		BK_LOGE(TAG, "register element fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	BK_LOGI(TAG, "--------- step4: pipeline link ----------\n");
	/* pipeline play */
	if (BK_OK != audio_pipeline_link(play_pipeline, (const char *[]) {"raw_write", "g711_dec", "onboard_spk"}, 3)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

#if 0
	BK_LOGI(TAG, "--------- step5: init event listener ----------\n");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
	if (BK_OK != audio_pipeline_set_listener(record_pipeline, evt)) {
		BK_LOGE(TAG, "set listener fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif
	return BK_OK;
}

static bk_err_t play_pipeline_close(void)
{
	BK_LOGI(TAG, "%s \n", __func__);

#if 0
	BK_LOGI(TAG, "%s, stop play pipeline \n", __func__);
	if (BK_OK != audio_pipeline_stop(play_pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_wait_for_stop(play_pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		play_pipeline = NULL;
	}
#endif

	BK_LOGI(TAG, "%s, terminate play pipeline \n", __func__);
	if (BK_OK != audio_pipeline_terminate(play_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(play_pipeline, raw_write)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(play_pipeline, g711_dec)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
	if (BK_OK != audio_pipeline_unregister(play_pipeline, onboard_spk)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

#if 0
	if (BK_OK != audio_pipeline_remove_listener(play_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_event_iface_destroy(evt)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif

	if (BK_OK != audio_pipeline_deinit(play_pipeline)) {
		BK_LOGE(TAG, "pipeline terminate fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		play_pipeline = NULL;
	}
	if (BK_OK != audio_element_deinit(raw_write)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		raw_write = NULL;
	}
	if (BK_OK != audio_element_deinit(g711_dec)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		g711_dec = NULL;
	}
	if (BK_OK != audio_element_deinit(onboard_spk)) {
		BK_LOGE(TAG, "element deinit fail, %d \n", __LINE__);
		return BK_FAIL;
	} else {
		onboard_spk = NULL;
	}

	return BK_OK;
}

static bk_err_t voice_read_mic_data(char *buffer, uint32_t size)
{
	return raw_stream_read(raw_read, buffer, size);
}

static bk_err_t send_mic_data_send_msg(voice_send_mic_op_t op, void *param)
{
	bk_err_t ret;
	voice_send_mic_msg_t msg;

	msg.op = op;
	msg.param = param;
	if (voice_send_mic_msg_que) {
		ret = rtos_push_to_queue(&voice_send_mic_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("send_mic_data_send_msg fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static void send_mic_data_task_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	int read_size = 0;

	uint8_t *aud_temp_data = os_malloc(RAW_READ_SIZE);
	if (!aud_temp_data)
	{
		BK_LOGE(TAG, "malloc aud_temp_data\n");
		goto voice_send_exit;
	}
	os_memset(aud_temp_data, 0, RAW_READ_SIZE);

	voice_send_mic_op_t task_state = VOICE_SEND_MIC_IDLE;

	voice_send_mic_msg_t msg;
	uint32_t wait_time = BEKEN_WAIT_FOREVER;
	while (1) {
		ret = rtos_pop_from_queue(&voice_send_mic_msg_que, &msg, wait_time);
		if (kNoErr == ret) {
			switch (msg.op) {
				case VOICE_SEND_MIC_IDLE:
					task_state = VOICE_SEND_MIC_IDLE;
					wait_time = BEKEN_WAIT_FOREVER;
					break;

				case VOICE_SEND_MIC_EXIT:
					LOGD("goto: VOICE_SEND_MIC_EXIT \r\n");
					goto voice_send_exit;
					break;

				case VOICE_SEND_MIC_START:
					task_state = VOICE_SEND_MIC_START;
					wait_time = 0;
					break;

				default:
					break;
			}
		}

		/* read mic data and send */
		if (task_state == VOICE_SEND_MIC_START) {
			//GPIO_UP(6);
			read_size = voice_read_mic_data((char *)aud_temp_data, RAW_READ_SIZE);
			if (read_size > 0) {
				read_size = voice_context.voice_send_packet(aud_temp_data, read_size);
			} else {
				BK_LOGE(TAG, "voice_read_mic_data fail, read_size: %d \n", read_size);
			}
			//GPIO_DOWN(6);
		}

	}

voice_send_exit:
	if (aud_temp_data) {
		os_free(aud_temp_data);
		aud_temp_data == NULL;
	}

	/* delete msg queue */
	ret = rtos_deinit_queue(&voice_send_mic_msg_que);
	if (ret != kNoErr) {
		LOGE("delete message queue fail \r\n");
	}
	voice_send_mic_msg_que = NULL;
	LOGI("delete send_mic_que \r\n");

	/* delete task */
	voice_send_mic_task_hdl = NULL;

	rtos_delete_thread(NULL);
}

static bk_err_t send_mic_data_init(void)
{
	bk_err_t ret = BK_OK;

	ret = rtos_init_queue(&voice_send_mic_msg_que,
						  "send_mic_que",
						  sizeof(voice_send_mic_msg_t),
						  2);
	if (ret != kNoErr)
	{
		LOGE("ceate voice send mic data message queue fail\n");
		return BK_FAIL;
	}
	LOGI("ceate voice send mic data message queue complete\n");

	ret = rtos_create_thread(&voice_send_mic_task_hdl,
							 6,
							 "send_mic_task",
							 (beken_thread_function_t)send_mic_data_task_main,
							 1024,
							 NULL);
	if (ret != kNoErr)
	{
		LOGE("Error: Failed to create send_mic_task task \n");
		ret = rtos_deinit_queue(&voice_send_mic_msg_que);
		if (ret != kNoErr) {
			LOGE("delete message queue fail \r\n");
		}
		voice_send_mic_msg_que = NULL;
		return kGeneralErr;
	}

	LOGI("init send_mic_task task complete \n");

	return BK_OK;
}


bk_err_t voice_init(voice_setup_t setup)
{
	if (!setup.voice_send_packet) {
		BK_LOGE(TAG, "voice_send_packet is NULL \n");
		return BK_FAIL;
	}

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_320M);

	voice_context.voice_send_packet = setup.voice_send_packet;

	/* init send mic data task */
	send_mic_data_init();

	record_pipeline_open();
	play_pipeline_open();

#if AEC_ENABLE
	if (!aec_alg_ref_rb) {
		aec_alg_ref_rb = rb_create(320, 1);
		TEST_CHECK_NULL(aec_alg_ref_rb);
	}

	if (BK_OK !=  audio_element_set_multi_input_ringbuf(aec_alg, aec_alg_ref_rb, 0)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK !=  audio_element_set_multi_output_ringbuf(onboard_spk, aec_alg_ref_rb, 0)) {
		BK_LOGE(TAG, "pipeline link fail, %d \n", __LINE__);
		return BK_FAIL;
	}
#endif

	return BK_OK;
}

bk_err_t voice_deinit(void)
{
	send_mic_data_send_msg(VOICE_SEND_MIC_EXIT, NULL);

	record_pipeline_close();
	play_pipeline_close();

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_AUDIO, PM_CPU_FRQ_DEFAULT);

#if AEC_ENABLE
	if (aec_alg_ref_rb) {
		rb_destroy(aec_alg_ref_rb);
		aec_alg_ref_rb = NULL;
	}
#endif

	return BK_OK;
}

bk_err_t voice_start(void)
{
	if (BK_OK != audio_pipeline_run(record_pipeline)) {
		BK_LOGE(TAG, "pipeline run fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_run(play_pipeline)) {
		BK_LOGE(TAG, "pipeline run fail \n");
		return BK_FAIL;
	}

	send_mic_data_send_msg(VOICE_SEND_MIC_START, NULL);

	return BK_OK;
}

bk_err_t voice_stop(void)
{
	send_mic_data_send_msg(VOICE_SEND_MIC_IDLE, NULL);

	if (BK_OK != audio_pipeline_stop(record_pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_stop(play_pipeline)) {
		BK_LOGE(TAG, "pipeline stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_wait_for_stop(record_pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}

	if (BK_OK != audio_pipeline_wait_for_stop(play_pipeline)) {
		BK_LOGE(TAG, "pipeline wait stop fail, %d \n", __LINE__);
		return BK_FAIL;
	}


	return BK_OK;
}

bk_err_t voice_write_spk_data(char *buffer, uint32_t size)
{
	return raw_stream_write(raw_write, buffer, size);
}

