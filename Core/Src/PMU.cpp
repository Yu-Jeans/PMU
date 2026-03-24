/*
 * PMU.cpp
 *
 *  Created on: 2026. 3. 24.
 *      Author: yujin
 */

#include "PMU.h"

PMU::PMU() : myEEPROM(0xA0) {}
/* 아래와 동일
PMU(
	myEEPROM = 0xA0;
}
*/
PMU::~PMU() {
}


void PMU::Init() {
    if (myEEPROM.LoadCalibration(&myCalData)) {
        printf("PMU: EEPROM Loaded!\r\n");
    } else {
        printf("PMU: EEPROM Load Failed! Default Set.\r\n");
        for(int i=0; i<4; i++) {
            myCalData.offset[i] = 0.0f;
            myCalData.gain[i] = 1.0f;
        }
    }
}

bool PMU::MeasureVolt() {
    // float raw_voltage = ADC_IC.GetVolt(); // (ADC 함수 완성 전까지 주석 처리)
    float raw_voltage = 1.23f; // 테스트용 임시 값

    float real_voltage = (raw_voltage * myCalData.gain[0]) + myCalData.offset[0]; // 보정식

    printf("CH1 Measure Voltage: %f V\r\n", real_voltage);

    return true;
}

