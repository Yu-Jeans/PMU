/*
 * ADS131A04.h
 *
 *  Created on: 2026. 3. 19.
 *      Author: yujin
 */

#ifndef INC_ADS131A04_H_
#define INC_ADS131A04_H_

#define ADC_CS_GPIO_Port GPIOB
#define ADC_CS_Pin       GPIO_PIN_4

#include "main.h"

class ADS131A04IPBSR{
private:
	uint16_t SpiTransfer16(uint16_t command);
public:
	ADS131A04IPBSR();
	~ADS131A04IPBSR();

	bool Init();
	uint16_t GetRawValue();
	bool Send_UNLOCK();
	float GetVolt();
};

#endif /* INC_ADS131A04_H_ */
