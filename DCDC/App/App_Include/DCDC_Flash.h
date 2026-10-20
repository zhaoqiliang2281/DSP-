#ifndef DCDC_Flash_H_
#define DCDC_Flash_H_


#define FLASH_ADDR_CALPRA            0x00000000 // 校准参数K B保存地址

#define  CAL_PRA_OK_FLAG  64321

#define  CALPRA_STRUCT_ADDR         ((Uint8 *)(&CalPra))
#define  CALPRA_LENGTH              ((Uint32)(&CalPra.endaddr - CALPRA_STRUCT_ADDR))    // sizeof(CalPra)

  
/*此结构体定义了校准参数*/
//#pragma pack(1) // 方便用sizeof算结构体长度时，得到确切的结果，不会因为内存对齐导致不一样的结果
typedef  struct
{
    Uint16 vCalFlag;     // 有校准数据标志
    Uint16 iCalFlag;     // 有校准数据标志

    Uint16 V_Meter_H;
    Uint16 V_ADC_H;
    Uint16 V_Meter_L;
    Uint16 V_ADC_L;

    Uint16 I_Meter_H;
    Uint16 I_ADC_H;
    Uint16 I_Meter_L;
    Uint16 I_ADC_L;

    float floatVK;
    float floatVB;
    float floatIK;
    float floatIB;

    Uint16 endFlag;     // 有校准数据标志
    Uint16 endaddr;     // 有校准数据标志


}CalPra_Struct;
//#pragma pack()

extern CalPra_Struct CalPra;


#define DUMMYDATA 0xa5

//LT采用GPIO41
#define EECSGPIO GPIO44
#define EECSMUX GpioCtrlRegs.GPBMUX1.bit.EECSGPIO
#define EECSDIR GpioCtrlRegs.GPBDIR.bit.EECSGPIO
#define CE_High()  GpioDataRegs.GPBSET.bit.EECSGPIO =1
#define CE_Low() GpioDataRegs.GPBCLEAR.bit.EECSGPIO =1


#define FLASH_WP_1  GpioDataRegs.GPASET.bit.GPIO8 = 1
#define FLASH_WP_0  GpioDataRegs.GPACLEAR.bit.GPIO8 = 1


/* Function Prototypes */

#define WREN_IST  0x06
#define CHIP_ERASE_IST 0x60
#define READ_STATUS_REG_IST 0x05 //P15
#define PAGE_PRG_IST 0x02 //P18
#define READ_DATA_IST 0x03 //23
#define WRITE_SREG_IST 0X01
#define SECTOR_ERASE_IST 0x20

#define SPIFLASH_ID 0x64  //通信中，flash的ID
#define SECTOR_SIZE 0x1000

#define FLASH_ID_I 0
#define FLASH_CMD_I 1
#define FLASH_ADDRH_I 2
#define FLASH_ADDRM_I 3
#define FLASH_ADDRL_I 4
#define FLASH_DATALENH_I 5
#define FLASH_DATALENM_I 6
#define FLASH_DATALENL_I 7
#define FLASH_CY_DATA_I 8


void Flash_Task_100ms(void);

void InitFlashCalPra(void);
void ReflashCalPra(void);
Uint8 CheckCalPra(void);

void ReadFlashCalPra(void);
void WriteFlashCalPra(void);

void ReadFlash(Uint32 addr,Uint8 *buff,Uint32 len);
Uint8 WriteFlash(Uint32 addr,Uint8 *buff,Uint32 len);

void Flash_PeripheralInit(void);
void SPI_IOinit();
void spi_init();
Uint8 Send_Byte(Uint16 a);
Uint16 Get_Byte(void);
void spi_fifo_init();
void WrSReg(Uint16 setReg);
void PageProgram(unsigned long Dst, unsigned char *byte,unsigned char len);
void ReadData(unsigned long Dst, unsigned char *Rxbuf,unsigned long len);
Uint16 Read_Status_2Reg(void);
unsigned char Read_Status_Register(void);
void WREN(void);
void EBSY(void);
void Sector_Erase(unsigned long Dst);
void Chip_Erase(void);
void Wait_Busy(void);








#endif

















