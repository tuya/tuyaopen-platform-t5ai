#ifndef __ASR_MB_H__
#define __ASR_MB_H__

typedef enum {
	EVENT_ASR_OPEN = 0,
	EVENT_ASR_CLOSE,
	EVENT_ASR_PROCESS,
	EVENT_ASR_RESULT
} asr_mb_event_t;

typedef enum {
	ASR_RESULT_MAJORDOMO = 0,	//小蜂管家
	ASR_RESULT_ARMINO,			//阿尔米诺
	ASR_RESULT_RECEPTION,		//会客模式
	ASR_RESULT_MEAL,			//用餐模式
	ASR_RESULT_LEAVE,			//离开模式
	ASR_RESULT_GOHOME			//回家模式
} asr_result_t;

typedef struct {
	asr_mb_event_t op;
	uint32_t data;
} asr_mb_msg_t;

#endif