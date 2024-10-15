#include "zephyr.h"
#include "stdint.h"
#include "sys/z_poll.h"
#include "sys/__assert.h"

#ifndef ARG_UNUSED
#define ARG_UNUSED(x) (void)(x)
#endif

struct event_cb {
    sys_dlist_t next;
    struct k_poll_event *events;
    int num_events;
    beken_semaphore_t sem;
};

static sys_dlist_t event_cb_list = SYS_DLIST_STATIC_INIT(&event_cb_list);
static volatile int event_cb_counter = 0;

/* must be called with interrupts locked */
static inline void clear_event_registration(struct k_poll_event *event)
{
    event->poller.thd = NULL;

    switch (event->type) {
        case K_POLL_TYPE_DATA_AVAILABLE:
            __ASSERT(event->queue, "invalid queue\n");
            sys_dlist_remove(&event->_node);
            break;
        case K_POLL_TYPE_SIGNAL:
            __ASSERT(event->signal, "invalid poll signal\n");
            sys_dlist_remove(&event->_node);
            break;
		default:
            __ASSERT(0, "invalid event type\n");
            break;
    }
}

/* must be called with interrupts locked */
static inline void clear_event_registrations(struct k_poll_event *events,
                                             int          last_registered,
                                             unsigned int key)
{
	unsigned int lock = irq_lock();
	for (; last_registered >= 0; last_registered--) {
		clear_event_registration(&events[last_registered]);
	}
	irq_unlock(lock);
}

int kpoll_event_callback(uint8_t event_type,void *obj)
{
	sys_dnode_t *event_next;
	sys_dnode_t *event_next_save;
	struct k_poll_event *events;
	unsigned int key;
	int ret = -1;

	key = irq_lock();
	SYS_DLIST_FOR_EACH_NODE_SAFE(&event_cb_list, event_next, event_next_save) {
		for (int i = 0; i < ((struct event_cb *)event_next)->num_events; i++) {
			struct event_cb *events = (struct event_cb *)event_next;
			if ((events->events[i].type == event_type)
			    &&(events->events[i].obj == obj)) {
				rtos_set_semaphore(&((struct event_cb *)event_next)->sem);
				ret = 0;
				break;
			}
		}
	}
	irq_unlock(key);
	(void)events;
	return ret;
}

static inline void set_event_state(struct k_poll_event *event, uint32_t state)
{
    event->poller.thd = NULL;
    event->state |= state;
}


/* must be called with interrupts locked */
static inline int is_condition_met(struct k_poll_event *event, uint32_t *state)
{
	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		if (k_sem_count_get(event->sem) > 0U) {
			*state = K_POLL_STATE_SEM_AVAILABLE;
			return 1;
		}
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		if (!k_queue_is_empty(event->queue)) {
			*state = K_POLL_STATE_FIFO_DATA_AVAILABLE;
			return 1;
		}
		break;
	case K_POLL_TYPE_SIGNAL:
		if (event->signal->signaled) {
			*state = K_POLL_STATE_SIGNALED;
			return 1;
		}
		break;
	case K_POLL_TYPE_IGNORE:
		break;
	default:
		__ASSERT(false, "invalid event type (0x%x)\n", event->type);
		break;
	}

	return 0;
}

static inline void add_event(sys_dlist_t *events, struct k_poll_event *event,
                             struct _poller *poller)
{
	struct k_poll_event *pending;

	pending = (struct k_poll_event *)sys_dlist_peek_tail(events);
	if(pending == NULL){
		sys_dlist_append(events, &event->_node);
		return;
	}

	sys_dlist_append(events, &event->_node);
}

/* must be called with interrupts locked */
static inline int register_event(struct k_poll_event *event,
                                 struct _poller *     poller)
{
	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		__ASSERT(event->sem != NULL, "invalid semaphore\n");
		add_event(&event->sem->poll_events, event, poller);
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT(event->queue, "invalid queue\n");
		///BT_INFO("%s %d register event:%p\r\n",__FUNCTION__,__LINE__,event);
		add_event(&event->queue->poll_events, event, poller);
		break;
	case K_POLL_TYPE_SIGNAL:
		__ASSERT(event->signal, "invalid poll signal\n");
		add_event(&event->signal->poll_events, event, poller);
		break;
	case K_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT(0, "invalid event type\n");
		break;
	}

	event->poller = *poller;

	return 0;
}

static bool polling_events(struct k_poll_event *events, int num_events,
                           uint32_t timeout, int *last_registered)
{
	bool polling = true;
	unsigned int key;
	struct _poller poller;
	int ii;

	poller.thd = rtos_get_current_thread();

	for (ii = 0; ii < num_events; ii++) {
		uint32_t state;
		key = irq_lock();

		if (is_condition_met(&events[ii], &state)) {
			set_event_state(&events[ii], state);
			polling = false;
		} else if (timeout != BEKEN_NO_WAIT/*K_NO_WAIT*/ && polling) {
			register_event(&events[ii], &poller);
			++(*last_registered);
		}

		irq_unlock(key);
	}
	return polling;
}

static int _signal_poll_event(struct k_poll_event *event, uint32_t state, int *must_reschedule,void *obj)
{
	int ret = -1;
	*must_reschedule = 0;
	//struct _poller *poller = &event->poller;

	BT_DBG("event:%p event->type:%d\r\n",event,event->type);
	if (event->type == K_POLL_TYPE_DATA_AVAILABLE){
		BT_DBG("kpoll_event_callback\r\n");
		set_event_state(event, state);
		ret = kpoll_event_callback(K_POLL_TYPE_FIFO_DATA_AVAILABLE, obj);
	} else if (event->type == K_POLL_TYPE_SEM_AVAILABLE) {
		BT_DBG("sem kpoll_event_callback\r\n");
		set_event_state(event, state);
		ret = kpoll_event_callback(K_POLL_TYPE_SEM_AVAILABLE, obj);
	} else if (event->type == K_POLL_TYPE_SIGNAL) {
		BT_DBG("signal kpoll_event_callback\r\n");
		set_event_state(event, state);
		ret = kpoll_event_callback(K_POLL_TYPE_SIGNAL, obj);
	}
	//(void)poller;
	return ret;
}

int _handle_obj_poll_events(sys_dlist_t *events, uint32_t state,void *obj)
{
	struct k_poll_event *poll_event;
	int must_reschedule;
	int ret = -1;

	poll_event = (struct k_poll_event *)sys_dlist_get(events);
	BT_VER("events:%p, poll_event:%p", events, poll_event);
	if (poll_event) {
		ret = _signal_poll_event(poll_event, state, &must_reschedule,obj);
	}
	(void)must_reschedule;
	return ret;
}

void k_poll_event_init(struct k_poll_event *event, uint32_t type, int mode,
                       void *obj)
{
    __ASSERT(mode == K_POLL_MODE_NOTIFY_ONLY,
             "only NOTIFY_ONLY mode is supported\n");
    __ASSERT(type < (1 << _POLL_NUM_TYPES), "invalid type\n");
    __ASSERT(obj, "must provide an object\n");

    event->poller.thd = NULL;
    /* event->tag is left uninitialized: the user will set it if needed */
    event->type   = type;
    event->state  = K_POLL_STATE_NOT_READY;
    event->mode   = mode;
    event->unused = 0;
    event->obj    = obj;
	sys_dnode_init(&event->_node);
}

int k_poll(struct k_poll_event *events, int num_events, k_timeout_t timeout)
{
	int last_registered = -1;
	unsigned int key;
	bool polling = true;
	struct event_cb eventcb = {0};

	key = irq_lock();
	/* find events whose condition is already fulfilled */
	polling = polling_events(events, num_events, timeout.ticks, &last_registered);
	BT_DBG("polling_events:%d\r\n", polling);

	if (polling == false) {
		irq_unlock(key);
		goto exit;
	}

	eventcb.events = events;
	eventcb.num_events = num_events;
	if (rtos_init_semaphore_adv(&eventcb.sem, 1, 0) != kNoErr) {
		BT_INFO("init semphore error\r\n");
	}

	sys_dlist_append(&event_cb_list, (sys_dnode_t *)&eventcb);
	event_cb_counter++;

	irq_unlock(key);

	rtos_get_semaphore(&eventcb.sem, timeout.ticks);

	key = irq_lock();
	last_registered = -1;
	polling_events(events, num_events, BEKEN_NO_WAIT/*K_NO_WAIT*/, &last_registered);

	sys_dlist_remove((sys_dnode_t *)&eventcb);
	event_cb_counter--;
	irq_unlock(key);
	rtos_deinit_semaphore(&eventcb.sem);

exit:
	clear_event_registrations(events, last_registered, key);

	return 0;
	(void)last_registered;
}

int k_poll_signal_raise(struct k_poll_signal *signal, int result)
{
	#if (CONFIG_ACLSEMI_BTC_FEATURE_INCLUDED)
	unsigned int key =  irq_lock();

	signal->result = result;
	signal->signaled = 1U;

	if (sys_dlist_is_empty(&signal->poll_events)) {
		irq_unlock(key);
		return 0;
	}
	int rc = _handle_obj_poll_events(&signal->poll_events, K_POLL_STATE_SIGNALED,(void*)signal);
	irq_unlock(key);
	#else ///CONFIG_ACLSEMI_BTC_FEATURE_INCLUDED
	unsigned int key =  irq_lock();
	struct k_poll_event *poll_event;

	signal->result = result;
	signal->signaled = 1U;

	poll_event = (struct k_poll_event *)sys_dlist_get(&signal->poll_events);
	if (poll_event == NULL) {
		irq_unlock(key);
		return 0;
	}
	int rc = _handle_obj_poll_events(&signal->poll_events, K_POLL_STATE_SIGNALED,(void*)signal);
	irq_unlock(key);
	#endif

	return rc;
}

static inline void z_object_init(const void *obj)
{
	ARG_UNUSED(obj);
}

void k_poll_signal_init(struct k_poll_signal *signal)
{
	sys_dlist_init(&signal->poll_events);
	signal->signaled = 0U;
	signal->result = 0;
	/* signal->result is left unitialized */
	z_object_init(signal);
}

/////////////////////////// Test demo ///////////////////////////////////////
#if defined(Z_POLL_TEST_CFG) && Z_POLL_TEST_CFG
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
//#include <os/os.h>
////#include <os/mem.h>

#define TP_EV_COUNT 3
/* k_poll event tags */
enum {
	TP_EVENT_CMD_TX,
	TP_EVENT_CMD_TX2,
	TP_EVENT_CMD_SEM
};

static struct k_fifo _cmd_tx_queue = {0};
static struct k_fifo _cmd_tx_queue2 = {0};
static struct k_sem _cmd_tx_sem = {0};

static void tp_process_events(struct k_poll_event *ev, int count)
{
	int m_count = count;
	struct alloc_node *msg = NULL;
	BT_DBG("count %d\r\n", count);

	for (; count; ev++, count--) {
		BT_DBG("ev[%d].state %u\r\n",m_count - count, ev->state);

		switch (ev->state) {
		case K_POLL_STATE_SIGNALED:
			BT_INFO("events[%d] K_POLL_STATE_SIGNALED handler\r\n",m_count - count);
			break;
		case K_POLL_STATE_FIFO_DATA_AVAILABLE:
			BT_INFO("events[%d] K_POLL_STATE_FIFO_DATA_AVAILABLE handler\r\n",m_count - count);
			if(ev->type == K_POLL_TYPE_DATA_AVAILABLE){
				msg = k_fifo_get(ev->fifo,K_NO_WAIT);
				if(msg){
					char *msg_type = (char*)&msg->data;
					BT_INFO("kp msg_type:%d\r\n",msg_type[0]);
					k_free(msg);
					msg = NULL;
				}
			}
			break;
		case K_POLL_STATE_SEM_AVAILABLE:
			BT_INFO("events[%d] K_POLL_STATE_SEM_AVAILABLE handler\r\n",m_count - count);
			if(ev->type == K_POLL_TYPE_SEM_AVAILABLE) {
				if(k_sem_take(ev->sem, K_NO_WAIT) == 0){
					BT_INFO("k_sem_take succeed\r\n");
				}else{
					BT_INFO("k_sem_take failed\r\n");
				}
			}
			break;
		case K_POLL_STATE_NOT_READY:
			BT_INFO("events[%d] K_POLL_STATE_NOT_READY handler\r\n",m_count - count);
			break;
		default:
			BT_INFO("Unexpected k_poll event state %u\r\n", ev->state);
			break;
		}
	}
	(void)m_count;
}

static void kp_thread_function( void* arg)
{
	int ev_count, err;
	void* *thd =  rtos_get_current_thread();
	struct k_poll_event events[TP_EV_COUNT] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&_cmd_tx_queue,
						TP_EVENT_CMD_TX),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_DATA_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&_cmd_tx_queue2,
						TP_EVENT_CMD_TX2),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&_cmd_tx_sem,
						TP_EVENT_CMD_SEM),
	};

	if(k_fifo_is_uninitialized(&_cmd_tx_queue) != 0){
		k_fifo_init(&_cmd_tx_queue);
	}
	if(k_fifo_is_uninitialized(&_cmd_tx_queue2) != 0){
		k_fifo_init(&_cmd_tx_queue2);
	}
	if(k_sem_is_uninitialized(&_cmd_tx_sem)!= 0){
		k_sem_init(&_cmd_tx_sem,0,10);
	}
	BT_INFO("%s(thd:%p) run\r\n",__FUNCTION__,thd);

	while ( 1 ) {
		for(ev_count = 0;ev_count<TP_EV_COUNT;ev_count++){
			events[ev_count].state = K_POLL_STATE_NOT_READY;
		}

		BT_DBG("Calling k_poll with %d events\r\n", ev_count);

		err = k_poll(events, ev_count, K_FOREVER);
		if(err == 0){
			tp_process_events(events, ev_count);
		}
		else{
			BT_INFO("k_poll return %d error\r\n",err);
		}
		/* Make sure we don't hog the CPU if there's all the time
		 * some ready events.
		 */
		k_yield();
	}
	rtos_delete_thread(NULL);
}

void z_poll_test_command(int argc, char **argv)
{
	static char msg_type = 0;

	if ( strcmp(argv[1], "poll_init") == 0 ) {
		void* qthread = {0};
		rtos_create_thread(&qthread,
							7,
							"kpt",
							kp_thread_function,
							3 * 1024,
							NULL);
	}
	else if ( strcmp(argv[1], "poll_set") == 0 ) {
		BT_INFO("kpoll fifo put\r\n");
		if(k_fifo_is_uninitialized(&_cmd_tx_queue) != 0){
			k_fifo_init(&_cmd_tx_queue);
		}
		k_fifo_put(&_cmd_tx_queue,NULL);
	}
	else if ( strcmp(argv[1], "poll_set2") == 0 ) {
		BT_INFO("kpoll fifo 2 put\r\n");
		if(k_fifo_is_uninitialized(&_cmd_tx_queue2) != 0){
			k_fifo_init(&_cmd_tx_queue2);
		}
		k_fifo_put(&_cmd_tx_queue2,NULL);
	}else if ( strcmp(argv[1], "poll_set3") == 0 ) {
		BT_INFO("kpoll fifo3 put\r\n");
		if(k_fifo_is_uninitialized(&_cmd_tx_queue2) != 0){
			k_fifo_init(&_cmd_tx_queue2);
		}
		struct alloc_node *msg = k_malloc(sizeof(struct alloc_node) + 10);
		if(msg == NULL){
			BT_INFO("kpoll fifo 3 malloc failed\r\n");
			return;
		}
		char *data = (char *)&msg->data;
		data[0] = msg_type;
		msg_type++;
		k_fifo_put(&_cmd_tx_queue2,msg);
	}else  if ( strcmp(argv[1], "poll_sem") == 0 ) {
		if(k_sem_is_uninitialized(&_cmd_tx_sem)!= 0){
			k_sem_init(&_cmd_tx_sem,0,10);
		}
		k_sem_give(&_cmd_tx_sem);
	}
}



#endif

///Eof



