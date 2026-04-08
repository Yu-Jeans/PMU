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
		GPIO_TypeDef* loadP_pmu, uint16_t loadN_pmu,
		GPIO_TypeDef* resetP_pmu, uint16_t resetN_pmu,

         I2C_HandleTypeDef* hi2c_eeprom, uint16_t addr_eeprom)

    : ADC_IC(hspi_adc, csPort_adc, csPin_adc),
	  PMU_IC(hspi_pmu, syncP_pmu, syncN_pmu, busyP_pmu, busyN_pmu, loadP_pmu, loadN_pmu, resetP_pmu, resetN_pmu),
	  myEEPROM(hi2c_eeprom, addr_eeprom){}

PMU::~PMU() {
}

bool PMU::Init() {
    bool success = true;

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

	// PMU 안전 한계선(Clamp, Comparator) 및 초기값 설정
	for (int ch = 0; ch < 4; ch++) {
		// 💡 [핵심 수정] 비트 마스크(1, 2, 4, 8)로 정확하게 채널 선택!
		AD5522::Channel current_ch = (AD5522::Channel)(1 << ch);

		PMU_IC.SetClamp(current_ch, AD5522::DAC_CLH, 0b101, 5.5f);
		PMU_IC.SetClamp(current_ch, AD5522::DAC_CLL, 0b101, -5.5f);

		PMU_IC.SetComparator(current_ch, AD5522::DAC_CPH, 0b101, 5.0f);
		PMU_IC.SetComparator(current_ch, AD5522::DAC_CPL, 0b101, -5.0f);

		PMU_IC.SetForceValue(current_ch, 0b101, 0.0f);
	}

	// PMU 실제 출력 스위치 ON
	for (int ch = 0; ch < 4; ch++) {
		AD5522::Channel current_ch = (AD5522::Channel)(1 << ch);
		PMU_IC.SetChannelMode(current_ch, true, AD5522::FV_MODE, AD5522::RANGE_2mA, AD5522::MI_MODE);
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






// [PMU.cpp의 맨 아래 Emergency_Stop() 함수]
void PMU::Emergency_Stop() {
    // 4개 채널 모두 출력 즉시 차단
    for (int ch = 0; ch < 4; ch++) {
        AD5522::Channel current_ch = (AD5522::Channel)(1 << ch);

        // 여기도 정확한 enum 이름으로 수정
        PMU_IC.SetChannelMode(current_ch, false, AD5522::FV_MODE, AD5522::RANGE_2mA, AD5522::MI_MODE);
    }
}
