#include "jtag.h"

void wait(uint32_t us)
{
    while(us--);
}

void Power_Init(void)
{

}


/**
 * @brief  引脚初始化
 * @details 	
 * @author   	
 * @data    	20181016
 */
void JTAGPin_Init(void)
{
    RCC_APB2PeriphClockCmd(JTAG_PIN_RCC,ENABLE);
    GPIO_InitTypeDef GPIOStructureInit;
    GPIOStructureInit.GPIO_Pin = TARG_TMS_PIN;
    GPIOStructureInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOStructureInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(JTAG_PIN_PORT, &GPIOStructureInit);
    
    GPIOStructureInit.GPIO_Pin = TARG_TCK_PIN;
    GPIOStructureInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOStructureInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(JTAG_PIN_PORT, &GPIOStructureInit);
        
    GPIOStructureInit.GPIO_Pin = TARG_TDI_PIN;           
    GPIOStructureInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOStructureInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(JTAG_PIN_PORT, &GPIOStructureInit);
        
    GPIOStructureInit.GPIO_Pin = TARG_TDO_PIN;
    GPIOStructureInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(JTAG_PIN_PORT, &GPIOStructureInit);
    
    GPIOStructureInit.GPIO_Pin = TARG_RST_PIN;
    GPIOStructureInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOStructureInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(JTAG_PIN_PORT, &GPIOStructureInit);
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
    GPIOStructureInit.GPIO_Pin = GPIO_Pin_8;
    GPIOStructureInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOStructureInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIOStructureInit);
    GPIOStructureInit.GPIO_Pin = GPIO_Pin_9;
    GPIOStructureInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOStructureInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIOStructureInit);
    
    POWEREN();
    POWER3V3();
    wait(40000);
    TMS_PIN_H();
    TDI_PIN_H();
    RST_PIN_L();
}

/**
 * @brief  使能外部调试
 * @details 	
 * @author   	
 * @data    	20181018
 */
void Target_External_Debug(void)
{
    RCC_APB2PeriphClockCmd(JTAG_EN_RCC,ENABLE);
    GPIO_InitTypeDef GPIOStructureInit;
    GPIOStructureInit.GPIO_Pin = TARG_JTAG_EN_PIN;
    GPIOStructureInit.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOStructureInit.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(JTAG_EN_PORT, &GPIOStructureInit);
    GPIO_ResetBits(JTAG_EN_PORT,TARG_JTAG_EN_PIN);
}

/**
 * @brief  复位JTAG状态机,进入Test-Logic-Reset状态
 * @details 	
 * @author   	
 * @data    	20181016
 */
void JTAG_Reset(void)
{
    RST_PIN_L();
    wait(100);
    RST_PIN_H();
}

/**
 * @brief  进入Test-Logic-Reset状态
 * @details 	
 * @author   	
 * @data    	20181016
 */
void Test_Logic_Reset(void)
{
    TMS_PIN_H();
    for(int i = 0; i < 6; i++)
    {
        TCK_PIN_H();
        wait(10);
        TCK_PIN_L();
        wait(10);
    }
}

/* 进入Run-TestIDLE状态 */
void Run_TestIDLE(void)
{
    TCK_PIN_L();
    TMS_PIN_L();
    wait(1);
    TCK_PIN_H();
    wait(300);
}

/* 进入Select-DR-Scan状态 */
void Select_DR_Scan(void)
{
    //Run_TestIDLE();
    TMS_PIN_H();
    TCK_PIN_L();
    wait(1);
    TCK_PIN_H();
    wait(1);
}

void Capture_DR(void)
{
    Select_DR_Scan();
    TMS_PIN_L();
    TCK_PIN_L();
    wait(1);
    TCK_PIN_H();
    wait(1);
}

/**
 * @brief  发送任意位数数据，最大64位，从Shift-DR状态到Exit1-DR状态
 * @param  data:发送的指令数组地址；bitnum:指令位数；retvaule:存储目标板输出线数据
 * @author   	
 * @data    	20181016
 */
uint32_t ShiftDR_To_ExtiDR(uint32_t *data, uint8_t bitnum ,uint32_t *retvaule)
{
    uint32_t *temp = 0;
    int i = 0;
    uint32_t buf = 0;
    buf = *data;
    temp = retvaule;
    TCK_PIN_L();
    TMS_PIN_L(); 
    wait(1);
    TCK_PIN_H();
    wait(10);
     for(i = 0; i < bitnum-1; i++)
    {
        TCK_PIN_L();
        if(buf & 0x01)
        {
            TDI_PIN_H();
        }
        else
        {
            TDI_PIN_L();
        }
        buf >>= 1;
        wait(5);
        
        TCK_PIN_H();
        wait(5);
        if(Read_TargetTDOPin())
        {
            *temp |= (0x01 << i);
        }
        else
        {
            *temp &= ~(0x01 << i);
        }
        if((i<32)&&(i == 31))
        {
            buf = *(data+1);
            temp++;
        }
    }
    TCK_PIN_L();
    TMS_PIN_H();
    if(buf & 0x01)
    {
        TDI_PIN_H();
    }
    else
    {
        TDI_PIN_L();
    }
    if(Read_TargetTDOPin())
    {
        *temp |= (0x01 << i);
    }
    else
    {
        *temp &= ~(0x01 << i);
    }
    TCK_PIN_H();
    wait(20); 
    TDI_PIN_H();
    return buf;
}

/**
 * @brief  发送任意位数数据，最大64位，从Select-DR-Scan状态到Exit1-DR状态
 * @param  data:发送的指令数组地址；bitnum:指令位数；retvaule:存储目标板输出线数据
 * @author   	
 * @data    	20181016
 */
uint32_t SelectDR_Exit1DR_Anybit(uint32_t *data, uint8_t bitnum ,uint32_t *retvaule)
{
    uint32_t *temp = 0;
    int i = 0;
    uint32_t buf = 0;
    buf = *data;
    temp = retvaule;
    Capture_DR();
    TCK_PIN_L();
    wait(1);
    TCK_PIN_H();
    wait(10);
    for(i = 0; i < bitnum-1; i++)
    {
        TCK_PIN_L();
        if(buf & 0x01)
        {
            TDI_PIN_H();
        }
        else
        {
            TDI_PIN_L();
        }
        buf >>= 1;
        wait(5);
        
        TCK_PIN_H();
        wait(5);
        if(Read_TargetTDOPin())
        {
            *temp |= (0x01 << i);
        }
        else
        {
            *temp &= ~(0x01 << i);
        }
        if((i<32)&&(i == 31))
        {
            buf = *(data+1);
            temp++;
        }
    }
    TCK_PIN_L();
    TMS_PIN_H();
    if(buf & 0x01)
    {
        TDI_PIN_H();
    }
    else
    {
        TDI_PIN_L();
    }
    if(Read_TargetTDOPin())
    {
        *temp |= (0x01 << i);
    }
    else
    {
        *temp &= ~(0x01 << i);
    }
    TCK_PIN_H();
    wait(20); 
    TDI_PIN_H();
    return buf;
}

void Pause_DR(void)
{
    TCK_PIN_L();
    TMS_PIN_L();
    wait(10);
    TCK_PIN_H();
    //wait(300);
}

void Exit1Or2_DR(void)
{
    TCK_PIN_L();
    TMS_PIN_H();
    wait(10);
    TCK_PIN_H();
}

void Update_DR(void)
{
    TCK_PIN_L();
    TMS_PIN_H();
    wait(10);
    TCK_PIN_H();
}

    /* IR Part */
void Select_IR_Scan(void)
{
    Select_DR_Scan();
    TCK_PIN_L();
    wait(1);
    TCK_PIN_H();
    wait(1);
}

void Capture_IR(void)
{
    Select_IR_Scan();
    TMS_PIN_L();
    TCK_PIN_L();
    wait(1);
    TCK_PIN_H();
    wait(1);
}

/**
 * @brief  发送任意位数指令，最大64位，从Select-IR-Scan状态到Exit1-IR状态
 * @param  data:发送的指令数组地址；bitnum:指令位数；retvaule:存储目标TDO信号线数据
 * @author   	
 * @data    	20181016
 */
uint32_t SelectIR_Exit1IR_Anybit(uint32_t *data, uint8_t bitnum ,uint32_t *retvaule)
{
    uint32_t *temp = 0;
    int i = 0;
    uint32_t buf = 0;
    buf = *data;
    temp = retvaule;
    Capture_IR();
    TCK_PIN_L();
    wait(1);
    TCK_PIN_H();
    wait(10);
    for(i = 0; i < bitnum-1; i++)
    {
        TCK_PIN_L();
        if(buf & 0x01)
        {
            TDI_PIN_H();
        }
        else
        {
            TDI_PIN_L();
        }
        buf >>= 1;
        wait(5);
        
        TCK_PIN_H();
        wait(5);
        if(Read_TargetTDOPin())
        {
            *temp |= (0x01 << i);
        }
        else
        {
            *temp &= ~(0x01 << i);
        }
        if((i<32)&&(i == 31))
        {
            buf = *(data+1);
            temp++;
        }
    }
    TCK_PIN_L();
    TMS_PIN_H();
    if(buf & 0x01)
    {
        TDI_PIN_H();
    }
    else
    {
        TDI_PIN_L();
    }
    if(Read_TargetTDOPin())
    {
        *temp |= (0x01 << i);
    }
    else
    {
        *temp &= ~(0x01 << i);
    }
    TCK_PIN_H();
    wait(20); 
    TDI_PIN_H();
    return buf;
}

void Pause_IR(void)
{
    TCK_PIN_L();
    TMS_PIN_L();
    wait(10);
    TCK_PIN_H();
    wait(600);
}

void Exit1Or2_IR(void)
{
    TCK_PIN_L();
    TMS_PIN_H();
    wait(10);
    TCK_PIN_H();
}

void Update_IR(void)
{
    TCK_PIN_L();
    TMS_PIN_H();
    wait(10);
    TCK_PIN_H();
}

void PauseIR_To_UpdateIR(void)
{
    Pause_IR();
    Exit1Or2_IR();
    Update_IR();
}

/*
void Select_DR_Scan(void)
{
    TCK_PIN_L();
    TMS_PIN_H();
    wait(100);
    TCK_PIN_H();
}

void Select_IR_Scan(void)
{
    TCK_PIN_L();
    TMS_PIN_H();
    wait(100);
    TCK_PIN_H();
}

void Capture_IR(void)
{
    TCK_PIN_L();
    TMS_PIN_L();
    wait(100);
    TCK_PIN_H();
}
*/

void JTAG_StatusInit(void)
{
    /* 初始化JTAG引脚 */
    JTAGPin_Init();
     //Target_External_Debug();
    JTAG_Reset();
    Test_Logic_Reset();
}

uint32_t Read_JTAG_IDCODE(void)
{
    uint32_t tmp = 0;
    uint32_t order = 0x05;
    uint32_t buf = 0;
   
    Run_TestIDLE();
    SelectIR_Exit1IR_Anybit(&order,6,&buf);
    Update_IR();
    Run_TestIDLE();
    SelectDR_Exit1DR_Anybit(&tmp,32,&buf);
    Update_DR();
    return buf;
}

uint32_t Read_DeviceID(void)
{
    uint32_t tmp = 0;
    uint32_t order = 0x08;
    uint32_t buf = 0;
    JTAGPin_Init();
    //Target_External_Debug();
    JTAG_Reset();
    Test_Logic_Reset();
    Run_TestIDLE();
    SelectIR_Exit1IR_Anybit(&order,6,&buf);
    Update_IR();
    Run_TestIDLE();
    SelectDR_Exit1DR_Anybit(&tmp,32,&buf);
    Update_DR();
    return buf;
}

/**
 * @brief  ICEPick模块处于连接状态
 * @param  调试连接寄存器中的内容
 * @author     
 * @data    	20181019
 */
uint8_t ICEPick_Connect(uint32_t *ConnRegister)
{
    uint32_t order = CONNECT;
    uint32_t data = 0;
    uint32_t tmp = 0;
    
    Run_TestIDLE();
    SelectIR_Exit1IR_Anybit(&order,6,&tmp); //发送连接命令
    Update_IR();
    Run_TestIDLE();
    SelectDR_Exit1DR_Anybit(&data,8,&tmp);  //读连接寄存器值
    Update_DR();
    Run_TestIDLE();
    SelectDR_Exit1DR_Anybit(&data,8,&tmp);  //读连接寄存器值
    Update_DR();
    
    if(tmp == 0x09)         //如果已经是连接状态就不修改
    {
        *ConnRegister = tmp;
        return 0;
    }
    else
    {
        data = 0x89;
        Run_TestIDLE();
        SelectIR_Exit1IR_Anybit(&order,6,&tmp); //发送连接命令
        Update_IR();
        Run_TestIDLE();
        SelectDR_Exit1DR_Anybit(&data,8,&tmp);  //写连接寄存器的值
        Update_DR();
    }
    *ConnRegister = tmp;
    return 0;
}

/**
 * @brief  检查连接寄存器是否已连接
 * @param  
 * @author     
 * @retrun DEVICE_DISCONNECTED:失败，0:成功
 * @data    	20181019
 */
uint32_t Check_Is_Conncet(void)
{
    uint32_t ctr = 0;
    ICEPick_Connect(&ctr);      //检查连接寄存器是否已连接
    if(ctr != 0x09)
    {
        return DEVICE_DISCONNECTED;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief  读JTAG寄存器
 * @param  addr：寄存器地址   RegVal：读出的寄存器值，可能会读多个寄存器，传入存放的数组地址   regnum:要读取寄存器的个数 
 * @author     
 * @retrun 1:失败，0:成功
 * @data    	20181019
 */
uint8_t Read_JTAG_Register(uint32_t *addr, uint32_t *RdOut, uint8_t regnum)
{
    uint32_t order = ROUTER;
    uint32_t data = 0;
#if 0
    if(Check_Is_Conncet() != 0) 
    {
        return 1;       //JTAG寄存器未连接
    }
#endif
    Run_TestIDLE();
    SelectIR_Exit1IR_Anybit(&order,6,RdOut);
    Update_IR();
    for(int i = 0; i < regnum; i++)
    {
        data = ((*(addr+i))|(0x00<<7)) << 24;
        Run_TestIDLE();
        SelectDR_Exit1DR_Anybit(&data,32,RdOut+i); 
        Update_DR();
        Run_TestIDLE();
        SelectDR_Exit1DR_Anybit(&data,32,RdOut+i); 
        Update_DR();
    }
    return 0;
}

/**
 * @brief  写JTAG寄存器
 * @param  addr：寄存器地址   WriteIn:要写入寄存器的值,可以是多个值，传入数组地址  regnum:要写的寄存器个数 
 * @author     
 * @retrun 1:失败，0:成功
 * @data    	20181019
 */
uint8_t Write_JTAG_Register(uint32_t *addr, uint32_t *WriteIn, uint8_t regnum)
{
    uint32_t order = ROUTER;
    uint32_t buf = 0;
    uint32_t data = 0;
#if 0
    if(Check_Is_Conncet() != 0) 
    {
        return 1;       //JTAG寄存器未连接
    }
#endif
    Run_TestIDLE();
    SelectIR_Exit1IR_Anybit(&order,6,&buf);
    Update_IR();
    for(int i = 0; i < regnum; i++)
    {
        data = (((*(addr+i))|(0x01<<7)) << 24) | (*(WriteIn+i));
        Run_TestIDLE();
        SelectDR_Exit1DR_Anybit(&data,32,&buf); 
        Update_DR();
        Run_TestIDLE();
        SelectDR_Exit1DR_Anybit(&data,32,&buf); 
        Update_DR();
    }
    return 0;
}

/**
 * @brief  配置System Control Register
 * @param  value：要写入寄存器的值
 * @author     
 * @retrun 1:失败，0:成功
 * @data    	20181019
 */
uint32_t Config_SYSCYL_Register(uint32_t value)
{
    uint32_t sysctl = SYS_CNTL;
    uint32_t readout = 0;
    uint32_t writein = value;
    Read_JTAG_Register(&sysctl,&readout,1);
    Write_JTAG_Register(&sysctl,&writein,1);
    Read_JTAG_Register(&sysctl,&readout,1);
    return 0;
}

/**
 * @brief  可以控制两块芯片
 * @param  
 * @author     
 * @retrun 1:失败，0:成功
 * @data    	20181026
 */
uint8_t TwoChip_Mode(void)
{
    uint32_t order[] = {0x02,0x3FA,0x3FE,0x3FB,0x3F};
    uint32_t data[] = {0xA0002008,0x20000000,0xA0002108,0x00,0xFFFFFFFF,0xF};
    uint32_t recv[10] = {0};
    
    
    Run_TestIDLE();
    SelectIR_Exit1IR_Anybit(&order[0],6,recv);
    PauseIR_To_UpdateIR();
    Capture_DR();
    Exit1Or2_DR();
    Pause_DR();
    Exit1Or2_DR();
    
    ShiftDR_To_ExtiDR(&data[1],32,recv);
    Pause_DR();
    Exit1Or2_DR();
    Update_DR();
    
    SelectDR_Exit1DR_Anybit(&data[1],32,recv);
    Pause_DR();
    Exit1Or2_DR();
    
    ShiftDR_To_ExtiDR(&data[0],32,recv);
    Pause_DR();
    Exit1Or2_DR();
    Update_DR();
    
    SelectDR_Exit1DR_Anybit(&data[0],32,recv);
    Pause_DR();
    Exit1Or2_DR();
    
    ShiftDR_To_ExtiDR(&data[1],32,recv);
    Pause_DR();
    Exit1Or2_DR();
    Update_DR();
        
    SelectDR_Exit1DR_Anybit(&data[1],32,recv);
    Pause_DR();
    Exit1Or2_DR();
    
    ShiftDR_To_ExtiDR(&data[2],32,recv);
    Pause_DR();
    Exit1Or2_DR();
    Update_DR();
    
    SelectDR_Exit1DR_Anybit(&data[3],32,recv);
    Pause_DR();
    Exit1Or2_DR();
    Update_DR();
    
    SelectIR_Exit1IR_Anybit(&order[4],6,recv);
    Update_IR();
    Run_TestIDLE();
    
    for(int i = 0; i < 6; i++)
    {
        TCK_PIN_L();
        wait(10);
        TCK_PIN_H();
        wait(10);
    }
    wait(100);
    SelectIR_Exit1IR_Anybit(&order[2],10,recv);
    Update_IR();
    if(((*recv) & 0x3FF) != 0x11)
    {
        return 1;       //由于其他原因不能控制两块芯片
    }
    Run_TestIDLE();
    SelectDR_Exit1DR_Anybit(&data[4],33,recv);
    Update_DR();
    return 0;
}


/**************TM4C129 FUNCTION*************************/

/**
 * @brief  TM4C129内部JTAG DP寄存器和AP寄存器格式，先发地位，[0]：0写，1读；[2:1]：DP或AP寄存器内部寄存器地址高2位；
+----------------------------------------------------------------------------------------------------------------------------------+
  36  |  35  |  34  |  33  |  32  |  31  |  30  |  29  |  28  |  27  |  26  |  25  |  24  |  23  |  22  |  21  |  19  |  18  |  17  |  16  |  15  |  14  |  13  |  12  |  11  |  10  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0
+----------------------------------------------------------------------------------------------------------------------------------+
忽略  |                                                             数   据                                                                                                                                                    |  地址高位 | 读/写                                                                                                         
 
 * @param  data:发送的数据数组地址；bitnum:数据位数；retvaule:存储目标板输出线数据  RdWr:0写，1读  DP_AP_addr：DP或AP地址
 * @author   	
 * @data    	20181016
 */
uint32_t SlectDR_ExitDR_36bit(uint32_t *data, uint8_t bitnum ,uint32_t *retvaule, uint8_t RdWr, uint8_t DP_AP_addr)
{
    uint32_t *temp = 0;
    uint32_t pairse = 0;
    int i = 0;
    uint32_t buf = 0;
    buf = *data;
    temp = retvaule;
    Capture_DR();
    TCK_PIN_L();
    wait(1);
    TCK_PIN_H();
    wait(10);
    
    pairse = RdWr ^ (DP_AP_addr & 0x01) ^ ((DP_AP_addr>>1) & 0x01);
    
    if(RdWr == 0)       //读写
    {
        TCK_PIN_L();
        TDI_PIN_L();
        wait(5);
        TCK_PIN_H();
        wait(5);
    }
    else
    {
        TCK_PIN_L();
        TDI_PIN_H();
        wait(5);
        TCK_PIN_H();
        wait(5);
    }

    for(int j = 0; j < 2; j++)      //地址
    {
        TCK_PIN_L();
        wait(5);
        if(DP_AP_addr & (0x01 << j))
        {
            TDI_PIN_H();
        }
        else
        {
            TDI_PIN_L();
        }
        TCK_PIN_H();
        wait(5);
    }
    
    for(i = 0; i < bitnum; i++)       //32位数据
    {
        TCK_PIN_L();
        if(buf & 0x01)
        {
            pairse = pairse ^ 0x01;
            TDI_PIN_H();
        }
        else
        {
            pairse = pairse ^ 0x00;
            TDI_PIN_L();        
        }
        buf >>= 1;
        wait(5);
        
        TCK_PIN_H();
        wait(5);
        if(Read_TargetTDOPin())
        {
            *temp |= (0x01 << i);
        }
        else
        {
            *temp &= ~(0x01 << i);
        }
        if((i<32)&&(i == 31))
        {
            buf = *(data+1);
            temp++;
        }
    }
    TCK_PIN_L();
    TMS_PIN_H();
    if(pairse)
    {
        TDI_PIN_H();
    }
    else
    {
        TDI_PIN_L();
    }
    TCK_PIN_H();
    return buf;
}

 
 /**
 * @brief  发送两芯片的JTAG指令
 * @param  No1Chip：TMS570LS1224的指令  No2Chip：TM4C129的指令
 * @author     
 * @retrun 1:失败，0:成功
 * @data    	20181026
 */
uint32_t jtagDAP_Command(uint8_t No1Chip, uint8_t No2Chip)
{
    uint32_t order = (No1Chip<<4)|No2Chip;
    uint32_t TDO_data = 0;
    Run_TestIDLE();
    SelectIR_Exit1IR_Anybit(&order,10,&TDO_data);
    Update_IR();
    if(((TDO_data) & 0x3FF) != 0x11)
    {
        return 1;      
    }
    return 0;
}

/**
 * @brief  读取JTAG_DP或者JTAG_AP寄存器
 * @param  data：读取到的值   DP_addr：寄存器里的寄存器地址  count:连续读取寄存器的数量
 * @author     
 * @retrun 1:失败，0:成功
 * @data    	20181026
 */
uint32_t Read_jtagDAP_Register(uint32_t *data, uint32_t DP_addr)
{
    uint32_t writein = 0x00000000;
    uint32_t TDO_data = 0;

    Run_TestIDLE();
    SlectDR_ExitDR_36bit(&writein,32,data,READ_DP, DP_addr);
    Update_DR();
    
    Run_TestIDLE();
    SlectDR_ExitDR_36bit(&writein,32,data,READ_DP, DP_addr);
    Update_DR();
    return 0;
}

/**
 * @brief  写入TAG_DP或者JTAG_AP寄存器
 * @param  Choose_Reg：选择操作的寄存器是JTAG-DP还是JTAG-AP(SLA_DPACC,SLA_APACC)  data：写入的值   DP_addr：DP寄存器里的寄存器地址  count:连续读取寄存器的数量
 * @author     
 * @retrun 1:失败，0:成功
 * @data    	20181026
 */
uint32_t Write_jtagDAP_Register(uint8_t Choose_Reg, uint32_t *data, uint32_t DP_addr)
{
    uint32_t TDO_data = 0;
    
    Run_TestIDLE();
    SlectDR_ExitDR_36bit(data,32,&TDO_data,0, DP_addr);
    Update_DR();
    return 0;
}

/******************************************************/

uint8_t test_read(void)
{
    uint32_t readout = 0;
    uint32_t writein[] = {0x01000000,0x80000052,0x80001314};
    Config_SYSCYL_Register(0x81021000);
    TwoChip_Mode();

    jtagDAP_Command(BYPASS,SLA_DPACC);
    Write_jtagDAP_Register(SLA_DPACC,&writein[0],SELECT_AP);
    
    jtagDAP_Command(BYPASS,SLA_APACC);
    Write_jtagDAP_Register(SLA_APACC,&writein[1],0x00);
    Write_jtagDAP_Register(SLA_APACC,&writein[2],0x01);
    return 0;
}

