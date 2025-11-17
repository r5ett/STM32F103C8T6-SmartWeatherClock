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
#include "rtc.h"
#include "timer.h"

//外部变量：时间戳，城市名，天气，温度
extern time_t g_timestamp;
extern char g_city[32];
extern char g_weather[32];
extern char g_temperature[8];
char time_str[30];  // 存储转换后的时间字符串

/* 运行计数器，每1s自增1 */
static volatile uint32_t runms;

/**
 * @brief 1ms定时器回调，提供毫秒计数值给主程序使用
 * 计数器超过1天就重置
 *
 */
static void timer_elapsed_callback(void)
{
    runms++;
    /* 24小时 x 60分钟 x 60秒 x 1000毫秒 */
    if (runms > 24 * 60 * 60 * 1000)
    {
        /* 计数器归零 */
        runms = 0;
    }
}

int main(void)
{ 
    // 初始化
    wifi_GPIO_Init();
    uart_init(9600);
    USART2_Init(115200);  // 初始化ESP8266通信串口
    Wifi_Init();     // 初始化WiFi（假设已包含网络连接逻辑）
	ST7789_Init();						   //初始化tft屏幕驱动
	open_backlight();
	TFT_full_color(BLACK);
	MyRTC_Init();		//RTC初始化
    /* 定时器2配置为1ms周期触发，即每1ms执行一次timer_elapsed_callback回调函数 */
    timer_init(1000);
    timer_elapsed_register(timer_elapsed_callback);
    timer_start();

    runms = 0;  /* 复位runms，第一次开机应当所有内容都显示出来 */
    uint32_t last_runms = runms;    /* 当前的runms的值，避免在显示第一个内容的时候，runms更新导致后面的内容无法显示 */
    bool weather_ok = false;    /* 天气获取成功标志位，成功读取天气后，10分钟再次更新，否则下一个循环再次尝试获取天气 */
    bool sntp_ok = false;   /* 与天气同理 */
    struct tm *tm_info = NULL;
    rtc_date_t date;

    while(1) {
        /* 每1ms才执行一次后面的代码，当定时器回调执行之后，runms++，与last_runms不匹配，continue就不执行 */
        if (runms == last_runms)
        {
            continue;
        }
        /* 更新last_runms值 */
        last_runms = runms;

        /* 更新天气信息 首次/失败重试 + 每10分钟定时校准*/
        if (!weather_ok || last_runms % (10 * 60 * 1000) == 0){
            weather_ok = true;
            // 1. 获取天气数据
            Wifi_GetWeather();  // 发送AT命令获取天气数据

            // 2. 解析数据
            ParseCity();
            ParseWeather();
            ParseTemperature();

            /* 根据城市内容，选择不同的字模显示 */
            const uint8_t *citi_first = NULL;
            const uint8_t *citi_last = NULL;
            if (strcmp(g_city, "Shenzhen") == 0) {
                citi_first = font_shen_32x32;
                citi_last = font_zhen_32x32;
            }
            
            TFT_display_32_32(citi_first,0+11*16,0,RED, GREEN);
            TFT_display_32_32(citi_last,0+13*16,0,RED, GREEN);

            /* 根据天气内容，选择不同的图片显示 */
            const uint8_t *img = NULL;
            if (strcmp(g_weather, "Cloudy") == 0) {
                img = duoyun_map;
            } else if (strcmp(g_weather, "Wind") == 0) {
                img = feng_map;
            } else if (strcmp(g_weather, "Clear") == 0) {
                img = qing_map;
            } else if (strcmp(g_weather, "Snow") == 0) {
                img = xue_map;
            } else if (strcmp(g_weather, "Overcast") == 0) {
                img = yin_map;
            } else if (strcmp(g_weather, "Rain") == 0) {
                img = yu_map;
            }

            if (img != NULL) { /* 如果有匹配的天气，则显示天气图标 */
                TFT_display_image(img, 20, 40);
                TFT_display_en16_32(g_temperature[0]-'0',0+8*16,40,RED, GREEN);
                TFT_display_en16_32(g_temperature[1]-'0',0+9*16,40,RED, GREEN);
                TFT_display_32_32(font_temper_32x32,0+10*16,40,RED, GREEN);
                TFT_display_en_string8_16(ASCII8x16_Table,g_weather,30,90,RED, BLACK);
            }

            // 退出透传并断开连接
            Wifi_Quit();
        }

        /* 联网同步时间 首次/失败重试 + 每1小时定时校准*/
        if (!sntp_ok || last_runms % (60 * 60 * 1000) == 0)
        {
            sntp_ok = true;
            /* esp8266从sntp服务器读取当前时间，并校准stm32f1的rtc时间 */
            Wifi_GetTime();
            ParseTimestamp();     // 解析时间戳到g_timestamp
            tm_info = localtime(&g_timestamp);
            rtc_set_timestamp(g_timestamp + 8 * 60 * 60);
        }

        /* 更新LCD时间信息，每100ms更新一次lcd上时间内容 */
        if (last_runms % 100 == 0){
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
        }
    }
}
