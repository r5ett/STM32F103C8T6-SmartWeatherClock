#define WIFITEST 0
#define ST7789TEST 0
#define WIFI_ST7789TEST 1

#if WIFITEST
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Delay.h"
#include "sys.h"
#include "usart.h"
#include "usart2.h"
#include "wifi.h"
#include "OLED.h"

// 声明全局解析变量（需与解析函数中的定义一致）
extern time_t g_timestamp;
extern char g_city[32];
extern char g_weather[32];
extern char g_temperature[8];
char time_str[30];  // 存储转换后的时间字符串
int main(void) {
    // 初始化外设
    wifi_GPIO_Init();
    OLED_Init();
    uart_init(9600);
    USART2_Init(115200);  // 初始化ESP8266通信串口
    Wifi_Init();     // 初始化WiFi（假设已包含网络连接逻辑）

    OLED_Clear();
    OLED_ShowString(1, 1, "Initializing...");
    delay_ms(1000);

    while(1) {
        // 1. 获取天气数据
        Wifi_GetWeather();  // 发送AT命令获取天气数据
        delay_ms(1000);       // 等待数据接收完成

        // 2. 解析数据
        ParseCity();
        ParseWeather();
        ParseTemperature();

        // 3. OLED显示（清屏后刷新）
        OLED_Clear();
        // 显示城市（第1行，第1列）
        OLED_ShowString(1, 1, "City:");
        OLED_ShowString(1, 6, g_city);  // 从第6列开始显示城市名

        // 显示天气（第2行，第1列）
        OLED_ShowString(2, 1, "Weather:");
        OLED_ShowString(2, 9, g_weather);  // 从第9列开始显示天气

        // 显示温度（第3行，第1列），拼接℃单位
        OLED_ShowString(3, 1, "Temp:");
        OLED_ShowString(3, 6, g_temperature);
        OLED_ShowString(3, 9, "C");  // 显示摄氏度符号（OLED字库若支持可直接用'℃'）

        // 4. 退出透传并断开连接
        Wifi_Quit();

        Wifi_GetTime();
        ParseTimestamp();     // 解析时间戳到g_timestamp
        struct tm *tm_info = localtime(&g_timestamp);
        OLED_ShowNum(4, 1, tm_info->tm_year + 1900, 4);         // 第4行显示时间字符串
        OLED_ShowNum(4, 5, tm_info->tm_mon + 1, 2);
        OLED_ShowNum(4, 7, tm_info->tm_mday, 2);
        OLED_ShowNum(4, 9, tm_info->tm_hour+8, 2);
        OLED_ShowNum(4, 11, tm_info->tm_min, 2);
        OLED_ShowNum(4, 13, tm_info->tm_sec, 2);
        delay_ms(30000);
    }
}
#endif	

#if ST7789TEST
#include "stm32f10x.h"                  // Device header
#include "Delay.h"//延迟函数
#include "ST7789.h"
#include "Font.h"
#include "image.h"
int main(void)
{ 
	ST7789_Init();						   //初始化tft屏幕驱动
	open_backlight();
	TFT_full_color(BLACK);
	//日期
	TFT_display_en16_32(2,0+0*16,0,RED, GREEN);
	TFT_display_en16_32(0,0+1*16,0,RED, GREEN);
	TFT_display_en16_32(2,0+2*16,0,RED, GREEN);
	TFT_display_en16_32(5,0+3*16,0,RED, GREEN);
	TFT_display_16_32(font_heng_16x32,0+4*16,0,RED, GREEN);
	TFT_display_en16_32(1,0+5*16,0,RED, GREEN);
	TFT_display_en16_32(1,0+6*16,0,RED, GREEN);
	TFT_display_16_32(font_heng_16x32,0+7*16,0,RED, GREEN);
	TFT_display_en16_32(1,0+8*16,0,RED, GREEN);
	TFT_display_en16_32(3,0+9*16,0,RED, GREEN);
	//城市
	TFT_display_32_32(font_shen_32x32,0+11*16,0,RED, GREEN);
	TFT_display_32_32(font_zhen_32x32,0+13*16,0,RED, GREEN);
	//天气
	TFT_display_image(duoyun_map, 20, 40);
	//温度
	TFT_display_en16_32(2,0+8*16,40,RED, GREEN);
	TFT_display_en16_32(5,0+9*16,40,RED, GREEN);
	TFT_display_32_32(font_temper_32x32,0+10*16,40,RED, GREEN);
	
	//TFT_display_16_32(font_maohao_16x32,20+3*30,200,RED, GREEN);
	while(1)
	{
		
	}
}

#endif

#if WIFI_ST7789TEST
#include "stm32f10x.h"                  // Device header
#include "ST7789.h"
#include "Font.h"
#include "image.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Delay.h"
#include "sys.h"
#include "usart.h"
#include "usart2.h"
#include "wifi.h"
#include "OLED.h"
#include "rtc.h"
#include "timer.h"

extern time_t g_timestamp;
extern char g_city[32];
extern char g_weather[32];
extern char g_temperature[8];
char time_str[30];  // 存储转换后的时间字符串

int main(void)
{ 
    // wifi初始化
    wifi_GPIO_Init();
    OLED_Init();
    uart_init(9600);
    USART2_Init(115200);  // 初始化ESP8266通信串口
    Wifi_Init();     // 初始化WiFi（假设已包含网络连接逻辑）
    OLED_Clear();
    OLED_ShowString(1, 1, "Initializing...");
    delay_ms(1000);
    //ST7789初始化
	ST7789_Init();						   //初始化tft屏幕驱动
	open_backlight();
	TFT_full_color(BLACK);
	
	//城市
	TFT_display_32_32(font_shen_32x32,0+11*16,0,RED, GREEN);
	TFT_display_32_32(font_zhen_32x32,0+13*16,0,RED, GREEN);
	//天气
	TFT_display_image(duoyun_map, 20, 40);
 
    // 1. 获取天气数据
    Wifi_GetWeather();  // 发送AT命令获取天气数据
    delay_ms(1000);       // 等待数据接收完成

    // 2. 解析数据
    ParseCity();
    ParseWeather();
    ParseTemperature();

    //3. 显示温度
	TFT_display_en16_32(g_temperature[0]-'0',0+8*16,40,RED, GREEN);
	TFT_display_en16_32(g_temperature[1]-'0',0+9*16,40,RED, GREEN);
	TFT_display_32_32(font_temper_32x32,0+10*16,40,RED, GREEN);
	
	TFT_display_en_string8_16(ASCII8x16_Table,g_weather,30,90,RED, BLACK);
	// 显示天气（第2行，第1列）
    OLED_ShowString(2, 1, "Weather:");
    OLED_ShowString(2, 9, g_weather);  // 从第9列开始显示天气
    //oled
    // 显示温度（第3行，第1列），拼接℃单位
    OLED_ShowString(3, 1, "Temp:");
    OLED_ShowString(3, 6, g_temperature);
    OLED_ShowString(3, 9, "C");  // 显示摄氏度符号（OLED字库若支持可直接用'℃'）
	
	// 退出透传并断开连接
    Wifi_Quit();
    
	MyRTC_Init();		//RTC初始化
    while(1) {
       	//4. 显示时间
		Wifi_GetTime();
		ParseTimestamp();     // 解析时间戳到g_timestamp
		struct tm *tm_info = localtime(&g_timestamp);
		
		//日期
		TFT_display_en16_32((tm_info->tm_year+1900)/1000,0+0*16,0,RED, GREEN);
		TFT_display_en16_32((tm_info->tm_year+1900)/100%10,0+1*16,0,RED, GREEN);
		TFT_display_en16_32((tm_info->tm_year+1900)/10%10,0+2*16,0,RED, GREEN);
		TFT_display_en16_32((tm_info->tm_year+1900)%10,0+3*16,0,RED, GREEN);
		TFT_display_16_32(font_heng_16x32,0+4*16,0,RED, GREEN);
		TFT_display_en16_32((tm_info->tm_mon+1)/10,0+5*16,0,RED, GREEN);
		TFT_display_en16_32((tm_info->tm_mon+1)%10,0+6*16,0,RED, GREEN);
		TFT_display_16_32(font_heng_16x32,0+7*16,0,RED, GREEN);
		TFT_display_en16_32((tm_info->tm_mday)/10,0+8*16,0,RED, GREEN);
		TFT_display_en16_32((tm_info->tm_mday)%10,0+9*16,0,RED, GREEN);
		rtc_date_t date;
		//OLED
		OLED_ShowNum(4, 1, tm_info->tm_year + 1900, 4);         // 第4行显示时间字符串
		OLED_ShowNum(4, 5, tm_info->tm_mon + 1, 2);
		OLED_ShowNum(4, 7, tm_info->tm_mday, 2);
		OLED_ShowNum(4, 9, tm_info->tm_hour+8, 2);
		OLED_ShowNum(4, 11, tm_info->tm_min, 2);
		OLED_ShowNum(4, 13, tm_info->tm_sec, 2);

		//rtc

		rtc_set_timestamp(g_timestamp + 8 * 60 * 60);
		rtc_get_date(&date);
		//ST7789 时间
		TFT_display_en16_32((date.hour)/10,0+1*16,150,RED, GREEN);
		TFT_display_en16_32((date.hour)%10,0+2*16,150,RED, GREEN);
		TFT_display_16_32(font_maohao_16x32,0+3*16,150,RED, GREEN);
		TFT_display_en16_32((date.minute)/10,0+4*16,150,RED, GREEN);
		TFT_display_en16_32((date.minute)%10,0+5*16,150,RED, GREEN);
		TFT_display_16_32(font_maohao_16x32,0+6*16,150,RED, GREEN);
		TFT_display_en16_32((date.second)/10,0+7*16,150,RED, GREEN);
		TFT_display_en16_32((date.second)%10,0+8*16,150,RED, GREEN);
		
		delay_s(10);
    }
}

#endif
