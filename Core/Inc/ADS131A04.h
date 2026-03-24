/*
 * ADS131A04.h
 *
 *  Created on: 2026. 3. 19.
 *      Author: yujin
 */

#ifndef INC_ADS131A04_H_
#define INC_ADS131A04_H_

#include "main.h"

class ADS131A04IPBSR{
private:

public:
	ADS131A04IPBSR();
	~ADS131A04IPBSR();

	bool Init();
	bool GetRawValue();
	bool Send_UNLOCK();
	float GetVolt();
};

#endif /* INC_ADS131A04_H_ */
