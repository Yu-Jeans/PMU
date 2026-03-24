/*
 * ADS131A04.cpp
 *
 *  Created on: 2026. 3. 19.
 *      Author: yujin
 */
#include "ADS131A04.h"
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi1;

// 생성자 구현
ADS131A04IPBSR::ADS131A04IPBSR() {
    // 초기화 코드 등... 비어있더라도 중괄호 {} 는 있어야 함
}

// 소멸자 구현
ADS131A04IPBSR::~ADS131A04IPBSR() {
    // 정리 코드 등... 비어있더라도 중괄호 {} 는 있어야 함
}

bool ADS131A04IPBSR::Init() {
	uint16_t received_word = 0;

	HAL_Delay(10);

	HAL_SPI_Receive(&hspi1, (uint8_t*)&received_word, 1, 100);

	if((received_word & 0xFF00) == 0xFF00) {

		uint8_t device_id = (received_word & 0x00FF);

		Send_UNLOCK();

		return true;
	}
	else {
		return false;
	}
}

bool ADS131A04IPBSR::Send_UNLOCK() {
	uint8_t unlock_cmd[2] = {0x06, 0x55};
	uint8_t receive_data[2] = {0,0};

	HAL_SPI_TransmitReceive(&hspi1, unlock_cmd, receive_data, 2, 100);

	return true;
}

bool ADS131A04IPBSR::GetRawValue() {
    uint8_t read_cmd[2] = {0x00, 0x00};
    uint8_t receive_data[2] = {0, 0};

    HAL_SPI_TransmitReceive(&hspi1, read_cmd, receive_data, 2, 100);

    uint16_t raw_adc_value = (receive_data[0] << 8) | receive_data[1]; //ADC 칩 MSB First, 배열에는 데이터 도착 순서대로 담김. data[0]에 MSB 부분이 들어감

    if (raw_adc_value != 0) {
        return true;
    } else {
        return false;
    }
}


float ADS131A04IPBSR::GetVolt() {
    uint16_t raw = GetRawValue();

    float voltage = (float)raw * (4.0f / 65535.0f); //vref 4v라고 가정했을 떄 최대가 65535

    return voltage;
}
