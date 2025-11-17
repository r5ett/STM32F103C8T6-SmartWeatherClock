#include "stm32f10x.h"                  // Device header
#include "wifi.h"
#include "delay.h"
#include "usart.h"
#include "usart2.h"
#include "string.h"
#include "time.h"     // 用于 time_t 类型
#include "stdlib.h"     // 用于atoi或strtoul

/*
			ESP01s       STM32
			 3V3----------3.3V
			 GND----------GND
			 RX-----------PA2
			 TX-----------PA3
			 RST----------PA4
*/

//第一步、wifi模块上电先重启一下

void wifi_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;                     
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA , ENABLE);  
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;                
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;        
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   		 
	GPIO_Init(GPIOA, &GPIO_InitStructure);            		 
	GPIO_SetBits(GPIOA,GPIO_Pin_4);
}

void rst_wifi(void)  //拉低PA4，即RST引脚，先复位
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_4);
	delay_ms(1000);
	GPIO_SetBits(GPIOA,GPIO_Pin_4);
}


//第二步、开始进行AT指令配置


//判断串口二收到的数据是不是前面定义的ack（期待的应答结果）
u8* wifi_check_cmd(u8 *str)
{
	char *strx = 0;
	
	// 检查USART2是否接收到完整数据（最高位标志）
	if(USART2_RX_STA&0X8000)
	{
		
		 // 在接收缓冲区末尾添加结束符'\0'，形成字符串
		USART2_RX_BUF[USART2_RX_STA&0X7FFF] = 0;
		
		// 在接收缓存中搜索目标字符串
		strx = strstr((const char*)USART2_RX_BUF,(const char*)str);
	}
	return (u8*)strx;
}


//此函数发送AT指令并等待特定的响应，返回0表示成功收到响应，1表示超时。
//放一个命令函数在这
//cmd：发送的AT指令
//ack：期待的回答
//time：等待时间(单位10ms)
//返回值：0、发送成功。 1、发送失败
u8 wifi_send_cmd(u8 *cmd,u8 *ack,u16 time)
{
	u8 res = 0;
	USART2_RX_STA = 0;
	u2_printf("%s",cmd);/* 发送cmd命令 */
	if(time)
	{
		while(--time)
		{
			delay_ms(10);
			if(USART2_RX_STA&0X8000) //串口二接收到数据
			{
				//判断接受的数据是不是想要的
				if(wifi_check_cmd(ack))
				{
					break;
				}
				USART2_RX_STA = 0;
			}
		}
		if(time == 0) res = 1;
	}
	return res;
	
}

// 修改后的init_wifi_time函数
// 在获取时间戳的命令后，提取并转换时间
void Wifi_Init(void) {
    // 1 AT, AT测试指令 - 检查与ESP8266的基本通信
    while (wifi_send_cmd("AT\r\n", "OK", 50)) {
        printf("AT响应失败\r\n");
    }
    
    // 2 AT+RST 软复位模块
    while (wifi_send_cmd("AT+RST\r\n", "OK", 50)) {
        printf("AT复位失败\r\n");
    }
    
    // 3 将Wi-Fi模块设置为Station（STA）模式，客户端
    while (wifi_send_cmd("AT+CWMODE=1\r\n", "OK", 50)) {
        printf("STA模式设置失败\r\n");
    }
    
    // 4 重新连接WiFi（确保网络可用，避免init_wifi_zhixin后连接断开）
    while (wifi_send_cmd("AT+CWJAP=\"r5ett\",\"20011212\"\r\n", "OK", 500)) {
        printf("连接WIFI失败\r\n");
    }
}

void Wifi_GetTime(void){
	//配置NTP服务器（阿里云公共NTP，国内延迟低）
    while (wifi_send_cmd("AT+CIPSNTPCFG=1,8,\"ntp.aliyun.com\"\r\n", "OK", 500)) {
        printf("NTP服务器配置失败\r\n");
    }

    //延长NTP同步等待时间（至少10秒，确保时间戳完全更新）
    delay_ms(1000);
	while (wifi_send_cmd("AT+SYSTIMESTAMP?\r\n", "OK", 500)) {
        printf("时间戳失败\r\n");
    }
	delay_s(2);
	while (wifi_send_cmd("AT+SYSTIMESTAMP?\r\n", "OK", 500)) {
        printf("时间戳失败\r\n");
    }
	delay_s(2);
	while (wifi_send_cmd("AT+SYSTIMESTAMP?\r\n", "OK", 500)) {
        printf("时间戳失败\r\n");
    }
}

void Wifi_GetWeather(void){
	//配置NTP服务器（阿里云公共NTP，国内延迟低）
    while (wifi_send_cmd("AT+CIPSTART=\"TCP\",\"api.seniverse.com\",80\r\n", "OK", 500)) {
        printf("服务器配置失败\r\n");
    }

	while (wifi_send_cmd("AT+CIPMODE=1\r\n", "OK", 500)) {
        printf("透传模式失败\r\n");
    }

	while (wifi_send_cmd("AT+CIPSEND\r\n", "OK", 500)) {
        printf("数据传输失败\r\n");
    }

	while (wifi_send_cmd("GET https://api.seniverse.com/v3/weather/now.json?key=SO6RLcWaPVlrrBjr1&location=shenzhen&language=en&unit=c\r\n", "results", 1000)) {
        printf("心知天气失败\r\n");
    }

}

//退出透传并断开连接
void Wifi_Quit(void){
	while (wifi_send_cmd("+++", "+++", 500)) {
       printf("禁止esp8266透传模式失败\r\n");
   	}
	while (wifi_send_cmd("AT+CIPMODE=0\r\n", "OK", 500)) {
       printf("禁止esp8266透传模式失败\r\n");
   	}

	while (wifi_send_cmd("AT+CIPCLOSE\r\n", "OK", 500)) {
       printf("断开服务器连接失败\r\n");
  	}
}

// 全局变量存储解析结果
time_t g_timestamp = 0;  	 // 时间戳
char g_city[32] = {0};       // 城市名称
char g_weather[32] = {0};    // 天气状况
char g_temperature[8] = {0}; // 温度值

void ParseTimestamp(void) {
    char *prefix = "+SYSTIMESTAMP:";
    char *start;
    // 查找时间戳前缀
    start = strstr((char*)USART2_RX_BUF, prefix);
    if (start != NULL) {
        // 跳过前缀，指向数字部分
        start += strlen(prefix);
        // 转换字符串为整数（时间戳）
        g_timestamp = strtoul(start, NULL, 10);
    } else {
        g_timestamp = 0;  // 解析失败
    }
}

/**
 * 解析JSON中的城市名称（如"Shenzhen"）
 * 匹配格式："name":"城市名"
 */
void ParseCity(void) {
    char *prefix = "\"name\":\"";  // JSON中城市名称的前缀
    char *start, *end;
    uint16_t len;

    // 查找前缀位置
    start = strstr((char*)USART2_RX_BUF, prefix);
    if (start == NULL) {
        g_city[0] = '\0';  // 解析失败，清空结果
        return;
    }

    // 跳过前缀，指向城市名开始位置
    start += strlen(prefix);
    
    // 查找值的结束符（下一个双引号）
    end = strchr(start, '"');
    if (end == NULL) {
        g_city[0] = '\0';  // 未找到结束符，解析失败
        return;
    }

    // 计算城市名长度并复制（避免溢出）
    len = end - start;
    if (len >= sizeof(g_city)) {
        len = sizeof(g_city) - 1;  // 预留结束符位置
    }
    strncpy(g_city, start, len);
    g_city[len] = '\0';  // 手动添加字符串结束符
}

/**
 * 解析JSON中的天气状况（如"Cloudy"）
 * 匹配格式："text":"天气状况"
 */
void ParseWeather(void) {
    char *prefix = "\"text\":\"";  // JSON中天气状况的前缀
    char *start, *end;
    uint16_t len;

    start = strstr((char*)USART2_RX_BUF, prefix);
    if (start == NULL) {
        g_weather[0] = '\0';
        return;
    }

    start += strlen(prefix);
    end = strchr(start, '"');
    if (end == NULL) {
        g_weather[0] = '\0';
        return;
    }

    len = end - start;
    if (len >= sizeof(g_weather)) {
        len = sizeof(g_weather) - 1;
    }
    strncpy(g_weather, start, len);
    g_weather[len] = '\0';
}

/**
 * 解析JSON中的温度值（如"22"）
 * 匹配格式："temperature":"温度值"
 */
void ParseTemperature(void) {
    char *prefix = "\"temperature\":\"";  // JSON中温度的前缀
    char *start, *end;
    uint16_t len;

    start = strstr((char*)USART2_RX_BUF, prefix);
    if (start == NULL) {
        g_temperature[0] = '\0';
        return;
    }

    start += strlen(prefix);
    end = strchr(start, '"');
    if (end == NULL) {
        g_temperature[0] = '\0';
        return;
    }

    len = end - start;
    if (len >= sizeof(g_temperature)) {
        len = sizeof(g_temperature) - 1;
    }
    strncpy(g_temperature, start, len);
    g_temperature[len] = '\0';
}
