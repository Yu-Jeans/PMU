/*
 * ADS131A04.cpp
 *
 *  Created on: 2026. 3. 19.
 *      Author: yujin
 */
#include "ADS131A04.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

extern SPI_HandleTypeDef hspi2;

ADS131A04IPBSR::ADS131A04IPBSR() {}
ADS131A04IPBSR::~ADS131A04IPBSR() {}

bool ADS131A04IPBSR::Init() {
	uint8_t tx_cmd[2] = {0x00, 0x00};
	uint8_t rx_data[2] = {0, 0};

	vTaskDelay(pdMS_TO_TICKS(10));

	HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi2, tx_cmd, rx_data, 2, 100);
	HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_SET);

	uint16_t received_word = (rx_data[0] << 8) | rx_data[1];

	if((received_word & 0xFF00) == 0xFF00) {

		uint8_t device_id = (received_word & 0x00FF);

		printf("ADS131A04 Ready! Device ID: 0x%02X\r\n", device_id);

		Send_UNLOCK();

		return true;
	}
	else {
		printf("ADS131A04 Init Failed. Rx: 0x%04X\r\n", received_word);
		return false;
	}
}

bool ADS131A04IPBSR::Send_UNLOCK() {
	uint8_t unlock_cmd[2] = {0x06, 0x55};
	uint8_t receive_data[2] = {0,0};

	HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi2, unlock_cmd, receive_data, 2, 100);
	HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_SET);

	return true;
}

uint16_t ADS131A04IPBSR::GetRawValue() {
    uint8_t read_cmd[2] = {0x00, 0x00};
    uint8_t receive_data[2] = {0, 0};

    HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi2, read_cmd, receive_data, 2, 100);
    HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_SET);

    uint16_t raw_adc_value = (receive_data[0] << 8) | receive_data[1]; //ADC 칩 MSB First, 배열에는 데이터 도착 순서대로 담김. data[0]에 MSB 부분이 들어감

    return raw_adc_value;
}


float ADS131A04IPBSR::GetVolt() {
    uint16_t raw = GetRawValue();

    float voltage = (float)raw * (4.0f / 65535.0f); //vref 4v라고 가정했을 떄 최대가 65535

    return voltage;
}
