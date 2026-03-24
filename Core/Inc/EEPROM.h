/*
 * EEPROM.h
 *
 *  Created on: 2026. 3. 20.
 *      Author: yujin
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#pragma once
#include "main.h"

// 🚨 패딩 방지 마법의 주문!
// float(4바이트) * 8개 = 총 32바이트 (24FC064의 정확히 1페이지 사이즈!)
typedef struct {
    float offset[4]; // 4개 채널의 영점 오차 (ex: +0.001V)
    float gain[4];   // 4개 채널의 기울기 오차 (ex: *1.002)
} __attribute__((packed)) CalibrationData_t;

class EEPROM24FC064 {
private:
    uint16_t devAddress; // 아파트 주소 (0xA0)

public:
    EEPROM24FC064(uint16_t addr) : devAddress(addr) {}
/* 아래와 동일
EEPROM24FC064(uint16_t addr) {
    devAddress = addr;
}
*/
    bool SaveCalibration(CalibrationData_t* data);
    bool LoadCalibration(CalibrationData_t* data);
};

#endif /* INC_EEPROM_H_ */
