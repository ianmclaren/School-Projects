/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include "stm32f4_discovery.h"
/* Kernel includes. */
#include "stm32f4xx.h"
#include "../FreeRTOS_Source/include/FreeRTOS.h"
#include "../FreeRTOS_Source/include/queue.h"
#include "../FreeRTOS_Source/include/semphr.h"
#include "../FreeRTOS_Source/include/task.h"
#include "../FreeRTOS_Source/include/timers.h"



/*-----------------------------------------------------------*/
#define mainQUEUE_LENGTH 100

#define green  	1
#define red  	2
#define blue  	3


/*-----------------------------------------------------------*/
/* Defining values used in program execution */
/*-----------------------------------------------------------*/
#define green_led	LED4
#define red_led		LED5
#define blue_led	LED6

#define TASK1_ID  1
#define TASK2_ID  2
#define TASK3_ID  3

//TASK PRIORITIES TODO: Find and define the correct values
#define TASK_CREATION_PRIORITY        1
#define DD_TASK_GENERATOR_PRIORITY    2
#define TASK_EXECUTION_PRIORITY       4
#define MONITOR_PRIORITY              0
#define DD_TASK_SCHEDULER_PRIORITY    3

/*-----------------------------------------------------------*/
/* Defining Test Benches */
/* Only 1 should be uncommented at a time */
/*-----------------------------------------------------------*/

// Test Bench 1
#define TASK1_EXECUTION_TIME   95
#define TASK1_PERIOD           500
#define TASK2_EXECUTION_TIME   150
#define TASK2_PERIOD           500
#define TASK3_EXECUTION_TIME   250
#define TASK3_PERIOD           500

//// Test Bench 2
//#define TASK1_EXECUTION_TIME  95
//#define TASK1_PERIOD         250
//#define TASK2_EXECUTION_TIME 150
//#define TASK2_PERIOD         500
//#define TASK3_EXECUTION_TIME 250
//#define TASK3_PERIOD         750
//
//// Test Bench 3
//#define TASK1_EXECUTION_TIME 100
//#define TASK1_PERIOD         500
//#define TASK2_EXECUTION_TIME 200
//#define TASK2_PERIOD         500
//#define TASK3_EXECUTION_TIME 200
//#define TASK3_PERIOD         500


/*-----------------------------------------------------------*/
/* Defining Structures Recommended in Slides and Lab Manual */
/*-----------------------------------------------------------*/

typedef enum {PERIODIC, APERIODIC} task_type;

typedef struct dd_task {
	TaskHandle_t t_handle;
	task_type type;
	uint32_t task_id;
	uint32_t release_time;
	uint32_t absolute_deadline;
	uint32_t completion_time;
} dd_task;

typedef struct dd_task_node_t {
	dd_task task;
	struct dd_task_node_t *next_task;
} dd_task_node_t;

// This is the enumerator for the different requests that can be sent to the DDS
typedef enum {TASK_CREATED, TASK_COMPLETED, GET_ACTIVE_LIST, GET_COMPLETED_LIST, GET_OVERDUE_LIST} request_type;

/*
 * TODO: Implement this function for any hardware specific clock configuration
 * that was not already performed before main() was called.
 */
static void prvSetupHardware( void );

/*
 * The queue send and receive tasks as described in the comments at the top of
 * this file.
 */

/*-----------------------------------------------------------*/
/* Defining Functions */
/*-----------------------------------------------------------*/

static void user_defined_task_1( void *pvParameters );
static void user_defined_task_2( void *pvParameters );
static void user_defined_task_3( void *pvParameters );
static void complete_dd_task( uint32_t task_id );
static void release_dd_task(TaskHandle_t t_handle, task_type type, uint32_t task_id, uint32_t absolute_deadline );

static void DD_Task_Generator_1( void *pvParameters );
static void DD_Task_Generator_2( void *pvParameters );
static void DD_Task_Generator_3( void *pvParameters );

static dd_task_node_t *insert_dd_task_to_other_list( dd_task task, dd_task_node_t *head);
static dd_task_node_t *insert_dd_task_to_active_list( dd_task task, dd_task_node_t *head);
static dd_task_node_t *remove_earliest_deadline_dd_task( dd_task_node_t *head );
static void DD_Scheduler( void *pvParameters );

//The three declarations below are recommended in the lab manual, but fail the build
//static **dd_task_list get_active_dd_task_list(void);
//static **dd_task_list get_complete_dd_task_list(void);
//static **dd_task_list get_overdue_dd_task_list(void);

/* HANDLES */

QueueHandle_t request_queue = NULL;
QueueHandle_t dd_task_queue = NULL;
QueueHandle_t completed_tasks_queue = NULL;
//QueueHandle_t currently_executing_task_queue = NULL;

TaskHandle_t dd_task_1_generator_handle = NULL;
TaskHandle_t dd_task_2_generator_handle = NULL;
TaskHandle_t dd_task_3_generator_handle = NULL;

TaskHandle_t dd_scheduler_handle = NULL;

SemaphoreHandle_t xTaskExecutionMutex = NULL;


/*-----------------------------------------------------------*/

int main(void)
{

	/* Initialize LEDs needed for Project 2 */
	STM_EVAL_LEDInit(green_led);
	STM_EVAL_LEDInit(red_led);
	STM_EVAL_LEDInit(blue_led);

	/* Configure the system ready to run the demo.  The clock configuration
	can be done here if it was not done before main() was called. */
	prvSetupHardware();

	/* Create the queue used by the DDS to receive requests .*/
	request_queue = xQueueCreate( 	mainQUEUE_LENGTH,		/* The number of items the queue can hold. */
							sizeof( request_type ) );	/* The size of each item the queue holds. */

	/* Create the queue used by the generator to place created dd_tasks */
	dd_task_queue = xQueueCreate( mainQUEUE_LENGTH, sizeof(dd_task) );

	/* Create the queue that receives completed task ids */
	completed_tasks_queue = xQueueCreate( mainQUEUE_LENGTH, sizeof(uint32_t) );

	/* Create the queue used by the scheduler to place the executing dd_task */
//	currently_executing_task_queue = xQueueCreate( mainQUEUE_LENGTH, sizeof(dd_task) );

	/* Add to the registry, for the benefit of kernel aware debugging. */
	vQueueAddToRegistry( request_queue, "RequestQueue" );
	vQueueAddToRegistry( dd_task_queue, "DDTaskQueue" );
//	vQueueAddToRegistry( completed_tasks_queue, "CompletedQueue");
//	vQueueAddToRegistry( currently_executing_task_queue, "ExecutingQueue");

	xTaskCreate(DD_Task_Generator_1, "DD Task 1 Generator", configMINIMAL_STACK_SIZE, NULL, DD_TASK_GENERATOR_PRIORITY, &dd_task_1_generator_handle);
	xTaskCreate(DD_Task_Generator_2, "DD Task 2 Generator", configMINIMAL_STACK_SIZE, NULL, DD_TASK_GENERATOR_PRIORITY, &dd_task_2_generator_handle);
	//xTaskCreate(DD_Task_Generator_3, "DD Task 3 Generator", configMINIMAL_STACK_SIZE, NULL, DD_TASK_GENERATOR_PRIORITY, &dd_task_3_generator_handle);

	xTaskCreate(DD_Scheduler, "DD Scheduler", configMINIMAL_STACK_SIZE, NULL, DD_TASK_SCHEDULER_PRIORITY, &dd_scheduler_handle);

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	return 0;
}

/*-----------------------------------------------------------*/

static void complete_dd_task(uint32_t task_id){
	xQueueSend(completed_tasks_queue, &task_id, portMAX_DELAY);
}

/*-----------------------------------------------------------*/

static void release_dd_task(TaskHandle_t t_handle, task_type type, uint32_t task_id, uint32_t absolute_deadline ){
	request_type request = TASK_CREATED;

	dd_task *ddtask;
	ddtask = (dd_task*)pvPortMalloc(sizeof(dd_task));

	if(ddtask == NULL){
		return;
	}

	ddtask->t_handle = t_handle;
	ddtask->type = type;
	ddtask->task_id = task_id;
	ddtask->absolute_deadline = absolute_deadline;
	ddtask->release_time = xTaskGetTickCount();

	//Put dd_task on queue
	xQueueSend(dd_task_queue, ddtask, portMAX_DELAY);
	//Put task_created onto request queue
	xQueueSend(request_queue, &request, portMAX_DELAY);
}

/*-----------------------------------------------------------*/

static void DD_Task_Generator_1( void *pvParameters )
{
	printf("DD_Task_Generator_1 Started\n");
	TaskHandle_t dd_task_1_handle = NULL;
	BaseType_t result;

	while(1)
	{
		/* Creating the DD_Task */
		result = xTaskCreate(user_defined_task_1, "dd_task_1", configMINIMAL_STACK_SIZE, NULL, TASK_CREATION_PRIORITY, &dd_task_1_handle);
		if (result != pdPASS) {
			printf("Failed to create dd_task1"); //Checking to make sure the task was created
		}

		printf("dd_task_generator 1 released task!\n");
		release_dd_task(dd_task_1_handle, PERIODIC, TASK1_ID, xTaskGetTickCount() + pdMS_TO_TICKS(TASK1_PERIOD));

		// Wait for the Tasks's period before generating the task again
		vTaskDelay(pdMS_TO_TICKS(TASK1_PERIOD));
	}
}

/*-----------------------------------------------------------*/

static void DD_Task_Generator_2( void *pvParameters )
{
	printf("DD_Task_Generator_2 Started\n");
	TaskHandle_t dd_task_2_handle = NULL;
	BaseType_t result;

	while(1)
	{
		/* Creating the DD_Task */
		result = xTaskCreate(user_defined_task_2, "dd_task_2", configMINIMAL_STACK_SIZE, NULL, TASK_CREATION_PRIORITY, &dd_task_2_handle);
		if (result != pdPASS) {
			printf("Failed to create dd_task_2"); //Checking to make sure the task was created
		}

		printf("dd_task_generator 2 released task!\n");
		release_dd_task(dd_task_2_handle, PERIODIC, TASK2_ID, xTaskGetTickCount() + pdMS_TO_TICKS(TASK2_PERIOD));

		// Wait for the Tasks's period before generating the task again
		vTaskDelay(pdMS_TO_TICKS(TASK2_PERIOD));
	}
}

/*-----------------------------------------------------------*/

static void DD_Task_Generator_3( void *pvParameters )
{
	printf("DD_Task_Generator_3 Started\n");
	TaskHandle_t dd_task_3_handle = NULL;
	BaseType_t result;

	while(1)
	{
		/* Creating the DD_Task */
		result = xTaskCreate(user_defined_task_3, "dd_task_3", configMINIMAL_STACK_SIZE, NULL, TASK_CREATION_PRIORITY, &dd_task_3_handle);
		if (result != pdPASS) {
			printf("Failed to create dd_task_3"); //Checking to make sure the task was created
		}

		printf("dd_task_generator 3 released task!\n");
		release_dd_task(dd_task_3_handle, PERIODIC, TASK3_ID, xTaskGetTickCount() + pdMS_TO_TICKS(TASK3_PERIOD));

		// Wait for the Tasks's period before generating the task again
		vTaskDelay(pdMS_TO_TICKS(TASK3_PERIOD));
	}
}

/*-----------------------------------------------------------*/

static void user_defined_task_1( void *pvParameters )
{
	printf("User Defined Task 1 Suspended\n");
	//Suspend Task execution (of ceiling task w/ NULL) to allow DDS to handle scheduling
	vTaskSuspend(NULL);

	//When we hit this line, we've resumed from the scheduler
	printf("User Defined Task 1 Started\n");
	TickType_t completion_time = xTaskGetTickCount() + pdMS_TO_TICKS(TASK1_EXECUTION_TIME);

	STM_EVAL_LEDOn(blue_led);
	while (xTaskGetTickCount() < completion_time){} // Wait until the completion time has passed
	STM_EVAL_LEDOff(blue_led);

	complete_dd_task(TASK1_ID);
	vTaskDelete(NULL);
}

/*-----------------------------------------------------------*/

static void user_defined_task_2( void *pvParameters )
{
	printf("User Defined Task 2 Started\n");
	//Suspend Task execution (of ceiling task w/ NULL) to allow DDS to handle scheduling
	vTaskSuspend(NULL);

	//When we hit this line, we've resumed from the scheduler
	printf("User Defined Task 2 Started\n");
	TickType_t completion_time = xTaskGetTickCount() + pdMS_TO_TICKS(TASK2_EXECUTION_TIME);

	STM_EVAL_LEDOn(green_led);
	while (xTaskGetTickCount() < completion_time){} // Wait until the completion time has passed
	STM_EVAL_LEDOff(green_led);

	complete_dd_task(TASK2_ID);
	vTaskDelete(NULL);
}

/*-----------------------------------------------------------*/

static void user_defined_task_3( void *pvParameters )
{
	printf("User Defined Task 3 Started\n");
	//Suspend Task execution (of ceiling task w/ NULL) to allow DDS to handle scheduling
	vTaskSuspend(NULL);

	//When we hit this line, we've resumed from the scheduler
	printf("User Defined Task 3 Started\n");
	TickType_t completion_time = xTaskGetTickCount() + pdMS_TO_TICKS(TASK3_EXECUTION_TIME);

	STM_EVAL_LEDOn(red_led);
	while (xTaskGetTickCount() < completion_time){} // Wait until the completion time has passed
	STM_EVAL_LEDOff(red_led);

	complete_dd_task(TASK3_ID);
	vTaskDelete(NULL);
}

/*-----------------------------------------------------------*/

static dd_task_node_t *insert_dd_task_to_other_list( dd_task task, dd_task_node_t *head ){
	//1. Creating the dd_task_node
	dd_task_node_t *new_node;
	new_node = (dd_task_node_t*)pvPortMalloc(sizeof(dd_task_node_t));

	if(new_node == NULL){
		return NULL;
	}

	new_node->task = task;
	new_node->next_task = NULL;

	// 2. Insert the dd_task_node at the end of the list
	if (head == NULL){
		head = new_node;
		return head;
	}

	dd_task_node_t *temp = head;
	while (temp->next_task != NULL){
		temp = temp->next_task;
	}
	temp->next_task = new_node;
	return head;
}

/*-----------------------------------------------------------*/

static dd_task_node_t *insert_dd_task_to_active_list( dd_task task, dd_task_node_t *head ) {
	//1. Creating the dd_task_node
	dd_task_node_t *new_node;
	new_node = (dd_task_node_t*)pvPortMalloc(sizeof(dd_task_node_t));

	if(new_node == NULL){
		return NULL;
	}

	new_node->task = task;
	new_node->next_task = NULL;

	//2. If the list is empty, return the new node as the list
	if (head == NULL){
		head = new_node;
		return head;
	}

	//3. If the new task has the earliest deadline out of all the tasks in the list
	if (task.absolute_deadline <= head->task.absolute_deadline){
		new_node->next_task = head;
		head = new_node;
		return head;
	}

	//4. Traversing the list to find the correct, sorted postion for the new node based on deadline
	dd_task_node_t *temp = head;
	while (temp->next_task != NULL && temp->next_task->task.absolute_deadline < task.absolute_deadline) {
		temp = temp->next_task;
	}

	//5. Add the new node to the list
	new_node->next_task = temp->next_task;
	temp->next_task = new_node;
	return head;
}

/*-----------------------------------------------------------*/

static dd_task_node_t *remove_earliest_deadline_dd_task(dd_task_node_t *head) {
    // Check if the list is empty
    if (head == NULL) {
        return NULL;
    }

    // Store the first node
    dd_task_node_t *earliest_deadline_dd_task_node = head;

    // Update head to point to the next node
    head = head->next_task;

    // Clear the next pointer of the removed node
    earliest_deadline_dd_task_node->next_task = NULL;

    return earliest_deadline_dd_task_node;
}

/*-----------------------------------------------------------*/

static void DD_Scheduler( void *pvParameters )
{
	printf("DD Scheduler Started\n");

	dd_task_node_t *active_tasks_head = NULL;
	dd_task_node_t *completed_tasks_head = NULL;
	dd_task_node_t *overdue_tasks_head = NULL;

	request_type received_request;
	dd_task received_dd_task;
	dd_task dd_task;
	uint32_t completed_dd_task_id;
	dd_task_node_t* task_to_execute;

	TaskHandle_t handle = NULL;

	while(1){
		if(xQueueReceive(request_queue, &received_request, 0) == pdPASS){ //Only proceed if we received a request
			switch(received_request){
				case TASK_CREATED:
					if(xQueueReceive(dd_task_queue, &received_dd_task, portMAX_DELAY) != pdPASS){
						break;
					}

					active_tasks_head = insert_dd_task_to_active_list(received_dd_task, active_tasks_head);

					task_to_execute = remove_earliest_deadline_dd_task(active_tasks_head);
					if (task_to_execute != NULL) {
						handle = task_to_execute->task.t_handle;
						vTaskPrioritySet(handle, TASK_EXECUTION_PRIORITY);
						//xQueueSend(currently_executing_task_queue, &task_to_execute, portMAX_DELAY);
						vTaskResume(handle);
					    vPortFree(task_to_execute);  // Free memory if no longer needed. I believe this is where we want to free the memory
					}

					break;
				case TASK_COMPLETED:
					if(xQueueReceive(completed_tasks_queue, &completed_dd_task_id, portMAX_DELAY) != pdPASS){
						break;
					}

					//xQueueReceive(currently_executing_task_queue, &received_dd_task, portMAX_DELAY);
					//received_dd_task.completion_time = xTaskGetTickCount();

					//if(received_dd_task.completion_time < received_dd_task.absolute_deadline){ // If completed in time
						//completed_tasks_head = insert_dd_task_to_other_list(received_dd_task, completed_tasks_head);
					//} else { //Otherwise, task is overdue
					//	overdue_tasks_head = insert_dd_task_to_other_list(received_dd_task, overdue_tasks_head);
					//}

					break;
				case GET_ACTIVE_LIST: //TODO: Implement

					break;
				case GET_COMPLETED_LIST: //TODO: Implement
					break;
				case GET_OVERDUE_LIST: //TODO: Implement
					break;
				default:
				//Error
					break;
			}
		}
		vTaskDelay(5);
	}
}


/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* The malloc failed hook is enabled by setting
	configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

	Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software 
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  pxCurrentTCB can be
	inspected in the debugger if the task name passed into this function is
	corrupt. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
	FreeRTOSConfig.h.

	This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amount of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Ensure all priority bits are assigned as preemption priority bits.
	http://www.freertos.org/RTOS-Cortex-M3-M4.html */
	NVIC_SetPriorityGrouping( 0 );

	/* TODO: Setup the clocks, etc. here, if they were not configured before
	main() was called. */
}
