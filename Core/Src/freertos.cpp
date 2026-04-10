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
#include <ctype.h>
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
/* Definitions for PMU_Task */
osThreadId_t PMU_TaskHandle;
const osThreadAttr_t PMU_Task_attributes = {
  .name = "PMU_Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for commandTask */
osThreadId_t commandTaskHandle;
const osThreadAttr_t commandTask_attributes = {
  .name = "commandTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for uartLoggingTask */
osThreadId_t uartLoggingTaskHandle;
const osThreadAttr_t uartLoggingTask_attributes = {
  .name = "uartLoggingTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ADC_Task */
osThreadId_t ADC_TaskHandle;
const osThreadAttr_t ADC_Task_attributes = {
  .name = "ADC_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
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
/* Definitions for pmuCmdQueue */
osMessageQueueId_t pmuCmdQueueHandle;
const osMessageQueueAttr_t pmuCmdQueue_attributes = {
  .name = "pmuCmdQueue"
};


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartPMU_Task(void *argument);
void StartCommandTask(void *argument);
void StartUartLoggingTask(void *argument);
void StartADC_Task(void *argument);

extern void MX_USB_DEVICE_Init(void);
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

  /* Create the queue(s) */
  /* creation of uartQueue */
  uartQueueHandle = osMessageQueueNew (4, sizeof(PMU_Data_t), &uartQueue_attributes);

  /* creation of usbRxQueue */
  usbRxQueueHandle = osMessageQueueNew (128, sizeof(uint8_t), &usbRxQueue_attributes);

  /* creation of pmuCmdQueue */
  pmuCmdQueueHandle = osMessageQueueNew (8, sizeof(PMU_CmdPacket_t), &pmuCmdQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of PMU_Task */
  PMU_TaskHandle = osThreadNew(StartPMU_Task, NULL, &PMU_Task_attributes);

  /* creation of commandTask */
  commandTaskHandle = osThreadNew(StartCommandTask, NULL, &commandTask_attributes);

  /* creation of uartLoggingTask */
  uartLoggingTaskHandle = osThreadNew(StartUartLoggingTask, NULL, &uartLoggingTask_attributes);

  /* creation of ADC_Task */
  ADC_TaskHandle = osThreadNew(StartADC_Task, NULL, &ADC_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartPMU_Task */
/**
  * @brief  Function implementing the PMU_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartPMU_Task */
void StartPMU_Task(void *argument)
{
  /* USER CODE BEGIN StartPMU_Task */
  mySystem.Init();

  PMU_CmdPacket_t order;
  /* Infinite loop */
  for(;;)
  {
	    if (osMessageQueueGet(pmuCmdQueueHandle, &order, NULL, osWaitForever) == osOK)
		{
			switch (order.cmd_type)
			{
				case CMD_FORCE_VOLTAGE:
					mySystem.SetOutputVoltage(order.channel, order.value);
					break;

				case CMD_FORCE_CURRENT:
					mySystem.SetOutputCurrent(order.channel, order.value);
					break;

				case CMD_HIGH_Z_V:
				    mySystem.SetHighZ(order.channel, AD5522::HIGH_Z_V);
				    break;

				case CMD_HIGH_Z_I:
				    mySystem.SetHighZ(order.channel, AD5522::HIGH_Z_I);
				    break;

				case CMD_MEAS_VOLTAGE:
					mySystem.MeasureVolt(order.channel);
					xTaskNotifyGive((TaskHandle_t)commandTaskHandle);
					break;

				case CMD_MEAS_CURRENT:
					mySystem.MeasureCurrent(order.channel);
					xTaskNotifyGive((TaskHandle_t)commandTaskHandle);
					break;

				case CMD_MEAS_TEMP:
				    mySystem.MeasureTemp(order.channel);
				    xTaskNotifyGive((TaskHandle_t)commandTaskHandle);
				    break;

				case CMD_EMERGENCY_STOP:
					mySystem.Emergency_Stop();
					break;

				case CMD_SAVE_CALIBRATION:
				    mySystem.SaveCalibrationToEEPROM();
				    break;
			}
			// 명령을 수행할 때마다 UART 로깅창(uartQueue)으로 최신 상태 업데이트
			osMessageQueuePut(uartQueueHandle, &mySystem.latestData, 0, 0);
		}
  }
  /* USER CODE END StartPMU_Task */
}

/* USER CODE BEGIN Header_StartUartLoggingTask */
/**
* @brief Function implementing the uartLoggingTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUartLoggingTask */
void StartUartLoggingTask(void *argument)
{
  /* USER CODE BEGIN StartUartLoggingTask */
  extern UART_HandleTypeDef huart6; // main.cpp에 있는 UART6 핸들 가져오기
  PMU_Data_t logData;
  char msg[256];
  /* Infinite loop */
  for(;;)
  {
	    if (osMessageQueueGet(uartQueueHandle, &logData, NULL, osWaitForever) == osOK) {
			int len = 0;

			// TEMP 알람 발생 시
			if (logData.system_status == SYS_ALARM_TEMP) {
				len = sprintf(msg, "\r\n[CRITICAL] TEMPERATURE ALARM!!! SYSTEM STOPPED.\r\n");
			}
			// CGALM 알람 발생 시
			else if (logData.system_status == SYS_ALARM_CGALM) {
				len = sprintf(msg, "\r\n[CRITICAL] CLAMP(CG) ALARM!!! SYSTEM STOPPED.\r\n");
			}
			// 비상시, comparator 상한, 하한 감지되었을 때 (ADC_Task가 보냄)
			else if (logData.comparator_status != 0) {
				len = sprintf(msg, "\r\n[WARN] COMPARATOR OUT OF BOUNDS! (STS:0x%02X)\r\n"
								   "  => [Volt] CH0:%.3f, CH1:%.3f, CH2:%.3f, CH3:%.3f\r\n"
								   "  => [Curr] CH0:%.3f, CH1:%.3f, CH2:%.3f, CH3:%.3f\r\n",
							  logData.comparator_status,
							  logData.voltage[0], logData.voltage[1],
							  logData.voltage[2], logData.voltage[3],
			                  logData.current[0], logData.current[1],
			                  logData.current[2], logData.current[3]);
			}
			// 평상시, 정상 측정 데이터일 때 (PMU_Task가 보냄)
			else {
				len = sprintf(msg, "[LOG] V0:%.3f, V1:%.3f, V2:%.3f, V3:%.3f, I0:%.3f, I1:%.3f, I2:%.3f, I3:%.3f (STS:OK)\r\n",
							  logData.voltage[0], logData.voltage[1],
							  logData.voltage[2], logData.voltage[3],
							  logData.current[0], logData.current[1],
							  logData.current[2], logData.current[3]);
			}
			HAL_UART_Transmit(&huart6, (uint8_t*)msg, len, 100);
		}
  }
  /* USER CODE END StartUartLoggingTask */
}

void ProcessCommand(const char* cmdBuffer) {
    char cmdStr[4];
    int ch;
    float val = 0.0f;

	PMU_CmdPacket_t order;


	if (strcmp(cmdBuffer, "stop") == 0) {
		order.cmd_type = CMD_EMERGENCY_STOP;
		osMessageQueuePut(pmuCmdQueueHandle, &order, 0, portMAX_DELAY);
		printf("\r\n[WARNING] EMERGENCY STOP!\r\n");
		return;
	}


	if (sscanf(cmdBuffer, "%s %d %f", cmdStr, &ch, &val) >= 2) {
		if (ch < 0 || ch > 3) {
			printf("\r\n[ERROR] Invalid Channel (0~3)\r\n");
			return;
		}

		for(int i = 0; cmdStr[i]; i++) cmdStr[i] = tolower(cmdStr[i]);

		if (strcmp(cmdStr, "fv") == 0) {
			order.cmd_type = CMD_FORCE_VOLTAGE; order.channel = ch; order.value = val;
			osMessageQueuePut(pmuCmdQueueHandle, &order, 0, portMAX_DELAY);
			printf("\r\n[OK] CH%d FV -> %.3f V\r\n", ch, val);
		}
		else if (strcmp(cmdStr, "fi") == 0) {
			order.cmd_type = CMD_FORCE_CURRENT; order.channel = ch; order.value = val;
			osMessageQueuePut(pmuCmdQueueHandle, &order, 0, portMAX_DELAY);
			printf("\r\n[OK] CH%d FI -> %.1f uA\r\n", ch, val);
		}
		else if (strcmp(cmdStr, "hzv") == 0) {
		    order.cmd_type = CMD_HIGH_Z_V;
		    order.channel = ch;
		    osMessageQueuePut(pmuCmdQueueHandle, &order, 0, portMAX_DELAY);
		    printf("\r\n[OK] CH%d -> HIGH-Z (Voltage Preload) Mode\r\n", ch);
		}
		else if (strcmp(cmdStr, "hzi") == 0) {
		    order.cmd_type = CMD_HIGH_Z_I;
		    order.channel = ch;
		    osMessageQueuePut(pmuCmdQueueHandle, &order, 0, portMAX_DELAY);
		    printf("\r\n[OK] CH%d -> HIGH-Z (Current Preload) Mode\r\n", ch);
		}
		else if (strcmp(cmdStr, "mv") == 0) {
			order.cmd_type = CMD_MEAS_VOLTAGE; order.channel = ch;
			osMessageQueuePut(pmuCmdQueueHandle, &order, 0, portMAX_DELAY);
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			printf("\r\n[MEAS] CH%d MV: %.4f V\r\n", ch, mySystem.latestData.voltage[ch]);
		}
		else if (strcmp(cmdStr, "mi") == 0) {
			order.cmd_type = CMD_MEAS_CURRENT; order.channel = ch;
			osMessageQueuePut(pmuCmdQueueHandle, &order, 0, portMAX_DELAY);
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			printf("\r\n[MEAS] CH%d MI: %.3f uA\r\n", ch, mySystem.latestData.current[ch]);
		}
		else if (strcmp(cmdStr, "mt") == 0) {
		    order.cmd_type = CMD_MEAS_TEMP;
		    order.channel = ch;
		    osMessageQueuePut(pmuCmdQueueHandle, &order, 0, portMAX_DELAY);
		    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			printf("CH%d Die Temp: %.1f C\r\n", ch, mySystem.latestData.temp[ch]);
		}


		else {
			printf("\r\n[ERROR] Unknown Command: %s\r\n", cmdStr);
		}
	}
	else {
		printf("\r\n[ERROR] Invalid Format (ex: FV 0 5.0)\r\n");
	}
}

void StartCommandTask(void *argument)
{
    /* init code for USB_DEVICE */
    MX_USB_DEVICE_Init();

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

/* USER CODE BEGIN Header_StartADC_Task */
/**
* @brief Function implementing the ADC_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartADC_Task */
void StartADC_Task(void *argument)
{
  /* USER CODE BEGIN StartADC_Task */
  /* Infinite loop */
  for(;;)
  {
	  // 1. HAL_GPIO_EXTI_Callback에서 보내는 알람(인터럽트) 대기
	  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	  // 인터럽트가 최신화한 비교기 상태를 확인
	  if (mySystem.latestData.comparator_status != 0) {
		  // 값 즉시 업데이트
		  mySystem.EmergencyMeasureAll();
		  // 상태가 정상이 아닐 때만 로그 창(uartQueue)으로 데이터 강제 전송
		  osMessageQueuePut(uartQueueHandle, &mySystem.latestData, 0, 0);
	  }
  }
  /* USER CODE END StartADC_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
