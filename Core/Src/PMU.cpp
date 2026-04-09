/*
 * PMU.cpp
 *
 *  Created on: 2026. 3. 24.
 *      Author: yujin
 */

#include "PMU.h"
#include "cmsis_os2.h"

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

		current_state_range[ch]  = AD5522::RANGE_2mA;
        current_force_mode[ch]   = AD5522::FV_MODE;
        current_measure_mode[ch] = AD5522::MI_MODE;
	}

    // 칩들이 깨어난 후 PMU 차원의 추가 설정
    if (success) {
		printf("PMU: Initialization Complete. System Online.\r\n");
	} else {
		printf("PMU: Initialization Failed. System Halted.\r\n");
	}
    return success;
}



float PMU::GetRangeResistance(AD5522::CurrentRange range) {
    switch(range) {
        case AD5522::RANGE_5uA:   return 200000.0f; // 200 kΩ
        case AD5522::RANGE_20uA:  return 50000.0f;  // 50 kΩ
        case AD5522::RANGE_200uA: return 5000.0f;   // 5 kΩ
        case AD5522::RANGE_2mA:   return 500.0f;    // 500 Ω
        default:                  return 1.0f;
    }
}


void PMU::SetOutputVoltage(int ch, float target_volt) {
    AD5522::Channel dac_ch = (AD5522::Channel)(1 << ch);

	float calibrated_volt = (target_volt - myCalData.v_offset[ch]) / myCalData.v_gain[ch];

	PMU_IC.SetChannelMode(dac_ch, true, AD5522::FV_MODE, AD5522::RANGE_2mA, current_measure_mode[ch]);
    PMU_IC.SetForceValue(dac_ch, 0b101, calibrated_volt);

    current_force_mode[ch] = AD5522::FV_MODE;
}



void PMU::SetOutputCurrent(int ch, float target_current_uA) {
    AD5522::Channel dac_ch = (AD5522::Channel)(1 << ch);

	AD5522::CurrentRange auto_range;

	float abs_current = (target_current_uA < 0) ? -target_current_uA : target_current_uA;

	if (abs_current <= 5.0f) {
		auto_range = AD5522::RANGE_5uA;
	} else if (abs_current <= 20.0f) {
		auto_range = AD5522::RANGE_20uA;
	} else if (abs_current <= 200.0f) {
		auto_range = AD5522::RANGE_200uA;
	} else if (abs_current <= 2000.0f) {
		auto_range = AD5522::RANGE_2mA;
	} else {
		printf("\r\n[ERROR] CH%d Current out of limit (Max 2mA)\r\n", ch);
		return;
	}

    float calibrated_current = (target_current_uA - myCalData.i_offset[ch]) / myCalData.i_gain[ch];

    PMU_IC.SetChannelMode(dac_ch, true, AD5522::FI_MODE, auto_range, current_measure_mode[ch]);
    PMU_IC.SetForceValue(dac_ch, auto_range, calibrated_current);

    current_state_range[ch] = auto_range;
    current_force_mode[ch] = AD5522::FI_MODE;
}



void PMU::MeasureVolt(int ch) {
	AD5522::Channel dac_ch = (AD5522::Channel)(1 << ch);

	if (current_measure_mode[ch] != AD5522::MV_MODE) {
		PMU_IC.SetChannelMode(dac_ch, true, current_force_mode[ch], current_state_range[ch], AD5522::MV_MODE);
		current_measure_mode[ch] = AD5522::MV_MODE;
		osDelay(2);
	}

	float raw_voltage = ADC_IC.GetVolt(ch);
	float pure_voltage = (raw_voltage - 2.25f) * 5.0f;
	float real_voltage = (pure_voltage * myCalData.v_gain[ch]) + myCalData.v_offset[ch];

    printf("CH%d Measure Voltage: %.4f V\r\n", ch, real_voltage);
    latestData.voltage[ch]= real_voltage;
}



void PMU::MeasureCurrent(int ch) {
    AD5522::Channel dac_ch = (AD5522::Channel)(1 << ch);

    if (current_measure_mode[ch] != AD5522::MI_MODE) {
		PMU_IC.SetChannelMode(dac_ch, true, current_force_mode[ch], current_state_range[ch], AD5522::MI_MODE);
		current_measure_mode[ch] = AD5522::MI_MODE;
		osDelay(2);
	}

    AD5522::CurrentRange current_range = current_state_range[ch];
    float raw_voltage = ADC_IC.GetVolt(ch);
    float range_resistance = GetRangeResistance(current_range);

    float pure_v = raw_voltage - 2.25f;
    float current_uA_raw = (pure_v / (range_resistance* 2.0f)) * 1000000.0f;

    float calculated_current = current_uA_raw * myCalData.i_gain[ch] + myCalData.i_offset[ch];
    float abs_current = (calculated_current < 0) ? -calculated_current : calculated_current;

    AD5522::CurrentRange next_range = current_range;

    if (abs_current <= 5.0f) {
		next_range = AD5522::RANGE_5uA;
	}
	else if (abs_current <= 20.0f) {
		next_range = AD5522::RANGE_20uA;
	}
	else if (abs_current <= 200.0f) {
		next_range = AD5522::RANGE_200uA;
	}
	else {
		next_range = AD5522::RANGE_2mA;
	}

    if (current_range != next_range) {
		PMU_IC.SetChannelMode(dac_ch, true, current_force_mode[ch], next_range, current_measure_mode[ch]);
		current_state_range[ch] = next_range;
		osDelay(2);

		raw_voltage = ADC_IC.GetVolt(ch);
		range_resistance = GetRangeResistance(next_range);


	    pure_v = raw_voltage - 2.25f;
		current_uA_raw = (pure_v / (range_resistance * 2.0f)) * 1000000.0f;
		calculated_current = current_uA_raw * myCalData.i_gain[ch] + myCalData.i_offset[ch];
	}

    printf("CH%d Measure Current: %.3f uA\r\n", ch, calculated_current);
    latestData.current[ch] = calculated_current;
}



// 4개 채널 모두 출력 즉시 차단
void PMU::Emergency_Stop() {
	PMU_IC.SetChannelMode(AD5522::ALL, false, AD5522::FV_MODE, AD5522::RANGE_2mA, AD5522::MI_MODE);
}
