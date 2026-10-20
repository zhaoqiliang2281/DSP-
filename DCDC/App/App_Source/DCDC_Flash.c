 


#include "DCDC_GlobalHeadFile.h"


CalPra_Struct CalPra;

/****************************************************************************
 * @brief： FLASH更新任务判断
 * @param： 无
 * @retval：无
 * @note：
 *
 ****************************************************************************/
void Flash_Task_100ms(void)
{
    if(canSetPra.needReflashCalPraFlag==1)
    {
        canSetPra.needReflashCalPraFlag=0;
        ReflashCalPra();  // 计算校准参数 / 保存flash / 传入cla
    }
}


/****************************************************************************
 * @brief： 判断是否需要写FLASH默认校准参数 / 读出校准参数
 * @param： 无
 * @retval：无
 * @note：
 *
 ****************************************************************************/
void InitFlashCalPra(void)
{
    ReadFlashCalPra(); // 读出校准参数

    if((CalPra.endFlag!=CAL_PRA_OK_FLAG)||(CheckCalPra()==0))
    {
        // 默认值如下：
        CalPra.V_Meter_H=49999;  // 135000对4096   CalPra.V_Meter / V_ADC = 33倍上下
        CalPra.V_ADC_H=1517;     // 4096
        CalPra.V_Meter_L=2999;
        CalPra.V_ADC_L=91;

        CalPra.floatVK=32.959326;
        CalPra.floatVB=0;

        // 默认值如下：
        CalPra.I_Meter_H=50009;  // 125A 对 4096   CalPra.I_Meter / (I_ADC-2048) = 60.918倍
        CalPra.I_ADC_H=2869;     //
        CalPra.I_Meter_L=2985;
        CalPra.I_ADC_L=2097;

        CalPra.floatIK=60.911917;
        CalPra.floatIB=-124747.2;

        CalPra.endFlag=CAL_PRA_OK_FLAG;     // 有校准数据标志

        WriteFlashCalPra(); // 写校准参数
    }

    // 传入cla计算
    g_strClaIn_AdcRawData.floatVK = CalPra.floatVK;
    g_strClaIn_AdcRawData.floatVB = CalPra.floatVB;
    g_strClaIn_AdcRawData.floatIK = CalPra.floatIK;
    g_strClaIn_AdcRawData.floatIB = CalPra.floatIB;
}


/****************************************************************************
 * @brief： 计算校准参数 / 保存flash / 传入cla
 * @param： 无
 * @retval：无
 * @note：
 *
 ****************************************************************************/
void ReflashCalPra(void)
{
    //检查校准参数
    if(CheckCalPra()==1)
    {
        // 计算校准参数：
        CalPra.floatVK = ((float)CalPra.V_Meter_H - (float)CalPra.V_Meter_L) / ((float)CalPra.V_ADC_H - (float)CalPra.V_ADC_L);
        CalPra.floatVB = (float)CalPra.V_Meter_H - (float)CalPra.V_ADC_H * CalPra.floatVK;

        CalPra.floatIK = ((float)CalPra.I_Meter_H - (float)CalPra.I_Meter_L) / ((float)CalPra.I_ADC_H - (float)CalPra.I_ADC_L);
        CalPra.floatIB = (float)CalPra.I_Meter_H - (float)CalPra.I_ADC_H * CalPra.floatIK;


        WriteFlashCalPra(); // 写校准参数


        // 传入cla计算
        g_strClaIn_AdcRawData.floatVK = CalPra.floatVK;
        g_strClaIn_AdcRawData.floatVB = CalPra.floatVB;
        g_strClaIn_AdcRawData.floatIK = CalPra.floatIK;
        g_strClaIn_AdcRawData.floatIB = CalPra.floatIB;

    }
    else
    {
        // 下发参数有错时，重新读出校准参数，以更新CalPra结构体
        ReadFlashCalPra(); // 读出校准参数
    }
}


/****************************************************************************
 * @brief： 检查校准参数
 * @param： 无
 * @retval：0：错误  1：正常
 * @note：
 *
 ****************************************************************************/
Uint8 CheckCalPra(void)
{
    Uint16 temp;
    Uint16 temp1;

    // 校准ADC值不能为0V，后面除法会出错
//    if((CalPra.V_ADC_H==0)||(CalPra.V_ADC_L==0)||(CalPra.I_ADC_H==2048)||(CalPra.I_ADC_L==2048))
    if((CalPra.V_ADC_H==0)||(CalPra.V_ADC_L==0))
        return 0;

    temp = CalPra.V_Meter_H / CalPra.V_ADC_H;  // 135000对4096   CalPra.V_Meter / V_ADC = 33倍上下
    if((temp<20)||(temp>50))
        return 0;

    temp = CalPra.V_Meter_L / CalPra.V_ADC_L;  // 135000对4096   CalPra.V_Meter / V_ADC = 33倍上下
    if((temp<20)||(temp>50))
        return 0;

//    temp1=CalPra.I_ADC_H-2048;
//    if(temp1==0)
//        temp1=1;
//    temp = CalPra.I_Meter_H / temp1;  // 125A 对 4096    CalPra.I_Meter / (I_ADC-2048) = 60.918倍
//    if((temp<20)||(temp>100))
//        return 0;
//
//    temp1=CalPra.I_ADC_L-2048;
//    if(temp1==0)
//        temp1=1;
//    temp = CalPra.I_Meter_L / temp1;  // 125A 对 4096    CalPra.I_Meter / (I_ADC-2048) = 60.918倍
//    if((temp<20)||(temp>100))
//        return 0;

    return 1;
}

/****************************************************************************
 * @brief： 写FLASH校准数据
 * @param： 无
 * @retval：无
 * @note：
 *
 ****************************************************************************/
void ReadFlashCalPra(void)
{
    DisableDog();
    ReadFlash(FLASH_ADDR_CALPRA,CALPRA_STRUCT_ADDR,CALPRA_LENGTH);
    ServiceDog();  // 重置看门狗定时器
    EnableDog();
}

/****************************************************************************
 * @brief： 读FLASH中的校准数据
 * @param： 无
 * @retval：无
 * @note：
 *
 ****************************************************************************/
void WriteFlashCalPra(void)
{
    DisableDog();

    /* 擦除芯片 */
//    printf("\r\n\r\n请不要发频繁擦除芯片；正开始擦除整个芯片.....稍等片刻");
    WREN();
//    Chip_Erase();  // 这个要19S
    Sector_Erase(FLASH_ADDR_CALPRA);  // 这个很快，可能在200ms以内
    Wait_Busy();
//    printf("\r\n芯片擦除完毕");

    WriteFlash(FLASH_ADDR_CALPRA,CALPRA_STRUCT_ADDR,CALPRA_LENGTH);

    ServiceDog();  // 重置看门狗定时器
    EnableDog();
}


/****************************************************************************
 * @brief： 读flash
 * @param： addr：地址
 *          *buff：保存缓冲区，u8类型
 *          len：读取长度，字节数，应是4的倍数
 * @retval：无
 * @note：
 *
 ****************************************************************************/
void ReadFlash(Uint32 addr,Uint8 *buff,Uint32 len)
{
    ReadData(addr,buff,len);
}

/****************************************************************************
 * @brief： 写flash
 * @param： addr：地址
 *          *buff：保存缓冲区，u8类型
 *          len：读取长度，字节数，应是4的倍数
 * @retval：无
 * @note：
 *
 ****************************************************************************/
Uint8 WriteFlash(Uint32 addr,Uint8 *buff,Uint32 len)
{
    WREN();
    PageProgram(addr,buff,len);
    Wait_Busy();

    return 1;
}


/****************************************************************************
 * @brief： FLASH初始化
 * @param： 无
 * @retval：无
 * @note：
 *
 ****************************************************************************/
void Flash_PeripheralInit(void)
{
    SPI_IOinit();
    spi_fifo_init(); // Initialize the Spi FIFO
    spi_init();      // init SPI
}


void SPI_IOinit()
{
EALLOW;
    EECSMUX = 0;
    EECSDIR = 1;

    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 0;   // Enable pull-up on GPIO16 (SPISIMOA)
    GpioCtrlRegs.GPAPUD.bit.GPIO17 = 0;   // Enable pull-up on GPIO17 (SPISOMIA)
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0;   // Enable pull-up on GPIO18 (SPICLKA)
    GpioCtrlRegs.GPAPUD.bit.GPIO19 = 0;   // Enable pull-up on GPIO19 (SPISTEA)

    GpioCtrlRegs.GPAQSEL2.bit.GPIO16 = 3; // Asynch input GPIO16 (SPISIMOA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3; // Asynch input GPIO17 (SPISOMIA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO18 = 3; // Asynch input GPIO18 (SPICLKA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO19 = 3; // Asynch input GPIO19 (SPISTEA)

    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 1; // Configure GPIO16 as SPISIMOA
    GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 1; // Configure GPIO17 as SPISOMIA
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1; // Configure GPIO18 as SPICLKA
    GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 1; // Configure GPIO19 as SPISTEA

    // WP
    GpioCtrlRegs.GPAPUD.bit.GPIO8 = 1;      // Disable pull-up on GPIO3 (GPIO3)
    GpioDataRegs.GPACLEAR.bit.GPIO8 = 1; // Set GPIO3 lower
    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0;   // Configure GPIO3 as GPIO3
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 1;      // GPIO3 = output,0-Input(default),1-Output
    // 这里直接WP允许写
    FLASH_WP_1;

EDIS;
}

//CLOCK PHASE = 0:
//CLOCK POLARITY  = 0

void spi_init()
{
    CE_High();

    SpiaRegs.SPICCR.bit.SPISWRESET = 0; //P25  SW Reset on
    SpiaRegs.SPICCR.bit.SPICHAR = 7;    //p26   8bit
    SpiaRegs.SPICCR.bit.CLKPOLARITY = 0 ; //P25 When no SPI data is sent, SPICLK is at low
    SpiaRegs.SPICTL.bit.CLK_PHASE = 1;   //P25   Data is output on the rising edge of the SPICLK signal;
                                         //      input data is latched on the falling edge of the SPICLK signal.
    SpiaRegs.SPICTL.bit.MASTER_SLAVE  =1;//P26   SPI configured as a master.

    SpiaRegs.SPIBRR =3;             // For SPIBRR = 3 to 127;  SPI Baud Rate = LSPCLK/(SPIBRR+1)

    SpiaRegs.SPICCR.bit.SPISWRESET = 1; //P25  SW Reset off

    SpiaRegs.SPIPRI.bit.FREE = 1;   // Set so breakpoints don't disturb xmission
    SpiaRegs.SPIPRI.bit.STEINV = 0; //P34 SPISTE is active low (normal)
}

Uint8 Send_Byte(Uint16 a)
{
    Uint16 rdata;

    SpiaRegs.SPICTL.bit.TALK  =1;                //P23 Enable Transmit path
    SpiaRegs.SPITXBUF=(a<<8)&0xff00;
    while(SpiaRegs.SPIFFRX.bit.RXFFST == 0) { }     // Wait until data is received
//  while(SpiaRegs.SPISTS.bit.INT_FLAG == 0) {} // Waits until data rx’d
    rdata = SpiaRegs.SPIRXBUF;
    return rdata;
}

Uint16 Get_Byte(void)
{
    Uint8 rdata;
    SpiaRegs.SPICTL.bit.TALK = 0; // Disable Transmit pat
    SpiaRegs.SPITXBUF = DUMMYDATA; // Send dummy to start
    // NOTE: because TALK = 0, data does not tx onto SPISIMOA pin
    //while(SpiaRegs.SPISTS.bit.INT_FLAG !=1) {} // Wait until data rece
    while(SpiaRegs.SPIFFRX.bit.RXFFST ==0) { }  // Wait until data is received
    rdata = SpiaRegs.SPIRXBUF; // Master reads data
    return  (rdata&0x00ff); // return  rdata // BO$$
}

void spi_fifo_init()
{
// Initialize SPI FIFO registers
//    SpiaRegs.SPIFFTX.all=0xE040;
     SpiaRegs.SPIFFTX.bit.SPIRST =1;  //P31 SPI FIFO can resume transmit or receive.
     SpiaRegs.SPIFFTX.bit.SPIFFENA =1;//SPI FIFO enhancements are enabled
     SpiaRegs.SPIFFTX.bit.TXFFST = 1; //Re-enable Transmit FIFO operation
     SpiaRegs.SPIFFTX.bit.TXFFINTCLR = 1;//Write 1 to clear TXFFINT flag in bit 7.
     SpiaRegs.SPIFFTX.bit.TXFFIENA = 0; //不用中断


    //SpiaRegs.SPIFFRX.all=0x2044;
     SpiaRegs.SPIFFRX.bit.RXFFOVFCLR = 1;
     SpiaRegs.SPIFFRX.bit.RXFFINTCLR  =1;
     SpiaRegs.SPIFFRX.bit.RXFFST = 1;
     SpiaRegs.SPIFFRX.bit.RXFFIENA = 0; //不用FIFO接收中断
     SpiaRegs.SPIFFRX.bit.RXFFIL = 4;

    SpiaRegs.SPIFFCT.all=0x0;  //P32
}

/************************************************************************/
//P15/P22
//写入状态寄存器
//先写入低8位，后写入高2位
/************************************************************************/
void WrSReg(Uint16 setReg)
{
    CE_Low();
    Send_Byte(WRITE_SREG_IST);
    Send_Byte((setReg&0x00ff));
    Send_Byte((setReg&0x0300)>>8);
    CE_High();
}

/************************************************************************/
//P33
//写入A23--A0
//指令 0x02
//1到256 byte写入
//最后一个byte要写入 0
/************************************************************************/
void PageProgram(unsigned long Dst, unsigned char *byte,unsigned char len)
{
    unsigned char i;
    CE_Low();                   /* enable device */
    Send_Byte(PAGE_PRG_IST);            /* send Byte Program command */
    Send_Byte(((Dst & 0xFFFFFF) >> 16));/* send 3 address bytes */
    Send_Byte(((Dst & 0xFFFF) >> 8));
    Send_Byte(Dst & 0xFF);
    for(i=0;i<len;i++)
    {
        //判断忙标志，再进行写操作
        //printf(" %x",(*byte)&0xff);
//      Send_Byte(*(byte++));

        Send_Byte(*byte>>8);    /* 1次写2个字节 */
        Send_Byte(*byte);
        byte++;
    }
    CE_High();              /* disable device */
}


/************************************************************************/
//P23
//指令 0x03
//地址 A23--A0
/************************************************************************/
void ReadData(unsigned long Dst, unsigned char *Rxbuf,unsigned long len)
{
    unsigned long i = 0;
    CE_Low();               /* enable device */
    Send_Byte(READ_DATA_IST);           /* read command */
    Send_Byte(((Dst & 0xFFFFFF) >> 16));    /* send 3 address bytes */
    Send_Byte(((Dst & 0xFFFF) >> 8));
    Send_Byte(Dst & 0xFF);
    for (i = 0; i < len; i++)       /* read until no_bytes is reached */
    {
//      *(Rxbuf++) = Get_Byte();    /* receive byte and store at address 80H - FFH */
//      printf(" 0x%x",(*(Rxbuf-1))&0xff);//调试预留函数，可取消该句

        *(Rxbuf) = Get_Byte()<<8;    /* 1次读2个字节 */
        *(Rxbuf++) +=  Get_Byte();
//        printf("# 0x%x",(*(Rxbuf-1))&0xff);//调试预留函数，可取消该句
//        printf(" 0x%x",(*(Rxbuf-1))&0xff);//调试预留函数，可取消该句

    }
    CE_High();              /* disable device */
}


/************************************************************************/
//P23
//指令 0x03
//地址 A23--A0
/************************************************************************/
Uint16 Read_Status_2Reg(void)
{
    Uint16 wbyte1,wbyte2;

    CE_Low();
    Send_Byte(READ_STATUS_REG_IST);
    wbyte1 = Get_Byte();
    CE_High();

    CE_Low();
    Send_Byte(0x35);        //READ_STATUS_REG_IST
    wbyte2 = Get_Byte();
    CE_High();

    wbyte2<<=6;
    wbyte2 &=0xff00;
    wbyte2 += wbyte1&0xff;
    CE_High();
    return wbyte2;
}


/************************************************************************/
/* PROCEDURE: Read_Status_Register                  */
/*                                  */
/* This procedure read the status register and returns the byte.    */
/*                                  */
/* Input:                               */
/*      None                            */
/*                                  */
/* Returns:                             */
/*      byte                            */
/************************************************************************/
unsigned char Read_Status_Register(void)
{
    unsigned char byte = 0;
    CE_Low();           /* enable device */
    Send_Byte(READ_STATUS_REG_IST);     /* send RDSR command */
    byte = Get_Byte();      /* receive byte */
    CE_High();          /* disable device */
    return byte;
}


/************************************************************************/
/* PROCEDURE: WREN                          */
/*                                  */
/* This procedure enables the Write Enable Latch.  It can also be used  */
/* to Enables Write Status Register.                    */
/*                                  */
/* Input:                               */
/*      None                            */
/*                                  */
/* Returns:                             */
/*      Nothing                         */
/************************************************************************/
void WREN(void)
{
    CE_Low();           /* enable device */
    Send_Byte(WREN_IST);        /* send WREN command */
    CE_High();          /* disable device */
}


/************************************************************************/
/* PROCEDURE: EBSY                          */
/*                                  */
/* This procedure enable SO to output RY/BY# status during AAI      */
/* programming.                             */
/*                                  */
/* Input:                               */
/*      None                            */
/*                                  */
/* Returns:                             */
/*      Nothing                         */
/************************************************************************/
void EBSY(void)
{
    CE_Low();           /* enable device */
    Send_Byte(0x70);    /* send EBSY command */
    CE_High();          /* disable device */
}


/************************************************************************/
/* PROCEDURE: Sector_Erase                      */
/*                                  */
/* This procedure Sector Erases the Chip.               */
/*                                  */
/* Input:                               */
/*      Dst:        Destination Address 000000H - 3FFFFFH   */
/*                                  */
/* Returns:                             */
/*      Nothing                         */
/************************************************************************/
void Sector_Erase(unsigned long Dst)
{


    CE_Low();               /* enable device */
    Send_Byte(0x20);            /* send Sector Erase command */
    Send_Byte(((Dst & 0xFFFFFF) >> 16));    /* send 3 address bytes */
    Send_Byte(((Dst & 0xFFFF) >> 8));
    Send_Byte(Dst & 0xFF);
    CE_High();              /* disable device */
}



/************************************************************************/
/* PROCEDURE: Chip_Erase                        */
/*                                  */
/* This procedure erases the entire Chip.               */
/*                                  */
/* Input:                               */
/*      None                            */
/*                                  */
/* Returns:                             */
/*      Nothing                         */
/************************************************************************/
void Chip_Erase(void)
{
    CE_Low();               /* enable device */
    Send_Byte(CHIP_ERASE_IST);          /* send Chip Erase command (60h or C7h) */
    CE_High();              /* disable device */
}


/************************************************************************/
/* PROCEDURE: Wait_Busy                         */
/*                                  */
/* This procedure waits until device is no longer busy (can be used by  */
/* Byte-Program, Sector-Erase, Block-Erase, Chip-Erase).        */
/*                                  */
/* Input:                               */
/*      None                            */
/*                                  */
/* Returns:                             */
/*      Nothing                         */
/************************************************************************/
void Wait_Busy(void)
{
    Uint32 waitCnt=0;
    unsigned char Wbusy;
    Uint16 temp;

    waitCnt = 600000;
    Wbusy =Read_Status_Register(); //HK P15  Status_Register目录
    while ((Wbusy & 0x01) == 0x01)  /* waste time until not busy */
    {
        Wbusy =Read_Status_Register();

        if(waitCnt==0)
        {
            WREN();
            WrSReg(0x0000);//上点要设置状态，取消写保护才能写入
//            printf("\r\n-----!!!!!!!!!!!!!!!!-----------");
            temp = Read_Status_2Reg();
//            printf("Wait Flash busy,and reWrite REG is %x",SPIFlash.SReg&0x03ff);
            break;
        }

        else
            waitCnt--;
    }
}




