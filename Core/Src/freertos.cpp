/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "PMU.h"
#include "cmsis_os.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb_device.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
extern "C" {
    void StartDefaultTask(void *argument);
    void MX_FREERTOS_Init(void);
}
extern PMU mySystem;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for commandTask */
osThreadId_t commandTaskHandle;
const osThreadAttr_t commandTask_attributes = {
  .name = "commandTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for uartQueue */
osMessageQueueId_t uartQueueHandle;
const osMessageQueueAttr_t uartQueue_attributes = {
  .name = "uartQueue"
};
/* Definitions for usbRxQueue */
osMessageQueueId_t usbRxQueueHandle;
const osMessageQueueAttr_t usbRxQueue_attributes = {
  .name = "usbRxQueue"
};


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartCommandTask(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */


  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of commandTask */
  commandTaskHandle = osThreadNew(StartCommandTask, NULL, &commandTask_attributes);


  /* add threads, ... */


  /* Create the queue(s) */

  /* creation of uartQueue */
  uartQueueHandle = osMessageQueueNew (4, sizeof(PMU_Data_t), &uartQueue_attributes);

  /* creation of usbRxQueue */
  usbRxQueueHandle = osMessageQueueNew (128, sizeof(uint8_t), &usbRxQueue_attributes);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
	mySystem.Init();
  /* Infinite loop */
	for(;;)
	  {
   		  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
   		for(int i = 0; i < 4; i++) {
				mySystem.MeasureVolt(i);
			}
   		osMessageQueuePut(uartQueueHandle, &mySystem.latestData, 0, 0);
	  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void ProcessCommand(const char* cmdBuffer) {
    char cmd;
    int ch;
    float val;

    // 1. V나 I 명령어 처리 ("V 0 5.0")
    if (sscanf(cmdBuffer, "%c %d %f", &cmd, &ch, &val) == 3) {
        if (cmd == 'V' || cmd == 'v') {
            mySystem.SetOutputVoltage(ch, val);
            printf("\r\n[OK] CH%d Voltage -> %.3f V\r\n", ch, val);
        }
        else if (cmd == 'I' || cmd == 'i') {
            mySystem.SetOutputCurrent(ch, val);
            printf("\r\n[OK] CH%d Current -> %.1f uA\r\n", ch, val);
        }
    }
    // 2. STOP 명령어 처리
    else if (strcmp(cmdBuffer, "STOP") == 0) {
        mySystem.Emergency_Stop();
        printf("\r\n[WARNING] EMERGENCY STOP!\r\n");
    }
    // 3. 알 수 없는 명령어
    else {
        printf("\r\n[ERROR] Unknown Command\r\n");
    }
}

void StartCommandTask(void *argument)
{
    char rxBuffer[64];
    uint8_t index = 0;
    uint8_t rxData;

    printf("\r\n=== PMU USB Command Mode ===\r\nPMU> ");

    for(;;)
    {
        if (osMessageQueueGet(usbRxQueueHandle, &rxData, NULL, portMAX_DELAY) == osOK)
        {
            printf("%c", rxData); // 사용자가 친 글자 화면에 보여주기

            if (rxData == '\r' || rxData == '\n') // 엔터키를 쳤을 때
            {
                if (index > 0) {
                    rxBuffer[index] = '\0'; // 문자열 닫기

                    ProcessCommand(rxBuffer);

                    index = 0; // 버퍼 비우기
                    printf("PMU> "); // 다음 입력 대기
                }
            }
            else if (rxData == '\b' || rxData == 0x7F) { // 백스페이스 처리
                if (index > 0) index--;
            }
            else { // 일반 글자 모으기
                if (index < 63) rxBuffer[index++] = rxData;
            }
        }
    }
}
/* USER CODE END Application */

