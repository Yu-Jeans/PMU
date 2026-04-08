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
    if (myEEPROM.Init()) {
		if (myEEPROM.LoadCalibration(&myCalData)) {
			printf("PMU: EEPROM Calibration Data Loaded!\r\n");
		} else {
			// 하드웨어는 정상이나, 내부에 저장된 캘리브레이션 데이터가 없거나 깨진 경우
			printf("PMU: [FATAL] EEPROM Calibration Data Load Failed!\r\n");
			success = false;
		}
	} else {
		// EEPROM 칩 자체가 응답하지 않는 경우
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

    // 칩들이 깨어난 후 PMU 차원의 추가 설정
    if (success) {
		printf("PMU: Initialization Complete. System Online.\r\n");
	} else {
		printf("PMU: Initialization Failed. System Halted.\r\n");
	}
    return success;
}




void PMU::SetOutputVoltage(int ch, float target_volt) {
    float calibrated_volt = (target_volt - myCalData.v_offset[ch]) / myCalData.v_gain[ch];

    AD5522::Channel dac_ch = (AD5522::Channel)(1 << ch);

    PMU_IC.SetForceValue(dac_ch, 0b101, calibrated_volt);
}

void PMU::SetOutputCurrent(int ch, float target_current_uA) {
    float calibrated_current = (target_current_uA - myCalData.i_offset[ch]) / myCalData.i_gain[ch];

    AD5522::Channel dac_ch = (AD5522::Channel)(1 << ch);

    PMU_IC.SetForceValue(dac_ch, 0b101, calibrated_current);
}

void PMU::MeasureVolt(int ch) {
	float raw_voltage = ADC_IC.GetVolt(ch);

	float real_voltage = (raw_voltage * myCalData.v_gain[ch]) + myCalData.v_offset[ch];

    printf("CH%d Measure Voltage: %f V\r\n", ch, real_voltage);

    latestData.voltage[ch]= real_voltage;
}

void PMU::MeasureCurrent(int ch) {
    float raw_voltage = ADC_IC.GetVolt(ch);

    float real_current = (raw_voltage * myCalData.i_gain[ch]) + myCalData.i_offset[ch];

    printf("CH%d Measure Current: %.2f uA\r\n", ch, real_current);

    latestData.current[ch] = real_current;
}

// [PMU.cpp의 맨 아래 Emergency_Stop() 함수]
// 4개 채널 모두 출력 즉시 차단
void PMU::Emergency_Stop() {
	PMU_IC.SetChannelMode(AD5522::ALL, false, AD5522::FV_MODE, AD5522::RANGE_2mA, AD5522::MI_MODE);
}
