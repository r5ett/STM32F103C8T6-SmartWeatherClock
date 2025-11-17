#ifndef __WIFI_H
#define __WIFI_H	 

#include "time.h" 

void wifi_GPIO_Init(void);
void rst_wifi(void);
void init_wifi_zhixin(void);
void Wifi_Init(void);
void Wifi_GetTime(void);
void Wifi_GetWeather(void);
void Wifi_Quit(void);
void ParseTimestamp(void);
void ParseCity(void);
void ParseWeather(void);
void ParseTemperature(void);

#endif
