#ifndef __BSP_XPT2046_LCD_H
#define	__BSP_XPT2046_LCD_H


#include "stm32f1xx.h"


/* XPT2046 �����������ź�ָʾ���Ŷ���(��ʹ���ж�)*/
#define    XPT2046_PENIRQ_GPIO_PORT   GPIOE
#define    XPT2046_PENIRQ_GPIO_PIN    GPIO_PIN_4

//�����ź���Ч��ƽ
#define   XPT2046_PENIRQ_ActiveLevel       0
#define   XPT2046_PENIRQ_Read()        HAL_GPIO_ReadPin ( XPT2046_PENIRQ_GPIO_PORT, XPT2046_PENIRQ_GPIO_PIN )



/*XPT2046 ������ģ��SPI���Ŷ���*/

#define    XPT2046_SPI_CS_PIN		          GPIO_PIN_13
#define    XPT2046_SPI_CS_PORT		        GPIOD

#define	   XPT2046_SPI_CLK_PIN	          GPIO_PIN_0
#define    XPT2046_SPI_CLK_PORT	          GPIOE

#define	   XPT2046_SPI_MOSI_PIN	          GPIO_PIN_2
#define	   XPT2046_SPI_MOSI_PORT	        GPIOE

#define	   XPT2046_SPI_MISO_PIN	          GPIO_PIN_3
#define	   XPT2046_SPI_MISO_PORT	        GPIOE


/* ֱ�Ӳ����Ĵ����ķ�������IO */
#define	digitalHi(p,i)			{p->BSRR=i;}	//����Ϊ�ߵ�ƽ		
#define digitalLo(p,i)			{p->BSRR=(uint32_t)i << 16;}//����͵�ƽ

#define   XPT2046_CS_ENABLE()      digitalHi( XPT2046_SPI_CS_PORT, XPT2046_SPI_CS_PIN )    
#define   XPT2046_CS_DISABLE()     digitalLo ( XPT2046_SPI_CS_PORT, XPT2046_SPI_CS_PIN )  

#define   XPT2046_CLK_HIGH()       digitalHi ( XPT2046_SPI_CLK_PORT, XPT2046_SPI_CLK_PIN )    
#define   XPT2046_CLK_LOW()        digitalLo ( XPT2046_SPI_CLK_PORT, XPT2046_SPI_CLK_PIN ) 

#define   XPT2046_MOSI_1()         digitalHi ( XPT2046_SPI_MOSI_PORT, XPT2046_SPI_MOSI_PIN ) 
#define   XPT2046_MOSI_0()         digitalLo ( XPT2046_SPI_MOSI_PORT, XPT2046_SPI_MOSI_PIN )

#define   XPT2046_MISO()           HAL_GPIO_ReadPin ( XPT2046_SPI_MISO_PORT, XPT2046_SPI_MISO_PIN )



/******************************* XPT2046 �������������� ***************************/
//У׼������ʱ���������ADֵ������� 
#define             XPT2046_THRESHOLD_CalDiff                    2               

#define	            XPT2046_CHANNEL_X 	                          0x90 	          //ͨ��Y+��ѡ�������	
#define	            XPT2046_CHANNEL_Y 	                          0xd0	          //ͨ��X+��ѡ�������

//��������д��FLASH��ı�־
#define							FLASH_TOUCH_PARA_FLAG_VALUE					0xA5

//������־д��FLASH��ĵ�ַ
#define 							FLASH_TOUCH_PARA_FLAG_ADDR						(1*1024)

//��������д��FLASH��ĵ�ַ
#define 							FLASH_TOUCH_PARA_ADDR									(2*1024)


/*��Ϣ���*/
#define XPT2046_DEBUG_ON         0

#define XPT2046_INFO(fmt,arg...)           printf("<<-XPT2046-INFO->> "fmt"\n",##arg)
#define XPT2046_ERROR(fmt,arg...)          printf("<<-XPT2046-ERROR->> "fmt"\n",##arg)
#define XPT2046_DEBUG(fmt,arg...)          do{\
                                          if(XPT2046_DEBUG_ON)\
                                          printf("<<-XPT2046-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                          }while(0)

/******************************* ���� XPT2046 ��ص��������� ***************************/
typedef	struct          //Һ������ṹ�� 
{		
	/*����ֵ��ʾ��������*/
   int16_t x;			//��¼���µĴ�������ֵ
   int16_t y; 
	
	/*���ڼ�¼��������ʱ(����)����һ�δ���λ��*/
	 int16_t pre_x;		
   int16_t pre_y;
	
} strType_XPT2046_Coordinate;   


typedef struct         //У׼���ӽṹ�� 
{
	 float An,  		 //ע:sizeof(long double) = 8
					Bn,     
					Cn,   
					Dn,    
					En,    
					Fn,     
					Divider;
	
} strType_XPT2046_Calibration;


typedef struct         //У׼ϵ���ṹ�壨����ʹ�ã�
{
	 float dX_X,  			 
					dX_Y,     
					dX,   
					dY_X,    
					dY_Y,    
					dY;

} strType_XPT2046_TouchPara;

/******����״̬�����******/
typedef enum
{
	XPT2046_STATE_RELEASE  = 0,	//�����ͷ�
	XPT2046_STATE_WAITING,			//��������
	XPT2046_STATE_PRESSED,			//��������
}enumTouchState	;

#define TOUCH_PRESSED 				1
#define TOUCH_NOT_PRESSED			0

//����������ֵ
#define DURIATION_TIME				2

/******************************* ���� XPT2046 ��ص��ⲿȫ�ֱ��� ***************************/
extern volatile uint8_t               ucXPT2046_TouchFlag;

extern strType_XPT2046_TouchPara      strXPT2046_TouchPara[];



/******************************** XPT2046 �������������� **********************************/
void XPT2046_Init( void );
uint8_t XPT2046_Touch_Calibrate( uint8_t LCD_Mode );
uint8_t XPT2046_TouchDetect(void);
uint8_t XPT2046_Get_TouchedPoint( strType_XPT2046_Coordinate * displayPtr, strType_XPT2046_TouchPara * para );
void XPT2046_TouchDown(strType_XPT2046_Coordinate * touch);
void XPT2046_TouchUp(strType_XPT2046_Coordinate * touch);
void XPT2046_TouchEvenHandler(void );
void Calibrate_or_Get_TouchParaWithFlash(uint8_t LCD_Mode,uint8_t forceCal);

#endif /* __BSP_TOUCH_H */
