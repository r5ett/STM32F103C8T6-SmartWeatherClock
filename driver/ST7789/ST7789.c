#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "ST7789.h"
#include "string.h"
#include "Font.h"

/*	STM32F103C8T6接线

	软件模拟SPI							默认电平
	------------------------------------------	
	PA4--SCL	|	低电平有效 				1
	------------------------------------------
	PA5--SDA	|	数据在SCL上升沿锁存	  	1	
	------------------------------------------	
	PA6--RESET	|	低电平有效				
	------------------------------------------
	PA7--AO/DC	|	1：参数					1
				|	0：命令
	------------------------------------------	
	PA8--CS		|	低电平使能				
	------------------------------------------				|
	PC14--BL	|	背光	1：
							0：
	------------------------------------------
*/
/*************************************
*@details 打开屏幕背光
*@param[in] void
*@return void
***************************************/
void open_backlight(void)
{
	TFT_BLK_Set();
}

/*************************************
*@details 关闭屏幕背光
*@param[in] void
*@return void
***************************************/
void close_backlight(void)
{
	TFT_BLK_Clr();
}
/*************************************
*@details 写入命名，数据是在SCLK低电平时设置的，
*		随后通过SCLK的上升沿让ST7789显示屏采样该数据
*@param[in] cmd 8位命令
*@return void
***************************************/
void TFT_SEND_CMD(u8 cmd)
{	
	u8 i;			  
	TFT_DC_Clr();//命令模式			
	TFT_CS_Clr();//CS低使能
	for(i=0;i<8;i++)
	{			  
		TFT_SCLK_Clr();//SCL低电平设置SDA
		if(cmd&0x80)
		   TFT_SDIN_Set();
		else 
		   TFT_SDIN_Clr();
		TFT_SCLK_Set();//SCL高电平ST7789采集SDA
		cmd<<=1;   
	}				 		  
	TFT_CS_Set();//CS高失能
	TFT_DC_Set();//数据模式   	  
} 
/*************************************
*@details 写入数据，原理同上
*@param[in] data 8位数据
*@return void
***************************************/
void TFT_SEND_DATA(u8 data)
{	
	u8 i;			  
	TFT_DC_Set();//数据模式			
	TFT_CS_Clr();//低使能
	for(i=0;i<8;i++)
	{			  
		TFT_SCLK_Clr();
		if(data&0x80)
		   TFT_SDIN_Set();
		else 
		   TFT_SDIN_Clr();
		TFT_SCLK_Set();
		data<<=1;   
	}				 		  
	TFT_CS_Set();
//	TFT_DC_Set();   	  
} 
/*************************************
* @brief   LCD_SetWindows
* @details  设置LCD显示窗口,设置完成后就可以连续发
			送颜色数据了，无需再一次一次设置坐标
* @param   startX：窗口起点x轴坐标
* 		   startY：窗口起点y轴坐标
* 		   width： 窗口宽度
* 		   height：窗口高度
* @return void  
*************************************/
void TFT_SetWindows(uint16_t startX,uint16_t startY,uint16_t width,uint16_t height)
{
	//判断是否有偏移
#if TFT_X_OFFSET
	startX += TFT_X_OFFSET;
#endif	
#if TFT_Y_OFFSET
	startY += TFT_Y_OFFSET;
#endif		
	
	TFT_SEND_CMD(0x2A);		//发送设置X轴坐标的命令0x2A
	//参数SC[15:0]	->	设置起始列地址，也就是设置X轴起始坐标
	TFT_SEND_DATA(startX>>8);				//先写高8位
	TFT_SEND_DATA(startX&0x00FF);			//再写低8位
	//参数EC[15:0]	->	设置窗口X轴结束的列地址，因为参数usXwidth是窗口长度，所以要转为列地址再发送
	TFT_SEND_DATA((startX+width-1)>>8);				//先写高8位
	TFT_SEND_DATA((startX+width-1)&0x00FF);			//再写低8位

	TFT_SEND_CMD(0x2B);		//发送设置Y轴坐标的命令0x2B
	//参数SP[15:0]	->	设置起始行地址，也就是设置Y轴起始坐标
	TFT_SEND_DATA(startY>>8);				//先写高8位
	TFT_SEND_DATA(startY&0x00FF);			//再写低8位
	//参数EP[15:0]	->	设置窗口Y轴结束的列地址，因为参数usYheight是窗口高度，所以要转为行地址再发送
	TFT_SEND_DATA((startY+height-1)>>8);				//先写高8位
	TFT_SEND_DATA((startY+height-1)&0x00FF);			//再写低8位
	//它的作用是通知显示屏：接下来发送的数据将被写入到前面通过0x2A和0x2B设定的窗口区域的GRAM(图形内存)中
	TFT_SEND_CMD(0x2C);			//开始往GRAM里写数据
}
/*************************************
*@details 在整个屏幕上填充颜色
*@param[in] void
*@return void
***************************************/
void TFT_full_color(unsigned int color)
{
	unsigned int ROW,column;
	
	TFT_SetWindows(0,0,TFT_SCREEN_WIDTH,TFT_SCREEN_HEIGHT);
	for(ROW=0;ROW<TFT_SCREEN_HEIGHT;ROW++)             //ROW loop
	{ 
		for(column=0;column<TFT_SCREEN_WIDTH;column++) //column loop
		{

			TFT_SEND_DATA(color>>8);
			TFT_SEND_DATA(color);
		}
	}
}
/*************************************
*@details 	显示一个英文字母8*16
*@param[in] address：字符库数据地址
			char_en: 要显示的编码 要加单引号表示
*			startX： X起始坐标
*			startY： Y起始坐标
*			textColor：字体显示颜色
*			backgroundColor:背景色
***************************************/
void TFT_display_en8_16(const uint8_t *address ,const uint8_t char_en ,uint16_t startX,uint16_t startY,uint16_t textColor, uint16_t backgroundColor)
{
	uint16_t column;
	uint8_t tm=0,temp;
	TFT_SetWindows(startX, startY, 8, 16); //设置字符显示区域
	for(column = 0; column < 16; column++)  
	{
		// A的ASCII码是65，空格符ASCII=32
		temp =address[(char_en - 32) * 16 + column ]; //从英文字库数组中，精准定位到当前行字模数据
		for(tm = 0; tm < 8; tm++)
		{
			if(temp&0x01)		//0000 0001
			{
				TFT_SEND_DATA(textColor>>8);
				TFT_SEND_DATA(textColor);
			}
			else 
			{
				TFT_SEND_DATA(backgroundColor>>8);
				TFT_SEND_DATA(backgroundColor);
			}
			temp >>= 1;
		}
	}
}
/*************************************
*@details 	显示24x48大小的数字/符号
*@param[in] index：字符索引（0-'0',1-'1',...,9-'9',10-':',11-' '）
			startX：X起始坐标
			startY：Y起始坐标
			textColor：字体显示颜色
			backgroundColor:背景色
*@return void
***************************************/
void TFT_display_en24_48(uint8_t index, uint16_t startX, uint16_t startY,
                         uint16_t textColor, uint16_t backgroundColor)
{
    uint8_t row;         // 行索引（0~47，共48行）
    uint8_t byte_in_row; // 行内的字节索引（0~2，每行3字节）
    uint8_t bit;         // 字节内的比特索引（0~7，每比特对应1像素）
    uint8_t temp;
    TFT_SetWindows(startX, startY, 24, 48); // 设置正确的显示区域（宽24，高48）
    // 遍历每一行（0~47）
    for (row = 0; row < 48; row++) {
        // 遍历当前行的3个字节（每个字节对应8个像素，共3×8=24像素，匹配宽度）
        for (byte_in_row = 0; byte_in_row < 3; byte_in_row++) {
            temp = font_time_24x48_data[index][row * 3 + byte_in_row]; // 获取当前行的当前字节
            // 遍历字节内的8个比特（每个比特对应1个像素），从高位到低位（MSB first）
            for (bit = 0; bit < 8; bit++) {
                if (temp & 0x80) { // 最高位为1，显示文字色
                    TFT_SEND_DATA(textColor >> 8);
                    TFT_SEND_DATA(textColor);
                } else { // 最高位为0，显示背景色
                    TFT_SEND_DATA(backgroundColor >> 8);
                    TFT_SEND_DATA(backgroundColor);
                }
                temp <<= 1; // 处理下一个比特
            }
        }
    }
}
/*************************************
*@details 	显示16x32大小的数字/符号
*@param[in] index：字符索引（0-'0',1-'1',...,9-'9',10-':',11-' '）
			startX：X起始坐标
			startY：Y起始坐标
			textColor：字体显示颜色
			backgroundColor:背景色
*@return void
***************************************/
void TFT_display_en16_32(uint8_t index, uint16_t startX, uint16_t startY,
                         uint16_t textColor, uint16_t backgroundColor)
{
    uint8_t row;         // 行索引（0~47，共48行）
    uint8_t byte_in_row; // 行内的字节索引（0~2，每行3字节）
    uint8_t bit;         // 字节内的比特索引（0~7，每比特对应1像素）
    uint8_t temp;
    TFT_SetWindows(startX, startY, 16, 32); // 设置正确的显示区域（宽24，高48）
    // 遍历每一行（0~47）
    for (row = 0; row < 32; row++) {
        // 遍历当前行的3个字节（每个字节对应8个像素，共3×8=24像素，匹配宽度）
        for (byte_in_row = 0; byte_in_row < 2; byte_in_row++) {
            temp = font_temper_16x32_data[index][row * 2 + byte_in_row]; // 获取当前行的当前字节
            // 遍历字节内的8个比特（每个比特对应1个像素），从高位到低位（MSB first）
            for (bit = 0; bit < 8; bit++) {
                if (temp & 0x80) { // 最高位为1，显示文字色
                    TFT_SEND_DATA(textColor >> 8);
                    TFT_SEND_DATA(textColor);
                } else { // 最高位为0，显示背景色
                    TFT_SEND_DATA(backgroundColor >> 8);
                    TFT_SEND_DATA(backgroundColor);
                }
                temp <<= 1; // 处理下一个比特
            }
        }
    }
}

void TFT_display_16_32(const uint8_t *address ,uint16_t startX, uint16_t startY,
                         uint16_t textColor, uint16_t backgroundColor)
{
    uint8_t row;         // 行索引（0~47，共48行）
    uint8_t byte_in_row; // 行内的字节索引（0~2，每行3字节）
    uint8_t bit;         // 字节内的比特索引（0~7，每比特对应1像素）
    uint8_t temp;
    TFT_SetWindows(startX, startY, 16, 32); // 设置正确的显示区域（宽24，高48）
    // 遍历每一行（0~47）
    for (row = 0; row < 32; row++) {
        // 遍历当前行的3个字节（每个字节对应8个像素，共3×8=24像素，匹配宽度）
        for (byte_in_row = 0; byte_in_row < 2; byte_in_row++) {
            temp = address[row * 2 + byte_in_row]; // 获取当前行的当前字节
            // 遍历字节内的8个比特（每个比特对应1个像素），从高位到低位（MSB first）
            for (bit = 0; bit < 8; bit++) {
                if (temp & 0x80) { // 最高位为1，显示文字色
                    TFT_SEND_DATA(textColor >> 8);
                    TFT_SEND_DATA(textColor);
                } else { // 最高位为0，显示背景色
                    TFT_SEND_DATA(backgroundColor >> 8);
                    TFT_SEND_DATA(backgroundColor);
                }
                temp <<= 1; // 处理下一个比特
            }
        }
    }
}
void TFT_display_32_32(const uint8_t *address ,uint16_t startX, uint16_t startY,
                         uint16_t textColor, uint16_t backgroundColor)
{
    uint8_t row;         // 行索引（0~47，共48行）
    uint8_t byte_in_row; // 行内的字节索引（0~2，每行3字节）
    uint8_t bit;         // 字节内的比特索引（0~7，每比特对应1像素）
    uint8_t temp;
    TFT_SetWindows(startX, startY, 32, 32); // 设置正确的显示区域（宽24，高48）
    // 遍历每一行（0~47）
    for (row = 0; row < 32; row++) {
        // 遍历当前行的3个字节（每个字节对应8个像素，共3×8=24像素，匹配宽度）
        for (byte_in_row = 0; byte_in_row < 4; byte_in_row++) {
            temp = address[row * 4 + byte_in_row]; // 获取当前行的当前字节
            // 遍历字节内的8个比特（每个比特对应1个像素），从高位到低位（MSB first）
            for (bit = 0; bit < 8; bit++) {
                if (temp & 0x80) { // 最高位为1，显示文字色
                    TFT_SEND_DATA(textColor >> 8);
                    TFT_SEND_DATA(textColor);
                } else { // 最高位为0，显示背景色
                    TFT_SEND_DATA(backgroundColor >> 8);
                    TFT_SEND_DATA(backgroundColor);
                }
                temp <<= 1; // 处理下一个比特
            }
        }
    }
}
/*************************************
*@details 	 显示英文字符串8*16
*@param[in]  address：字符数据
			str:英文字符串
*			startX：X起始坐标
*			startY：Y起始坐标
*			textColor：字体显示颜色
*			backgroundColor:背景色
*@return void
***************************************/
void TFT_display_en_string8_16(const uint8_t *address ,const uint8_t *str ,uint16_t startX,uint16_t startY,uint16_t textColor, uint16_t backgroundColor)
{
	uint16_t i;
	for(i = 0; i < strlen((const char *)str); i++)
	{
		TFT_display_en8_16(address ,str[i] , startX + i*8, startY, textColor, backgroundColor);
	}
}
/*************************************
*@details 	显示16x16的汉字
*@param[in] address:汉字字库地址
*			startX：X起始坐标
*			startY：Y起始坐标
*			textColor：字体显示颜色
*			backgroundColor:背景色
*@return void
***************************************/
void TFT_display_char16_16(const uint8_t *address ,uint16_t startX,uint16_t startY,
							uint16_t textColor, uint16_t backgroundColor)
{
	unsigned int column;
	unsigned char tm=0,temp;

	TFT_SetWindows(startX, startY, 16, 16);
	
	for(column = 0; column < 32; column++)  //一个16X16的中文字符占用32Byte
	{
		temp =* address;
		for(tm = 0; tm < 8; tm++)			//每次发送一Byte
		{
			if(temp&0x01)					//！！！逆序取模，若为1填充文本颜色
			{
				TFT_SEND_DATA(textColor>>8);
				TFT_SEND_DATA(textColor);
			}
			else 							//否则填充背景颜色
			{
				TFT_SEND_DATA(backgroundColor>>8);
				TFT_SEND_DATA(backgroundColor);
			}
			temp >>= 1;						//完成一位后进行右移
		}
		address++;							//传输下一字节
	}
}  
/*************************************
*@details 	显示图片函数
*@param[in] address:图片数据地址
*			startX：X起始坐标
*			startY：Y起始坐标
*@return void
***************************************/
void TFT_display_image(const uint8_t *address, uint16_t startX, uint16_t startY)
{
	uint16_t image_width;//图片宽
	uint16_t image_hight;//图片高
	uint16_t x,y;
	image_width = address[0];
	image_hight = address[1];
	TFT_SetWindows(startX, startY, image_width, image_hight);
	for(y = 0; y < image_hight*2; y++)
	{
		for(x = 0; x < image_width; x++)
		{
			//发送图片数据
			TFT_SEND_DATA(address[y*image_width + x + 2]);// +2跳过前2字节（宽高数据）
		}
	}	
}
//引脚初始化，启动驱动程序
void ST7789_Init(void)
{
	uint8_t lcd_data = 0x00;
	uint16_t start_x = 0;					//x起始坐标
	uint16_t end_x = TFT_SCREEN_WIDTH;		//x终点坐标
	uint16_t start_y = 0;
	uint16_t end_y = TFT_SCREEN_HEIGHT;
	#if TFT_X_OFFSET							//如果x有偏移
		start_x += TFT_X_OFFSET;
		end_x += TFT_X_OFFSET;
	#endif	
	#if TFT_Y_OFFSET							//如果y有偏移
		start_y += TFT_Y_OFFSET;
		end_y += TFT_Y_OFFSET;
	#endif	
	
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_4 | GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将引脚初始化为推挽输出
	GPIO_SetBits(GPIOA, GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_4);//默认高电平
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);					//初始化为推挽输出
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);// 设置中断优先级分组2
								
	//ST7789驱动
	TFT_SCLK_Set();			//特别注意！！
	TFT_RST_Clr();
	delay_ms(1000);
	TFT_RST_Set();
	delay_ms(1000);
    TFT_SEND_CMD(0x11); 		 //Sleep Out
	delay_ms(120);               //delay120ms 
	//-----------------------ST7789V Frame rate setting-----------------//
	//************************************************
	TFT_SEND_CMD(0x3A);        //颜色数据格式RGB565 
	// 假设我这里需要设置为 65K的RGB接口，16位像素；那么参数应该是 0101 0101 ，16进制为(0x55)
	TFT_SEND_DATA(0x55);
	TFT_SEND_CMD(0xC5); 		//VCOM1
	TFT_SEND_DATA(0x1A); //0x1A是VCOM电压的「配置值」
/*		
	(0,0)*********240***********->
	*
	*
	*
	320           240x320
	*
	*
	*
	↓
**/
	//数据显示从上到下、从左到右等
	TFT_SEND_CMD(0x36);                 // 屏幕显示方向设置

#if DISPLAY_BOTTOM_TO_TOP
	lcd_data |= (1<<7);
#else	
	lcd_data |= (0<<7);
#endif
#if DISPLAY_RIGHT_TO_LEFT
	lcd_data |= (1<<6);
#else	
	lcd_data |= (0<<6);
#endif

#if DISPLAY_OVERTURN//不翻转显示
	lcd_data |= (1<<5);//翻转显示
#else
	lcd_data |= (0<<5);	
#endif
	TFT_SEND_DATA(lcd_data);

/*****显示位置，注意，x和y是根据屏幕显示方向来定的，不一定统一**********/
	TFT_SEND_CMD(0x2A); //设置显示区域 x轴起始及结束坐标 
	TFT_SEND_DATA((start_x>>8)&0xff);
	TFT_SEND_DATA(start_x&0xff);
	TFT_SEND_DATA((end_x>>8)&0xff);
	TFT_SEND_DATA(end_x&0xff);

	TFT_SEND_CMD(0x2B); //设置显示区域 Y轴起始及结束坐标
	TFT_SEND_DATA((start_y>>8)&0xff);
	TFT_SEND_DATA(start_y&0xff);
	TFT_SEND_DATA((end_y>>8)&0xff);
	TFT_SEND_DATA(end_y&0xff);
	
	//-------------ST7789V Frame rate setting-----------//
	TFT_SEND_CMD(0xb2);		//Porch Setting
	TFT_SEND_DATA(0x05);
	TFT_SEND_DATA(0x05);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x33);
	TFT_SEND_DATA(0x33);

	TFT_SEND_CMD(0xb7);			//Gate Control
	TFT_SEND_DATA(0x05);			//12.2v   -10.43v
	//--------------ST7789V Power setting---------------//
	TFT_SEND_CMD(0xBB);//VCOM
	TFT_SEND_DATA(0x3F);

	TFT_SEND_CMD(0xC0); //Power control
	TFT_SEND_DATA(0x2c);

	TFT_SEND_CMD(0xC2);		//VDV and VRH Command Enable
	TFT_SEND_DATA(0x01);

	TFT_SEND_CMD(0xC3);			//VRH Set
	TFT_SEND_DATA(0x0F);		//4.3+( vcom+vcom offset+vdv)

	TFT_SEND_CMD(0xC4);			//VDV Set
	TFT_SEND_DATA(0x20);				//0v

	TFT_SEND_CMD(0xC6);				//Frame Rate Control in Normal Mode
	TFT_SEND_DATA(0X01);			//111Hz

	TFT_SEND_CMD(0xd0);				//Power Control 1
	TFT_SEND_DATA(0xa4);
	TFT_SEND_DATA(0xa1);

	TFT_SEND_CMD(0xE8);				//Power Control 1
	TFT_SEND_DATA(0x03);

	TFT_SEND_CMD(0xE9);				//Equalize time control
	TFT_SEND_DATA(0x09);
	TFT_SEND_DATA(0x09);
	TFT_SEND_DATA(0x08);
	//---------------ST7789V gamma setting-------------//
	TFT_SEND_CMD(0xE0); //Set Gamma
	TFT_SEND_DATA(0xD0);
	TFT_SEND_DATA(0x05);
	TFT_SEND_DATA(0x09);
	TFT_SEND_DATA(0x09);
	TFT_SEND_DATA(0x08);
	TFT_SEND_DATA(0x14);
	TFT_SEND_DATA(0x28);
	TFT_SEND_DATA(0x33);
	TFT_SEND_DATA(0x3F);
	TFT_SEND_DATA(0x07);
	TFT_SEND_DATA(0x13);
	TFT_SEND_DATA(0x14);
	TFT_SEND_DATA(0x28);
	TFT_SEND_DATA(0x30);
	 
	TFT_SEND_CMD(0XE1); //Set Gamma
	TFT_SEND_DATA(0xD0);
	TFT_SEND_DATA(0x05);
	TFT_SEND_DATA(0x09);
	TFT_SEND_DATA(0x09);
	TFT_SEND_DATA(0x08);
	TFT_SEND_DATA(0x03);
	TFT_SEND_DATA(0x24);
	TFT_SEND_DATA(0x32);
	TFT_SEND_DATA(0x32);
	TFT_SEND_DATA(0x3B);
	TFT_SEND_DATA(0x14);
	TFT_SEND_DATA(0x13);
	TFT_SEND_DATA(0x28);
	TFT_SEND_DATA(0x2F);

	//TFT_SEND_CMD(0x21); 		//反显
   
	TFT_SEND_CMD(0x29);         //开启显示 
	
}
	
