/*
 * FreeRTOS V202012.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */


/*
 * Tests the extra queue functionality introduced in FreeRTOS.org V4.5.0 -
 * including xQueueSendToFront(), xQueueSendToBack(), xQueuePeek() and
 * mutex behaviour.
 *
 * See the comments above the prvSendFrontAndBackTest() and
 * prvLowPriorityMutexTask() prototypes below for more information.
 */

/* Standard includes. */
#include <stdlib.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define genqQUEUE_LENGTH		( 5 )
#define intsemNO_BLOCK			( 0 )
#define genqSHORT_BLOCK			( pdMS_TO_TICKS( 2 ) )

#define genqMUTEX_LOW_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define genqMUTEX_TEST_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define genqMUTEX_MEDIUM_PRIORITY	( tskIDLE_PRIORITY + 3 )
#define genqMUTEX_HIGH_PRIORITY		( tskIDLE_PRIORITY + 4 )

#define FUNC_OPTIMIZE_LEVEL              __attribute__((optimize("-O0")))
#define CONFIG_PRIORITY_INVERSION_SINGLE_CORE       0
#define CONFIG_PRIO_INVERSION_DEBUG        1
#define X_ERROR_DETECTED()         do{\
										xErrorDetected = pdTRUE;\
									}while(0)

#ifndef genqMUTEX_TEST_TASK_STACK_SIZE
	#define genqMUTEX_TEST_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#endif

#ifndef genqGENERIC_QUEUE_TEST_TASK_STACK_SIZE
	#define genqGENERIC_QUEUE_TEST_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#endif
/*-----------------------------------------------------------*/

/*
 * Tests the behaviour of the xQueueSendToFront() and xQueueSendToBack()
 * macros by using both to fill a queue, then reading from the queue to
 * check the resultant queue order is as expected.  Queue data is also
 * peeked.
 */
static void prvSendFrontAndBackTest( void *pvParameters );

void vStartGenericQueueTasks( UBaseType_t uxPriority );
BaseType_t xAreGenericQueueTasksStillRunning( void );
void vMutexISRInteractionTest( void );

/*
 * The following three tasks are used to demonstrate the mutex behaviour.
 * Each task is given a different priority to demonstrate the priority
 * inheritance mechanism.
 *
 * The low priority task obtains a mutex.  After this a high priority task
 * attempts to obtain the same mutex, causing its priority to be inherited
 * by the low priority task.  The task with the inherited high priority then
 * resumes a medium priority task to ensure it is not blocked by the medium
 * priority task while it holds the inherited high priority.  Once the mutex
 * is returned the task with the inherited priority returns to its original
 * low priority, and is therefore immediately preempted by first the high
 * priority task and then the medium priority task before it can continue.
 */
static void prvLowPriorityMutexTask( void *pvParameters );
static void prvMediumPriorityMutexTask( void *pvParameters );
static void prvHighPriorityMutexTask( void *pvParameters );

/*
 * Tests the behaviour when a low priority task inherits the priority of a
 * higher priority task when taking two mutexes, and returns the mutexes in
 * first the same order as the two mutexes were obtained, and second the
 * opposite order as the two mutexes were obtained.
 */
#if CONFIG_PRIO_INVERSION_DEBUG
void prvTakeTwoMutexesReturnInDifferentOrder( SemaphoreHandle_t xMutex, SemaphoreHandle_t xLocalMutex ) FUNC_OPTIMIZE_LEVEL;
void prvTakeTwoMutexesReturnInSameOrder( SemaphoreHandle_t xMutex, SemaphoreHandle_t xLocalMutex ) FUNC_OPTIMIZE_LEVEL;
void prvLowPriorityMutexTask( void *pvParameters ) FUNC_OPTIMIZE_LEVEL;
#else
static void prvTakeTwoMutexesReturnInSameOrder( SemaphoreHandle_t xMutex, SemaphoreHandle_t xLocalMutex );
static void prvTakeTwoMutexesReturnInDifferentOrder( SemaphoreHandle_t xMutex, SemaphoreHandle_t xLocalMutex );
#endif

#if( INCLUDE_xTaskAbortDelay == 1 )
	#if( configUSE_PREEMPTION == 0 )
		#error The additional tests included when INCLUDE_xTaskAbortDelay is 1 expect preemption to be used.
	#endif

	/* Tests the behaviour when a low priority task inherits the priority of a
	high priority task only for the high priority task to timeout before
	obtaining the mutex. */
	static void prvHighPriorityTimeout( SemaphoreHandle_t xMutex );
#endif

/*-----------------------------------------------------------*/

/* Flag that will be latched to pdTRUE should any unexpected behaviour be
detected in any of the tasks. */
static volatile BaseType_t xErrorDetected = pdFALSE;

/* Counters that are incremented on each cycle of a test.  This is used to
detect a stalled task - a test that is no longer running. */
static volatile uint32_t ulLoopCounter = 0;
static volatile uint32_t ulLoopCounter2 = 0;

/* The variable that is guarded by the mutex in the mutex demo tasks. */
static volatile uint32_t ulGuardedVariable = 0;

/* Handles used in the mutex test to suspend and resume the high and medium
priority mutex test tasks. */
static TaskHandle_t xHighPriorityMutexTask;
static TaskHandle_t xMediumPriorityMutexTask;
static TaskHandle_t xLowPriorityMutexTask;

/* If INCLUDE_xTaskAbortDelay is 1 additional tests are performed, requiring an
additional task. */
#if( INCLUDE_xTaskAbortDelay == 1 )
	static TaskHandle_t xSecondMediumPriorityMutexTask;
#endif

/* Lets the high priority semaphore task know that its wait for the semaphore
was aborted, in which case not being able to obtain the semaphore is not to be
considered an error. */
static volatile BaseType_t xBlockWasAborted = pdFALSE;

/*-----------------------------------------------------------*/

void vStartGenericQueueTasks( UBaseType_t uxPriority )
{
SemaphoreHandle_t xMutex;

	/* Create the mutex used by the prvMutexTest task. */
	xMutex = xSemaphoreCreateMutex();
	if( xMutex != NULL )
	{
		/* vQueueAddToRegistry() adds the mutex to the registry, if one is
		in use.  The registry is provided as a means for kernel aware
		debuggers to locate mutexes and has no purpose if a kernel aware
		debugger is not being used.  The call to vQueueAddToRegistry() will be
		removed by the pre-processor if configQUEUE_REGISTRY_SIZE is not
		defined or is defined to be less than 1. */
		vQueueAddToRegistry( ( QueueHandle_t ) xMutex, "Gen_Queue_Mutex" );

		/* Create the mutex demo tasks and pass it the mutex just created.  We
		are passing the mutex handle by value so it does not matter that it is
		declared on the stack here. */
		rtos_create_thread( &xLowPriorityMutexTask, genqMUTEX_LOW_PRIORITY , "MuLow", prvLowPriorityMutexTask, genqMUTEX_TEST_TASK_STACK_SIZE, ( void * ) xMutex);
		rtos_create_thread( &xMediumPriorityMutexTask, genqMUTEX_MEDIUM_PRIORITY, "MuMed", prvMediumPriorityMutexTask, configMINIMAL_STACK_SIZE, NULL  );
		rtos_create_thread( &xHighPriorityMutexTask, genqMUTEX_HIGH_PRIORITY, "MuHigh", prvHighPriorityMutexTask, genqMUTEX_TEST_TASK_STACK_SIZE, ( void * ) xMutex );
	}
}
/*-----------------------------------------------------------*/

static void prvSendFrontAndBackTest( void *pvParameters )
{
uint32_t ulData, ulData2, ulLoopCounterSnapshot;
QueueHandle_t xQueue;

	xQueue = ( QueueHandle_t ) pvParameters;
	for( ;; )
	{
		/* The queue is empty, so sending an item to the back of the queue
		should have the same effect as sending it to the front of the queue.

		First send to the front and check everything is as expected. */
		ulLoopCounterSnapshot = ulLoopCounter;
		xQueueSendToFront( xQueue, ( void * ) &ulLoopCounterSnapshot, intsemNO_BLOCK );

		if( uxQueueMessagesWaiting( xQueue ) != 1 )
		{
			X_ERROR_DETECTED();
		}

		if( xQueueReceive( xQueue, ( void * ) &ulData, intsemNO_BLOCK ) != pdPASS )
		{
			X_ERROR_DETECTED();
		}

		/* The data we sent to the queue should equal the data we just received
		from the queue. */
		if( ulLoopCounter != ulData )
		{
			X_ERROR_DETECTED();
		}

		/* Then do the same, sending the data to the back, checking everything
		is as expected. */
		if( uxQueueMessagesWaiting( xQueue ) != 0 )
		{
			X_ERROR_DETECTED();
		}

		ulLoopCounterSnapshot = ulLoopCounter;
		xQueueSendToBack( xQueue, ( void * ) &ulLoopCounterSnapshot, intsemNO_BLOCK );

		if( uxQueueMessagesWaiting( xQueue ) != 1 )
		{
			X_ERROR_DETECTED();
		}

		if( xQueueReceive( xQueue, ( void * ) &ulData, intsemNO_BLOCK ) != pdPASS )
		{
			X_ERROR_DETECTED();
		}

		if( uxQueueMessagesWaiting( xQueue ) != 0 )
		{
			X_ERROR_DETECTED();
		}

		/* The data sent to the queue should equal the data just received from
		the queue. */
		if( ulLoopCounter != ulData )
		{
			X_ERROR_DETECTED();
		}

		#if configUSE_PREEMPTION == 0
			taskYIELD();
		#endif



		/* Place 2, 3, 4 into the queue, adding items to the back of the queue. */
		for( ulData = 2; ulData < 5; ulData++ )
		{
			xQueueSendToBack( xQueue, ( void * ) &ulData, intsemNO_BLOCK );
		}

		/* Now the order in the queue should be 2, 3, 4, with 2 being the first
		thing to be read out.  Now add 1 then 0 to the front of the queue. */
		if( uxQueueMessagesWaiting( xQueue ) != 3 )
		{
			X_ERROR_DETECTED();
		}
		ulData = 1;
		xQueueSendToFront( xQueue, ( void * ) &ulData, intsemNO_BLOCK );
		ulData = 0;
		xQueueSendToFront( xQueue, ( void * ) &ulData, intsemNO_BLOCK );

		/* Now the queue should be full, and when we read the data out we
		should receive 0, 1, 2, 3, 4. */
		if( uxQueueMessagesWaiting( xQueue ) != 5 )
		{
			X_ERROR_DETECTED();
		}

		if( xQueueSendToFront( xQueue, ( void * ) &ulData, intsemNO_BLOCK ) != errQUEUE_FULL )
		{
			X_ERROR_DETECTED();
		}

		if( xQueueSendToBack( xQueue, ( void * ) &ulData, intsemNO_BLOCK ) != errQUEUE_FULL )
		{
			X_ERROR_DETECTED();
		}

		#if configUSE_PREEMPTION == 0
			taskYIELD();
		#endif

		/* Check the data we read out is in the expected order. */
		for( ulData = 0; ulData < genqQUEUE_LENGTH; ulData++ )
		{
			/* Try peeking the data first. */
			if( xQueuePeek( xQueue, &ulData2, intsemNO_BLOCK ) != pdPASS )
			{
				X_ERROR_DETECTED();
			}

			if( ulData != ulData2 )
			{
				X_ERROR_DETECTED();
			}


			/* Now try receiving the data for real.  The value should be the
			same.  Clobber the value first so we know we really received it. */
			ulData2 = ~ulData2;
			if( xQueueReceive( xQueue, &ulData2, intsemNO_BLOCK ) != pdPASS )
			{
				X_ERROR_DETECTED();
			}

			if( ulData != ulData2 )
			{
				X_ERROR_DETECTED();
			}
		}

		/* The queue should now be empty again. */
		if( uxQueueMessagesWaiting( xQueue ) != 0 )
		{
			X_ERROR_DETECTED();
		}

		#if configUSE_PREEMPTION == 0
			taskYIELD();
		#endif


		/* Our queue is empty once more, add 10, 11 to the back. */
		ulData = 10;
		if( xQueueSend( xQueue, &ulData, intsemNO_BLOCK ) != pdPASS )
		{
			X_ERROR_DETECTED();
		}
		ulData = 11;
		if( xQueueSend( xQueue, &ulData, intsemNO_BLOCK ) != pdPASS )
		{
			X_ERROR_DETECTED();
		}

		if( uxQueueMessagesWaiting( xQueue ) != 2 )
		{
			X_ERROR_DETECTED();
		}

		/* Now we should have 10, 11 in the queue.  Add 7, 8, 9 to the
		front. */
		for( ulData = 9; ulData >= 7; ulData-- )
		{
			if( xQueueSendToFront( xQueue, ( void * ) &ulData, intsemNO_BLOCK ) != pdPASS )
			{
				X_ERROR_DETECTED();
			}
		}

		/* Now check that the queue is full, and that receiving data provides
		the expected sequence of 7, 8, 9, 10, 11. */
		if( uxQueueMessagesWaiting( xQueue ) != 5 )
		{
			X_ERROR_DETECTED();
		}

		if( xQueueSendToFront( xQueue, ( void * ) &ulData, intsemNO_BLOCK ) != errQUEUE_FULL )
		{
			X_ERROR_DETECTED();
		}

		if( xQueueSendToBack( xQueue, ( void * ) &ulData, intsemNO_BLOCK ) != errQUEUE_FULL )
		{
			X_ERROR_DETECTED();
		}

		#if configUSE_PREEMPTION == 0
			taskYIELD();
		#endif

		/* Check the data we read out is in the expected order. */
		for( ulData = 7; ulData < ( 7 + genqQUEUE_LENGTH ); ulData++ )
		{
			if( xQueueReceive( xQueue, &ulData2, intsemNO_BLOCK ) != pdPASS )
			{
				X_ERROR_DETECTED();
			}

			if( ulData != ulData2 )
			{
				X_ERROR_DETECTED();
			}
		}

		if( uxQueueMessagesWaiting( xQueue ) != 0 )
		{
			X_ERROR_DETECTED();
		}

		/* Increment the loop counter to indicate these tasks are still
		executing. */
		ulLoopCounter++;
	}
}
/*-----------------------------------------------------------*/

#if( INCLUDE_xTaskAbortDelay == 1 )
static void prvHighPriorityTimeout( SemaphoreHandle_t xMutex )
{
static UBaseType_t uxLoopCount = 0;

	/* The tests in this function are very similar, the slight variations
	are for code coverage purposes. */

	/* Take the mutex.  It should be available now.  Check before and after
	taking that the holder is reported correctly. */
	if( xSemaphoreGetMutexHolder( xMutex ) != NULL )
	{
		X_ERROR_DETECTED();
	}
	if( xSemaphoreTake( xMutex, intsemNO_BLOCK ) != pdPASS )
	{
		X_ERROR_DETECTED();
	}
	if( xSemaphoreGetMutexHolder( xMutex ) != xTaskGetCurrentTaskHandle() )
	{
		X_ERROR_DETECTED();
	}

	/* This task's priority should be as per that assigned when the task was
	created. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_LOW_PRIORITY )
	{
		X_ERROR_DETECTED();
	}

	/* Now unsuspend the high priority task.  This will attempt to take the
	mutex, and block when it finds it cannot obtain it. */
	vTaskResume( xHighPriorityMutexTask );

	/* This task should now have inherited the priority of the high priority
	task as by now the high priority task will have attempted to obtain the
	mutex. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_HIGH_PRIORITY )
	{
		X_ERROR_DETECTED();
	}

	/* Unblock a second medium priority task.  It too will attempt to take
	the mutex and enter the Blocked state - it won't run yet though as this
	task has inherited a priority above it. */
	vTaskResume( xSecondMediumPriorityMutexTask );

	/* This task should still have the priority of the high priority task as
	that had already been inherited as is the highest priority of the three
	tasks using the mutex. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_HIGH_PRIORITY )
	{
		X_ERROR_DETECTED();
	}

	/* On some loops, block for a short while to provide additional
	code coverage.  Blocking here will allow the medium priority task to
	execute and so also block on the mutex so when the high priority task
	causes this task to disinherit the high priority it is inherited down to
	the priority of the medium priority task.  When there is no delay the
	medium priority task will not run until after the disinheritance, so
	this task will disinherit back to its base priority, then only up to the
	medium priority after the medium priority has executed. */
	vTaskDelay( uxLoopCount & ( UBaseType_t ) 0x07 );

	/* Now force the high priority task to unblock.  It will fail to obtain
	the mutex and go back to the suspended state - allowing this task to
	execute again.  xBlockWasAborted is set to pdTRUE so the higher priority
	task knows that its failure to obtain the semaphore is not an error. */
	xBlockWasAborted = pdTRUE;
	if( xTaskAbortDelay( xHighPriorityMutexTask ) != pdPASS )
	{
		X_ERROR_DETECTED();
	}

	/* This task has inherited the priority of xHighPriorityMutexTask so
	could still be running even though xHighPriorityMutexTask is no longer
	blocked.  Delay for a short while to ensure xHighPriorityMutexTask gets
	a chance to run - indicated by this task changing priority.  It should
	disinherit the high priority task, but then inherit the priority of the
	medium priority task that is waiting for the same mutex. */
	while( uxTaskPriorityGet( NULL ) != genqMUTEX_MEDIUM_PRIORITY )
	{
		/* If this task gets stuck here then the check variables will stop
		incrementing and the check task will detect the error. */
		vTaskDelay( genqSHORT_BLOCK );
	}

	/* Now force the medium priority task to unblock.  xBlockWasAborted is
	set to pdTRUE so the medium priority task knows that its failure to
	obtain the semaphore is not an error. */
	xBlockWasAborted = pdTRUE;
	if( xTaskAbortDelay( xSecondMediumPriorityMutexTask ) != pdPASS )
	{
		X_ERROR_DETECTED();
	}

	/* This time no other tasks are waiting for the mutex, so this task
	should return to its base priority.  This might not happen straight
	away as it is running at the same priority as the task it just
	unblocked. */
	while( uxTaskPriorityGet( NULL ) != genqMUTEX_LOW_PRIORITY )
	{
		/* If this task gets stuck here then the check variables will stop
		incrementing and the check task will detect the error. */
		vTaskDelay( genqSHORT_BLOCK );
	}

	/* Give the semaphore back ready for the next test.  Check the mutex
	holder before and after using the "FromISR" version for code coverage. */
	if( xSemaphoreGetMutexHolderFromISR( xMutex ) != xTaskGetCurrentTaskHandle() )
	{
		X_ERROR_DETECTED();
	}
	xSemaphoreGive( xMutex );
	if( xSemaphoreGetMutexHolderFromISR( xMutex ) != NULL )
	{
		X_ERROR_DETECTED();
	}

	configASSERT( xErrorDetected == pdFALSE );



	/* Now do the same again, but this time unsuspend the tasks in the
	opposite order.  This takes a different path though the code because
	when the high priority task has its block aborted there is already
	another task in the list of tasks waiting for the mutex, and the
	low priority task drops down to that priority, rather than dropping
	down to its base priority before inheriting the priority of the medium
	priority task. */
	if( xSemaphoreTake( xMutex, intsemNO_BLOCK ) != pdPASS )
	{
		X_ERROR_DETECTED();
	}

	if( uxTaskPriorityGet( NULL ) != genqMUTEX_LOW_PRIORITY )
	{
		X_ERROR_DETECTED();
	}

	/* This time unsuspend the medium priority task first.  This will
	attempt to take the mutex, and block when it finds it cannot obtain it. */
	vTaskResume( xSecondMediumPriorityMutexTask );

	/* This time this task should now have inherited the priority of the
	medium task. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_MEDIUM_PRIORITY )
	{
		X_ERROR_DETECTED();
	}

	/* This time the high priority task in unsuspended second. */
	vTaskResume( xHighPriorityMutexTask );

	/* The high priority task should already have run, causing this task to
	inherit a priority for the second time. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_HIGH_PRIORITY )
	{
		X_ERROR_DETECTED();
	}

	/* This time, when the high priority task has its delay aborted and it
	fails to obtain the mutex this task will immediately have its priority
	lowered down to that of the highest priority task waiting on the mutex,
	which is the medium priority task. */
	xBlockWasAborted = pdTRUE;
	if( xTaskAbortDelay( xHighPriorityMutexTask ) != pdPASS )
	{
		X_ERROR_DETECTED();
	}

	while( uxTaskPriorityGet( NULL ) != genqMUTEX_MEDIUM_PRIORITY )
	{
		/* If this task gets stuck here then the check variables will stop
		incrementing and the check task will detect the error. */
		vTaskDelay( genqSHORT_BLOCK );
	}

	/* And finally, when the medium priority task also have its delay
	aborted there are no other tasks waiting for the mutex so this task
	returns to its base priority. */
	xBlockWasAborted = pdTRUE;
	if( xTaskAbortDelay( xSecondMediumPriorityMutexTask ) != pdPASS )
	{
		X_ERROR_DETECTED();
	}

	while( uxTaskPriorityGet( NULL ) != genqMUTEX_LOW_PRIORITY )
	{
		/* If this task gets stuck here then the check variables will stop
		incrementing and the check task will detect the error. */
		vTaskDelay( genqSHORT_BLOCK );
	}

	/* Give the semaphore back ready for the next test. */
	xSemaphoreGive( xMutex );

	configASSERT( xErrorDetected == pdFALSE );

	/* uxLoopCount is used to add a variable delay, and in-so-doing provide
	additional code coverage. */
	uxLoopCount++;
}

#endif /* INCLUDE_xTaskAbortDelay == 1 */
/*-----------------------------------------------------------*/

void prvTakeTwoMutexesReturnInDifferentOrder( SemaphoreHandle_t xMutex, SemaphoreHandle_t xLocalMutex )
{
	/* Take the mutex.  It should be available now. */
	BK_LOGD(NULL, "[core%d]lpmt-xSemaphoreTakeM\r\n", (*((volatile uint32_t *)(0x20000000))));
	if( xSemaphoreTake( xMutex, intsemNO_BLOCK ) != pdPASS )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected0\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Set the guarded variable to a known start value. */
	ulGuardedVariable = 0;

	/* This task's priority should be as per that assigned when the task was
	created. */
	BK_LOGD(NULL, "[core%d]lpmt1-prio:%d\r\n", (*((volatile uint32_t *)(0x20000000))), uxTaskPriorityGet(NULL));
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_LOW_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected1\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Now unsuspend the high priority task.  This will attempt to take the
	mutex, and block when it finds it cannot obtain it. */
	BK_LOGD(NULL, "[core%d]lpmt-resume-high\r\n", (*((volatile uint32_t *)(0x20000000))));
	vTaskResume( xHighPriorityMutexTask );

	#if configUSE_PREEMPTION == 0
		taskYIELD();
	#endif

	/* Ensure the task is reporting its priority as blocked and not
	suspended (as it would have done in versions up to V7.5.3). */
	#if( INCLUDE_eTaskGetState == 1 )
	{
		configASSERT( eTaskGetState( xHighPriorityMutexTask ) == eBlocked );
	}
	#endif /* INCLUDE_eTaskGetState */

	/* This task should now have inherited the priority of the high priority
	task as by now the high priority task will have attempted to obtain the
	mutex. */
	BK_LOGD(NULL, "[core%d]lpmt2-prio:%d\r\n", (*((volatile uint32_t *)(0x20000000))), uxTaskPriorityGet(NULL));
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_HIGH_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected2\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Attempt to set the priority of this task to the test priority -
	between the idle priority and the medium/high test priorities, but the
	actual priority should remain at the high priority. */
	vTaskPrioritySet( NULL, genqMUTEX_TEST_PRIORITY );
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_HIGH_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected3\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Now unsuspend the medium priority task.  This should not run as the
	inherited priority of this task is above that of the medium priority
	task. */
	BK_LOGD(NULL, "[p:%d][core%d]lpmt-resume-medium\r\n", uxTaskPriorityGet( NULL ), (*((volatile uint32_t *)(0x20000000))));
	vTaskResume( xMediumPriorityMutexTask );

	/* If the medium priority task did run then it will have incremented the
	guarded variable. */
	if( ulGuardedVariable != 0 )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected4\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Take the local mutex too, so two mutexes are now held. */
	BK_LOGD(NULL, "[p:%d][core%d]lpmt-xSemaphoreTakeLM\r\n", uxTaskPriorityGet( NULL ), (*((volatile uint32_t *)(0x20000000))));
	if( xSemaphoreTake( xLocalMutex, intsemNO_BLOCK ) != pdPASS )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected5\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* When the semaphore is given back the priority of this task should not
	yet be disinherited because the local mutex is still held.  This is a
	simplification to allow FreeRTOS to be integrated with middleware that
	attempts to hold multiple mutexes without bloating the code with complex
	algorithms.  It is possible that the high priority mutex task will
	execute as it shares a priority with this task. */
	BK_LOGD(NULL, "[core%d]lpmt-xSemaphoreGiveM\r\n", (*((volatile uint32_t *)(0x20000000))));
	if( xSemaphoreGive( xMutex ) != pdPASS )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected6\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	#if configUSE_PREEMPTION == 0
		taskYIELD();
	#endif

	/* The guarded variable is only incremented by the medium priority task,
	which still should not have executed as this task should remain at the
	higher priority, ensure this is the case. */
	if( ulGuardedVariable != 0 )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected7\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	if( uxTaskPriorityGet( NULL ) != genqMUTEX_HIGH_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected8\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Now also give back the local mutex, taking the held count back to 0.
	This time the priority of this task should be disinherited back to the
	priority to which it was set while the mutex was held.  This means
	the medium priority task should execute and increment the guarded
	variable.   When this task next	runs both the high and medium priority
	tasks will have been suspended again. */
	BK_LOGD(NULL, "[core%d]lpmt-xSemaphoreGiveLM\r\n", (*((volatile uint32_t *)(0x20000000))));
	if( xSemaphoreGive( xLocalMutex ) != pdPASS )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected9\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	#if configUSE_PREEMPTION == 0
		taskYIELD();
	#endif

	/* Check the guarded variable did indeed increment... */
	if( ulGuardedVariable != 1 )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected10\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* ... and that the priority of this task has been disinherited to
	genqMUTEX_TEST_PRIORITY. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_TEST_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected11\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Set the priority of this task back to its original value, ready for
	the next loop around this test. */
	vTaskPrioritySet( NULL, genqMUTEX_LOW_PRIORITY );
}

/*-----------------------------------------------------------*/
void prvTakeTwoMutexesReturnInSameOrder( SemaphoreHandle_t xMutex, SemaphoreHandle_t xLocalMutex )
{
	/* Take the mutex.  It should be available now. */
	BK_LOGD(NULL, "[core%d]lp_task-xSemaphoreTakeM\r\n", (*((volatile uint32_t *)(0x20000000))));
	if( xSemaphoreTake( xMutex, intsemNO_BLOCK ) != pdPASS )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected12\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Set the guarded variable to a known start value. */
	ulGuardedVariable = 0;

	/* This task's priority should be as per that assigned when the task was
	created. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_LOW_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected13\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Now unsuspend the high priority task.  This will attempt to take the
	mutex, and block when it finds it cannot obtain it. */
	BK_LOGD(NULL, "[core%d]lp_task-vTaskResumeHT\r\n", (*((volatile uint32_t *)(0x20000000))));
	vTaskResume( xHighPriorityMutexTask );

	#if configUSE_PREEMPTION == 0
		taskYIELD();
	#endif

	/* Ensure the task is reporting its priority as blocked and not
	suspended (as it would have done in versions up to V7.5.3). */
	#if( INCLUDE_eTaskGetState == 1 )
	{
		configASSERT( eTaskGetState( xHighPriorityMutexTask ) == eBlocked );
	}
	#endif /* INCLUDE_eTaskGetState */

	/* This task should now have inherited the priority of the high priority
	task as by now the high priority task will have attempted to obtain the
	mutex. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_HIGH_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected14\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Now unsuspend the medium priority task.  This should not run as the
	inherited priority of this task is above that of the medium priority
	task. */
	BK_LOGD(NULL, "[core%d]lp_task-xTaskResumeMT\r\n", (*((volatile uint32_t *)(0x20000000))));
	vTaskResume( xMediumPriorityMutexTask );

	/* If the medium priority task did run then it will have incremented the
	guarded variable. */
	if( ulGuardedVariable != 0 )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected15\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Take the local mutex too, so two mutexes are now held. */
	BK_LOGD(NULL, "[core%d]lp_task-xSemaphoreTakeLM\r\n", (*((volatile uint32_t *)(0x20000000))));
	if( xSemaphoreTake( xLocalMutex, intsemNO_BLOCK ) != pdPASS )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected16\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* When the local semaphore is given back the priority of this task should
	not yet be disinherited because the shared mutex is still held.  This is a
	simplification to allow FreeRTOS to be integrated with middleware that
	attempts to hold multiple mutexes without bloating the code with complex
	algorithms.  It is possible that the high priority mutex task will
	execute as it shares a priority with this task. */
	BK_LOGD(NULL, "[core%d]lp_task-xSemaphoreGiveLM\r\n", (*((volatile uint32_t *)(0x20000000))));
	if( xSemaphoreGive( xLocalMutex ) != pdPASS )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected17\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	#if configUSE_PREEMPTION == 0
		taskYIELD();
	#endif

	/* The guarded variable is only incremented by the medium priority task,
	which still should not have executed as this task should remain at the
	higher priority, ensure this is the case. */
	if( ulGuardedVariable != 0 )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected18\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	if( uxTaskPriorityGet( NULL ) != genqMUTEX_HIGH_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected19\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* Now also give back the shared mutex, taking the held count back to 0.
	This time the priority of this task should be disinherited back to the
	priority at which it was created.  This means the medium priority task
	should execute and increment the guarded variable.  When this task next runs
	both the high and medium priority tasks will have been suspended again. */
	BK_LOGD(NULL, "[core%d]lp_task-xSemaphoreGiveM\r\n", (*((volatile uint32_t *)(0x20000000))));
	if( xSemaphoreGive( xMutex ) != pdPASS )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected20\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	#if configUSE_PREEMPTION == 0
		taskYIELD();
	#endif

	/* Check the guarded variable did indeed increment... */
	if( ulGuardedVariable != 1 )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected21\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}

	/* ... and that the priority of this task has been disinherited to
	genqMUTEX_LOW_PRIORITY. */
	if( uxTaskPriorityGet( NULL ) != genqMUTEX_LOW_PRIORITY )
	{
		BK_LOGD(NULL, "[core%d]xErrorDetected22\r\n", (*((volatile uint32_t *)(0x20000000))));
		X_ERROR_DETECTED();
	}
}
/*-----------------------------------------------------------*/

static void prvLowPriorityMutexTask( void *pvParameters )
{
SemaphoreHandle_t xMutex = ( SemaphoreHandle_t ) pvParameters, xLocalMutex;

	BK_LOGD(NULL, "[core%d]Mutex with priority inheritance test started.\r\n", (*((volatile uint32_t *)(0x20000000))));

	/* The local mutex is used to check the 'mutexs held' count. */
	xLocalMutex = xSemaphoreCreateMutex();
	configASSERT( xLocalMutex );

	for( ;; )
	{
		/* The first tests exercise the priority inheritance when two mutexes
		are taken then returned in a different order to which they were
		taken. */

		BK_LOGD(NULL, "[core%d]lpmt-prvTakeTwoMutexesReturnInDifferentOrder\r\n", (*((volatile uint32_t *)(0x20000000))));
		prvTakeTwoMutexesReturnInDifferentOrder( xMutex, xLocalMutex );

		/* Just to show this task is still running. */
		ulLoopCounter2 ++;

		#if configUSE_PREEMPTION == 0
			taskYIELD();
		#endif

		/* The second tests exercise the priority inheritance when two mutexes
		are taken then returned in the same order in which they were taken. */
		BK_LOGD(NULL, "[core%d]lpmt-prvTakeTwoMutexesReturnInSameOrder\r\n", (*((volatile uint32_t *)(0x20000000))));
		prvTakeTwoMutexesReturnInSameOrder( xMutex, xLocalMutex );

		/* Just to show this task is still running. */
		ulLoopCounter2++;

		#if configUSE_PREEMPTION == 0
			taskYIELD();
		#endif
	}
}
/*-----------------------------------------------------------*/

static void prvMediumPriorityMutexTask( void *pvParameters )
{
	( void ) pvParameters;
	BK_LOGD(NULL, "[core%d]prvMediumPriorityMutexTask.\r\n", (*((volatile uint32_t *)(0x20000000))));

	for( ;; )
	{
		/* The medium priority task starts by suspending itself.  The low
		priority task will unsuspend this task when required. */
		BK_LOGD(NULL, "[core%d]mpmt-vTaskSuspend\r\n", (*((volatile uint32_t *)(0x20000000))));
		vTaskSuspend( NULL );

		/* When this task unsuspends all it does is increment the guarded
		variable, this is so the low priority task knows that it has
		executed. */
		ulGuardedVariable++;
	}
}
/*-----------------------------------------------------------*/

static void prvHighPriorityMutexTask( void *pvParameters )
{
SemaphoreHandle_t xMutex = ( SemaphoreHandle_t ) pvParameters;

	BK_LOGD(NULL, "[core%d]prvHighPriorityMutexTask.\r\n", (*((volatile uint32_t *)(0x20000000))));
	for( ;; )
	{
		/* The high priority task starts by suspending itself.  The low
		priority task will unsuspend this task when required. */
		BK_LOGD(NULL, "[core%d]hpmt-vTaskSuspend\r\n", (*((volatile uint32_t *)(0x20000000))));
		vTaskSuspend( NULL );

		/* When this task unsuspends all it does is attempt to obtain the
		mutex.  It should find the mutex is not available so a block time is
		specified. */
		BK_LOGD(NULL, "[core%d]hpmt-xSemaphoreTake\r\n", (*((volatile uint32_t *)(0x20000000))));
		if( xSemaphoreTake( xMutex, portMAX_DELAY ) != pdPASS )
		{
			/* This task would expect to obtain the mutex unless its wait for
			the mutex was aborted. */
			if( xBlockWasAborted == pdFALSE )
			{
				BK_LOGD(NULL, "[core%d]hpmt-TakeError\r\n", (*((volatile uint32_t *)(0x20000000))));
				X_ERROR_DETECTED();
			}
			else
			{
				BK_LOGD(NULL, "[core%d]hpmt-xBlockWasAborted\r\n", (*((volatile uint32_t *)(0x20000000))));
				xBlockWasAborted = pdFALSE;
			}
		}
		else
		{
			/* When the mutex is eventually obtained it is just given back before
			returning to suspend ready for the next cycle. */
			BK_LOGD(NULL, "[core%d]hpmt-xSemaphoreGive\r\n", (*((volatile uint32_t *)(0x20000000))));
			if( xSemaphoreGive( xMutex ) != pdPASS )
			{
				BK_LOGD(NULL, "[core%d]hpmt-xSemaphoreGiveError\r\n", (*((volatile uint32_t *)(0x20000000))));
				X_ERROR_DETECTED();
			}
		}
	}
}
/*-----------------------------------------------------------*/


/* This is called to check that all the created tasks are still running. */
BaseType_t xAreGenericQueueTasksStillRunning( void )
{
static uint32_t ulLastLoopCounter = 0, ulLastLoopCounter2 = 0;

	/* If the demo task is still running then we expect the loop counters to
	have incremented since this function was last called. */
	if( ulLastLoopCounter == ulLoopCounter )
	{
		X_ERROR_DETECTED();
	}

	if( ulLastLoopCounter2 == ulLoopCounter2 )
	{
		X_ERROR_DETECTED();
	}

	ulLastLoopCounter = ulLoopCounter;
	ulLastLoopCounter2 = ulLoopCounter2;

	/* Errors detected in the task itself will have latched xErrorDetected
	to true. */

	return ( BaseType_t ) !xErrorDetected;
}

void prio_inversion_test( void )
{
	vStartGenericQueueTasks(tskIDLE_PRIORITY);
}
// eof

