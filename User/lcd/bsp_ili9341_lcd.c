/**
  ******************************************************************************
  * @file    bsp_ili9341_lcd.c
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   ili9341Һ��������
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� F103 STM32 ������ 
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 

  #include "./lcd/bsp_ili9341_lcd.h"
  #include "./font/fonts.h"	
  
  //����Һ��ɨ�跽����仯��XY���ؿ���
  //����ILI9341_GramScan�������÷���ʱ���Զ�����
  uint16_t LCD_X_LENGTH = ILI9341_LESS_PIXEL;
  uint16_t LCD_Y_LENGTH = ILI9341_MORE_PIXEL;
  
  //Һ����ɨ��ģʽ����������Ҫ���ڷ���ѡ�������ļ������
  //������ѡֵΪ0-7
  //����ILI9341_GramScan�������÷���ʱ���Զ�����
  //LCD�ճ�ʼ�����ʱ��ʹ�ñ�Ĭ��ֵ
  uint8_t LCD_SCAN_MODE = 6;
  
  //����Һ��������ic�� ID
  uint16_t lcdid = LCDID_UNKNOWN;
  
  static sFONT *LCD_Currentfonts = &Font8x16;  //Ӣ������
  static uint16_t CurrentTextColor   = BLACK;//ǰ��ɫ
  static uint16_t CurrentBackColor   = WHITE;//����ɫ
  
  __inline void                 ILI9341_Write_Cmd           ( uint16_t usCmd );
  __inline void                 ILI9341_Write_Data          ( uint16_t usData );
  __inline uint16_t             ILI9341_Read_Data           ( void );
  static void                   ILI9341_Delay               ( __IO uint32_t nCount );
  static void                   ILI9341_REG_Config          ( void );
  static void                   ILI9341_SetCursor           ( uint16_t usX, uint16_t usY );
  static __inline void          ILI9341_FillColor           ( uint32_t ulAmout_Point, uint16_t usColor );
  static uint16_t               ILI9341_Read_PixelData      ( void );
  
  /**
    * @brief  ��ILI9341д������
    * @param  usCmd :Ҫд���������Ĵ�����ַ��
    * @retval ��
    */	
   void ILI9341_Write_Cmd ( uint16_t usCmd )
  {
      * ( __IO uint16_t * ) ( FSMC_Addr_ILI9341_CMD ) = usCmd;
      
  }
  
  
  /**
    * @brief  ��ILI9341д������
    * @param  usData :Ҫд�������
    * @retval ��
    */	
   void ILI9341_Write_Data ( uint16_t usData )
  {
      * ( __IO uint16_t * ) ( FSMC_Addr_ILI9341_DATA ) = usData;
      
  }
  
  
  /**
    * @brief  ��ILI9341��ȡ����
    * @param  ��
    * @retval ��ȡ��������
    */	
   uint16_t ILI9341_Read_Data ( void )
  {
      return ( * ( __IO uint16_t * ) ( FSMC_Addr_ILI9341_DATA ) );
      
  }
  
  
  /**
    * @brief  ���� ILI9341 ����ʱ����
    * @param  nCount ����ʱ����ֵ
    * @retval ��
    */	
  static void ILI9341_Delay ( __IO uint32_t nCount )
  {
    for ( ; nCount != 0; nCount -- );
      
  }
  
  
  /**
   * @brief  ��ʼ��ILI9341�Ĵ���
   * @param  ��
   * @retval ��
   */
  static void ILI9341_REG_Config ( void )
  {
    lcdid = ILI9341_ReadID();
    
    if(lcdid == LCDID_ILI9341)
    {
      /*  Power control B (CFh)  */
      DEBUG_DELAY  ();
      ILI9341_Write_Cmd ( 0xCF  );
      ILI9341_Write_Data ( 0x00  );
      ILI9341_Write_Data ( 0x81  );
      ILI9341_Write_Data ( 0x30  );
      
      /*  Power on sequence control (EDh) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xED );
      ILI9341_Write_Data ( 0x64 );
      ILI9341_Write_Data ( 0x03 );
      ILI9341_Write_Data ( 0x12 );
      ILI9341_Write_Data ( 0x81 );
      
      /*  Driver timing control A (E8h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xE8 );
      ILI9341_Write_Data ( 0x85 );
      ILI9341_Write_Data ( 0x10 );
      ILI9341_Write_Data ( 0x78 );
      
      /*  Power control A (CBh) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xCB );
      ILI9341_Write_Data ( 0x39 );
      ILI9341_Write_Data ( 0x2C );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x34 );
      ILI9341_Write_Data ( 0x02 );
      
      /* Pump ratio control (F7h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xF7 );
      ILI9341_Write_Data ( 0x20 );
      
      /* Driver timing control B */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xEA );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x00 );
      
      /* Frame Rate Control (In Normal Mode/Full Colors) (B1h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xB1 );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x1B );
      
      /*  Display Function Control (B6h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xB6 );
      ILI9341_Write_Data ( 0x0A );
      ILI9341_Write_Data ( 0xA2 );
      
      /* Power Control 1 (C0h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xC0 );
      ILI9341_Write_Data ( 0x35 );
      
      /* Power Control 2 (C1h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xC1 );
      ILI9341_Write_Data ( 0x11 );
      
      /* VCOM Control 1 (C5h) */
      ILI9341_Write_Cmd ( 0xC5 );
      ILI9341_Write_Data ( 0x45 );
      ILI9341_Write_Data ( 0x45 );
      
      /*  VCOM Control 2 (C7h)  */
      ILI9341_Write_Cmd ( 0xC7 );
      ILI9341_Write_Data ( 0xA2 );
      
      /* Enable 3G (F2h) */
      ILI9341_Write_Cmd ( 0xF2 );
      ILI9341_Write_Data ( 0x00 );
      
      /* Gamma Set (26h) */
      ILI9341_Write_Cmd ( 0x26 );
      ILI9341_Write_Data ( 0x01 );
      DEBUG_DELAY ();
      
      /* Positive Gamma Correction */
      ILI9341_Write_Cmd ( 0xE0 ); //Set Gamma
      ILI9341_Write_Data ( 0x0F );
      ILI9341_Write_Data ( 0x26 );
      ILI9341_Write_Data ( 0x24 );
      ILI9341_Write_Data ( 0x0B );
      ILI9341_Write_Data ( 0x0E );
      ILI9341_Write_Data ( 0x09 );
      ILI9341_Write_Data ( 0x54 );
      ILI9341_Write_Data ( 0xA8 );
      ILI9341_Write_Data ( 0x46 );
      ILI9341_Write_Data ( 0x0C );
      ILI9341_Write_Data ( 0x17 );
      ILI9341_Write_Data ( 0x09 );
      ILI9341_Write_Data ( 0x0F );
      ILI9341_Write_Data ( 0x07 );
      ILI9341_Write_Data ( 0x00 );
      
      /* Negative Gamma Correction (E1h) */
      ILI9341_Write_Cmd ( 0XE1 ); //Set Gamma
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x19 );
      ILI9341_Write_Data ( 0x1B );
      ILI9341_Write_Data ( 0x04 );
      ILI9341_Write_Data ( 0x10 );
      ILI9341_Write_Data ( 0x07 );
      ILI9341_Write_Data ( 0x2A );
      ILI9341_Write_Data ( 0x47 );
      ILI9341_Write_Data ( 0x39 );
      ILI9341_Write_Data ( 0x03 );
      ILI9341_Write_Data ( 0x06 );
      ILI9341_Write_Data ( 0x06 );
      ILI9341_Write_Data ( 0x30 );
      ILI9341_Write_Data ( 0x38 );
      ILI9341_Write_Data ( 0x0F );
      
      /* memory access control set */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0x36 ); 	
      ILI9341_Write_Data ( 0xC8 );    /*����  ���Ͻǵ� (���)�����½� (�յ�)ɨ�跽ʽ*/
      DEBUG_DELAY ();
      
      /* column address control set */
      ILI9341_Write_Cmd ( CMD_SetCoordinateX ); 
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0xEF );
      
      /* page address control set */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( CMD_SetCoordinateY ); 
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x01 );
      ILI9341_Write_Data ( 0x3F );
      
      /*  Pixel Format Set (3Ah)  */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0x3a ); 
      ILI9341_Write_Data ( 0x55 );
      
      /* Sleep Out (11h)  */
      ILI9341_Write_Cmd ( 0x11 );	
      ILI9341_Delay ( 0xAFFf<<2 );
      DEBUG_DELAY ();
      
      /* Display ON (29h) */
      ILI9341_Write_Cmd ( 0x29 ); 
      }
    
    else if(lcdid == LCDID_ST7789V)
    {
      /*  Power control B (CFh)  */
      DEBUG_DELAY  ();
      ILI9341_Write_Cmd ( 0xCF  );
      ILI9341_Write_Data ( 0x00  );
      ILI9341_Write_Data ( 0xC1  );
      ILI9341_Write_Data ( 0x30  );
      
      /*  Power on sequence control (EDh) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xED );
      ILI9341_Write_Data ( 0x64 );
      ILI9341_Write_Data ( 0x03 );
      ILI9341_Write_Data ( 0x12 );
      ILI9341_Write_Data ( 0x81 );
      
      /*  Driver timing control A (E8h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xE8 );
      ILI9341_Write_Data ( 0x85 );
      ILI9341_Write_Data ( 0x10 );
      ILI9341_Write_Data ( 0x78 );
      
      /*  Power control A (CBh) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xCB );
      ILI9341_Write_Data ( 0x39 );
      ILI9341_Write_Data ( 0x2C );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x34 );
      ILI9341_Write_Data ( 0x02 );
      
      /* Pump ratio control (F7h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xF7 );
      ILI9341_Write_Data ( 0x20 );
      
      /* Driver timing control B */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xEA );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x00 );
      
      
      /* Power Control 1 (C0h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xC0 );   //Power control
      ILI9341_Write_Data ( 0x21 );  //VRH[5:0]
      
      /* Power Control 2 (C1h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xC1 );   //Power control
      ILI9341_Write_Data ( 0x11 );  //SAP[2:0];BT[3:0]
      
      /* VCOM Control 1 (C5h) */
      ILI9341_Write_Cmd ( 0xC5 );
      ILI9341_Write_Data ( 0x2D );
      ILI9341_Write_Data ( 0x33 );
      
      /*  VCOM Control 2 (C7h)  */
  //	ILI9341_Write_Cmd ( 0xC7 );
  //	ILI9341_Write_Data ( 0XC0 );
      
      /* memory access control set */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0x36 );   //Memory Access Control
      ILI9341_Write_Data ( 0x00 );  /*����  ���Ͻǵ� (���)�����½� (�յ�)ɨ�跽ʽ*/
      DEBUG_DELAY ();
      
      ILI9341_Write_Cmd(0x3A);   
      ILI9341_Write_Data(0x55); 
      
        /* Frame Rate Control (In Normal Mode/Full Colors) (B1h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xB1 );
      ILI9341_Write_Data ( 0x00 );
      ILI9341_Write_Data ( 0x17 );
      
      /*  Display Function Control (B6h) */
      DEBUG_DELAY ();
      ILI9341_Write_Cmd ( 0xB6 );
      ILI9341_Write_Data ( 0x0A );
      ILI9341_Write_Data ( 0xA2 );
      
      ILI9341_Write_Cmd(0xF6);    			
      ILI9341_Write_Data(0x01); 
      ILI9341_Write_Data(0x30); 
      
      /* Enable 3G (F2h) */
      ILI9341_Write_Cmd ( 0xF2 );
      ILI9341_Write_Data ( 0x00 );
      
      /* Gamma Set (26h) */
      ILI9341_Write_Cmd ( 0x26 );
      ILI9341_Write_Data ( 0x01 );
      DEBUG_DELAY ();
      
      /* Positive Gamma Correction */
      ILI9341_Write_Cmd(0xe0); //Positive gamma
      ILI9341_Write_Data(0xd0);         
      ILI9341_Write_Data(0x00); 
      ILI9341_Write_Data(0x02); 
      ILI9341_Write_Data(0x07); 
      ILI9341_Write_Data(0x0b); 
      ILI9341_Write_Data(0x1a); 
      ILI9341_Write_Data(0x31); 
      ILI9341_Write_Data(0x54); 
      ILI9341_Write_Data(0x40); 
      ILI9341_Write_Data(0x29); 
      ILI9341_Write_Data(0x12); 
      ILI9341_Write_Data(0x12); 
      ILI9341_Write_Data(0x12); 
      ILI9341_Write_Data(0x17);
  
      /* Negative Gamma Correction (E1h) */
      ILI9341_Write_Cmd(0xe1); //Negative gamma
      ILI9341_Write_Data(0xd0); 
      ILI9341_Write_Data(0x00); 
      ILI9341_Write_Data(0x02); 
      ILI9341_Write_Data(0x07); 
      ILI9341_Write_Data(0x05); 
      ILI9341_Write_Data(0x25); 
      ILI9341_Write_Data(0x2d); 
      ILI9341_Write_Data(0x44); 
      ILI9341_Write_Data(0x45); 
      ILI9341_Write_Data(0x1c); 
      ILI9341_Write_Data(0x18); 
      ILI9341_Write_Data(0x16); 
      ILI9341_Write_Data(0x1c); 
      ILI9341_Write_Data(0x1d); 
    
      
  //	/* column address control set */
  //	ILI9341_Write_Cmd ( CMD_SetCoordinateX ); 
  //	ILI9341_Write_Data ( 0x00 );
  //	ILI9341_Write_Data ( 0x00 );
  //	ILI9341_Write_Data ( 0x00 );
  //	ILI9341_Write_Data ( 0xEF );
  //	
  //	/* page address control set */
  //	DEBUG_DELAY ();
  //	ILI9341_Write_Cmd ( CMD_SetCoordinateY ); 
  //	ILI9341_Write_Data ( 0x00 );
  //	ILI9341_Write_Data ( 0x00 );
  //	ILI9341_Write_Data ( 0x01 );
  //	ILI9341_Write_Data ( 0x3F );
      
      
      /* Sleep Out (11h)  */
      ILI9341_Write_Cmd ( 0x11 );	  //Exit Sleep
      ILI9341_Delay ( 0xAFFf<<2 );
      DEBUG_DELAY ();
      
      /* Display ON (29h) */
      ILI9341_Write_Cmd ( 0x29 );   //Display on
      
      ILI9341_Write_Cmd(0x2c);
    }
  }
  
  
  /**
   * @brief  ILI9341��ʼ�����������Ҫ�õ�lcd��һ��Ҫ�����������
   * @param  ��
   * @retval ��
   */
  void ILI9341_Init ( void )
  {
      ILI9341_BackLed_Control ( ENABLE );      //����LCD�����
      ILI9341_Rst ();
      ILI9341_REG_Config ();
      
      //����Ĭ��ɨ�跽������ 6 ģʽΪ�󲿷�Һ�����̵�Ĭ����ʾ����  
      ILI9341_GramScan(LCD_SCAN_MODE);
  }
  
  
  
  /**
   * @brief  ILI9341G����LED����
   * @param  enumState �������Ƿ�ʹ�ܱ���LED
    *   �ò���Ϊ����ֵ֮һ��
    *     @arg ENABLE :ʹ�ܱ���LED
    *     @arg DISABLE :���ñ���LED
   * @retval ��
   */
  void  ILI9341_BackLed_Control ( FunctionalState enumState )
  {
      if ( enumState )
    {
      digitalL( GPIOD, GPIO_PIN_12);	
    }
      else
    {
      digitalH( GPIOD, GPIO_PIN_12);
    }		
  }
  
  
  /**
   * @brief  ��ȡLCD����оƬID�����������ڲ��Եײ�Ķ�д����
   * @param  ��
   * @retval ����ʱ����ֵΪLCD����оƬID: LCDID_ILI9341/LCDID_ST7789V
   *         ���򷵻�: LCDID_UNKNOWN
   */
  uint16_t ILI9341_ReadID(void)
  {
      uint16_t id = 0;
      
      ILI9341_Write_Cmd(0x04);
      ILI9341_Read_Data();
      ILI9341_Read_Data();
      id = ILI9341_Read_Data();
      id <<= 8;
      id |= ILI9341_Read_Data();
      
    if(id == LCDID_ST7789V)
    {
      return id;
    }
    else
    {
      ILI9341_Write_Cmd(0xD3);
      ILI9341_Read_Data();
      ILI9341_Read_Data();
      id = ILI9341_Read_Data();
      id <<= 8;
      id |= ILI9341_Read_Data();
      if(id == LCDID_ILI9341)
      {
        return id;
      }
    }
    
      return LCDID_UNKNOWN;
  }
  
  
  /**
   * @brief  ILI9341G ������λ
   * @param  ��
   * @retval ��
   */
  void ILI9341_Rst( void )
  {			
      digitalL( GPIOE,GPIO_PIN_1);	 //�͵�ƽ��λ
  
      ILI9341_Delay ( 0xAFF ); 						   
  
      digitalH( GPIOE,GPIO_PIN_1);		 	 
  
      ILI9341_Delay ( 0xAFF ); 	 	
      
  }
  
  
  /**
   * @brief  ����ILI9341��GRAM��ɨ�跽�� 
   * @param  ucOption ��ѡ��GRAM��ɨ�跽�� 
   *     @arg 0-7 :������ѡֵΪ0-7��˸�����
   *
   *	����������0��3��5��6 ģʽ�ʺϴ���������ʾ���֣�
   *				���Ƽ�ʹ������ģʽ��ʾ����	����ģʽ��ʾ���ֻ��о���Ч��			
   *		
   *	����0��2��4��6 ģʽ��X��������Ϊ240��Y��������Ϊ320
   *	����1��3��5��7 ģʽ��X��������Ϊ320��Y��������Ϊ240
   *
   *	���� 6 ģʽΪ�󲿷�Һ�����̵�Ĭ����ʾ����
   *	���� 3 ģʽΪ����ͷ����ʹ�õķ���
   *	���� 0 ģʽΪBMPͼƬ��ʾ����ʹ�õķ���
   *
   * @retval ��
   * @note  ����ͼ����A��ʾ���ϣ�V��ʾ���£�<��ʾ����>��ʾ����
                      X��ʾX�ᣬY��ʾY��
  
  ------------------------------------------------------------
  ģʽ0��				.		ģʽ1��		.	ģʽ2��			.	ģʽ3��					
                      A		.					A		.		A					.		A									
                      |		.					|		.		|					.		|							
                      Y		.					X		.		Y					.		X					
                      0		.					1		.		2					.		3					
      <--- X0 o		.	<----Y1	o		.		o 2X--->  .		o 3Y--->	
  ------------------------------------------------------------	
  ģʽ4��				.	ģʽ5��			.	ģʽ6��			.	ģʽ7��					
      <--- X4 o		.	<--- Y5 o		.		o 6X--->  .		o 7Y--->	
                      4		.					5		.		6					.		7	
                      Y		.					X		.		Y					.		X						
                      |		.					|		.		|					.		|							
                      V		.					V		.		V					.		V		
  ---------------------------------------------------------				
                                               LCD��ʾ��
                                  |-----------------|
                                  |			Ұ��Logo		|
                                  |									|
                                  |									|
                                  |									|
                                  |									|
                                  |									|
                                  |									|
                                  |									|
                                  |									|
                                  |-----------------|
                                  ��Ļ���棨��240����320��
  
   *******************************************************/
  void ILI9341_GramScan ( uint8_t ucOption )
  {	
      //������飬ֻ������0-7
      if(ucOption >7 )
          return;
      
      //����ģʽ����LCD_SCAN_MODE��ֵ����Ҫ���ڴ�����ѡ��������
      LCD_SCAN_MODE = ucOption;
      
      //����ģʽ����XY��������ؿ���
      if(ucOption%2 == 0)	
      {
          //0 2 4 6ģʽ��X�������ؿ���Ϊ240��Y����Ϊ320
          LCD_X_LENGTH = ILI9341_LESS_PIXEL;
          LCD_Y_LENGTH =	ILI9341_MORE_PIXEL;
      }
      else				
      {
          //1 3 5 7ģʽ��X�������ؿ���Ϊ320��Y����Ϊ240
          LCD_X_LENGTH = ILI9341_MORE_PIXEL;
          LCD_Y_LENGTH =	ILI9341_LESS_PIXEL; 
      }
  
      //0x36��������ĸ�3λ����������GRAMɨ�跽��	
      ILI9341_Write_Cmd ( 0x36 );
    if(lcdid == LCDID_ILI9341)
    {
      ILI9341_Write_Data ( 0x08 |(ucOption<<5));//����ucOption��ֵ����LCD��������0-7��ģʽ
    }
    else if(lcdid == LCDID_ST7789V)
    {
      ILI9341_Write_Data ( 0x00 |(ucOption<<5));//����ucOption��ֵ����LCD��������0-7��ģʽ
    }
      ILI9341_Write_Cmd ( CMD_SetCoordinateX ); 
      ILI9341_Write_Data ( 0x00 );		/* x ��ʼ�����8λ */
      ILI9341_Write_Data ( 0x00 );		/* x ��ʼ�����8λ */
      ILI9341_Write_Data ( ((LCD_X_LENGTH-1)>>8)&0xFF ); /* x ���������8λ */	
      ILI9341_Write_Data ( (LCD_X_LENGTH-1)&0xFF );				/* x ���������8λ */
  
      ILI9341_Write_Cmd ( CMD_SetCoordinateY ); 
      ILI9341_Write_Data ( 0x00 );		/* y ��ʼ�����8λ */
      ILI9341_Write_Data ( 0x00 );		/* y ��ʼ�����8λ */
      ILI9341_Write_Data ( ((LCD_Y_LENGTH-1)>>8)&0xFF );	/* y ���������8λ */	 
      ILI9341_Write_Data ( (LCD_Y_LENGTH-1)&0xFF );				/* y ���������8λ */
  
      /* write gram start */
      ILI9341_Write_Cmd ( CMD_SetPixel );	
  }
  
  
  
  /**
   * @brief  ��ILI9341��ʾ���Ͽ���һ������
   * @param  usX �����ض�ɨ�跽���´��ڵ����X����
   * @param  usY �����ض�ɨ�跽���´��ڵ����Y����
   * @param  usWidth �����ڵĿ���
   * @param  usHeight �����ڵĸ߶�
   * @retval ��
   */
  void ILI9341_OpenWindow ( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight )
  {	
      ILI9341_Write_Cmd ( CMD_SetCoordinateX ); 				 /* ����X���� */
      ILI9341_Write_Data ( usX >> 8  );	 /* �ȸ�8λ��Ȼ���8λ */
      ILI9341_Write_Data ( usX & 0xff  );	 /* ������ʼ��ͽ�����*/
      ILI9341_Write_Data ( ( usX + usWidth - 1 ) >> 8  );
      ILI9341_Write_Data ( ( usX + usWidth - 1 ) & 0xff  );
  
      ILI9341_Write_Cmd ( CMD_SetCoordinateY ); 			     /* ����Y����*/
      ILI9341_Write_Data ( usY >> 8  );
      ILI9341_Write_Data ( usY & 0xff  );
      ILI9341_Write_Data ( ( usY + usHeight - 1 ) >> 8 );
      ILI9341_Write_Data ( ( usY + usHeight - 1) & 0xff );
      
  }
  
  
  /**
   * @brief  �趨ILI9341�Ĺ������
   * @param  usX �����ض�ɨ�跽���¹���X����
   * @param  usY �����ض�ɨ�跽���¹���Y����
   * @retval ��
   */
  static void ILI9341_SetCursor ( uint16_t usX, uint16_t usY )	
  {
      ILI9341_OpenWindow ( usX, usY, 1, 1 );
  }
  
  
  /**
   * @brief  ��ILI9341��ʾ������ĳһ��ɫ������ص�
   * @param  ulAmout_Point ��Ҫ�����ɫ�����ص������Ŀ
   * @param  usColor ����ɫ
   * @retval ��
   */
  static __inline void ILI9341_FillColor ( uint32_t ulAmout_Point, uint16_t usColor )
  {
      uint32_t i = 0;
      
      
      /* memory write */
      ILI9341_Write_Cmd ( CMD_SetPixel );	
          
      for ( i = 0; i < ulAmout_Point; i ++ )
          ILI9341_Write_Data ( usColor );
          
      
  }
  
  
  /**
   * @brief  ��ILI9341��ʾ����ĳһ������ĳ����ɫ��������
   * @param  usX �����ض�ɨ�跽���´��ڵ����X����
   * @param  usY �����ض�ɨ�跽���´��ڵ����Y����
   * @param  usWidth �����ڵĿ���
   * @param  usHeight �����ڵĸ߶�
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_Clear ( uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight )
  {
      ILI9341_OpenWindow ( usX, usY, usWidth, usHeight );
  
      ILI9341_FillColor ( usWidth * usHeight, CurrentBackColor );		
      
  }
  
  
  /**
   * @brief  ��ILI9341��ʾ����ĳһ����ĳ����ɫ�������
   * @param  usX �����ض�ɨ�跽���¸õ��X����
   * @param  usY �����ض�ɨ�跽���¸õ��Y����
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_SetPointPixel ( uint16_t usX, uint16_t usY )	
  {	
      if ( ( usX < LCD_X_LENGTH ) && ( usY < LCD_Y_LENGTH ) )
    {
          ILI9341_SetCursor ( usX, usY );
          
          ILI9341_FillColor ( 1, CurrentTextColor );
      }
      
  }
  
  
  /**
   * @brief  ��ȡ GRAM ��һ����������
   * @param  ��
   * @retval ��������
   */
  static uint16_t ILI9341_Read_PixelData ( void )	
  {	
      uint16_t usRG=0, usB=0 ;
  
      
      ILI9341_Write_Cmd ( 0x2E );   /* ������ */
      //ȥ��ǰһ�ζ�ȡ���
      ILI9341_Read_Data (); 	      /*FIRST READ OUT DUMMY DATA*/
      
      //��ȡ��ɫͨ������ɫͨ����ֵ
      usRG = ILI9341_Read_Data ();  	/*READ OUT RED AND GREEN DATA  */
      usB = ILI9341_Read_Data ();  		/*READ OUT BLUE DATA*/
  
    return ( (usRG&0xF800)| ((usRG<<3)&0x7E0) | (usB>>11) );
  }
  
  
  
  /**
   * @brief  ��ȡ ILI9341 ��ʾ����ĳһ����������������
   * @param  usX �����ض�ɨ�跽���¸õ��X����
   * @param  usY �����ض�ɨ�跽���¸õ��Y����
   * @retval ��������
   */
  uint16_t ILI9341_GetPointPixel ( uint16_t usX, uint16_t usY )
  { 
      uint16_t usPixelData;
  
      
      ILI9341_SetCursor ( usX, usY );
      
      usPixelData = ILI9341_Read_PixelData ();
      
      return usPixelData;
      
  }
  
  
  /**
   * @brief  �� ILI9341 ��ʾ����ʹ�� Bresenham �㷨���߶� 
   * @param  usX1 �����ض�ɨ�跽�����߶ε�һ���˵�X����
   * @param  usY1 �����ض�ɨ�跽�����߶ε�һ���˵�Y����
   * @param  usX2 �����ض�ɨ�跽�����߶ε���һ���˵�X����
   * @param  usY2 �����ض�ɨ�跽�����߶ε���һ���˵�Y����
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DrawLine ( uint16_t usX1, uint16_t usY1, uint16_t usX2, uint16_t usY2 )
  {
      uint16_t us; 
      uint16_t usX_Current, usY_Current;
      
      int32_t lError_X = 0, lError_Y = 0, lDelta_X, lDelta_Y, lDistance; 
      int32_t lIncrease_X, lIncrease_Y; 	
      
      
      lDelta_X = usX2 - usX1; //������������ 
      lDelta_Y = usY2 - usY1; 
      
      usX_Current = usX1; 
      usY_Current = usY1; 
      
      
      if ( lDelta_X > 0 ) 
          lIncrease_X = 1; //���õ������� 
      
      else if ( lDelta_X == 0 ) 
          lIncrease_X = 0;//��ֱ�� 
      
      else 
    { 
      lIncrease_X = -1;
      lDelta_X = - lDelta_X;
    } 
  
      
      if ( lDelta_Y > 0 )
          lIncrease_Y = 1; 
      
      else if ( lDelta_Y == 0 )
          lIncrease_Y = 0;//ˮƽ�� 
      
      else 
    {
      lIncrease_Y = -1;
      lDelta_Y = - lDelta_Y;
    } 
  
      
      if (  lDelta_X > lDelta_Y )
          lDistance = lDelta_X; //ѡȡ�������������� 
      
      else 
          lDistance = lDelta_Y; 
  
      
      for ( us = 0; us <= lDistance + 1; us ++ )//������� 
      {  
          ILI9341_SetPointPixel ( usX_Current, usY_Current );//���� 
          
          lError_X += lDelta_X ; 
          lError_Y += lDelta_Y ; 
          
          if ( lError_X > lDistance ) 
          { 
              lError_X -= lDistance; 
              usX_Current += lIncrease_X; 
          }  
          
          if ( lError_Y > lDistance ) 
          { 
              lError_Y -= lDistance; 
              usY_Current += lIncrease_Y; 
          } 
          
      }  
      
      
  }   
  
  
  /**
   * @brief  �� ILI9341 ��ʾ���ϻ�һ������
   * @param  usX_Start �����ض�ɨ�跽���¾��ε���ʼ��X����
   * @param  usY_Start �����ض�ɨ�跽���¾��ε���ʼ��Y����
   * @param  usWidth�����εĿ��ȣ���λ�����أ�
   * @param  usHeight�����εĸ߶ȣ���λ�����أ�
   * @param  ucFilled ��ѡ���Ƿ����þ���
    *   �ò���Ϊ����ֵ֮һ��
    *     @arg 0 :���ľ���
    *     @arg 1 :ʵ�ľ��� 
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DrawRectangle ( uint16_t usX_Start, uint16_t usY_Start, uint16_t usWidth, uint16_t usHeight, uint8_t ucFilled )
  {
      if ( ucFilled )
      {
          ILI9341_OpenWindow ( usX_Start, usY_Start, usWidth, usHeight );
          ILI9341_FillColor ( usWidth * usHeight ,CurrentTextColor);	
      }
      else
      {
          ILI9341_DrawLine ( usX_Start, usY_Start, usX_Start + usWidth - 1, usY_Start );
          ILI9341_DrawLine ( usX_Start, usY_Start + usHeight - 1, usX_Start + usWidth - 1, usY_Start + usHeight - 1 );
          ILI9341_DrawLine ( usX_Start, usY_Start, usX_Start, usY_Start + usHeight - 1 );
          ILI9341_DrawLine ( usX_Start + usWidth - 1, usY_Start, usX_Start + usWidth - 1, usY_Start + usHeight - 1 );		
      }
  
  }
  
  
  /**
   * @brief  �� ILI9341 ��ʾ����ʹ�� Bresenham �㷨��Բ
   * @param  usX_Center �����ض�ɨ�跽����Բ�ĵ�X����
   * @param  usY_Center �����ض�ɨ�跽����Բ�ĵ�Y����
   * @param  usRadius��Բ�İ뾶����λ�����أ�
   * @param  ucFilled ��ѡ���Ƿ�����Բ
    *   �ò���Ϊ����ֵ֮һ��
    *     @arg 0 :����Բ
    *     @arg 1 :ʵ��Բ
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DrawCircle ( uint16_t usX_Center, uint16_t usY_Center, uint16_t usRadius, uint8_t ucFilled )
  {
      int16_t sCurrentX, sCurrentY;
      int16_t sError;
      
      
      sCurrentX = 0; sCurrentY = usRadius;	  
      
      sError = 3 - ( usRadius << 1 );     //�ж��¸���λ�õı�־
      
      
      while ( sCurrentX <= sCurrentY )
      {
          int16_t sCountY;
          
          
          if ( ucFilled ) 			
              for ( sCountY = sCurrentX; sCountY <= sCurrentY; sCountY ++ ) 
              {                      
                  ILI9341_SetPointPixel ( usX_Center + sCurrentX, usY_Center + sCountY );           //1���о����� 
                  ILI9341_SetPointPixel ( usX_Center - sCurrentX, usY_Center + sCountY );           //2       
                  ILI9341_SetPointPixel ( usX_Center - sCountY,   usY_Center + sCurrentX );           //3
                  ILI9341_SetPointPixel ( usX_Center - sCountY,   usY_Center - sCurrentX );           //4
                  ILI9341_SetPointPixel ( usX_Center - sCurrentX, usY_Center - sCountY );           //5    
          ILI9341_SetPointPixel ( usX_Center + sCurrentX, usY_Center - sCountY );           //6
                  ILI9341_SetPointPixel ( usX_Center + sCountY,   usY_Center - sCurrentX );           //7 	
          ILI9341_SetPointPixel ( usX_Center + sCountY,   usY_Center + sCurrentX );           //0				
              }
          
          else
          {          
              ILI9341_SetPointPixel ( usX_Center + sCurrentX, usY_Center + sCurrentY );             //1���о�����
              ILI9341_SetPointPixel ( usX_Center - sCurrentX, usY_Center + sCurrentY );             //2      
              ILI9341_SetPointPixel ( usX_Center - sCurrentY, usY_Center + sCurrentX );             //3
              ILI9341_SetPointPixel ( usX_Center - sCurrentY, usY_Center - sCurrentX );             //4
              ILI9341_SetPointPixel ( usX_Center - sCurrentX, usY_Center - sCurrentY );             //5       
              ILI9341_SetPointPixel ( usX_Center + sCurrentX, usY_Center - sCurrentY );             //6
              ILI9341_SetPointPixel ( usX_Center + sCurrentY, usY_Center - sCurrentX );             //7 
              ILI9341_SetPointPixel ( usX_Center + sCurrentY, usY_Center + sCurrentX );             //0
      }			
          
          
          sCurrentX ++;
  
          
          if ( sError < 0 ) 
              sError += 4 * sCurrentX + 6;	  
          
          else
          {
              sError += 10 + 4 * ( sCurrentX - sCurrentY );   
              sCurrentY --;
          } 	
          
          
      }
      
      
  }
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾһ��Ӣ���ַ�
   * @param  usX �����ض�ɨ�跽�����ַ�����ʼX����
   * @param  usY �����ض�ɨ�跽���¸õ����ʼY����
   * @param  cChar ��Ҫ��ʾ��Ӣ���ַ�
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DispChar_EN ( uint16_t usX, uint16_t usY, const char cChar )
  {
      uint8_t  byteCount, bitCount,fontLength;	
      uint16_t ucRelativePositon;
      uint8_t *Pfont;
      
      //��ascii���ƫ�ƣ���ģ��������ASCII����ǰ32����ͼ�η��ţ�
      ucRelativePositon = cChar - ' ';
      
      //ÿ����ģ���ֽ���
      fontLength = (LCD_Currentfonts->Width*LCD_Currentfonts->Height)/8;
          
      //��ģ�׵�ַ
      /*ascii���ƫ��ֵ����ÿ����ģ���ֽ����������ģ��ƫ��λ��*/
      Pfont = (uint8_t *)&LCD_Currentfonts->table[ucRelativePositon * fontLength];
      
      //������ʾ����
      ILI9341_OpenWindow ( usX, usY, LCD_Currentfonts->Width, LCD_Currentfonts->Height);
      
      ILI9341_Write_Cmd ( CMD_SetPixel );			
  
      //���ֽڶ�ȡ��ģ����
      //����ǰ��ֱ����������ʾ���ڣ���ʾ���ݻ��Զ�����
      for ( byteCount = 0; byteCount < fontLength; byteCount++ )
      {
              //һλһλ����Ҫ��ʾ����ɫ
              for ( bitCount = 0; bitCount < 8; bitCount++ )
              {
                      if ( Pfont[byteCount] & (0x80>>bitCount) )
                          ILI9341_Write_Data ( CurrentTextColor );			
                      else
                          ILI9341_Write_Data ( CurrentBackColor );
              }	
      }	
  }
  
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾӢ���ַ���
   * @param  line �����ض�ɨ�跽�����ַ�������ʼY����
    *   ��������ʹ�ú�LINE(0)��LINE(1)�ȷ�ʽָ���������꣬
    *   ��LINE(x)����ݵ�ǰѡ�������������Y����ֵ��
      *		��ʾ������ʹ��LINE��ʱ����Ҫ��Ӣ���������ó�Font8x16
   * @param  pStr ��Ҫ��ʾ��Ӣ���ַ������׵�ַ
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DispStringLine_EN (  uint16_t line,  char * pStr )
  {
      uint16_t usX = 0;
      
      while ( * pStr != '\0' )
      {
          if ( ( usX - ILI9341_DispWindow_X_Star + LCD_Currentfonts->Width ) > LCD_X_LENGTH )
          {
              usX = ILI9341_DispWindow_X_Star;
              line += LCD_Currentfonts->Height;
          }
          
          if ( ( line - ILI9341_DispWindow_Y_Star + LCD_Currentfonts->Height ) > LCD_Y_LENGTH )
          {
              usX = ILI9341_DispWindow_X_Star;
              line = ILI9341_DispWindow_Y_Star;
          }
          
          ILI9341_DispChar_EN ( usX, line, * pStr);
          
          pStr ++;
          
          usX += LCD_Currentfonts->Width;
          
      }
      
  }
  
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾӢ���ַ���
   * @param  usX �����ض�ɨ�跽�����ַ�����ʼX����
   * @param  usY �����ض�ɨ�跽�����ַ�����ʼY����
   * @param  pStr ��Ҫ��ʾ��Ӣ���ַ������׵�ַ
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DispString_EN ( 	uint16_t usX ,uint16_t usY,  char * pStr )
  {
      while ( * pStr != '\0' )
      {
          if ( ( usX - ILI9341_DispWindow_X_Star + LCD_Currentfonts->Width ) > LCD_X_LENGTH )
          {
              usX = ILI9341_DispWindow_X_Star;
              usY += LCD_Currentfonts->Height;
          }
          
          if ( ( usY - ILI9341_DispWindow_Y_Star + LCD_Currentfonts->Height ) > LCD_Y_LENGTH )
          {
              usX = ILI9341_DispWindow_X_Star;
              usY = ILI9341_DispWindow_Y_Star;
          }
          
          ILI9341_DispChar_EN ( usX, usY, * pStr);
          
          pStr ++;
          
          usX += LCD_Currentfonts->Width;
          
      }
      
  }
  
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾӢ���ַ���(��Y�᷽��)
   * @param  usX �����ض�ɨ�跽�����ַ�����ʼX����
   * @param  usY �����ض�ɨ�跽�����ַ�����ʼY����
   * @param  pStr ��Ҫ��ʾ��Ӣ���ַ������׵�ַ
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DispString_EN_YDir (	 uint16_t usX,uint16_t usY ,  char * pStr )
  {	
      while ( * pStr != '\0' )
      {
          if ( ( usY - ILI9341_DispWindow_Y_Star + LCD_Currentfonts->Height ) >LCD_Y_LENGTH  )
          {
              usY = ILI9341_DispWindow_Y_Star;
              usX += LCD_Currentfonts->Width;
          }
          
          if ( ( usX - ILI9341_DispWindow_X_Star + LCD_Currentfonts->Width ) >  LCD_X_LENGTH)
          {
              usX = ILI9341_DispWindow_X_Star;
              usY = ILI9341_DispWindow_Y_Star;
          }
          
          ILI9341_DispChar_EN ( usX, usY, * pStr);
          
          pStr ++;
          
          usY += LCD_Currentfonts->Height;		
      }	
  }
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾһ�������ַ�
   * @param  usX �����ض�ɨ�跽�����ַ�����ʼX����
   * @param  usY �����ض�ɨ�跽�����ַ�����ʼY����
   * @param  usChar ��Ҫ��ʾ�������ַ��������룩
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */ 
  void ILI9341_DispChar_CH ( uint16_t usX, uint16_t usY, uint16_t usChar )
  {
      uint8_t rowCount, bitCount;
      uint8_t ucBuffer [ WIDTH_CH_CHAR*HEIGHT_CH_CHAR/8 ];	
    uint16_t usTemp; 	
  
      //������ʾ����
      ILI9341_OpenWindow ( usX, usY, WIDTH_CH_CHAR, HEIGHT_CH_CHAR );
      
      ILI9341_Write_Cmd ( CMD_SetPixel );
      
      //ȡ��ģ����  
    GetGBKCode ( ucBuffer, usChar );	
      
      for ( rowCount = 0; rowCount < HEIGHT_CH_CHAR; rowCount++ )
      {
      /* ȡ�������ֽڵ����ݣ���lcd�ϼ���һ�����ֵ�һ�� */
          usTemp = ucBuffer [ rowCount * 2 ];
          usTemp = ( usTemp << 8 );
          usTemp |= ucBuffer [ rowCount * 2 + 1 ];
          
          for ( bitCount = 0; bitCount < WIDTH_CH_CHAR; bitCount ++ )
          {			
              if ( usTemp & ( 0x8000 >> bitCount ) )  //��λ��ǰ 
                ILI9341_Write_Data ( CurrentTextColor );				
              else
                  ILI9341_Write_Data ( CurrentBackColor );			
          }		
      }
      
  }
  
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾ�����ַ���
   * @param  line �����ض�ɨ�跽�����ַ�������ʼY����
    *   ��������ʹ�ú�LINE(0)��LINE(1)�ȷ�ʽָ���������꣬
    *   ��LINE(x)����ݵ�ǰѡ�������������Y����ֵ��
      *		��ʾ������ʹ��LINE��ʱ����Ҫ��Ӣ���������ó�Font8x16
   * @param  pStr ��Ҫ��ʾ��Ӣ���ַ������׵�ַ
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DispString_CH ( 	uint16_t usX , uint16_t usY, char * pStr )
  {	
      uint16_t usCh;
  
      
      while( * pStr != '\0' )
      {		
          if ( ( usX - ILI9341_DispWindow_X_Star + WIDTH_CH_CHAR ) > LCD_X_LENGTH )
          {
              usX = ILI9341_DispWindow_X_Star;
              usY += HEIGHT_CH_CHAR;
          }
          
          if ( ( usY - ILI9341_DispWindow_Y_Star + HEIGHT_CH_CHAR ) > LCD_Y_LENGTH )
          {
              usX = ILI9341_DispWindow_X_Star;
              usY = ILI9341_DispWindow_Y_Star;
          }	
          
          usCh = * ( uint16_t * ) pStr;	
        usCh = ( usCh << 8 ) + ( usCh >> 8 );
  
          ILI9341_DispChar_CH ( usX, usY, usCh );
          
          usX += WIDTH_CH_CHAR;
          
          pStr += 2;           //һ�����������ֽ� 
  
      }	   
      
  }
  
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾ��Ӣ���ַ���
   * @param  line �����ض�ɨ�跽�����ַ�������ʼY����
    *   ��������ʹ�ú�LINE(0)��LINE(1)�ȷ�ʽָ���������꣬
    *   ��LINE(x)����ݵ�ǰѡ�������������Y����ֵ��
      *		��ʾ������ʹ��LINE��ʱ����Ҫ��Ӣ���������ó�Font8x16
   * @param  pStr ��Ҫ��ʾ���ַ������׵�ַ
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DispStringLine_EN_CH (  uint16_t line, char * pStr )
  {
      uint16_t usCh;
      uint16_t usX = 0;
      
      while( * pStr != '\0' )
      {
          if ( * pStr <= 126 )	           	//Ӣ���ַ�
          {
              if ( ( usX - ILI9341_DispWindow_X_Star + LCD_Currentfonts->Width ) > LCD_X_LENGTH )
              {
                  usX = ILI9341_DispWindow_X_Star;
                  line += LCD_Currentfonts->Height;
              }
              
              if ( ( line - ILI9341_DispWindow_Y_Star + LCD_Currentfonts->Height ) > LCD_Y_LENGTH )
              {
                  usX = ILI9341_DispWindow_X_Star;
                  line = ILI9341_DispWindow_Y_Star;
              }			
          
            ILI9341_DispChar_EN ( usX, line, * pStr );
              
              usX +=  LCD_Currentfonts->Width;
          
            pStr ++;
  
          }
          
          else	                            //�����ַ�
          {
              if ( ( usX - ILI9341_DispWindow_X_Star + WIDTH_CH_CHAR ) > LCD_X_LENGTH )
              {
                  usX = ILI9341_DispWindow_X_Star;
                  line += HEIGHT_CH_CHAR;
              }
              
              if ( ( line - ILI9341_DispWindow_Y_Star + HEIGHT_CH_CHAR ) > LCD_Y_LENGTH )
              {
                  usX = ILI9341_DispWindow_X_Star;
                  line = ILI9341_DispWindow_Y_Star;
              }	
              
              usCh = * ( uint16_t * ) pStr;	
              
              usCh = ( usCh << 8 ) + ( usCh >> 8 );		
  
              ILI9341_DispChar_CH ( usX, line, usCh );
              
              usX += WIDTH_CH_CHAR;
              
              pStr += 2;           //һ�����������ֽ� 
          
      }
          
    }	
  } 
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾ��Ӣ���ַ���
   * @param  usX �����ض�ɨ�跽�����ַ�����ʼX����
   * @param  usY �����ض�ɨ�跽�����ַ�����ʼY����
   * @param  pStr ��Ҫ��ʾ���ַ������׵�ַ
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DispString_EN_CH ( 	uint16_t usX , uint16_t usY, char * pStr )
  {
      uint16_t usCh;
      
      while( * pStr != '\0' )
      {
          if ( * pStr <= 126 )	           	//Ӣ���ַ�
          {
              if ( ( usX - ILI9341_DispWindow_X_Star + LCD_Currentfonts->Width ) > LCD_X_LENGTH )
              {
                  usX = ILI9341_DispWindow_X_Star;
                  usY += LCD_Currentfonts->Height;
              }
              
              if ( ( usY - ILI9341_DispWindow_Y_Star + LCD_Currentfonts->Height ) > LCD_Y_LENGTH )
              {
                  usX = ILI9341_DispWindow_X_Star;
                  usY = ILI9341_DispWindow_Y_Star;
              }			
          
            ILI9341_DispChar_EN ( usX, usY, * pStr );
              
              usX +=  LCD_Currentfonts->Width;
          
            pStr ++;
  
          }
          
          else	                            //�����ַ�
          {
              if ( ( usX - ILI9341_DispWindow_X_Star + WIDTH_CH_CHAR ) > LCD_X_LENGTH )
              {
                  usX = ILI9341_DispWindow_X_Star;
                  usY += HEIGHT_CH_CHAR;
              }
              
              if ( ( usY - ILI9341_DispWindow_Y_Star + HEIGHT_CH_CHAR ) > LCD_Y_LENGTH )
              {
                  usX = ILI9341_DispWindow_X_Star;
                  usY = ILI9341_DispWindow_Y_Star;
              }	
              
              usCh = * ( uint16_t * ) pStr;	
              
              usCh = ( usCh << 8 ) + ( usCh >> 8 );		
  
              ILI9341_DispChar_CH ( usX, usY, usCh );
              
              usX += WIDTH_CH_CHAR;
              
              pStr += 2;           //һ�����������ֽ� 
          
      }
          
    }	
  } 
  
  /**
   * @brief  �� ILI9341 ��ʾ������ʾ��Ӣ���ַ���(��Y�᷽��)
   * @param  usX �����ض�ɨ�跽�����ַ�����ʼX����
   * @param  usY �����ض�ɨ�跽�����ַ�����ʼY����
   * @param  pStr ��Ҫ��ʾ����Ӣ���ַ������׵�ַ
   * @note ��ʹ��LCD_SetBackColor��LCD_SetTextColor��LCD_SetColors����������ɫ
   * @retval ��
   */
  void ILI9341_DispString_EN_CH_YDir (  uint16_t usX,uint16_t usY , char * pStr )
  {
      uint16_t usCh;
      
      while( * pStr != '\0' )
      {			
              //ͳһʹ�ú��ֵĿ��������㻻��
              if ( ( usY - ILI9341_DispWindow_Y_Star + HEIGHT_CH_CHAR ) >LCD_Y_LENGTH  )
              {
                  usY = ILI9341_DispWindow_Y_Star;
                  usX += WIDTH_CH_CHAR;
              }			
              if ( ( usX - ILI9341_DispWindow_X_Star + WIDTH_CH_CHAR ) >  LCD_X_LENGTH)
              {
                  usX = ILI9341_DispWindow_X_Star;
                  usY = ILI9341_DispWindow_Y_Star;
              }
              
          //��ʾ	
          if ( * pStr <= 126 )	           	//Ӣ���ַ�
          {			
              ILI9341_DispChar_EN ( usX, usY, * pStr);
              
              pStr ++;
              
              usY += HEIGHT_CH_CHAR;		
          }
          else	                            //�����ַ�
          {			
              usCh = * ( uint16_t * ) pStr;	
              
              usCh = ( usCh << 8 ) + ( usCh >> 8 );		
  
              ILI9341_DispChar_CH ( usX,usY , usCh );
              
              usY += HEIGHT_CH_CHAR;
              
              pStr += 2;           //һ�����������ֽ� 
          
      }
          
    }	
  } 
  
  /***********************��������****************************/
  #define ZOOMMAXBUFF 16384
  uint8_t zoomBuff[ZOOMMAXBUFF] = {0};	//�������ŵĻ��棬���֧�ֵ�128*128
  uint8_t zoomTempBuff[1024] = {0};
  
  /**
   * @brief  ������ģ�����ź����ģ��1�����ص���8������λ����ʾ
                                          0x01��ʾ�ʼ���0x00��ʾ�հ���
   * @param  in_width ��ԭʼ�ַ�����
   * @param  in_heig ��ԭʼ�ַ��߶�
   * @param  out_width �����ź���ַ�����
   * @param  out_heig�����ź���ַ��߶�
   * @param  in_ptr ���ֿ�����ָ��	ע�⣺1pixel 1bit
   * @param  out_ptr �����ź���ַ����ָ�� ע��: 1pixel 8bit
   *		out_ptrʵ����û������������ĳ���ֱ�������ȫ��ָ��zoomBuff��
   * @param  en_cn ��0ΪӢ�ģ�1Ϊ����
   * @retval ��
   */
  void ILI9341_zoomChar(uint16_t in_width,	//ԭʼ�ַ�����
                                      uint16_t in_heig,		//ԭʼ�ַ��߶�
                                      uint16_t out_width,	//���ź���ַ�����
                                      uint16_t out_heig,	//���ź���ַ��߶�
                                      uint8_t *in_ptr,	//�ֿ�����ָ��	ע�⣺1pixel 1bit
                                      uint8_t *out_ptr, //���ź���ַ����ָ�� ע��: 1pixel 8bit
                                      uint8_t en_cn)		//0ΪӢ�ģ�1Ϊ����	
  {
      uint8_t *pts,*ots;
      //����Դ��ģ��Ŀ����ģ��С���趨����������ӣ�����16��Ϊ�˰Ѹ�������ת�ɶ�������
      unsigned int xrIntFloat_16=(in_width<<16)/out_width+1; 
    unsigned int yrIntFloat_16=(in_heig<<16)/out_heig+1;
      
      unsigned int srcy_16=0;
      unsigned int y,x;
      uint8_t *pSrcLine;
      
      uint16_t byteCount,bitCount;
      
      //�������Ƿ�Ϸ�
      if(in_width >= 32) return;												//�ֿⲻ��������32����
      if(in_width * in_heig == 0) return;	
      if(in_width * in_heig >= 1024 ) return; 					//����������� 32*32
      
      if(out_width * out_heig == 0) return;	
      if(out_width * out_heig >= ZOOMMAXBUFF ) return; //����������� 128*128
      pts = (uint8_t*)&zoomTempBuff;
      
      //Ϊ�������㣬�ֿ��������1 pixel/1bit ӳ�䵽1pixel/8bit
      //0x01��ʾ�ʼ���0x00��ʾ�հ���
      if(en_cn == 0x00)//Ӣ��
      {
          //Ӣ�ĺ������ֿ����±߽粻�ԣ����ڴ˴���������Ҫע��tempBuff��ֹ���
              for(byteCount=0;byteCount<in_heig*in_width/8;byteCount++)	
              {
                  for(bitCount=0;bitCount<8;bitCount++)
                      {						
                          //��Դ��ģ������λӳ�䵽�ֽ�
                          //in_ptr��bitXΪ1����pts�������ֽ�ֵΪ1
                          //in_ptr��bitXΪ0����pts�������ֽ�ֵΪ0
                          *pts++ = (in_ptr[byteCount] & (0x80>>bitCount))?1:0; 
                      }
              }				
      }
      else //����
      {			
              for(byteCount=0;byteCount<in_heig*in_width/8;byteCount++)	
              {
                  for(bitCount=0;bitCount<8;bitCount++)
                      {						
                          //��Դ��ģ������λӳ�䵽�ֽ�
                          //in_ptr��bitXΪ1����pts�������ֽ�ֵΪ1
                          //in_ptr��bitXΪ0����pts�������ֽ�ֵΪ0
                          *pts++ = (in_ptr[byteCount] & (0x80>>bitCount))?1:0; 
                      }
              }		
      }
  
      //zoom����
      pts = (uint8_t*)&zoomTempBuff;	//ӳ����Դ����ָ��
      ots = (uint8_t*)&zoomBuff;	//������ݵ�ָ��
      for (y=0;y<out_heig;y++)	/*�б���*/
      {
                  unsigned int srcx_16=0;
          pSrcLine=pts+in_width*(srcy_16>>16);				
          for (x=0;x<out_width;x++) /*�������ر���*/
          {
              ots[x]=pSrcLine[srcx_16>>16]; //��Դ��ģ���ݸ��Ƶ�Ŀ��ָ����
              srcx_16+=xrIntFloat_16;			//������ƫ��Դ���ص�
          }
          srcy_16+=yrIntFloat_16;				  //������ƫ��Դ���ص�
          ots+=out_width;						
      }
      /*���������ź����ģ����ֱ�Ӵ洢��ȫ��ָ��zoomBuff����*/
      out_ptr = (uint8_t*)&zoomBuff;	//out_ptrû����ȷ�������������ֱ�Ӹĳ���ȫ�ֱ���ָ�룡
      
      /*ʵ�������ʹ��out_ptr����Ҫ������һ�䣡����
          ֻ����Ϊout_ptrû��ʹ�ã��ᵼ��warning��ǿ��֢*/
      out_ptr++; 
  }			
  
  
  /**
   * @brief  �������ź����ģ��ʾ�ַ�
   * @param  Xpos ���ַ���ʾλ��x
   * @param  Ypos ���ַ���ʾλ��y
   * @param  Font_width ���ַ�����
   * @param  Font_Heig���ַ��߶�
   * @param  c ��Ҫ��ʾ����ģ����
   * @param  DrawModel ���Ƿ�ɫ��ʾ 
   * @retval ��
   */
  void ILI9341_DrawChar_Ex(uint16_t usX, //�ַ���ʾλ��x
                                                  uint16_t usY, //�ַ���ʾλ��y
                                                  uint16_t Font_width, //�ַ�����
                                                  uint16_t Font_Height,  //�ַ��߶� 
                                                  uint8_t *c,						//��ģ����
                                                  uint16_t DrawModel)		//�Ƿ�ɫ��ʾ
  {
    uint32_t index = 0, counter = 0;
  
      //������ʾ����
      ILI9341_OpenWindow ( usX, usY, Font_width, Font_Height);
      
      ILI9341_Write_Cmd ( CMD_SetPixel );		
      
      //���ֽڶ�ȡ��ģ����
      //����ǰ��ֱ����������ʾ���ڣ���ʾ���ݻ��Զ�����
      for ( index = 0; index < Font_Height; index++ )
      {
              //һλһλ����Ҫ��ʾ����ɫ
              for ( counter = 0; counter < Font_width; counter++ )
              {
                      //���ź����ģ���ݣ���һ���ֽڱ�ʾһ������λ
                      //�����ֽ�ֵΪ1��ʾ������Ϊ�ʼ�
                      //�����ֽ�ֵΪ0��ʾ������Ϊ����
                      if ( *c++ == DrawModel )
                          ILI9341_Write_Data ( CurrentBackColor );			
                      else
                          ILI9341_Write_Data ( CurrentTextColor );
              }	
      }	
  }
  
  
  /**
   * @brief  �������ź����ģ��ʾ�ַ���
   * @param  Xpos ���ַ���ʾλ��x
   * @param  Ypos ���ַ���ʾλ��y
   * @param  Font_width ���ַ����ȣ�Ӣ���ַ��ڴ˻�����/2��ע��Ϊż��
   * @param  Font_Heig���ַ��߶ȣ�ע��Ϊż��
   * @param  c ��Ҫ��ʾ���ַ���
   * @param  DrawModel ���Ƿ�ɫ��ʾ 
   * @retval ��
   */
  void ILI9341_DisplayStringEx(uint16_t x, 		//�ַ���ʾλ��x
                                                           uint16_t y, 				//�ַ���ʾλ��y
                                                           uint16_t Font_width,	//Ҫ��ʾ��������ȣ�Ӣ���ַ��ڴ˻�����/2��ע��Ϊż��
                                                           uint16_t Font_Height,	//Ҫ��ʾ������߶ȣ�ע��Ϊż��
                                                           uint8_t *ptr,					//��ʾ���ַ�����
                                                           uint16_t DrawModel)  //�Ƿ�ɫ��ʾ
  
  
  
  {
      uint16_t Charwidth = Font_width; //Ĭ��ΪFont_width��Ӣ�Ŀ���Ϊ���Ŀ��ȵ�һ��
      uint8_t *psr = NULL;
      uint8_t Ascii;	//Ӣ��
      uint16_t usCh;  //����
      uint8_t ucBuffer [ WIDTH_CH_CHAR*HEIGHT_CH_CHAR/8 ];	
      
      while ( *ptr != '\0')
      {
              /****��������*****/
              if ( ( x - ILI9341_DispWindow_X_Star + Charwidth ) > LCD_X_LENGTH )
              {
                  x = ILI9341_DispWindow_X_Star;
                  y += Font_Height;
              }
              
              if ( ( y - ILI9341_DispWindow_Y_Star + Font_Height ) > LCD_Y_LENGTH )
              {
                  x = ILI9341_DispWindow_X_Star;
                  y = ILI9341_DispWindow_Y_Star;
              }	
              
          if(*ptr > 0x80) //���������
          {			
              Charwidth = Font_width;
              usCh = * ( uint16_t * ) ptr;				
              usCh = ( usCh << 8 ) + ( usCh >> 8 );
              GetGBKCode ( ucBuffer, usCh );	//ȡ��ģ����
              //������ģ���ݣ�Դ��ģΪ16*16
              ILI9341_zoomChar(WIDTH_CH_CHAR,HEIGHT_CH_CHAR,Charwidth,Font_Height,(uint8_t *)&ucBuffer,psr,1); 
              //��ʾ�����ַ�
              ILI9341_DrawChar_Ex(x,y,Charwidth,Font_Height,(uint8_t*)&zoomBuff,DrawModel);
              x+=Charwidth;
              ptr+=2;
          }
          else
          {
                  Charwidth = Font_width / 2;
                  Ascii = *ptr - 32;
                  //ʹ��16*24����������ģ����
                  ILI9341_zoomChar(16,24,Charwidth,Font_Height,(uint8_t *)&Font16x24.table[Ascii * Font16x24.Height*Font16x24.Width/8],psr,0);
                //��ʾ�����ַ�
                  ILI9341_DrawChar_Ex(x,y,Charwidth,Font_Height,(uint8_t*)&zoomBuff,DrawModel);
                  x+=Charwidth;
                  ptr++;
          }
      }
  }
  
  
  /**
   * @brief  �������ź����ģ��ʾ�ַ���(��Y�᷽��)
   * @param  Xpos ���ַ���ʾλ��x
   * @param  Ypos ���ַ���ʾλ��y
   * @param  Font_width ���ַ����ȣ�Ӣ���ַ��ڴ˻�����/2��ע��Ϊż��
   * @param  Font_Heig���ַ��߶ȣ�ע��Ϊż��
   * @param  c ��Ҫ��ʾ���ַ���
   * @param  DrawModel ���Ƿ�ɫ��ʾ 
   * @retval ��
   */
  void ILI9341_DisplayStringEx_YDir(uint16_t x, 		//�ַ���ʾλ��x
                                                                           uint16_t y, 				//�ַ���ʾλ��y
                                                                           uint16_t Font_width,	//Ҫ��ʾ��������ȣ�Ӣ���ַ��ڴ˻�����/2��ע��Ϊż��
                                                                           uint16_t Font_Height,	//Ҫ��ʾ������߶ȣ�ע��Ϊż��
                                                                           uint8_t *ptr,					//��ʾ���ַ�����
                                                                           uint16_t DrawModel)  //�Ƿ�ɫ��ʾ
  {
      uint16_t Charwidth = Font_width; //Ĭ��ΪFont_width��Ӣ�Ŀ���Ϊ���Ŀ��ȵ�һ��
      uint8_t *psr = NULL;
      uint8_t Ascii;	//Ӣ��
      uint16_t usCh;  //����
      uint8_t ucBuffer [ WIDTH_CH_CHAR*HEIGHT_CH_CHAR/8 ];	
      
      while ( *ptr != '\0')
      {			
              //ͳһʹ�ú��ֵĿ��������㻻��
              if ( ( y - ILI9341_DispWindow_X_Star + Font_width ) > LCD_X_LENGTH )
              {
                  y = ILI9341_DispWindow_X_Star;
                  x += Font_width;
              }
              
              if ( ( x - ILI9341_DispWindow_Y_Star + Font_Height ) > LCD_Y_LENGTH )
              {
                  y = ILI9341_DispWindow_X_Star;
                  x = ILI9341_DispWindow_Y_Star;
              }	
              
          if(*ptr > 0x80) //���������
          {			
              Charwidth = Font_width;
              usCh = * ( uint16_t * ) ptr;				
              usCh = ( usCh << 8 ) + ( usCh >> 8 );
              GetGBKCode ( ucBuffer, usCh );	//ȡ��ģ����
              //������ģ���ݣ�Դ��ģΪ16*16
              ILI9341_zoomChar(WIDTH_CH_CHAR,HEIGHT_CH_CHAR,Charwidth,Font_Height,(uint8_t *)&ucBuffer,psr,1); 
              //��ʾ�����ַ�
              ILI9341_DrawChar_Ex(x,y,Charwidth,Font_Height,(uint8_t*)&zoomBuff,DrawModel);
              y+=Font_Height;
              ptr+=2;
          }
          else
          {
                  Charwidth = Font_width / 2;
                  Ascii = *ptr - 32;
                  //ʹ��16*24����������ģ����
                  ILI9341_zoomChar(16,24,Charwidth,Font_Height,(uint8_t *)&Font16x24.table[Ascii * Font16x24.Height*Font16x24.Width/8],psr,0);
                //��ʾ�����ַ�
                  ILI9341_DrawChar_Ex(x,y,Charwidth,Font_Height,(uint8_t*)&zoomBuff,DrawModel);
                  y+=Font_Height;
                  ptr++;
          }
      }
  }
  
  
  /**
    * @brief  ����Ӣ����������
    * @param  fonts: ָ��Ҫѡ�������
      *		����Ϊ����ֵ֮һ
    * 	@arg��Font24x32;
    * 	@arg��Font16x24;
    * 	@arg��Font8x16;
    * @retval None
    */
  void LCD_SetFont(sFONT *fonts)
  {
    LCD_Currentfonts = fonts;
  }
  
  /**
    * @brief  ��ȡ��ǰ��������
    * @param  None.
    * @retval ���ص�ǰ��������
    */
  sFONT *LCD_GetFont(void)
  {
    return LCD_Currentfonts;
  }
  
  
  /**
    * @brief  ����LCD��ǰ��(����)��������ɫ,RGB565
    * @param  TextColor: ָ��ǰ��(����)��ɫ
    * @param  BackColor: ָ��������ɫ
    * @retval None
    */
  void LCD_SetColors(uint16_t TextColor, uint16_t BackColor) 
  {
    CurrentTextColor = TextColor; 
    CurrentBackColor = BackColor;
  }
  
  /**
    * @brief  ��ȡLCD��ǰ��(����)��������ɫ,RGB565
    * @param  TextColor: �����洢ǰ��(����)��ɫ��ָ�����
    * @param  BackColor: �����洢������ɫ��ָ�����
    * @retval None
    */
  void LCD_GetColors(uint16_t *TextColor, uint16_t *BackColor)
  {
    *TextColor = CurrentTextColor;
    *BackColor = CurrentBackColor;
  }
  
  /**
    * @brief  ����LCD��ǰ��(����)��ɫ,RGB565
    * @param  Color: ָ��ǰ��(����)��ɫ 
    * @retval None
    */
  void LCD_SetTextColor(uint16_t Color)
  {
    CurrentTextColor = Color;
  }
  
  /**
    * @brief  ����LCD�ı�����ɫ,RGB565
    * @param  Color: ָ��������ɫ 
    * @retval None
    */
  void LCD_SetBackColor(uint16_t Color)
  {
    CurrentBackColor = Color;
  }
  
  /**
    * @brief  ���ĳ������
    * @param  Line: ָ��Ҫɾ������
    *   ��������ʹ�ú�LINE(0)��LINE(1)�ȷ�ʽָ��Ҫɾ�����У�
    *   ��LINE(x)����ݵ�ǰѡ�������������Y����ֵ����ɾ����ǰ����߶ȵĵ�x�С�
    * @retval None
    */
  void LCD_ClearLine(uint16_t Line)
  {
    ILI9341_Clear(0,Line,LCD_X_LENGTH,((sFONT *)LCD_GetFont())->Height);	/* ��������ʾȫ�� */
  
  }
  /*********************end of file*************************/
  
  
  