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
#include "stdio.h"

class PMU{
private:
	ADS131A04IPBSR ADC_IC; //ADC는 PMU 안(예를 들면 PMU의 동작을 정의하는 곳)에서만 사용 가능
	EEPROM24FC064  myEEPROM;
	CalibrationData_t myCalData;
	//AD5522 PMU_IC;
public:
	PMU();
	~PMU();

	void Init();
	void Loop();
	bool MeasureOhm();
	bool MeasureVolt();
	bool MeasureAmp();
};



#endif /* INC_PMU_H_ */
