/*
 * PMU.h
 *
 *  Created on: 2026. 3. 24.
 *      Author: yujin
 */

#ifndef INC_PMU_H_
#define INC_PMU_H_

#include "ADS131A04.h"
#include "EEPROM.h"
#include "AD5522.h"
#include "stdio.h"

typedef struct {
    float voltage[4];
    float current[4];

	uint8_t comparator_status;
} PMU_Data_t;

typedef enum {
    CMD_FORCE_VOLTAGE,
    CMD_FORCE_CURRENT,
    CMD_MEAS_VOLTAGE,
    CMD_MEAS_CURRENT,
    CMD_EMERGENCY_STOP
} PMU_CmdType_t;

typedef struct {
    PMU_CmdType_t cmd_type;
    uint8_t channel;
    float value;
} PMU_CmdPacket_t;

class PMU{
private:
	ADS131A04IPBSR ADC_IC; //ADC는 PMU 안(예를 들면 PMU의 동작을 정의하는 곳)에서만 사용 가능
	AD5522         PMU_IC;
	EEPROM24FC064  myEEPROM;
	CalibrationData_t myCalData;

	AD5522::CurrentRange current_state_range[4];
	AD5522::ForceMode    current_force_mode[4];
    AD5522::MeasureMode  current_measure_mode[4];
public:
	PMU(SPI_HandleTypeDef* hspi_adc, GPIO_TypeDef* csPort_adc, uint16_t csPin_adc,

			SPI_HandleTypeDef* hspi_pmu,
			GPIO_TypeDef* syncP_pmu, uint16_t syncN_pmu,
			GPIO_TypeDef* busyP_pmu, uint16_t busyN_pmu,
			GPIO_TypeDef* loadP_pmu, uint16_t loadN_pmu,
			GPIO_TypeDef* resetP_pmu, uint16_t resetN_pmu,

	        I2C_HandleTypeDef* hi2c_eeprom, uint16_t addr_eeprom);
	~PMU();

	PMU_Data_t latestData;
	bool Init();
	void Loop();
	float GetRangeResistance(AD5522::CurrentRange range);
	void SetOutputVoltage(int ch, float target_volt);
	void SetOutputCurrent(int ch, float current_uA);
	void MeasureVolt(int ch);
	void MeasureCurrent(int ch);
	void Emergency_Stop();

	//bool MeasureOhm();
	//bool MeasureAmp();
};



#endif /* INC_PMU_H_ */
