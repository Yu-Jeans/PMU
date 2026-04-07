/*
 * PMU.cpp
 *
 *  Created on: 2026. 3. 24.
 *      Author: yujin
 */

#include "PMU.h"

PMU::PMU(SPI_HandleTypeDef* hspi_adc, GPIO_TypeDef* csPort_adc, uint16_t csPin_adc,

		SPI_HandleTypeDef* hspi_pmu,
		GPIO_TypeDef* syncP_pmu, uint16_t syncN_pmu,
		GPIO_TypeDef* busyP_pmu, uint16_t busyN_pmu,
		GPIO_TypeDef* resetP_pmu, uint16_t resetN_pmu,

         I2C_HandleTypeDef* hi2c_eeprom, uint16_t addr_eeprom)

    : ADC_IC(hspi_adc, csPort_adc, csPin_adc),
	  PMU_IC(hspi_pmu, syncP_pmu, syncN_pmu, busyP_pmu, busyN_pmu, resetP_pmu, resetN_pmu),
	  myEEPROM(hi2c_eeprom, addr_eeprom){}

PMU::~PMU() {
}

bool PMU::Init() {
    bool success = true;

    // ADC 초기화 (ADC_IC)
    if (!ADC_IC.Init()) {
        printf("PMU: ADC(ADS131A04) Hardware Init Failed!\r\n");
        success = false;
    }

    // AD5522 초기화 (PMU_IC)
    if (!PMU_IC.Init()) {
        printf("PMU: DAC(AD5522) Hardware Init Failed!\r\n");
        success = false;
    }


    // EEPROM 초기화 (myEEPROM)
    if (myEEPROM.Init()) { // EEPROM 칩 자체의 초기화 확인 (만약 Init 함수가 있다면)
		if (myEEPROM.LoadCalibration(&myCalData)) {
			printf("PMU: EEPROM Calibration Data Loaded!\r\n");
		} else {
			// 하드웨어는 정상이나, 내부에 저장된 캘리브레이션 데이터가 없거나 깨진 경우
			printf("PMU: [FATAL] EEPROM Calibration Data Load Failed!\r\n");
			success = false; // 신뢰할 수 없는 장비이므로 에러 처리!

			// 주의: 기본값을 덮어씌워 강제로 동작하게 하는 안전장치를 둘 수도 있지만,
			// 정밀 측정 장비라면 아예 동작을 막고 재교정(Recalibration)을 요구하는 것이 정석입니다.
		}
	} else {
		// EEPROM 칩 자체가 응답하지 않는 경우 (하드웨어 불량)
		printf("PMU: [FATAL] EEPROM Hardware Init Failed!\r\n");
		success = false;
	}

    // 3. 칩들이 깨어난 후 PMU 차원의 추가 설정
    if (success) {
		printf("PMU: Initialization Complete. System Online.\r\n");
	} else {
		printf("PMU: Initialization Failed. System Halted.\r\n");
	}

    return success;
}


bool PMU::MeasureVolt() {
    // float raw_voltage = ADC_IC.GetVolt(); // (ADC 함수 완성 전까지 주석 처리)
    float raw_voltage = 1.23f; // 테스트용 임시 값

    float real_voltage = (raw_voltage * myCalData.gain[0]) + myCalData.offset[0]; // 보정식

    printf("CH1 Measure Voltage: %f V\r\n", real_voltage);

    return true;
}
