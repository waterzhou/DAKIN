/*************************************************************************
    > File Name: hspiadapter.c
    > Author: Water Zhou
    > Mail: jianzhengzhou@yandex.com 
    > Created Time: 2016年09月23日 星期五 14时19分04秒
 ************************************************************************/

#include "esp_common.h"
#include "driver/spi.h"

int hspi_write_data(u8 *data, int len)
{
	int i = 0;
	for(i  = 0; i <len; i++)
	{
		spi_tx8(HSPI, *(data+i));
	}
	return i;
}