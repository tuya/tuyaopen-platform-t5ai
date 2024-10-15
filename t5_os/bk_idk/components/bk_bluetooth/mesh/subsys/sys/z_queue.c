#include "zephyr.h"

#include <os/mem.h>

#include "sys/z_queue.h"
#include "sys/z_poll.h"

extern unsigned int Z_WORK_SUB_TIME_DELT(uint32_t a, uint32_t b);


void k_queue_init(struct k_queue *queue)
{
	if(queue == NULL) {
		return;
	}
	sys_slist_init(&queue->data_q);
///	rtos_init_semaphore(&queue->lock);
	rtos_init_semaphore(&queue->sem,Z_QUEUE_SEM_SIZE);
	sys_dlist_init(&queue->poll_events);
}

static inline int handle_poll_events(struct k_queue *queue, uint32_t state)
{
    return _handle_obj_poll_events(&queue->poll_events, state,(void*)queue);
}

void queue_cancel_wait(struct k_queue *queue)
{
	handle_poll_events(queue, K_POLL_STATE_NOT_READY);
}

static int32_t queue_insert(struct k_queue *queue, void *prev, void *data,
					bool alloc, bool is_append)
{
	unsigned int key;
	sys_snode_t *node = (sys_snode_t *)data;

	if(queue == NULL){
		return -1;
	}
#if ((!defined(Z_QUEUE_ALLOC_NODE)) || (!Z_QUEUE_ALLOC_NODE))
	if (alloc) {
		return -1;
	}
#endif
    node->next = NULL;
	key = irq_lock();
	if (is_append) {
		prev = sys_slist_peek_tail(&queue->data_q);
	}

#if Z_QUEUE_ALLOC_NODE
	if (alloc) {
		struct alloc_node *anode;

		anode = k_malloc(sizeof(struct alloc_node));
		if (anode == NULL) {
			irq_unlock(key);
			return -ENOMEM;
		}
		memset(anode,0,sizeof(struct alloc_node));
		anode->data = data;
		data = anode;
	}
#endif
	if ( data != NULL ) {
    	sys_slist_insert(&queue->data_q, prev, data);
	}

	if(queue->sem == NULL) {
		rtos_init_semaphore(&queue->sem,Z_QUEUE_SEM_SIZE);
	}

    if(handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE) != 0){
		rtos_set_semaphore(&queue->sem);
    }
	irq_unlock(key);

	return 0;
}

void k_queue_append(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, NULL, data, false, true);
}

int32_t k_queue_alloc_append(struct k_queue *queue, void *data)
{
	return queue_insert(queue, NULL, data, true, true);
}

void k_queue_prepend(struct k_queue *queue, void *data)
{
	(void)queue_insert(queue, sys_slist_peek_tail(&queue->data_q), data, false, false);
}

int32_t k_queue_alloc_prepend(struct k_queue *queue, void *data)
{
	return queue_insert(queue, NULL, data, true, false);
}

void k_queue_insert(struct k_queue *queue, void *prev, void *data)
{
	(void)queue_insert(queue, prev, data, false, false);
}

int k_queue_append_list(struct k_queue *queue, void *head, void *tail)
{
	unsigned int key;

	if(queue == NULL){
		return -EINVAL;
	}
	/* invalid head or tail of list */
	if(head == NULL || tail == NULL) {
		return -EINVAL;
	}
	key = irq_lock();

	sys_slist_append_list(&queue->data_q, head, tail);
	if(queue->sem == NULL) {
		rtos_init_semaphore(&queue->sem,Z_QUEUE_SEM_SIZE);
	}

	handle_poll_events(queue, K_POLL_STATE_DATA_AVAILABLE);
	rtos_set_semaphore(&queue->sem);

	irq_unlock(key);

	return 0;
}

void *k_queue_get(struct k_queue *queue, k_timeout_t timeout)
{
	unsigned int key;
	void *data = NULL;
	int ret = kGeneralErr;
	uint32_t prev = k_uptime_get_32();

	if(queue == NULL){
		return NULL;
	}
retry:
	key = irq_lock();
	data = sys_slist_get(&queue->data_q);
	if(queue->sem == NULL) {
		rtos_init_semaphore(&queue->sem,Z_QUEUE_SEM_SIZE);
	}

	if ( data != NULL ) {
		rtos_get_semaphore(&queue->sem,BEKEN_NO_WAIT);
		irq_unlock(key);
		return data;
	}else if(rtos_get_sema_count(&queue->sem) > 0){
		do{
			ret = rtos_get_semaphore(&queue->sem,BEKEN_NO_WAIT);
			if(kNoErr != ret) {
				break;
			}
		}while(rtos_get_sema_count(&queue->sem) > 0);
	}

	if (timeout.ticks == BEKEN_NO_WAIT/*K_NO_WAIT*/) {
		irq_unlock(key);
		return NULL;
	}
	uint32_t now = k_uptime_get_32();
	unsigned int delt = Z_WORK_SUB_TIME_DELT(now , prev);
	timeout.ticks = (timeout.ticks > delt) ? (timeout.ticks - delt) : 0 ;
	irq_unlock(key);

	if(timeout.ticks > 0){
		prev = now;
		ret = rtos_get_semaphore(&queue->sem,timeout.ticks);
	}
	if(kNoErr == ret) {
		goto retry;
	}

	return NULL;
}

int k_queue_is_empty(struct k_queue *queue)
{
#if 0   ///run default check queue is empty.this is for check NULL queue msg send,but This is not a normal application
	///There may be some unconsidered logical problems with opening this. somewhere unknown
	if(!sys_slist_is_empty(&queue->data_q)){
		return 0;
	}
	if(rtos_get_sema_count(&queue->sem) > 0){
		return 0;
	}
	return 1;
#else  ///default check queue is empty.
	return (int)sys_slist_is_empty(&queue->data_q);
#endif
}

int k_queue_is_uninitialized(struct k_queue *queue)
{
	if(queue && queue->sem){
		return 0;
	}
	if(queue){
		return 1;
	}else{
		return -1;
	}
}

/////////////////////////// Test demo ///////////////////////////////////////
#if defined(Z_QUEUE_TEST_CFG) && Z_QUEUE_TEST_CFG
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
//#include <os/os.h>
////#include <os/mem.h>

#define Z_QUEUE_TEST_NUM  2
static struct k_queue _k_queue[Z_QUEUE_TEST_NUM];
struct kq_test_node {
	sys_snode_t node;
	char msg_type;
	char data[1];
};

static void q_thread_function( void* arg)
{
	struct k_queue *kq = (struct k_queue *)arg;
	k_timeout_t timeout;
	void* *thd =  rtos_get_current_thread();
	struct kq_test_node* msg = NULL;

	BT_INFO("%s(thd:%p) run\r\n",__FUNCTION__,thd);
	timeout.ticks = 5 * 1000;
	while(1){
		msg = k_queue_get(kq,timeout);
		if( msg == NULL ) {
			BT_INFO("%s(thd:%p) timeout %d mS\r\n",__FUNCTION__,thd,timeout.ticks);
		} else {
			BT_INFO("%s(thd:%p) get msg_type:%d\r\n",__FUNCTION__,thd,msg->msg_type);
			os_free(msg);
		}
	}
	rtos_delete_thread(NULL);
}

void z_queue_test_command(int argc, char **argv)
{
	int idx = 0;

	if ( strcmp(argv[1], "queue_init") == 0 ) {
		if ( argc == 3 ) {
			idx = atoi(argv[2]);
			if(idx >= Z_QUEUE_TEST_NUM){
				BT_INFO("k_queue index(%d) > %d error\r\n",idx,Z_QUEUE_TEST_NUM);
				return;
			}
		}
		if(k_queue_is_uninitialized(&_k_queue[idx]) != 0){
			k_queue_init(&_k_queue[idx]);
		}
		void* qthread = {0};
		rtos_create_thread(&qthread,
							7,
							"zqt",
							q_thread_function,
							1024,
							&_k_queue[idx]);
	}
	else if ( strcmp(argv[1], "queue_set") == 0 ) {
		if ( argc == 3 ) {
			idx = atoi(argv[2]);
			if(idx >= Z_QUEUE_TEST_NUM){
				BT_INFO("k_queue index(%d) > %d error\r\n",idx,Z_QUEUE_TEST_NUM);
				return;
			}
		}
		static char msg_type = 0;
		struct kq_test_node* msg = os_malloc(sizeof(struct kq_test_node));

		if(msg){
			BT_INFO("k_queue msg_type:%d\r\n",msg_type);
			msg->node.next = NULL;
			msg->msg_type = msg_type;
			msg_type++;
			k_queue_append(&_k_queue[idx],(void*)msg);
		}else{
			BT_INFO("k_queue msg malloc failed\r\n");
		}
	}
}



#endif

///Eof

