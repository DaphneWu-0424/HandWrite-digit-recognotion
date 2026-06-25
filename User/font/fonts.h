#ifndef __FONT_H
#define __FONT_H       

#include "stm32f1xx.h"
#include "./font/fonts.h"



/** @defgroup FONTS_Exported_Types
  * @{
  */ 
typedef struct _tFont
{    
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;
  
} sFONT;

extern sFONT Font24x32;
extern sFONT Font16x24;
extern sFONT Font8x16;


/*******************����********** ����ʾ������ʾ���ַ���С ***************************/
#define      WIDTH_CH_CHAR		                16	    //�����ַ����� 
#define      HEIGHT_CH_CHAR		              	16		  //�����ַ��߶� 



#define LINE(x) ((x) * (((sFONT *)LCD_GetFont())->Height))

//LINEYͳһʹ�ú�����ģ�ĸ߶�
#define LINEY(x) ((x) * (WIDTH_CH_CHAR))




//0��ʾʹ��SD����ģ�������ʾFLASH��ģ,����SD����ģ���ļ�ϵͳ���ٶ����ܶࡣ

#define GBKCODE_FLASH 						1

#if GBKCODE_FLASH
	/*ʹ��FLASH��ģ*/
	/*�����ֿ�洢��FLASH����ʼ��ַ*/
	/*FLASH*/
	#define GBKCODE_START_ADDRESS   387*4096


	/*��ȡ�ֿ�ĺ���*/
	//�����ȡ�����ַ���ģ����ĺ�������ucBufferΪ�����ģ��������usCharΪ�����ַ��������룩
	#define      GetGBKCode( ucBuffer, usChar )  GetGBKCode_from_EXFlash( ucBuffer, usChar )  
	int GetGBKCode_from_EXFlash( uint8_t * pBuffer, uint16_t c);

#else
	/*ʹ��SD��ģ*/


	/*SD����ģ·��*/
	#define GBKCODE_FILE_NAME			"0:/Font/GB2312_H1616.FON"


	/*��ȡ�ֿ�ĺ���*/
	//�����ȡ�����ַ���ģ����ĺ�������ucBufferΪ�����ģ��������usCharΪ�����ַ��������룩

	#define GetGBKCode( ucBuffer, usChar )  GetGBKCode_from_sd( ucBuffer, usChar )
	int GetGBKCode_from_sd ( uint8_t * pBuffer, uint16_t c);

#endif



#endif /*end of __FONT_H    */