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

class PMU{
private:
	ADS131A04IPBSR ADC_IC; //ADC는 PMU 안(예를 들면 PMU의 동작을 정의하는 곳)에서만 사용 가능
	AD5522         PMU_IC;
	EEPROM24FC064  myEEPROM;
	CalibrationData_t myCalData;
	//AD5522 PMU_IC;
public:
	PMU(SPI_HandleTypeDef* hspi_adc, GPIO_TypeDef* csPort_adc, uint16_t csPin_adc,

			SPI_HandleTypeDef* hspi_pmu,
			GPIO_TypeDef* syncP_pmu, uint16_t syncN_pmu,
			GPIO_TypeDef* busyP_pmu, uint16_t busyN_pmu,
			GPIO_TypeDef* loadP_pmu, uint16_t loadN_pmu,
			GPIO_TypeDef* resetP_pmu, uint16_t resetN_pmu,

	        I2C_HandleTypeDef* hi2c_eeprom, uint16_t addr_eeprom);
	~PMU();

	bool Init();
	void Loop();
	void Emergency_Stop();

	bool MeasureOhm();
	bool MeasureVolt();
	bool MeasureAmp();
};



#endif /* INC_PMU_H_ */
