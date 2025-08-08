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
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_adc.h"



/*-----------------------------------------------------------*/
#define mainQUEUE_LENGTH 100

#define green  	0
#define amber  	1
#define red  	2

/* Traffic Light LEDs */
#define green_light GPIO_Pin_0
#define amber_light GPIO_Pin_1
#define red_light 	GPIO_Pin_2

/* Shift Registers (Cars) */
#define Shift_Register_Clock	GPIO_Pin_7
#define Shift_Register_Data		GPIO_Pin_6
#define Shift_Register_Reset	GPIO_Pin_8

// Define the traffic rate levels
typedef enum {
    LIGHT,
    MEDIUM,
    HEAVY,
    VERY_HEAVY
} TrafficRate;

/*
 * TODO: Implement this function for any hardware specific clock configuration
 * that was not already performed before main() was called.
 */
static void prvSetupHardware( void );

/*
 * The queue send and receive tasks as described in the comments at the top of
 * this file.
 */
/********* PROTOTYPES *********/

// Traffic Lights
static void TrafficLight_Manager_Task( void *pvParameters );
static void Init_Traffic_Light();
static uint16_t switchTrafficLightColour(uint16_t current_colour);
static uint16_t getTrafficLightWaitTime(TrafficRate traffic_rate, uint16_t current_colour);

// Traffic Rate
static void Init_Potentiometer();
static void Init_ADC1();
static void Init_PC3();
static uint16_t readPotentiometer();
static void TrafficRate_Adjustment_Task( void *pvParameters );
static TrafficRate getTrafficRate( uint16_t read_adc_value );

// Traffic Flow
static void Traffic_Manager_Task( void *pvParameters );
static void GPIO_Init_Shift_Register(void);
static void GPIO_Reset_Shift_Register(void);
static void GPIO_Shift_Register_bit_On(void);
static void GPIO_Shift_Register_bit_Off(void);
static uint16_t getTrafficGap(TrafficRate rate);

xQueueHandle xQueue_handle = 0;
xQueueHandle xTrafficRateQueue = 0; //Queue used to pass the current traffic rate between tasks
xQueueHandle xTrafficLightColourQueue = 0;

//Timer for the traffic light
//static void vTrafficLightTimerCallback( xTimerHandle timer );
//xTimerHandle xTrafficLightTimer = 0;


/*-----------------------------------------------------------*/

int main(void)
{
	/* Initialize Traffic Light */
	Init_Traffic_Light();

	/* Initialize Potentiometer */
	Init_Potentiometer();

	/* Initialize Shift Register */
	GPIO_Init_Shift_Register();
	GPIO_Reset_Shift_Register();

	/* Configure the system ready to run the demo.  The clock configuration
	can be done here if it was not done before main() was called. */
	prvSetupHardware();

    /* Setup Queues */
	xTrafficRateQueue = xQueueCreate(1, sizeof(TrafficRate));
	xTrafficLightColourQueue = xQueueCreate(1, sizeof(uint16_t));

	/* Setup Tasks */
	xTaskCreate( TrafficRate_Adjustment_Task, "Traffic Rate Adjustment", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate( Traffic_Manager_Task, "Traffic Flow Manager", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate( TrafficLight_Manager_Task, "Traffic Light Manager", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

	/* Setup Timer */
//	TimerHandle_t xTrafficLightTimer = xTimerCreate("Traffic Light Timer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, vTrafficLightTimerCallback);
//	xTimerStart(xTrafficLightTimer, 0);

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	while(1);

	return 0;
}

/*-----------------------------------------------------------*/

/***********************************************************/
/*				TRAFFIC LIGHT FUNCTIONS				       */
/***********************************************************/

/* Initialize the red, amber, and green LEDs for the traffic light.
 * These three lights will correspond to GPIO Pins PC0, PC1, PC2.
 */
static void Init_Traffic_Light()
{
	/* Enable Clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef GPIO_TrafficLight_InitStruct;

	// Configuration
	GPIO_TrafficLight_InitStruct.GPIO_Pin = red_light | amber_light | green_light;
	GPIO_TrafficLight_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_TrafficLight_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_TrafficLight_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_TrafficLight_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

	/* Enable GPIO Pins */
	GPIO_Init(GPIOC, &GPIO_TrafficLight_InitStruct);
}

/*-----------------------------------------------------------*/

static uint16_t switchTrafficLightColour( uint16_t current_colour )
{
	switch (current_colour) {
		case green:
			GPIO_SetBits(GPIOC, amber_light);
			GPIO_ResetBits(GPIOC, green_light | red_light);
			return amber;
		case amber:
			GPIO_SetBits(GPIOC, red_light);
			GPIO_ResetBits(GPIOC, green_light | amber_light);
			return red;
			break;
		case red:
			GPIO_SetBits(GPIOC, green_light);
			GPIO_ResetBits(GPIOC, amber_light | red_light);
			return green;
		default:
			return 3;
	}
}

/*-----------------------------------------------------------*/

static uint16_t getTrafficLightWaitTime(TrafficRate traffic_rate, uint16_t current_colour)
{
	switch (current_colour){
		case green:
			switch (traffic_rate) {
				case LIGHT:
					return 2500;
				case MEDIUM:
					return 3000;
				case HEAVY:
					return 4000;
				case VERY_HEAVY:
					return 5000;
				default:
					return 6000;
			}
		case red:
			switch (traffic_rate) {
				case LIGHT:
					return 5000;
				case MEDIUM:
					return 4000;
				case HEAVY:
					return 3000;
				case VERY_HEAVY:
					return 2500;
				default:
					return 2000;
			}
		default:
			return 1000;
	}
}

/*-----------------------------------------------------------*/

static void TrafficLight_Manager_Task( void *pvParameters )
{
	uint16_t light_time_to_wait = 5000; // Default time to wait
	uint16_t traffic_light_state = red;
	TrafficRate traffic_rate = HEAVY;
    while(1)
    {
    	xQueueReceive(xTrafficRateQueue, &traffic_rate, 50);
    	traffic_light_state = switchTrafficLightColour(traffic_light_state);
    	light_time_to_wait = getTrafficLightWaitTime(traffic_rate, traffic_light_state);
    	xQueueSend(xTrafficRateQueue, &traffic_rate, 50);

        vTaskDelay(pdMS_TO_TICKS(light_time_to_wait));
    }
}

/*-----------------------------------------------------------*/

//static void vTrafficLightTimerCallback( xTimerHandle timer ){
//	uint16_t traffic_light_colour;
//	TrafficRate traffic_rate;
//	uint16_t time_to_wait;
//	xQueueReceive(xTrafficLightColourQueue, &traffic_light_colour, 100);
//	xQueueReceive(xTrafficRateQueue, &traffic_rate, 50);
//
//	traffic_light_colour = switchTrafficLightColour(traffic_light_colour);
//	time_to_wait = getTrafficLightWaitTime(traffic_rate, traffic_light_colour);
//
//	xTimerChangePeriod(xTrafficLightTimer, pdMS_TO_TICKS(time_to_wait), 0);
//
//	xQueueSend(xTrafficLightColourQueue, &traffic_light_colour, 100);
//	xQueueSend(xTrafficRateQueue, &traffic_rate, 50);
//}

/***********************************************************/
/*				POTENTIOMETER FUNCTIONS				       */
/***********************************************************/

/*-----------------------------------------------------------*/

/* Initialize the red, amber, and green LEDs for the traffic light.
 * These three lights will correspond to GPIO Pins PC0, PC1, PC2.
 */
static void Init_Potentiometer()
{
	Init_PC3(); // This is the GPIO pin that will be reading the potentiometer's values
	Init_ADC1();
}

/*-----------------------------------------------------------*/

/* Initialize the ADC1
 */
static void Init_ADC1()
{
	/* Enable Clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_InitTypeDef ADC_InitStruct;
	// Configuration
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_ExternalTrigConv = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStruct.ADC_NbrOfConversion = 1;
	ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;

	/* Enable ADC1 */
	ADC_Init(ADC1, &ADC_InitStruct);

	/* Channel Setup */
	ADC_Cmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 1, ADC_SampleTime_84Cycles);
}

/*-----------------------------------------------------------*/

/* Initialize the PC3 as the reader for the ADC
 */
static void Init_PC3()
{

	GPIO_InitTypeDef GPIO_PC3_InitStruct;

	// Configuration
	GPIO_PC3_InitStruct.GPIO_Pin = GPIO_Pin_3;
	GPIO_PC3_InitStruct.GPIO_Mode = GPIO_Mode_AN;
	GPIO_PC3_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

	/* Enable GPIO Pins */
	GPIO_Init(GPIOC, &GPIO_PC3_InitStruct);
}

/*-----------------------------------------------------------*/

/* Reads the potentiometer and returns the value
 */
static uint16_t readPotentiometer()
{
	uint16_t read_value;
	ADC_SoftwareStartConv(ADC1);
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); // Wait until conversion finished
	read_value = ADC_GetConversionValue(ADC1);
	printf("Potentiometer reading: %d\n", read_value);
	return read_value;
}

/*-----------------------------------------------------------*/

/* Return the correct traffic rate given the read adc value
 * */
static TrafficRate getTrafficRate(uint16_t read_adc_value)
{

	if (read_adc_value < 1400) {
		return LIGHT;
	} else if (read_adc_value >= 1400 && read_adc_value < 2600) {
		return MEDIUM;
	} else if (read_adc_value >= 2600 && read_adc_value < 4000) {
		return HEAVY;
	} else {
		return VERY_HEAVY;
	}
}

/*-----------------------------------------------------------*/

/* Return the correct gap in traffic based on the current rate
 * */
static uint16_t getTrafficGap(TrafficRate rate)
{
	switch (rate) {
		case LIGHT:
			return 3;
		case MEDIUM:
			return 2;
		case HEAVY:
			return 1;
		case VERY_HEAVY:
			return 0;
		default:
			return 12;
	}
}

/*-----------------------------------------------------------*/

static void TrafficRate_Adjustment_Task( void *pvParameters ) //TODO: Test
{
	uint16_t potentiometer_reading = 0;
	TrafficRate new_traffic_rate = HEAVY;
	while(1){
		xQueueReceive(xTrafficRateQueue, &new_traffic_rate, portMAX_DELAY); //block in the case of an emtpy queue, until a new value is received
		potentiometer_reading = readPotentiometer();
		new_traffic_rate = getTrafficRate(potentiometer_reading);
		xQueueSend(xTrafficRateQueue, &new_traffic_rate, 50); // Send flow rate to queue
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

/*-----------------------------------------------------------*/

static void Traffic_Manager_Task( void *pvParameters )
{
	TrafficRate traffic_rate = HEAVY;
	uint16_t traffic_gap = 3;
    while(1)
    {
    	xQueueReceive(xTrafficRateQueue, &traffic_rate, 50); // Get flow rate from queue

    	traffic_gap = getTrafficGap(traffic_rate);

    	xQueueSend(xTrafficRateQueue, &traffic_rate, 50); // Send flow rate back to queue

    	GPIO_Shift_Register_bit_On(); // Add a car
    	vTaskDelay(pdMS_TO_TICKS(500));

    	for(int i = 0; i < traffic_gap; i++)
    	{
    		GPIO_Shift_Register_bit_Off(); // Add appropriate number of blank cars based on traffic rate
    		vTaskDelay(pdMS_TO_TICKS(500));
    	}
    }
}

/*-----------------------------------------------------------*/

/***********************************************************/
/*				SHIFT REGISTER FUNCTIONS				   */
/***********************************************************/

static void GPIO_Init_Shift_Register(void)
{
	GPIO_InitTypeDef GPIO_ShiftRegister_InitStruct;
	/* Enable the GPIO_LED Clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	/* Configure the GPIO_LED pin */
	GPIO_ShiftRegister_InitStruct.GPIO_Pin = Shift_Register_Reset | Shift_Register_Clock | Shift_Register_Data;
	GPIO_ShiftRegister_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_ShiftRegister_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_ShiftRegister_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_ShiftRegister_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_ShiftRegister_InitStruct);
}

static void GPIO_Reset_Shift_Register(void)
{
	GPIO_ResetBits(GPIOC, Shift_Register_Reset);
	for (int i = 0; i < 10; i++);		// Wait for 10 clock cycles
	GPIO_SetBits(GPIOC, Shift_Register_Reset);
}

static void GPIO_Shift_Register_bit_On(void)
{
	// Reset the data pin
	GPIO_SetBits(GPIOC, Shift_Register_Data);
	// Generate a pulse on the clock pin
	GPIO_ResetBits(GPIOC, Shift_Register_Clock);
	for (int i = 0; i < 5; i++);		// Wait for 5 clock cycles
	GPIO_SetBits(GPIOC, Shift_Register_Clock);
	printf("A car was added to the intersection\n");
}

static void GPIO_Shift_Register_bit_Off(void)
{
	// Reset the data pin
	GPIO_ResetBits(GPIOC, Shift_Register_Data);
	// Generate a pulse on the clock pin
	GPIO_ResetBits(GPIOC, Shift_Register_Clock);
	for (int i = 0; i < 5; i++);		// Wait for 5 clock cycles
	GPIO_SetBits(GPIOC, Shift_Register_Clock);
	printf("No car at this time!\n");
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
